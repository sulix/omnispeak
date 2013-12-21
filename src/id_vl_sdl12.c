#include "id_vl.h"
#include <SDL.h>

static SDL_Surface *vl_sdl12_screenSurface;
static int vl_sdl12_screenWidth;
static int vl_sdl12_screenHeight;

static void VL_SDL12_SetVideoMode(int w, int h)
{
	vl_sdl12_screenSurface = SDL_SetVideoMode(w,h,32,SDL_DOUBLEBUF|SDL_HWSURFACE);
	vl_sdl12_screenWidth = w;
	vl_sdl12_screenHeight = h;
}

static void VL_SDL12_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL12_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	SDL_Surface *s;
	s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
	//VL_SDL12_SurfaceRect(s, 0, 0, w, h, 0);
	return s;
}

static void VL_SDL12_DestroySurface(void *surface)
{
	//TODO: Implement
}

static long VL_SDL12_GetSurfaceMemUse(void *surface)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	return surf->w*surf->h*4;
}

static void VL_SDL12_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface*) dst_surface;
	SDL_Rect rect = {x,y,w,h};
	uint32_t sdlcolour = 0xff000000 | (VL_EGAPalette[colour].r << 16) | (VL_EGAPalette[colour].g << 8) | (VL_EGAPalette[colour].b);
	SDL_FillRect(surf,&rect,sdlcolour);
}

static void VL_SDL12_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	SDL_Surface *surf = (SDL_Surface *)src_surface;
	SDL_Surface *dest = (SDL_Surface *)dst_surface;
	SDL_Rect srcr = {sx,sy,sw,sh};
	SDL_Rect dstr = {x,y,sw,sh};
	SDL_BlitSurface(surf,&srcr, dest, &dstr);
}

static void VL_SDL12_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	SDL_Surface *srf = (SDL_Surface *)surface;
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

static void VL_SDL12_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h) {
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToRGB(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedToRGBA(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedBlitClipToRGB(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToRGBA(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppBlitToRGB(src, surf->pixels, x, y, surf->pitch, w,h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_Present(void *surface, int scrlX, int scrlY)
{
	// TODO: Verify this is a VL_SurfaceUsage_FrontBuffer
	SDL_Surface *surf = (SDL_Surface *)surface;
	VL_SDL12_SurfaceToSurface(surface, vl_sdl12_screenSurface, 0, 0, scrlX, scrlY, vl_sdl12_screenWidth, vl_sdl12_screenHeight);
	SDL_Flip(vl_sdl12_screenSurface);
}

VL_Backend vl_sdl12_backend =
{
	.setVideoMode = &VL_SDL12_SetVideoMode,
	.createSurface = &VL_SDL12_CreateSurface,
	.destroySurface = &VL_SDL12_DestroySurface,
	.getSurfaceMemUse = &VL_SDL12_GetSurfaceMemUse,
	.surfaceRect = &VL_SDL12_SurfaceRect,
	.surfaceToSurface = &VL_SDL12_SurfaceToSurface,
	.surfaceToSelf = &VL_SDL12_SurfaceToSelf,
	.unmaskedToSurface = &VL_SDL12_UnmaskedToSurface,
	.maskedToSurface = &VL_SDL12_MaskedToSurface,
	.maskedBlitToSurface = &VL_SDL12_MaskedBlitToSurface,
	.bitToSurface = &VL_SDL12_BitToSurface,
	.bitBlitToSurface = &VL_SDL12_BitBlitToSurface,
	.present = &VL_SDL12_Present
};

VL_Backend *VL_SDL12_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl12_backend;
}
