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

#include "id_vh.h"
#include "id_vl.h"
#include "id_mm.h"
#include "id_ca.h"
#include "id_rf.h"


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

bool vh_dirtyBlocks[RF_BUFFER_WIDTH_TILES*RF_BUFFER_HEIGHT_TILES]; 

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

// "Buffer" drawing routines.
// These routines (VHB_*) mark the tiles they draw over as 'dirty', so that
// id_rf can redraw them next frame.

// Mark a block (in pixels) as dirty. Returns true if any tiles were dirtied, false otherwise.
bool VH_MarkUpdateBlock(int x1px, int y1px, int x2px, int y2px)
{
	// Convert pixel coords to tile coords.
	int x1tile = x1px >> 4;
	int y1tile = y1px >> 4;
	int x2tile = x2px >> 4;
	int y2tile = y2px >> 4;

	if (x1tile >= RF_BUFFER_WIDTH_TILES) return false;
	x1tile = (x1tile<0)?0:x1tile;

	if (y1tile >= RF_BUFFER_HEIGHT_TILES) return false;
	y1tile = (y1tile<0)?0:y1tile;

	if (x2tile < 0) return false;
	x2tile = (x2tile>=RF_BUFFER_WIDTH_TILES)?x2tile:(RF_BUFFER_WIDTH_TILES-1);

	if (y2tile < 0) return false;
	y2tile = (y2tile>=RF_BUFFER_HEIGHT_TILES)?y2tile:(RF_BUFFER_HEIGHT_TILES-1);

	for (int y = y1tile; y <= y2tile; y++)
	{
		for (int x = x1tile; x <= x2tile; x++)
		{
			vh_dirtyBlocks[y*RF_BUFFER_WIDTH_TILES+x] = true;
		}
	}
	return true;
}
