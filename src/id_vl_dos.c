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

#include "id_vl.h"
#include "id_vl_private.h"
#include "id_us.h"

#include <stdlib.h>
#include <string.h>

#include <pc.h>
#include <dpmi.h>
#include <sys/movedata.h>

static int vl_null_screenWidth;
static int vl_null_screenHeight;

/* TODO (Overscan border):
 * - If a texture is used for offscreen rendering with scaling applied later,
 * it's better to have the borders within the texture itself.
 */

typedef struct VL_DOS_Surface
{
	VL_SurfaceUsage use;
	int w, h;
	void *data;
} VL_DOS_Surface;

static void VL_DOS_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		vl_null_screenWidth = VL_EGAVGA_GFX_WIDTH;
		vl_null_screenHeight = VL_EGAVGA_GFX_HEIGHT;
		__dpmi_regs r;
		r.x.ax = 0x13;
		__dpmi_int(0x10, &r);
	}
	else
	{
		__dpmi_regs r;
		r.x.ax = 0x00;
		__dpmi_int(0x10, &r);
	}
}

static void VL_DOS_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_DOS_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface*)malloc(sizeof(VL_DOS_Surface));
	surf->use = usage;
	surf->w = w;
	surf->h = h;
	surf->data = malloc(w * h); // 8-bit pal for now
	return surf;
}

static void VL_DOS_DestroySurface(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface*)surface;
	if (surf->data)
		free(surf->data);
	free(surf);
}

static long VL_DOS_GetSurfaceMemUse(void *surface)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	return surf->w*surf->h;
}

static void VL_DOS_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	if (w) *w = surf->w;
	if (h) *h = surf->h;
}

static void VL_DOS_RefreshPaletteAndBorderColor(void *screen)
{
	for (int i = 0; i < 16; i++)
	{
		outportb(0x3C8, i);
		
		outportb(0x3C9, VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0]); // red
		outportb(0x3C9, VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1]); // green
		outportb(0x3C9, VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2]); // blue
	}
	
	__dpmi_regs r;
	r.h.al = 0x10;
	r.h.ah = 1;
	r.h.bh = vl_emuegavgaadapter.bordercolor;
	__dpmi_int(0x10, &r);
}

static int VL_DOS_SurfacePGet(void *surface, int x, int y)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface*) surface;
	return ((uint8_t*)surf->data)[y*surf->w+x];
}

static void VL_DOS_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface*) dst_surface;
	for (int _y = y; _y < y+h; ++_y)
	{
		memset(((uint8_t*)surf->data)+_y*surf->w+x, colour, w);
	}
}

static void VL_DOS_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
  mapmask &= 0xF;
  colour &= mapmask;

	VL_DOS_Surface *surf = (VL_DOS_Surface*) dst_surface;
	for (int _y = y; _y < y+h; ++_y)
	{
    for (int _x = x; _x < x+w; ++ _x)
    {
      uint8_t *p = ((uint8_t*)surf->data) + _y*surf->w + _x;
      *p &= ~mapmask;
      *p |= colour;
    }
	}
}

static void VL_DOS_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)src_surface;
	VL_DOS_Surface *dest = (VL_DOS_Surface *)dst_surface;
	for (int _y = sy; _y < sy+sh; ++_y)
	{
		memcpy(((uint8_t*)dest->data)+(_y-sy+y)*dest->w+x,((uint8_t*)surf->data)+_y*surf->w+sx, sw);
	}
}

static void VL_DOS_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_DOS_Surface *srf = (VL_DOS_Surface *)surface;
	bool directionX = sx > x;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(((uint8_t*)srf->data)+((yi+y)*srf->w+x),((uint8_t*)srf->data)+((sy+yi)*srf->w+sx),sw);
		}
	}
	else
	{
		for (int yi = sh-1; yi >= 0; --yi)
		{
			memmove(((uint8_t*)srf->data)+((yi+y)*srf->w+x),((uint8_t*)srf->data)+((sy+yi)*srf->w+sx),sw);
		}
	}

}

