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

// Orbatrices
void CK6_SpawnOrbatrix(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Orbatrix;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x180;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  obj->user4 = 1;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_OrbatrixFloat0"));
}

void CK6_OrbatrixFloat(CK_object *obj)
{
  if (US_RndT() < 0x20)
  {
    obj->currentAction = CK_GetActionByName("CK6_ACT_OrbatrixUncurl2");
  }
  else if (obj->clipRects.unitY2 != ck_keenObj->clipRects.unitY2)
  {
    int dx = ck_keenObj->posX - obj->posX;
    obj->xDirection = dx < 0 ? IN_motion_Left : IN_motion_Right;
    if (dx > - 0x500 && dx < 0x500)
      obj->currentAction = CK_GetActionByName("CK6_ACT_OrbatrixCurl0");
  }
}

void CK6_OrbatrixCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    CK_ShotHit(b);
    CK_SetAction2(a, CK_GetActionByName("CK6_ACT_OrbatrixUncurl2"));
  }
}

void CK6_OrbatrixDraw(CK_object *obj)
{
  obj->posY -= obj->user3;
  CK_BasicDrawFunc2(obj);
  obj->posY += obj->user3;

  obj->user3 += obj->user4 * SD_GetSpriteSync() * 4;

  if (obj->user3 > 0x80)
  {
    obj->user3 = 0x80;
    obj->user4 = -1;
  }
  else if (obj->user3 < -0x80)
  {
    obj->user3 = -0x80;
    obj->user4 = 1;
  }
}

#define SOUND_ORBATRIXBOUNCE 0x26
void CK6_OrbatrixBounceDraw(CK_object *obj)
{
  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

  if (obj->topTI)
    obj->velY = -obj->velY;

  if (obj->topTI || obj->leftTI || obj->rightTI)
  {
    obj->velX = -obj->velX;
    SD_PlaySound(SOUND_ORBATRIXBOUNCE);

    if (obj->topTI && --obj->user1 == 0)
    {
      CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_OrbatrixUncurl0"));
      obj->user2 = 0x180;
    }
  }
}

void CK6_OrbatrixCurl(CK_object *obj)
{
  if (obj->user3 >= 0x10)
  {
    obj->velX = obj->xDirection * 60;
    obj->velY = -32;
    obj->posY -= obj->user3;
    obj->user1 = 5;
    obj->currentAction = obj->currentAction->next;
  }

  obj->visible = true;
}

void CK6_OrbatrixUncurlThink(CK_object *obj)
{
  ck_nextY = SD_GetSpriteSync() * -8;
  obj->user2 += ck_nextY;
  if (obj->user2 <= 0)
  {
    ck_nextY -= obj->user2;
    obj->currentAction = obj->currentAction->next;
  }
}

void CK6_OrbatrixCol2(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_KillKeen();
  }
  else if (b->type == CT_Stunner)
  {
    CK_ShotHit(b);
    a->velX = 0;
  }
}

// Bips

void CK6_BipWalk(CK_object *obj)
{
  if (obj->clipRects.unitY2 == ck_keenObj->clipRects.unitY2)
  {
    if (ck_keenObj->clipRects.unitX1 - 0x40 < obj->clipRects.unitX2)
      obj->xDirection = IN_motion_Right;

    if (ck_keenObj->clipRects.unitX2 + 0x40 > obj->clipRects.unitX1)
      obj->xDirection = IN_motion_Left;
  }
  else
  {
    if (US_RndT() < 0x10)
    {
      obj->xDirection = - obj->xDirection;
      obj->currentAction = CK_GetActionByName("CK6_ACT_BipStand0");
    }
  }
}

#define SOUND_BIPSQUISH 22
void CK6_BipCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player && b->deltaPosY == 0)
  {
    SD_PlaySound(SOUND_BIPSQUISH);
    a->type = CT_Friendly;
    CK_SetAction2(a, CK_GetActionByName("CK6_ACT_BipSquished0"));
  }
}

void CK6_SpawnBipship(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Bipship;
  obj->active = OBJ_ACTIVE;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x180;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->velX = obj->xDirection * 20;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BipshipFly0"));
}

