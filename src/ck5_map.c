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
				// current_level = infotile - 0xC000;
				//GameData.level_state = 2;
				//SD_PlaySound(12);
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

#if 0

AnimateMapTeleporter(int tileX, int tileY)
{

	PlaceSound(obj->posX, obj->posY, 0x29, false);

	MU_X = (tileX << 8);
	MU_Y = (tileY << 8);

	for (var8 = 0; var8 < 130; )
	{

		RF_Refresh();
		spritesyncx2 = SpriteSync * 2;
		var8 += spritesync;
		if (ck_keenObj->posX == MU_X && ck_keenObj->ypos == MU_Y) break;

		if (ck_keenObj->ypos < MU_Y)
		{
			ck_keenObj->ypos += spritesyncx2;
			if (ck_keenObj->ypos > MU_Y) obj->posY = MU_Y;
		}
		else if (ck_keenObj->ypos > MU_Y)
		{
			ck_keenObj->ypos -= spritesyncx2;
			if (ck_keenObj->ypos < MU_Y) obj->posY = MU_Y;
		}

		if (ck_keenObj->posX < MU_X)
		{
			ck_keenObj->posX += spritesyncx2;
			if (ck_keenObj->posX > MU_X) obj->posX = MU_X;
		}
		else if (ck_keenObj->posX > MU_X)
		{
			ck_keenObj->posX -= spritesyncx2;
			if (ck_keenObj->posX < MU_X) obj->posX = MU_X;
		}

		ck_keenObj->frame = ((TimeCount >> 3) % 3) + 0xF8;
		// RF_PlaceSprite(&ck_keenObj->int35, ck_keenObj->posX, ck_keenObj->ypos,ck_keenObj->frame,0,obj->zLayer);

		var2 = (TimeCount >> 2)&1 + 0xA7F; // lighting blot tile

		RF_ReplaceTiles(&var2, FGPLANE, tileX, tileY, 1, 1);
	}

	var2 = 0x427;
	RF_ReplaceTiles(&var2, FGPLANE, tileX, tileY, 1, 1);

	var2 = CA_TileAtPos(tileX, tileY, INFOPLANE);

	tileX = var2 >> 8;
	tileY = var2 & 0x7F; // should be 0xFF?
	ck_keenObj->posX = (tileX << 8);
	ck_keenObj->ypos = (tileY << 8);
	ck_keenObj->xDirection = IN_motion_None;
	ck_keenObj->yDirection = IN_motion_Down;
	ck_keenObj->user1 = 4;
	CK_SetAction(ck_keenObj, ck_keenObj->action);
	SetActiveLimits(ck_keenObj);

	for (obj = ck_keenObj->next; obj != NULL; obj = obj->next)
	{

		if (obj->active || obj->type != 8 ||
				obj->clipRects.tile.lr.x < ScrollX0_T - 1 || obj->clipRects.tileX1 > ScrollX1_T + 1 ||
				obj->clipRects.tileY2 < ScrollY0_T - 1 || obj->clipRects.tileY1 > ScrollY1_T + 1) continue;

		obj->int2 = 1;
		obj->active = 1;
		// RF_PlaceSprite(&obj->int35, obj->posX,obj->posY,obj->frame,0,obj->zLayer);
	}

	sub_1CE03(obj_special);
	RF_Refresh();
	RF_Refresh();
	PlaceSound(obj->posX, obj->posY, 0x29, false);

	for (var8 = 0; var8 < 90; )
	{

		RF_Refresh();
		var8 += SpriteSync;
		ck_keenObj->ypos += SpriteSync * 3;
		ck_keenObj->frame = (TimeCount >> 3) % 3 + 0xFB;
		// RF_PlaceSprite(&ck_keenObj->int35,ck_keenObj->posX,ck_keenObj->ypos,ck_keenObj->frame,0,ck_keenObj->zLayer);
		var2 = (TimeCount >> 2)&1 + 0xA7F; // animate return lighting bolt
		RF_ReplaceTiles(&var2, FGPLANE, tileX, tileY, 1, 1);
	}

	var2 = 0;
	RF_ReplaceTiles(&var2, FGPLANE, tileX, tileY, 1, 1);
	KeenXVel = KeenYVel = 0;
	NoClipGroundCheck(ck_keenObj);
	return;
}


