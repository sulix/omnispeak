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

  CK_ACT_AddFunction("CK6_FlectTurn", &CK6_FlectTurn);
  CK_ACT_AddFunction("CK6_FlectWalk", &CK6_FlectWalk);
  CK_ACT_AddColFunction("CK6_FlectCol", &CK6_FlectCol);
  CK_ACT_AddFunction("CK6_FlectDraw", &CK6_FlectDraw);
}
