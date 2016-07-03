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
#include "ck4_ep.h"

#include <stdio.h>

// =========================================================================

void CK4_SpawnWormmouth(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Wormmouth;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) + 0x8F;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_WormmouthMove0"));
}

void CK4_WormmouthLookRight(CK_object *obj)
{
  if (ck_keenObj->posX > obj->posX)
  {
     obj->xDirection = IN_motion_Right;
     obj->currentAction = CK_GetActionByName("CK4_ACT_WormmouthMove0");
  }
}

void CK4_WormmouthGoToPlayer(CK_object *obj)
{
  obj->xDirection = ck_keenObj->posX > obj->posX ? IN_motion_Right : IN_motion_Left;
}

void CK4_WormmouthLookLeft(CK_object *obj)
{
  if (ck_keenObj->posX < obj->posX)
  {
    obj->xDirection = IN_motion_Left;
    obj->currentAction = CK_GetActionByName("CK4_ACT_WormmouthMove0");
  }
}

void CK4_WormmouthMove(CK_object *obj)
{
  int16_t dx = (int16_t)ck_keenObj->posX - (int16_t)obj->posX;
  int16_t dy = (int16_t)ck_keenObj->clipRects.unitY2 - (int16_t)obj->clipRects.unitY2;

  if ((dx < -0x300 || dx > 0x300) && US_RndT() < 6)
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_WormmouthPeep0");
    return;
  }

  if (dy >= -0x100 && dy <= 0x100)
  {
    if (obj->xDirection == IN_motion_Right && dx > 0x80 && dx < 0x180 ||
        obj->xDirection == IN_motion_Left && dx < -0x80 && dx > -0x200)
    {
      SD_PlaySound(SOUND_WORMMOUTHBITE);
      obj->currentAction = CK_GetActionByName("CK4_ACT_WormmouthBite0");
    }
  }
}

void CK4_WormmouthCheckShot(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
    CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_WormmouthStunned0"));
}

void CK4_WormmouthCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    if (a->xDirection == IN_motion_Right && a->posX <= b->posX ||
        a->xDirection == IN_motion_Left && a->posX >= b->posX)
    {
      CK_KillKeen();
      return;
    }
  }

  CK4_WormmouthCheckShot(a, b);
}

// Clouds

void CK4_SpawnCloud(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Cloud;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 2;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->xDirection = IN_motion_Right;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_CloudDormant0"));
}

void CK4_CloudMove(CK_object *obj)
{
  if (US_RndT() < SD_GetSpriteSync())
    obj->xDirection = ck_keenObj->posX < obj->posX ? IN_motion_Left : IN_motion_Right;

  CK_PhysAccelHorz(obj, obj->xDirection, 10);

  if (ck_keenObj->clipRects.unitY2 >= obj->clipRects.unitY2 &&
      ck_keenObj->clipRects.unitY1 - obj->clipRects.unitY2 <= 0x400 &&
      obj->clipRects.unitX1 < ck_keenObj->clipRects.unitX2 &&
      obj->clipRects.unitX2 > ck_keenObj->clipRects.unitX1)
  {
     obj->currentAction = CK_GetActionByName("CK4_ACT_CloudCheckStrike0");
  }
}

void CK4_CloudCheckStrike(CK_object *obj)
{
  CK_PhysAccelHorz(obj, obj->xDirection, 10);

  // Align the cloud on an 8px boundary so that the lightning bolt,
  // which has one shift, is aligned properly
  if (ck_nextX < 0 && (obj->posX & 0x7F) + ck_nextX <= 0)
  {
    ck_nextX = -(obj->posX & 0x7F);
    obj->currentAction = CK_GetActionByName("CK4_ACT_CloudStrike0");
  }

  if (ck_nextX > 0 && (obj->posX & 0x7F) + ck_nextX >= 0x80)
  {
    ck_nextX = 0x80 - (obj->posX & 0x7F);
    obj->currentAction = CK_GetActionByName("CK4_ACT_CloudStrike0");
  }
}

