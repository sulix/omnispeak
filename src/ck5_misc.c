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
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_act.h"
#include "ck5_ep.h"
#include <stdio.h>

CK_Episode ck5_episode ={
	"CK5",
	&CK5_SetupFunctions,
	&CK5_ScanInfoLayer
};


// Contains some keen-5 specific functions.

//StartSprites + 
int CK5_ItemSpriteChunks[] ={
	122, 124, 126, 128,
	108, 110, 112, 114, 116, 118,
	120, 131, 105
};

int CK5_ItemNotifyChunks[] ={
	232, 232, 232, 232,
	195, 196, 197, 198, 199, 200,
	201, 202, 209
};


// Think function for adding gravity

void CK_Fall(CK_object *obj)
{
	CK_PhysGravityHigh(obj);
	obj->nextX = obj->velX * CK_GetTicksPerFrame();
}

// Think function for adding a slightly lower amount of gravity

void CK_Fall2(CK_object *obj)
{
	CK_PhysGravityMid(obj);
	obj->nextX = obj->velX * CK_GetTicksPerFrame();
}

void CK_Glide(CK_object *obj)
{
	obj->nextX = obj->velX * CK_GetTicksPerFrame();
	obj->nextY = obj->velY * CK_GetTicksPerFrame();
}

void CK_BasicDrawFunc1(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * For walking and turning around at edges 
 */
void CK_BasicDrawFunc2(CK_object *obj)
{
	// Hit wall walking right; turn around and go left
	if (obj->xDirection == IN_motion_Right && obj->leftTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Hit wall walking left; turn around and go right
	else if (obj->xDirection == IN_motion_Left && obj->rightTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Walked off of ledge; turn around
	else if (obj->topTI == 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Think function for stunned creatures
 */
void CK_BasicDrawFunc4(CK_object *obj)
{
	int starsX, starsY;

	// Handle physics
	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = 0;
	}

	if (obj->bottomTI)
	{
		obj->velY = 0;
	}

	if (obj->topTI)
	{
		obj->velX = obj->velY = 0;
		if (obj->currentAction->next)
		{
			CK_SetAction2(obj, obj->currentAction->next);
		}
	}

	// Draw the primary chunk
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);


	// Draw the stunner stars

	starsX = starsY = 0;

	switch (obj->type)
	{
	case CT_Sparky:
		starsX += 0x40;
		break;
	case 14:
		starsY -= 0x80;
		break;
	case 0x17:
		starsY -= 0x80;
		break;
	}

	// Tick the star 3-frame animation forward
	if (obj->user1 += CK_GetTicksPerFrame() > 10)
	{
		obj->user1 -= 10;
		if (++obj->user2 > 3)
			obj->user2 = 0;
	}

	// FIXME: Will cause problems on 64-bit systems
	RF_AddSpriteDraw((RF_SpriteDrawEntry**) (&obj->user3), obj->posX + starsX, obj->posY + starsY, obj->user2 + 143, false, 3);
}

void CK5_PointItem(CK_object *obj)
{
	//	obj->timeUntillThink = 20;
	obj->visible = true;
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
}

void CK5_BlockPlatform(CK_object *obj)
{
	int nextPosUnit, nextPosTile;

	if (obj->nextX || obj->nextY)
	{
		return;
	}
	//TODO: Implement properly.
	obj->nextX = obj->xDirection * 12 * CK_GetTicksPerFrame();
	obj->nextY = obj->yDirection * 12 * CK_GetTicksPerFrame();

	if (obj->xDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitX2 + obj->nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ck_currentMapNumber]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			obj->nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + obj->nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ck_currentMapNumber]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = 1;
			//TODO: Change DeltaVelocity
			//CK_PhysUpdateX(obj, 256 - nextPosUnit&255);
			obj->nextX += 256 - nextPosUnit & 255;
		}
	}
	else if (obj->yDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitY2 + obj->nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ck_currentMapNumber]->width * nextPosTile + obj->clipRects.tileX1 + obj->user1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile - 2, 2) == 0x1F)
			{
				//Stop the platform.
				obj->visible = true;
				obj->nextY = 0;
			}
			else
			{
				obj->yDirection = -1;
				//TODO: Change DeltaVelocity
				obj->nextY -= ( nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + obj->nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ck_currentMapNumber]->width * nextPosTile + obj->clipRects.tileX1 + obj->user1] == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile + 2, 2) == 0x1F)
			{
				// Stop the platform.
				obj->visible = true;
				obj->nextY = 0;
			}
			else
			{
				obj->yDirection = 1;
				//TODO: Change DeltaVelocity
				obj->nextY +=  256 - (nextPosUnit & 255);
			}
		}
	}
}

