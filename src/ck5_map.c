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
#include "ck5_ep.h"

#include <string.h>
#include <stdio.h>

void CK5_MapKeenWalk(CK_object * obj);

// =========================================================================


/*
 * MapKeen Thinks
 * user1 stores compass direction
 * user2 stores animation frame counter
 * user3 stores some sort of velocity
 */

static int ck_mapKeenFrames[] ={ 0xF7, 0x106, 0xF4, 0xFD, 0xFA, 0x100, 0xF1, 0x103 };
static int word_417BA[] ={ 2, 3, 1, 3, 4, 6, 0, 2};

void CK_SpawnMapKeen(int tileX, int tileY)
{

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
	ck_keenObj->gfxChunk = 0xF4;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK5_ACT_MapKeenStart"));
}

void CK5_MapKeenTeleSpawn(int tileX, int tileY)
{

	ck_keenObj->type = 2;
	ck_keenObj->posX = (tileX << 8);
	ck_keenObj->posY = (tileY << 8);
	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->zLayer = 1;
	ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;
	ck_keenObj->user1 = 6;
	ck_keenObj->user2 = 3;
	ck_keenObj->user3 = 0;
	ck_keenObj->gfxChunk = 244;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK5_ACT_MapKeenStart"));
}


//look for level entrry

void CK5_ScanForLevelEntry(CK_object * obj)
{

	int tx, ty;
	int tileY_0 = obj->clipRects.tileY1;

	for (ty = obj->clipRects.tileY1; ty <= obj->clipRects.tileY2; ty++)
	{
		for (tx = obj->clipRects.tileX1; tx <= obj->clipRects.tileX2; tx++)
		{
			int infotile =CA_TileAtPos(tx, ty, 2);
			if (infotile > 0xC000 && infotile < 0xC012)
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

void CK5_MapKeenStill(CK_object * obj)
{

	if (ck_inputFrame.dir != IN_dir_None)
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_MapKeenWalk0");
		obj->user2 = 0;
		CK5_MapKeenWalk(obj);
	}

	if (ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed || ck_keenState.shootIsPressed)
	{
		CK5_ScanForLevelEntry(obj);
	}
}

void CK5_MapKeenWalk(CK_object * obj)
{

	if (obj->user3 == 0)
	{
		obj->xDirection = ck_inputFrame.xDirection;
		obj->yDirection = ck_inputFrame.yDirection;
		if (ck_keenState.pogoIsPressed || ck_keenState.jumpIsPressed || ck_keenState.shootIsPressed)
			CK5_ScanForLevelEntry(obj);

		// Go back to standing if no arrows pressed
		if (ck_inputFrame.dir == IN_dir_None)
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_MapKeenStart");
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

// FIXME: Keen teleports, but animation doesn't work

void CK5_AnimateMapTeleporter(int tileX, int tileY)
{

	int unitX, unitY;
	uint16_t timer, animTile, ticsx2;

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
		ck_keenObj->gfxChunk = ((SD_GetTimeCount() >> 3) % 3) + 0xF8;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);

		animTile = ((SD_GetTimeCount() >> 2)&1) + 0xA7F; // lighting bolt tile

		RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	}

	// Done Teleporting; Move keen to destination
	animTile = 0x427;
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
		ck_keenObj->gfxChunk = (SD_GetTimeCount() >> 3) % 3 + 0xFB;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);
		animTile = ((SD_GetTimeCount() >> 2)&1) + 0xA7F; // animate return lighting bolt
		RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	}

	animTile = 0;
	RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	ck_nextX = ck_nextY = 0;
	CK_PhysUpdateNormalObj(ck_keenObj);
}


//this is a move proc.. closes doors on keen

#if 0

MapKeenElevator(CK_object * obj)
{

	int var2, s, var4;

	ck_nextY = obj->yDirection * 64 * CK_GetTicksPerFrame();
	if (obj->posX != obj->user2)
	{
		ck_nextX = obj->xDirection * 12 * CK_GetTicksPerFrame();
		if (obj->xDirection == IN_motion_Right && ck_nextX + obj->posX > obj->user2 ||
				obj->xDirection == IN_motion_Left && ck_nextX + obj->posX < obj->user2)
		{
			ck_nextX = obj->user2 - obj->posX;
		}
	}
	// draw the doors closing
	for (var2 = 0; var2 <= 5; var2++)
	{
		for (var4 = 0; var4 < 2; var4++)
		{
			for (s =0; s < 2; s++)
			{
				var10[var4 * 2 + s] = CA_TileAtPos(var2 * 2 + s, var4, 1);
			}
			RF_ReplaceTiles(&var10, 1, var6, var8 - 2, 2, 2);
			RF_Refresh();
			VL_WaitVBL(8);

		}
	}

	for (var4 = 0; var4 < 32; var4++)
	{
		obj->posY += 8;
		obj->frame = ((var4 / 4) % 3) + 0xFB;
		// RF_PlaceSprite(&obj->int35, obj->posX,obj->posY,obj->frame,0,obj->zLayer);
	}

	obj->clipping = 1;
	return;
}
#endif

