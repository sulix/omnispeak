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
#include "ck4_ep.h"
#include "ck5_ep.h"

#include <string.h>
#include <stdio.h>

void CK_MapKeenWalk(CK_object * obj);

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


	// Draw the score if it's changed
	if ((scorebox->user1 != (ck_gameState.keenScore>>16)) || (scorebox->user2 != (ck_gameState.keenScore&0xFFFF)))
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

		sprintf(buf, "%d", (int)ck_gameState.keenScore);
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

		scorebox->user1 = ck_gameState.keenScore>>16;
		scorebox->user2 = ck_gameState.keenScore&0xFFFF;
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

// =========================================================================


/*
 * MapKeen Thinks
 * user1 stores compass direction
 * user2 stores animation frame counter
 * user3 stores some sort of velocity
 */

int *ck_mapKeenFrames;
static int word_417BA[] ={ 2, 3, 1, 3, 4, 6, 0, 2};

void CK_SpawnMapKeen(int tileX, int tileY)
{
  if (ck_currentEpisode->ep == EP_CK4 && ck_gameState.levelState == LS_Foot)
  {
    ck_keenObj->clipped = CLIP_not;
    ck_keenObj->type = CT_Player;
    ck_keenObj->posX = ck_gameState.mapPosX;
    ck_keenObj->posY = ck_gameState.mapPosY;
    ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
    ck_keenObj->zLayer = PRIORITIES - 1;
    ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;

    if (ck_gameState.mapPosX < 0x1400)
    {
      // Going to pyramid of forbidden at 1E, 37
      ck_keenObj->user1 = 280;
      ck_keenObj->velX = (0x1E00 - ck_keenObj->posX) / 280 + 1;
      ck_keenObj->velY = (0x3700 - ck_keenObj->posY) / 280 + 1;
    }
    else
    {
      // Return flight to 0x10, 0x2F
      ck_keenObj->user1 = 140;
      ck_keenObj->velX = (0x1000 - ck_keenObj->posX) / 140 + 1;
      ck_keenObj->velY = (0x2F00 - ck_keenObj->posY) / 140 + 1;
    }

    CK_SetAction(ck_keenObj, CK_GetActionByName("CK4_ACT_MapKeenFoot0"));
    return;
  }

	ck_keenObj->type = 2;
	if (ck_gameState.mapPosX == 0)
	{
		ck_keenObj->posX = (tileX << 8);
		ck_keenObj->posY = (tileY << 8);
	}
	else
	{
		ck_keenObj->posX = ck_gameState.mapPosX;
		ck_keenObj->posY = ck_gameState.mapPosY;
	}

	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->zLayer = 1;
	ck_keenObj->xDirection= ck_keenObj->yDirection = IN_motion_None;
	ck_keenObj->user1 = 6;
	ck_keenObj->user2 = 3;
	ck_keenObj->user3 = 0;
  ck_keenObj->gfxChunk = SPR_MAPKEEN_STAND_W;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK_ACT_MapKeenStart"));
}

//look for level entrry

void CK_ScanForLevelEntry(CK_object * obj)
{

	int tx, ty;
	int tileY_0 = obj->clipRects.tileY1;

	for (ty = obj->clipRects.tileY1; ty <= obj->clipRects.tileY2; ty++)
	{
		for (tx = obj->clipRects.tileX1; tx <= obj->clipRects.tileX2; tx++)
		{
			int infotile =CA_TileAtPos(tx, ty, 2);
			if (infotile > 0xC000 && infotile <= 0xC012)
			{
				// Vanilla keen stores the current map loaded in the cache manager
				// and the "current_map" variable stored in the gamestate
				// would have been changed here.
				ck_gameState.mapPosX = obj->posX;
				ck_gameState.mapPosY = obj->posY;
				ck_gameState.currentLevel = infotile - 0xC000;
				ck_gameState.levelState = 2;
				SD_PlaySound(SOUND_UNKNOWN12);
				return;
			}
		}
	}
}

void CK_MapKeenStill(CK_object * obj)
{

	if (ck_inputFrame.dir != IN_dir_None)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_MapKeenWalk0");
		obj->user2 = 0;
		CK_MapKeenWalk(obj);
	}

	if (ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed || ck_keenState.shootIsPressed)
	{
		CK_ScanForLevelEntry(obj);
	}
}