void CK4_CloudDraw(CK_object *obj)
{
  if (obj->leftTI)
  {
    obj->velX = 0;
    obj->xDirection = IN_motion_Left;
  }
  else if (obj->rightTI)
  {
    obj->velX = 0;
    obj->xDirection = IN_motion_Right;
  }

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_CloudStrike(CK_object *obj)
{
  CK_object *bolt = CK_GetNewObj(true);
  bolt->type = CT4_Bolt;
  bolt->active = OBJ_EXISTS_ONLY_ONSCREEN;
  bolt->clipped = CLIP_not;
  bolt->posX = obj->posX + 0x100;
  bolt->posY = obj->posY + 0x100;
  CK_SetAction(bolt, CK_GetActionByName("CK4_ACT_Lightning0"));
  SD_PlaySound(SOUND_LIGHTNINGBOLT);
}

void CK4_CloudCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
    CK_SetAction2(a, CK_GetActionByName("CK4_ACT_CloudAwaken0"));
}

// Berkeloids
// user1 = vertical offset for floating behaviour
// user2 = instantaneous floating velocity

void CK4_SpawnBerkeloid(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Berkeloid;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 2;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x200;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  obj->user2 = 8;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_BerkeloidMove0"));
}

void CK4_BerkeloidMove(CK_object *obj)
{
  if (US_RndT() < 0x20)
    obj->xDirection = ck_keenObj->posX < obj->posX ? IN_motion_Left : IN_motion_Right;

  if (US_RndT() < 8)
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_BerkeloidThrow0");
  }
  else if (US_RndT() < 0x40)
  {
    int16_t dx = ck_keenObj->posX - obj->posX;
    int16_t dy = ck_keenObj->posY - obj->posY;

    if (dy >= -0x100 && dy <= 0x100) {
      if (obj->xDirection == IN_motion_Right && dx > 0 ||
          obj->xDirection == IN_motion_Left && dx < 0)
      {
        obj->currentAction = CK_GetActionByName("CK4_ACT_BerkeloidThrow0");
        SD_PlaySound(SOUND_BERKELOIDTHROW);
      }
    }
  }
}

void CK4_BerkeloidThrow(CK_object *obj)
{
  CK_object *ball = CK_GetNewObj(true);
  ball->active = OBJ_EXISTS_ONLY_ONSCREEN;
  ball->type = CT4_Berkeloid;
  ball->posY = obj->posY + 0x80;
  ball->velY = -8;
  if (obj->xDirection == IN_motion_Right)
  {
    ball->velX = 48;
    ball->posX = obj->posX + 0x200;
    ball->xDirection = IN_motion_Right;
  }
  else
  {
     ball->velX = -48;
     ball->posX = obj->posX - 0x100;
     ball->xDirection = IN_motion_Left;
  }

  CK_SetAction(ball, CK_GetActionByName("CK4_ACT_FireballAir0"));
  obj->visible = true;
}

void CK4_BerkeloidThrowDone(CK_object *obj)
{
  obj->timeUntillThink = 4;
  obj->visible = true;
}

void CK4_BerkeloidCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    CK_ShotHit(b);
  }
  else if (b->type == CT_Player)
  {
     CK_KillKeen();
  }
}

void CK4_FireballDraw(CK_object *obj)
{
  if (obj->rightTI || obj->leftTI)
    obj->velX = 0;

  if (obj->topTI)
  {
     SD_PlaySound(SOUND_FIREBALLLAND);
     CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_FireballGround0"));
  }

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_BerkeloidHover(CK_object *obj)
{
  obj->user1 = obj->user2 * SD_GetSpriteSync() + obj->user1;
  if (obj->user1 > 0)
  {
    obj->user1 = 0;
    obj->user2 = -8;
  }
  else if (obj->user1 < -0x100)
  {
    obj->user1 = -0x100;
    obj->user2 = 8;
  }

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY + obj->user1, obj->gfxChunk, false, obj->zLayer);
}

