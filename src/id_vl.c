/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2012 David Gow <david@ingeniumdigital.com>

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
#include "id_ca.h"
#include "id_mm.h"
#include "id_vl_private.h"

#include "ck_cross.h"
#include "id_us.h"

#include <stdlib.h>

// EGA color palette in RGB format (technically more can be chosen with the VGA)
const uint8_t VL_EGARGBColorTable[16][3] = {
	0x00, 0x00, 0x00, // Black
	0x00, 0x00, 0xaa, // Blue
	0x00, 0xaa, 0x00, // Green
	0x00, 0xaa, 0xaa, // Cyan
	0xaa, 0x00, 0x00, // Red
	0xaa, 0x00, 0xaa, // Magenta
	0xaa, 0x55, 0x00, // Brown
	0xaa, 0xaa, 0xaa, // Light Grey
	0x55, 0x55, 0x55, // Dark Grey
	0x55, 0x55, 0xff, // Light Blue
	0x55, 0xff, 0x55, // Light Green
	0x55, 0xff, 0xff, // Light Cyan
	0xff, 0x55, 0x55, // Light Red
	0xff, 0x55, 0xff, // Light Magenta
	0xff, 0xff, 0x55, // Yellow
	0xff, 0xff, 0xff, // White
};

// EGA signal palettes (the 17th entry of each row is the overscan border color)
// NOTE: Vanilla Keen can modify some of these (e.g. the border color)
uint8_t vl_palette[6][17] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // Pitch black
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 24, 25, 26, 27, 28, 29, 30, 31, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31, 0, // Default palette
	0, 1, 2, 3, 4, 5, 6, 7, 31, 31, 31, 31, 31, 31, 31, 31, 0,
	31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, // Full bright
};

bool vl_screenFaded = false;

bool vl_started = false;

static int vl_memused, vl_numsurfaces;

VL_EGAVGAAdapter vl_emuegavgaadapter;

uint16_t vl_border_color = 0; // An extra variable used in vanilla Keen code

static VL_Backend *vl_currentBackend;

static int vl_mapMask = 0xF;

#if 0
VL_EGAPaletteEntry VL_EGAPalette[16];

void VL_SetPalEntry(int id, uint8_t r, uint8_t g, uint8_t b)
{
	VL_EGAPalette[id].r = r;
	VL_EGAPalette[id].g = g;
	VL_EGAPalette[id].b = b;
}
#endif

/* Gets a value representing 6 EGA signals determining a color number
and returns it in a "Blue Green Red Intensity" 4-bit format.
Usually, the 6 signals represented by the given input mean:
"Blue Green Red Secondary-Blue Secondary-Green Secondary-Red". However, for
the historic reason of compatibility with CGA monitors, on the 200-lines mode
used by Keen the Secondary-Green signal is treated as an Intensity one and
the two other intensity signals are ignored.                               */
uint8_t VL_ConvertEGASignalToEGAEntry(const uint8_t color)
{
	return (color & 7) | ((color & 16) >> 1);
}

void VL_SetPaletteAndBorderColor(uint8_t *palette)
{
	for (int i = 0; i < 16; i++)
		vl_emuegavgaadapter.palette[i] = VL_ConvertEGASignalToEGAEntry(palette[i]);
	vl_emuegavgaadapter.bordercolor = VL_ConvertEGASignalToEGAEntry(palette[16]);
	vl_currentBackend->refreshPaletteAndBorderColor(vl_emuegavgaadapter.screen);
}

// Update the palette (but not the border colour) from a CK_VAR-style intptr_t array.
void VL_SetPaletteFromArray(intptr_t *palArray)
{
	for (int i = 0; i < 16; i++)
		vl_emuegavgaadapter.palette[i] = VL_ConvertEGASignalToEGAEntry(palArray[i]);
	vl_currentBackend->refreshPaletteAndBorderColor(vl_emuegavgaadapter.screen);
}

void VL_ColorBorder(uint16_t color)
{
	vl_emuegavgaadapter.bordercolor = VL_ConvertEGASignalToEGAEntry(color); // Updated EGA/VGA status
	vl_currentBackend->refreshPaletteAndBorderColor(vl_emuegavgaadapter.screen);
	vl_border_color = color; // Used by Keen
}