void CK_MapKeenWalk(CK_object * obj)
{

	if (obj->user3 == 0)
	{
		obj->xDirection = ck_inputFrame.xDirection;
		obj->yDirection = ck_inputFrame.yDirection;
		if (ck_keenState.pogoIsPressed || ck_keenState.jumpIsPressed || ck_keenState.shootIsPressed)
			CK_ScanForLevelEntry(obj);

		// Go back to standing if no arrows pressed
		if (ck_inputFrame.dir == IN_dir_None)
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_MapKeenStart");
			obj->gfxChunk = ck_mapKeenFrames[obj->user1] + 3;
			return;
		}
		else
		{
			obj->user1 = ck_inputFrame.dir;
		}
	}
	else
	{
		if ((obj->user3 -= 4) < 0)
			obj->user3 = 0;
	}

	// Advance Walking Frame Animation
	if (++obj->user2 == 4)
		obj->user2 = 0;
	obj->gfxChunk = ck_mapKeenFrames[obj->user1] + word_417BA[obj->user2];

	//walk hi lo sound
	if (obj->user2 == 1)
	{
		SD_PlaySound(SOUND_KEENWALK0);
	}
	else if (obj->user2 == 3)
	{
		SD_PlaySound(SOUND_KEENWALK1);
	}
}

// =========================================================================

// TELEPORTERS

void CK_AnimateMapTeleporter(int tileX, int tileY)
{

	int unitX, unitY;
	uint16_t timer, animTile, ticsx2;

  int boltTile = ck_currentEpisode->ep == EP_CK5 ? 0xA7F :
    (ck_currentEpisode->ep == EP_CK6 ? 0xA35 : 0);
  int doneTile = ck_currentEpisode->ep == EP_CK5 ? 0x427 :
    (ck_currentEpisode->ep == EP_CK6 ? 0xA45 : 0);
  int doneTile2 = ck_currentEpisode->ep == EP_CK5 ? 0 :
    (ck_currentEpisode->ep == EP_CK6 ? 0xA45 : 0);

	SD_PlaySound(SOUND_UNKNOWN41);

	unitX = (tileX << 8);
	unitY = (tileY << 8);

	// Teleport Out
	for (timer = 0; timer < 130; )
	{
		// NOTE: I think that the original keen game used
		// RF_Refresh() to delay this loop
		// Simulate this by adding a 1/35 second delay

		// UPDATE (Feb 19 2014): Not done anymore, but VL_Present is called.

		RF_Refresh();
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();

		ticsx2 = SD_GetSpriteSync() * 2;
		timer += SD_GetSpriteSync();
		if (ck_keenObj->posX == unitX && ck_keenObj->posY == unitY)
			break;

		// Move Keen closer to the target on every loop
		if (ck_keenObj->posY < unitY)
		{
			ck_keenObj->posY += ticsx2;
			if (ck_keenObj->posY > unitY)
				ck_keenObj->posY = unitY;
		}
		else if (ck_keenObj->posY > unitY)
		{
			ck_keenObj->posY -= ticsx2;
			if (ck_keenObj->posY < unitY)
				ck_keenObj->posY = unitY;
		}

		if (ck_keenObj->posX < unitX)
		{
			ck_keenObj->posX += ticsx2;
			if (ck_keenObj->posX > unitX)
				ck_keenObj->posX = unitX;
		}
		else if (ck_keenObj->posX > unitX)
		{
			ck_keenObj->posX -= ticsx2;
			if (ck_keenObj->posX < unitX)
				ck_keenObj->posX = unitX;
		}

		// Draw Keen walking into target
		ck_keenObj->gfxChunk = ((SD_GetTimeCount() >> 3) % 3) + SPR_MAPKEEN_WALK1_N;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);

		animTile = ((SD_GetTimeCount() >> 2)&1) + boltTile; // lighting bolt tile

		RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	}

	// Done Teleporting; Move keen to destination
	animTile = doneTile;
	RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);

	// Destination is set in Infoplane above teleporter
	animTile = CA_TileAtPos(tileX, tileY, 2);

	tileX = animTile >> 8;
	tileY = animTile & 0xFF; // 0x7F in disasm, should be 0xFF?
	ck_keenObj->posX = (tileX << 8);
	ck_keenObj->posY = (tileY << 8);
	ck_keenObj->xDirection = IN_motion_None;
	ck_keenObj->yDirection = IN_motion_Down;
	ck_keenObj->user1 = 4;
	CK_SetAction(ck_keenObj, ck_keenObj->currentAction);
	CK_CentreCamera(ck_keenObj);
	// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
	VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);

	// Set objects to be active if they're inside the screen
	for (CK_object *obj = ck_keenObj->next; obj != NULL; obj = obj->next)
	{

		if (obj->active || obj->type != 8 ||
				obj->clipRects.tileX2 < (rf_scrollXUnit >> 8) - 1 || obj->clipRects.tileX1 > (rf_scrollXUnit >> 8) + (320 >> 4) + 1 || obj->clipRects.tileY2 < (rf_scrollYUnit >> 8) - 1 || obj->clipRects.tileY1 > (rf_scrollYUnit >> 8) + (208 >> 4) + 1)
			continue;

		obj->visible = 1;
		obj->active = OBJ_ACTIVE;
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}

	CK_UpdateScoreBox(ck_scoreBoxObj);
	RF_Refresh();
	VL_Present();
	RF_Refresh();
	SD_PlaySound(SOUND_UNKNOWN41);

	for (timer = 0; timer < 90; )
	{

		//NOTE: Same delay tactic used here too
		//UPDATE (Feb 19 2014): Again not
		RF_Refresh();
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();

		timer += SD_GetSpriteSync();
		ck_keenObj->posY += SD_GetSpriteSync() * 3;
		ck_keenObj->gfxChunk = (SD_GetTimeCount() >> 3) % 3 + SPR_MAPKEEN_WALK1_S;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);
		animTile = ((SD_GetTimeCount() >> 2)&1) + boltTile; // animate return lighting bolt
		RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	}

	animTile = doneTile2;
	RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	ck_nextX = ck_nextY = 0;
	CK_PhysUpdateNormalObj(ck_keenObj);
}


