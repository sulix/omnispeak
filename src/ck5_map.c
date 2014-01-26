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
#include "id_rf.h"
#include "id_sd.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_act.h"
#include "ck5_ep.h"
#include <stdio.h>

#if 0

/*
int bouncearray [16][4] or [4][16] or [8][8]
 */

// =========================================================================
//something to do with keen spawning and the demo sign

DemoSignSpawn(void)
{

	CK_object* obj;

	obj->type = 1;
	obj->zLayer = 3;
	obj->active = 2;
	obj->clipping = 0;
	obj->user2 = obj->user1 = obj->user3 = obj->user = -1;

	if (InHighScores)
	{
		obj->action = 0x788;
		return;
	}

	if (DemoMode)
	{
		CK_SetAction(obj, ACTION_DEMOSIGN);
		CACHEGR(0x6B);
		return;
	}

	CK_SetAction(obj, 0x133E);

	return;
}

void DemoSign( CK_object *demo)
{

	if (	demo->posX == ScrollX_MU && demo->posY == ScrollY_MU ) return;
	demo->posX = ScrollX_MU;
	demo->posY = ScrollY_MU;

	//place demo sprite in center top
	// RF_PlaceSprite( &(demo->int35), demo->posX+0x0A00â€“0x0200, demo->posY+0x80,0x81,0,3);

	return;
}

//has something to do with MapKeen Spawn
#endif

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
#if 0
	if (word_47258 == 0)
	{
#endif
		ck_keenObj->posX = (tileX << 8);
		ck_keenObj->posY = (tileY << 8);
#if 0
	}
	else
	{
		ck_keenObj->posX = word_47258;
		ck_keenObj->posY = word_4725A;
	}
#endif

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
	ck_keenObj->active = OBJ_ACTIVE;
	ck_keenObj->zLayer = 1;
	ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;
	ck_keenObj->user1 = 6;
	ck_keenObj->user2 = 3;
	ck_keenObj->user3;
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
				ck_nextMapNumber = infotile - 0xC000;
				ck_gameState.levelState = 2;
				SD_PlaySound(12);
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
	int var8;
	uint16_t animTile;

	SD_PlaySound(0x29);

	unitX = (tileX << 8);
	unitY = (tileY << 8);

	// Teleport Out
	for (var8 = 0; var8 < 130; )
	{
		int spritesyncx2;

		// NOTE: I think that the original keen game used
		// RF_Refresh() to delay this loop
		// Simulate this by adding a 1/35 second delay 

		RF_Refresh();
		VL_DelayTics(2);
		CK_SetTicsPerFrame();
		VL_Present();

		spritesyncx2 = CK_GetTicksPerFrame() * 2;
		var8 += CK_GetTicksPerFrame();
		if (ck_keenObj->posX == unitX && ck_keenObj->posY == unitY)
			break;

		// Move Keen closer to the target on every loop
		if (ck_keenObj->posY < unitY)
		{
			ck_keenObj->posY += spritesyncx2;
			if (ck_keenObj->posY > unitY)
				ck_keenObj->posY = unitY;
		}
		else if (ck_keenObj->posY > unitY)
		{
			ck_keenObj->posY -= spritesyncx2;
			if (ck_keenObj->posY < unitY)
				ck_keenObj->posY = unitY;
		}

		if (ck_keenObj->posX < unitX)
		{
			ck_keenObj->posX += spritesyncx2;
			if (ck_keenObj->posX > unitX)
				ck_keenObj->posX = unitX;
		}
		else if (ck_keenObj->posX > unitX)
		{
			ck_keenObj->posX -= spritesyncx2;
			if (ck_keenObj->posX < unitX)
				ck_keenObj->posX = unitX;
		}

		ck_keenObj->gfxChunk = ((CK_GetNumTotalTics() >> 3) % 3) + 0xF8;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);

		animTile = ((CK_GetNumTotalTics() >> 2)&1) + 0xA7F; // lighting bolt tile

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

	// Set objects to be active if they're inside the screen
	for (CK_object *obj = ck_keenObj->next; obj != NULL; obj = obj->next)
	{

		if (obj->active || obj->type != 8 ||
				obj->clipRects.tileX2 < (rf_scrollXUnit >> 8) - 1 || obj->clipRects.tileX1 > (rf_scrollXUnit >> 8) + (320 >> 8) + 1 || obj->clipRects.tileY2 < (rf_scrollYUnit >> 8) - 1 || obj->clipRects.tileY1 > (rf_scrollYUnit >> 8) + (200 >> 8) + 1)
			continue;

		obj->visible = 1;
		obj->active = OBJ_ACTIVE;
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}

	// TODO: Redraw Scorebox here 

	RF_Refresh();
	RF_Refresh();
	SD_PlaySound(0x29);

	for (var8 = 0; var8 < 90; )
	{

		//NOTE: Same delay tactic used here too
		RF_Refresh();
		VL_DelayTics(2);
		CK_SetTicsPerFrame();
		VL_Present();

		var8 += CK_GetTicksPerFrame();
		ck_keenObj->posY += CK_GetTicksPerFrame() * 3;
		ck_keenObj->gfxChunk = (CK_GetNumTotalTics() >> 3) % 3 + 0xFB;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);
		animTile = ((CK_GetNumTotalTics() >> 2)&1) + 0xA7F; // animate return lighting bolt
		RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	}

	animTile = 0;
	RF_ReplaceTiles(&animTile, 1, tileX, tileY, 1, 1);
	ck_keenObj->nextX = ck_keenObj->nextY = 0;
	CK_PhysUpdateNormalObj(ck_keenObj);
}


