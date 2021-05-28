#include "id_sd.h"
#include "id_us.h"
#include "id_vl.h"
#include "id_vl_private.h"

#include "ck_cross.h"

#include <stdlib.h>
#include <string.h>

static int vl_null_screenWidth;
static int vl_null_screenHeight;

/* TODO (Overscan border):
 * - If a texture is used for offscreen rendering with scaling applied later,
 * it's better to have the borders within the texture itself.
 */

typedef struct VL_NULL_Surface
{
	VL_SurfaceUsage use;
	int w, h;
	void *data;
} VL_NULL_Surface;

static void VL_NULL_SetVideoMode(int mode)
{
	if (mode == 0xD)
	{
		vl_null_screenWidth = VL_EGAVGA_GFX_WIDTH;
		vl_null_screenHeight = VL_EGAVGA_GFX_HEIGHT;
	}
	else
	{
	}
}

static void VL_NULL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour);
static void *VL_NULL_CreateSurface(int w, int h, VL_SurfaceUsage usage)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)malloc(sizeof(VL_NULL_Surface));
	surf->w = w;
	surf->h = h;
	surf->data = malloc(w * h); // 8-bit pal for now
	return surf;
}

static void VL_NULL_DestroySurface(void *surface)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)surface;
	if (surf->data)
		free(surf->data);
	free(surf);
}

static long VL_NULL_GetSurfaceMemUse(void *surface)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)surface;
	return surf->w * surf->h;
}

static void VL_NULL_GetSurfaceDimensions(void *surface, int *w, int *h)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)surface;
	if (w)
		*w = surf->w;
	if (h)
		*h = surf->h;
}

static void VL_NULL_RefreshPaletteAndBorderColor(void *screen)
{
}

static int VL_NULL_SurfacePGet(void *surface, int x, int y)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)surface;
	return ((uint8_t *)surf->data)[y * surf->w + x];
}

static void VL_NULL_SurfaceRect(void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		memset(((uint8_t *)surf->data) + _y * surf->w + x, colour, w);
	}
}

static void VL_NULL_SurfaceRect_PM(void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	mapmask &= 0xF;
	colour &= mapmask;

	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	for (int _y = y; _y < y + h; ++_y)
	{
		for (int _x = x; _x < x + w; ++_x)
		{
			uint8_t *p = ((uint8_t *)surf->data) + _y * surf->w + _x;
			*p &= ~mapmask;
			*p |= colour;
		}
	}
}

static void VL_NULL_SurfaceToSurface(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)src_surface;
	VL_NULL_Surface *dest = (VL_NULL_Surface *)dst_surface;
	for (int _y = sy; _y < sy + sh; ++_y)
	{
		memcpy(((uint8_t *)dest->data) + (_y - sy + y) * dest->w + x, ((uint8_t *)surf->data) + _y * surf->w + sx, sw);
	}
}

static void VL_NULL_SurfaceToSelf(void *surface, int x, int y, int sx, int sy, int sw, int sh)
{
	VL_NULL_Surface *srf = (VL_NULL_Surface *)surface;
	bool directionX = sx > x;
	bool directionY = sy > y;

	if (directionY)
	{
		for (int yi = 0; yi < sh; ++yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->w + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->w + sx), sw);
		}
	}
	else
	{
		for (int yi = sh - 1; yi >= 0; --yi)
		{
			memmove(((uint8_t *)srf->data) + ((yi + y) * srf->w + x), ((uint8_t *)srf->data) + ((sy + yi) * srf->w + sx), sw);
		}
	}
}

static void VL_NULL_UnmaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_UnmaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_NULL_UnmaskedToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int mapmask)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_UnmaskedToPAL8_PM(src, surf->data, x, y, surf->w, w, h, mapmask);
}

static void VL_NULL_MaskedToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_MaskedToPAL8(src, surf->data, x, y, surf->w, w, h);
}

static void VL_NULL_MaskedBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_MaskedBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h);
}