void CK4_BerkeloidDraw(CK_object *obj)
{
  // DrawFunc2 with hovering added
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
	else if (obj->topTI == 0)
	{
		obj->posX -= obj->deltaPosX * 2;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

  CK4_BerkeloidHover(obj);
}

// Foot and worms
void CK4_SpawnInchworm(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Inchworm;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 2;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Inchworm0"));
  obj->actionTimer = US_RndT() / 32; // So the worms don't all inch in unison?
}

void CK4_SpawnFoot(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Foot;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY - 3 << G_T_SHIFT;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Foot1"));
}

void CK4_InchwormMove(CK_object *obj)
{
  obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
}

void CK4_InchwormCol(CK_object *a, CK_object *b)
{
	int32_t lastTimeCount = SD_GetLastTimeCount();

  if (b->type == CT4_Inchworm)
  {
    // Here, DOS keen stores the low word of the 32-bit lastTimeCount = user1
    // Omnispeak can store the whole thing. Either way, it's irrelevant, because
    // user1 is just being used as a "static" variable for this function for this frame
    if (a->user1 != lastTimeCount)
    {
      a->user1 = lastTimeCount;
      a->user2 = 0;
    }

    if (++a->user2 == 11)
    {
      // Turn worm into foot
      SD_PlaySound(SOUND_FOOTAPPEAR);
      a->posY -= 0x500;
      a->type = CT4_Foot;
      CK_SetAction2(a, CK_GetActionByName("CK4_ACT_Foot1"));

      // Spawn Foot poofs
      CK_object *poof = CK_GetNewObj(true);
      poof->posX = a->posX - 0x80;
      poof->posY = a->posY + 0x100;
      poof->zLayer = PRIORITIES - 1;
      CK_SetAction(poof, CK_GetActionByName("CK4_ACT_FootPoof0"));

      poof = CK_GetNewObj(true);
      poof->posX = a->posX + 0x100;
      poof->posY = a->posY + 0x180;
      poof->zLayer = PRIORITIES - 1;
      CK_SetAction(poof, CK_GetActionByName("CK4_ACT_FootPoof0"));

      poof = CK_GetNewObj(true);
      poof->posX = a->posX + 0x280;
      poof->posY = a->posY + 0x100;
      poof->zLayer = PRIORITIES - 1;
      CK_SetAction(poof, CK_GetActionByName("CK4_ACT_FootPoof0"));

      poof = CK_GetNewObj(true);
      poof->posX = a->posX;
      poof->posY = a->posY - 0x80;
      poof->zLayer = PRIORITIES - 1;
      CK_SetAction(poof, CK_GetActionByName("CK4_ACT_FootPoof0"));

      // Remove all other worms on level
      for (CK_object *o = ck_keenObj; o; o = o->next)
      {
        if (o->type == CT4_Inchworm)
          CK_RemoveObj(o);
      }
    }
  }
}

void CK4_FootCol(CK_object *a, CK_object *b)
{
  // Just an empty function :(
}

// Bounder

void CK4_SpawnBounder(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Bounder;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x80;
  obj->yDirection = IN_motion_Down;
  obj->xDirection = IN_motion_None;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Bounder0"));
}

void CK4_BounderCheckShot(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    a->user1 = a->user2 = a->user3 = 0;
    a->user4 = a->type;
    CK_ShotHit(b);
    CK_SetAction2(a, CK_GetActionByName("CK4_ACT_BounderStunned0"));
    a->type = CT4_StunnedCreature;
    a->velY -= 32;
  }
}