void CK6_BipShotDraw(CK_object *obj)
{
  if (obj->topTI || obj->bottomTI || obj->rightTI || obj->leftTI)
    CK_RemoveObj(obj);
  else
    RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_BipshipTurn(CK_object *obj)
{
  CK_PhysAccelHorz(obj, obj->xDirection, 20);
}

void CK6_BipshipFly(CK_object *obj)
{
  CK_PhysAccelHorz(obj, obj->xDirection, 20);
  int xdir = obj->xDirection;
  int ycheck = ck_keenObj->clipRects.unitY2 + 0x100 - obj->clipRects.unitY2;
  if (ycheck <= 0x200 && ycheck >= 0)
  {
    // Fire!!
    xdir = (ck_keenObj->posX < obj->posX) ? IN_motion_Left : IN_motion_Right;
    if (obj->xDirection == xdir && US_RndT() < SD_GetSpriteSync() * 2)
    {
      SD_PlaySound(SOUND_KEENSHOOT);
      CK_object *shot = CK_GetNewObj(true);
      shot->type = CT6_EnemyShot;
      shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
      shot->zLayer = PRIORITIES - 3;
      if (obj->xDirection == IN_motion_Right)
      {
        shot->posX = obj->posX + 0x100;
        shot->velX = 64;
      }
      else
      {
        shot->posX = obj->posX;
        shot->velX = -64;
      }

      shot->posY = obj->posY + 0xA0;
      shot->velY = 16;
      CK_SetAction(shot, CK_GetActionByName("CK6_ACT_BipShot0"));
    }
  }

  int startx = obj->clipRects.tileXmid + 2 * xdir;
  int y = obj->clipRects.tileY1;
  uint16_t *tile = CA_TilePtrAtPos(startx, y, 1);

  for (y; y <= obj->clipRects.tileY2; y++, tile+=CA_GetMapWidth())
  {
    if (TI_ForeLeft(*tile) || TI_ForeRight(*tile))
    {
      xdir = -xdir;
      goto checkTurn;
    }
  }

  // And turn around at ledge-ends
  if (!TI_ForeTop(*tile))
    xdir = -xdir;

checkTurn:

  if (obj->xDirection != xdir)
  {
    obj->xDirection = xdir;
    CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_BipshipTurn0"));
  }
}

#define SOUND_BIPSHIPCRASH 24
void CK6_BipshipCrash(CK_object *obj)
{
  SD_PlaySound(SOUND_BIPSHIPCRASH);

  // Spawn smoke
  CK_object *smoke = CK_GetNewObj(true);
  smoke->type = CT_Friendly;
  smoke->active = OBJ_ACTIVE;
  smoke->zLayer = PRIORITIES - 2;
  smoke->posX = obj->posX;
  smoke->posY = obj->posY - 0x180;
  CK_SetAction(smoke, CK_GetActionByName("CK6_ACT_BipshipSmoke0"));
  smoke->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;

  CK_object *bip = CK_GetNewObj(true);
  bip->type = CT6_Bip;
  bip->active = OBJ_ACTIVE;
  bip->zLayer = PRIORITIES - 4;
  bip->posX = obj->posX;
  bip->posY = obj->posY - 0x80;
  bip->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  CK_SetAction(bip, CK_GetActionByName("CK6_ACT_BipStand0"));

}

void CK6_BipshipCol(CK_object  *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    CK_ShotHit(b);
    CK_SetAction2(a, CK_GetActionByName("CK6_ACT_BipshipHit0"));
  }
}


// Flects

#define SOUND_FLECTSHOT 0x2A
void CK6_SpawnFlect(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Flect;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x100;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_FlectWalk0"));
}

void CK6_FlectTurn(CK_object *obj)
{
  if (ck_keenObj->posX < obj->posX)
  {
    if (obj->xDirection == IN_motion_Left)
    {
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectWalk0");
    }
    else
    {
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");
      obj->xDirection = IN_motion_Left;
    }
  }
  else
  {
    if (obj->xDirection == IN_motion_Right)
    {
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectWalk0");
    }
    else
    {
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");
      obj->xDirection = IN_motion_Right;
    }
  }
}

void CK6_FlectWalk(CK_object *obj)
{
  if (ck_keenObj->posX < obj->posX && obj->xDirection == IN_motion_Right)
  {
    if (obj->xDirection == IN_motion_Left)
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");

    obj->xDirection = IN_motion_Left;
  }

  if (ck_keenObj->posX > obj->posX && obj->xDirection == IN_motion_Left)
  {
    if (obj->xDirection == IN_motion_Right)
      obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");

    obj->xDirection = IN_motion_Right;
  }

  if (US_RndT() < 0x20)
    obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn0");
}

void CK6_FlectCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_PhysPushX(b, a);
  }
  else if (b->type == CT_Stunner)
  {
    if (b->xDirection == IN_motion_None)
    {
      CK_StunCreature(a,b,CK_GetActionByName("CK6_ACT_FlectStunned0"));
    }
    else if (a->xDirection != b->xDirection)
    {
      b->xDirection = a->xDirection;
      b->user4 = 1; // Shot is hazardous to keen now
      SD_PlaySound(SOUND_FLECTSHOT);
    }
  }
}

void CK6_FlectDraw(CK_object *obj)
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
		CK_SetAction2(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Setup all of the functions in this file.
 */
void CK6_Obj2_SetupFunctions()
{

  CK_ACT_AddFunction("CK6_OrbatrixFloat", &CK6_OrbatrixFloat);
  CK_ACT_AddColFunction("CK6_OrbatrixCol", &CK6_OrbatrixCol);
  CK_ACT_AddFunction("CK6_OrbatrixDraw", &CK6_OrbatrixDraw);
  CK_ACT_AddFunction("CK6_OrbatrixBounceDraw", &CK6_OrbatrixBounceDraw);
  CK_ACT_AddFunction("CK6_OrbatrixCurl", &CK6_OrbatrixCurl);
  CK_ACT_AddFunction("CK6_OrbatrixUncurlThink", &CK6_OrbatrixUncurlThink);
  CK_ACT_AddColFunction("CK6_OrbatrixCol2", &CK6_OrbatrixCol2);

  CK_ACT_AddFunction("CK6_BipWalk", &CK6_BipWalk);
  CK_ACT_AddColFunction("CK6_BipCol", &CK6_BipCol);
  CK_ACT_AddFunction("CK6_BipShotDraw", &CK6_BipShotDraw);
  CK_ACT_AddFunction("CK6_BipshipTurn", &CK6_BipshipTurn);
  CK_ACT_AddFunction("CK6_BipshipFly", &CK6_BipshipFly);
  CK_ACT_AddFunction("CK6_BipshipCrash", &CK6_BipshipCrash);
  CK_ACT_AddColFunction("CK6_BipshipCol", &CK6_BipshipCol);

  CK_ACT_AddFunction("CK6_FlectTurn", &CK6_FlectTurn);
  CK_ACT_AddFunction("CK6_FlectWalk", &CK6_FlectWalk);
  CK_ACT_AddColFunction("CK6_FlectCol", &CK6_FlectCol);
  CK_ACT_AddFunction("CK6_FlectDraw", &CK6_FlectDraw);
}
