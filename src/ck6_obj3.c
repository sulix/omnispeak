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

/*
 * Setup all of the functions in this file.
 */

void CK6_SpawnCeilick(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT6_Ceilick;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->clipped = CLIP_not;
  obj->posX = tileX << G_T_SHIFT;
  obj->user1 = obj->posY = tileY << G_T_SHIFT;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK6_ACT_CeilickWait0"));
}

void CK6_Ceilick(CK_object *obj)
{
  if (ck_keenObj->posY - obj->posY < 0x280 && // <- relies on unsignedness of posY
      ck_keenObj->posY - obj->posY >= 0 && // this line not in dos version
      obj->clipRects.unitX2 + 0x10 > ck_keenObj->clipRects.unitX1 &&
      obj->clipRects.unitX1 - 0x10 < ck_keenObj->clipRects.unitX2)
  {
    SD_PlaySound(0x33);
    obj->currentAction = CK_GetActionByName("CK6_ACT_CeilickStrike00");

  }
}

void CK6_CeilickDescend(CK_object *obj)
{
  SD_PlaySound(0x37);
}

void CK6_CeilickStunned(CK_object *obj)
{
  obj->visible = true;
}

void CK6_CeilickCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    a->posY = a->user1;
    CK_ShotHit(b);
    a->user1 = a->user2 = a->user3 = 0;
    a->user4 = a->type;
    CK_SetAction2(a, CK_GetActionByName("CK6_ACT_CeilickStunned0"));
    a->type = CT6_StunnedCreature;
  }
}

void CK6_Obj3_SetupFunctions()
{

  CK_ACT_AddFunction("CK6_Ceilick", &CK6_Ceilick);
  CK_ACT_AddFunction("CK6_CeilickDescend", &CK6_CeilickDescend);
  CK_ACT_AddFunction("CK6_CeilickStunned", &CK6_CeilickStunned);
  CK_ACT_AddColFunction("CK6_CeilickCol", &CK6_CeilickCol);

}
