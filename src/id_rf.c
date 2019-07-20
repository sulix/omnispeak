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
#include "id_rf.h"
#include "id_ca.h"
#include "id_mm.h"
#include "id_ti.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"
#include "ck_cross.h"
#include "ck_ep.h"
#include "ck_play.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Proper dirty-rectangle drawing is not working yet. Disable it for now.
#define ALWAYS_REDRAW

// Maximum number of pages we can write to.
#define RF_MAX_BUFFERS 2

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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

int rf_freeSpriteEraserIndex[RF_MAX_BUFFERS] = {0};
RF_SpriteEraser rf_spriteErasers[RF_MAX_SPRITETABLEENTRIES * RF_MAX_BUFFERS];
// Animated tile management
// (This is hairy)

// A pointer (index into array when 32-bit) to this struct
// is stored in the info-plane of each animated tile.

typedef struct RF_AnimTileTimer
{
	int tileNumber;
	int timeToSwitch;
	// These are in Keen 6 only, used for emitting sounds (levels 6, 8)
	int tileWithSound;
	int numOfOnScreenTiles;
	int sound;
} RF_AnimTileTimer;

/*** Used for saved games compatibility ***/
static uint16_t RFL_ConvertAnimTileTimerIndexTo16BitOffset(int i)
{
	return ck_currentEpisode->animTileSize * i + ck_currentEpisode->animTilesOffset;
}

static int RFL_ConvertAnimTileTimer16BitOffsetToIndex(uint16_t offset)
{
	return (offset - ck_currentEpisode->animTilesOffset) / ck_currentEpisode->animTileSize;
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
#define RF_BUFFER_SIZE (RF_BUFFER_WIDTH_TILES * RF_BUFFER_HEIGHT_TILES)
uint8_t rf_dirtyBlocks[RF_MAX_BUFFERS * RF_BUFFER_SIZE];

// An offset added to the dirty block buffer to account for scrolling.
int rf_dirtyBufferOffset = 0;

void RFL_MarkBlockDirty(int x, int y, int val, int page)
{
	if (x >= RF_BUFFER_WIDTH_TILES || y >= RF_BUFFER_HEIGHT_TILES)
		return;
	if (page == -1)
	{
		for (int _page = 0; _page < VL_GetNumBuffers(); ++_page)
		{
			RFL_MarkBlockDirty(x, y, val, _page);
		}
	}
	else
	{
		if (rf_dirtyBufferOffset < 0)
			rf_dirtyBufferOffset += RF_BUFFER_SIZE;
		size_t offset = ((y * RF_BUFFER_WIDTH_TILES + x) +
					(page * RF_BUFFER_SIZE) + rf_dirtyBufferOffset) %
			(RF_MAX_BUFFERS * RF_BUFFER_SIZE);
		rf_dirtyBlocks[offset] = val;
	}
}

uint8_t RFL_IsBlockDirty(int x, int y, int page)
{
	if (x >= RF_BUFFER_WIDTH_TILES || y >= RF_BUFFER_HEIGHT_TILES)
		return 0;
	if (page == -1)
		return RFL_IsBlockDirty(x, y, VL_GetActiveBuffer());

	if (rf_dirtyBufferOffset < 0)
		rf_dirtyBufferOffset += RF_BUFFER_SIZE;
	size_t offset = ((y * RF_BUFFER_WIDTH_TILES + x) +
				(page * RF_BUFFER_SIZE) + rf_dirtyBufferOffset) %
		(RF_MAX_BUFFERS * RF_BUFFER_SIZE);
	return rf_dirtyBlocks[offset];
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
		rf_onscreenAnimTiles[i].next = &rf_onscreenAnimTiles[i + 1];
	}

	rf_onscreenAnimTiles[RF_MAX_ONSCREENANIMTILES - 1].next = 0;

	rf_firstOnscreenAnimTile = 0;

	if (ck_currentEpisode->ep == EP_CK6)
		for (RF_AnimTileTimer *animTileTimer = rf_animTileTimers; animTileTimer->tileNumber; ++animTileTimer)
			animTileTimer->numOfOnScreenTiles = 0;
}

void RFL_SetupSpriteTable()
{
	rf_freeSpriteTableEntry = rf_spriteTable;
	rf_numSpriteDraws = 0;

	for (int i = 0; i < RF_MAX_SPRITETABLEENTRIES - 1; ++i)
	{
		rf_spriteTable[i].next = &rf_spriteTable[i + 1];
		rf_spriteTable[i + 1].prevNextPtr = &(rf_spriteTable[i].next);
	}

	rf_spriteTable[RF_MAX_SPRITETABLEENTRIES - 1].next = 0;

	for (int i = 0; i < RF_NUM_SPRITE_Z_LAYERS; ++i)
	{
		rf_firstSpriteTableEntry[i] = 0;
	}
}

// Some Keen 6 specific function

void RFL_MarkTileWithSound(RF_AnimTileTimer *animTileTimer, int tile)
{
	// FIXME: Define the sounds in ck6_misc.c?
	static const uint16_t tilesWithSounds[4] = {0x8868, 0x88a0, 58, 59};
	for (int i = 0; i < 2; ++i)
		if (tile == tilesWithSounds[i])
		{
			animTileTimer->tileWithSound = tile;
			animTileTimer->sound = tilesWithSounds[i + 2];
			return;
		}
}

