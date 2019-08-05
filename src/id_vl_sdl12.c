#include "assert.h"
#include <SDL.h>
#include <string.h>
#include "id_vl.h"
#include "id_vl_private.h"
#include "ck_cross.h"

static SDL_Surface *vl_sdl12_screenSurface;
static SDL_Rect vl_sdl12_screenBorderedRect;
static SDL_Rect vl_sdl12_screenWholeRect;
static int vl_sdl12_desktopWidth = -1, vl_sdl12_desktopHeight = -1;
static void VL_SDL12_SetVideoMode(int mode)
{
	if (mode == 0x0D)
	{
		// WARNING - This may be set ONLY before the first call to SDL_SetVideoMode!
		if ((vl_sdl12_desktopWidth < 0) || (vl_sdl12_desktopHeight < 0))
		{
			const SDL_VideoInfo *vidinfo = SDL_GetVideoInfo();
			if (vidinfo)
			{
				vl_sdl12_desktopWidth = vidinfo->current_w;
				vl_sdl12_desktopHeight = vidinfo->current_h;
			}
			else // Just in case this fails...
			{
				vl_sdl12_desktopWidth = 2 * VL_EGAVGA_GFX_WIDTH;
				vl_sdl12_desktopHeight = 2 * VL_EGAVGA_GFX_HEIGHT;
			}
		}

		if (vl_isFullScreen)
		{
			vl_sdl12_screenSurface = SDL_SetVideoMode(vl_sdl12_desktopWidth,
				vl_sdl12_desktopHeight,
				0, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN);
			vl_sdl12_screenWholeRect.w = VL_VGA_GFX_SHRUNK_WIDTH_PLUS_BORDER;
			vl_sdl12_screenWholeRect.h = VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER;
			vl_sdl12_screenWholeRect.x = (vl_sdl12_desktopWidth - vl_sdl12_screenWholeRect.w) / 2;
			vl_sdl12_screenWholeRect.y = (vl_sdl12_desktopHeight - vl_sdl12_screenWholeRect.h) / 2;
			vl_sdl12_screenBorderedRect.w = VL_EGAVGA_GFX_WIDTH;
			vl_sdl12_screenBorderedRect.h = VL_EGAVGA_GFX_HEIGHT;
			vl_sdl12_screenBorderedRect.x = vl_sdl12_screenWholeRect.x + VL_VGA_GFX_SHRUNK_LEFTBORDER_WIDTH;
			vl_sdl12_screenBorderedRect.y = vl_sdl12_screenWholeRect.y + VL_VGA_GFX_SHRUNK_TOPBORDER_HEIGHT;
		}
		else
		{
			vl_sdl12_screenSurface = SDL_SetVideoMode(VL_VGA_GFX_SHRUNK_WIDTH_PLUS_BORDER,
				VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER,
				0, SDL_DOUBLEBUF | SDL_HWSURFACE);
			vl_sdl12_screenWholeRect.x = 0;
			vl_sdl12_screenWholeRect.y = 0;
			vl_sdl12_screenWholeRect.w = VL_VGA_GFX_SHRUNK_WIDTH_PLUS_BORDER;
			vl_sdl12_screenWholeRect.h = VL_VGA_GFX_SHRUNK_HEIGHT_PLUS_BORDER;
			vl_sdl12_screenBorderedRect.x = VL_VGA_GFX_SHRUNK_LEFTBORDER_WIDTH;
			vl_sdl12_screenBorderedRect.y = VL_VGA_GFX_SHRUNK_TOPBORDER_HEIGHT;
			vl_sdl12_screenBorderedRect.w = VL_EGAVGA_GFX_WIDTH;
			vl_sdl12_screenBorderedRect.h = VL_EGAVGA_GFX_HEIGHT;
		}
		SDL_WM_SetCaption(VL_WINDOW_TITLE, VL_WINDOW_TITLE);
		// Hide mouse cursor
		SDL_ShowCursor(0);
	}
	else
	{
		SDL_ShowCursor(1);
		// The main window will be freed by SDL_Quit()
	}
}

static void VL_SDL12_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL12_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	SDL_Surface *s;
	s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
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
	return surf->pitch * surf->h;
}

static void VL_SDL12_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL12_RefreshPaletteAndBorderColor(void *screen)
{
	SDL_Surface *surf = (SDL_Surface *)screen;
	static SDL_Color sdl12_palette[16];

	for (int i = 0; i < 16; i++)
	{
		sdl12_palette[i].r = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0];
		sdl12_palette[i].g = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1];
		sdl12_palette[i].b = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2];
	}
	SDL_SetPalette(surf, SDL_LOGPAL, sdl12_palette, 0, 16);
	SDL_FillRect(vl_sdl12_screenSurface, &vl_sdl12_screenWholeRect,
		SDL_MapRGB(vl_sdl12_screenSurface->format,
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2]));
}