// Sets up the EGA palette used in the original keen games.
void VL_SetDefaultPalette()
{
	vl_palette[3][16] = vl_border_color;
	VL_SetPaletteAndBorderColor(vl_palette[3]);
	vl_screenFaded = false;
#if 0
	VL_SetPalEntry(0,0x00, 0x00, 0x00); // Black
	VL_SetPalEntry(1,0x00, 0x00, 0xAA); // Blue
	VL_SetPalEntry(2,0x00, 0xAA, 0x00); // Green
	VL_SetPalEntry(3,0x00, 0xAA, 0xAA); // Cyan
	VL_SetPalEntry(4,0xAA, 0x00, 0x00); // Red
	VL_SetPalEntry(5,0xAA, 0x00, 0xAA); // Magenta
	VL_SetPalEntry(6,0xAA, 0x55, 0x00); // Brown
	VL_SetPalEntry(7,0xAA, 0xAA, 0xAA); // Light Grey
	VL_SetPalEntry(8,0x55, 0x55, 0x55); // Dark Grey
	VL_SetPalEntry(9,0x55, 0x55, 0xFF); // Light Blue
	VL_SetPalEntry(10,0x55, 0xFF, 0x55); // Light Green
	VL_SetPalEntry(11,0x55, 0xFF, 0xFF); // Light Cyan
	VL_SetPalEntry(12,0xFF, 0x55, 0x55); // Light Red
	VL_SetPalEntry(13,0xFF, 0x55, 0xFF); // Light Magenta
	VL_SetPalEntry(14,0xFF, 0xFF, 0x55); // Yellow
	VL_SetPalEntry(15,0xFF, 0xFF, 0xFF); // White
#endif
}

void VL_SetPaletteByID(int id)
{
	vl_palette[id][16] = vl_border_color;
	VL_SetPaletteAndBorderColor(vl_palette[id]);
}

void VL_SetPalette(uint8_t *palette)
{
	palette[16] = vl_border_color;
	VL_SetPaletteAndBorderColor(palette);
}

void VL_FadeToBlack(void)
{
	for (int i = 3; i >= 0; i--)
	{
		vl_palette[i][16] = vl_border_color;
		VL_SetPaletteAndBorderColor(vl_palette[i]);
		VL_Present();
		VL_DelayTics(6);
	}
	vl_screenFaded = true;
}

void VL_FadeFromBlack(void)
{
	for (int i = 0; i < 4; i++)
	{
		vl_palette[i][16] = vl_border_color;
		VL_SetPaletteAndBorderColor(vl_palette[i]);
		VL_Present();
		VL_DelayTics(6);
	}
	vl_screenFaded = false;
}

/*
 * VL_Clip:
 * This is a somewhat nasty function which clips an input rect to an output.
 * As input, it takes:
 *  - src_{w,h} as the "source" rectange, to be clipped to the "dest" dst_{x,y,w,h}
 * It then outputs:
 *  - A new src_{w,h} (called final{W,H}), which is the final width, height of the src
 *  - A new dst_{x,y}, which is the offset _in source coordinates_ of the top left of
 *    the final source rect.
 */
void VL_Clip(int *src_w, int *src_h, int *dst_x, int *dst_y, int dst_w, int dst_h)
{
	int initialX = CK_Cross_max(-(*dst_x), 0);
	int initialY = CK_Cross_max(-(*dst_y), 0);
	int finalW = CK_Cross_min(CK_Cross_max(dst_w - (*dst_x), 0), CK_Cross_min(*src_w, *dst_x + *src_w));
	int finalH = CK_Cross_min(CK_Cross_max(dst_h - (*dst_y), 0), CK_Cross_min(*src_h, *dst_y + *src_h));

	*src_w = CK_Cross_max(finalW, 0);
	*src_h = CK_Cross_max(finalH, 0);
	*dst_x = initialX;
	*dst_y = initialY;
}
#if 0
void VL_UnmaskedToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_b = (uint8_t*)src;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));

			int pixel = ((srcptr_i[plane_off] & plane_bit)?8:0) |
					((srcptr_r[plane_off] & plane_bit)?4:0) |
					((srcptr_g[plane_off] & plane_bit)?2:0) |
					((srcptr_b[plane_off] & plane_bit)?1:0);

			// XRGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = 0xFF;
		}
	}
}
#endif