void RF_MarkTileGraphics()
{
	memset(rf_animTileTimers, 0, sizeof(rf_animTileTimers));
	rf_numAnimTileTimers = 0;
	// WARNING: As in the original codebase, the given variable is NOT initialized.
	// This may lead to undefined behaviors in calls to RFL_MarkTileWithSound,
	// although they aren't reproduced in vanilla Keen 6 in practice.
	int i;
	for (int tileY = 0; tileY < rf_mapHeightTiles; ++tileY)
	{
		for (int tileX = 0; tileX < rf_mapWidthTiles; ++tileX)
		{
			bool needNewTimer = true;
			int backTile = CA_mapPlanes[0][tileY * rf_mapWidthTiles + tileX];

			CA_MarkGrChunk(ca_gfxInfoE.offTiles16 + backTile);
			if (TI_BackAnimTile(backTile))
			{
				if (TI_BackAnimTime(backTile))
				{
					// Is the info-plane free?
					// It doesn't need to be.  A spawn-code can be placed over an animating tile
					// and the map will operate properly
#if 0
					if (CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX])
						Quit("RF_MarkTileGraphics: Info plane above animated bg tile in use.");
#endif

					for (i = 0; i < rf_numAnimTileTimers; ++i)
					{
						if (rf_animTileTimers[i].tileNumber == backTile)
						{
							// Add the timer index to the info-plane
							CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
							needNewTimer = false;
							break;
						}
					}

					if (needNewTimer)
					{
						if (i >= RF_MAX_ANIMTILETIMERS)
							Quit("RF_MarkTileGraphics: Too many unique animations");

						RF_AnimTileTimer *animTileTimer = &rf_animTileTimers[i];
						animTileTimer->tileNumber = backTile;
						animTileTimer->timeToSwitch = TI_BackAnimTime(backTile);
						if (ck_currentEpisode->ep == EP_CK6)
						{
							animTileTimer->numOfOnScreenTiles = 0;
							animTileTimer->sound = -1;
						}
						CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
						rf_numAnimTileTimers++;
					}
					else
						continue;
				}

				if (ck_currentEpisode->ep == EP_CK6)
					RFL_MarkTileWithSound(&rf_animTileTimers[i], backTile);
				// Mark all tiles in the animation loop to be cached.
				int animLength = 0;
				int nextTile = backTile + TI_BackAnimTile(backTile);
				while (TI_BackAnimTile(nextTile) && (nextTile != backTile))
				{
					if (ck_currentEpisode->ep == EP_CK6)
						RFL_MarkTileWithSound(&rf_animTileTimers[i], nextTile);
					CA_MarkGrChunk(ca_gfxInfoE.offTiles16 + nextTile);
					nextTile += TI_BackAnimTile(nextTile);
					if (++animLength > RF_MAX_ANIM_LOOP)
					{
						Quit("RF_MarkTileGraphics: Unending background animation");
					}
				}
			}
		}
	}
	for (int tileY = 0; tileY < rf_mapHeightTiles; ++tileY)
	{
		for (int tileX = 0; tileX < rf_mapWidthTiles; ++tileX)
		{
			bool needNewTimer = true;
			int foreTile = CA_mapPlanes[1][tileY * rf_mapWidthTiles + tileX];

			CA_MarkGrChunk(ca_gfxInfoE.offTiles16m + foreTile);
			if (TI_ForeAnimTile(foreTile))
			{
				if (TI_ForeAnimTime(foreTile))
				{
					// Is the info-plane free?
					//if (CA_mapPlanes[2][tileY*rf_mapWidthTiles+tileX])
					//	Quit("RF_MarkTileGraphics: Info plane above animated fg tile in use.");

					for (i = 0; i < rf_numAnimTileTimers; ++i)
					{
						if (rf_animTileTimers[i].tileNumber == (foreTile | 0x8000))
						{
							// Add the timer index to the info-plane
							CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
							needNewTimer = false;
							break;
						}
					}

					if (needNewTimer)
					{
						if (i >= RF_MAX_ANIMTILETIMERS)
							Quit("RF_MarkTileGraphics: Too many unique animations");

						RF_AnimTileTimer *animTileTimer = &rf_animTileTimers[i];
						animTileTimer->tileNumber = foreTile | 0x8000;
						animTileTimer->timeToSwitch = TI_ForeAnimTime(foreTile);
						if (ck_currentEpisode->ep == EP_CK6)
						{
							animTileTimer->numOfOnScreenTiles = 0;
							animTileTimer->sound = -1;
						}
						CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX] = RFL_ConvertAnimTileTimerIndexTo16BitOffset(i);
						rf_numAnimTileTimers++;
					}
					else
						continue;
				}
				if (ck_currentEpisode->ep == EP_CK6)
					RFL_MarkTileWithSound(&rf_animTileTimers[i], foreTile | 0x8000);

				// Mark all tiles in the animation loop to be cached.
				int animLength = 0;
				int nextTile = foreTile + TI_ForeAnimTile(foreTile);
				while (TI_ForeAnimTile(nextTile) && (nextTile != foreTile))
				{
					if (ck_currentEpisode->ep == EP_CK6)
						RFL_MarkTileWithSound(&rf_animTileTimers[i], nextTile | 0x8000);
					CA_MarkGrChunk(ca_gfxInfoE.offTiles16m + nextTile);
					nextTile += TI_ForeAnimTile(nextTile);
					if (++animLength > RF_MAX_ANIM_LOOP)
					{
						Quit("RF_MarkTileGraphics: Unending foreground animation");
					}
				}
			}
		}
	}
}