//this is a move proc.. closes doors on keen

#if 0

MapKeenElevator(CK_object * obj)
{

	int var2, s, var4;

	obj->nextY = obj->yDirection * 64 * CK_GetTicksPerFrame();
	if (obj->posX != obj->user2)
	{
		obj->nextX = obj->xDirection * 12 * CK_GetTicksPerFrame();
		if (obj->xDirection == IN_motion_Right && obj->nextX + obj->posX > obj->user2 ||
				obj->xDirection == IN_motion_Left && obj->nextX + obj->posX < obj->user2)
		{
			obj->nextX = obj->user2 - obj->posX;
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



//sub_1D959
void AnimateMapElevator(int tileX, int tileY, int dir);

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

		/*
	case MISCFLAG_LEFTELEVATOR:
		AnimateMapElevator(midTileX, midTileY, 0);
		break;

	case MISCFLAG_RIGHTELEVATOR:
		AnimateMapElevator(midTileX, midTileY, -1);
		break;
		 */
	}
}

#if 0

MapFlagSpawn(int tileX, int tileY)
{

	GetNewObj(0);

	GameData.new_object->clipping = 0;
	GameData.new_object->zLayer = 3;
	GameData.new_object->type = 8;
	GameData.new_object->active = 1;
	GameData.new_object->posX = (tileX << 8) - 80;
	GameData.new_object->posY = (tileY << 8) - 480;
	GameData.new_object->int13 = Rand() / 16;
	CK_SetAction(GameData.new_object, ACTION_MAPFLAG0);
}

/*
 * Gem Door Opening stuff
 *
 */
void DoorOpen(CK_object * obj)
{
	int tilearray[0x30], i;
	int far *t;

	t = CA_TileAtPos_FP(obj->posX, obj->posY, 1); // note pos is in tiles for door
	for (i = 0; obj->user1 + 2 > i; i++, t+= map_width_T)
		tilearray[i] = *t + 1;

	RF_ReplaceTiles(tilearray, 1, obj->posX, obj->posY, 1, obj->user1 + 2);
}

void DoorPause (objtype * obj)
{
	// close the door, remove the gem
	if (++obj->user2 > 5);
	obj->action = &a_doorclose0;
}

void DoorClose(CK_object * obj)
{
	int tilearray[0x30], i;
	int far *t;

	t = CA_TileAtPos_FP(obj->posX, obj->posY, 1); // note pos is in tiles for door
	for (i = 0; obj->user1 + 2 > i; i++, t+= map_width_T)
		tilearray[i] = *t - 1;

	RF_ReplaceTiles(tilearray, 1, obj->posX, obj->posY, 1, obj->user1 + 2);
	if (obj->action == &a_doorclose2)
		RF_ReplaceTiles((int far *) &obj->user3, 1,
										(unsigned) obj->user & 0xFF, (unsigned) obj->user >> 8, 1, 1);
}

SecurityDoorOpen(CK_object * obj)
{

	int tilearray[0x30];
	int *d = tilearray;
	t = CA_TileAtPos(obj->posX, obj->posY, 1); // note pos is in tiles for door
	for (var2 = 0; var2 < 4; var2++, t+= map_width_T)
	{
		for (s = 0; s < 4; s++)
		{
			tilearray[d++] = *(t + s) - 4;
		}
	}

	RF_ReplaceTiles(tilearray, 1, obj->posX, obj->posY, 4, 4);
	if (++obj->user1 == 3) obj->action = NULL;
	return;
}

#endif

/*
 * Setup all of the functions in this file.
 */
void CK5_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_MapKeenStill", &CK5_MapKeenStill);
	CK_ACT_AddFunction("CK5_MapKeenWalk", &CK5_MapKeenWalk);
}
