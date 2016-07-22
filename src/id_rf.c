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

//TODO: id_heads.h
#include "ck_ep.h"
#include "ck_play.h"
#include "id_rf.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "id_ca.h"
#include "id_mm.h"
#include "id_ti.h"

#include <stdbool.h>
#include <stdio.h>
#include "SDL.h"

// Proper dirty-rectangle drawing is not working yet. Disable it for now.
#define ALWAYS_REDRAW


#define MAX(a,b) (((a) > (b))?(a):(b))
#define MIN(a,b) (((a) < (b))?(a):(b))

// The refresh manager (ID_RF) is the core of the game's smooth-scrolling
// engine. It renders tiles to an offscreen bufferand then blits from
// there to a screen buffer, which then has the sprites overlayed.
// John Carmack refers to this technique as 'Virtual Tile Refresh'.

// These buffers are all larger than the physical screen, and scroll
// tile-by-tile, not pixel by pixel. The smooth-scrolling effect
// is implemented by changing which part of the buffer is displayed.


// Scroll blocks prevent the camera from moving beyond a certain row/column
#define RF_MAX_SCROLLBLOCKS 6
static int rf_horzScrollBlocks[RF_MAX_SCROLLBLOCKS];
static int rf_vertScrollBlocks[RF_MAX_SCROLLBLOCKS];
static int rf_numHorzScrollBlocks;
static int rf_numVertScrollBlocks;

void *rf_tileBuffer;
//static void *rf_screenBuffer;
static int rf_mapWidthTiles;
static int rf_mapHeightTiles;

//Scroll variables
//static int rf_scrollXTile;
//static int rf_scrollYTile;
int rf_scrollXUnit;
int rf_scrollYUnit;
//static int rf_scrollX
int rf_scrollXMinUnit;
int rf_scrollYMinUnit;
int rf_scrollXMaxUnit;
int rf_scrollYMaxUnit;


#define RF_MAX_SPRITETABLEENTRIES 48
#define RF_NUM_SPRITE_Z_LAYERS 4

// Rectangle of previous sprite position.
typedef struct RF_SpriteEraser
{
	int pxX;
	int pxY;
	int pxW;
	int pxH;
} RF_SpriteEraser;

// Pool from which sprite draw entries are allocated.
RF_SpriteDrawEntry rf_spriteTable[RF_MAX_SPRITETABLEENTRIES];
RF_SpriteDrawEntry *rf_freeSpriteTableEntry;

RF_SpriteDrawEntry *rf_firstSpriteTableEntry[RF_NUM_SPRITE_Z_LAYERS];
static int rf_numSpriteDraws;

int rf_freeSpriteEraserIndex = 0;
RF_SpriteEraser rf_spriteErasers[RF_MAX_SPRITETABLEENTRIES];
// Animated tile management
// (This is hairy)

// A pointer (index into array when 32-bit) to this struct
// is stored in the info-plane of each animated tile.

typedef struct RF_AnimTileTimer
{
	int tileNumber;
	int timeToSwitch;
} RF_AnimTileTimer;


/*** Used for saved games compatibility ***/
static uint16_t RFL_ConvertAnimTileTimerIndexTo16BitOffset(int i)
{
	return 4*i + ck_currentEpisode->animTilesOffset;
}

static int RFL_ConvertAnimTileTimer16BitOffsetToIndex(uint16_t offset)
{
	return (offset-ck_currentEpisode->animTilesOffset) / 4;
}


typedef struct RF_OnscreenAnimTile
{
	int tileX;
	int tileY;
	int tile;
	int plane;
	int timerIndex;
	struct RF_OnscreenAnimTile *next;
	struct RF_OnscreenAnimTile *prev;
} RF_OnscreenAnimTile;
#define RF_MAX_ANIMTILETIMERS 180
#define RF_MAX_ONSCREENANIMTILES 90
#define RF_MAX_ANIM_LOOP 20

int rf_numAnimTileTimers;
RF_AnimTileTimer rf_animTileTimers[RF_MAX_ANIMTILETIMERS];

RF_OnscreenAnimTile rf_onscreenAnimTiles[RF_MAX_ONSCREENANIMTILES];
RF_OnscreenAnimTile *rf_firstOnscreenAnimTile, *rf_freeOnscreenAnimTile;

// Block dirty state
int rf_dirtyBlocks[RF_BUFFER_WIDTH_TILES * RF_BUFFER_HEIGHT_TILES];

void RFL_MarkBlockDirty(int x, int y, int val)
{
	if (x >= RF_BUFFER_WIDTH_TILES || y >= RF_BUFFER_HEIGHT_TILES)
		return;
	rf_dirtyBlocks[y*RF_BUFFER_WIDTH_TILES+x] = val; 
}

void RF_SetScrollBlock(int tileX, int tileY, bool vertical)
{
	if (!vertical)
	{
		if (rf_numHorzScrollBlocks == RF_MAX_SCROLLBLOCKS)
			Quit("RF_SetScrollBlock: Too many horizontal scroll blocks");
		rf_horzScrollBlocks[rf_numHorzScrollBlocks] = tileX;
		rf_numHorzScrollBlocks++;
	}
	else
	{
		if (rf_numVertScrollBlocks == RF_MAX_SCROLLBLOCKS)
			Quit("RF_SetScrollBlock: Too many vertical scroll blocks");
		rf_vertScrollBlocks[rf_numVertScrollBlocks] = tileY;
		rf_numVertScrollBlocks++;
	}
}

void RFL_SetupOnscreenAnimList()
{
	rf_freeOnscreenAnimTile = rf_onscreenAnimTiles;

	for (int i = 0; i < RF_MAX_ONSCREENANIMTILES - 1; ++i)
	{
		rf_onscreenAnimTiles[i].next = &rf_onscreenAnimTiles[i+1];
	}

	rf_onscreenAnimTiles[RF_MAX_ONSCREENANIMTILES - 1].next = 0;

	rf_firstOnscreenAnimTile = 0;
}