void RFL_CheckForAnimTile(int tileX, int tileY)
{
	int backTile = CA_mapPlanes[0][tileY * rf_mapWidthTiles + tileX];
	int foreTile = CA_mapPlanes[1][tileY * rf_mapWidthTiles + tileX];

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
		ost->timerIndex = RFL_ConvertAnimTileTimer16BitOffsetToIndex(CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX]);
		if (ck_currentEpisode->ep == EP_CK6)
			++rf_animTileTimers[ost->timerIndex].numOfOnScreenTiles;

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
		ost->timerIndex = RFL_ConvertAnimTileTimer16BitOffsetToIndex(CA_mapPlanes[2][tileY * rf_mapWidthTiles + tileX]);
		if (ck_currentEpisode->ep == EP_CK6)
			++rf_animTileTimers[ost->timerIndex].numOfOnScreenTiles;

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
		if ((ost->tileX >= tileX && ost->tileX < (tileX + tileW)) && (ost->tileY >= tileY && ost->tileY < (tileY + tileH)))
		{
			if (ck_currentEpisode->ep == EP_CK6)
				--rf_animTileTimers[ost->timerIndex].numOfOnScreenTiles;
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev) ? prev : rf_firstOnscreenAnimTile;
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
			if (ck_currentEpisode->ep == EP_CK6)
				--rf_animTileTimers[ost->timerIndex].numOfOnScreenTiles;
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev) ? prev : rf_firstOnscreenAnimTile;
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
			if (ck_currentEpisode->ep == EP_CK6)
				--rf_animTileTimers[ost->timerIndex].numOfOnScreenTiles;
			RF_OnscreenAnimTile *obsolete = ost;
			if (prev)
				prev->next = ost->next;
			else
				rf_firstOnscreenAnimTile = ost->next;
			ost = (prev) ? prev : rf_firstOnscreenAnimTile;
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
		rf_animTileTimers[i].timeToSwitch -= SD_GetSpriteSync();

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

			if (ck_currentEpisode->ep == EP_CK6)
				if (rf_animTileTimers[i].numOfOnScreenTiles && (rf_animTileTimers[i].tileNumber == rf_animTileTimers[i].tileWithSound) && (rf_animTileTimers[i].sound != (uint16_t)-1))
					SD_PlaySound(rf_animTileTimers[i].sound);
		}
	}

	// Update the onscreen tiles.
	for (RF_OnscreenAnimTile *ost = rf_firstOnscreenAnimTile; ost; ost = ost->next)
	{
		int tile = rf_animTileTimers[ost->timerIndex].tileNumber & ~0x8000;
		if (tile != ost->tile)
		{
			ost->tile = tile;
			int screenTileX = ost->tileX - RF_UnitToTile(rf_scrollXUnit);
			int screenTileY = ost->tileY - RF_UnitToTile(rf_scrollYUnit);

			if (screenTileX < 0 || screenTileX > RF_BUFFER_WIDTH_TILES || screenTileY < 0 || screenTileY > RF_BUFFER_HEIGHT_TILES)
			{
				//printf("Out of bounds: %d, %d (sc: %d,%d) (tl: %d,%d,%d):%d\n", screenTileX, screenTileY,
				//	rf_scrollXUnit >> 8, rf_scrollYUnit >> 8, ost->tileX, ost->tileY,ost->plane, ost->tile);
				Quit("RFL_AnimateTiles: Out of bounds!");
			}

			CA_mapPlanes[ost->plane][ost->tileY * rf_mapWidthTiles + ost->tileX] = tile;

			RF_RenderTile16(screenTileX, screenTileY, CA_mapPlanes[0][ost->tileY * rf_mapWidthTiles + ost->tileX]);
			RF_RenderTile16m(screenTileX, screenTileY, CA_mapPlanes[1][ost->tileY * rf_mapWidthTiles + ost->tileX]);
			RFL_MarkBlockDirty(screenTileX, screenTileY, 1, -1);
		}
	}
}

void (*rf_drawFunc)(void);

void RF_SetDrawFunc(void (*func)(void))
{
	rf_drawFunc = func;
}

void RF_Startup()
{
	// Create the tile backing buffer
	rf_tileBuffer = VL_CreateSurface(RF_BUFFER_WIDTH_PIXELS, RF_BUFFER_HEIGHT_PIXELS);
}

void RF_Shutdown()
{
	VL_DestroySurface(rf_tileBuffer);
}