static int VL_SDL12_SurfacePGet(void *surface, int x, int y)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_LockSurface(surf);
	int col = ((uint8_t *)surf->pixels)[y * surf->pitch + x];
	SDL_UnlockSurface(surf);
	return col;
}

static void VL_SDL12_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_Rect rect = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
	SDL_FillRect(surf, &rect, colour);
}

static void VL_SDL12_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	for (int py = y; py < y + h; py++)
		for (int px = x; px < x + w; px++)
		{
			uint8_t *p = ((uint8_t *)surf->pixels) + py * surf->w + px;
			*p &= ~mapmask;
			*p |= colour;
		}
}

static void VL_SDL12_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	// Sadly we cannot naively use SDL_BlitSurface, since the surfaces may
	// both be 8-bit but with different palettes, which we ignore.
	SDL_Surface *surf = (SDL_Surface *)src_surface;
	SDL_Surface *dest = (SDL_Surface *)dst_surface;
	for (int _y = sy; _y < sy + sh; ++_y)
	{
		memcpy(((uint8_t *)dest->pixels) + (_y - sy + y) * dest->w + x, ((uint8_t *)surf->pixels) + _y * surf->w + sx, sw);
	}
#if 0
	SDL_Rect srcr = {sx,sy,sw,sh};
	SDL_Rect dstr = {x,y,sw,sh};
	SDL_BlitSurface(surf,&srcr, dest, &dstr);
#endif
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
			memmove((uint8_t *)srf->pixels + ((yi + y) * srf->pitch + x), (uint8_t *)srf->pixels + ((sy + yi) * srf->pitch + sx), sw);
		}
	}
	else
	{
		for (int yi = sh - 1; yi >= 0; --yi)
		{
			memmove((uint8_t *)srf->pixels + ((yi + y) * srf->pitch + x), (uint8_t *)srf->pixels + ((sy + yi) * srf->pitch + sx), sw);
		}
	}

	SDL_UnlockSurface(srf);
}

static void VL_SDL12_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, colour, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppXorWithPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppBlitToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppInvBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL12_ScrollSurface(void *surface, int x, int y)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	int dx = 0, dy = 0, sx = 0, sy = 0;
	int w = surf->w - CK_Cross_max(x, -x), h = surf->h - CK_Cross_max(y, -y);
	if (x > 0)
	{
		dx = 0;
		sx = x;
	}
	else
	{
		dx = -x;
		sx = 0;
	}
	if (y > 0)
	{
		dy = 0;
		sy = y;
	}
	else
	{
		dy = -y;
		sy = 0;
	}
	VL_SDL12_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL12_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	// TODO: Verify this is a VL_SurfaceUsage_FrontBuffer
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_Rect srcr = {(Sint16)scrlX, (Sint16)scrlY, vl_sdl12_screenBorderedRect.w, vl_sdl12_screenBorderedRect.h};
	SDL_BlitSurface(surf, &srcr, vl_sdl12_screenSurface, &vl_sdl12_screenBorderedRect);
	//VL_SDL12_SurfaceToSurface(surface, vl_sdl12_screenSurface, 0, 0, scrlX, scrlY, vl_sdl12_screenWidth, vl_sdl12_screenHeight);
	SDL_Flip(vl_sdl12_screenSurface);
}

static int VL_SDL12_GetActiveBufferId(void *surface)
{
	(void)surface;
	return 0;
}

static int VL_SDL12_GetNumBuffers(void *surface)
{
	(void)surface;
	return 1;
}

static void VL_SDL12_FlushParams()
{
}

static void VL_SDL12_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl12_backend =
	{
		/*.setVideoMode =*/&VL_SDL12_SetVideoMode,
		/*.createSurface =*/&VL_SDL12_CreateSurface,
		/*.destroySurface =*/&VL_SDL12_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL12_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL12_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL12_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL12_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL12_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL12_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL12_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL12_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL12_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL12_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL12_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL12_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL12_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL12_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL12_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL12_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL12_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL12_ScrollSurface,
		/*.present =*/&VL_SDL12_Present,
		/*.getActiveBufferId =*/&VL_SDL12_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL12_GetNumBuffers,
		/*.flushParams =*/&VL_SDL12_FlushParams,
		/*.waitVBLs =*/&VL_SDL12_WaitVBLs};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl12_backend;
}