void RFL_SetupSpriteTable()
{
	rf_freeSpriteTableEntry = rf_spriteTable;
	rf_numSpriteDraws = 0;

	for (int i = 0; i < RF_MAX_SPRITETABLEENTRIES - 1; ++i)
	{
		rf_spriteTable[i].next = &rf_spriteTable[i+1];
		rf_spriteTable[i+1].prevNextPtr = &(rf_spriteTable[i].next);
	}

	rf_spriteTable[RF_MAX_SPRITETABLEENTRIES - 1].next = 0;

	for (int i = 0; i < RF_NUM_SPRITE_Z_LAYERS; ++i)
	{
		rf_firstSpriteTableEntry[i] = 0;
	}
}

void RF_MarkTileGraphics()
{
	memset(rf_animTileTimers, 0, sizeof(rf_animTileTimers));
	rf_numAnimTileTimers = 0;
	for (int tileY = 0; tileY < rf_mapHeightTiles; ++tileY)
	{
		for (int tileX = 0; tileX < rf_mapWidthTiles; ++tileX)
		{
			bool needNewTimer = true;
			int backTile = CA_mapPlanes[0][tileY*rf_mapWidthTiles+tileX];

			CA_MarkGrChunk(ca_gfxInfoE.offTiles16 + backTile);
			if (TI_BackAnimTile(backTile))
			{
				if (TI_BackAnimTime(backTile))
				{
					// Is the info-plane free?
					if (CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX])
						Quit("RF_MarkTileGraphics: Info plane above animated bg tile in use.");


					for (int i = 0; i < rf_numAnimTileTimers; ++i)
					{
						if (rf_animTileTimers[i].tileNumber == backTile)
						{
							// Add the timer index to the info-plane
							CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
							needNewTimer = false;
							break;
						}
					}

					if (needNewTimer)
					{
						if (rf_numAnimTileTimers >= RF_MAX_ANIMTILETIMERS)
							Quit("RF_MarkTileGraphics: Too many unique animations");
						rf_animTileTimers[rf_numAnimTileTimers].tileNumber = backTile;
						rf_animTileTimers[rf_numAnimTileTimers].timeToSwitch = TI_BackAnimTime(backTile);
						CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(rf_numAnimTileTimers);
						rf_numAnimTileTimers++;
					}
					else
						continue;
				}

				// Mark all tiles in the animation loop to be cached.
				int animLength = 0;
				int nextTile = backTile;
				do
				{
					nextTile += TI_BackAnimTile(nextTile);
					animLength++;
					CA_MarkGrChunk(ca_gfxInfoE.offTiles16 + nextTile);
					if (animLength > RF_MAX_ANIM_LOOP)
					{
						Quit("RF_MarkTileGraphics: Unending background animation");
					}
				} while (TI_BackAnimTile(nextTile) && nextTile != backTile);
			}
		}
	}
	for (int tileY = 0; tileY < rf_mapHeightTiles; ++tileY)
	{
		for (int tileX = 0; tileX < rf_mapWidthTiles; ++tileX)
		{
			bool needNewTimer = true;
			int foreTile = CA_mapPlanes[1][tileY*rf_mapWidthTiles+tileX];

			CA_MarkGrChunk(ca_gfxInfoE.offTiles16m + foreTile);
			if (TI_ForeAnimTile(foreTile))
			{
				if (TI_ForeAnimTime(foreTile))
				{
					// Is the info-plane free?
					//if (CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX])
					//	Quit("RF_MarkTileGraphics: Info plane above animated fg tile in use.");

					for (int i = 0; i < rf_numAnimTileTimers; ++i)
					{
						if (rf_animTileTimers[i].tileNumber == (foreTile | 0x8000))
						{
							// Add the timer index to the info-plane
							CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
							needNewTimer = false;
							break;
						}
					}

					if (needNewTimer)
					{
						if (rf_numAnimTileTimers >= RF_MAX_ANIMTILETIMERS)
							Quit("RF_MarkTileGraphics: Too many unique animations");

						rf_animTileTimers[rf_numAnimTileTimers].tileNumber = foreTile | 0x8000;
						rf_animTileTimers[rf_numAnimTileTimers].timeToSwitch = TI_ForeAnimTime(foreTile);
						CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(rf_numAnimTileTimers);
						rf_numAnimTileTimers++;
					}
					else
						continue;
				}

				// Mark all tiles in the animation loop to be cached.
				int animLength = 0;
				int nextTile = foreTile;
				do
				{
					nextTile += TI_ForeAnimTile(nextTile);
					animLength++;
					CA_MarkGrChunk(ca_gfxInfoE.offTiles16m + nextTile);
					if (animLength > RF_MAX_ANIM_LOOP)
					{
						Quit("RF_MarkTileGraphics: Unending foreground animation");
					}
				} while (TI_ForeAnimTile(nextTile) && nextTile != foreTile);

			}

		}
	}
}


void RFL_CheckForAnimTile(int tileX, int tileY)
{
	int backTile = CA_mapPlanes[0][tileY*rf_mapWidthTiles+tileX];
	int foreTile = CA_mapPlanes[1][tileY*rf_mapWidthTiles+tileX];


	if (TI_BackAnimTile(backTile) != 0 && TI_BackAnimTime(backTile) != 0)
	{
		if (!rf_freeOnscreenAnimTile)
			Quit("RFL_CheckForAnimTile: No free spots in tilearray!");

		RF_OnscreenAnimTile *ost = rf_freeOnscreenAnimTile;
		rf_freeOnscreenAnimTile = rf_freeOnscreenAnimTile->next;

		ost->tileX = tileX;
		ost->tileY = tileY;
		ost->tile = backTile;
		ost->plane = 0;
		ost->timerIndex = RFL_ConvertAnimTileTimer16BitOffsetToIndex(CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX]);


		ost->next = rf_firstOnscreenAnimTile;
		ost->prev = 0;
		rf_firstOnscreenAnimTile = ost;
	}

	if (TI_ForeAnimTile(foreTile) != 0 && TI_ForeAnimTime(foreTile) != 0)
	{
		if (!rf_freeOnscreenAnimTile)
			Quit("RFL_CheckForAnimTile: No free spots in tilearray!");

		RF_OnscreenAnimTile *ost = rf_freeOnscreenAnimTile;
		rf_freeOnscreenAnimTile = rf_freeOnscreenAnimTile->next;

		ost->tileX = tileX;
		ost->tileY = tileY;
		ost->tile = foreTile;
		ost->plane = 1;
		ost->timerIndex = RFL_ConvertAnimTileTimer16BitOffsetToIndex(CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX]);

		ost->prev = 0;
		ost->next = rf_firstOnscreenAnimTile;
		rf_firstOnscreenAnimTile = ost;
	}
}

