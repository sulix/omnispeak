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
#include "id_ca.h"
#include "id_in.h"
#include "id_rf.h"
#include "ck6_ep.h"

#include <stdio.h>

// =========================================================================

//

void CK6_BloogletItem(CK_object *obj)
{
  if (obj->topTI)
  {
    obj->currentAction = CK_GetActionByName("CK_ACT_item");
    if (++obj->gfxChunk == obj->user3)
      obj->gfxChunk = obj->user2;
  }

  CK_PhysGravityHigh(obj);
}

// Go Plats

void CK6_GoPlatDraw(CK_object *obj)
{
  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
  RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user3Ptr), obj->posX + 0x100, obj->posY + 0x100, obj->user1 + 425, false, 0);
}


// Bloogs

void CK6_SpawnBloog(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Bloog;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x200;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogWalk0"));
}
// Bloogs
void CK6_Bloog(CK_object *obj)
{
  if (US_RndT() < 0x20)
    obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
}

void CK6_BloogCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_KillKeen();
  }
  else if (b->type == CT_Stunner)
  {
    CK_StunCreature(a,b,CK_GetActionByName("CK6_ACT_BloogStunned0"));
  }
}

int ck6_smashScreenDistance;
int16_t ck6_smashScreenOfs [] =
{
  0, -64, -64, -64, 64, 64, 64, -200, -200, -200, 200, 200, 200,
  -250, -250, -250, 250, 250, 250, -250, -250, -250, 250, 250, 250
};

void CK6_SpawnBloogguard(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Bloogguard;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x280;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  obj->user2 = 3;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogguardWalk0"));
}

void CK6_BloogguardWalk(CK_object *obj)
{
  if (US_RndT() < 0x20)
    obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;

  if (obj->xDirection == IN_motion_Right && ck_keenObj->posX > obj->posX ||
      obj->xDirection == IN_motion_Left && ck_keenObj->posX < obj->posX)
  {
    if (obj->clipRects.unitY2 == ck_keenObj->clipRects.unitY2)
    {
      if (US_RndT() < 0x20)
        obj->currentAction = CK_GetActionByName("CK6_ACT_BloogguardClub0");
    }
  }
}

#define SOUND_BLOOGGUARDSMASH 0x34
void CK6_BloogguardSmash(CK_object *obj)
{
  SD_PlaySound(SOUND_BLOOGGUARDSMASH);
  ck6_smashScreenDistance = 25;
  if (ck_keenObj->topTI)
    CK_SetAction2(ck_keenObj, CK_GetActionByName("CK6_ACT_keenStunned0"));
}

void CK6_BloogguardCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_KillKeen();
  }
  else if (b->type == CT_Stunner)
  {
    if (--a->user2 == 0)
    {
      CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_BloogguardStunned0"));
    }
    else
    {
      a->user1 = 2;
      a->visible = true;
      CK_ShotHit(b);
    }
  }
}

// Also used for Fleex and Nospikes
void CK6_MultihitDraw(CK_object *obj)
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
		obj->posX -= obj->deltaPosX * 2;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

  if (obj->user1)
  {
    obj->user1--;
    RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, true, obj->zLayer);
  }
  else
  {
    RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
  }
}

// Blooglets
void CK6_SpawnBlooglet(int tileX, int tileY, int type)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Blooglet;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x80;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  obj->user1 = type;

  switch (type%4)
  {
    case 0:
      CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogletRRun0"));
      break;
    case 1:
      CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogletYRun0"));
      break;
    case 2:
      CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogletBRun0"));
      break;
    case 3:
      CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BloogletGRun0"));
      break;
  }
}

#define SOUND_BLOOGLETGEM 5
static char *stunnedBloogletActions[] = {
  "CK6_ACT_BloogletRStunned0",
  "CK6_ACT_BloogletYStunned0",
  "CK6_ACT_BloogletBStunned0",
  "CK6_ACT_BloogletGStunned0",
};

extern int16_t CK6_ItemSpriteChunks[];
void CK6_BloogletCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    if (b->currentAction->collide)
    {
      ck_keenIgnoreVertClip = true;
      CK_PhysPushX(b, a);
      ck_keenIgnoreVertClip = false;
    }
  }
  else if (b->type == CT_Stunner)
  {
    int color = a->user1 & 3;
    if (a->user1 > 3)
    {
      CK_object *gem = CK_GetNewObj(false);
      gem->clipped = true;
      gem->zLayer = PRIORITIES - 2;
      gem->type = CT6_Item;
      gem->posX = a->posX;
      gem->posY = a->posY;
      gem->yDirection = IN_motion_Up;
      gem->velY = -40;
      gem->user1 = color;
      gem->user2 = gem->gfxChunk = CK6_ItemSpriteChunks[color];
      gem->user3 = gem->user2 + 2;
      CK_SetAction(gem, CK_GetActionByName("CK6_ACT_bloogletItem1"));
      SD_PlaySound(SOUND_BLOOGLETGEM);
    }

    CK_StunCreature(a, b, CK_GetActionByName(stunnedBloogletActions[color]));
  }
}

/*
 * Setup all of the functions in this file.
 */
void CK6_Obj1_SetupFunctions()
{
  CK_ACT_AddFunction("CK6_BloogletItem", &CK6_BloogletItem);

  CK_ACT_AddFunction("CK6_GoPlatDraw", &CK6_GoPlatDraw);

  CK_ACT_AddFunction("CK6_Bloog", &CK6_Bloog);
  CK_ACT_AddColFunction("CK6_BloogCol", &CK6_BloogCol);

  CK_ACT_AddFunction("CK6_BloogguardWalk", &CK6_BloogguardWalk);
  CK_ACT_AddFunction("CK6_BloogguardSmash", &CK6_BloogguardSmash);
  CK_ACT_AddColFunction("CK6_BloogguardCol", &CK6_BloogguardCol);
  CK_ACT_AddFunction("CK6_MultihitDraw", &CK6_MultihitDraw);

  CK_ACT_AddColFunction("CK6_BloogletCol", &CK6_BloogletCol);


}