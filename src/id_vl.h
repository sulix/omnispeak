#ifndef ID_VL_H
#define ID_VL_H

#include <stdint.h>

void VL_SetDefaultPalette();
void VL_SetPalEntry(int id, uint8_t r, uint8_t g, uint8_t b);
void VL_UnmaskedToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h);
void VL_MaskedBlitClipToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int dw, int dh);
void VL_1bppToRGBA(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);
void VL_1bppBlitToRGB(void *src,void *dest, int x, int y, int pitch, int w, int h, int colour);

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
void VL_1bppBlitToScreen(void *src, int x, int y, int w, int h, int colour);

int VL_GetTics(bool wait);
void VL_Present();
#endif //ID_VL_H