void RFL_RemoveAnimRect(int tileX, int tileY, int tileW, int tileH)
{
	RF_OnscreenAnimTile *ost, *prev;

	prev = 0;
	ost = rf_firstOnscreenAnimTile;


	while (ost)
	{
		if ((ost->tileX >= tileX && ost->tileX < (tileX+tileW)) && (ost->tileY >= tileY && ost->tileY < (tileY+tileH)))
		{
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev)?prev:rf_firstOnscreenAnimTile;
			obsolete->next = rf_freeOnscreenAnimTile;
			rf_freeOnscreenAnimTile = obsolete;
			continue;
		}
		prev = ost;
		ost = ost->next;
	}
}

void RFL_RemoveAnimCol(int tileX)
{
	RF_OnscreenAnimTile *ost, *prev;

	prev = 0;
	ost = rf_firstOnscreenAnimTile;

	while (ost)
	{
		if (ost->tileX == tileX)
		{
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev)?prev:rf_firstOnscreenAnimTile;
			obsolete->next = rf_freeOnscreenAnimTile;
			rf_freeOnscreenAnimTile = obsolete;
			continue;
		}
		prev = ost;
		ost = ost->next;
	}
}

void RFL_RemoveAnimRow(int tileY)
{
	RF_OnscreenAnimTile *ost, *prev;

	prev = 0;

	ost = rf_firstOnscreenAnimTile;

	while (ost)
	{
		if (ost->tileY == tileY)
		{
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev)?prev:rf_firstOnscreenAnimTile;
			obsolete->next = rf_freeOnscreenAnimTile;
			rf_freeOnscreenAnimTile = obsolete;
			continue;
		}
		prev = ost;
		ost = ost->next;
	}
}

void RFL_AnimateTiles()
{
	// Update the timers.

	for (int i = 0; i < rf_numAnimTileTimers; ++i)
	{
		rf_animTileTimers[i].timeToSwitch-=SD_GetSpriteSync();


		if (rf_animTileTimers[i].timeToSwitch <= 0)
		{
			if (rf_animTileTimers[i].tileNumber & 0x8000)
			{
				int tile = rf_animTileTimers[i].tileNumber & ~0x8000;
				tile += TI_ForeAnimTile(tile);
				rf_animTileTimers[i].timeToSwitch += TI_ForeAnimTime(tile);
				rf_animTileTimers[i].tileNumber = tile | 0x8000;
			}
			else
			{
				int tile = rf_animTileTimers[i].tileNumber;
				tile += TI_BackAnimTile(tile);
				rf_animTileTimers[i].timeToSwitch += TI_BackAnimTime(tile);
				rf_animTileTimers[i].tileNumber = tile;
			}
		}
	}

	// Update the onscreen tiles.
	for (RF_OnscreenAnimTile *ost = rf_firstOnscreenAnimTile; ost; ost = ost->next)
	{
		int tile = rf_animTileTimers[ost->timerIndex].tileNumber & ~0x8000;
		if (tile != ost->tile)
		{
			ost->tile = tile;
			int screenTileX = ost->tileX - (rf_scrollXUnit >> 8);
			int screenTileY = ost->tileY - (rf_scrollYUnit >> 8);

			if (screenTileX < 0 || screenTileX > RF_BUFFER_WIDTH_TILES || screenTileY < 0 || screenTileY > RF_BUFFER_HEIGHT_TILES)
			{
				//printf("Out of bounds: %d, %d (sc: %d,%d) (tl: %d,%d,%d):%d\n", screenTileX, screenTileY,
				//	rf_scrollXUnit >> 8, rf_scrollYUnit >> 8, ost->tileX, ost->tileY,ost->plane, ost->tile);
				Quit("RFL_AnimateTiles: Out of bounds!");
			}

			CA_mapPlanes[ost->plane][ost->tileY*rf_mapWidthTiles+ost->tileX] = tile;

			RF_RenderTile16(screenTileX, screenTileY, CA_mapPlanes[0][ost->tileY*rf_mapWidthTiles+ost->tileX]);
			RF_RenderTile16m(screenTileX, screenTileY, CA_mapPlanes[1][ost->tileY*rf_mapWidthTiles+ost->tileX]);
			RFL_MarkBlockDirty(screenTileX, screenTileY, 1);
		}
	}
}

void (*rf_drawFunc) (void);

void RF_SetDrawFunc(void (*func) (void))
{
	rf_drawFunc = func;
}

void RF_Startup()
{
	// Create the tile backing buffer
	rf_tileBuffer = VL_CreateSurface(RF_BUFFER_WIDTH_PIXELS, RF_BUFFER_HEIGHT_PIXELS);

}

// TODO: More to change? Also, originally mapNum is a global variable.
void RF_NewMap(void)
{
	rf_mapWidthTiles = CA_MapHeaders[ca_mapOn]->width;
	rf_mapHeightTiles = CA_MapHeaders[ca_mapOn]->height;
	rf_scrollXMinUnit = 0x0200;		//Two-tile wide border around map
	rf_scrollYMinUnit = 0x0200;
	rf_scrollXMaxUnit = ((CA_MapHeaders[ca_mapOn]->width - RF_SCREEN_WIDTH_TILES - 2) << 8);
	rf_scrollYMaxUnit = ((CA_MapHeaders[ca_mapOn]->height - RF_SCREEN_HEIGHT_TILES - 2) << 8);

	// Reset the scroll-blocks
	rf_numVertScrollBlocks = rf_numHorzScrollBlocks = 0;

	RFL_SetupOnscreenAnimList();
	RFL_SetupSpriteTable();
	//RF_MarkTileGraphics();

	rf_freeSpriteEraserIndex = 0;

	// Set-up a two-tile wide border
	RF_SetScrollBlock(0,1,true);
	RF_SetScrollBlock(0,CA_MapHeaders[ca_mapOn]->height-2,true);
	RF_SetScrollBlock(1,0,false);
	RF_SetScrollBlock(CA_MapHeaders[ca_mapOn]->width-2,0,false);

	SD_SetLastTimeCount(SD_GetTimeCount());
	SD_SetSpriteSync(1);
}

