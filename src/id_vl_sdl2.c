#include "assert.h"
#include <SDL.h>
#include <string.h>
#include "id_vl.h"
#include "id_vl_private.h"
#include "ck_cross.h"

static SDL_Window *vl_sdl2_window;
static SDL_Renderer *vl_sdl2_renderer;
static SDL_Texture *vl_sdl2_texture;
static SDL_Palette *vl_sdl2_palette;
static SDL_Surface *vl_sdl2_stagingSurface;
static SDL_Texture *vl_sdl2_scaledTarget;
static SDL_Rect vl_sdl2_screenBorderedRect;
static SDL_Rect vl_sdl2_screenWholeRect;
static bool vl_sdl2_bilinearSupport = true;
static int vl_sdl2_desktopWidth = -1, vl_sdl2_desktopHeight = -1;

static void VL_SDL2_ResizeWindow()
{
	int realWinH, realWinW, curW, curH;
	SDL_GetRendererOutputSize(vl_sdl2_renderer, &realWinW, &realWinH);
	VL_CalculateRenderRegions(realWinW, realWinH);

	if (vl_sdl2_scaledTarget)
	{
		// Check if the scaled render target is the correct size.
		SDL_QueryTexture(vl_sdl2_scaledTarget, 0, 0, &curW, &curH);
		if (curW == vl_integerWidth && curH == vl_integerHeight)
		{
			return;
		}

		// We need to recreate the scaled render target. Do so.
		SDL_DestroyTexture(vl_sdl2_scaledTarget);
		vl_sdl2_scaledTarget = 0;
	}

	if (!vl_isIntegerScaled)
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		vl_sdl2_scaledTarget = SDL_CreateTexture(vl_sdl2_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, vl_integerWidth, vl_integerHeight);
	}
}

static void VL_SDL2_SetVideoMode(int mode)
{
	if (mode == 0x0D)
	{
		// Here is how the dimensions of the window are currently picked:
		// 1. The emulated 320x200 sub-window is first zoomed
		// by a factor of 3 (for each dimension) to 960x600.
		// 2. The height is then multiplied by 1.2, so the internal contents
		// (without the borders) have the aspect ratio of 4:3.
		//
		// There are a few more tricks in use to handle the overscan border
		// and VGA line doubling.
		vl_sdl2_window = SDL_CreateWindow(VL_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			VL_DEFAULT_WINDOW_WIDTH, VL_DEFAULT_WINDOW_HEIGHT,
			SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | (vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));

		SDL_SetWindowMinimumSize(vl_sdl2_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

		//VL_SDL2GL_SetIcon(vl_sdl2_window);

#ifdef VL_SDL2_REQUEST_SOFTWARE
		vl_sdl2_renderer = SDL_CreateRenderer(vl_sdl2_window, -1, SDL_RENDERER_SOFTWARE);
#else
		vl_sdl2_renderer = SDL_CreateRenderer(vl_sdl2_window, -1, 0);
#endif

		SDL_RendererInfo info;
		SDL_GetRendererInfo(vl_sdl2_renderer, &info);
		if (info.flags & SDL_RENDERER_SOFTWARE)
			vl_sdl2_bilinearSupport = false;

		if (!vl_isIntegerScaled && !vl_sdl2_bilinearSupport)
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Using SDL2 software renderer without integer scaling. Pixel size may be inconsistent.\n");

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		vl_sdl2_texture = SDL_CreateTexture(vl_sdl2_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT);

		vl_sdl2_palette = SDL_AllocPalette(256);

		// As we can't do on-GPU palette conversions with SDL2,
		// we do a PAL8->RGBA conversion of the visible area to this surface each frame.
		vl_sdl2_stagingSurface = SDL_CreateRGBSurface(0, VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT, 32, 0, 0, 0, 0);

		VL_SDL2_ResizeWindow();
		SDL_ShowCursor(0);
	}
	else
	{
		SDL_ShowCursor(1);
		SDL_DestroyTexture(vl_sdl2_scaledTarget);
		SDL_DestroyTexture(vl_sdl2_texture);
		SDL_DestroyRenderer(vl_sdl2_renderer);
		SDL_DestroyWindow(vl_sdl2_window);
	}
}

static void VL_SDL2_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL2_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	SDL_Surface *s;
	s = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
	return s;
}

static void VL_SDL2_DestroySurface(void *surface)
{
	//TODO: Implement
}

static long VL_SDL2_GetSurfaceMemUse(void *surface)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	if (!surf)
		return 0;
	return surf->pitch * surf->h;
}

static void VL_SDL2_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	if (!surf)
		return;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL2_RefreshPaletteAndBorderColor(void *screen)
{
	SDL_Surface *surf = (SDL_Surface *)screen;
	static SDL_Color sdl2_palette[16];

	for (int i = 0; i < 16; i++)
	{
		sdl2_palette[i].r = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0];
		sdl2_palette[i].g = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1];
		sdl2_palette[i].b = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2];
	}
	SDL_SetPaletteColors(vl_sdl2_palette, sdl2_palette, 0, 16);
	SDL_SetSurfacePalette(surf, vl_sdl2_palette);
}

