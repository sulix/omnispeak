/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2018 Omnispeak Authors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "id_us.h"
#include "id_vl.h"
#include "id_vl_private.h"
#include "id_sd.h"
#include "ck_cross.h"
// For "Fix Jerky Motion"
#include "ck_play.h"

#include <stdlib.h>
#include <string.h>

#include <dos.h>
#include <dpmi.h>
#include <pc.h>
#include <sys/movedata.h>
#include <sys/nearptr.h>

static int vl_dos_screenWidth;
static int vl_dos_screenHeight;

#define EGA_ATR_INDEX 0x3C0
#define EGA_ATR_MODE 0x10
#define EGA_ATR_PELPAN 19
#define EGA_SC_INDEX 0x3C4
#define EGA_SC_MAP_MASK 0x02
#define EGA_GC_INDEX 0x3CE
#define EGA_GC_READMAP 0x04
#define EGA_GC_MODE 0x05
#define EGA_CRTC_INDEX 0x3D4
#define EGA_CRTC_DATA 0x3D5
#define EGA_CRTC_OFFSET 0x13
#define EGA_CRTC_START_ADDR_HIGH 0x0C
#define EGA_CRTC_START_ADDR_LOW 0x0D

#define VL_DOS_MAXSPRSHIFTS 4

typedef struct VL_DOS_Surface
{
	VL_SurfaceUsage use;
	int w, h;
	volatile void *data;
	volatile void *data2;
	int activePage;
} VL_DOS_Surface;

static int vl_dos_palSegment;
static int vl_dos_palSelector;

static void VL_DOS_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		vl_dos_screenWidth = VL_EGAVGA_GFX_WIDTH;
		vl_dos_screenHeight = VL_EGAVGA_GFX_HEIGHT;
		__dpmi_regs r;
		r.x.ax = 0x0D;
		__dpmi_int(0x10, &r);

		// We need to allocate some memory for our palette, which needs
		// to be accessible from the BIOS. Because the DOS allocation
		// functions take the memory size in paragraphs (16-byte chunks),
		// and we need 17 bytes (16 colours + the border colour), we
		// allocate two paragraphs.
		vl_dos_palSegment = __dpmi_allocate_dos_memory(2, &vl_dos_palSelector);
	}
	else
	{
		__dpmi_regs r;
		r.x.ax = 0x03;
		__dpmi_int(0x10, &r);

		// Free our palette memory.
		__dpmi_free_dos_memory(vl_dos_palSelector);
	}
}

void *vl_dos_realScreen = 0;
intptr_t vl_dos_vmemAlloc = 0xA0000;
// TODO: Correct, detect, and allow configuration of this.
intptr_t vl_dos_maxvmemPtr = 0xB0000;

static void VL_DOS_SetEGAWriteMode(int write_mode)
{
	outportw(EGA_GC_INDEX, (write_mode << 8) | EGA_GC_MODE);
}

static uint8_t *VL_DOS_GetSurfacePlanePointer(VL_DOS_Surface *surf, int plane)
{
	uint8_t *base_ptr = (uint8_t *)(surf->activePage ? surf->data2 : surf->data);
	size_t plane_length = surf->h * (surf->w / 8);
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		// Set the plane.
		uint16_t plane_val = (0x0100 << plane);
		outportw(EGA_SC_INDEX, plane_val | EGA_SC_MAP_MASK);
		outportw(EGA_GC_INDEX, (plane << 8) | EGA_GC_READMAP);
		// Add the plane length to switch to the active page.
		return base_ptr;
	}
	else
		return base_ptr + plane_length * plane;
}

// The screen will scroll a maximum of 16 pixels (1 tile) each frame
#define VL_DOS_MAX_SCROLL_HORZ 16
#define VL_DOS_MAX_SCROLL_VERT 16