void RF_RenderTile16(int x, int y, int tile)
{
	CA_CacheGrChunk(ca_gfxInfoE.offTiles16+tile);
	VL_UnmaskedToSurface(ca_graphChunks[ca_gfxInfoE.offTiles16+tile],rf_tileBuffer,x*16,y*16,16,16);
}

void RF_RenderTile16m(int x, int y, int tile)
{
	if (!tile) return;
	CA_CacheGrChunk(ca_gfxInfoE.offTiles16m+tile);
	VL_MaskedBlitToSurface(ca_graphChunks[ca_gfxInfoE.offTiles16m+tile],rf_tileBuffer,x*16,y*16,16,16);
}

void RF_ForceRefresh(void)
{
  RF_Reposition(rf_scrollXUnit, rf_scrollYUnit);
  RF_Refresh();
  RF_Refresh();
}

/*
 * Copy a rectangle of tiles, in all planes, from one source coordinate on the map
 * to another destination coordinate
 */
void RF_ReplaceTileBlock(int srcx, int srcy, int destx, int desty, int width, int height)
{

	int tx, ty;
	uint16_t *src_bgtile_ptr, *src_fgtile_ptr, *src_infotile_ptr;
	uint16_t *dst_bgtile_ptr, *dst_fgtile_ptr, *dst_infotile_ptr;
	intptr_t src_offset, dst_offset,offset_delta,new_row_offset;

	RFL_RemoveAnimRect(destx, desty, width, height);

	//
	src_offset = srcy * rf_mapWidthTiles + srcx;
	dst_offset = desty * rf_mapWidthTiles + desty;
	src_bgtile_ptr = CA_mapPlanes[0] + src_offset;
	src_fgtile_ptr = CA_mapPlanes[1] + src_offset;
	src_infotile_ptr = CA_mapPlanes[2] + src_offset;
	offset_delta = dst_offset - src_offset;
	new_row_offset = rf_mapWidthTiles - width;

	for (ty = 0; ty < height; ty++)
	{
		for (tx = 0; tx < width; tx++)
		{
			bool different;
			int screenX, screenY;

			src_bgtile_ptr = &CA_mapPlanes[0][(srcy+ty)*rf_mapWidthTiles+(srcx+tx)];
			src_fgtile_ptr = &CA_mapPlanes[1][(srcy+ty)*rf_mapWidthTiles+(srcx+tx)];
			src_infotile_ptr = &CA_mapPlanes[2][(srcy+ty)*rf_mapWidthTiles+(srcx+tx)];
			dst_bgtile_ptr = &CA_mapPlanes[0][(desty+ty)*rf_mapWidthTiles+(destx+tx)];
			dst_fgtile_ptr = &CA_mapPlanes[1][(desty+ty)*rf_mapWidthTiles+(destx+tx)];
			dst_infotile_ptr = &CA_mapPlanes[2][(desty+ty)*rf_mapWidthTiles+(destx+tx)];

			// Check if there is a different tile being copied
			if (*dst_bgtile_ptr != *src_bgtile_ptr || *dst_fgtile_ptr != *src_fgtile_ptr ||
					*dst_infotile_ptr != *src_infotile_ptr)
			{
				*dst_bgtile_ptr = *src_bgtile_ptr;
				*dst_fgtile_ptr = *src_fgtile_ptr;
				*dst_infotile_ptr = *src_infotile_ptr;
				different = true;
			}
			else
			{
				different = false;
			}

			// If the tile is onscreen...
			screenX = destx + tx - (rf_scrollXUnit >> 8);
			screenY = desty + ty - (rf_scrollYUnit >> 8);
			if (screenX >= 0 && screenY >= 0 && screenY < RF_BUFFER_HEIGHT_TILES && screenX < RF_BUFFER_WIDTH_TILES)
			{
				// Redraw it if it's different.
				if (different)
				{
					RFL_MarkBlockDirty(screenX, screenY, 1);
					RF_RenderTile16(screenX, screenY, *dst_bgtile_ptr);
					RF_RenderTile16m(screenX, screenY, *dst_fgtile_ptr);
				}
				// And check it for animations.
				RFL_CheckForAnimTile(destx + tx, desty + ty);
			}

			src_bgtile_ptr++;
			src_fgtile_ptr++;
			src_infotile_ptr++;
		}

		src_bgtile_ptr += new_row_offset;
		src_fgtile_ptr += new_row_offset;
		src_infotile_ptr += new_row_offset;
	}
}


void RF_ReplaceTiles(uint16_t *tilePtr, int plane, int dstX, int dstY, int width, int height)
{
	RFL_RemoveAnimRect(dstX, dstY, width, height);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int dstTileX = dstX + x;
			int dstTileY = dstY + y;

			int tileScreenX = dstTileX - (rf_scrollXUnit >> 8);
			int tileScreenY = dstTileY - (rf_scrollYUnit >> 8);
			int oldTile = CA_mapPlanes[plane][dstTileY*rf_mapWidthTiles+dstTileX];
			int newTile = tilePtr[y*width+x];
			// Update the tile on the map.
			if (oldTile != newTile)
			{
				CA_mapPlanes[plane][dstTileY*rf_mapWidthTiles+dstTileX] = newTile;
			}
			// If the tile is onscreen...
			if (tileScreenX >= 0 && tileScreenX < RF_BUFFER_WIDTH_TILES &&
				tileScreenY >= 0 && tileScreenY < RF_BUFFER_HEIGHT_TILES)
			{
				// Redraw it if it has changed.
				if (oldTile != newTile)
				{
					RFL_MarkBlockDirty(tileScreenX, tileScreenY, 1);
					RF_RenderTile16(tileScreenX, tileScreenY, CA_mapPlanes[0][dstTileY*rf_mapWidthTiles+dstTileX]);
					RF_RenderTile16m(tileScreenX, tileScreenY, CA_mapPlanes[1][dstTileY*rf_mapWidthTiles+dstTileX]);
				}
				// And check it for animations.
				RFL_CheckForAnimTile(dstTileX, dstTileY);
			}
		}
	}
}

