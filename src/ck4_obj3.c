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


// Mimrocks


void CK4_SpawnMimrock(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Mimrock;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 1;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0xD0;
  obj->xDirection = IN_motion_Right;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Mimrock0"));
}

void CK4_MimrockWait(CK_object *obj)
{
  if (ABS((int16_t)(obj->clipRects.unitY2 - ck_keenObj->clipRects.unitY2)) <= 0x500)
  {
    if (ABS((int16_t)(obj->posX - ck_keenObj->posX)) > 0x300)
    {
      if (ck_keenObj->posX < obj->posX)
      {
        if (ck_keenObj->xDirection == IN_motion_Left)
        {
          obj->xDirection = IN_motion_Left;
          obj->currentAction = CK_GetActionByName("CK4_ACT_MimrockWalk0");
        }
      }
      else
      {
        if (ck_keenObj->xDirection == IN_motion_Right)
        {
          obj->xDirection = IN_motion_Right;
          obj->currentAction = CK_GetActionByName("CK4_ACT_MimrockWalk0");
        }
      }
    }
  }
}

void CK4_MimrockCheckJump(CK_object *obj)
{
  if ((ABS((int16_t)(obj->clipRects.unitY2 - ck_keenObj->clipRects.unitY2)) <= 0x500) &&
      (obj->xDirection == ck_keenObj->xDirection))
  {
    if (ABS((int16_t)(obj->posX - ck_keenObj->posX)) < 0x400)
    {
      obj->velX = obj->xDirection * 20;
      obj->velY = -40;
      ck_nextY = obj->velY * SD_GetSpriteSync();
      obj->currentAction = CK_GetActionByName("CK4_ACT_MimrockJump0");
    }
  }
  else
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_Mimrock0");
  }
}

void CK4_MimrockCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Stunner)
  {
    a->user1 = a->user2 = a->user3 = 0;
    a->user4 = a->type;
    a->type = CT4_StunnedCreature;
    CK_ShotHit(b);
    CK_SetAction2(a, CK_GetActionByName("CK4_ACT_MimrockStunned0"));
    a->velY -= 16;
  }
}

void CK4_MimrockAirCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_KillKeen();
  }
  else
  {
    CK4_MimrockCol(a,b);
  }
}

