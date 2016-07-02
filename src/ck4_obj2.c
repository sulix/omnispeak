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
}