void RFL_RenderForeTiles()
{
	int scrollXtile = rf_scrollXUnit >> 8;
	int scrollYtile = rf_scrollYUnit >> 8;

	for (int stx =  scrollXtile; stx < scrollXtile + RF_BUFFER_WIDTH_TILES; ++stx)
	{
		for (int sty = scrollYtile; sty < scrollYtile + RF_BUFFER_HEIGHT_TILES; ++sty)
		{
			int tile = CA_mapPlanes[1][sty*rf_mapWidthTiles+stx];
			int bufferX = stx - scrollXtile;
			int bufferY = sty - scrollYtile;
			if (!rf_dirtyBlocks[bufferY*RF_BUFFER_WIDTH_TILES+bufferX]) continue;
			if (!tile) continue;
			if (!(TI_ForeMisc(tile) & 0x80)) continue;
			VL_MaskedBlitToScreen(ca_graphChunks[ca_gfxInfoE.offTiles16m+tile],(stx - scrollXtile)*16,
											(sty - scrollYtile)*16,16,16);
		}
	}
}

// Renders a new horizontal row.
// dir: true = bottom, false = top
void RFL_NewRowHorz(bool dir)
{
	int bufferRow;
	int mapRow;
	if (dir)
	{
		bufferRow = RF_BUFFER_HEIGHT_TILES - 1;
	}
	else
	{
		bufferRow = 0;
	}
	mapRow = (rf_scrollYUnit >> 8) + bufferRow;
	int xOffset = (rf_scrollXUnit >> 8);

	// Add tiles to onscreen animation list
	for (int i = 0; i < RF_BUFFER_WIDTH_TILES; ++i)
	{
		RFL_CheckForAnimTile(i+xOffset,mapRow);
		RFL_MarkBlockDirty(i, bufferRow, 1);
		RF_RenderTile16(i,bufferRow,CA_mapPlanes[0][mapRow*rf_mapWidthTiles+xOffset+i]);
		RF_RenderTile16m(i,bufferRow,CA_mapPlanes[1][mapRow*rf_mapWidthTiles+xOffset+i]);
	}
}

// Renders a new vertical row.
// dir: true = right, false = left
void RFL_NewRowVert(bool dir)
{
	int bufferCol;
	int mapCol;
	if (dir)
	{
		bufferCol = RF_BUFFER_WIDTH_TILES - 1;
	}
	else
	{
		bufferCol = 0;
	}
	mapCol = (rf_scrollXUnit >> 8) + bufferCol;
	int yOffset = (rf_scrollYUnit >> 8);

	// Check for animated tiles
	for (int i = 0; i < RF_BUFFER_HEIGHT_TILES; ++i)
	{
		RFL_CheckForAnimTile(mapCol,i+yOffset);
		RFL_MarkBlockDirty(bufferCol, i, 1);
		RF_RenderTile16(bufferCol,i,CA_mapPlanes[0][(yOffset+i)*rf_mapWidthTiles+mapCol]);
		RF_RenderTile16m(bufferCol,i,CA_mapPlanes[1][(yOffset+i)*rf_mapWidthTiles+mapCol]);
	}
}


void RF_RepositionLimit(int scrollXunit, int scrollYunit)
{
	rf_scrollXUnit = scrollXunit;
	rf_scrollYUnit = scrollYunit;

	// Keep us within the map bounds.
	if (scrollXunit < rf_scrollXMinUnit) rf_scrollXUnit = rf_scrollXMinUnit;
	if (scrollYunit < rf_scrollYMinUnit) rf_scrollYUnit = rf_scrollYMinUnit;
	if (scrollXunit > rf_scrollXMaxUnit) rf_scrollXUnit = rf_scrollXMaxUnit;
	if (scrollYunit > rf_scrollYMaxUnit) rf_scrollYUnit = rf_scrollYMaxUnit;

	int scrollXtile = rf_scrollXUnit >> 8;
	int scrollYtile = rf_scrollYUnit >> 8;

	// Loop through the horizontal scroll blocks.
	for (int scrollBlockID = 0; scrollBlockID < rf_numHorzScrollBlocks; ++scrollBlockID)
	{
		// If one is in the left half of the screen...
		if (rf_horzScrollBlocks[scrollBlockID] >= scrollXtile &&
			scrollXtile + 10 >= rf_horzScrollBlocks[scrollBlockID])
		{
			// reposition ourselves to be to its right.
			rf_scrollXUnit = (rf_horzScrollBlocks[scrollBlockID] << 8) + 0x100;
			break;
		}

		// If one is in the right half of the screen...
		if (scrollXtile + 11 <= rf_horzScrollBlocks[scrollBlockID] &&
			scrollXtile + 20 >= rf_horzScrollBlocks[scrollBlockID])
		{
			// reposition ourselved to be at its left.
			rf_scrollXUnit = (rf_horzScrollBlocks[scrollBlockID] << 8) - 0x1400;
			break;
		}
	}

	// Loop through the vertical scroll blocks.
	for (int scrollBlockID = 0; scrollBlockID < rf_numVertScrollBlocks; ++scrollBlockID)
	{
		// If one is in the top half of the screen...
		if (rf_vertScrollBlocks[scrollBlockID] >= scrollYtile &&
			scrollYtile + 6 >= rf_vertScrollBlocks[scrollBlockID])
		{
			// reposition ourselves to be beneath it.
			rf_scrollYUnit = (rf_vertScrollBlocks[scrollBlockID] << 8) + 0x100;
			break;
		}

		// If one is in the bottom half of the screen...
		if (scrollYtile + 7 <= rf_vertScrollBlocks[scrollBlockID] &&
			scrollYtile + 13 >= rf_vertScrollBlocks[scrollBlockID])
		{
			// reposition ourselves to be above it.
			rf_scrollYUnit = (rf_vertScrollBlocks[scrollBlockID] << 8) - 0xD00;
			break;
		}
	}

}

