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

#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck_act.h"
#include "id_heads.h"
#include "id_rf.h"
#include "id_us.h"

// This file contains some object functions (think, etc) which are common to
// several episodes.
// AFAIK, There never was a "ck_obj.c" in the DOS source

void CK_DoorOpen(CK_object *obj)
{
	uint16_t tilesToReplace[0x30];

	if (obj->user1 + 2 > 0x30)
	{
		Quit("Door too tall!");
	}

	for (int i = 0; i < obj->user1 + 2; ++i)
	{
		tilesToReplace[i] = CA_TileAtPos(obj->posX, obj->posY+i, 1) + 1;
	}

	RF_ReplaceTiles(tilesToReplace, 1, obj->posX, obj->posY, 1, obj->user1 + 2);
}

void CK_SecurityDoorOpen(CK_object *obj)
{
	uint16_t tilesToReplace[0x30];
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			tilesToReplace[y*4+x] = CA_TileAtPos(obj->posX+x, obj->posY+y, 1) - 4;
		}
	}

	RF_ReplaceTiles(tilesToReplace, 1, obj->posX, obj->posY, 4, 4);
	obj->user1++;
	if (obj->user1 == 3)
	{
		obj->currentAction = 0;
	}
}

// Should be an integer array
// Is int* array for multiple episode support
int *CK_ItemSpriteChunks[] ={
  &SPR_GEM_A1, &SPR_GEM_B1, &SPR_GEM_C1, &SPR_GEM_D1,
  &SPR_100_PTS1, &SPR_200_PTS1, &SPR_500_PTS1, &SPR_1000_PTS1, &SPR_2000_PTS1, &SPR_5000_PTS1,
  &SPR_1UP1, &SPR_STUNNER1, &SPR_SECURITYCARD_1
};

// Object and Centilife functions "should" be in ckx_obj1.c
// but they are similar enough between episodes to put them all here
void CK_SpawnItem(int tileX, int tileY, int itemNumber)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->clipped = CLIP_not;
	//obj->active = OBJ_ACTIVE;
	obj->zLayer = 2;
	obj->type = CT_CLASS(Item);
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->yDirection = -1;
	obj->user1 = itemNumber;
	obj->gfxChunk = (int16_t)*CK_ItemSpriteChunks[itemNumber];
	obj->user2 = obj->gfxChunk;
	obj->user3 = obj->gfxChunk + 2;
	CK_SetAction(obj, CK_GetActionByName("CK_ACT_item") );
	// TODO: Wrong place to cache?
	CA_CacheGrChunk(obj->gfxChunk);
	CA_CacheGrChunk(obj->gfxChunk + 1);
}

void CK_SpawnCentilifeNotify(int tileX, int tileY)
{
	CK_object *notify = CK_GetNewObj(true);
	notify->type = 1;
	notify->clipped = CLIP_not;
	notify->zLayer = 3;
	notify->posX = tileX << 8;
	notify->posY = tileY << 8;

	CK_SetAction(notify, CK_GetActionByName("CK_ACT_CentilifeNotify1"));
}

void CK_PointItem(CK_object *obj)
{
	//	obj->timeUntillThink = 20;
	obj->visible = true;
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
}

// Keen 6 specific (but also appears to be present in the Keen 5 EXE)
void CK_FallingItem(CK_object *obj)
{
	if (obj->topTI)
		CK_SetAction(obj, CK_GetActionByName("CK_ACT_item"));
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
	CK_PhysGravityHigh(obj);
}

// Platforms

// CK4: ck4_obj2.c
// CK5: ck5_obj?.c - This is the RED axis platform
// CK6 ??
void CK_AxisPlatform(CK_object *obj)
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

void CK_SpawnFallPlat(int tileX, int tileY)
{
	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_CLASS(Platform);
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = tileX << 8;
	new_object->user1 = new_object->posY = tileY << 8;
	new_object->xDirection = IN_motion_None;
	new_object->yDirection = IN_motion_Down;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK_ACT_FallPlat0"));
}

void CK_FallPlatSit (CK_object *obj)
{

	if (obj == ck_keenState.platform)
	{
		ck_nextY = SD_GetSpriteSync() * 16;
		obj->velY = 0;
		if ((unsigned)(obj->posY + ck_nextY - obj->user1) >= 0x80)
			obj->currentAction = CK_GetActionByName("CK_ACT_FallPlat1");
	}
}

void CK_FallPlatFall (CK_object *obj)
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
			obj->currentAction = CK_GetActionByName("CK_ACT_FallPlat2");
	}
}

void CK_FallPlatRise (CK_object *obj)
{
	if (ck_keenState.platform == obj)
	{
		obj->velY = 0;
		obj->currentAction = CK_GetActionByName("CK_ACT_FallPlat1");
	}
	else if ((unsigned) obj->posY <= (unsigned) obj->user1)
	{
		ck_nextY = obj->user1 - obj->posY;
		obj->currentAction = CK_GetActionByName("CK_ACT_FallPlat0");
	}
}

void CK_OBJ_SetupFunctions()
{
	CK_ACT_AddFunction("CK_DoorOpen", &CK_DoorOpen);
	CK_ACT_AddFunction("CK_SecurityDoorOpen", &CK_SecurityDoorOpen);
	CK_ACT_AddFunction("CK_PointItem", &CK_PointItem);
	CK_ACT_AddFunction("CK_FallingItem", &CK_FallingItem);

	CK_ACT_AddFunction("CK_AxisPlatform", &CK_AxisPlatform);

	CK_ACT_AddFunction("CK_FallPlatSit", &CK_FallPlatSit);
	CK_ACT_AddFunction("CK_FallPlatFall", &CK_FallPlatFall);
	CK_ACT_AddFunction("CK_FallPlatRise", &CK_FallPlatRise);
}