// =========================================================================
// Map Flags

typedef struct
{
  uint16_t x, y;

} CK_FlagPoint;

static CK_FlagPoint ck_flagPoints[30];

void CK_MapFlagSpawn(int tileX, int tileY)
{

	CK_object *flag = CK_GetNewObj(false);

	flag->clipped = CLIP_not;
	flag->zLayer = 3;
  flag->type = CT_CLASS(MapFlag);
	flag->active = OBJ_ACTIVE;
  flag->posX = (tileX << 8) + (ck_currentEpisode->ep == EP_CK5 ? -0x50 : 0x60);
	flag->posY = (tileY << 8) - 0x1E0;
	flag->actionTimer = US_RndT() / 16;

	CK_SetAction(flag, CK_GetActionByName("CK_ACT_MapFlag0"));
}

void CK_FlippingFlagSpawn(int tileX, int tileY)
{
  int32_t dx, dy;

  CK_object *obj = CK_GetNewObj(false);
  obj->clipped = CLIP_not;
  obj->zLayer = PRIORITIES - 1;
  obj->type = CT_CLASS(MapFlag);
  obj->posX = ck_gameState.mapPosX - 0x100;
  obj->posY = ck_gameState.mapPosY - 0x100;

  // Destination coords
  obj->user1 = (tileX << G_T_SHIFT) + 0x60;
  obj->user2 = (tileY << G_T_SHIFT) - 0x260;

  dx = (int32_t)obj->user1 - (int32_t)obj->posX;
  dy = (int32_t)obj->user2 - (int32_t)obj->posY;

  // Make a table of coordinates for the flag's path
  for (int i = 0; i < 30; i++)
  {
     // Draw points in a straight line between keen and the holster
     ck_flagPoints[i].x = obj->posX + dx * (i < 24 ? i : 24) / 24;
     ck_flagPoints[i].y = obj->posY + dy * i / 30;

     // Offset th eY points to mimic a parabolic trajectory
     if (i < 10)
       ck_flagPoints[i].y -= i * 0x30; // going up
     else if (i < 15)
       ck_flagPoints[i].y -= i * 16 + 0x140;
     else if (i < 20)
       ck_flagPoints[i].y -= (20 - i) * 16 + 0x1E0;
     else
       ck_flagPoints[i].y -= (29 - i) * 0x30;
  }

  CK_SetAction(obj, CK_GetActionByName("CK_ACT_MapFlagFlips0"));

}

void CK_MapFlagThrown(CK_object *obj)
{
  // Might this be a source of non-determinism?
  // (if screen unfades at different rates based on diff architecture)
  if (!vl_screenFaded)
  {
    SD_StopSound();
    SD_PlaySound(SOUND_FLAGFLIP);
    obj->currentAction = obj->currentAction->next;
  }

}

void CK_MapFlagFall(CK_object *obj)
{
  obj->user3 += SD_GetSpriteSync();

  if (obj->user3 > 50)
    obj->user3 = 50;

  obj->posX = ck_flagPoints[obj->user3/2].x;
  obj->posY = ck_flagPoints[obj->user3/2].y;

  obj->visible = true;
  if (!obj->user1)
    SD_PlaySound(SOUND_FLAGFLIP);
}

void CK_MapFlagLand(CK_object *obj)
{
  // Plop the flag in its holster
  obj->posX = obj->user1;
  obj->posY = obj->user2 + 0x80;
  obj->zLayer = PRIORITIES - 1;

  SD_PlaySound(SOUND_FLAGLAND);
}

/*
 * Setup all of the functions in this file.
 */
void CK_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK_MapKeenStill", &CK_MapKeenStill);
	CK_ACT_AddFunction("CK_MapKeenWalk", &CK_MapKeenWalk);
  CK_ACT_AddFunction("CK_MapFlagThrown", &CK_MapFlagThrown);
  CK_ACT_AddFunction("CK_MapFlagFall", &CK_MapFlagFall);
  CK_ACT_AddFunction("CK_MapFlagLand", &CK_MapFlagLand);
}