void VL_UnmaskedToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr_b = (uint8_t *)src;
	uint8_t *srcptr_g = srcptr_b + (w / 8) * h;
	uint8_t *srcptr_r = srcptr_g + (w / 8) * h;
	uint8_t *srcptr_i = srcptr_r + (w / 8) * h;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			int pixel = ((srcptr_i[plane_off] & plane_bit) ? 8 : 0) |
				((srcptr_r[plane_off] & plane_bit) ? 4 : 0) |
				((srcptr_g[plane_off] & plane_bit) ? 2 : 0) |
				((srcptr_b[plane_off] & plane_bit) ? 1 : 0);

			dstptr[(sy + y) * pitch + (sx + x)] = pixel;
		}
	}
}

// Used by the Terminator and Star Wars sequences to draw bitmaps to select planes
void VL_UnmaskedToPAL8_PM(void *src, void *dest, int x, int y, int pitch, int w, int h, int mapmask)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr_b = (uint8_t *)src;
	uint8_t *srcptr_g = srcptr_b + (w / 8) * h;
	uint8_t *srcptr_r = srcptr_g + (w / 8) * h;
	uint8_t *srcptr_i = srcptr_r + (w / 8) * h;

	mapmask &= 0xF;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			int pixel = ((mapmask & 8) && (srcptr_i[plane_off] & plane_bit) ? 8 : 0) |
				((mapmask & 4) && (srcptr_r[plane_off] & plane_bit) ? 4 : 0) |
				((mapmask & 2) && (srcptr_g[plane_off] & plane_bit) ? 2 : 0) |
				((mapmask & 1) && (srcptr_b[plane_off] & plane_bit) ? 1 : 0);

			dstptr[(sy + y) * pitch + (sx + x)] &= ~mapmask;
			dstptr[(sy + y) * pitch + (sx + x)] |= pixel;
		}
	}
}

#if 0
void VL_MaskedToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_a = (uint8_t*)src;
	uint8_t *srcptr_b = srcptr_a + (w/8)*h;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));

			int pixel = ((srcptr_i[plane_off] & plane_bit)?8:0) |
					((srcptr_r[plane_off] & plane_bit)?4:0) |
					((srcptr_g[plane_off] & plane_bit)?2:0) |
					((srcptr_b[plane_off] & plane_bit)?1:0);

			// ARGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = ((srcptr_a[plane_off] & plane_bit)?0xFF:0x00);
		}
	}
}
#endif

void VL_MaskedToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr_a = (uint8_t *)src;
	uint8_t *srcptr_b = srcptr_a + (w / 8) * h;
	uint8_t *srcptr_g = srcptr_b + (w / 8) * h;
	uint8_t *srcptr_r = srcptr_g + (w / 8) * h;
	uint8_t *srcptr_i = srcptr_r + (w / 8) * h;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			int pixel = ((srcptr_i[plane_off] & plane_bit) ? 8 : 0) |
				((srcptr_r[plane_off] & plane_bit) ? 4 : 0) |
				((srcptr_g[plane_off] & plane_bit) ? 2 : 0) |
				((srcptr_b[plane_off] & plane_bit) ? 1 : 0);

			dstptr[(sy + y) * pitch + (sx + x)] = pixel & ((srcptr_a[plane_off] & plane_bit) ? 0xF0 : 0x00);
		}
	}
}

#if 0
void VL_MaskedBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_a = (uint8_t*)src;
	uint8_t *srcptr_b = srcptr_a + (w/8)*h;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));


			if ((srcptr_a[plane_off] & plane_bit) != 0) {
				continue;
			}
			int pixel = ((srcptr_i[plane_off] & plane_bit)?8:0) |
					((srcptr_r[plane_off] & plane_bit)?4:0) |
					((srcptr_g[plane_off] & plane_bit)?2:0) |
					((srcptr_b[plane_off] & plane_bit)?1:0);

			// XRGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = 0xFF;
		}
	}
}
#endif

void VL_MaskedBlitToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr_a = (uint8_t *)src;
	uint8_t *srcptr_b = srcptr_a + (w / 8) * h;
	uint8_t *srcptr_g = srcptr_b + (w / 8) * h;
	uint8_t *srcptr_r = srcptr_g + (w / 8) * h;
	uint8_t *srcptr_i = srcptr_r + (w / 8) * h;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			if ((srcptr_a[plane_off] & plane_bit) == 0)
			{
				dstptr[(sy + y) * pitch + (sx + x)] = 0;
			}
			int pixel = ((srcptr_i[plane_off] & plane_bit) ? 8 : 0) |
				((srcptr_r[plane_off] & plane_bit) ? 4 : 0) |
				((srcptr_g[plane_off] & plane_bit) ? 2 : 0) |
				((srcptr_b[plane_off] & plane_bit) ? 1 : 0);

			dstptr[(sy + y) * pitch + (sx + x)] |= pixel;
		}
	}
}

