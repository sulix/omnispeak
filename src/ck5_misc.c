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
#include "id_vl.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_act.h"
#include "ck5_ep.h"
#include <stdio.h>

CK_EpisodeDef ck5_episode ={
  Ep_CK5,
	"CK5",
	&CK5_SetupFunctions,
	&CK5_ScanInfoLayer,
  &CK5_DefineConstants
};


// Contains some keen-5 specific functions.

//StartSprites +
int16_t CK5_ItemSpriteChunks[] ={
	224, 226, 228, 230,
	210, 212, 214, 216, 218, 220,
	222, 233, 207
};

void CK5_PointItem(CK_object *obj)
{
	//	obj->timeUntillThink = 20;
	obj->visible = true;
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
}

void CK5_RedAxisPlatform(CK_object *obj)
{
	uint16_t nextPosUnit, nextPosTile;

	if (ck_nextX || ck_nextY)
	{
		return;
	}
	//TODO: Implement properly.
	ck_nextX = obj->xDirection * 12 * SD_GetSpriteSync();
	ck_nextY = obj->yDirection * 12 * SD_GetSpriteSync();

	if (obj->xDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitX2 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			ck_nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = 1;
			//TODO: Change DeltaVelocity
			//CK_PhysUpdateX(obj, 256 - nextPosUnit&255);
			ck_nextX += (256 - nextPosUnit) & 255;
		}
	}
	else if (obj->yDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitY2 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile - 2, 2) == 0x1F)
			{
				//Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = -1;
				//TODO: Change DeltaVelocity
				ck_nextY -= ( nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile + 2, 2) == 0x1F)
			{
				// Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = 1;
				//TODO: Change DeltaVelocity
				ck_nextY +=  256 - (nextPosUnit & 255);
			}
		}
	}
}

void CK5_PurpleAxisPlatform(CK_object *obj)
{
	uint16_t nextPosUnit, nextPosTile;

	if (ck_nextX || ck_nextY)
	{
		return;
	}
	//TODO: Implement properly.
	ck_nextX = obj->xDirection * 12 * SD_GetSpriteSync();
	ck_nextY = obj->yDirection * 12 * SD_GetSpriteSync();

	if (obj->xDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitX2 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			ck_nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = 1;
			//TODO: Change DeltaVelocity
			//CK_PhysUpdateX(obj, 256 - nextPosUnit&255);
			ck_nextX += (256 - nextPosUnit) & 255;
		}
	}
	else if (obj->yDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitY2 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1 + 1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile - 2, 2) == 0x1F)
			{
				//Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = -1;
				//TODO: Change DeltaVelocity
				ck_nextY -= ( nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1 + 1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile + 2, 2) == 0x1F)
			{
				// Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = 1;
				//TODO: Change DeltaVelocity
				ck_nextY +=  256 - (nextPosUnit & 255);
			}
		}
	}
}

void CK5_SpawnFallPlat(int tileX, int tileY)
{
	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_Platform;
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = tileX << 8;
	new_object->user1 = new_object->posY = tileY << 8;
	new_object->xDirection = IN_motion_None;
	new_object->yDirection = IN_motion_Down;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_FallPlat0"));
}

void CK5_FallPlatSit (CK_object *obj)
{

	if (obj == ck_keenState.platform)
	{
		ck_nextY = SD_GetSpriteSync() * 16;
		obj->velY = 0;
		if ((unsigned)(obj->posY + ck_nextY - obj->user1) >= 0x80)
			obj->currentAction = CK_GetActionByName("CK5_ACT_FallPlat1");
	}
}

void CK5_FallPlatFall (CK_object *obj)
{
	uint16_t newY, newYT;

	CK_PhysGravityHigh(obj);
	newY = obj->clipRects.unitY2 + ck_nextY;
	newYT = newY >> 8;

	// Stop falling if platform hits a block
	if ((obj->clipRects.tileY2 != newYT) && (CA_TileAtPos(obj->clipRects.tileX1, newYT, 2) == 0x1F))
	{
		ck_nextY = 0xFF - (obj->clipRects.unitY2 & 0xFF);
		if (ck_keenState.platform != obj)
			obj->currentAction = CK_GetActionByName("CK5_ACT_FallPlat2");
	}
}

void CK5_FallPlatRise (CK_object *obj)
{
	if (ck_keenState.platform == obj)
	{
		obj->velY = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_FallPlat1");
	}
	else if ((unsigned) obj->posY <= (unsigned) obj->user1)
	{
		ck_nextY = obj->user1 - obj->posY;
		obj->currentAction = CK_GetActionByName("CK5_ACT_FallPlat0");
	}
}