static void VL_NULL_BitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_1bppToPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_NULL_BitToSurface_PM(void *src, void *dst_surface, int x, int y, int w, int h, int colour, int mapmask)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_1bppToPAL8_PM(src, surf->data, x, y, surf->w, w, h, colour, mapmask);
}

static void VL_NULL_BitXorWithSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_1bppXorWithPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_NULL_BitBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_1bppBlitToPAL8(src, surf->data, x, y, surf->w, w, h, colour);
}

static void VL_NULL_BitInvBlitToSurface(void *src, void *dst_surface, int x, int y, int w, int h, int colour)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)dst_surface;
	VL_1bppInvBlitClipToPAL8(src, surf->data, x, y, surf->w, w, h, surf->w, surf->h, colour);
}

static int VL_NULL_GetActiveBufferId(void *surface)
{
	(void)surface;
	return 0;
}

static int VL_NULL_GetNumBuffers(void *surface)
{
	(void)surface;
	return 1;
}

static void VL_NULL_ScrollSurface(void *surface, int x, int y)
{
	VL_NULL_Surface *surf = (VL_NULL_Surface *)surface;
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
	VL_NULL_SurfaceToSelf(surface, dx, dy, sx, sy, w, h);
}

static void VL_NULL_Present(void *surface, int scrlX, int scrlY, bool singleBuffered)
{
	SD_SetTimeCount(SD_GetTimeCount() + 1);
}

void VL_NULL_FlushParams()
{
}

void VL_NULL_WaitVBLs(int vbls)
{
	SD_SetTimeCount(SD_GetTimeCount() + CK_Cross_max(vbls,1));
}

// Unfortunately, we can't take advantage of designated initializers in C++.
VL_Backend vl_null_backend =
	{
		/*.setVideoMode =*/&VL_NULL_SetVideoMode,
		/*.createSurface =*/&VL_NULL_CreateSurface,
		/*.destroySurface =*/&VL_NULL_DestroySurface,
		/*.getSurfaceMemUse =*/&VL_NULL_GetSurfaceMemUse,
		/*.getSurfaceDimensions =*/&VL_NULL_GetSurfaceDimensions,
		/*.refreshPaletteAndBorderColor =*/&VL_NULL_RefreshPaletteAndBorderColor,
		/*.surfacePGet =*/&VL_NULL_SurfacePGet,
		/*.surfaceRect =*/&VL_NULL_SurfaceRect,
		/*.surfaceRect_PM =*/&VL_NULL_SurfaceRect_PM,
		/*.surfaceToSurface =*/&VL_NULL_SurfaceToSurface,
		/*.surfaceToSelf =*/&VL_NULL_SurfaceToSelf,
		/*.unmaskedToSurface =*/&VL_NULL_UnmaskedToSurface,
		/*.unmaskedToSurface_PM =*/&VL_NULL_UnmaskedToSurface_PM,
		/*.maskedToSurface =*/&VL_NULL_MaskedToSurface,
		/*.maskedBlitToSurface =*/&VL_NULL_MaskedBlitToSurface,
		/*.bitToSurface =*/&VL_NULL_BitToSurface,
		/*.bitToSurface_PM =*/&VL_NULL_BitToSurface_PM,
		/*.bitXorWithSurface =*/&VL_NULL_BitXorWithSurface,
		/*.bitBlitToSurface =*/&VL_NULL_BitBlitToSurface,
		/*.bitInvBlitToSurface =*/&VL_NULL_BitInvBlitToSurface,
		/*.scrollSurface =*/&VL_NULL_ScrollSurface,
		/*.present =*/&VL_NULL_Present,
		/*.getActiveBufferId =*/&VL_NULL_GetActiveBufferId,
		/*.getNumBuffers =*/&VL_NULL_GetNumBuffers,
		/*.flushParams =*/&VL_NULL_FlushParams,
		/*.waitVBLs =*/&VL_NULL_WaitVBLs};

VL_Backend *VL_Impl_GetBackend()
{
	return &vl_null_backend;
}