#if 0
void VL_MaskedBlitClipToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_a = (uint8_t*)src;
	uint8_t *srcptr_b = srcptr_a + (w/8)*h;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;
	int initialX = CK_Cross_max(-x,0);
	int initialY = CK_Cross_max(-y,0);
	int finalW = CK_Cross_min(CK_Cross_max(dw-x,0), w);
	int finalH = CK_Cross_min(CK_Cross_max(dh-y,0), h);

	for(int sy = initialY; sy < finalH; ++sy)
	{
		for(int sx = initialX; sx < finalW; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));


			if ((srcptr_a[plane_off] & plane_bit) != 0) {
				continue;
			}
			int pixel = ((srcptr_i[plane_off] & plane_bit)?8:0) |
					((srcptr_r[plane_off] & plane_bit)?4:0) |
					((srcptr_g[plane_off] & plane_bit)?2:0) |
					((srcptr_b[plane_off] & plane_bit)?1:0);

			// XRGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = 0xFF;
		}
	}
}
#endif

void VL_MaskedBlitClipToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int dw, int dh)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr_a = (uint8_t *)src;
	uint8_t *srcptr_b = srcptr_a + (w / 8) * h;
	uint8_t *srcptr_g = srcptr_b + (w / 8) * h;
	uint8_t *srcptr_r = srcptr_g + (w / 8) * h;
	uint8_t *srcptr_i = srcptr_r + (w / 8) * h;
	int initialX = CK_Cross_max(-x, 0);
	int initialY = CK_Cross_max(-y, 0);
	int finalW = CK_Cross_min(CK_Cross_max(dw - x, 0), w);
	int finalH = CK_Cross_min(CK_Cross_max(dh - y, 0), h);

	for (int sy = initialY; sy < finalH; ++sy)
	{
		for (int sx = initialX; sx < finalW; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			if ((srcptr_a[plane_off] & plane_bit) == 0)
			{
				dstptr[(sy + y) * pitch + (sx + x)] = 0;
			}
			int pixel = ((srcptr_i[plane_off] & plane_bit) ? 8 : 0) |
				((srcptr_r[plane_off] & plane_bit) ? 4 : 0) |
				((srcptr_g[plane_off] & plane_bit) ? 2 : 0) |
				((srcptr_b[plane_off] & plane_bit) ? 1 : 0);

			dstptr[(sy + y) * pitch + (sx + x)] |= pixel;
		}
	}
}

#if 0
void VL_1bppToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr= (uint8_t*)src;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));

			int pixel = ((srcptr[plane_off] & plane_bit)?colour:0);

			// ARGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = ((srcptr[plane_off] & plane_bit)?0xFF:0x00);
		}
	}
}
#endif

void VL_1bppToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			int pixel = ((srcptr[plane_off] & plane_bit) ? colour : colour & 0xF0);

			dstptr[(sy + y) * pitch + (sx + x)] = pixel;
		}
	}
}

void VL_1bppToPAL8_PM(void *src, void *dest, int x, int y, int pitch, int w, int h, int colour, int mapmask)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;

	mapmask &= 0xF;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * w + sx) & 7));

			int pixel = ((srcptr[plane_off] & plane_bit) ? colour : colour & 0xF0) & mapmask;

			dstptr[(sy + y) * pitch + (sx + x)] &= ~mapmask;
			dstptr[(sy + y) * pitch + (sx + x)] |= pixel;
		}
	}
}

#if 0
void VL_1bppXorWithRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr = (uint8_t*)src;

	int spitch = ((w + 7)/8)*8;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1<<(7-((sy * spitch + sx) & 7));

			if (!(srcptr[plane_off] & plane_bit)) continue;

			// XRGB LE output
			// TODO: Doesn't it seem a bit hackish?
			dstptr[(sy+y)*pitch+(sx+x)*4+0] ^= VL_EGAPalette[colour].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] ^= VL_EGAPalette[colour].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] ^= VL_EGAPalette[colour].r;
			//dstptr[(sy+y)*pitch+(sx+x)*4+3] = 0xFF;
		}
	}
}
#endif

