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

void CK4_SpawnSmirky(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Smirky;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 1;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x180;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Smirky0"));
}

void CK4_SmirkySearch(CK_object *obj)
{
  obj->currentAction = CK_GetActionByName("CK4_ACT_SmirkyJump0");

  // Jump for item goodies
  for (CK_object *o = ck_keenObj; o; o = o->next)
  {
    if (o->type == CT4_Item)
    {
      if (o->active == OBJ_ACTIVE &&
          o->clipRects.unitX2 > obj->clipRects.unitX1 &&
          o->clipRects.unitX1 < obj->clipRects.unitX2 &&
          o->clipRects.unitY2 < obj->clipRects.unitY1 &&
          o->clipRects.unitY2 > obj->clipRects.unitY1 + 0x300)
      {
         obj->velX = 0;
         obj->velY = -48;
         return;
      }
    }
  }

  // Jump for tile goodies
  // DOS Keen increments a far pointer to the foreground plane, but it is
  // semantically equivalent to calculate the offset on every iteration
  for (int y = obj->clipRects.tileY1 - 3; y < obj->clipRects.tileY1; y++)
  {
    for (int x = obj->clipRects.tileX1; x < obj->clipRects.tileX2 + 1; x++)
    {
      int mf = TI_ForeMisc(CA_TileAtPos(x, y, 1)) & 0x7F;
      if (mf == MF_Centilife || mf >= MF_Points100 && mf <= MF_Stunner)
      {
         obj->velX = 0;
         obj->velY = -48;
         return;
      }
    }
  }

  // Decide to teleport for some reason
  if (obj->user1 >= 2)
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_SmirkyTele0");
    return;
  }

  // Decide to hop to the side if there's something to land on
  for (int i = 0; i < 4; i++)
  {
    int y = obj->clipRects.tileY2 - 2 + i;
    int x = obj->clipRects.tileXmid + obj->xDirection * 4;
    int tf = TI_ForeTop(CA_TileAtPos(x, y, 1));
    if (tf)
    {
      obj->velX = obj->xDirection * 20;
      obj->velY = -24;
      return;
    }
  }

  // Decide to teleport if counter hits threshold
  if (++obj->user1 == 2)
  {
    SD_PlaySound(SOUND_SMIRKYTELE);
    obj->currentAction = CK_GetActionByName("CK4_ACT_SmirkyTele0");
    return;
  }

  // Finally, just turn around and hop back where we came from
  obj->velX = -obj->xDirection * 20;
  obj->velY = -24;
}

void CK4_SmirkyTeleport(CK_object *obj)
{
  obj->user1 = 0;  // reset tleport timer
  CK_object *o;
  for (o = ck_keenObj; o; o = o->next)
  {
    if (o->type == CT4_Item)
    {
      obj->posX = o->posX - 0x80;
      obj->posY = o->posY;
      CK_SetAction(obj, CK_GetActionByName("CK4_ACT_SmirkyTele0"));
    }
  }

  // Remove smirky if all the treasure items are gone
  if (!o)
  {
    CK_RemoveObj(obj);
    return;
  }
}

void CK4_SmirkyCol(CK_object *a, CK_object *b)
{
  if (b->type == CT4_Item)
  {
    b->type = CT_Friendly;
    b->zLayer = PRIORITIES - 1;
    CK_SetAction2(b, CK_GetActionByName("CK4_ACT_StolenItem0"));
    SD_PlaySound(SOUND_SMIRKYSTEAL);
  }
  else if (b->type == CT_Stunner)
  {
    a->user1 = a->user2 = a->user3 = 0;
    a->user4 = a->type;
    a->type = CT4_StunnedCreature;
    CK_ShotHit(b);
    CK_SetAction2(a, CK_GetActionByName("CK4_ACT_SmirkyStunned0"));
    a->velY -= 16;
  }
}

void CK4_SmirkyCheckTiles(CK_object *obj)
{
  for (int y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; y++)
  {
    for (int x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; x++)
    {
      int mf = TI_ForeMisc(CA_TileAtPos(x, y, 1)) & 0x7F;
      if (mf == MF_Centilife || mf >= MF_Points100 && mf <= MF_Stunner)
      {
        static uint16_t emptyTile = 0;  // actually this is the same address in memory as the
        // lifewater drop emptytile... suggesting that it's a global var
        RF_ReplaceTiles(&emptyTile, 1, x, y, 1, 1);

        CK_object *o = CK_GetNewObj(true);
        o->type = CT_Friendly;
        o->zLayer = PRIORITIES - 1;
        o->clipped = CLIP_not;
        o->posX = x << G_T_SHIFT;
        o->posY = y << G_T_SHIFT;
        o->active = OBJ_EXISTS_ONLY_ONSCREEN;
        CK_SetAction2(o, CK_GetActionByName("CK4_ACT_StolenItem0"));
      }
    }
  }
}

void CK4_SmirkyDraw(CK_object *obj)
{
  CK4_SmirkyCheckTiles(obj);

  if (obj->topTI)
    CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_Smirky0"));

  if (obj->leftTI || obj->rightTI)
  {
    obj->user1++;
    obj->xDirection = -obj->xDirection;
    obj->velX = 0;
  }

  if (obj->topTI)
    obj->velY = 0;

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Setup all of the functions in this file.
 */
void CK4_Obj3_SetupFunctions()
{

  CK_ACT_AddFunction("CK4_SmirkySearch", &CK4_SmirkySearch);
  CK_ACT_AddFunction("CK4_SmirkyTeleport", &CK4_SmirkyTeleport);
  CK_ACT_AddColFunction("CK4_SmirkyCol", &CK4_SmirkyCol);
  CK_ACT_AddFunction("CK4_SmirkyDraw", &CK4_SmirkyDraw);
}
