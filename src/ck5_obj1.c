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
#include "id_rf.h"

#include <stdio.h>

void CK5_TurretSpawn(int tileX, int tileY, int direction)
{
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 0;
  obj->gfxChunk = 0;
  obj->active = true;
  obj->clipRects.tileX1 = obj->clipRects.tileX2 = tileX;
  obj->clipRects.tileY1 = obj->clipRects.tileY2 = tileY;

  obj->posX = tileX << 8;
  obj->posY = tileY << 8;
  obj->clipRects.unitX1 = obj->clipRects.unitX2 = tileX << 8;
  obj->clipRects.unitX2 = obj->clipRects.unitY2 = tileY << 8;

  obj->user1 = direction;

  CK_SetAction(obj, CK_GetActionByName("CK5_ACT_turretWait"));
}

void CK5_TurretShoot(CK_object *obj)
{
  CK_object *shot = CK_GetNewObj(true);

  shot->type = 0;	//TurretShot
  shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
  //shot->clipped = true;
  shot->posX = obj->posX;
  shot->posY = obj->posY;

  printf("Shooting!\n");
  switch (obj->user1)
  {
  case 0:
    shot->velX = 0;
    shot->velY = -64;
    break;
  case 1:
    shot->velX = 64;
    shot->velY = 0;
    break;
  case 2:
    shot->velX = 0;
    shot->velY = 64;
    break;
  case 3:
    shot->velX = -64;
    shot->velY = 0;
    break;
  }

  CK_SetAction(shot, CK_GetActionByName("CK5_ACT_turretShot1"));

  //CK_SetAction(obj, &CK5_ACT_turretWait);
}

void CK5_Glide(CK_object *obj)
{
  obj->nextX = obj->velX;
  obj->nextY = obj->velY;
}

void CK5_TurretShotCol(CK_object *me, CK_object *other)
{
  //TODO: Kill Keen
}

void CK5_TurretShotDraw(CK_object *obj)
{
  if (obj->topTI || obj->bottomTI || obj->leftTI || obj->rightTI)
  {
    printf("Shot Hit\n");
    //obj->clipped=false;
    CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_turretShotHit1"));
  }

  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->currentAction->chunkLeft, false, 0);

}

/*
 * Spawn a "Sneak Plat".
 */
void CK5_SneakPlatSpawn(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 6;
  obj->active = OBJ_ALWAYS_ACTIVE;
  obj->zLayer = 0;
  obj->posX = tileX << 8;
  obj->posY = tileY << 8;
  obj->xDirection = 0;
  obj->yDirection = 1;
  obj->clipped = false;

  CK_SetAction(obj, CK_GetActionByName("CK5_ACT_sneakPlatWait"));
  CA_CacheGrChunk(obj->gfxChunk);
}

void CK5_SneakPlatThink(CK_object *obj)
{
  if (ck_keenObj->currentAction == CK_GetActionByName("CK_ACT_keenJump1"))
  {
    if (ck_keenObj->xDirection == 1)
    {
      int dist = obj->clipRects.unitX1 - ck_keenObj->clipRects.unitX2;
      if (dist > 0x400 || dist < 0) return;
    }
    else
    {
      int dist = ck_keenObj->clipRects.unitX1 - obj->clipRects.unitX2;
      if (dist > 0x400 || dist < 0) return;
    }

    int vertDist = ck_keenObj->posY - obj->posY;
    if (vertDist < -0x600 || vertDist > 0x600) return;

    obj->xDirection = ck_keenObj->xDirection;
    obj->currentAction = CK_GetActionByName("CK5_ACT_sneakPlatSneak");
  }
}

/*
 * Spawn a GoPlat
 */
void CK5_GoPlatSpawn(int tileX, int tileY, int direction, bool purple)
{
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 6;
  obj->active = OBJ_ALWAYS_ACTIVE;
  obj->zLayer = 0;
  obj->posX = tileX << 8;
  obj->posY = tileY << 8;
  obj->xDirection = 0;
  obj->yDirection = 0;
  obj->clipped = false;

  if (purple)
  {
    obj->posX += 0x40;
    obj->posY += 0x40;
    CK_SetAction(obj, CK_GetActionByName("CK5_ACT_purpleGoPlat"));
  }
  else
  {
    CK_SetAction(obj, CK_GetActionByName("CK5_ACT_redGoPlat"));
  }


  int mapW = CA_MapHeaders[ck_currentMapNumber]->width;
  int mapH = CA_MapHeaders[ck_currentMapNumber]->height;
  CA_mapPlanes[2][tileY * mapW + tileX] = direction + 0x5B;

  obj->user1 = direction;
  obj->user2 = 256;

}