void VL_1bppXorWithPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;

	int spitch = ((w + 7) / 8) * 8;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * spitch + sx) & 7));

			if (!(srcptr[plane_off] & plane_bit))
				continue;

			dstptr[(sy + y) * pitch + (sx + x)] ^= colour;
		}
	}
}

#if 0
void VL_1bppBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr= (uint8_t*)src;

	int spitch = ((w + 7)/8)*8;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1<<(7-((sy * spitch + sx) & 7));

			if (!(srcptr[plane_off] & plane_bit)) continue;

			int pixel = ((srcptr[plane_off] & plane_bit)?colour:0);

			// XRGB LE output
			dstptr[(sy+y)*pitch+(sx+x)*4+0] = VL_EGAPalette[pixel].b;
			dstptr[(sy+y)*pitch+(sx+x)*4+1] = VL_EGAPalette[pixel].g;
			dstptr[(sy+y)*pitch+(sx+x)*4+2] = VL_EGAPalette[pixel].r;
			dstptr[(sy+y)*pitch+(sx+x)*4+3] = 0xFF;
		}
	}
}
#endif

void VL_1bppBlitToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;

	int spitch = ((w + 7) / 8) * 8;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * spitch + sx) & 7));

			if (!(srcptr[plane_off] & plane_bit))
				continue;

			dstptr[(sy + y) * pitch + (sx + x)] = colour;
		}
	}
}

// This is used in VH_DrawSpriteMask.
void VL_1bppInvBlitToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;

	int spitch = ((w + 7) / 8) * 8;

	for (int sy = 0; sy < h; ++sy)
	{
		for (int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * spitch + sx) & 7));

			if ((srcptr[plane_off] & plane_bit))
				continue;

			dstptr[(sy + y) * pitch + (sx + x)] = colour;
		}
	}
}

void VL_1bppInvBlitClipToPAL8(void *src, void *dest, int x, int y, int pitch, int w, int h, int dw, int dh, int colour)
{
	uint8_t *dstptr = (uint8_t *)dest;
	uint8_t *srcptr = (uint8_t *)src;
	int initialX = CK_Cross_max(-x, 0);
	int initialY = CK_Cross_max(-y, 0);
	int finalW = CK_Cross_min(CK_Cross_max(dw - x, 0), w);
	int finalH = CK_Cross_min(CK_Cross_max(dh - y, 0), h);
	int spitch = ((w + 7) / 8) * 8;

	for (int sy = initialY; sy < finalH; ++sy)
	{
		for (int sx = initialX; sx < finalW; ++sx)
		{
			int plane_off = (sy * spitch + sx) >> 3;
			int plane_bit = 1 << (7 - ((sy * spitch + sx) & 7));

			if ((srcptr[plane_off] & plane_bit))
				continue;

			dstptr[(sy + y) * pitch + (sx + x)] = colour;
		}
	}
}

int VL_MemUsed()
{
	return vl_memused;
}

int VL_NumSurfaces()
{
	return vl_numsurfaces;
}

void VL_InitScreen(void)
{
	vl_currentBackend = VL_Impl_GetBackend();
	vl_memused = 0;
	vl_numsurfaces = 1;
	vl_currentBackend->setVideoMode(0xD);
	vl_emuegavgaadapter.screen = vl_currentBackend->createSurface(21 * 16, 14 * 16, VL_SurfaceUsage_FrontBuffer);
	vl_memused += vl_currentBackend->getSurfaceMemUse(vl_emuegavgaadapter.screen);
	// NOTE: The overscan border color is *not* yet set to cyan here
	VL_SetPaletteAndBorderColor(vl_palette[3]);
	vl_started = true;
}

static const char *vl_parmStrings[] = { "HIDDENCARD", "NOPAN", "" };

bool vl_hiddenCard = false;
bool vl_noPan = false;

void VL_Startup()
{
	// Check command line args.
	for (int i = 1; i < us_argc; ++i)
	{
		int parmIdx = US_CheckParm(us_argv[i], vl_parmStrings);
		switch (parmIdx)
		{
		case 0:
			vl_hiddenCard = true;
			break;
		case 1:
			vl_noPan = true;
			break;
		}
	}
	VL_InitScreen();
}