static void VL_DOS_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_DOS_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)malloc(sizeof(VL_DOS_Surface));
	surf->use = usage;
	surf->w = w;
	surf->h = h;
	surf->activePage = 0;
	if (usage == VL_SurfaceUsage_FrontBuffer)
	{
		if (__djgpp_nearptr_enable())
		{
			size_t bufferSize = w / 8 * h ;
			surf->data = (void *)(__djgpp_conventional_base + vl_dos_vmemAlloc);
			surf->data2 = (void *)(__djgpp_conventional_base + vl_dos_vmemAlloc + bufferSize);
		}
		else
		{
			Quit("Couldn't enable djgpp nearptr (extended data segment size)");
		}
	}
	else
		surf->data = malloc(w * h / 2);
	return surf;
}

static void VL_DOS_DestroySurface(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	if ((surf->use != VL_SurfaceUsage_FrontBuffer) && surf->data)
		free((void*)surf->data);
	free(surf);
}

static long VL_DOS_GetSurfaceMemUse(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	return (surf->w * surf->h / 8) * 4;
}

static void VL_DOS_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_DOS_RefreshPaletteAndBorderColor(void *screen)
{
	// This is not really the right way to access this memory: we're given
	// a selector intended for accessing it from protected mode. But this
	// works (at least in DosBox).
	intptr_t dospalmem = vl_dos_palSegment * 16;
	uint8_t *dospal = (uint8_t *)(__djgpp_conventional_base + dospalmem);

	for (int i = 0; i < 16; i++)
	{
		// We need to convert back from an EGA index (IRGB), to a
		// six-bit DAC value (xIxRGB) where the intensity value is
		// stored in the green-intensity bit.
		uint8_t val = vl_emuegavgaadapter.palette[i];
		uint8_t realval = (val & 7) | ((val & 8) << 1);
		dospal[i] = realval;
	}
	dospal[16] = (vl_border_color & 7) | ((vl_border_color & 8) << 1);

	// Use int 10h, AX=1002h to set the palette.
	// http://www.ctyme.com/intr/rb-0116.htm
	__dpmi_regs r;
	r.x.ax = 0x1002;
	r.x.es = vl_dos_palSegment;
	r.x.dx = 0;
	__dpmi_int(0x10, &r);
}

static int VL_DOS_SurfacePGet(void *surface, int x, int y)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	int pixel_index_byte = (y * surf->w + x) / 8;
	int pixel_index_bit = 7 - ((y * surf->w + x) % 8);
	uint8_t val = 0;
	for (int plane = 0; plane < 4; ++plane)
	{
		uint8_t *px_plane_byte = VL_DOS_GetSurfacePlanePointer(surf, plane) + pixel_index_byte;
		uint8_t bit = ((*px_plane_byte) >> pixel_index_bit) & 1;
		val |= bit << plane;
	}
	return val;
}

uint8_t vl_dos_leftmask[8] = {0xff, 0x7f, 0x3f, 0x1f, 0xf, 7, 3, 1};
uint8_t vl_dos_rightmask[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

static void VL_DOS_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int pitch = surf->w / 8;
	int line_width = w / 8;
	int x_end_bit = x + w - 1;
	int x_low_byte = x / 8;
	int x_high_byte = x_end_bit / 8;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; ++plane)
	{
		uint8_t *plane_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane);
		for (int _y = y; _y < y + h; ++_y)
		{
			uint8_t *line_ptr = plane_ptr + (_y * pitch) + x_low_byte;
			if (x_low_byte == x_high_byte)
			{
				uint8_t mask = vl_dos_leftmask[x & 7] & vl_dos_rightmask[x_end_bit & 7];
				line_ptr[0] &= ~mask;
				if ((colour & (1 << plane)))
					line_ptr[0] |= mask;
			}
			else
			{

				line_ptr[0] &= ~vl_dos_leftmask[x & 7];
				if ((colour & (1 << plane)))
					line_ptr[0] |= vl_dos_leftmask[x & 7];
				if (x_high_byte - x_low_byte > 0)
					memset(line_ptr + 1, (colour & (1 << plane)) ? 255 : 0, x_high_byte - x_low_byte - 1);
				line_ptr[x_high_byte - x_low_byte] &= ~vl_dos_rightmask[x_end_bit & 7];
				if ((colour & (1 << plane)))
					line_ptr[x_high_byte - x_low_byte] |= vl_dos_rightmask[x_end_bit & 7];
			}
		}
	}
}