void RFL_SmoothScrollLimit(int scrollXdelta, int scrollYdelta)
{
	rf_scrollXUnit += scrollXdelta;
	rf_scrollYUnit += scrollYdelta;

	int scrollXtile = rf_scrollXUnit >> 8;
	int scrollYtile = rf_scrollYUnit >> 8;

	if (scrollXdelta > 0)
	{
		scrollXtile += RF_SCREEN_WIDTH_TILES;
		for (int sb = 0; sb < rf_numHorzScrollBlocks; ++sb)
		{
			if (rf_horzScrollBlocks[sb] == scrollXtile)
			{
				rf_scrollXUnit &= ~0xff;
				break;
			}
		}
	}
	else if (scrollXdelta < 0)
	{
		for (int sb = 0; sb < rf_numHorzScrollBlocks; ++sb)
		{
			if (rf_horzScrollBlocks[sb] == scrollXtile)
			{
				rf_scrollXUnit &= ~0xff;
				rf_scrollXUnit += 256;
			}
		}
	}

	if (scrollYdelta > 0)
	{
		scrollYtile += RF_SCREEN_HEIGHT_TILES;
		for (int sb = 0; sb < rf_numVertScrollBlocks; ++sb)
		{
			if (rf_vertScrollBlocks[sb] == scrollYtile)
			{
				rf_scrollYUnit &= ~0xff;
				break;
			}
		}
	}
	else if (scrollYdelta < 0)
	{
		for (int sb = 0; sb < rf_numVertScrollBlocks; ++sb)
		{
			if (rf_vertScrollBlocks[sb] == scrollYtile)
			{
				rf_scrollYUnit &= ~0xff;
				rf_scrollYUnit += 256;
			}
		}
	}


}

void RFL_CalcTics()
{
	uint32_t inctime;
	if ((uint32_t)SD_GetLastTimeCount() > SD_GetTimeCount())
		SD_SetTimeCount(SD_GetLastTimeCount());
	if (in_demoState != IN_Demo_Off)
	{
		uint32_t new_time = SD_GetLastTimeCount();
		// TODO/FIXME: Better handling of this in the future
		while (new_time + 6 > SD_GetTimeCount())
		{
			// As long as this takes no more than 10ms...
			SDL_Delay(1);
		}
		// We do not want to lose demo sync
		SD_SetLastTimeCount(new_time + 3);
		SD_SetTimeCount(new_time + 6);
		SD_SetSpriteSync(3);
		return;
	}
	// TODO/FIXME: Better handling of this in the future
	do
	{
		inctime = SD_GetTimeCount();
		SD_SetSpriteSync((uint16_t)(inctime & 0xFFFF) - (uint16_t)(SD_GetLastTimeCount() & 0xFFFF));
		if (SD_GetSpriteSync() >= 2)
			break;
		// As long as this takes no more than 10ms...
		SDL_Delay(1);
	} while (1);
	SD_SetLastTimeCount(inctime);
	if (SD_GetSpriteSync() > 5)
	{
		SD_SetTimeCount(SD_GetTimeCount() - (SD_GetSpriteSync() - 5));
		SD_SetSpriteSync(5);
	}
}


void RF_Reposition(int scrollXunit, int scrollYunit)
{
	//TODO: Implement scrolling properly
	//NOTE: This should work now.
	RFL_RemoveAnimRect(rf_scrollXUnit >> 8, rf_scrollXUnit >> 8, RF_BUFFER_WIDTH_TILES, RF_BUFFER_HEIGHT_TILES);
	RF_RepositionLimit(scrollXunit, scrollYunit);
	int scrollXtile = rf_scrollXUnit >> 8;
	int scrollYtile = rf_scrollYUnit >> 8;

	RFL_SetupOnscreenAnimList();


	for (int ty = 0; ty < RF_BUFFER_HEIGHT_TILES; ++ty)
	{
		for (int tx = 0; tx < RF_BUFFER_WIDTH_TILES; ++tx)
		{
			RFL_CheckForAnimTile(tx+scrollXtile,ty+scrollYtile);
			RFL_MarkBlockDirty(tx,ty,1);
			RF_RenderTile16(tx,ty,CA_mapPlanes[0][(ty+scrollYtile) * rf_mapWidthTiles + tx + scrollXtile]);
			RF_RenderTile16m(tx,ty,CA_mapPlanes[1][(ty+scrollYtile) * rf_mapWidthTiles + tx + scrollXtile]);
		}
	}
}

