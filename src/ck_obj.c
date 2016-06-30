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

void CK_OBJ_SetupFunctions()
{
	CK_ACT_AddFunction("CK_DoorOpen", &CK_DoorOpen);
	CK_ACT_AddFunction("CK_SecurityDoorOpen", &CK_SecurityDoorOpen);
  CK_ACT_AddFunction("CK_PointItem", &CK_PointItem);
}