void CK5_MapKeenElevator(CK_object *keen)
{
	int tileX, tileY;

	// Move keen in the Y direction
	ck_nextY = keen->yDirection * 64 * SD_GetSpriteSync();

	if (keen->posX != keen->user2)
	{
		ck_nextX = keen->xDirection * 12 * SD_GetSpriteSync();
		if (keen->xDirection == IN_motion_Right && ck_nextX + keen->posX > keen->user2 ||
				keen->xDirection == IN_motion_Left && ck_nextX + keen->posX < keen->user2)
		{
			ck_nextX = keen->user2 - keen->posX;
		}
	}

	//1D776
	// Update hitbox
	keen->clipRects.unitX1 = keen->posX + ck_nextX;
	keen->clipRects.unitX2 = keen->clipRects.unitX1 + 0xFF;

	keen->clipRects.unitY1 = keen->posY + ck_nextY;
	keen->clipRects.unitY2 = keen->clipRects.unitY1 + 0xFF;

	// If keen has not yet hit the Y destination, keep moving
	if (keen->yDirection == IN_motion_Down)
	{
		if ((uint16_t) (keen->posY + ck_nextY)<(uint16_t) keen->user1)
			return;
	}
	else
	{
		if ((uint16_t) (keen->posY + ck_nextY)>(uint16_t) keen->user1)
			return;
	}

	// 1D7C5
	// Arrived at destination; turn travelling keen back into normal map keen
	ck_nextX = ck_nextY = 0;
	keen->posX = keen->user2;
	keen->posY = keen->user1;
	keen->zLayer = 1;
	keen->user1 = 4;  // Keen faces south
	keen->user2 = 3;
	keen->user3 = 0;
	ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;
	keen->currentAction = CK_GetActionByName("CK5_ACT_MapKeenStart");
	keen->gfxChunk = 0xFD;
	keen->clipped = CLIP_normal;
	tileX = keen->posX >> 8;
	tileY = keen->posY >> 8;
	CK_MapCamera(keen);
	// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
	VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);

	//Draw the scorebox
	CK_UpdateScoreBox(ck_scoreBoxObj);

	RF_Refresh();
	VL_Present();
	RF_Refresh();

	keen->posY -= 0x100;
	RF_AddSpriteDraw(&keen->sde, keen->posX, keen->posY, keen->gfxChunk, false, keen->zLayer);
	SD_PlaySound(SOUND_UNKNOWN63);

	// Animate the elevator operation
	uint16_t tile_array[4];
	for (int frame = 0; frame <= 5; frame++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				tile_array[y * 2 + x] = CA_TileAtPos(frame * 2 + x, y, 1);
			}
		}
		RF_ReplaceTiles(tile_array, 1, tileX, tileY-2, 2, 2);
		RF_Refresh();
		//VL_DelayTics(2);
		VL_DelayTics(8); // Simulate 8 calls to VL_WaitVBL();
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	// Draw keen walking out of elevator
	for (int frame = 0; frame < 32; frame++)
	{
		keen->posY += 8;
		keen->gfxChunk = 0xFB + (frame/4)%3;
		RF_AddSpriteDraw(&keen->sde, keen->posX, keen->posY, keen->gfxChunk, false, keen->zLayer);
		RF_Refresh();
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();

	}

	keen->clipped = CLIP_normal;
}

/*
 * Keen enters an elevator
 */