static void VL_DOS_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h) {
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_DOS_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask) {
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_UnmaskedToPAL8_PM(src, surf->data, x, y, surf->w, w, h, mapmask);
}

static void VL_DOS_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_MaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_DOS_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_MaskedBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h);
}

static void VL_DOS_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_1bppToPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_DOS_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_1bppToPAL8_PM(src, surf->data, x, y, surf->w, w, h, colour, mapmask);
}

static void VL_DOS_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_1bppXorWithPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_DOS_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->w, w,h, colour);
}

static void VL_DOS_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)dst_surface;
	VL_1bppInvBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h, colour);
}

static void VL_DOS_Present(void *surface, int scrlX, int scrlY)
{
	VL_DOS_Surface *surf = (VL_DOS_Surface *)surface;
	
	// Wait for the previous retrace to end
	do {} while (inportb(0x3DA) & 8);
	
	for (int i = 0; i < 200; ++i)
	{
		int y = i + scrlY;
		int x = scrlX;
		
		uint8_t *surfLineAddress = (uint8_t*)(surf->data) + (y * surf->w) + x;
		uint32_t vgaLineAddress = 0xA0000 + 320 * i;
		dosmemput(surfLineAddress, 320, vgaLineAddress);
	}
	// Wait for the next retrace to begin
	do {} while (!(inportb(0x3DA) & 8));
}

void VL_DOS_FlushParams()
{
}

void VL_DOS_WaitVBLs(int vbls)
{
	for (int i = 0; i < vbls; ++i)
	{
		// Wait for the previous retrace to end
		do {} while (inportb(0x3DA) & 8);
		// Wait for the next retrace to begin
		do {} while (!(inportb(0x3DA) & 8));
	}
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_dos_backend =
{
	/*.setVideoMode =*/ &VL_DOS_SetVideoMode,
	/*.createSurface =*/ &VL_DOS_CreateSurface,
	/*.destroySurface =*/ &VL_DOS_DestroySurface,
	/*.getSurfaceMemUse =*/ &VL_DOS_GetSurfaceMemUse,
	/*.getSurfaceDimensions =*/ &VL_DOS_GetSurfaceDimensions,
	/*.refreshPaletteAndBorderColor =*/ &VL_DOS_RefreshPaletteAndBorderColor,
	/*.surfacePGet =*/ &VL_DOS_SurfacePGet,
	/*.surfaceRect =*/ &VL_DOS_SurfaceRect,
	/*.surfaceRect_PM =*/ &VL_DOS_SurfaceRect_PM,
	/*.surfaceToSurface =*/ &VL_DOS_SurfaceToSurface,
	/*.surfaceToSelf =*/ &VL_DOS_SurfaceToSelf,
	/*.unmaskedToSurface =*/ &VL_DOS_UnmaskedToSurface,
	/*.unmaskedToSurface_PM =*/ &VL_DOS_UnmaskedToSurface_PM,
	/*.maskedToSurface =*/ &VL_DOS_MaskedToSurface,
	/*.maskedBlitToSurface =*/ &VL_DOS_MaskedBlitToSurface,
	/*.bitToSurface =*/ &VL_DOS_BitToSurface,
	/*.bitToSurface_PM =*/ &VL_DOS_BitToSurface_PM,
	/*.bitXorWithSurface =*/ &VL_DOS_BitXorWithSurface,
	/*.bitBlitToSurface =*/ &VL_DOS_BitBlitToSurface,
	/*.bitInvBlitToSurface =*/ &VL_DOS_BitInvBlitToSurface,
	/*.present =*/ &VL_DOS_Present,
	/*.flushParams =*/ &VL_DOS_FlushParams,
	/*.waitVBLs =*/ &VL_DOS_WaitVBLs
};

VL_Backend *VL_Impl_GetBackend()
{
	return &vl_dos_backend;
}


