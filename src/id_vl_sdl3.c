#include "assert.h"
#include <SDL3/SDL.h>
#include <string.h>
#include "id_vl.h"
#include "id_vl_private.h"
#include "ck_cross.h"

// We need the window to be exported so we can access it in ID_IN.
SDL_Window *vl_sdl3_window;
static SDL_Renderer *vl_sdl3_renderer;
static SDL_Texture *vl_sdl3_texture;
static SDL_Palette *vl_sdl3_palette;
static SDL_Surface *vl_sdl3_stagingSurface;
static SDL_Texture *vl_sdl3_scaledTarget;
static bool vl_sdl3_bilinearSupport = true;

static void VL_SDL3_ResizeWindow()
{
	int realWinH, realWinW, curW, curH;
	SDL_GetCurrentRenderOutputSize(vl_sdl3_renderer, &realWinW, &realWinH);
	VL_CalculateRenderRegions(realWinW, realWinH);

	if (vl_sdl3_scaledTarget)
	{
		// Check if the scaled render target is the correct size.
		SDL_PropertiesID props = SDL_GetTextureProperties(vl_sdl3_scaledTarget);
		curW = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_WIDTH_NUMBER, 0);
		curH = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0);
		if (curW == vl_integerWidth && curH == vl_integerHeight)
		{
			return;
		}

		// We need to recreate the scaled render target. Do so.
		SDL_DestroyTexture(vl_sdl3_scaledTarget);
		vl_sdl3_scaledTarget = 0;
	}

	if (!vl_isIntegerScaled)
	{
		vl_sdl3_scaledTarget = SDL_CreateTexture(vl_sdl3_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, vl_integerWidth, vl_integerHeight);
		SDL_SetTextureScaleMode(vl_sdl3_scaledTarget, SDL_SCALEMODE_LINEAR);

	}
}

void VL_SDL2GL_SetIcon(SDL_Window *wnd);

static void VL_SDL3_SetVideoMode(int mode)
{
	if (mode == 0x0D)
	{
		SDL_Rect desktopBounds;
		const SDL_DisplayID *displayIDs = SDL_GetDisplays(NULL);
		if (SDL_GetDisplayUsableBounds(*displayIDs, &desktopBounds))
		{
			CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't get usable display bounds to calculate default scale: \"%s\"\n", SDL_GetError());
		}
		int scale = VL_CalculateDefaultWindowScale(desktopBounds.w, desktopBounds.h);
		vl_sdl3_window = SDL_CreateWindow(VL_WINDOW_TITLE,
			VL_DEFAULT_WINDOW_WIDTH(scale), VL_DEFAULT_WINDOW_HEIGHT(scale),
			SDL_WINDOW_RESIZABLE | (vl_isFullScreen ? SDL_WINDOW_FULLSCREEN : 0));

		SDL_SetWindowMinimumSize(vl_sdl3_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);

		VL_SDL2GL_SetIcon(vl_sdl3_window);

		SDL_PropertiesID renderProps = SDL_CreateProperties();
		if (vl_swapInterval)
			SDL_SetNumberProperty(renderProps, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, vl_swapInterval);
#ifdef VL_SDL3_REQUEST_SOFTWARE
		SDL_SetStringProperty(renderProps, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_SOFTWARE_RENDERER);
#endif
		SDL_SetPointerProperty(renderProps, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, vl_sdl3_window);
		vl_sdl3_renderer = SDL_CreateRendererWithProperties(renderProps);

		if (!strcmp(SDL_GetRendererName(vl_sdl3_renderer), SDL_SOFTWARE_RENDERER))
			vl_sdl3_bilinearSupport = false;

		if (!vl_isIntegerScaled && !vl_sdl3_bilinearSupport)
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Using SDL3 software renderer without integer scaling. Pixel size may be inconsistent.\n");

		vl_sdl3_texture = SDL_CreateTexture(vl_sdl3_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT);
		SDL_SetTextureScaleMode(vl_sdl3_texture, SDL_SCALEMODE_NEAREST);

		vl_sdl3_palette = SDL_CreatePalette(256);

		// As we can't do on-GPU palette conversions with SDL3,
		// we do a PAL8->RGBA conversion of the visible area to this surface each frame.
		vl_sdl3_stagingSurface = SDL_CreateSurface(VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT, SDL_PIXELFORMAT_RGBA8888);

		VL_SDL3_ResizeWindow();
		SDL_HideCursor();
	}
	else
	{
		SDL_ShowCursor();
		SDL_DestroyTexture(vl_sdl3_scaledTarget);
		SDL_DestroyTexture(vl_sdl3_texture);
		SDL_DestroyRenderer(vl_sdl3_renderer);
		SDL_DestroyWindow(vl_sdl3_window);
	}
}