void VL_Shutdown()
{
	if (vl_started)
	{
		vl_currentBackend->destroySurface(vl_emuegavgaadapter.screen);
		vl_currentBackend->setVideoMode(0);
		vl_memused = 0;
		vl_numsurfaces = 0;
		vl_started = false;
	}
}

void VL_ResizeScreen(int w, int h)
{
	vl_memused -= vl_currentBackend->getSurfaceMemUse(vl_emuegavgaadapter.screen);
	vl_currentBackend->destroySurface(vl_emuegavgaadapter.screen);

	vl_emuegavgaadapter.screen = vl_currentBackend->createSurface(w, h, VL_SurfaceUsage_FrontBuffer);
	vl_memused += vl_currentBackend->getSurfaceMemUse(vl_emuegavgaadapter.screen);
}

bool vl_isFullScreen;
bool vl_isAspectCorrected;
bool vl_isIntegerScaled;
bool vl_hasOverscanBorder;
int vl_swapInterval;

void VL_SetParams(bool isFullScreen, bool isAspectCorrected, bool hasOverscan, bool isIntegerScaled)
{
	vl_isFullScreen = isFullScreen;
	vl_isAspectCorrected = isAspectCorrected;
	vl_hasOverscanBorder = hasOverscan;
	vl_isIntegerScaled = isIntegerScaled;
	if (vl_currentBackend)
		vl_currentBackend->flushParams();
}

void VL_ToggleFullscreen()
{
	VL_SetParams(!vl_isFullScreen, vl_isAspectCorrected, vl_hasOverscanBorder, vl_isIntegerScaled);
}

void VL_ToggleAspect()
{
	VL_SetParams(vl_isFullScreen, !vl_isAspectCorrected, vl_hasOverscanBorder, vl_isIntegerScaled);
}

void VL_ToggleBorder()
{
	VL_SetParams(vl_isFullScreen, vl_isAspectCorrected, !vl_hasOverscanBorder, vl_isIntegerScaled);
}

void VL_ToggleInteger()
{
	VL_SetParams(vl_isFullScreen, vl_isAspectCorrected, vl_hasOverscanBorder, !vl_isIntegerScaled);
}

void VL_ToggleVSync()
{
	// TODO: Support Adaptive VSync (-1)
	vl_swapInterval ^= 1;
	if (vl_currentBackend)
		vl_currentBackend->flushParams();
}

void *VL_CreateSurface(int w, int h)
{
	void *s = vl_currentBackend->createSurface(w, h, VL_SurfaceUsage_Default);
	vl_memused += vl_currentBackend->getSurfaceMemUse(s);
	vl_numsurfaces++;
	return s;
}

void VL_DestroySurface(void *surf)
{
	if (!vl_started || !surf)
		return;

	vl_memused -= vl_currentBackend->getSurfaceMemUse(surf);
	vl_numsurfaces--;
	vl_currentBackend->destroySurface(surf);
}

void *VL_SetScreen(void *surf)
{
	void *old_screen = vl_emuegavgaadapter.screen;
	vl_emuegavgaadapter.screen = surf;
	return old_screen;
}

int VL_SurfacePGet(void *surf, int x, int y)
{
	return vl_currentBackend->surfacePGet(surf, x, y);
}

void VL_SurfaceRect(void *dst, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->surfaceRect(dst, x, y, w, h, colour);
}

void VL_ScreenRect(int x, int y, int w, int h, int colour)
{
	vl_currentBackend->surfaceRect(vl_emuegavgaadapter.screen, x, y, w, h, colour);
}

void VL_ScreenRect_PM(int x, int y, int w, int h, int colour)
{
	vl_currentBackend->surfaceRect_PM(vl_emuegavgaadapter.screen, x, y, w, h, colour, vl_mapMask);
}

void VL_ScreenToScreen(int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSelf(vl_emuegavgaadapter.screen, x, y, sx, sy, sw, sh);
}

void VL_SurfaceToSurface(void *src, void *dst, int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSurface(src, dst, x, y, sx, sy, sw, sh);
}

void VL_SurfaceToScreen(void *src, int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSurface(src, vl_emuegavgaadapter.screen, x, y, sx, sy, sw, sh);
}

void VL_SurfaceToSelf(void *surf, int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSelf(surf, x, y, sx, sy, sw, sh);
}