static int VL_SDL2_SurfacePGet(void *surface, int x, int y)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_LockSurface(surf);
	int col = ((uint8_t *)surf->pixels)[y * surf->pitch + x];
	SDL_UnlockSurface(surf);
	return col;
}

static void VL_SDL2_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_Rect rect = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
	SDL_FillRect(surf, &rect, colour);
}

static void VL_SDL2_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
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

static void VL_SDL2_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
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

static void VL_SDL2_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
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

static void VL_SDL2_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, colour, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppXorWithPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppBlitToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppInvBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL2_ScrollSurface(void *surface, int x, int y)
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
	VL_SDL2_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL2_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	// TODO: Verify this is a VL_SurfaceUsage_FrontBuffer
	VL_SDL2_ResizeWindow();
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_Rect srcr = {(Sint16)scrlX, (Sint16)scrlY, VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT};
	SDL_Rect integerRect = {(Sint16)vl_renderRgn_x, (Sint16)vl_renderRgn_y, vl_integerWidth, vl_integerHeight};
	SDL_Rect renderRect = {(Sint16)vl_renderRgn_x, (Sint16)vl_renderRgn_y, vl_renderRgn_w, vl_renderRgn_h};
	SDL_Rect fullRect = {(Sint16)vl_fullRgn_x, (Sint16)vl_fullRgn_y, vl_fullRgn_w, vl_fullRgn_h};

	SDL_BlitSurface(surf, &srcr, vl_sdl2_stagingSurface, 0);
	SDL_UpdateTexture(vl_sdl2_texture, 0, vl_sdl2_stagingSurface->pixels, vl_sdl2_stagingSurface->pitch);
	if (vl_sdl2_scaledTarget)
	{
		SDL_SetRenderTarget(vl_sdl2_renderer, vl_sdl2_scaledTarget);
		SDL_SetRenderDrawColor(vl_sdl2_renderer,
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2],
			255);
		SDL_RenderClear(vl_sdl2_renderer);
		SDL_RenderCopy(vl_sdl2_renderer, vl_sdl2_texture, 0, &renderRect);
		SDL_SetRenderTarget(vl_sdl2_renderer, 0);
		SDL_SetRenderDrawColor(vl_sdl2_renderer, 0, 0, 0, 255);
		SDL_RenderClear(vl_sdl2_renderer);

		SDL_RenderSetViewport(vl_sdl2_renderer, 0);
		SDL_RenderCopy(vl_sdl2_renderer, vl_sdl2_scaledTarget, 0, &fullRect);
	}
	else
	{
		SDL_SetRenderTarget(vl_sdl2_renderer, 0);
		SDL_RenderSetViewport(vl_sdl2_renderer, 0);
		SDL_SetRenderDrawColor(vl_sdl2_renderer, 0, 0, 0, 255);
		SDL_RenderClear(vl_sdl2_renderer);

		SDL_RenderSetViewport(vl_sdl2_renderer, 0);
		SDL_RenderSetViewport(vl_sdl2_renderer, &fullRect);
		SDL_SetRenderDrawColor(vl_sdl2_renderer,
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2],
			255);
		SDL_RenderFillRect(vl_sdl2_renderer, 0);
		SDL_RenderCopy(vl_sdl2_renderer, vl_sdl2_texture, 0, &renderRect);

	}

	SDL_RenderPresent(vl_sdl2_renderer);
}

static int VL_SDL2_GetActiveBufferId(void *surface)
{
	(void)surface;
	return 0;
}

static int VL_SDL2_GetNumBuffers(void *surface)
{
	(void)surface;
	return 1;
}

static void VL_SDL2_FlushParams()
{
	SDL_SetWindowFullscreen(vl_sdl2_window, vl_isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	SDL_SetWindowMinimumSize(vl_sdl2_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);
	VL_SDL2_ResizeWindow();
}

static void VL_SDL2_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl2_backend =
	{
		/*.setVideoMode =*/&VL_SDL2_SetVideoMode,
		/*.createSurface =*/&VL_SDL2_CreateSurface,
		/*.destroySurface =*/&VL_SDL2_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL2_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL2_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL2_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL2_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL2_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL2_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL2_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL2_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL2_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL2_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL2_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL2_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL2_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL2_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL2_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL2_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL2_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL2_ScrollSurface,
		/*.present =*/&VL_SDL2_Present,
		/*.getActiveBufferId =*/&VL_SDL2_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL2_GetNumBuffers,
		/*.flushParams =*/&VL_SDL2_FlushParams,
		/*.waitVBLs =*/&VL_SDL2_WaitVBLs,
};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl2_backend;
}