void CK4_MimrockJumpDraw(CK_object *obj)
{
  if (obj->topTI)
  {
    SD_PlaySound(SOUND_MIMROCKJUMP);
    obj->velY = -20;
    CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_MimrockBounce0"));
  }

  if (obj->leftTI || obj->topTI)
    obj->velX = 0;

  if (obj->topTI || obj->bottomTI)
    obj->velY = 0;

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_MimrockBounceDraw(CK_object *obj)
{
  if (obj->topTI)
  {
    SD_PlaySound(SOUND_MIMROCKJUMP);
    CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_Mimrock0"));
  }

  if (obj->leftTI || obj->topTI)
    obj->velX = 0;

  if (obj->topTI)
    obj->velY = 0;

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Dopefish


void CK4_SpawnDopefish(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Dopefish;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 2;
  obj->clipped = CLIP_simple;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x300;
  obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_DopefishSwim0"));
}

void CK4_KillKeenUnderwater(CK_object *obj)
{
  ck_gameState.levelState = LS_Died;
}

void CK4_DopefishMove(CK_object *obj)
{
  if (!obj->user1)
    obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;

  CK_PhysAccelHorz2(obj, obj->xDirection, 10);

  if (obj->posY < ck_keenObj->posY)
    CK_PhysAccelVert1(obj, 1, 10);
  else
    CK_PhysAccelVert1(obj, -1, 10);
}

void CK4_DopefishEat(CK_object *obj)
{
  CK_object *si = obj;

  si = (CK_object *)obj->user4;
  int16_t dy = si->posY - 0x100 - obj->posY;
  int16_t dx;
  if (obj->xDirection == IN_motion_Right)
    dx = (si->clipRects.unitX2 + 0x20) - obj->clipRects.unitX2;
  else
    dx = (si->clipRects.unitX1 - 0x20) - obj->clipRects.unitX1;

  if (dx < 0)
  {
    ck_nextX = SD_GetSpriteSync() * -32;
    if (ck_nextX < dx)
      ck_nextX = dx;
  }
  else
  {
    ck_nextX = SD_GetSpriteSync() * 32;
    if (ck_nextX > dx)
      ck_nextX = dx;
  }

  if (dy < 0)
  {
    ck_nextY = SD_GetSpriteSync() * -32;
    if (ck_nextY < dy)
      ck_nextY = dy;
  }
  else
  {
    ck_nextY = SD_GetSpriteSync() * 32;
    if (ck_nextY > dy)
      ck_nextY = dy;
  }

  if (ck_nextX == dx && ck_nextY == dy)
  {
    if (si == ck_keenObj)
      CK_SetAction2(si, CK_GetActionByName("CK4_ACT_KeenEaten1"));
    else if (si->currentAction->next)
      CK_SetAction2(si, si->currentAction->next);
    else
      CK_RemoveObj(si);

    obj->currentAction = CK_GetActionByName("CK4_ACT_DopefishBurp0");
  }

}

void CK4_DopefishAfterBurp(CK_object *obj)
{
  // Move back to where the eating sequence started
  // so that the dopefish doesn't get caught in a wall
  int16_t dy = obj->user3 - obj->posY;
  int16_t dx = obj->user2 - obj->posX;

  if (dx < 0)
  {
    ck_nextX = SD_GetSpriteSync() * -32;
    if (ck_nextX < dx)
      ck_nextX = dx;
  }
  else
  {
    ck_nextX = SD_GetSpriteSync() * 32;
    if (ck_nextX > dx)
      ck_nextX = dx;
  }

  if (dy < 0)
  {
    ck_nextY = SD_GetSpriteSync() * -32;
    if (ck_nextY < dy)
      ck_nextY = dy;
  }
  else
  {
    ck_nextY = SD_GetSpriteSync() * 32;
    if (ck_nextY > dy)
      ck_nextY = dy;
  }

  if (ck_nextX == dx && ck_nextY == dy)
  {
    obj->currentAction = obj->currentAction->next;
    obj->clipped = CLIP_simple;
  }
}

void CK4_DopefishBurp(CK_object *obj)
{
  // Spawn a bubble
  CK_object *bubble = CK_GetNewObj(true);
  bubble->posX = obj->posX + 0x380;
  bubble->posY = obj->posY + 0x200;
  bubble->type = CT_Friendly;
  bubble->zLayer = PRIORITIES - 1;
  bubble->active = OBJ_EXISTS_ONLY_ONSCREEN;
  bubble->clipped = CLIP_not;
  bubble->velY = -20;
  bubble->velX = 4;
  CK_SetAction(bubble, CK_GetActionByName("CK4_ACT_DopeBubble0"));
  SD_PlaySound(SOUND_DOPEFISHBURP);
}

void CK4_Bubbles(CK_object *obj)
{
  CK_Glide(obj);
  if (US_RndT() < SD_GetSpriteSync() * 16)
    obj->velX = -obj->velX;

  if (obj->posY < 0x300)
    CK_RemoveObj(obj);
}

void CK4_DopefishCol(CK_object *a, CK_object *b)
{
  if (b->type == CT4_Schoolfish)
  {
    CK_SetAction2(b, CK_GetActionByName("CK4_ACT_SchoolfishEaten0"));
  }
  else if (b->type == CT_Player && !ck_godMode)
  {
    b->type = CT_Friendly;
    b->clipped = CLIP_not;
    SD_PlaySound(SOUND_KEENDIE);
    CK_SetAction2(b, CK_GetActionByName("CK4_ACT_KeenEaten0"));
  }
  else
  {
   return;
  }

  a->user2 = a->posX;
  a->user3 = a->posY;
  a->user4 = (intptr_t)b;
  a->xDirection = b->clipRects.unitXmid < a->clipRects.unitXmid ? IN_motion_Left : IN_motion_Right;
  CK_SetAction2(a, CK_GetActionByName("CK4_ACT_DopefishEat0"));
  a->clipped = CLIP_not;
}

void CK4_FishDraw(CK_object *obj)
{
  if (obj->bottomTI || obj->topTI)
    if (!obj->user1)
      obj->user1++;

  if (obj->rightTI || obj->leftTI)
  {
    obj->velX = 0;
    obj->xDirection = -obj->xDirection;
    obj->user1 = 1;
  }

  if (!obj->bottomTI && !obj->topTI)
    obj->user1 = 0;

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_SpawnSchoolfish(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Schoolfish;
  obj->active = OBJ_ACTIVE;
  obj->clipped = CLIP_simple;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->xDirection = IN_motion_Right;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_SchoolfishSwim0"));
}

void CK4_SchoolfishMove(CK_object *obj)
{
  if (!obj->user1)
    obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;

  CK_PhysAccelHorz2(obj, obj->xDirection, 10);

  if (obj->posY < ck_keenObj->posY)
    CK_PhysAccelVert1(obj, 1, 10);
  else
    CK_PhysAccelVert1(obj, -1, 10);
}

// Sprites - the underwater kind

void CK4_SpawnSprite(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Sprite;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->clipped = CLIP_not;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->user1 = obj->posY;
  obj->xDirection = IN_motion_Right;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Sprite0"));
}

void CK4_SpritePatrol(CK_object *obj)
{
  CK_PhysAccelVert1(obj, obj->yDirection, 8);
  if (obj->user1 - obj->posY > 0x20)
    obj->yDirection = IN_motion_Down;

  if (obj->posY - obj->user1 > 0x20)
    obj->yDirection = IN_motion_Up;

  if (ck_keenObj->clipRects.unitY1 < obj->clipRects.unitY2 &&
      ck_keenObj->clipRects.unitY2 > obj->clipRects.unitY1)
  {

    obj->xDirection = ck_keenObj->posX < obj->posX ? IN_motion_Left : IN_motion_Right;
    obj->currentAction = CK_GetActionByName("CK4_ACT_SpriteAiming0");
  }
}

void CK4_SpriteAim(CK_object *obj)
{
  if (ck_keenObj->clipRects.unitY1 < obj->clipRects.unitY2 &&
      ck_keenObj->clipRects.unitY2 > obj->clipRects.unitY1)
  {
    obj->currentAction = CK_GetActionByName("CK4_ACT_SpriteShooting0");
  }
}

void CK4_SpriteShoot(CK_object *obj)
{
  CK_object *shot = CK_GetNewObj(true);
  shot->posX = obj->posX;
  shot->posY = obj->posY + 0x80;
  shot->zLayer = PRIORITIES - 4;
  shot->type = CT4_Sprite;
  shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
  SD_PlaySound(SOUND_KEENSHOOT);
  shot->xDirection = obj->xDirection;
  CK_SetAction(shot, CK_GetActionByName("CK4_ACT_SpriteBullet0"));
  SD_PlaySound(SOUND_SPRITESHOOT);
}

void CK4_ProjectileDraw(CK_object *obj)
{
  if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
  {
    CK_RemoveObj(obj);
    return;
  }

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_SpawnMine(int tileX, int tileY, int direction)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Mine;
  obj->active = OBJ_ALWAYS_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;

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

  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Mine0"));
}

void CK4_MineCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player)
  {
    CK_SetAction2(a, CK_GetActionByName("CK4_ACT_MineExplode0"));
    SD_PlaySound(SOUND_CK4MINEEXPLODE);
    CK_KillKeen();
  }
}

// Lindsey

void CK4_SpawnLindsey(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Lindsey;
  obj->active = OBJ_ACTIVE;
  obj->zLayer = PRIORITIES - 4;
  obj->posX = tileX << G_T_SHIFT;
  obj->user1 = obj->posY = (tileY << G_T_SHIFT) - 0x100;
  obj->yDirection = IN_motion_Down;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Lindsey0"));
}

void CK4_LindseyFloat(CK_object *obj)
{
  CK_PhysAccelVert1(obj, obj->yDirection, 8);
  if (obj->user1 - obj->posY > 0x20)
    obj->yDirection = IN_motion_Down;

  if (obj->posY - obj->user1 > 0x20)
    obj->yDirection = IN_motion_Up;
}

void CK4_SpawnDartGun(int tileX, int tileY, int direction)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT_Friendly;
  obj->active = OBJ_ACTIVE;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = tileY << G_T_SHIFT;
  obj->clipped = CLIP_not;
  obj->user1 = direction;
  switch (direction)
  {
    case 0:
      obj->posY -= 0x30;
      obj->posX += 0x90;
      obj->gfxChunk = 0x17C;
      break;

    case 1:
      obj->posX += 0x80;
      obj->posY += 0x50;
      obj->gfxChunk = 0x180;
      break;

    case 2:
      obj->posX += 0x90;
      obj->gfxChunk -0x17E;
      break;

    case 3:
      obj->posY += 0x70;
      obj->posX -= 0x30;
      obj->gfxChunk = 0x182;
      break;
  }

  CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_DartGun0"));
}