void RF_SmoothScroll(int scrollXdelta, int scrollYdelta)
{
	int oldScrollXTile = (rf_scrollXUnit >> 8), oldScrollYTile = (rf_scrollYUnit >> 8);
	RFL_SmoothScrollLimit(scrollXdelta,scrollYdelta);
	int scrollXTileDelta = (rf_scrollXUnit >> 8) - oldScrollXTile;
	int scrollYTileDelta = (rf_scrollYUnit >> 8) - oldScrollYTile;


	// If we're not moving to a new tile at all, we can quit now.
	if (scrollXTileDelta == 0 && scrollYTileDelta == 0)
		return;


	if (scrollXTileDelta > 1 || scrollXTileDelta < -1 || scrollYTileDelta > 1 || scrollYTileDelta < -1)
	{
		// We redraw the whole thing if we move too much.
		RF_Reposition(rf_scrollXUnit, rf_scrollYUnit);
		return;
	}


	int dest_x = (scrollXTileDelta < 0)?16:0;
	int dest_y = (scrollYTileDelta < 0)?16:0;

	int src_x = (scrollXTileDelta > 0)?16:0;
	int src_y = (scrollYTileDelta > 0)?16:0;

	int wOffset = (scrollXTileDelta)?-16:0;
	int hOffset = (scrollYTileDelta)?-16:0;

	VL_SurfaceToSelf(rf_tileBuffer,dest_x,dest_y,src_x,src_y,RF_BUFFER_WIDTH_PIXELS+wOffset, RF_BUFFER_HEIGHT_PIXELS+hOffset);
	for (int y = 0; y < RF_BUFFER_HEIGHT_TILES; ++y)
	{
		for (int x = 0; x < RF_BUFFER_WIDTH_TILES; ++x)
		{
			RFL_MarkBlockDirty(x,y,1);
		}
	}

	if (scrollXTileDelta)
	{
		RFL_NewRowVert((scrollXTileDelta>0));
		if (scrollXTileDelta>0)
		{
			RFL_RemoveAnimCol((rf_scrollXUnit >> 8) - 1);
		}
		else
		{
			RFL_RemoveAnimCol((rf_scrollXUnit >> 8) + RF_BUFFER_WIDTH_TILES);
		}
	}

	if (scrollYTileDelta)
	{
		RFL_NewRowHorz((scrollYTileDelta>0));
		if (scrollYTileDelta>0)
		{
			RFL_RemoveAnimRow((rf_scrollYUnit >> 8) - 1);
		}
		else
		{
			RFL_RemoveAnimRow((rf_scrollYUnit >> 8) + RF_BUFFER_HEIGHT_TILES);
		}
	}
}


void RF_PlaceEraser(int pxX, int pxY, int pxW, int pxH)
{
#ifndef ALWAYS_REDRAW
	if (rf_freeSpriteEraserIndex == RF_MAX_SPRITETABLEENTRIES)
		Quit("Too many sprite erasers.");
	rf_spriteErasers[rf_freeSpriteEraserIndex].pxX = pxX;
	rf_spriteErasers[rf_freeSpriteEraserIndex].pxY = pxY;
	rf_spriteErasers[rf_freeSpriteEraserIndex].pxW = pxW;
	rf_spriteErasers[rf_freeSpriteEraserIndex].pxH = pxH;
	rf_freeSpriteEraserIndex++;
#endif
}

void RF_EraseRegion(int pxX, int pxY, int pxW, int pxH)
{
	int pixelX = pxX - (rf_scrollXUnit >> 8)*16;
	int pixelY = pxY - (rf_scrollYUnit >> 8)*16;
	int tileX1 = MAX(pixelX >> 4, 0);
	int tileY1 = MAX(pixelY >> 4, 0);
	int tileX2 = MIN((pixelX + pxW) >> 4, RF_BUFFER_WIDTH_TILES-1);
	int tileY2 = MIN((pixelY + pxH) >> 4, RF_BUFFER_HEIGHT_TILES-1);


}


void RF_RemoveSpriteDraw(RF_SpriteDrawEntry **drawEntry)
{
	if (!drawEntry) return;
	if (!(*drawEntry)) return;

	rf_numSpriteDraws--;
#if 0
	if (rf_firstSpriteTableEntry[(*drawEntry)->zLayer] == (*drawEntry))
	{
		(*drawEntry)->prev = 0;
		rf_firstSpriteTableEntry[(*drawEntry)->zLayer] = (*drawEntry)->next;
	}

	if ((*drawEntry)->prev)
		(*drawEntry)->prev->next = (*drawEntry)->next;

#endif
	int old_sprite_number = (*drawEntry)->chunk - ca_gfxInfoE.offSprites;
	VH_SpriteTableEntry oldSprite = VH_GetSpriteTableEntry(old_sprite_number);
	RF_PlaceEraser((*drawEntry)->x + (oldSprite.originX >> 4), (*drawEntry)->y + (oldSprite.originY >> 4), (*drawEntry)->sw, (*drawEntry)->sh);

	if ((*drawEntry)->next)
		(*drawEntry)->next->prevNextPtr = (*drawEntry)->prevNextPtr;

	(*((*drawEntry)->prevNextPtr)) = (*drawEntry)->next;
	(*drawEntry)->next = rf_freeSpriteTableEntry;
	rf_freeSpriteTableEntry = *drawEntry;
	*drawEntry = 0;
}

void RFL_ProcessSpriteErasers()
{
#ifndef ALWAYS_REDRAW
	for (int i = 0; i < rf_freeSpriteEraserIndex; ++i)
	{
		rf_spriteErasers[i].pxX -= (rf_scrollXUnit >> 8)*16;
		rf_spriteErasers[i].pxY -= (rf_scrollYUnit >> 8)*16;
		int x = MAX(rf_spriteErasers[i].pxX - 8, 0);
		int y = MAX(rf_spriteErasers[i].pxY - 8, 0);
		int x2 = MIN(rf_spriteErasers[i].pxX+rf_spriteErasers[i].pxW + 8, RF_BUFFER_WIDTH_PIXELS-1);
		int y2 = MIN(rf_spriteErasers[i].pxY+rf_spriteErasers[i].pxH + 8, RF_BUFFER_HEIGHT_PIXELS-1);

		// Only process if on-screen.
		if (x2 < x || y2 < y)
			continue;

		VL_SurfaceToScreen(rf_tileBuffer, x, y, x, y, x2-x, y2-y);

		// Mark the affected tiles dirty with '2', to force sprites to
		// redraw.
		int tileX1 = x >> 4;
		int tileY1 = y >> 4;
		int tileX2 = x2 >> 4;
		int tileY2 = y2 >> 4;
		for (int ty = tileY1; ty <= tileY2; ++ty)
		{
			for (int tx = tileX1; tx <= tileX2; ++tx)
			{
				RFL_MarkBlockDirty(tx, ty, 2);
			}
		}
	}
#endif
	//Reset
	rf_freeSpriteEraserIndex = 0;
}