static void VL_SDL3_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_SDL3_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	SDL_Surface *s;
	s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_INDEX8);
	return s;
}

static void VL_SDL3_DestroySurface(void *surface)
{
	//TODO: Implement
}

static long VL_SDL3_GetSurfaceMemUse(void *surface)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	if (!surf)
		return 0;
	return surf->pitch * surf->h;
}

static void VL_SDL3_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	if (!surf)
		return;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_SDL3_RefreshPaletteAndBorderColor(void *screen)
{
	SDL_Surface *surf = (SDL_Surface *)screen;
	static SDL_Color sdl3_palette[16];

	for (int i = 0; i < 16; i++)
	{
		sdl3_palette[i].r = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][0];
		sdl3_palette[i].g = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][1];
		sdl3_palette[i].b = VL_EGARGBColorTable[vl_emuegavgaadapter.palette[i]][2];
		sdl3_palette[i].a = 255;
	}
	SDL_SetPaletteColors(vl_sdl3_palette, sdl3_palette, 0, 16);
	SDL_SetSurfacePalette(surf, vl_sdl3_palette);
}

static int VL_SDL3_SurfacePGet(void *surface, int x, int y)
{
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_LockSurface(surf);
	int col = ((uint8_t *)surf->pixels)[y * surf->pitch + x];
	SDL_UnlockSurface(surf);
	return col;
}

static void VL_SDL3_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_Rect rect = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
	SDL_FillSurfaceRect(surf, &rect, colour);
}

static void VL_SDL3_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	for (int py = y; py < y + h; py++)
		for (int px = x; px < x + w; px++)
		{
			uint8_t *p = ((uint8_t *)surf->pixels) + py * surf->w + px;
			*p &= ~mapmask;
			*p |= colour;
		}
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	// Sadly we cannot naively use SDL_BlitSurface, since the surfaces may
	// both be 8-bit but with different palettes, which we ignore.
	SDL_Surface *surf = (SDL_Surface *)src_surface;
	SDL_Surface *dest = (SDL_Surface *)dst_surface;

	SDL_Rect srcr = {sx, sy, sw, sh};
	SDL_Rect dstr = {x, y, 0, 0};

	SDL_BlitSurface(surf, &srcr, dest, &dstr);
}

static void VL_SDL3_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	SDL_Surface *srf = (SDL_Surface *)surface;
	SDL_LockSurface(srf);
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

static void VL_SDL3_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_UnmaskedToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedToPAL8(src, surf->pixels, x, y, surf->pitch, w, h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_MaskedBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppToPAL8_PM(src, surf->pixels, x, y, surf->pitch, w, h, colour, mapmask);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppXorWithPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppBlitToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	SDL_Surface *surf = (SDL_Surface *)dst_surface;
	SDL_LockSurface(surf);
	VL_1bppInvBlitClipToPAL8(src, surf->pixels, x, y, surf->pitch, w, h, surf->w, surf->h, colour);
	SDL_UnlockSurface(surf);
}

