#include "id_mm.h"
#include "id_ca.h"

#include <SDL/SDL.h>

#define max(a,b) (((a) < (b))?(b):(a))
#define min(a,b) (((a) < (b))?(a):(b))

static int vl_memused, vl_numsurfaces;

typedef struct VL_EGAPaletteEntry
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} VL_EGAPaletteEntry;

static VL_EGAPaletteEntry VL_EGAPalette[16];

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


static SDL_Surface *vl_screen;

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
	SDL_Init(SDL_INIT_VIDEO);
	vl_memused = 0;
	vl_numsurfaces = 1;
	vl_screen = SDL_SetVideoMode(320,200,32,0);
	vl_memused += 320*200*4; //320x200x32bit
}

void *VL_CreateSurface(int w, int h)
{
	//TODO: Big-endian support and Optimize flags (alpha is rarely needed)
	SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	vl_memused += w*h*4;
	vl_numsurfaces++;
	return s;
}

void VL_SurfaceRect(void *dst, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface*) dst;
	SDL_Rect rect = {x,y,w,h};
	uint32_t sdlcolour = 0xff000000 | (VL_EGAPalette[colour].r << 16) | (VL_EGAPalette[colour].g << 8) | (VL_EGAPalette[colour].b);
	SDL_FillRect(surf,&rect,sdlcolour);
}

void VL_ScreenRect(int x, int y, int w, int h, int colour)
{
	VL_SurfaceRect(vl_screen,x,y,w,h,colour);
}

void VL_SurfaceToSurface(void *src, void *dst, int x, int y, int sx, int sy, int sw, int sh)
{
	SDL_Surface *surf = (SDL_Surface *)src;
	SDL_Surface *dest = (SDL_Surface *)dst;
	SDL_Rect srcr = {sx,sy,sw,sh};
	SDL_Rect dstr = {x,y,sw,sh};
	SDL_BlitSurface(surf,&srcr, dest, &dstr);
}

void VL_SurfaceToScreen(void *src, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_SurfaceToSurface(src,vl_screen, x, y, sx, sy, sw, sh);
}

void VL_SurfaceToSelf(void *surf, int x, int y, int sx, int sy, int sw, int sh)
{
	SDL_Surface *srf = (SDL_Surface *)surf;
	SDL_LockSurface(srf);
	bool directionX = sx > x;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(srf->pixels+((yi+y)*srf->pitch+x*4),srf->pixels+((sy+yi)*srf->pitch+sx*4),sw*4);
		}
	}
	else	
	{
		for (int yi = sh-1; yi >= 0; --yi)
		{
			memmove(srf->pixels+((yi+y)*srf->pitch+x*4),srf->pixels+((sy+yi)*srf->pitch+sx*4),sw*4);
		}
	}


	SDL_UnlockSurface(srf);

}

void VL_UnmaskedToSurface(void *src, void *dest, int x, int y, int w, int h) {
	SDL_Surface *surf = (SDL_Surface *)dest;
	SDL_LockSurface(vl_screen);
	VL_UnmaskedToRGB(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(vl_screen);
}



void VL_UnmaskedToScreen(void *src, int x, int y, int w, int h) {
	SDL_LockSurface(vl_screen);
	VL_UnmaskedToRGB(src, vl_screen->pixels, x, y, vl_screen->pitch, w, h);
	SDL_UnlockSurface(vl_screen);
}

void VL_MaskedToSurface(void *src, void *dest, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dest;
	SDL_LockSurface(surf);
	VL_MaskedToRGBA(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

void VL_MaskedBlitToSurface(void *src, void *dest, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dest;
	SDL_LockSurface(surf);
	VL_MaskedBlitToRGB(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

// This is not the function you are looking for.
// It does not perform masking, simply overwrites the screen's alpha channel.
void VL_MaskedToScreen(void *src, int x, int y, int w, int h)
{
	SDL_LockSurface(vl_screen);
	VL_MaskedToRGBA(src, vl_screen->pixels, x, y, vl_screen->pitch, w, h);
	SDL_UnlockSurface(vl_screen);
}

void VL_MaskedBlitToScreen(void *src, int x, int y, int w, int h)
{
	// Masked blits to screen are clipped.
	SDL_LockSurface(vl_screen);
	VL_MaskedBlitClipToRGB(src, vl_screen->pixels, x, y, vl_screen->pitch, w, h,320,200);
	SDL_UnlockSurface(vl_screen);
}

void VL_1bppToScreen(void *src, int x, int y, int w, int h, int colour)
{
	SDL_LockSurface(vl_screen);
	VL_1bppToRGBA(src, vl_screen->pixels, x, y, vl_screen->pitch, w, h, colour);
	SDL_UnlockSurface(vl_screen);
}

void VL_1bppBlitToScreen(void *src, int x, int y, int w, int h, int colour)
{
	SDL_LockSurface(vl_screen);
	VL_1bppBlitToRGB(src, vl_screen->pixels, x, y, vl_screen->pitch, w,h, colour);
	SDL_UnlockSurface(vl_screen);
}

static long vl_lastFrameTime;

int VL_GetTics(bool wait)
{
	int tics;
	do
	{
		tics = (SDL_GetTicks() - vl_lastFrameTime)/(1000/70);
	} while (!tics || !wait);
	return tics;
}

void VL_Present()
{
	vl_lastFrameTime = SDL_GetTicks();
	SDL_Flip(vl_screen);
}