void VL_UnmaskedToSurface(void *src, void *dest, int x, int y, int w, int h)
{
	vl_currentBackend->unmaskedToSurface(src, dest, x, y, w, h);
}

void VL_UnmaskedToScreen(void *src, int x, int y, int w, int h)
{
	vl_currentBackend->unmaskedToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h);
}

void VL_UnmaskedToScreen_PM(void *src, int x, int y, int w, int h)
{
	vl_currentBackend->unmaskedToSurface_PM(src, vl_emuegavgaadapter.screen, x, y, w, h, vl_mapMask);
}

void VL_MaskedToSurface(void *src, void *dest, int x, int y, int w, int h)
{
	vl_currentBackend->maskedToSurface(src, dest, x, y, w, h);
}

void VL_MaskedBlitToSurface(void *src, void *dest, int x, int y, int w, int h)
{
	vl_currentBackend->maskedBlitToSurface(src, dest, x, y, w, h);
}

// This is not the function you are looking for.
// It does not perform masking, simply overwrites the screen's alpha channel.
void VL_MaskedToScreen(void *src, int x, int y, int w, int h)
{
	vl_currentBackend->maskedToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h);
}

void VL_MaskedBlitToScreen(void *src, int x, int y, int w, int h)
{
	vl_currentBackend->maskedBlitToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h);
}

void VL_1bppToScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h, colour);
}

void VL_1bppToScreen_PM(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitToSurface_PM(src, vl_emuegavgaadapter.screen, x, y, w, h, colour, vl_mapMask);
}

void VL_1bppXorWithScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitXorWithSurface(src, vl_emuegavgaadapter.screen, x, y, w, h, colour);
}

void VL_1bppBlitToScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitBlitToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h, colour);
}

void VL_1bppInvBlitToScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitInvBlitToSurface(src, vl_emuegavgaadapter.screen, x, y, w, h, colour);
}

void VL_ScrollScreen(int x, int y)
{
	vl_currentBackend->scrollSurface(vl_emuegavgaadapter.screen, x, y);
}

static long vl_lastFrameTime;

int VL_GetTics(int wait)
{
	int tics = 0;
	do
	{
		tics = SD_GetTimeCount() - vl_lastFrameTime;
	} while (tics < wait);
	return tics;
}

void VL_DelayTics(int tics)
{
	vl_currentBackend->waitVBLs(tics);
}

void VL_Yield()
{
	vl_currentBackend->waitVBLs(0);
}

static int vl_scrollXpixels = 0;
static int vl_scrollYpixels = 0;

void VL_SetScrollCoords(int x, int y)
{
	if (!vl_noPan)
		vl_scrollXpixels = x;
	vl_scrollYpixels = y;
}

int VL_GetScrollX(void)
{
	return vl_scrollXpixels;
}

int VL_GetScrollY(void)
{
	return vl_scrollYpixels;
}

// The full on-screen region, including overscan border.
int vl_fullRgn_x;
int vl_fullRgn_y;
int vl_fullRgn_w;
int vl_fullRgn_h;

// The region of fullRgn, excluding overscan border.
int vl_renderRgn_x;
int vl_renderRgn_y;
int vl_renderRgn_w;
int vl_renderRgn_h;

// The integer scaled render size.
int vl_integerWidth;
int vl_integerHeight;