void RF_AddSpriteDraw(RF_SpriteDrawEntry **drawEntry, int unitX, int unitY, int chunk, bool allWhite, int zLayer)
{
	bool insertNeeded = true;
	if (chunk <= 0)
	{
		RF_RemoveSpriteDraw(drawEntry);
		return;
	}

	RF_SpriteDrawEntry *sde = *drawEntry;

	if (sde)
	{
		int old_sprite_number = sde->chunk - ca_gfxInfoE.offSprites;
		VH_SpriteTableEntry oldSprite = VH_GetSpriteTableEntry(old_sprite_number);
		RF_PlaceEraser(sde->x + (oldSprite.originX >> 4), sde->y + (oldSprite.originY >> 4), sde->sw, sde->sh);

		//TODO: Support changing zLayers properly.
		if (zLayer == sde->zLayer)
		{
			insertNeeded = false;
		}
		else
		{
			if (sde->next)
			{
				sde->next->prevNextPtr = sde->prevNextPtr;
			}
			*(sde->prevNextPtr) = sde->next;
		}
	}
	else if (rf_freeSpriteTableEntry)
	{
		// Grab a new spritedraw pointer.
		sde = rf_freeSpriteTableEntry;
		rf_freeSpriteTableEntry = rf_freeSpriteTableEntry->next;
		rf_numSpriteDraws++;
	}
	else
	{
		Quit("RF_AddSpriteDraw: No free spots in spritearray");
	}

	// Add the SpriteDrawEntry to the table for its z-layer.

	if (insertNeeded)
	{
		if (rf_firstSpriteTableEntry[zLayer])
			rf_firstSpriteTableEntry[zLayer]->prevNextPtr = &sde->next;
		sde->next = rf_firstSpriteTableEntry[zLayer];
		rf_firstSpriteTableEntry[zLayer] = sde;
		sde->prevNextPtr = &rf_firstSpriteTableEntry[zLayer];
	}

	//TODO: Remove
	CA_CacheGrChunk(chunk);

	void *sprite_data = ca_graphChunks[chunk];
	if (!sprite_data)
		Quit("RF_AddSpriteDraw: Placed an uncached sprite");

	int sprite_number = chunk - ca_gfxInfoE.offSprites;



	sde->chunk = chunk;
	sde->zLayer = zLayer;
	sde->x = (unitX >> 4);
	sde->y = (unitY >> 4);
	sde->sw = VH_GetSpriteTableEntry(sprite_number).width*8;
	sde->sh = VH_GetSpriteTableEntry(sprite_number).height;
	sde->maskOnly = allWhite;
	sde->dirty = true;

	*drawEntry = sde;
}

void RFL_DrawSpriteList()
{
	for (int zLayer = 0; zLayer < RF_NUM_SPRITE_Z_LAYERS; ++zLayer)
	{
		// All but the final z layer (3) are below fore-foreground tiles.
		if (zLayer == 3)
			RFL_RenderForeTiles();

		for (RF_SpriteDrawEntry *sde = rf_firstSpriteTableEntry[zLayer]; sde; sde=sde->next)
		{
			int pixelX = sde->x - (rf_scrollXUnit >> 8)*16;
			int pixelY = sde->y - (rf_scrollYUnit >> 8)*16;
			int tileX1 = MAX(pixelX >> 4, 0);
			int tileY1 = MAX(pixelY >> 4, 0);
			int tileX2 = MIN((pixelX + sde->sw) >> 4, RF_BUFFER_WIDTH_TILES-1);
			int tileY2 = MIN((pixelY + sde->sh) >> 4, RF_BUFFER_HEIGHT_TILES-1);

			// Check the sprite is in-bounds.
			if (tileX2 < tileX1 || tileY2 < tileY1)
				continue;
#ifdef ALWAYS_REDRAW
			sde->dirty = 1;
#endif

			if (!sde->dirty)
			{
				for (int y = tileY1; y <= tileY2; ++y)
				{
					for (int x = tileX1; x <= tileX2; ++x)
					{
						if (rf_dirtyBlocks[y*RF_BUFFER_WIDTH_TILES+x])
						{
							sde->dirty = true;
							goto drawSprite;
						}
					}
				}

			}

drawSprite:
			if (sde->dirty)
			{
				if (sde->maskOnly)
				{
					VH_DrawSpriteMask(pixelX, pixelY, sde->chunk, 15);
				}
				else
				{
					VH_DrawSprite(pixelX, pixelY, sde->chunk);
				}
				for (int y = tileY1; y <= tileY2; ++y)
				{
					for (int x = tileX1; x <= tileX2; ++x)
					{
						RFL_MarkBlockDirty(x,y,3);
					}
				}
				sde->dirty = false;

			}

		}
	}
}

RF_SpriteDrawEntry *tmp = 0;

void RFL_UpdateTiles()
{
#ifndef ALWAYS_REDRAW
	for (int y = 0; y < RF_BUFFER_HEIGHT_TILES; ++y)
	{
		for (int x = 0; x < RF_BUFFER_WIDTH_TILES; ++x)
		{
			if (rf_dirtyBlocks[y*RF_BUFFER_WIDTH_TILES+x] == 1)
				VL_SurfaceToScreen(rf_tileBuffer,x*16, y*16, x*16, y*16, 16, 16);
		}
	}
#endif
}

void RF_Refresh()
{
	RFL_AnimateTiles();

#ifdef ALWAYS_REDRAW
	VL_SurfaceToScreen(rf_tileBuffer,0,0,0,0,RF_BUFFER_WIDTH_PIXELS,RF_BUFFER_HEIGHT_PIXELS);
#endif

	//TODO: Work out how to do scrolling before using this
	RFL_UpdateTiles();
	RFL_ProcessSpriteErasers();

	RFL_DrawSpriteList();

	// No blocks should be dirty after the frame has been rendered.
	for (int y = 0; y < RF_BUFFER_HEIGHT_TILES; ++y)
	{
		for (int x = 0; x < RF_BUFFER_WIDTH_TILES; ++x)
		{
			RFL_MarkBlockDirty(x,y,0);
		}
	}

	if (rf_drawFunc)
		rf_drawFunc();

	RFL_CalcTics();
}