static void VL_SDL3_ScrollSurface(void *surface, int x, int y)
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
	VL_SDL3_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_SDL3_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	// TODO: Verify this is a VL_SurfaceUsage_FrontBuffer
	VL_SDL3_ResizeWindow();
	SDL_Surface *surf = (SDL_Surface *)surface;
	SDL_Rect srcr = {(Sint16)scrlX, (Sint16)scrlY, VL_EGAVGA_GFX_WIDTH, VL_EGAVGA_GFX_HEIGHT};
	SDL_FRect renderRect = {(float)vl_renderRgn_x, (float)vl_renderRgn_y, (float)vl_renderRgn_w, (float)vl_renderRgn_h};
	SDL_Rect fullRect = {(Sint16)vl_fullRgn_x, (Sint16)vl_fullRgn_y, vl_fullRgn_w, vl_fullRgn_h};
	SDL_FRect fullRectF = {(float)vl_fullRgn_x, (float)vl_fullRgn_y, (float)vl_fullRgn_w, (float)vl_fullRgn_h};

	SDL_BlitSurface(surf, &srcr, vl_sdl3_stagingSurface, 0);
	SDL_LockSurface(vl_sdl3_stagingSurface);
	SDL_UpdateTexture(vl_sdl3_texture, 0, vl_sdl3_stagingSurface->pixels, vl_sdl3_stagingSurface->pitch);
	SDL_UnlockSurface(vl_sdl3_stagingSurface);
	if (vl_sdl3_scaledTarget)
	{
		SDL_SetRenderTarget(vl_sdl3_renderer, vl_sdl3_scaledTarget);
		SDL_SetRenderDrawColor(vl_sdl3_renderer,
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2],
			255);
		SDL_RenderClear(vl_sdl3_renderer);
		SDL_RenderTexture(vl_sdl3_renderer, vl_sdl3_texture, 0, &renderRect);
		SDL_SetRenderTarget(vl_sdl3_renderer, 0);
		SDL_SetRenderDrawColor(vl_sdl3_renderer, 0, 0, 0, 255);
		SDL_RenderClear(vl_sdl3_renderer);

		SDL_SetRenderViewport(vl_sdl3_renderer, 0);
		SDL_RenderTexture(vl_sdl3_renderer, vl_sdl3_scaledTarget, 0, &fullRectF);
	}
	else
	{
		SDL_SetRenderTarget(vl_sdl3_renderer, 0);
		SDL_SetRenderViewport(vl_sdl3_renderer, 0);
		SDL_SetRenderDrawColor(vl_sdl3_renderer, 0, 0, 0, 255);
		SDL_RenderClear(vl_sdl3_renderer);

		SDL_SetRenderViewport(vl_sdl3_renderer, 0);
		SDL_SetRenderViewport(vl_sdl3_renderer, &fullRect);
		SDL_SetRenderDrawColor(vl_sdl3_renderer,
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][0],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][1],
			VL_EGARGBColorTable[vl_emuegavgaadapter.bordercolor][2],
			255);
		SDL_RenderFillRect(vl_sdl3_renderer, 0);
		SDL_RenderTexture(vl_sdl3_renderer, vl_sdl3_texture, 0, &renderRect);

	}

	SDL_RenderPresent(vl_sdl3_renderer);
}

static int VL_SDL3_GetActiveBufferId(void *surface)
{
	(void)surface;
	return 0;
}

static int VL_SDL3_GetNumBuffers(void *surface)
{
	(void)surface;
	return 1;
}

static void VL_SDL3_SyncBuffers(void *surface)
{
	(void)surface;
}

static void VL_SDL3_UpdateRect(void *surface, int x, int y, int w, int h)
{
	(void)surface;
	(void)x;
	(void)y;
	(void)w;
	(void)h;
}

static void VL_SDL3_FlushParams()
{
	SDL_SetWindowFullscreen(vl_sdl3_window, vl_isFullScreen ? SDL_TRUE : SDL_FALSE);
	SDL_SetWindowMinimumSize(vl_sdl3_window, VL_VGA_GFX_SCALED_WIDTH_PLUS_BORDER / VL_VGA_GFX_WIDTH_SCALEFACTOR, VL_VGA_GFX_SCALED_HEIGHT_PLUS_BORDER / VL_VGA_GFX_HEIGHT_SCALEFACTOR);
	SDL_SetRenderVSync(vl_sdl3_renderer, vl_swapInterval);
	VL_SDL3_ResizeWindow();
}

static void VL_SDL3_WaitVBLs(int vbls)
{
	SDL_Delay(vbls * 1000 / 70);
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_sdl3_backend =
	{
		/*.setVideoMode =*/&VL_SDL3_SetVideoMode,
		/*.createSurface =*/&VL_SDL3_CreateSurface,
		/*.destroySurface =*/&VL_SDL3_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_SDL3_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_SDL3_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_SDL3_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_SDL3_SurfacePGet,
		/*.surfaceRect =*/&VL_SDL3_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_SDL3_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_SDL3_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_SDL3_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_SDL3_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_SDL3_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_SDL3_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_SDL3_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_SDL3_BitToSurface,
		/*.bitToSurface_PM =*/&VL_SDL3_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_SDL3_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_SDL3_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_SDL3_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_SDL3_ScrollSurface,
		/*.present =*/&VL_SDL3_Present,
		/*.getActiveBufferId =*/&VL_SDL3_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_SDL3_GetNumBuffers,
		/*.syncBuffers =*/&VL_SDL3_SyncBuffers,
		/*.updateRect =*/&VL_SDL3_UpdateRect,
		/*.flushParams =*/&VL_SDL3_FlushParams,
		/*.waitVBLs =*/&VL_SDL3_WaitVBLs,
};

VL_Backend *VL_Impl_GetBackend()
{
	SDL_Init(SDL_INIT_VIDEO);
	return &vl_sdl3_backend;
}