void CK4_DartGun(CK_object *obj)
{
  CK_object *dart = CK_GetNewObj(true);
  dart->posX = obj->posX;
  dart->posY = obj->posY;
  dart->type = CT4_EnemyShot;
  dart->active = OBJ_EXISTS_ONLY_ONSCREEN;

  switch (obj->user1)
  {
    case 0:
      dart->xDirection = IN_motion_None;
      dart->yDirection = IN_motion_Up;
      CK_SetAction(dart, CK_GetActionByName("CK4_ACT_DartUp0"));
      break;
    case 1:
      dart->xDirection = IN_motion_Right;
      dart->yDirection = IN_motion_None;
      CK_SetAction(dart, CK_GetActionByName("CK4_ACT_DartHorz0"));
      break;
    case 2:
      dart->xDirection = IN_motion_None;
      dart->yDirection = IN_motion_Down;
      CK_SetAction(dart, CK_GetActionByName("CK4_ACT_DartDown0"));
      break;
    case 3:
      dart->xDirection = IN_motion_None;
      dart->yDirection = IN_motion_Left;
      CK_SetAction(dart, CK_GetActionByName("CK4_ACT_DartHorz0"));
      break;
  }

  SD_PlaySound(SOUND_DARTSHOOT);
}

// Wetsuit
void CK4_SpawnWetsuit(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);
  obj->type = CT4_Wetsuit;
  obj->active = OBJ_ACTIVE;
  obj->posX = tileX << G_T_SHIFT;
  obj->posY = (tileY << G_T_SHIFT) - 0x100;
  CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Wetsuit0"));
}