void CK4_BounderDraw(CK_object *obj)
{
  if (obj->bottomTI)
    obj->velY = 0;

  if (obj->topTI)
  {
    obj->user2++;
    if (CK_ObjectVisible(obj))
      SD_PlaySound(SOUND_MUSHROOMLEAP);

    obj->velY = -50;

    if (ck_keenState.platform == obj)
    {
      obj->user2 = 0;
      if (ck_keenObj->clipRects.unitX1 < obj->clipRects.unitX1 - 0x40 )
      {
        obj->xDirection = IN_motion_Left;
      }
      else if (ck_keenObj->clipRects.unitX2 > obj->clipRects.unitX2 + 0x40)
      {
        obj->xDirection = IN_motion_Right;
      }
      else
      {
        obj->xDirection = IN_motion_None;
      }
      obj->velX = obj->xDirection * 24;
    }
    else if (obj->user2 > 2 && obj->user1)
    {
      // Change direction every third bounce, and not if just changed direction
      obj->user1 = 0;
      int16_t r = US_RndT();
      if (r < 100)
        obj->xDirection = IN_motion_Left;
      else if (r < 200)
        obj->xDirection = IN_motion_Right;
      else
        obj->xDirection = IN_motion_None;

      obj->velX = obj->xDirection * 24;
    }
    else
    {
      // Bounce in same direction; can potentially change direction on next bounce
      obj->user1 = 1;
      obj->velX = 0;
      obj->xDirection = IN_motion_None;
      CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_Bounder0"));
    }

    if (obj->xDirection != IN_motion_None)
    {
      CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_BounderMoveHorz0"));
    }
    else
    {
      CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_Bounder0"));
    }
  }

  if (obj->leftTI || obj->rightTI)
  {
    obj->xDirection = -obj->xDirection;
    obj->velX = -obj->velX;
  }

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Licks

void CK4_SpawnLick(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Lick;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 2;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  obj->timeUntillThink = US_RndT() / 0x40;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_LickHop2"));
}

void CK4_LickMove(CK_object *obj)
{
  obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;

  int16_t dx = ck_keenObj->posX - obj->posX;
  int16_t dy = ck_keenObj->posY - obj->posY;

  if (dy >= -0x100 && dy <= 0x100)
  {
    if (obj->xDirection == IN_motion_Right && dx > -0x20 && dx < 0x180 ||
        obj->xDirection == IN_motion_Left && dx < 0x20 && dx > -0x200)
    {
      SD_PlaySound(SOUND_LICKFLAME);
      CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_LickFlame0"));
      return;
    }
  }

  if (ABS(dx) > 0x300)
  {
    obj->velX = obj->xDirection * 32;
    obj->velY = -32;
  }
  else
  {
    obj->velX = (obj->xDirection * 32) / 2;
    obj->velY = -16;
  }
}

void CK4_LickCheckShot(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_LickStunned0"));
    a->velY -= 16;
  }
}

void CK4_LickCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    if (a->xDirection == IN_motion_Right && ck_keenObj->posX > a->posX ||
        a->xDirection == IN_motion_Left && ck_keenObj->posX < a->posX)
    {
      CK_KillKeen();
      return;
    }
  }

  CK4_LickCheckShot(a,b);
}

void CK4_LickDraw(CK_object *obj)
{
  if (obj->topTI)
  {
    CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_LickHop3"));
  }

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Platforms (CK4-unique behaviour)

void CK4_SpawnAxisPlatform(int tileX, int tileY, int direction)
{
	CK_object *obj = CK_GetNewObj(false);

  obj->type = CT4_Platform;
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

  CK_SetAction(obj, CK_GetActionByName("CK_ACT_AxisPlatform"));
}

// AxisPlatformMove in ck_obj.c

void CK4_PlatformDraw(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

  // Draw the flames

  int16_t di = (SD_GetLastTimeCount() / 4)&1;

  if (obj->xDirection == IN_motion_Right)
  {
    RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user2), obj->posX - 0x10, obj->posY + 0x30, di + 485, false, PRIORITIES - 4);
    if (obj->user3) {
      RF_RemoveSpriteDraw((RF_SpriteDrawEntry**)&(obj->user3));
    }
  }
  else if (obj->xDirection == IN_motion_Left)
  {
    if (obj->user2) {
      RF_RemoveSpriteDraw((RF_SpriteDrawEntry**)&(obj->user2));
    }
    RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user3), obj->posX + 0x300, obj->posY + 0x50, di + 485, false, PRIORITIES - 3);
  }
  else if (obj->yDirection == IN_motion_Up)
  {
    RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user2), obj->posX + 0x20, obj->posY + 0x90, di + 489, false, PRIORITIES - 4);
    RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user3), obj->posX + 0x2E0, obj->posY + 0x80, di + 487, false, PRIORITIES - 4);
  }
  else if (obj->yDirection == IN_motion_Down)
  {
    if (di)
    {
      RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user2), obj->posX + 0x20, obj->posY + 0x90, di + 489, false, PRIORITIES - 4);
      RF_AddSpriteDraw((RF_SpriteDrawEntry **)&(obj->user3), obj->posX + 0x2E0, obj->posY + 0x80, di + 487, false, PRIORITIES - 4);
    }
    else
    {
      if (obj->user2)
        RF_RemoveSpriteDraw((RF_SpriteDrawEntry**)&(obj->user2));
      if (obj->user3)
        RF_RemoveSpriteDraw((RF_SpriteDrawEntry**)&(obj->user3));
    }
  }
}



