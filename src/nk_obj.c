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
#include "ck_net.h"

#include "nk_keen.h"

#include "nk5_ep.h"

#include "id_heads.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_ti.h"
#include "id_ca.h"
#include "id_sd.h"

// For all the shitty debug stuff  I have.
#include <stdio.h>

#define max(a,b) (((a)>(b))?(a):(b))


// Gemdoors
void NK_DoorOpen(CK_object *obj)
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

// Should be an integer array
// Is int* array for multiple episode support
int *NK_ItemSpriteChunks[] ={
  &SPR_GEM_A1, &SPR_GEM_B1, &SPR_GEM_C1, &SPR_GEM_D1,
  &SPR_100_PTS1, &SPR_200_PTS1, &SPR_500_PTS1, &SPR_1000_PTS1, &SPR_2000_PTS1, &SPR_5000_PTS1,
  &SPR_1UP1, &SPR_STUNNER1, &SPR_BOMB
};

// Object and Centilife functions "should" be in ckx_obj1.c
// but they are similar enough between episodes to put them all here
void NK_SpawnItem(int tileX, int tileY, int itemNumber)
{
	CK_object *obj = CK_Net_GetNewObj(false);

	obj->clipped = CLIP_not;
	obj->zLayer = 2;
	obj->type = CTN_Item;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY);
	obj->yDirection = -1;
	obj->user1 = itemNumber;
	obj->gfxChunk = (int16_t)*NK_ItemSpriteChunks[itemNumber];
	obj->user2 = obj->gfxChunk;
	obj->user3 = obj->gfxChunk + 2;
	CK_SetAction(obj, CK_GetActionByName("NK_ACT_item") );
}

void NK_PointItem(CK_object *obj)
{
	//	obj->timeUntillThink = 20;
	obj->visible = true;
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
}

void NK_SpawnCentilifeNotify(int tileX, int tileY)
{
	CK_object *notify = CK_Net_GetNewObj(true);
	notify->type = 1;
	notify->clipped = CLIP_not;
	notify->zLayer = 3;
	notify->posX = RF_TileToUnit(tileX);
	notify->posY = RF_TileToUnit(tileY);

	CK_SetAction(notify, CK_GetActionByName("NK_ACT_CentilifeNotify1"));
}

void NK_ObjSetupFunctions()
{
    // Items
	CK_ACT_AddFunction("NK_DoorOpen", &NK_DoorOpen);
    CK_ACT_AddFunction("NK_PointItem", &NK_PointItem);
}