void CK4_WetsuitCol(CK_object *a, CK_object *b)
{
  if (b->type == CT_Player && !b->topTI)
  {
    ck_gameState.ep.ck4.wetsuit = true;
    SD_PlaySound(SOUND_FOOTAPPEAR);
    CK4_ShowWetsuitMessage();
    //RF_ForceRefresh();
    ck_gameState.levelState = LS_LevelComplete;
  }
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

  CK_ACT_AddFunction("CK4_MimrockWait", &CK4_MimrockWait);
  CK_ACT_AddFunction("CK4_MimrockCheckJump", &CK4_MimrockCheckJump);
  CK_ACT_AddColFunction("CK4_MimrockCol", &CK4_MimrockCol);
  CK_ACT_AddColFunction("CK4_MimrockAirCol", &CK4_MimrockAirCol);
  CK_ACT_AddFunction("CK4_MimrockJumpDraw", &CK4_MimrockJumpDraw);
  CK_ACT_AddFunction("CK4_MimrockBounceDraw", &CK4_MimrockBounceDraw);

  CK_ACT_AddFunction("CK4_KillKeenUnderwater", &CK4_KillKeenUnderwater);
  CK_ACT_AddFunction("CK4_DopefishMove", &CK4_DopefishMove);
  CK_ACT_AddFunction("CK4_DopefishEat", &CK4_DopefishEat);
  CK_ACT_AddFunction("CK4_DopefishAfterBurp", &CK4_DopefishAfterBurp);
  CK_ACT_AddFunction("CK4_DopefishBurp", &CK4_DopefishBurp);
  CK_ACT_AddFunction("CK4_Bubbles", &CK4_Bubbles);
  CK_ACT_AddColFunction("CK4_DopefishCol", &CK4_DopefishCol);
  CK_ACT_AddFunction("CK4_FishDraw", &CK4_FishDraw);
  CK_ACT_AddFunction("CK4_SchoolfishMove", &CK4_SchoolfishMove);

  CK_ACT_AddFunction("CK4_SpritePatrol", &CK4_SpritePatrol);
  CK_ACT_AddFunction("CK4_SpriteAim", &CK4_SpriteAim);
  CK_ACT_AddFunction("CK4_SpriteShoot", &CK4_SpriteShoot);
  CK_ACT_AddFunction("CK4_ProjectileDraw", &CK4_ProjectileDraw);

  CK_ACT_AddColFunction("CK4_MineCol", &CK4_MineCol);

  CK_ACT_AddFunction("CK4_LindseyFloat", &CK4_LindseyFloat);

  CK_ACT_AddFunction("CK4_DartGun", &CK4_DartGun);

  CK_ACT_AddColFunction("CK4_WetsuitCol", &CK4_WetsuitCol);

}