// MISC Keen 5 functions

// Teleporter Lightning Spawn

void CK5_SpawnLightning ()
{
	CK_object *new_object;

	// Spawn the top lightning
	new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = (ck_keenObj->clipRects.tileX1 << 8) - 0x80;
	new_object->posY = (ck_keenObj->clipRects.tileY2 << 8) - 0x500;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LightningH0"));

	// Spawn the vertical lightning that covers keen
	new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = (ck_keenObj->clipRects.tileX1 << 8);
	new_object->posY = (ck_keenObj->clipRects.tileY1 << 8) - 0x80;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LightningV0"));

	SD_PlaySound(SOUND_UNKNOWN41);
}

// Fuse Explosion Spawn

void CK5_SpawnFuseExplosion(int tileX, int tileY)
{
	CK_object *new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = (tileX - 1) << 8;
	new_object->posY = tileY << 8;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_FuseExplosion0"));
	SD_PlaySound(SOUND_UNKNOWN52);
}

// Level Ending Object Spawn

void CK5_SpawnLevelEnd(void)
{
	CK_object *new_object = CK_GetNewObj(false);
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LevelEnd"));
}

// LevelEnd Behaviour
// If in the QED, end the game
// Otherwise, do the Korath Fuse message

void CK5_LevelEnd(CK_object *obj)
{
	ck_gameState.levelState = (ca_mapOn == 12) ? 15 : 14;
}

void CK5_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
	CK_ACT_AddColFunction("CK_DeadlyCol", &CK_DeadlyCol);
	CK5_Obj1_SetupFunctions();
	CK5_Obj2_SetupFunctions();
	CK5_Obj3_SetupFunctions();
	CK5_Map_SetupFunctions();
	CK_ACT_AddFunction("CK_Fall", &CK_Fall);
	CK_ACT_AddFunction("CK_Fall2", &CK_Fall2);
	CK_ACT_AddFunction("CK_Glide", &CK_Glide);
	CK_ACT_AddFunction("CK_BasicDrawFunc1", &CK_BasicDrawFunc1);
	CK_ACT_AddFunction("CK_BasicDrawFunc2", &CK_BasicDrawFunc2);
	CK_ACT_AddFunction("CK_BasicDrawFunc4", &CK_BasicDrawFunc4);
	CK_ACT_AddFunction("CK5_PointItem", &CK5_PointItem);
	CK_ACT_AddFunction("CK5_RedAxisPlatform", &CK5_RedAxisPlatform);
	CK_ACT_AddFunction("CK5_PurpleAxisPlatform", &CK5_PurpleAxisPlatform);
	CK_ACT_AddFunction("CK5_FallPlatSit", &CK5_FallPlatSit);
	CK_ACT_AddFunction("CK5_FallPlatFall", &CK5_FallPlatFall);
	CK_ACT_AddFunction("CK5_FallPlatRise", &CK5_FallPlatRise);
	CK_ACT_AddFunction("CK5_LevelEnd", &CK5_LevelEnd);
}

void CK5_DefineConstants(void)
{
  PIC_TITLESCREEN = 88;

  SPR_DEMOSIGN = 0x6B;

  SPR_SECURITYCARD_1 = 207;
  SPR_GEM_A1 = 224;
  SPR_GEM_B1 = 226;
  SPR_GEM_C1 = 228;
  SPR_GEM_D1 = 230;
  SPR_100_PTS1 = 210;
  SPR_200_PTS1 = 212;
  SPR_500_PTS1 = 214;
  SPR_1000_PTS1 = 216;
  SPR_2000_PTS1 = 218;
  SPR_5000_PTS1 = 220;
  SPR_1UP1 = 222;
  SPR_STUNNER1 = 233;

  SPR_SCOREBOX = 235;

  SPR_MAPKEEN_WALK1_N = 248;
  SPR_MAPKEEN_STAND_N = 250;
  SPR_MAPKEEN_STAND_NE = 265;
  SPR_MAPKEEN_STAND_E = 247;
  SPR_MAPKEEN_STAND_SE = 256;
  SPR_MAPKEEN_WALK1_S = 251;
  SPR_MAPKEEN_STAND_S = 253;
  SPR_MAPKEEN_STAND_SW = 259;
  SPR_MAPKEEN_STAND_W = 244;
  SPR_MAPKEEN_STAND_NW = 262;

  TILE8_DIGIT_0 = 0x6;
  TILE8_DIGIT_EMPTY = 0x29;

}