//this is a move proc.. closes doors on keen

MapKeenElevator(CK_object * obj)
{

	KeenYVel = obj->yDirection * 64 * SpriteSync;
	if (obj->posX != obj->user2)
	{
		KeenXVel = obj->xDirection * 12 * SpriteSync;
		if (obj->xDirection == IN_motion_Right && KeenXVel + obj->posX > obj->user2 ||
				obj->motion == IN_motion_Left && KeenXVel + obj->posX < obj->user2)
		{
			KeenXVel = obj->user2 - obj->posX;
		}
	}
	// draw the doors closing
	for (var2 = 0; var2 <= 5; var2++)
	{
		for (var4 = 0; var4 < 2; var4++)
		{
			for (s =0; s < 2; s++)
			{
				var10[var4 * 2 + s] = CA_TileAtPos(var2 * 2 + s, var4, FGPLANE);
			}
			RF_ReplaceTiles(&var10, FGPLANE, var6, var8 - 2, 2, 2);
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



//sub_1D959
AnimateMapElevator(int tileX, int tileY, int dir);

//Thisis called from playloop

MapMiscFlagsCheck(CK_object * keen)
{

	if (keen->user3) return;

	midX = keen->clipRects.tileXmid;
	midY = MU2TILE(AVG(keen->clipRects.unitY1 - keen->clipRects.unitY2) + keen->clipRects.unitY1);
	fgmisc = TI_FGMiscFlags[*CA_TileAtPos(midX, midY, FGTILE)];

	switch (fgmisc)
	{

	case MISCFLAG_TELEPORT:
		AnimateMapTeleporter(midX, midY);
		return;

	case MISCFLAG_LEFTELEVATOR:
		AnimateMapElevator(midX, midY, 0);
		return;

	case MISCFLAG_RIGHTELEVATOR:
		AnimateMapElevator(midX, midY, -1);
		return;
	}

	return;
}
#endif

#if 0

MapFlagSpawn(int tileX, int tileY)
{

	GetNewObj(0);

	GameData.new_object->clipping = 0;
	GameData.new_object->zLayer = 3;
	GameData.new_object->type = 8;
	GameData.new_object->active = 1;
	GameData.new_object->posX = (tileX << 8) - 80;
	GameData.new_object->ypos = (tileY << 8) - 480;
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

	t = CA_TileAtPos_FP(obj->posX, obj->posY, FGPLANE); // note pos is in tiles for door
	for (i = 0; obj->user1 + 2 > i; i++, t+= map_width_T)
		tilearray[i] = *t + 1;

	RF_ReplaceTiles(tilearray, FGPLANE, obj->posX, obj->posY, 1, obj->user1 + 2);
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

	t = CA_TileAtPos_FP(obj->posX, obj->posY, FGPLANE); // note pos is in tiles for door
	for (i = 0; obj->user1 + 2 > i; i++, t+= map_width_T)
		tilearray[i] = *t - 1;

	RF_ReplaceTiles(tilearray, FGPLANE, obj->posX, obj->posY, 1, obj->user1 + 2);
	if (obj->action == &a_doorclose2)
		RF_ReplaceTiles((int far *) &obj->user3, FGPLANE,
										(unsigned) obj->user & 0xFF, (unsigned) obj->user >> 8, 1, 1);
}

SecurityDoorOpen(CK_object * obj)
{

	int tilearray[0x30];
	int *d = tilearray;
	t = CA_TileAtPos(obj->posX, obj->posY, FGPLANE); // note pos is in tiles for door
	for (var2 = 0; var2 < 4; var2++, t+= map_width_T)
	{
		for (s = 0; s < 4; s++)
		{
			tilearray[d++] = *(t + s) - 4;
		}
	}

	RF_ReplaceTiles(tilearray, FGPLANE, obj->posX, obj->posY, 4, 4);
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
