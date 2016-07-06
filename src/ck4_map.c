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

#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_rf.h"
#include "id_sd.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_act.h"
#include "ck4_ep.h"

#include <string.h>
#include <stdio.h>

// =========================================================================

void CK4_MapKeenFoot(CK_object *obj)
{
  int spriteSync = SD_GetSpriteSync();
  obj->user1 -= spriteSync;
  ck_nextX = spriteSync * obj->velX;
  ck_nextY = spriteSync * obj->velY;
  if (obj->user1 <= 0)
  {
    ck_nextX -= obj->velX * -obj->user1;
    ck_nextY -= obj->velY * -obj->user1;
    obj->zLayer = PRIORITIES - 3;
    obj->user1 = 6;
    obj->user2 = 3;
    obj->user3 = 0;
    ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;  // use global pointer for some reason
    obj->currentAction = CK_GetActionByName("CK_ACT_MapKeenStart");
    obj->gfxChunk = 256;
    obj->clipped = CLIP_normal;
  }
}

static int ck4_swimFrames[] = {280, 288, 282, 290, 284, 292, 286, 294};
void CK4_MapKeenSwim(CK_object *obj)
{
  if (obj->user3)
  {
    if ((obj->user3 -= 6) < 0)
      obj->user3 = 0;
  }
  else
  {
    obj->xDirection = ck_inputFrame.xDirection;
    obj->yDirection = ck_inputFrame.yDirection;

    if (ck_inputFrame.xDirection || ck_inputFrame.yDirection)
      obj->user1 = ck_inputFrame.dir;
  }

  obj->gfxChunk = ck4_swimFrames[obj->user1] + obj->user2;

  if (++obj->user2 == 2)
    obj->user2 = 0;

  SD_PlaySound(obj->user2 ? SOUND_KEENSWIMHI : SOUND_KEENSWIMLO);
}

static int ck4_wetsuitOffs[] = {4, 6, 0, 2};
static int ck4_wetsuitOffs2[] = { 4 , 5 ,6, 7, 0, 1, 2, 3 };
void CK4_MapMiscFlagsCheck(CK_object *obj)
{
  if (!obj->user3)
  {
    int midX = obj->clipRects.tileXmid;
    int midY = (obj->clipRects.unitY1 + (obj->clipRects.unitY2 - obj->clipRects.unitY1) / 2) >> G_T_SHIFT;
    int mf = TI_ForeMisc(CA_TileAtPos(midX, midY, 1));

    if (mf == MF_WaterN || mf == MF_WaterE || mf == MF_WaterS || mf == MF_WaterW)
    {
      if (!ck_gameState.ep.ck4.wetsuit)
      {
        SD_PlaySound(SOUND_NEEDKEYCARD);
        CK4_ShowCantSwimMessage();
        // RF_ForceRefresh();
        ck_nextX = -obj->deltaPosX;
        ck_nextY = -obj->deltaPosY;
        obj->xDirection = obj->yDirection = 0;
        CK_PhysUpdateNormalObj(obj);
      }
      else
      {
        obj->user1 = ck4_wetsuitOffs[mf-MF_WaterN];

        if (obj->currentAction == CK_GetActionByName("CK4_ACT_MapKeenSwim0"))
          obj->user1 = ck4_wetsuitOffs2[obj->user1];

        switch (obj->user1)
        {
          case 0:
            obj->xDirection = 0;
            obj->yDirection = -1;
            break;
          case 2:
            obj->xDirection = 1;
            obj->yDirection = 0;
            break;
          case 4:
            obj->xDirection = 0;
            obj->yDirection = 1;
            break;
          case 6:
            obj->xDirection = -1;
            obj->yDirection = 0;
            break;
        }

        obj->user2 = 0;
        obj->user3 = 18;

        if (obj->currentAction == CK_GetActionByName("CK4_ACT_MapKeenSwim0"))
          CK_SetAction2(obj, CK_GetActionByName("CK_ACT_MapKeenStart"));
        else
          CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_MapKeenSwim0"));
      }
    }
  }
}


void CK4_Map_SetupFunctions()
{
  CK_ACT_AddFunction("CK4_MapKeenFoot", &CK4_MapKeenFoot);
  CK_ACT_AddFunction("CK4_MapKeenSwim", &CK4_MapKeenSwim);
}