/*
 * Spawn an enemy projectile
 * Note that the behaviour is slightly different from DOS Keen
 * DOS Keen SpawnEnemyShot returns 0 if shot is spawned, or -1 otherwise
 * omnispeak CK5_SpawnEnemyShot returns pointer if succesful, NULL otherwise
 */

CK_object *CK5_SpawnEnemyShot(int posX, int posY, CK_action *action)
{
	CK_object *new_object = CK_GetNewObj(true);

	if (!new_object)
		return NULL;

	new_object->posX = posX;
	new_object->posY = posY;
	new_object->type = CT_EnemyShot;
	new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
	CK_SetAction(new_object, action);

	if (CK_NotStuckInWall(new_object))
	{
		return new_object;
	}
	else
	{
		CK_RemoveObj(new_object);
		return NULL;
	}
}

void CK5_SpawnItem(int tileX, int tileY, int itemNumber)
{

	CK_object *obj = CK_GetNewObj(false);

	obj->clipped = CLIP_not;
	//obj->active = OBJ_ACTIVE;
	obj->zLayer = 2;
	obj->type = 5;	//OBJ_ITEM
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->yDirection = -1;
	obj->user1 = itemNumber;
	obj->gfxChunk = CK5_ItemSpriteChunks[itemNumber];
	obj->user2 = obj->gfxChunk;
	obj->user3 = obj->gfxChunk + 2;
	CK_SetAction(obj, CK_GetActionByName("CK5_act_item") );
	// TODO: Wrong place to cache?
	CA_CacheGrChunk(obj->gfxChunk);
	CA_CacheGrChunk(obj->gfxChunk + 1);
}

void CK5_SpawnAxisPlatform(int tileX, int tileY, int direction, bool purple)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ALWAYS_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;

	switch (direction)
	{
	case 0:
		obj->xDirection = 0;
		obj->yDirection = -1;
		break;
	case 1:
		obj->xDirection = 1;
		obj->yDirection = 0;
		break;
	case 2:
		obj->xDirection = 0;
		obj->yDirection = 1;
		break;
	case 3:
		obj->xDirection = -1;
		obj->yDirection = 0;
		break;
	}

	if (purple)
	{
		obj->posX += 0x40;
		obj->posY += 0x40;
		CK_SetAction(obj, CK_GetActionByName("CK5_act_purpleAxisPlatform"));
	}
	else
	{

		CK_SetAction(obj, CK_GetActionByName("CK5_act_redAxisPlatform"));
	}
	// TODO: These should *not* be done here.
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

void CK5_SpawnRedStandPlatform(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = obj->user1 = tileY << 8;
	obj->xDirection = 0;
	obj->yDirection = 1;
	obj->clipped = CLIP_not;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_redStandPlatform"));
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

void CK5_OpenMapTeleporter(int tileX, int tileY)
{
	uint16_t tile_array[4];
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			tile_array[y * 2 + x] = CA_TileAtPos(5 * 2 + x, y, 1);
		}
	}
	RF_ReplaceTiles(tile_array, 1, tileX, tileY, 2, 2);
}

void CK5_CloseMapTeleporter(int tileX, int tileY)
{
	uint16_t tile_array[4];
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			tile_array[y * 2 + x] = CA_TileAtPos(x, y, 1);
		}
	}
	RF_ReplaceTiles(tile_array, 1, tileX, tileY, 2, 2);
}