// TODO: More to change? Also, originally mapNum is a global variable.
void RF_NewMap(void)
{
	rf_mapWidthTiles = CA_MapHeaders[ca_mapOn]->width;
	rf_mapHeightTiles = CA_MapHeaders[ca_mapOn]->height;
	rf_scrollXMinUnit = 0x0200; //Two-tile wide border around map
	rf_scrollYMinUnit = 0x0200;
	rf_scrollXMaxUnit = RF_TileToUnit(CA_MapHeaders[ca_mapOn]->width - RF_SCREEN_WIDTH_TILES - 2);
	rf_scrollYMaxUnit = RF_TileToUnit(CA_MapHeaders[ca_mapOn]->height - RF_SCREEN_HEIGHT_TILES - 2);

	// Reset the scroll-blocks
	rf_numVertScrollBlocks = rf_numHorzScrollBlocks = 0;

	RFL_SetupOnscreenAnimList();
	RFL_SetupSpriteTable();
	//RF_MarkTileGraphics();

	for (int page = 0; page < VL_GetNumBuffers(); ++page)
		rf_freeSpriteEraserIndex[page] = 0;

	// Set-up a two-tile wide border
	RF_SetScrollBlock(0, 1, true);
	RF_SetScrollBlock(0, CA_MapHeaders[ca_mapOn]->height - 2, true);
	RF_SetScrollBlock(1, 0, false);
	RF_SetScrollBlock(CA_MapHeaders[ca_mapOn]->width - 2, 0, false);

	SD_SetLastTimeCount(SD_GetTimeCount());
	SD_SetSpriteSync(1);
}

void RF_RenderTile16(int x, int y, int tile)
{
	//TODO: We should be able to remove this at some point, as it should have already been cached by
	// CacheMarks. Last time I tried that, I recall it failing, but it's something we should investigate
	// at some point.
	if (!ca_graphChunks[ca_gfxInfoE.offTiles16 + tile])
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Tried to render a Tile16 which was not cached. tile = %d, chunk = %d.\n", tile, ca_gfxInfoE.offTiles16 + tile);
		CA_CacheGrChunk(ca_gfxInfoE.offTiles16 + tile);
	}

	// Some levels, notably Keen 6's "Guard Post 3" use empty background tiles (i.e. tiles with offset
	// FFFFFF in EGAHEAD). CA_CacheGrChunk() leaves these as NULL pointers. As we'd otherwise crash,
	// we just skip rendering these (though the possibility is there to use hardcoded null pointer data
	// as with F10+Y). At least in this case, the tile is hidden anyway, so it doesn't matter.
	if (!ca_graphChunks[ca_gfxInfoE.offTiles16 + tile])
		return;

	VL_UnmaskedToSurface(ca_graphChunks[ca_gfxInfoE.offTiles16 + tile], rf_tileBuffer, x * 16, y * 16, 16, 16);
}

void RF_RenderTile16m(int x, int y, int tile)
{
	if (!tile)
		return;
	if (!ca_graphChunks[ca_gfxInfoE.offTiles16m + tile])
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Tried to render a Tile16m which was not cached. tile = %d, chunk = %d.\n", tile, ca_gfxInfoE.offTiles16m + tile);
		CA_CacheGrChunk(ca_gfxInfoE.offTiles16m + tile);
	}
	VL_MaskedBlitToSurface(ca_graphChunks[ca_gfxInfoE.offTiles16m + tile], rf_tileBuffer, x * 16, y * 16, 16, 16);
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
	intptr_t src_offset, dst_offset, offset_delta, new_row_offset;

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

			src_bgtile_ptr = &CA_mapPlanes[0][(srcy + ty) * rf_mapWidthTiles + (srcx + tx)];
			src_fgtile_ptr = &CA_mapPlanes[1][(srcy + ty) * rf_mapWidthTiles + (srcx + tx)];
			src_infotile_ptr = &CA_mapPlanes[2][(srcy + ty) * rf_mapWidthTiles + (srcx + tx)];
			dst_bgtile_ptr = &CA_mapPlanes[0][(desty + ty) * rf_mapWidthTiles + (destx + tx)];
			dst_fgtile_ptr = &CA_mapPlanes[1][(desty + ty) * rf_mapWidthTiles + (destx + tx)];
			dst_infotile_ptr = &CA_mapPlanes[2][(desty + ty) * rf_mapWidthTiles + (destx + tx)];

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
			screenX = destx + tx - RF_UnitToTile(rf_scrollXUnit);
			screenY = desty + ty - RF_UnitToTile(rf_scrollYUnit);
			if (screenX >= 0 && screenY >= 0 && screenY < RF_BUFFER_HEIGHT_TILES && screenX < RF_BUFFER_WIDTH_TILES)
			{
				// Redraw it if it's different.
				if (different)
				{
					RFL_MarkBlockDirty(screenX, screenY, 1, -1);
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

			int tileScreenX = dstTileX - RF_UnitToTile(rf_scrollXUnit);
			int tileScreenY = dstTileY - RF_UnitToTile(rf_scrollYUnit);
			int oldTile = CA_mapPlanes[plane][dstTileY * rf_mapWidthTiles + dstTileX];
			int newTile = tilePtr[y * width + x];
			// Update the tile on the map.
			if (oldTile != newTile)
			{
				CA_mapPlanes[plane][dstTileY * rf_mapWidthTiles + dstTileX] = newTile;
			}
			// If the tile is onscreen...
			if (tileScreenX >= 0 && tileScreenX < RF_BUFFER_WIDTH_TILES &&
				tileScreenY >= 0 && tileScreenY < RF_BUFFER_HEIGHT_TILES)
			{
				// Redraw it if it has changed.
				if (oldTile != newTile)
				{
					RFL_MarkBlockDirty(tileScreenX, tileScreenY, 1, -1);
					RF_RenderTile16(tileScreenX, tileScreenY, CA_mapPlanes[0][dstTileY * rf_mapWidthTiles + dstTileX]);
					RF_RenderTile16m(tileScreenX, tileScreenY, CA_mapPlanes[1][dstTileY * rf_mapWidthTiles + dstTileX]);
				}
				// And check it for animations.
				RFL_CheckForAnimTile(dstTileX, dstTileY);
			}
		}
	}
}

