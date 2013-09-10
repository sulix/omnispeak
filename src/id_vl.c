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

#include "id_mm.h"
#include "id_ca.h"
#include "id_vl.h"

#include <SDL.h>

#define max(a,b) (((a) < (b))?(b):(a))
#define min(a,b) (((a) < (b))?(a):(b))

static int vl_memused, vl_numsurfaces;


VL_EGAPaletteEntry VL_EGAPalette[16];
void VL_SetPalEntry(int id, uint8_t r, uint8_t g, uint8_t b)
{
	VL_EGAPalette[id].r = r;
	VL_EGAPalette[id].g = g;
	VL_EGAPalette[id].b = b;
}

// Sets up the EGA palette used in the original keen games.
void VL_SetDefaultPalette()
{
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
}
	

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

void VL_UnmaskedToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h)
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

			dstptr[(sy+y)*pitch+(sx+x)] = pixel;
		}
	}		
}

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

void VL_MaskedToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h)
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

			dstptr[(sy+y)*pitch+(sx+x)] = pixel & ((srcptr_a[plane_off] & plane_bit)?0xF0:0x00);
		}
	}		
}

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

void VL_MaskedBlitToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h)
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

			dstptr[(sy+y)*pitch+(sx+x)] = pixel;
		}
	}		
}

void VL_MaskedBlitClipToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_a = (uint8_t*)src;
	uint8_t *srcptr_b = srcptr_a + (w/8)*h;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;
	int initialX = max(-x,0);
	int initialY = max(-y,0);
	int finalW = min(max(dw-x,0), w);
	int finalH = min(max(dh-y,0), h);

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

void VL_MaskedBlitClipToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr_a = (uint8_t*)src;
	uint8_t *srcptr_b = srcptr_a + (w/8)*h;
	uint8_t *srcptr_g = srcptr_b + (w/8)*h;
	uint8_t *srcptr_r = srcptr_g + (w/8)*h;
	uint8_t *srcptr_i = srcptr_r + (w/8)*h;
	int initialX = max(-x,0);
	int initialY = max(-y,0);
	int finalW = min(max(dw-x,0), w);
	int finalH = min(max(dh-y,0), h);

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

			dstptr[(sy+y)*pitch+(sx+x)] = pixel;
		}
	}		
}


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

void VL_1bppToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour)
{
	uint8_t *dstptr = (uint8_t*)dest;
	uint8_t *srcptr= (uint8_t*)src;

	for(int sy = 0; sy < h; ++sy)
	{
		for(int sx = 0; sx < w; ++sx)
		{
			int plane_off = (sy * w + sx) >> 3;
			int plane_bit = 1<<(7-((sy * w + sx) & 7));
			
			int pixel = ((srcptr[plane_off] & plane_bit)?colour:colour&0xF0);

			dstptr[(sy+y)*pitch+(sx+x)] = pixel;
		}
	}		
}

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

void VL_1bppBlitToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour)
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
			
			dstptr[(sy+y)*pitch+(sx+x)] = colour;
		}
	}		
}

static void *vl_screen;
static VL_Backend *vl_currentBackend;

int VL_MemUsed()
{
	return vl_memused;
}

int VL_NumSurfaces()
{
	return vl_numsurfaces;
}

void VL_InitScreen()
{
	VL_SetDefaultPalette();
	vl_currentBackend = VL_SDL2GL_GetBackend();	
	vl_memused = 0;
	vl_numsurfaces = 1;
	vl_currentBackend->setVideoMode(320,200);
	vl_screen = vl_currentBackend->createSurface(21*16,14*16,VL_SurfaceUsage_FrontBuffer);
	vl_memused += vl_currentBackend->getSurfaceMemUse(vl_screen);
}

void *VL_CreateSurface(int w, int h)
{
	void *s = vl_currentBackend->createSurface(w,h,VL_SurfaceUsage_Default);
	vl_memused += vl_currentBackend->getSurfaceMemUse(s);
	vl_numsurfaces++;
	return s;
}

void VL_SurfaceRect(void *dst, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->surfaceRect(dst,x,y,w,h,colour);
}

void VL_ScreenRect(int x, int y, int w, int h, int colour)
{
	vl_currentBackend->surfaceRect(vl_screen,x,y,w,h,colour);
}

void VL_SurfaceToSurface(void *src, void *dst, int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSurface(src, dst, x, y, sx, sy, sw, sh);
}

void VL_SurfaceToScreen(void *src, int x, int y, int sx, int sy, int sw, int sh)
{
	vl_currentBackend->surfaceToSurface(src,vl_screen, x, y, sx, sy, sw, sh);
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
	vl_currentBackend->unmaskedToSurface(src, vl_screen, x, y, w, h);
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
	vl_currentBackend->maskedToSurface(src, vl_screen, x, y, w, h);
}

void VL_MaskedBlitToScreen(void *src, int x, int y, int w, int h)
{
	vl_currentBackend->maskedBlitToSurface(src, vl_screen, x, y, w, h);
}

void VL_1bppToScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitToSurface(src, vl_screen, x, y, w, h, colour);
}

void VL_1bppBlitToScreen(void *src, int x, int y, int w, int h, int colour)
{
	vl_currentBackend->bitBlitToSurface(src, vl_screen, x, y, w, h, colour);
}

static long vl_lastFrameTime;

int VL_GetTics(int wait)
{
	int tics;
	do
	{
		tics = (SDL_GetTicks() - vl_lastFrameTime)/(1000/70);
	} while (tics < wait);
	return tics;
}

void VL_DelayTics(int tics)
{
	SDL_Delay(tics*1000/70);
}

static int vl_scrollXpixels;
static int vl_scrollYpixels;

void VL_SetScrollCoords(int x, int y)
{
	vl_scrollXpixels = x;
	vl_scrollYpixels = y;
}

void VL_Present()
{
	vl_lastFrameTime = SDL_GetTicks();
	vl_currentBackend->present(vl_screen, vl_scrollXpixels, vl_scrollYpixels);
}