// TODO: Cache stuff here instead of spawner handlers
void CK5_ScanInfoLayer()
{

	//TODO: Work out where to store current map number, etc.
	int mapW = CA_MapHeaders[ca_mapOn]->width;
	int mapH = CA_MapHeaders[ca_mapOn]->height;

	ck_gameState.fusesRemaining = 0;

	for (int y = 0; y < mapH; ++y)
	{
		for (int x = 0; x < mapW; ++x)
		{
			int infoValue = CA_mapPlanes[2][y * mapW + x];
			switch (infoValue)
			{
			case 1:
				CK_SpawnKeen(x, y, 1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				if (ck_gameState.levelState != 13)
					CK_SpawnMapKeen(x, y);
				break;

			case 6:
				if (ck_gameState.difficulty < D_Hard) break;
			case 5:
				if (ck_gameState.difficulty < D_Normal) break;
			case 4:
				CK5_SpawnSparky(x, y);
				break;

			case 9:
				if (ck_gameState.difficulty < D_Hard) break;
			case 8:
				if (ck_gameState.difficulty < D_Normal) break;
			case 7:
				CK5_SpawnMine(x, y);
				break;

			case 12:
				if (ck_gameState.difficulty < D_Hard) break;
			case 11:
				if (ck_gameState.difficulty < D_Normal) break;
			case 10:
				CK5_SpawnSlice(x, y, CD_north);
				break;

			case 15:
				if (ck_gameState.difficulty < D_Hard) break;
			case 14:
				if (ck_gameState.difficulty < D_Normal) break;
			case 13:
				CK5_SpawnRobo(x, y);
				break;

			case 18:
				if (ck_gameState.difficulty < D_Hard) break;
			case 17:
				if (ck_gameState.difficulty < D_Normal) break;
			case 16:
				CK5_SpawnSpirogrip(x, y);
				break;

			case 21:
				if (ck_gameState.difficulty < D_Hard) break;
			case 20:
				if (ck_gameState.difficulty < D_Normal) break;
			case 19:
				CK5_SpawnSliceDiag(x, y);
				break;

			case 24:
				if (ck_gameState.difficulty < D_Hard) break;
			case 23:
				if (ck_gameState.difficulty < D_Normal) break;
			case 22:
				CK5_SpawnSlice(x, y, CD_east);
				break;

			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				//RF_SetScrollBlock(x, y, false);
				if (ck_gameState.levelState == 13)
					CK5_MapKeenTeleSpawn(x, y);
				break;
			case 27:
			case 28:
			case 29:
			case 30:
				CK5_SpawnAxisPlatform(x, y, infoValue - 27, false);
				break;
			case 32:
				CK5_SpawnFallPlat(x, y);
				break;

			case 33:
				if (ck_gameState.difficulty > D_Easy) break;
			case 34:
				if (ck_gameState.difficulty > D_Normal) break;
			case 35:
				CK5_SpawnRedStandPlatform(x, y);
				break;

			case 36:
			case 37:
			case 38:
			case 39:
				CK5_GoPlatSpawn(x, y, infoValue - 36, false);
				break;
			case 40:
				CK5_SneakPlatSpawn(x, y);
				break;
			case 41:
				if (ck_gameState.currentLevel == 12)
				{
					ck_gameState.fusesRemaining = 4;
					CK5_QEDSpawn(x, y);
				}
				else
				{
					ck_gameState.fusesRemaining++;
				}
				break;
			case 44:
				if (ck_gameState.difficulty < D_Hard) break;
			case 43:
				if (ck_gameState.difficulty < D_Normal) break;
			case 42:
				CK5_SpawnAmpton(x, y);
				break;

			case 53:
				if (ck_gameState.difficulty < D_Hard) break;
			case 49:
				if (ck_gameState.difficulty < D_Normal) break;
			case 45:
				CK5_TurretSpawn(x, y, 0);
				break;

			case 54:
				if (ck_gameState.difficulty < D_Hard) break;
			case 50:
				if (ck_gameState.difficulty < D_Normal) break;
			case 46:
				CK5_TurretSpawn(x, y, 1);
				break;

			case 55:
				if (ck_gameState.difficulty < D_Hard) break;
			case 51:
				if (ck_gameState.difficulty < D_Normal) break;
			case 47:
				CK5_TurretSpawn(x, y, 2);
				break;

			case 56:
				if (ck_gameState.difficulty < D_Hard) break;
			case 52:
				if (ck_gameState.difficulty < D_Normal) break;
			case 48:
				CK5_TurretSpawn(x, y, 3);
				break;

			case 69:
				// Spawn extra stunner if Keen has low ammo
				if (ck_gameState.numShots >= 5)
					break;
				infoValue = 68;
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63:
			case 64:
			case 65:
			case 66:
			case 67:
			case 68:
				CK5_SpawnItem(x, y, infoValue - 57);
				break;
			case 70:
				CK5_SpawnItem(x, y, infoValue - 58); // Omegamatic Keycard
				break;

			case 73:
				if (ck_gameState.difficulty < D_Hard) break;
			case 72:
				if (ck_gameState.difficulty < D_Normal) break;
			case 71:
				CK5_SpawnVolte(x, y);
				break;

			case 76:
				if (ck_gameState.difficulty < D_Hard) break;
			case 75:
				if (ck_gameState.difficulty < D_Normal) break;
			case 74:
				CK5_SpawnShelly(x, y);
				break;

			case 79:
				if (ck_gameState.difficulty < D_Hard) break;
			case 78:
				if (ck_gameState.difficulty < D_Normal) break;
			case 77:
				CK5_SpawnSpindred(x, y);
				break;

			case 80:
			case 81:
			case 82:
			case 83:
				CK5_GoPlatSpawn(x, y, infoValue - 80, true);
				break;
			case 84:
			case 85:
			case 86:
			case 87:
				CK5_SpawnAxisPlatform(x, y, infoValue - 84, true);
				break;

			case 90:
				if (ck_gameState.difficulty < D_Hard) break;
			case 89:
				if (ck_gameState.difficulty < D_Normal) break;
			case 88:
				CK5_SpawnMaster(x, y);
				break;

			case 101:
				if (ck_gameState.difficulty < D_Hard) break;
			case 100:
				if (ck_gameState.difficulty < D_Normal) break;
			case 99:
				CK5_SpawnShikadi(x, y);
				break;

			case 104:
				if (ck_gameState.difficulty < D_Hard) break;
			case 103:
				if (ck_gameState.difficulty < D_Normal) break;
			case 102:
				CK5_SpawnShocksund(x, y);
				break;

			case 107:
				if (ck_gameState.difficulty < D_Hard) break;
			case 106:
				if (ck_gameState.difficulty < D_Normal) break;
			case 105:
				CK5_SpawnSphereful(x, y);
				break;

			case 124:
				CK5_SpawnKorath(x, y);
				break;
			case 125:
				// TODO: Signal about teleportation (caching)
				break;
			}
		}
	}

	for (CK_object *obj = ck_keenObj; obj != NULL; obj = obj->next)
	{
		if (obj->active != OBJ_ALWAYS_ACTIVE)
			obj->active = OBJ_INACTIVE;
	}
	// TODO: Some more stuff (including opening elevator after breaking fuses)

	if (ck_gameState.currentLevel == 0)
	{
		int keenYTilePos = ck_keenObj->posY >> 8;

		// The top of the lower shaft is opened if you're above its entrance or
		// on Korath III.
		if (keenYTilePos < 75 || keenYTilePos > 100)
		{
			CK5_CloseMapTeleporter(24, 76);
			CK5_OpenMapTeleporter(22, 55);
		}

		// Unlock the entrance to the upper shaft if we're below the top and the
		// fuses are broken.
		if (ck_gameState.levelsDone[4] &&
			ck_gameState.levelsDone[6] &&
			ck_gameState.levelsDone[8] &&
			ck_gameState.levelsDone[10] &&
			keenYTilePos > 39)
		{
			CK5_OpenMapTeleporter(26, 55);
		}

		// Unlock the top elevator when we're at the top or on Korath III.
		if (keenYTilePos < 39 || keenYTilePos > 100)
		{
			CK5_OpenMapTeleporter(24, 30);
		}
	}
}

// Galaxy Explosion Ending Sequence
uint8_t endsplosion_pal_change[][18] =
{
	{ 0x8, 0x8, 0x7, 0xF, 0x7, 0x8, 0x0, 0x8, 0x7, 0xF, 0x7, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	{ 0x7, 0x7, 0x7, 0x7, 0x7, 0xF, 0x7, 0x8, 0x0, 0x7, 0xF, 0x7, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0},
};

uint8_t endsplosion_palette[17] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x3 };

/*
 * There can be 4000 stars in the galaxy ending
 * Each star is defined by an initial position and velocity vector
 * Each pixel is 0x80 units square, upper left of screen is (0,0)
 * When galaxy explodes, star is updated by its velocity component
 *  on every tick, until it exceeds screen boundaries.
 */

typedef struct CK5_GalExplode
{
	uint16_t x[4000];
	int16_t dx[4000];
	uint16_t y[4000];
	int16_t dy[4000];
} CK5_GalExplode;

void CK_GalExplodePageFlip(int offset)
{

	VL_Present();
}

void CK_GalExplodeUpdateCoords(int offset)
{
	// NOTE: Normally, the offset would be set to allow for page flipping
	// But we don't need to worry about that in omnispeak

	// Blank the video buffer
	VH_Bar(0, 0, 320, 200, 0);

	CK5_GalExplode *info = (CK5_GalExplode*)ca_graphChunks[4925];

	// Update the star positions
	// Each pixel is 0x80 x 0x80 "distance units"
	for (int i = 3999; i >= 0; i--)
	{
		uint16_t newPos;

		newPos = info->x[i] + info->dx[i];
		if (newPos > 320 * 0x80)
			continue;
		info->x[i] = newPos;

		newPos = info->y[i] + info->dy[i];
		if (newPos > 200 * 0x80)
			continue;
		info->y[i] = newPos;

		VH_Plot(info->x[i]/0x80, info->y[i]/0x80, 0xF);
	}
}

void CK5_ExplodeGalaxy()
{
	// purge_chunks()
	VL_SetScrollCoords(0,0);

	VL_FadeToBlack();
	CA_CacheGrChunk(89); // Galaxy Pic
	CA_CacheGrChunk(4925); // Star Coords
	CA_CacheGrChunk(98);  // Game Over Pic

	// VW_SetLineWidth(40);
	// VH_SetScreen(0);
	// VW_ClearVideo(0);
	VL_ClearScreen(0);

	// Draw the galaxy
	VH_DrawBitmap(0, 0, 89);
	VL_FadeFromBlack();
	IN_ClearKeysDown();
	SD_PlaySound(SOUND_GALAXYEXPLODEPRE);


	// Galaxy is about to explode
	for (int i = 0; i < 18; i++)
	{
		IN_PumpEvents();

		endsplosion_palette[8] = endsplosion_pal_change[0][i];
		endsplosion_palette[7] = endsplosion_pal_change[0][i];
		VL_SetPaletteAndBorderColor(endsplosion_palette);
		//VW_WaitVBL(10);
		VL_DelayTics(10);

		if (IN_GetLastScan())
			goto done;

		VL_Present();
	}


	// Write Mode 2();
	// Set Plane Write Mask;

	SD_PlaySound(SOUND_GALAXYEXPLODE);
	VL_ClearScreen(0);

	for (int i = 0; i < 30; i++)
	{
		IN_PumpEvents();

		SD_SetLastTimeCount(SD_GetTimeCount());

		CK_GalExplodeUpdateCoords(2000);
		CK_GalExplodePageFlip(2000);

		// Delay
		while (SD_GetTimeCount() - SD_GetLastTimeCount() < 4)
			;

		SD_SetLastTimeCount(SD_GetTimeCount());

		CK_GalExplodeUpdateCoords(0);
		CK_GalExplodePageFlip(0);

		// Delay
		while (SD_GetTimeCount() - SD_GetLastTimeCount() < 4)
			;

		if (IN_GetLastScan())
			goto done;

	}

done:
	// Set Video back to normal
	VL_ClearScreen(0);
	// VW_SetLineWidth(0);
	VL_SetDefaultPalette();

	// RF_Reset();

	StartMusic(18);

	VH_DrawBitmap(32, 80, 98);
	VL_Present();

	IN_UserInput(24 * 70, false);

	StopMusic();
}

// Fuse Explosion Message

extern uint8_t ca_levelbit;
extern uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS];

