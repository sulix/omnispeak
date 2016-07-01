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

// Miragia

void CK4_SpawnMiragia(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT_Friendly;
  obj->active = OBJ_ALWAYS_ACTIVE;
  obj->posX = tileX;
  obj->posY = tileY;
  obj->currentAction = CK_GetActionByName("CK4_ACT_Miragia0");
}

void CK4_Miragia0(CK_object *obj)
{
  if ((ck_keenObj->clipRects.tileX2 >= obj->posX) &&
      (ck_keenObj->clipRects.tileX1 <= obj->posX + 5) &&
      (ck_keenObj->clipRects.tileY1 >= obj->posY) &&
      (ck_keenObj->clipRects.tileY2 <= obj->posY + 4))
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_Miragia7");
  }
  else
  {
    RF_ReplaceTileBlock(8, 60, obj->posX, obj->posY, 6, 4);
  }
}

void CK4_Miragia1(CK_object *obj) { RF_ReplaceTileBlock(14, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia2(CK_object *obj) { RF_ReplaceTileBlock(20, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia3(CK_object *obj) { RF_ReplaceTileBlock(26, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia4(CK_object *obj) { RF_ReplaceTileBlock(20, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia5(CK_object *obj) { RF_ReplaceTileBlock(14, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia6(CK_object *obj) { RF_ReplaceTileBlock(8, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia7(CK_object *obj) { RF_ReplaceTileBlock(2, 60, obj->posX, obj->posY, 6, 4); }

void CK4_SpawnCouncilMember(int tileX, int tileY)
{
   CK_object *obj = CK_GetNewObj(false);
   obj->type = CT4_CouncilMember;
   obj->active = OBJ_ACTIVE;
   obj->zLayer = PRIORITIES - 4;
   obj->posX = tileX << G_T_SHIFT;
   obj->posY = (tileY << G_T_SHIFT) - 369;
   obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
   obj->yDirection = IN_motion_Down;
   CK_SetAction(obj, CK_GetActionByName("CK4_ACT_CouncilWalk0"));
}

void CK4_CouncilWalk(CK_object *obj)
{
  if (SD_GetSpriteSync() << 3 > US_RndT())
    obj->currentAction = CK_GetActionByName("CK4_ACT_CouncilPause");
}


/*
 * Setup all of the functions in this file.
 */
void CK4_Obj1_SetupFunctions()
{
  CK_ACT_AddFunction("CK4_Miragia0", &CK4_Miragia0);
  CK_ACT_AddFunction("CK4_Miragia1", &CK4_Miragia1);
  CK_ACT_AddFunction("CK4_Miragia2", &CK4_Miragia2);
  CK_ACT_AddFunction("CK4_Miragia3", &CK4_Miragia3);
  CK_ACT_AddFunction("CK4_Miragia4", &CK4_Miragia4);
  CK_ACT_AddFunction("CK4_Miragia5", &CK4_Miragia5);
  CK_ACT_AddFunction("CK4_Miragia6", &CK4_Miragia6);
  CK_ACT_AddFunction("CK4_Miragia7", &CK4_Miragia7);

  CK_ACT_AddFunction("CK4_CouncilWalk", &CK4_CouncilWalk);
}
