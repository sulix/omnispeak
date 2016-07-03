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


void CK4_Map_SetupFunctions()
{
  CK_ACT_AddFunction("CK4_MapKeenFoot", &CK4_MapKeenFoot);
}