static int ck5_infoplaneArrowsX[8] ={0, 1, 0, -1, 1, 1, -1, -1};
static int ck5_infoplaneArrowsY[8] ={-1, 0, 1, 0, -1, 1, 1, -1};

void CK5_RedGoPlatThink(CK_object *obj)
{

  if (obj->nextX || obj->nextY) return;

  int delta = CK_GetTicksPerFrame()*12;

  // Will we reach a new tile?
  if (obj->user2 > delta)
  {
    // No... keep moving in the same direction.
    obj->user2 -= delta;

    int dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += delta;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= delta;
    }

    int dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += delta;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= delta;
    }
  }
  else
  {
    // Move to next tile.
    int dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += obj->user2;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= obj->user2;
    }

    int dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += obj->user2;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= obj->user2;
    }

    int tileX = (obj->posX + obj->nextX) >> 8;
    int tileY = (obj->posY + obj->nextY) >> 8;

    obj->user1 = CA_TileAtPos(tileX, tileY, 2) - 0x5B;

    if ((obj->user1 < 0) || (obj->user1 > 8))
    {
      Quit("Goplat moved to a bad spot.");
    }

    delta -= obj->user2;
    obj->user2 = 256 - delta;

    // Move in the new direction.
    dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += delta;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= delta;
    }

    dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += delta;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= delta;
    }
  }
}

void CK5_PurpleGoPlatThink(CK_object *obj)
{

  if (obj->nextX || obj->nextY) return;

  int delta = CK_GetTicksPerFrame()*12;

  // Will we reach a new tile?
  if (obj->user2 > delta)
  {
    // No... keep moving in the same direction.
    obj->user2 -= delta;

    int dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += delta;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= delta;
    }

    int dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += delta;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= delta;
    }
  }
  else
  {
    // Move to next tile.
    int dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += obj->user2;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= obj->user2;
    }

    int dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += obj->user2;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= obj->user2;
    }

    int tileX = (obj->posX + obj->nextX + 0x40) >> 8;
    int tileY = (obj->posY + obj->nextY + 0x40) >> 8;

    obj->user1 = CA_TileAtPos(tileX, tileY, 2) - 0x5B;

    if ((obj->user1 < 0) || (obj->user1 > 8))
    {
      Quit("Goplat moved to a bad spot.");
    }

    delta -= obj->user2;
    obj->user2 = 256 - delta;

    // Move in the new direction.
    dirX = ck5_infoplaneArrowsX[obj->user1];
    if (dirX == 1)
    {
      // Moving right.
      obj->nextX += delta;
    }
    else if (dirX == -1)
    {
      // Moving left
      obj->nextX -= delta;
    }

    dirY = ck5_infoplaneArrowsY[obj->user1];
    if (dirY == 1)
    {
      // Moving down
      obj->nextY += delta;
    }
    else if (dirY == -1)
    {
      // Moving up
      obj->nextY -= delta;
    }
  }
}

/*
 * Setup all of the functions in this file.
 */
void CK5_Obj1_SetupFunctions()
{
  CK_ACT_AddFunction("CK5_TurretShoot", &CK5_TurretShoot);
  CK_ACT_AddFunction("CK5_Glide", &CK5_Glide);
  CK_ACT_AddColFunction("CK5_TurretShotCol", &CK5_TurretShotCol);
  CK_ACT_AddFunction("CK5_TurretShotDraw", &CK5_TurretShotDraw);
  CK_ACT_AddFunction("CK5_SneakPlatThink", &CK5_SneakPlatThink);
  CK_ACT_AddFunction("CK5_RedGoPlatThink", &CK5_RedGoPlatThink);
  CK_ACT_AddFunction("CK5_PurpleGoPlatThink", &CK5_PurpleGoPlatThink);
}
