#ifndef ID_VH_H
#define ID_VH_H

#include <stdbool.h>
#include <stdint.h>

typedef struct VH_SpriteTableEntry
{
	uint16_t width, height;
	int16_t originX, originY;
	int16_t xl, yl, xh, yh;
	uint16_t shifts;
} __attribute((__packed__)) VH_SpriteTableEntry;


VH_SpriteTableEntry VH_GetSpriteTableEntry(int spriteNumber);

void VH_DrawTile8(int x, int y, int tile);
void VH_DrawTile8M(int x, int y, int tile);
void VH_DrawTile16(int x, int y, int tile);
void VH_DrawTile16M(int x, int y, int tile);
void VH_DrawBitmap(int x, int y, int chunk);
void VH_DrawMaskedBitmap(int x, int y, int chunk);
void VH_DrawSprite(int x, int y, int chunk);
void VH_DrawPropChar(int x, int y, int chunk, char c, int colour);
void VH_MeasurePropString(const char *string, int *width, int *height, int chunk);
void VH_DrawPropString(const char *string,int x, int y, int chunk, int colour);

#endif