static void VL_DOS_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	mapmask &= 0xF;
	colour &= mapmask;
	int pitch = surf->w / 8;
	int line_width = w / 8;
	int x_end_bit = x + w - 1;
	int x_low_byte = x / 8;
	int x_high_byte = x_end_bit / 8;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; ++plane)
	{
		if (!(mapmask & (1 << plane)))
			continue;
		uint8_t *plane_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane);
		for (int _y = y; _y < y + h; ++_y)
		{
			uint8_t *line_ptr = plane_ptr + (_y * pitch) + x_low_byte;
			if (x_low_byte == x_high_byte)
			{
				uint8_t mask = vl_dos_leftmask[x & 7] & vl_dos_rightmask[x_end_bit & 7];
				line_ptr[0] &= ~mask;
				if ((colour & (1 << plane)))
					line_ptr[0] |= mask;
			}
			else
			{

				line_ptr[0] &= ~vl_dos_leftmask[x & 7];
				if ((colour & (1 << plane)))
					line_ptr[0] |= vl_dos_leftmask[x & 7];
				if (x_high_byte - x_low_byte > 0)
					memset(line_ptr + 1, (colour & (1 << plane)) ? 255 : 0, x_high_byte - x_low_byte - 1);
				line_ptr[x_high_byte - x_low_byte + 1] &= ~vl_dos_rightmask[x_end_bit & 7];
				if ((colour & (1 << plane)))
					line_ptr[x_high_byte - x_low_byte + 1] |= vl_dos_rightmask[x_end_bit & 7];
			}
		}
	}
}

static void VL_DOS_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)src_surface;
	VL_DOS_Surface *dest = (VL_DOS_Surface *)dst_surface;
	int dst_byte_x_offset = x / 8;	// We automatically round coordinates to multiples of 8 (byte boundaries)
	int src_byte_x_offset = sx / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		for (int _y = sy; _y < sy + sh; ++_y)
		{
			uint8_t *src_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + (_y * (surf->w / 8)) + src_byte_x_offset;
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(dest, plane) + ((_y - sy + y) * (dest->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (sx + sw + 7) / 8 - src_byte_x_offset;
			memcpy(dst_ptr, src_ptr, copy_len);
		}
	}
}