void VL_CalculateRenderRegions(int realW, int realH)
{
	if (vl_isAspectCorrected)
	{
		/* HACK: Naturally, the ratio to compare to may be 4:3,
		 * but we use the default window dimensions instead
		 * (so 4:3 covers the contents without the overscan border).
		 */
		if (realW * VL_DEFAULT_WINDOW_HEIGHT(2) > realH * VL_DEFAULT_WINDOW_WIDTH(2)) // Wider than default ratio
		{
			vl_fullRgn_w = realH * VL_DEFAULT_WINDOW_WIDTH(2) / VL_DEFAULT_WINDOW_HEIGHT(2);
			vl_fullRgn_h = realH;
			vl_fullRgn_x = (realW - vl_fullRgn_w) / 2;
			vl_fullRgn_y = 0;
		}
		else // Thinner or equal to default ratio
		{
			vl_fullRgn_w = realW;
			vl_fullRgn_h = realW * VL_DEFAULT_WINDOW_HEIGHT(2) / VL_DEFAULT_WINDOW_WIDTH(2);
			vl_fullRgn_x = 0;
			vl_fullRgn_y = (realH - vl_fullRgn_h) / 2;
		}
	}
	else
	{
		vl_fullRgn_w = realW;
		vl_fullRgn_h = realH;
		vl_fullRgn_x = 0;
		vl_fullRgn_y = 0;
	}


	if (vl_isIntegerScaled)
	{
		vl_fullRgn_w = CK_Cross_max((vl_fullRgn_w / VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER) * VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER);
		vl_fullRgn_h = CK_Cross_max((vl_fullRgn_h / VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER) * VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER, VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER);
		vl_fullRgn_x = (realW - vl_fullRgn_w)/2;
		vl_fullRgn_y = (realH - vl_fullRgn_h)/2;
		vl_integerWidth = vl_fullRgn_w; 
		vl_integerHeight = vl_fullRgn_h;
	}
	else
	{
		vl_integerWidth = CK_Cross_max((vl_fullRgn_w / VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER) * VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER);
		vl_integerHeight = CK_Cross_max((vl_fullRgn_h / VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER) * VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER);
	}

	vl_renderRgn_x =
		vl_integerWidth * VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH /
		(VL_VGA_GFX_WIDTH_SCALEFACTOR * VL_EGAVGA_GFX_WIDTH +
			VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH + VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH);
	vl_renderRgn_y =
		vl_integerHeight * VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT /
		(VL_VGA_GFX_HEIGHT_SCALEFACTOR * VL_EGAVGA_GFX_HEIGHT +
			VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT + VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT);
	// Tricky calculations that preserve symmetry for the VGA
	vl_renderRgn_w = vl_integerWidth -
		vl_integerWidth * (VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH + VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH) /
			(VL_VGA_GFX_WIDTH_SCALEFACTOR * VL_EGAVGA_GFX_WIDTH +
				VL_VGA_GFX_SCALED_LEFTBORDER_WIDTH + VL_VGA_GFX_SCALED_RIGHTBORDER_WIDTH);
	vl_renderRgn_h = vl_integerHeight -
		vl_integerHeight * (VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT + VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT) /
			(VL_VGA_GFX_HEIGHT_SCALEFACTOR * VL_EGAVGA_GFX_HEIGHT +
				VL_VGA_GFX_SCALED_TOPBORDER_HEIGHT + VL_VGA_GFX_SCALED_BOTTOMBORDER_HEIGHT);
}

int VL_CalculateDefaultWindowScale(int desktopW, int desktopH)
{
	int scale = 1;
	while (true)
	{
		if (VL_DEFAULT_WINDOW_WIDTH(scale + 1) > desktopW)
			break;
		// Stricter bounds for height in order to accommodate a title bar.
		if (VL_DEFAULT_WINDOW_HEIGHT(scale + 1) >= desktopH)
			break;
		scale++;
	}
	return scale;
}

void VL_ClearScreen(int colour)
{
	int screenWidth, screenHeight;
	if (!vl_emuegavgaadapter.screen)
		return;
	vl_currentBackend->getSurfaceDimensions(vl_emuegavgaadapter.screen,
		&screenWidth, &screenHeight);
	VL_ScreenRect(0, 0, screenWidth, screenHeight, colour);
}

void VL_SetMapMask(int mapmask)
{
	vl_mapMask = mapmask & 0xF;
}

int VL_GetActiveBuffer()
{
	return vl_currentBackend->getActiveBufferId(vl_emuegavgaadapter.screen);
}

int VL_GetNumBuffers()
{
	return vl_currentBackend->getNumBuffers(vl_emuegavgaadapter.screen);
}

void VL_FixRefreshBuffer()
{
	vl_currentBackend->syncBuffers(vl_emuegavgaadapter.screen);
}

void VL_UpdateRect(int x, int y, int w, int h)
{
	vl_currentBackend->updateRect(vl_emuegavgaadapter.screen, x, y, w, h);
}

static bool vl_swapOnNextPresent = false;

void VL_SwapOnNextPresent()
{
	vl_swapOnNextPresent = true;
}

void VL_Present()
{
	vl_lastFrameTime = SD_GetTimeCount();
	vl_currentBackend->present(vl_emuegavgaadapter.screen, vl_scrollXpixels, vl_scrollYpixels, !vl_swapOnNextPresent);
	vl_swapOnNextPresent = false;
}