/*
 * Setup all of the functions in this file.
 */
void CK4_Obj2_SetupFunctions()
{

  CK_ACT_AddFunction("CK4_WormmouthLookRight", &CK4_WormmouthLookRight);
  CK_ACT_AddFunction("CK4_WormmouthGoToPlayer", &CK4_WormmouthGoToPlayer);
  CK_ACT_AddFunction("CK4_WormmouthLookLeft", &CK4_WormmouthLookLeft);
  CK_ACT_AddFunction("CK4_WormmouthMove", &CK4_WormmouthMove);
  CK_ACT_AddColFunction("CK4_WormmouthCheckShot", &CK4_WormmouthCheckShot);
  CK_ACT_AddColFunction("CK4_WormmouthCol", &CK4_WormmouthCol);

  CK_ACT_AddFunction("CK4_CloudMove", &CK4_CloudMove);
  CK_ACT_AddFunction("CK4_CloudDraw", &CK4_CloudDraw);
  CK_ACT_AddFunction("CK4_CloudCheckStrike", &CK4_CloudCheckStrike);
  CK_ACT_AddFunction("CK4_CloudStrike", &CK4_CloudStrike);
  CK_ACT_AddColFunction("CK4_CloudCol", &CK4_CloudCol);

  CK_ACT_AddFunction("CK4_BerkeloidMove", &CK4_BerkeloidMove);
  CK_ACT_AddFunction("CK4_BerkeloidThrow", &CK4_BerkeloidThrow);
  CK_ACT_AddFunction("CK4_BerkeloidThrowDone", &CK4_BerkeloidThrowDone);
  CK_ACT_AddColFunction("CK4_BerkeloidCol", &CK4_BerkeloidCol);
  CK_ACT_AddFunction("CK4_FireballDraw", &CK4_FireballDraw);
  CK_ACT_AddFunction("CK4_BerkeloidHover", &CK4_BerkeloidHover);
  CK_ACT_AddFunction("CK4_BerkeloidDraw", &CK4_BerkeloidDraw);

  CK_ACT_AddFunction("CK4_InchwormMove", &CK4_InchwormMove);
  CK_ACT_AddColFunction("CK4_InchwormCol", &CK4_InchwormCol);
  CK_ACT_AddColFunction("CK4_FootCol", &CK4_FootCol);

  CK_ACT_AddColFunction("CK4_BounderCheckShot", &CK4_BounderCheckShot);
  CK_ACT_AddFunction("CK4_BounderDraw", &CK4_BounderDraw);

  CK_ACT_AddFunction("CK4_LickMove", &CK4_LickMove);
  CK_ACT_AddColFunction("CK4_LickCheckShot", &CK4_LickCheckShot);
  CK_ACT_AddColFunction("CK4_LickCol", &CK4_LickCol);
  CK_ACT_AddFunction("CK4_LickDraw", &CK4_LickDraw);

  CK_ACT_AddFunction("CK4_PlatformDraw", &CK4_PlatformDraw);
}