static void VL_DOS_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_DOS_Surface *srf = (VL_DOS_Surface *)surface;
	bool directionX = sx > x;
	bool directionY = sy > y;

	int dst_byte_x_offset = x / 8;	// We automatically round coordinates to multiples of 8 (byte boundaries)
	int src_byte_x_offset = sx / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)

	int pitch = srf->w / 8;

	if (srf->use == VL_SurfaceUsage_FrontBuffer)
	{
		//int page = srf->activePage;
		for (int page = 0; page < 2; ++page)
		{
			uint8_t *plane_ptr = (uint8_t *)(srf->activePage ? srf->data2 : srf->data);
			// Enable all planes.
			outportw(EGA_SC_INDEX, 0x0F00 | EGA_SC_MAP_MASK);
			VL_DOS_SetEGAWriteMode(1);
			if (directionY)
			{
				for (int yi = 0; yi < sh; ++yi)
				{
					uint8_t *src_ptr = plane_ptr + ((sy + yi) * pitch) + src_byte_x_offset;
					uint8_t *dst_ptr = plane_ptr + ((yi + y) * pitch) + dst_byte_x_offset;
					if (directionX)
					{
						for (int xi = 0; xi < (sw / 8); ++xi)
							dst_ptr[xi] = src_ptr[xi];
					}
					else
					{
						for (int xi = (sw / 8) - 1; xi >= 0; --xi)
							dst_ptr[xi] = src_ptr[xi];
					}
				}
			}
			else
			{
				for (int yi = sh - 1; yi >= 0; --yi)
				{
					uint8_t *src_ptr = plane_ptr + ((sy + yi) * pitch) + src_byte_x_offset;
					uint8_t *dst_ptr = plane_ptr + ((yi + y) * pitch) + dst_byte_x_offset;
					if (directionX)
					{
						for (int xi = 0; xi < (sw / 8); ++xi)
							dst_ptr[xi] = src_ptr[xi];
					}
					else
					{
						for (int xi = (sw / 8) - 1; xi >= 0; --xi)
							dst_ptr[xi] = src_ptr[xi];
					}
				}
			}
		}
	}
	else
	{
		VL_DOS_SetEGAWriteMode(0);
		for (int plane = 0; plane < 4; plane++)
		{
			uint8_t *plane_ptr = VL_DOS_GetSurfacePlanePointer(srf, plane);
			if (directionY)
			{
				for (int yi = 0; yi < sh; ++yi)
				{
					uint8_t *src_ptr = plane_ptr + ((sy + yi) * pitch) + src_byte_x_offset;
					uint8_t *dst_ptr = plane_ptr + ((yi + y) * pitch) + dst_byte_x_offset;
					memmove(dst_ptr, src_ptr, sw / 8);
				}
			}
			else
			{
				for (int yi = sh - 1; yi >= 0; --yi)
				{
					uint8_t *src_ptr = plane_ptr + ((sy + yi) * pitch) + src_byte_x_offset;
					uint8_t *dst_ptr = plane_ptr + ((yi + y) * pitch) + dst_byte_x_offset;
					memmove(dst_ptr, src_ptr, sw / 8);
				}
			}
		}
	}
}

static void VL_DOS_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;

	int initial_x = x, initial_y = y, final_w = w, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w, surf->h);

	int src_plane_size = (w / 8) * h;
	int dst_byte_x_offset = (x) / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (src_plane_size * plane) + (_y * (w / 8)) + initial_x / 8;
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w + 7) / 8;
			memcpy(dst_ptr, src_ptr, copy_len);
		}
	}
}

static void VL_DOS_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int initial_x = x, initial_y = y, final_w = w, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w, surf->h);
	int src_plane_size = (w / 8) * h;
	int dst_byte_x_offset = x / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		if (!(mapmask & (1 << plane)))
			continue;
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (src_plane_size * plane) + (_y * (w / 8)) + initial_x / 8;
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w + 7) / 8;
			memcpy(dst_ptr, src_ptr, copy_len);
		}
	}
}

static void VL_DOS_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int initial_x = x, initial_y = y, final_w = w, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w, surf->h);
	int src_plane_size = (w / 8) * h;
	int dst_byte_x_offset = x / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (src_plane_size * (plane + 1)) + (_y * (w / 8));
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w + 7) / 8;
			memcpy(dst_ptr, src_ptr, copy_len);
		}
	}
}

static void VL_DOS_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int initial_x = x / 8, initial_y = y, final_w = w / 8, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w / 8, surf->h);
	int src_plane_size = (w / 8) * h;
	int dst_byte_x_offset = CK_Cross_max(0, x / 8); // We automatically round coordinates to multiples of 8 (byte boundaries)
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (src_plane_size * (plane + 1)) + (_y * (w / 8)) + (initial_x);
			uint8_t *mask_ptr = (uint8_t *)src + (_y * (w / 8)) + (initial_x);
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w);
			for (int _x = 0; _x < copy_len; ++_x)
			{
				*dst_ptr &= *(mask_ptr++);
				*(dst_ptr++) |= *(src_ptr++);
			}
		}
	}
}