void RFL_RenderForeTiles()
{
	int scrollXtile = RF_UnitToTile(rf_scrollXUnit);
	int scrollYtile = RF_UnitToTile(rf_scrollYUnit);

	for (int stx = scrollXtile; stx < scrollXtile + RF_BUFFER_WIDTH_TILES; ++stx)
	{
		for (int sty = scrollYtile; sty < scrollYtile + RF_BUFFER_HEIGHT_TILES; ++sty)
		{
			int tile = CA_mapPlanes[1][sty * rf_mapWidthTiles + stx];
			int bufferX = stx - scrollXtile;
			int bufferY = sty - scrollYtile;
#ifndef ALWAYS_REDRAW
			if (!RFL_IsBlockDirty(bufferX, bufferY, -1))
				continue;
#endif
			if (!tile)
				continue;
			if (!(TI_ForeMisc(tile) & 0x80))
				continue;
			VL_MaskedBlitToScreen(ca_graphChunks[ca_gfxInfoE.offTiles16m + tile], (stx - scrollXtile) * 16,
				(sty - scrollYtile) * 16, 16, 16);
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
	mapRow = RF_UnitToTile(rf_scrollYUnit) + bufferRow;
	int xOffset = RF_UnitToTile(rf_scrollXUnit);

	// Add tiles to onscreen animation list
	for (int i = 0; i < RF_BUFFER_WIDTH_TILES; ++i)
	{
		RFL_CheckForAnimTile(i + xOffset, mapRow);
		RFL_MarkBlockDirty(i, bufferRow, 1, -1);
		RF_RenderTile16(i, bufferRow, CA_mapPlanes[0][mapRow * rf_mapWidthTiles + xOffset + i]);
		RF_RenderTile16m(i, bufferRow, CA_mapPlanes[1][mapRow * rf_mapWidthTiles + xOffset + i]);
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
	mapCol = RF_UnitToTile(rf_scrollXUnit) + bufferCol;
	int yOffset = RF_UnitToTile(rf_scrollYUnit);

	// Check for animated tiles
	for (int i = 0; i < RF_BUFFER_HEIGHT_TILES; ++i)
	{
		RFL_CheckForAnimTile(mapCol, i + yOffset);
		RFL_MarkBlockDirty(bufferCol, i, 1, -1);
		RF_RenderTile16(bufferCol, i, CA_mapPlanes[0][(yOffset + i) * rf_mapWidthTiles + mapCol]);
		RF_RenderTile16m(bufferCol, i, CA_mapPlanes[1][(yOffset + i) * rf_mapWidthTiles + mapCol]);
	}
}

void RF_RepositionLimit(int scrollXunit, int scrollYunit)
{
	rf_scrollXUnit = scrollXunit;
	rf_scrollYUnit = scrollYunit;

	// Keep us within the map bounds.
	if (scrollXunit < rf_scrollXMinUnit)
		rf_scrollXUnit = rf_scrollXMinUnit;
	if (scrollYunit < rf_scrollYMinUnit)
		rf_scrollYUnit = rf_scrollYMinUnit;
	if (scrollXunit > rf_scrollXMaxUnit)
		rf_scrollXUnit = rf_scrollXMaxUnit;
	if (scrollYunit > rf_scrollYMaxUnit)
		rf_scrollYUnit = rf_scrollYMaxUnit;

	int scrollXtile = RF_UnitToTile(rf_scrollXUnit);
	int scrollYtile = RF_UnitToTile(rf_scrollYUnit);

	// Loop through the horizontal scroll blocks.
	for (int scrollBlockID = 0; scrollBlockID < rf_numHorzScrollBlocks; ++scrollBlockID)
	{
		// If one is in the left half of the screen...
		if (rf_horzScrollBlocks[scrollBlockID] >= scrollXtile &&
			scrollXtile + 10 >= rf_horzScrollBlocks[scrollBlockID])
		{
			// reposition ourselves to be to its right.
			rf_scrollXUnit = RF_TileToUnit(rf_horzScrollBlocks[scrollBlockID]) + 0x100;
			break;
		}

		// If one is in the right half of the screen...
		if (scrollXtile + 11 <= rf_horzScrollBlocks[scrollBlockID] &&
			scrollXtile + 20 >= rf_horzScrollBlocks[scrollBlockID])
		{
			// reposition ourselved to be at its left.
			rf_scrollXUnit = RF_TileToUnit(rf_horzScrollBlocks[scrollBlockID]) - 0x1400;
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
			rf_scrollYUnit = RF_TileToUnit(rf_vertScrollBlocks[scrollBlockID]) + 0x100;
			break;
		}

		// If one is in the bottom half of the screen...
		if (scrollYtile + 7 <= rf_vertScrollBlocks[scrollBlockID] &&
			scrollYtile + 13 >= rf_vertScrollBlocks[scrollBlockID])
		{
			// reposition ourselves to be above it.
			rf_scrollYUnit = RF_TileToUnit(rf_vertScrollBlocks[scrollBlockID]) - 0xD00;
			break;
		}
	}
}

void RFL_SmoothScrollLimit(int scrollXdelta, int scrollYdelta)
{
	rf_scrollXUnit += scrollXdelta;
	rf_scrollYUnit += scrollYdelta;

	int scrollXtile = RF_UnitToTile(rf_scrollXUnit);
	int scrollYtile = RF_UnitToTile(rf_scrollYUnit);

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
			VL_Yield();
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
		VL_Yield();
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
	RFL_RemoveAnimRect(RF_UnitToTile(rf_scrollXUnit), RF_UnitToTile(rf_scrollYUnit), RF_BUFFER_WIDTH_TILES, RF_BUFFER_HEIGHT_TILES);
	RF_RepositionLimit(scrollXunit, scrollYunit);
	int scrollXtile = RF_UnitToTile(rf_scrollXUnit);
	int scrollYtile = RF_UnitToTile(rf_scrollYUnit);

	RFL_SetupOnscreenAnimList();

	for (int ty = 0; ty < RF_BUFFER_HEIGHT_TILES; ++ty)
	{
		for (int tx = 0; tx < RF_BUFFER_WIDTH_TILES; ++tx)
		{
			RFL_CheckForAnimTile(tx + scrollXtile, ty + scrollYtile);
			RF_RenderTile16(tx, ty, CA_mapPlanes[0][(ty + scrollYtile) * rf_mapWidthTiles + tx + scrollXtile]);
			RF_RenderTile16m(tx, ty, CA_mapPlanes[1][(ty + scrollYtile) * rf_mapWidthTiles + tx + scrollXtile]);
			RFL_MarkBlockDirty(tx, ty, 1, -1);
		}
	}
}

void RF_SmoothScroll(int scrollXdelta, int scrollYdelta)
{
	int oldScrollXTile = RF_UnitToTile(rf_scrollXUnit), oldScrollYTile = RF_UnitToTile(rf_scrollYUnit);
	RFL_SmoothScrollLimit(scrollXdelta, scrollYdelta);
	int scrollXTileDelta = RF_UnitToTile(rf_scrollXUnit) - oldScrollXTile;
	int scrollYTileDelta = RF_UnitToTile(rf_scrollYUnit) - oldScrollYTile;

	// If we're not moving to a new tile at all, we can quit now.
	if (scrollXTileDelta == 0 && scrollYTileDelta == 0)
		return;

	if (scrollXTileDelta > 1 || scrollXTileDelta < -1 || scrollYTileDelta > 1 || scrollYTileDelta < -1)
	{
		// We redraw the whole thing if we move too much.
		RF_Reposition(rf_scrollXUnit, rf_scrollYUnit);
		return;
	}

	int dest_x = (scrollXTileDelta < 0) ? 16 : 0;
	int dest_y = (scrollYTileDelta < 0) ? 16 : 0;

	int src_x = (scrollXTileDelta > 0) ? 16 : 0;
	int src_y = (scrollYTileDelta > 0) ? 16 : 0;

	int wOffset = (scrollXTileDelta) ? -16 : 0;
	int hOffset = (scrollYTileDelta) ? -16 : 0;

	VL_SurfaceToSelf(rf_tileBuffer, dest_x, dest_y, src_x, src_y, RF_BUFFER_WIDTH_PIXELS + wOffset, RF_BUFFER_HEIGHT_PIXELS + hOffset);
	VL_ScrollScreen(scrollXTileDelta * 16, scrollYTileDelta * 16);

	// Scroll the dirty block buffer.
	rf_dirtyBufferOffset += scrollXTileDelta + scrollYTileDelta * RF_BUFFER_WIDTH_TILES;
	if (rf_dirtyBufferOffset < 0)
		rf_dirtyBufferOffset += RF_BUFFER_SIZE;

	if (scrollXTileDelta)
	{
		RFL_NewRowVert((scrollXTileDelta > 0));
		if (scrollXTileDelta > 0)
		{
			RFL_RemoveAnimCol(RF_UnitToTile(rf_scrollXUnit) - 1);
		}
		else
		{
			RFL_RemoveAnimCol(RF_UnitToTile(rf_scrollXUnit) + RF_BUFFER_WIDTH_TILES);
		}
	}

	if (scrollYTileDelta)
	{
		RFL_NewRowHorz((scrollYTileDelta > 0));
		if (scrollYTileDelta > 0)
		{
			RFL_RemoveAnimRow(RF_UnitToTile(rf_scrollYUnit) - 1);
		}
		else
		{
			RFL_RemoveAnimRow(RF_UnitToTile(rf_scrollYUnit) + RF_BUFFER_HEIGHT_TILES);
		}
	}
}

void RF_PlaceEraser(int pxX, int pxY, int pxW, int pxH)
{
#ifndef ALWAYS_REDRAW
	for (int page = 0; page < VL_GetNumBuffers(); ++page)
	{
		int arrayBase = RF_MAX_SPRITETABLEENTRIES * page;
		if (rf_freeSpriteEraserIndex[page] == RF_MAX_SPRITETABLEENTRIES)
			Quit("Too many sprite erasers.");
		int newIndex = rf_freeSpriteEraserIndex[page] + arrayBase;
		rf_spriteErasers[newIndex].pxX = pxX;
		rf_spriteErasers[newIndex].pxY = pxY;
		rf_spriteErasers[newIndex].pxW = pxW;
		rf_spriteErasers[newIndex].pxH = pxH;
		rf_freeSpriteEraserIndex[page]++;
	}
#endif
}

void RF_EraseRegion(int pxX, int pxY, int pxW, int pxH)
{
	int pixelX = pxX - RF_UnitToTile(rf_scrollXUnit) * 16;
	int pixelY = pxY - RF_UnitToTile(rf_scrollYUnit) * 16;
	int tileX1 = MAX(RF_PixelToTile(pixelX), 0);
	int tileY1 = MAX(RF_PixelToTile(pixelY), 0);
	int tileX2 = MIN(RF_PixelToTile(pixelX + pxW), RF_BUFFER_WIDTH_TILES - 1);
	int tileY2 = MIN(RF_PixelToTile(pixelY + pxH), RF_BUFFER_HEIGHT_TILES - 1);
}

void RF_RemoveSpriteDraw(RF_SpriteDrawEntry **drawEntry)
{
	if (!drawEntry)
		return;
	if (!(*drawEntry))
		return;

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
	VH_SpriteTableEntry *oldSprite = VH_GetSpriteTableEntry(old_sprite_number);
	RF_PlaceEraser((*drawEntry)->x + RF_UnitToPixel(oldSprite->originX), (*drawEntry)->y + RF_UnitToPixel(oldSprite->originY), (*drawEntry)->sw, (*drawEntry)->sh);

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
	int arrayBase = RF_MAX_SPRITETABLEENTRIES * VL_GetActiveBuffer();
	for (int i = arrayBase; i < arrayBase + rf_freeSpriteEraserIndex[VL_GetActiveBuffer()]; ++i)
	{
		rf_spriteErasers[i].pxX -= RF_UnitToTile(rf_scrollXUnit) * 16;
		rf_spriteErasers[i].pxY -= RF_UnitToTile(rf_scrollYUnit) * 16;
		int x = MAX(rf_spriteErasers[i].pxX - 8, 0);
		int y = MAX(rf_spriteErasers[i].pxY - 8, 0);
		int x2 = MIN(rf_spriteErasers[i].pxX + rf_spriteErasers[i].pxW + 8, RF_BUFFER_WIDTH_PIXELS - 1);
		int y2 = MIN(rf_spriteErasers[i].pxY + rf_spriteErasers[i].pxH + 8, RF_BUFFER_HEIGHT_PIXELS - 1);

		// Only process if on-screen.
		if (x2 < x || y2 < y)
			continue;

		VL_SurfaceToScreen(rf_tileBuffer, x, y, x, y, x2 - x + 1, y2 - y);

		// Mark the affected tiles dirty with '2', to force sprites to
		// redraw.
		int tileX1 = RF_PixelToTile(x);
		int tileY1 = RF_PixelToTile(y);
		int tileX2 = RF_PixelToTile(x2);
		int tileY2 = RF_PixelToTile(y2);
		for (int ty = tileY1; ty <= tileY2; ++ty)
		{
			for (int tx = tileX1; tx <= tileX2; ++tx)
			{
				RFL_MarkBlockDirty(tx, ty, 2, VL_GetActiveBuffer());
			}
		}
	}
#endif
	//Reset
	rf_freeSpriteEraserIndex[VL_GetActiveBuffer()] = 0;
}

void RF_AddSpriteDraw(RF_SpriteDrawEntry **drawEntry, int unitX, int unitY, int chunk, bool allWhite, int zLayer)
{
	bool insertNeeded = true;
	if (chunk == 0 || chunk == -1)
	{
		RF_RemoveSpriteDraw(drawEntry);
		return;
	}

	RF_SpriteDrawEntry *sde = *drawEntry;

	if (sde)
	{
		int old_sprite_number = sde->chunk - ca_gfxInfoE.offSprites;
		VH_SpriteTableEntry *oldSprite = VH_GetSpriteTableEntry(old_sprite_number);
		RF_PlaceEraser(sde->x + RF_UnitToPixel(oldSprite->originX), sde->y + RF_UnitToPixel(oldSprite->originY), sde->sw, sde->sh);

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

	int sprite_number = chunk - ca_gfxInfoE.offSprites;

	if (!ca_graphChunks[chunk])
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Trying to place an uncached sprite (chunk = %d, sprite = %d)\n", chunk, sprite_number);
		CA_CacheGrChunk(chunk);
	}
	void *sprite_data = ca_graphChunks[chunk];
	if (!sprite_data)
		Quit("RF_AddSpriteDraw: Placed an uncached sprite");

	sde->chunk = chunk;
	sde->zLayer = zLayer;
	sde->x = RF_UnitToPixel(unitX);
	sde->y = RF_UnitToPixel(unitY);
	sde->sw = VH_GetSpriteTableEntry(sprite_number)->width * 8;
	sde->sh = VH_GetSpriteTableEntry(sprite_number)->height;
	sde->maskOnly = allWhite;
	sde->updateCount = VL_GetNumBuffers();

	*drawEntry = sde;
}

RF_SpriteDrawEntry *RF_ConvertSpriteArray16BitOffsetToPtr(uint16_t drawEntryoffset)
{
	return drawEntryoffset ? &rf_spriteTable[(drawEntryoffset - ck_currentEpisode->spriteArrayOffset) / 32] : NULL;
}

uint16_t RF_ConvertSpriteArrayPtrTo16BitOffset(RF_SpriteDrawEntry *drawEntry)
{
	return drawEntry ? ((drawEntry - rf_spriteTable) * 32 + ck_currentEpisode->spriteArrayOffset) : 0;
}

void RF_RemoveSpriteDrawUsing16BitOffset(int16_t *drawEntryOffset)
{
	if (drawEntryOffset)
	{
		RF_SpriteDrawEntry *drawEntry = RF_ConvertSpriteArray16BitOffsetToPtr(*drawEntryOffset);
		RF_RemoveSpriteDraw(&drawEntry);
		*drawEntryOffset = 0;
	}
}

void RF_AddSpriteDrawUsing16BitOffset(int16_t *drawEntryOffset, int unitX, int unitY, int chunk, bool allWhite, int zLayer)
{
	RF_SpriteDrawEntry *drawEntry = RF_ConvertSpriteArray16BitOffsetToPtr(*drawEntryOffset);
	RF_AddSpriteDraw(&drawEntry, unitX, unitY, chunk, allWhite, zLayer);
	*drawEntryOffset = RF_ConvertSpriteArrayPtrTo16BitOffset(drawEntry);
}

void RFL_DrawSpriteList()
{
	for (int zLayer = 0; zLayer < RF_NUM_SPRITE_Z_LAYERS; ++zLayer)
	{
		// All but the final z layer (3) are below fore-foreground tiles.
		if (zLayer == 3)
			RFL_RenderForeTiles();

		for (RF_SpriteDrawEntry *sde = rf_firstSpriteTableEntry[zLayer]; sde; sde = sde->next)
		{
			int pixelX = sde->x - RF_UnitToTile(rf_scrollXUnit) * 16;
			int pixelY = sde->y - RF_UnitToTile(rf_scrollYUnit) * 16;
			int tileX1 = MAX(RF_PixelToTile(pixelX), 0);
			int tileY1 = MAX(RF_PixelToTile(pixelY), 0);
			int tileX2 = MIN(RF_PixelToTile(pixelX + sde->sw), RF_BUFFER_WIDTH_TILES - 1);
			int tileY2 = MIN(RF_PixelToTile(pixelY + sde->sh), RF_BUFFER_HEIGHT_TILES - 1);

			// Check the sprite is in-bounds.
			if (tileX2 < tileX1 || tileY2 < tileY1)
				continue;
#ifdef ALWAYS_REDRAW
			sde->updateCount = VL_GetNumBuffers();
#endif

			if (!sde->updateCount)
			{
				for (int y = tileY1; y <= tileY2; ++y)
				{
					for (int x = tileX1; x <= tileX2; ++x)
					{
						if (RFL_IsBlockDirty(x, y, -1))
						{
							sde->updateCount = 1;
							goto drawSprite;
						}
					}
				}
			}

		drawSprite:
			if (sde->updateCount)
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
						RFL_MarkBlockDirty(x, y, 3, VL_GetActiveBuffer());
					}
				}
				sde->updateCount--;
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
			if (RFL_IsBlockDirty(x, y, -1) == 1)
				VL_SurfaceToScreen(rf_tileBuffer, x * 16, y * 16, x * 16, y * 16, 16, 16);
		}
	}
#endif
}

void RF_Refresh()
{
	RFL_AnimateTiles();

#ifdef ALWAYS_REDRAW
	VL_SurfaceToScreen(rf_tileBuffer, 0, 0, 0, 0, RF_BUFFER_WIDTH_PIXELS, RF_BUFFER_HEIGHT_PIXELS);
#endif

	//TODO: Work out how to do scrolling before using this
	RFL_UpdateTiles();
	RFL_ProcessSpriteErasers();

	RFL_DrawSpriteList();

	// No blocks should be dirty on this page after the frame has been rendered.
	for (int y = 0; y < RF_BUFFER_HEIGHT_TILES; ++y)
		for (int x = 0; x < RF_BUFFER_WIDTH_TILES; ++x)
			RFL_MarkBlockDirty(x, y, 0, VL_GetActiveBuffer());

	if (rf_drawFunc)
		rf_drawFunc();

	VL_SwapOnNextPresent();
	VL_Present();

	RFL_CalcTics();
}
