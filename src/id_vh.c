#include "id_vh.h"
#include "id_vl.h"
#include "id_mm.h"
#include "id_ca.h"


typedef struct VH_BitmapTableEntry
{
	uint16_t width;
	uint16_t height;
} __attribute((__packed__)) VH_BitmapTableEntry; 

typedef struct VH_Font
{
	uint16_t height;
	uint16_t location[256];
	uint8_t width[256];
} __attribute((__packed__)) VH_Font;



//TODO: Should these functions cache the bitmap tables?
static VH_BitmapTableEntry VH_GetBitmapTableEntry(int bitmapNumber)
{
	VH_BitmapTableEntry *bitmapTable = (VH_BitmapTableEntry*)(ca_graphChunks[ca_gfxInfoE.hdrBitmaps]);
	return bitmapTable[bitmapNumber];
}

static VH_BitmapTableEntry VH_GetMaskedBitmapTableEntry(int bitmapNumber)
{
	VH_BitmapTableEntry *maskedTable = (VH_BitmapTableEntry*)(ca_graphChunks[ca_gfxInfoE.hdrMasked]);
	return maskedTable[bitmapNumber];
}

VH_SpriteTableEntry VH_GetSpriteTableEntry(int spriteNumber)
{
	VH_SpriteTableEntry *spriteTable = (VH_SpriteTableEntry*)(ca_graphChunks[ca_gfxInfoE.hdrSprites]);
	return spriteTable[spriteNumber];
}

void VH_DrawTile8(int x, int y, int tile)
{
	char *ptr = (char*)(ca_graphChunks[ca_gfxInfoE.offTiles8]) + (tile * 32);
	VL_UnmaskedToScreen(ptr,x,y,8,8);
}

void VH_DrawTile8M(int x, int y, int tile)
{
	char *ptr = (char*)(ca_graphChunks[ca_gfxInfoE.offTiles8m]) + (tile * 40);
	VL_MaskedBlitToScreen(ptr,x,y,8,8);
}

void VH_DrawTile16(int x, int y, int tile)
{	
	VL_UnmaskedToScreen(ca_graphChunks[ca_gfxInfoE.offTiles16 + tile], x, y, 16, 16);
}

void VH_DrawTile16M(int x, int y, int tile)
{
	if (!ca_graphChunks[ca_gfxInfoE.offTiles16m + tile]) return;
	VL_MaskedBlitToScreen(ca_graphChunks[ca_gfxInfoE.offTiles16m + tile], x, y, 16, 16);
}

void VH_DrawBitmap(int x, int y, int chunk)
{
	int bitmapNumber = chunk - ca_gfxInfoE.offBitmaps;
	
	VH_BitmapTableEntry dimensions = VH_GetBitmapTableEntry(bitmapNumber);
	
	VL_UnmaskedToScreen(ca_graphChunks[chunk], x, y, dimensions.width*8, dimensions.height);
}

void VH_DrawMaskedBitmap(int x, int y, int chunk)
{
	int bitmapNumber = chunk - ca_gfxInfoE.offMasked;

	VH_BitmapTableEntry dim = VH_GetMaskedBitmapTableEntry(bitmapNumber);

	VL_MaskedBlitToScreen(ca_graphChunks[chunk], x, y, dim.width*8, dim.height);
}

void VH_DrawSprite(int x, int y, int chunk)
{
	int spriteNumber = chunk - ca_gfxInfoE.offSprites;
	
	VH_SpriteTableEntry spr = VH_GetSpriteTableEntry(spriteNumber);

	VL_MaskedBlitToScreen(ca_graphChunks[chunk], x + (spr.originX >> 4), y + (spr.originY >> 4) , spr.width*8, spr.height);

}

void VH_DrawPropChar(int x, int y, int chunk, char c, int colour)
{
	VH_Font *fnt = (VH_Font*)ca_graphChunks[chunk];

	uint8_t *chardata = (uint8_t *)fnt + fnt->location[c]; 


	VL_1bppBlitToScreen(chardata, x, y, fnt->width[c], fnt->height, colour);
}

void VH_MeasureString(const char *string, int *width, int *height, VH_Font *fnt)
{
	*height = fnt->height;

	for (*width = 0; *string; string++)
	{
		*width += fnt->width[*string];
	}
}

void VH_MeasurePropString(const char *string, int *width, int *height, int chunk)
{
	VH_MeasureString(string,width,height,ca_graphChunks[chunk]);
}

void VH_DrawPropString(const char *string,int x, int y, int chunk, int colour)
{
	int w = 0;
	VH_Font *font = (VH_Font*)ca_graphChunks[chunk];
	for (w = 0; *string; *string++)
	{
		VH_DrawPropChar(x+w,y,chunk,*string,colour);
		w += font->width[*string];
	}
}