static void VL_DOS_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int dst_byte_x_offset = x / 8;
	int dst_bit_x_offset = x & 7;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		//if (!(colour & (1 << plane)))
		//	continue;
		for (int _y = 0; _y < h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (_y * ((w + 7) / 8));
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (w + 7) / 8;
			uint8_t prev_byte = 0;
			for (int i = 0; i < copy_len; ++i)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset)) | (src_ptr[i] >> dst_bit_x_offset);
				prev_byte = src_ptr[i];
				dst_ptr[i] = src_byte;
			}
			if (dst_bit_x_offset)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset));
				dst_ptr[copy_len] = src_byte;
			}
		}
	}
}

static void VL_DOS_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int dst_byte_x_offset = x / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	int dst_bit_x_offset = x & 7;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		//if (!(colour & (1 << plane)))
		//	continue;
		if (!(mapmask & (1 << plane)))
			continue;
		for (int _y = 0; _y < h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (_y * ((w + 7) / 8));
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (w + 7) / 8;
			uint8_t prev_byte = 0;
			for (int i = 0; i < copy_len; ++i)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset)) | (src_ptr[i] >> dst_bit_x_offset);
				prev_byte = src_ptr[i];
				dst_ptr[i] = src_byte;
			}
			if (dst_bit_x_offset)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset));
				dst_ptr[copy_len] = src_byte;
			}
		}
	}
}

static void VL_DOS_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int dst_byte_x_offset = x / 8; // We automatically round coordinates to multiples of 8 (byte boundaries)
	int dst_bit_x_offset = x & 7;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		if (!(colour & (1 << plane)))
			continue;
		for (int _y = 0; _y < h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (_y * ((w + 7) / 8));
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (w + 7) / 8;
			uint8_t prev_byte = 0x00;
			for (int x_byte = 0; x_byte < copy_len; ++x_byte)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset)) | (src_ptr[x_byte] >> dst_bit_x_offset);
				prev_byte = src_ptr[x_byte];
				uint8_t b = dst_ptr[x_byte];
				b ^= src_byte;
				dst_ptr[x_byte] = b;
			}
			uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset));
			uint8_t b = dst_ptr[copy_len];
			b ^= src_byte;
			dst_ptr[copy_len] = b;
		}
	}
}

static void VL_DOS_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int initial_x = x / 8, initial_y = y, final_w = w / 8, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w / 8, surf->h);
	int dst_byte_x_offset = CK_Cross_max(0, x / 8); // We automatically round coordinates to multiples of 8 (byte boundaries)
	int dst_bit_x_offset = x & 7;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		if (!(colour & (1 << plane)))
			continue;
		uint8_t prev_byte = 0;
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (_y * ((w + 7) / 8)) + (initial_x);
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w);
			for (int i = 0; i < copy_len; ++i)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset)) | (src_ptr[i] >> dst_bit_x_offset);
				prev_byte = src_ptr[i];
				dst_ptr[i] = src_byte;
			}
		}
	}
}

static void VL_DOS_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	int initial_x = x / 8, initial_y = y, final_w = w / 8, final_h = h;
	VL_Clip(&final_w, &final_h, &initial_x, &initial_y, surf->w / 8, surf->h);
	int dst_byte_x_offset = CK_Cross_max(0, x / 8); // We automatically round coordinates to multiples of 8 (byte boundaries)
	int dst_bit_x_offset = x & 7;
	VL_DOS_SetEGAWriteMode(0);
	for (int plane = 0; plane < 4; plane++)
	{
		if (!(colour & (1 << plane)))
			continue;
		uint8_t prev_byte = 0;
		for (int _y = initial_y; _y < initial_y + final_h; ++_y)
		{
			uint8_t *src_ptr = (uint8_t *)src + (_y * ((w + 7) / 8)) + (initial_x);
			uint8_t *dst_ptr = VL_DOS_GetSurfacePlanePointer(surf, plane) + ((_y + y) * (surf->w / 8)) + dst_byte_x_offset;
			size_t copy_len = (final_w);
			for (int i = 0; i < copy_len; ++i)
			{
				uint8_t src_byte = (prev_byte << (8 - dst_bit_x_offset)) | (src_ptr[i] >> dst_bit_x_offset);
				prev_byte = src_ptr[i];
				dst_ptr[i] |= ~src_byte;
			}
		}
	}
}

