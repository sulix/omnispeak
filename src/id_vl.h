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

#ifndef ID_VL_H
#define ID_VL_H

#include <stdint.h>
#include <stdbool.h>

/* Util Functions */

extern bool vl_screenFaded;

// EGA signal palettes (the 17th entry of each row is the overscan border color)
// NOTE: Vanilla Keen can modify some of these (e.g. the border color)
extern uint8_t vl_palette[6][17];
extern uint16_t vl_border_color;

#if 0
extern VL_EGAPaletteEntry VL_EGAPalette[16];
void VL_SetPalEntry(int id, uint8_t r, uint8_t g, uint8_t b);
#endif

void VL_SetPaletteByID(int id);
void VL_SetPalette(uint8_t *palette);
void VL_SetPaletteAndBorderColor(uint8_t *palette);
void VL_ColorBorder(uint16_t color);
void VL_SetDefaultPalette(void);
void VL_FadeToBlack(void);
void VL_FadeFromBlack(void);
//void VL_FadeToWhite(void); // Unused in vanilla Keen 5
//void VL_FadeFromWhite(void); // Unused in vanilla Keen 5
void VL_UnmaskedToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitClipToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh);
void VL_1bppToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppXorWithRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppInvBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);

void VL_UnmaskedToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitClipToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh);
void VL_1bppToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppXorWithPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppBlitToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppInvBlitToPAL8(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);

int VL_MemUsed();
int VL_NumSurfaces();

typedef enum VL_SurfaceUsage
{
	VL_SurfaceUsage_Default,
	VL_SurfaceUsage_FrameBuffer,
	VL_SurfaceUsage_FrontBuffer,
	VL_SurfaceUsage_Sprite
} VL_SurfaceUsage;

typedef struct VL_Backend
{
	void (*setVideoMode)(int mode);
	void* (*createSurface)(int w, int h, VL_SurfaceUsage usage);
	void (*destroySurface)(void *surface);
	long (*getSurfaceMemUse)(void *surface);
	void (*refreshPaletteAndBorderColor)(void *screen);
	void (*surfaceRect)(void *dst_surface, int x, int y, int w, int h, int colour);
	void (*surfaceToSurface)(void *src_surface, void *dst_surface, int x, int y, int sx, int sy, int sw, int sh);
	void (*surfaceToSelf)(void *surface, int x, int y, int sx, int sy, int sw, int sh);
	void (*unmaskedToSurface)(void *src, void *dst_surface, int x, int y, int w, int h);
	void (*maskedToSurface)(void *src, void *dst_surface, int x, int y, int w, int h);
	void (*maskedBlitToSurface)(void *src, void *dst_surface, int x, int y, int w, int h);
	void (*bitToSurface)(void *src, void *dst_surface, int x, int y, int w, int h, int colour);
	void (*bitXorWithSurface)(void *src, void *dst_surface, int x, int y, int w, int h, int colour);
	void (*bitBlitToSurface)(void *src, void *dst_surface, int x, int y, int w, int h, int colour);
	void (*bitInvBlitToSurface)(void *src, void *dst_surface, int x, int y, int w, int h, int colour);
	void (*present)(void *surface, int scrollXpx, int scrollYpx);
} VL_Backend;

void VL_InitScreen();
void *VL_CreateSurface(int w, int h);
void VL_SurfaceRect(void *dst, int x, int y, int w, int h, int colour);
void VL_ScreenRect(int x, int y, int w, int h, int colour);
void VL_SurfaceToSurface(void *src, void *dst, int x, int y, int sx, int sy, int sw, int sh);
void VL_SurfaceToScreen(void *src, int x, int y, int sx, int sy, int sw, int sh);
void VL_SurfaceToSelf(void *surf, int x, int y, int sx, int sy, int sw, int sh);
void VL_UnmaskedToSurface(void *src, void *dest, int x, int y, int w, int h); 
void VL_UnmaskedToScreen(void *src, int x, int y, int w, int h);
void VL_MaskedToSurface(void *src, void *dest, int x, int y, int w, int h);
void VL_MaskedBlitToSurface(void *src, void *dest, int x, int y, int w, int h);
void VL_MaskedToScreen(void *src, int x, int y, int w, int h);
void VL_MaskedBlitToScreen(void *src, int x, int y, int w, int h);
void VL_1bppToScreen(void *src, int x, int y, int w, int h, int colour);
void VL_1bppXorWithScreen(void *src, int x, int y, int w, int h, int colour);
void VL_1bppBlitToScreen(void *src, int x, int y, int w, int h, int colour);
void VL_1bppInvBlitToScreen(void *src, int x, int y, int w, int h, int colour);

void VL_DelayTics(int tics);
int VL_GetTics(int wait);
void VL_SetScrollCoords(int x, int y);
void VL_ClearScreen(int colour);
void VL_Present();

VL_Backend *VL_Impl_GetBackend(void);

#endif //ID_VL_H