void CK_DeadlyCol(CK_object *o1, CK_object *o2)
{
}

void CK5_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
	CK_ACT_AddColFunction("CK_DeadlyCol", &CK_DeadlyCol);
	CK5_Obj1_SetupFunctions();
	CK5_Obj2_SetupFunctions();
	CK5_Obj3_SetupFunctions();
	CK_ACT_AddFunction("CK_Fall", &CK_Fall);
	CK_ACT_AddFunction("CK_Fall2", &CK_Fall2);
	CK_ACT_AddFunction("CK_Glide", &CK_Glide);
	CK_ACT_AddFunction("CK_BasicDrawFunc1", &CK_BasicDrawFunc1);
	CK_ACT_AddFunction("CK_BasicDrawFunc2", &CK_BasicDrawFunc2);
	CK_ACT_AddFunction("CK_BasicDrawFunc4", &CK_BasicDrawFunc4);
	CK_ACT_AddFunction("CK5_PointItem", &CK5_PointItem);
	CK_ACT_AddFunction("CK5_BlockPlatform", &CK5_BlockPlatform);
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

	obj->clipped = false;
	obj->active = OBJ_ACTIVE;
	obj->type = 5;	//OBJ_ITEM
	obj->zLayer = 2;
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->user1 = itemNumber;
	obj->gfxChunk = CK5_ItemSpriteChunks[itemNumber] + ca_gfxInfoE.offSprites;
	obj->user2 = obj->gfxChunk;
	obj->user3 = obj->gfxChunk + 2;
	obj->user4 = CK5_ItemNotifyChunks[itemNumber];
	CK_SetAction(obj, CK_GetActionByName("CK5_act_item") );
	CA_CacheGrChunk(obj->gfxChunk);
	CA_CacheGrChunk(obj->gfxChunk + 1);
}

void CK5_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple)
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
		obj->user1 = 1;
		obj->posX += 0x40;
		obj->posY += 0x40;
		CK_SetAction(obj, CK_GetActionByName("CK5_act_purpleBlockPlatform"));
	}
	else
	{

		obj->user1 = 0;
		CK_SetAction(obj, CK_GetActionByName("CK5_act_redBlockPlatform"));
	}
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

void CK5_ScanInfoLayer()
{

	//TODO: Work out where to store current map number, etc.
	int mapW = CA_MapHeaders[ck_currentMapNumber]->width;
	int mapH = CA_MapHeaders[ck_currentMapNumber]->height;

	for (int y = 0; y < mapH; ++y)
	{
		for (int x = 0; x < mapW; ++x)
		{
			int infoValue = CA_mapPlanes[2][y * mapW + x];
			switch (infoValue)
			{
			case 1:
				CK_SpawnKeen(x, y, 1);
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				break;
			case 6:
			case 5:
			case 4:
				CK5_SpawnSparky(x, y);
				break;

			case 9:
			case 8:
			case 7:
				CK5_SpawnMine(x, y);
				break;

			case 12:
			case 11:
			case 10:
				CK5_SpawnSlice(x, y, CD_north);
				break;

			case 15:
			case 14:
			case 13:
				CK5_SpawnRobo(x, y);
				break;

			case 18:	//TODO: Difficulty hard
			case 17:	//TODO: Difficulty normal
			case 16:
				CK5_SpawnSpirogrip(x, y);
				break;

			case 21:
			case 20:
			case 19:
				CK5_SpawnSliceDiag(x, y);
				break;

			case 24:
			case 23:
			case 22:
				CK5_SpawnSlice(x, y, CD_east);
				break;

			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				RF_SetScrollBlock(x, y, false);
				break;
			case 27:
			case 28:
			case 29:
			case 30:
				CK5_SpawnRedBlockPlatform(x, y, infoValue - 27, false);
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
			case 44:
			case 43:
			case 42:
				CK5_SpawnAmpton(x, y);
				break;
			case 45:
			case 46:
			case 47:
			case 48:
				CK5_TurretSpawn(x, y, infoValue - 45);
				break;
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


			case 76:
			case 75:
			case 74:
				CK5_SpawnShelly(x, y);
				break;

			case 79:
			case 78:
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
				CK5_SpawnRedBlockPlatform(x, y, infoValue - 84, true);
				break;

			case 90:
			case 89:
			case 88:
				CK5_SpawnMaster(x, y);
				break;

			case 101:
			case 100:
			case 99:
				CK5_SpawnShikadi(x, y);
				break;

			case 104:
			case 103:
			case 102:
				CK5_SpawnShocksund(x, y);
				break;

			case 107:
			case 106:
			case 105:
				CK5_SpawnSphereful(x, y);
				break;

			case 124:
				CK5_SpawnKorath(x, y);
				break;
			}
		}
	}
}