static void VL_DOS_ScrollSurface(void *surface, int x, int y)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	int dest_x = (x < 0) ? -x : 0;
	int dest_y = (y < 0) ? -y : 0;

	int src_x = (x > 0) ? x : 0;
	int src_y = (y > 0) ? y : 0;

	int wOffset = (x) ? -16 : 0;
	int hOffset = (y) ? -16 : 0;
	if (surf->use == VL_SurfaceUsage_FrontBuffer)
	{
		ssize_t bytesToShift = (x / 8) + y * (surf->w / 8);
		size_t surfaceSize = surf->w / 8 * surf->h * 2;
		volatile uint8_t *oldData = (uint8_t *)surf->data;
		volatile uint8_t *newData = oldData + bytesToShift;
		if (newData < (uint8_t *)(__djgpp_conventional_base + 0xA0000) || newData + surfaceSize > (uint8_t *)(__djgpp_conventional_base + 0xAFFFF))
		{
			// We've shifted outside of valid video memory.
			if (bytesToShift < 0)
				newData = (uint8_t *)(__djgpp_conventional_base + 0xAFFFF - surfaceSize);
			else
				newData = (uint8_t *)(__djgpp_conventional_base + 0xA0000);
			outportw(EGA_SC_INDEX, 0x0F00 | EGA_SC_MAP_MASK);
			VL_DOS_SetEGAWriteMode(1);
			for (int i = 0; i < surfaceSize; ++i)
				newData[i] = oldData[i];
			newData += bytesToShift;
		}
		surf->data = newData;
		surf->data2 = newData + surfaceSize / 2;
	}
	else
	{
		VL_ScreenToScreen(dest_x, dest_y, src_x, src_y, surf->w + wOffset, surf->h + hOffset);
	}
}

static void VL_DOS_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	uint32_t timeCount = SD_GetTimeCount();
	bool latchpel = ck_fixJerkyMotion;

	// Wait for the previous horizontal retrace to end
	if (vl_swapInterval)
	{
		do
		{
			if (SD_GetTimeCount() - timeCount > 1)
				break;
		} while (inportb(0x3DA) & 1);
	}

	int were_interrupts_enabled = disable();
	// We need to make sure that the EGA/VGA has the right pitch
	uint16_t val = ((surf->w >> 4) << 8) | EGA_CRTC_OFFSET;
	outportw(EGA_CRTC_INDEX, val);
	if (!singleBuffered)
		surf->activePage ^= 1;

	// Set the offset for scanout
	uint16_t surface_vmem_offset = (uint16_t)(((uintptr_t)(surf->activePage ? surf->data : surf->data2) - (__djgpp_conventional_base + 0xA0000)) & 0xFFFF);
	uint16_t byte_offset = surface_vmem_offset + ((scrlY * surf->w + scrlX) >> 3);
	uint16_t crtc_start_low = ((byte_offset & 0xFF) << 8) | EGA_CRTC_START_ADDR_LOW;
	uint16_t crtc_start_high = (byte_offset & 0xFF00) | EGA_CRTC_START_ADDR_HIGH;
	// NOTE: According to the original game, some XT EGA cards don't like word OUTs to
	// the CRTC index, so Keen splits these into two byte OUTs. (It doesn't for the CRTC_OFFSET,
	// though, so clearly this wasn't a universal problem.)
	outportw(EGA_CRTC_INDEX, crtc_start_high);
	outportw(EGA_CRTC_INDEX, crtc_start_low);

	// Wait for the next retrace to begin
	if (latchpel)
	{
		enable();
		do
		{
			if (SD_GetTimeCount() - timeCount > 1)
				break;
		} while (!(inportb(0x3DA) & 8));
		disable();
	}
	uint8_t pel_pan_offset = (scrlX & 7);
	outportb(EGA_ATR_INDEX, EGA_ATR_PELPAN | 0x20);
	outportb(EGA_ATR_INDEX, pel_pan_offset);
	if (!latchpel)
	{
		enable();
		do
		{
			if (SD_GetTimeCount() - timeCount > 1)
				break;
		} while (!(inportb(0x3DA) & 8));
		disable();
	}

	if (were_interrupts_enabled)
		enable();

}