void CK5_FuseMessage()
{
	SD_WaitSoundDone();

	// Cache the Keen thumbs up pic
	CA_UpLevel();
	ca_graphChunkNeeded[0x5A] |= ca_levelbit;
	ca_graphChunkNeeded[0x5B] |= ca_levelbit;
	CA_CacheMarks(0);

	// VW_SyncPages();

	// Draw Keen Talking
	US_CenterWindow(0x1A, 8);
	US_SetWindowW(US_GetWindowW() - 0x30);
	VH_DrawBitmap(US_GetWindowW() + US_GetWindowX(), US_GetWindowY(), 0x5A);
	US_SetPrintY(US_GetPrintY() + 0xC);

	if (ck_gameState.currentLevel == 0xD)
		US_Print("I wonder what that\nfuse was for....\n");
	else
		US_Print("One of the four\nmachines protecting the\nmain elevator shaft--\ntoast!\n");

	// VW_UpdateScreen();
	VL_Present();
	// VW_WaitVBL(30);

	IN_ClearKeysDown();
	// TODO: Add Joystick compatability here
	// IN_WaitForButton();
	IN_WaitButton();

	// Draw the Keen Thumbs Up Pic
	VH_DrawBitmap(US_GetWindowW() + US_GetWindowX(), US_GetWindowY(), 0x5B);
	VL_Present();
	// VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
	// StopMusic();
}

