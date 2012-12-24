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

#ifndef ID_RF_H
#define ID_RF_H

#include <stdint.h>
#include <stdbool.h>

// Sprite Draw object
typedef struct RF_SpriteDrawEntry
{
	int chunk;
	int zLayer;
	int x, y;
	int sx, sy;
	int sw, sh;
	struct RF_SpriteDrawEntry **prevNextPtr;		// Pointer to the previous entry's 'next' pointer.
	struct RF_SpriteDrawEntry *next;
} RF_SpriteDrawEntry;


void RF_SetScrollBlock(int tileX, int tileY, bool vertical);
void RF_MarkTileGraphics();
void RF_Startup();
void RF_NewMap(int mapNum);
void RF_RenderTile16(int x, int y, int tile);
void RF_RenderTile16m(int x, int y, int tile);
void RF_ReplaceTiles(int16_t *tilePtr, int plane, int dstX, int dstY, int width, int height);
void RF_Reposition(int scrollXunit, int scrollYunit);
void RF_SmoothScroll(int scrollXdelta, int scrollYdelta);
void RF_RemoveSpriteDraw(RF_SpriteDrawEntry **drawEntry);
void RF_AddSpriteDraw(RF_SpriteDrawEntry **drawEntry, int unitX, int unitY, int chunk, bool allWhite, int zLayer);
void RF_Refresh();

#endif //ID_RF_H