int VL_DOS_GetActiveBufferId(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	return surf->activePage;
}

int VL_DOS_GetNumBuffers(void *surface)
{
	return 2;
}

void VL_DOS_SyncBuffers(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;

	size_t surfaceSize = surf->w / 8 * surf->h;
	volatile uint8_t *oldData = (volatile uint8_t *)(surf->activePage ? surf->data : surf->data2);
	volatile uint8_t *newData = (volatile uint8_t *)(surf->activePage ? surf->data2 : surf->data);
	outportw(EGA_SC_INDEX, 0x0F00 | EGA_SC_MAP_MASK);
	VL_DOS_SetEGAWriteMode(1);
	for (int i = 0; i < surfaceSize; ++i)
		newData[i] = oldData[i];

}

void VL_DOS_UpdateRect(void *surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;

	int byte_x_offset = x / 8;	// We automatically round coordinates to multiples of 8 (byte boundaries)
	int pitch = surf->w / 8;


	volatile uint8_t *oldData = (volatile uint8_t *)(surf->activePage ? surf->data2 : surf->data);
	volatile uint8_t *newData = (volatile uint8_t *)(surf->activePage ? surf->data : surf->data2);
	outportw(EGA_SC_INDEX, 0x0F00 | EGA_SC_MAP_MASK);
	VL_DOS_SetEGAWriteMode(1);
	for (int _y = y; _y < (y + h); ++_y)
		for (int _x = byte_x_offset; _x < byte_x_offset + (w / 8); ++_x)
			newData[_y*pitch+_x] = oldData[_y*pitch+_x];
}

void VL_DOS_FlushParams()
{
}

void VL_DOS_WaitVBLs(int vbls)
{
	for (int i = 0; i < vbls; ++i)
	{
		uint32_t timeCount = SD_GetTimeCount();
		// Wait for the previous retrace to end
		do
		{
			if (SD_GetTimeCount() - timeCount > 1)
				break;
		} while (inportb(0x3DA) & 8);
	}
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_dos_backend =
	{
		/*.setVideoMode =*/&VL_DOS_SetVideoMode,
		/*.createSurface =*/&VL_DOS_CreateSurface,
		/*.destroySurface =*/&VL_DOS_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_DOS_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_DOS_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_DOS_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_DOS_SurfacePGet,
		/*.surfaceRect =*/&VL_DOS_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_DOS_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_DOS_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_DOS_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_DOS_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_DOS_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_DOS_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_DOS_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_DOS_BitToSurface,
		/*.bitToSurface_PM =*/&VL_DOS_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_DOS_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_DOS_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_DOS_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_DOS_ScrollSurface,
		/*.present =*/&VL_DOS_Present,
		/*.getActiveBufferId =*/&VL_DOS_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_DOS_GetNumBuffers,
		/*.syncBuffers =*/&VL_DOS_SyncBuffers,
		/*.updateRect =*/&VL_DOS_UpdateRect,
		/*.flushParams =*/&VL_DOS_FlushParams,
		/*.waitVBLs =*/&VL_DOS_WaitVBLs};

VL_Backend *VL_Impl_GetBackend()
{
	return &vl_dos_backend;
}