void CK5_AnimateMapElevator(int tileX, int tileY, int dir)
{
	int unitX, unitY;
	int timer;
	int ticsx2;

	unitX = tileX << 8;
	unitY = tileY << 8;

	for (timer = 0; timer < 130; )
	{
		// NOTE: this function in omnispeak will calculate the new tile clipRects
		// Here, Vanilla keen calls a function which does not calculate new clipRects
		// But this is the only time that function is ever called,
		// so we can just call CK_ResetClipRects in this spot

		CK_ResetClipRects(ck_keenObj);
		CK_MapCamera(ck_keenObj);

		//Draw the scorebox
		CK_UpdateScoreBox(ck_scoreBoxObj);

		// Draw screen and delay 1/35th of a second
		RF_Refresh();
		// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
		VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();

		ticsx2 = SD_GetSpriteSync() * 2;
		timer += SD_GetSpriteSync();


		// If keen arrives at the target, then he's done walking
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
		// TODO: Is that done by the original code?
		ck_keenObj->gfxChunk = ((SD_GetTimeCount() >> 3) % 3) + 0xF8;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);
	}

	SD_PlaySound(SOUND_UNKNOWN63);


	// Draw the elevator operation
	uint16_t tile_array[4];
	for (int frame = 5; frame >= 0; frame--)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				tile_array[y * 2 + x] = CA_TileAtPos(frame * 2 + x, y, 1);
			}
		}
		RF_ReplaceTiles(tile_array, 1, tileX+dir, tileY-1, 2, 2);
		RF_Refresh();
		//VL_DelayTics(2);
		VL_DelayTics(8); // Simulate 8 calls to VL_WaitVBL();
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	// Keen is now behind the elevator door, so don't draw him
	RF_RemoveSpriteDraw(&ck_keenObj->sde);

	// Get the destination stored in the info plane
	// and store X in map units in user1; Y in user 2
	int dest_tile = CA_TileAtPos(tileX, tileY, 2);
	ck_keenObj->user2 = dest_tile / 256 * 256;
	ck_keenObj->user1 = (((dest_tile & 0x007F)+1) << 8);

	// Move keen based on the relative location of the target
	if ((uint16_t) ck_keenObj->user1 < (uint16_t) ck_keenObj->posY)
		ck_keenObj->yDirection = IN_motion_Up;
	else
		ck_keenObj->yDirection = IN_motion_Down;

	if ((uint16_t) ck_keenObj->user2 < (uint16_t) ck_keenObj->posX)
		ck_keenObj->xDirection = IN_motion_Left;
	else
		ck_keenObj->xDirection = IN_motion_Right;

	// Keen is going to become invisible and "fly" to the destination
	ck_keenObj->clipped = CLIP_not;
	ck_keenObj->currentAction = CK_GetActionByName("CK5_ACT_MapKeenElevator");

	// Note: This should have been added
	// ck_keenObj->user3 = 1;
}

//Thisis called from playloop

#define MISCFLAG_TELEPORT 0x14
#define MISCFLAG_LEFTELEVATOR 0x21
#define MISCFLAG_RIGHTELEVATOR 0x22

void CK_MapMiscFlagsCheck(CK_object *keen)
{

	int midTileX, midTileY;

	if (keen->user3)
		return;

	midTileX = keen->clipRects.tileXmid;
	midTileY = ((keen->clipRects.unitY2 - keen->clipRects.unitY1) / 2 + keen->clipRects.unitY1) >> 8;

	switch (TI_ForeMisc(CA_TileAtPos(midTileX, midTileY, 1)))
	{

	case MISCFLAG_TELEPORT:
		CK5_AnimateMapTeleporter(midTileX, midTileY);
		break;

	case MISCFLAG_LEFTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, 0);
		break;

	case MISCFLAG_RIGHTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, -1);
		break;
	}
}

void CK_MapFlagSpawn(int tileX, int tileY)
{

	CK_object *flag = CK_GetNewObj(false);

	flag->clipped = CLIP_not;
	flag->zLayer = 3;
	flag->type = CT_MapFlag;
	flag->active = OBJ_ACTIVE;
	flag->posX = (tileX << 8) - 0x50;
	flag->posY = (tileY << 8) - 0x1E0;
	flag->actionTimer = US_RndT() / 16;
	CK_SetAction(flag, CK_GetActionByName("CK5_ACT_MapFlag0"));
}

/*
 * Setup all of the functions in this file.
 */
void CK5_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_MapKeenStill", &CK5_MapKeenStill);
	CK_ACT_AddFunction("CK5_MapKeenWalk", &CK5_MapKeenWalk);
	CK_ACT_AddFunction("CK5_MapKeenElevator", &CK5_MapKeenElevator);
}
