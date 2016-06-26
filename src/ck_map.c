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

#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_rf.h"
#include "id_sd.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_act.h"

#include <string.h>
#include <stdio.h>

// =========================================================================


void CK_DemoSignSpawn()
{

	ck_scoreBoxObj->type = CT_Friendly;
	ck_scoreBoxObj->zLayer = 3;
	ck_scoreBoxObj->active = OBJ_ALWAYS_ACTIVE;
	ck_scoreBoxObj->clipped = CLIP_not;

	// Set all user vars to -1 to force scorebox redraw
	ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;

	if (ck_inHighScores)
	{
		// Don't display anything in the high scores
		ck_scoreBoxObj->currentAction = CK_GetActionByName("CK_ACT_NULL");
	}
	else if (IN_DemoGetMode())
	{
		// If this is a demo, display the DEMO banner
		CK_SetAction(ck_scoreBoxObj, CK_GetActionByName("CK_ACT_DemoSign"));
		CA_CacheGrChunk(SPR_DEMOSIGN);
	}
	else
	{
		// If a normal game, display the scorebox
		CK_SetAction(ck_scoreBoxObj, CK_GetActionByName("CK_ACT_ScoreBox"));
	}
}

void CK_DemoSign( CK_object *demo)
{
	if (	demo->posX == rf_scrollXUnit && demo->posY == rf_scrollYUnit )
		return;
	demo->posX = rf_scrollXUnit;
	demo->posY = rf_scrollYUnit;

	//place demo sprite in center top
	RF_AddSpriteDraw( &(demo->sde), demo->posX+0x0A00 - 0x200, demo->posY+0x80,SPR_DEMOSIGN,false,3);
}

/*
 * ScoreBox update
 * Scorebox works by drawing tile8s over the sprite that has been cached in
 * memory.
 *
 * Because vanilla keen cached four shifts of the scorebox, this redraw
 * procedure would have to be repeated for each shift.
 *
 * This is not the case for omnispeak, which just caches one copy of
 * each sprite.
 *
 */

/*
 * Draws a Tile8 to an unmasked planar graphic
 */
void CK_ScoreBoxDrawTile8(int tilenum, uint8_t *dest, int destWidth, int planeSize)
{
	uint8_t *src = (uint8_t *)ca_graphChunks[ca_gfxInfoE.offTiles8] + 32 * tilenum;

	// Copy the tile to the target bitmap
	for (int plane = 0; plane < 4; plane++, dest+=planeSize)
	{
		uint8_t *d = dest;
		for (int row = 0; row < 8; row++)
		{
			*d = *(src++);
			d += destWidth;
		}
	}
}

void CK_UpdateScoreBox(CK_object *scorebox)
{

	bool updated = false;

	// Don't draw anything for the high score level
	if (ck_inHighScores)
		return;

	// Show the demo sign for the demo mode
	if (IN_DemoGetMode())
	{
		CK_DemoSign(scorebox);
		return;
	}

	if (!ck_scoreBoxEnabled)
		return;


	// NOTE: Score is stored in user1 and user2 in original keen
	// Can just use user1 here, but be careful about loading/saving games!


	// Draw the score if it's changed
	if (scorebox->user1 != ck_gameState.keenScore)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and four rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 4 + 1;

		sprintf(buf, "%d", ck_gameState.keenScore);
		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 9; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);

		}

		scorebox->user1 = ck_gameState.keenScore;
		updated = true;

	}

	// Draw the number of shots if it's changed
	if (scorebox->user3 != ck_gameState.numShots)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and 12 rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 20 + 8;

		if (ck_gameState.numShots >= 99)
			sprintf(buf, "99");
		else
			sprintf(buf, "%d", ck_gameState.numShots);

		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 2; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);
		}

		scorebox->user3 = ck_gameState.numShots;
		updated = true;
	}

	// Draw the number of lives if it's changed
	if (scorebox->user4 != ck_gameState.numLives)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and 12 rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 20 + 3;

		if (ck_gameState.numLives >= 99)
			sprintf(buf, "99");
		else
			sprintf(buf, "%d", ck_gameState.numLives);

		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 2; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);
		}

		scorebox->user4 = ck_gameState.numLives;
		updated = true;
	}

	// Now draw the scorebox to the screen
	if (scorebox->posX != rf_scrollXUnit || scorebox->posY != rf_scrollYUnit)
	{
		scorebox->posX = rf_scrollXUnit;
		scorebox->posY = rf_scrollYUnit;
		updated = true;
	}

	if (updated)
		RF_AddSpriteDraw(&scorebox->sde, scorebox->posX + 0x40, scorebox->posY + 0x40, SPR_SCOREBOX, false, 3);

}
