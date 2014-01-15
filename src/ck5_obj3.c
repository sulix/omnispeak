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
#include "id_us.h"

#include <stdio.h>

extern int ck_ticsThisFrame;

#define ABS(x) ((x)>0?(x):(-(x)))

// Shikadi Mine Funcs

void CK5_SpawnMine(int tileX, int tileY)
{
  int i;
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 10; // ShikadiMine
  obj->active = OBJ_ACTIVE;
  obj->clipped = false;
  obj->posX = tileX * 0x100;
  obj->posY = tileY * 0x100 - 0x1F1;

  // X and Y offsets of the Dot relative to the mine
  obj->user2 = 0x100;
  obj->user3 = 0xD0;
  CK_SetAction(obj, CK_GetActionByName("CK5_ACT_Mine2"));
  obj->velX = 0x100;

  for (i = 0; i < 4; i++)
    if (CK5_Walk(obj, i))
      break;
  return;
}

// return 1 if path clear, 0 if path blocked

int CK5_MinePathClear(int tileX, int tileY)
{

  int t, x, y;

  t = CA_TileAtPos(tileX, tileY, 1);
  for (y = 0; y < 2; y++)
    for (x = 0; x < 3; x++)
      if (TI_ForeTop(t) || TI_ForeBottom(t) || TI_ForeLeft(t)
          || TI_ForeRight(t))
        return 0; //path is blocked

  return 1; // didn't hit anything
}

int CK5_Walk(CK_object *obj, CK_Controldir dir)
{

  int tx, ty;

#if 0
  tx = MU2TILE(o->xpos + KeenXVel);
  ty = MU2TILE(o->ypos + KeenYVel);
#endif

  tx = (obj->posX + obj->nextX);
  ty = (obj->posY + obj->nextY);

  switch (dir)
  {

  case CD_north: //up
    if (CK5_MinePathClear(tx, ty - 1))
    {
      obj->xDirection = IN_motion_None;
      obj->yDirection = IN_motion_Up;
      return 1;
    }
    else
    {
      return 0;
    }

  case CD_east: //right
    if (CK5_MinePathClear(tx + 1, ty))
    {
      obj->xDirection = IN_motion_Right;
      obj->yDirection = IN_motion_None;
      return 1;
    }
    else
    {
      return 0;
    }
  case CD_south: //down
    if (CK5_MinePathClear(tx, ty + 1))
    {
      obj->xDirection = IN_motion_None;
      obj->yDirection = IN_motion_Down;
      return 1;
    }
    else
    {
      return 0;
    }
  case CD_west: //left
    if (CK5_MinePathClear(tx - 1, ty))
    {
      obj->xDirection = IN_motion_Left;
      obj->yDirection = IN_motion_None;
      return 1;
    }
    else
    {
      return 0;
    }
  }
  Quit("CK5_Walk: Bad Dir");
}

/*
 * Pick direction to chase keen
 */
void CK5_SeekKeen(CK_object *obj)
{

  int mine_dirs[9] ={Dir_north, Dir_northwest, Dir_east, Dir_northeast,
    Dir_south, Dir_southeast, Dir_west, Dir_southwest, Dir_nodir};

  int i, deltaX, deltaY, principalDir, closestAxis, farthestAxis, cardinalDir;

  // Convert x and y motions into current controldirection
  if (obj->xDirection == IN_motion_Right)
    cardinalDir = CD_east;
  else if (obj->xDirection == IN_motion_Left)
    cardinalDir = CD_west;
  else if (obj->yDirection == IN_motion_Up)
    cardinalDir = CD_north;
  else if (obj->yDirection == IN_motion_Down)
    cardinalDir = CD_south;
  principalDir = mine_dirs[cardinalDir];

  // Determine which component (x or y) has the greatest absolute difference from keen
  // We want to move in that direction first?

  // Get position difference between Keen and Mine
  deltaX = ck_keenObj->posX - (obj->posX + obj->nextX);
  deltaY = ck_keenObj->posY - (obj->posY + obj->nextY);
  closestAxis = Dir_nodir;
  farthestAxis = Dir_nodir;

  if (deltaX > 0)
    farthestAxis = CD_east;
  if (deltaX < 0)
    farthestAxis = CD_west;
  if (deltaY > 0)
    closestAxis = CD_south;
  if (deltaY < 0)
    closestAxis = CD_north;

  if (ABS(deltaY) > ABS(deltaX))
  {
    int s = farthestAxis;
    farthestAxis = closestAxis;
    closestAxis = s;
  }

  // If one of the intended components is already the mine's movement
  // Then there's no need to check twice
  if (farthestAxis == principalDir)
    farthestAxis = Dir_nodir;
  if (closestAxis == principalDir)
    closestAxis = Dir_nodir;

  // Check if there's free space ahead first in the desired directions
  // and then finally in the current direction
  if (closestAxis != Dir_nodir && CK5_Walk(obj, closestAxis))
    return;
  if (farthestAxis != Dir_nodir && CK5_Walk(obj, farthestAxis))
    return;
  if (CK5_Walk(obj, cardinalDir))
    return;

  // Otherwise, look for some free space
  if (US_RndT() > 0x80)
  {
    for (i = 0; i <= 3; i++)
      if (i != principalDir)
        if (CK5_Walk(obj, i))
          return;
  }
  else
  {
    for (i = 3; i >= 0; i--)
      if (i != principalDir)
        if (CK5_Walk(obj, i))
          return;
  }

  // Finally, just keep going forward
  CK5_Walk(obj, principalDir);
  return;
}

#define SOUND_MINEEXPLODE 5

void CK5_MineMove(CK_object *obj)
{

  int deltaX, deltaY, delta, xDir, yDir;

  // Get distance to keen
  deltaX = obj->posX - ck_keenObj->posX;
  deltaY = obj->posY - ck_keenObj->posY;

  // Check if Mine should explode
  if (deltaX < 0x200 && deltaX > -0x500 && deltaY < 0x300 && deltaY > -0x50)
  {
    // TODO: Enable this
    //SD_PlaySound(SOUND_MINEEXPLODE);
    obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
    RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
    return;
  }

  delta = CK_GetTicksPerFrame() * 10;
  if (obj->velX <= delta)
  {
    obj->nextX = obj->xDirection * obj->velX;
    obj->nextY = obj->yDirection * obj->velY;
    delta -= obj->velX;
    xDir = obj->xDirection;
    yDir = obj->yDirection;
    CK5_SeekKeen(obj);
    obj->velX = 0x100;
    if (obj->xDirection != xDir || obj->yDirection != yDir)
    {
      obj->currentAction = CK_GetActionByName("CK5_ACT_Mine1");
      return;
    }
  }

  obj->velX -= delta;
  obj->nextX += delta * obj->xDirection;
  obj->nextY += delta * obj->yDirection;
  return;
}

void CK5_MineSprCol(CK_object *o1, CK_object *o2)
{

  if (o2->type == CT_Stunner)
    CK_ShotHit(o2);
  return;
}

void CK5_MineShrapSprCol(CK_object *o1, CK_object *o2)
{

  // Explode stunner
  if (o2->type == CT_Stunner)
  {
    CK_ShotHit(o2);
    return;
  }

  // Kill keen
  if (o2->type == CT_Player)
  {
    //KeenDie();
    return;
  }

  //blow up QED
  if (o2->type == 0x19)
  {
    // TODO: implement this
    /*
       FuseExplosionSpawn(o2->clipRects.tileX1, o2->clipRects.tileY1);
       FuseExplosionSpawn(o2->clipRects.tileX2, o2->clipRects.tileY1);
       FuseExplosionSpawn(o2->clipRects.tileX1, o2->clipRects.tileY2);
       FuseExplosionSpawn(o2->clipRects.tileX2, o2->clipRects.tileY2);
       RF_ReplaceTileBlock(0, 0, 0x10, 0xB, 4, 2);
       RF_ReplaceTileBlock(4, 0, 0x10, 0xD, 4, 2);
       LevelEndSpawn();
     *
     */
    CK_RemoveObj(o2);
  }

  return;
}

void CK5_MineMoveDotsToCenter(CK_object *obj)
{

  int deltaX, deltaY;
  int dotOffsetX, dotOffsetY;

  deltaX = obj->posX - ck_keenObj->posX;
  deltaY = obj->posY - ck_keenObj->posY;

  // Blow up if keen is nearby
  if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300
      && deltaY > -0x300)
  {
    //SD_PlaySound(SOUND_MINEEXPLODE);
    obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
    RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
    return;
  }

  obj->visible = true;
  dotOffsetX = 0x100;
  dotOffsetY = 0xD0;

  // Move Dot to the center, then change Action
  if (obj->user2 < dotOffsetX)
  {
    obj->user2 += CK_GetTicksPerFrame() * 4;
    if (obj->user2 >= dotOffsetX)
    {
      obj->user2 = dotOffsetX;
      obj->currentAction = obj->currentAction->next;
    }
  }
  else if (obj->user2 > dotOffsetX)
  {
    obj -= CK_GetTicksPerFrame() * 4;
    if (obj->user2 <= dotOffsetX)
    {
      obj->user2 = dotOffsetX;
      obj->currentAction = obj->currentAction->next;
    }
  }

  // Do the same in the Y direction
  if (obj->user3 < dotOffsetY)
  {
    obj->user3 += CK_GetTicksPerFrame() * 4;
    if (obj->user3 >= dotOffsetY)
    {
      obj->user3 = dotOffsetY;
      obj->currentAction = obj->currentAction->next;
    }
  }
  else if (obj->user3 > dotOffsetY)
  {
    obj -= CK_GetTicksPerFrame() * 4;
    if (obj->user3 <= dotOffsetY)
    {
      obj->user3 = dotOffsetY;
      obj->currentAction = obj->currentAction->next;
    }
  }

  return;
}

void CK5_MineMoveDotsToSides(CK_object *obj)
{

  int deltaX, deltaY, dotOffsetX, dotOffsetY;

  deltaX = obj->posX - ck_keenObj->posX;
  deltaY = obj->posY - ck_keenObj->posY;

  // Explode if Keen is nearby
  if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300
      && deltaY > -0x300)
  {
    //SD_PlaySound(SOUND_MINEEXPLODE);
    obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
    RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
    return;
  }

  obj->visible = 1;

  switch (obj->xDirection)
  {

  case IN_motion_Left:
    dotOffsetX = 0x80;
    break;
  case IN_motion_None:
    dotOffsetX = 0x100;
    break;
  case IN_motion_Right:
    dotOffsetX = 0x180;
    break;
  default:
    break;
  }

  switch (obj->yDirection)
  {

  case IN_motion_Up:
    dotOffsetY = 0x50;
    break;
  case IN_motion_None:
    dotOffsetY = 0xD0;
    break;
  case IN_motion_Down:
    dotOffsetY = 0x150;
    break;
  default:
    break;
  }

  // Move the dot and change action
  // when it reaches the desired offset
  if (obj->user2 < dotOffsetX)
  {
    obj->user2 += CK_GetTicksPerFrame() * 4;
    if (obj->user2 >= dotOffsetX)
    {
      obj->user2 = dotOffsetX;
      obj->currentAction = obj->currentAction->next;
    }
  }
  else if (obj->user2 > dotOffsetX)
  {
    obj -= CK_GetTicksPerFrame() * 4;
    if (obj->user2 <= dotOffsetX)
    {
      obj->user2 = dotOffsetX;
      obj->currentAction = obj->currentAction->next;
    }
  }

  // Do the same in the Y direction
  if (obj->user3 < dotOffsetY)
  {
    obj->user3 += CK_GetTicksPerFrame() * 4;
    if (obj->user3 >= dotOffsetY)
    {
      obj->user3 = dotOffsetY;
      obj->currentAction = obj->currentAction->next;
    }
  }
  else if (obj->user3 > dotOffsetY)
  {
    obj -= CK_GetTicksPerFrame() * 4;
    if (obj->user3 <= dotOffsetY)
    {
      obj->user3 = dotOffsetY;
      obj->currentAction = obj->currentAction->next;
    }
  }

  return;
}

void CK5_MineExplode(CK_object *obj)
{

  //SD_PlaySound(SOUND_MINEEXPLODE);

  // upleft
  CK_object *new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX;
  new_object->posY = obj->posY;
  new_object->velX = -US_RndT() / 8;
  new_object->velY = -0x48;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  // upright
  new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX + 0x100;
  new_object->posY = obj->posY;
  new_object->velX = US_RndT() / 8;
  new_object->velY = -0x48;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX;
  new_object->posY = obj->posY;
  new_object->velX = US_RndT() / 16 + 0x28;
  new_object->velY = -0x18;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX + 0x100;
  new_object->posY = obj->posY;
  new_object->velX = -0x28 - US_RndT() / 16;
  new_object->velY = -0x18;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX;
  new_object->posY = obj->posY;
  new_object->velX = 0x18;
  new_object->velY = 0x10;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  new_object = CK_GetNewObj(true);
  new_object->posX = obj->posX + 0x100;
  new_object->posY = obj->posY;
  new_object->velX = 0x18;
  new_object->velY = 0x10;
  CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

  return;

}

void CK5_MineTileCol(CK_object *obj)
{

  RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
                   obj->zLayer); //mine
  RF_AddSpriteDraw(&obj->user4, obj->posX + obj->user2,
                   obj->posY + obj->user3, 0x17B, 0, 2); //dot
  return;
}

/*
 * Tile collision for making the mine and shelley bits bounce
 * This was originally placed in CK5_MISC.C, but it makes more sense
 * to write it here
 */

void CK5_ShrapnelTileCol(CK_object *obj)
{
  int bouncePower; // is long in keen source
  int topflags_0, velY, absvelX;
  int d, motion;

  int bouncearray[8][8] ={
    { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    { 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0},
    { 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
    { 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
    { 0x3, 0x2, 0x1, 0x0, 0xF, 0xE, 0xD, 0xC},
    { 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
    { 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
    { 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4},
  };

  RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);


  // Bounce backwards if a side wall is hit
  {
    obj->velX = -obj->velX / 2;
  }

  // Bounce down if a top wall is hit
  if (obj->bottomTI)
  {
    obj->velY = -obj->velY / 2;
    return;
  }

  // Bounce sideways if a slope floor is hit
  topflags_0 = obj->topTI;
  if (topflags_0)
    return;

  if (obj->velY < 0)
    obj->velY = 0;

  absvelX = ABS(obj->velX);
  velY = obj->velY;

  if (absvelX > velY)
  {
    if (velY * 2 < absvelX)
    {
      d = 0;
      bouncePower = absvelX * 286;
    }
    else
    {
      d = 1;
      bouncePower = absvelX * 362;
    }
  }
  else
  {
    if (absvelX * 2 < velY)
    {
      d = 3;
      bouncePower = velY * 256;
    }
    else
    {
      d = 2;
      bouncePower = velY * 286;
    }
  }

  if (obj->velX > 0) //mirror around y-axis
    d = 7 - d;

  bouncePower /= 2; // absorb energy

  motion = bouncearray[obj->topTI][d];

  switch (motion)
  {
  case 0:
    obj->velX = bouncePower / 286;
    obj->velY = -obj->velX / 2;
    break;
  case 1:
    obj->velX = bouncePower / 362;
    obj->velY = -obj->velX;
    break;
  case 2:
    obj->velY = -(bouncePower / 286);
    obj->velX = -obj->velY / 2;
    break;
  case 3:
  case 4:
    obj->velX = 0;
    obj->velY = -(bouncePower / 256);
    break;
  case 5:
    obj->velY = -(bouncePower / 286);
    obj->velX = obj->velY / 2;
    break;
  case 6:
    obj->velY = -(bouncePower / 362);
    obj->velX = obj->velY;
    break;
  case 7:
    obj->velX = -(bouncePower / 286);
    obj->velY = obj->velX / 2;
    break;
  case 8:
    obj->velX = -(bouncePower / 286);
    obj->velY = -obj->velX / 2;
    break;
  case 9:
    obj->velX = -(bouncePower / 362);
    obj->velY = -obj->velX;
    break;
  case 0xA:
    obj->velY = bouncePower / 286;
    obj->velX = -obj->velY / 2;
    break;
  case 0xB:
  case 0xC:
    obj->velX = 0;
    obj->velY = -(bouncePower / 256);
    break;
  case 0xD:
    obj->velY = bouncePower / 286;
    obj->velX = obj->velY / 2;
    break;
  case 0xE:
    obj->velX = bouncePower / 362;
    obj->velY = bouncePower / 362;
    break;
  case 0xF:
    obj->velX = bouncePower / 256;
    obj->velY = obj->velX / 2;
    break;
  default: break;
  }

  // if speed is lower than threshhold, then disappear
  if (bouncePower < 0x1000)
    CK_SetAction2(obj, obj->currentAction->next);

}

// Spirogrip Funcs

void CK5_SpawnSpirogrip(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 13; // Spirogrip
  obj->active = true;
  obj->posX = (tileX << 8);
  obj->posY = (tileY << 8) - 256;

  obj->xDirection = -1; // Left
  obj->yDirection = 1; //Down

  CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SpirogripSpin1"));
}

void CK5_SpirogripSpin(CK_object *obj)
{
  // If we're bored of spinning...
  if (US_RndT() > 20)
    return;

  // TODO: Play sound (0x3D)

  // and we're in the right direction, fly!
  if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin1"))
    obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyUp");
  else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin3"))
    obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyRight");
  else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin5"))
    obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyDown");
  else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin7"))
    obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyLeft");

}

void CK5_SpirogripFlyDraw(CK_object *obj)
{
  // Draw the sprite
  RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, 0,
                   obj->zLayer);

  // Check if we've collided with a tile
  if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
  {
    obj->currentAction = obj->currentAction->next;
    //TODO: Play sound (0x1B)
  }
}
// Korath Funcs

void CK5_KorathDraw(CK_object *obj)
{
  if (obj->xDirection == 1 && obj->leftTI)
  {
    obj->posX -= obj->deltaPosX;
    obj->xDirection = -1;
    obj->timeUntillThink = US_RndT() / 32;
    CK_SetAction(obj, obj->currentAction);
  }
  else if (obj->xDirection == -1 && obj->rightTI)
  {
    obj->posX -= obj->deltaPosX;
    obj->xDirection = 1;
    obj->timeUntillThink = US_RndT() / 32;
    CK_SetAction(obj, obj->currentAction);
  }
  else if (!obj->topTI)
  {
    obj->posX -= obj->deltaPosX;
    obj->xDirection = -obj->xDirection;
    obj->timeUntillThink = US_RndT() / 32;
    obj->nextX = 0;

    CK_PhysUpdateNormalObj(obj);
    //CK_SetAction(obj, obj->currentAction);
  }

  RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
                   obj->zLayer);
}

void CK5_KorathWalk(CK_object *obj)
{
  if (US_RndT() < 10)
  {
    obj->nextX = 0;
    obj->xDirection = US_RndT() < 128 ? 1 : -1;
    obj->currentAction = CK_GetActionByName("CK5_ACT_KorathWait");
  }
}

void CK5_KorathColFunc(CK_object *obj, CK_object *other)
{
  //todo: if keen
  if (other->currentAction->collide)
  {
    CK_PhysPushX(other, obj);
    return;
  }
}

void CK5_SpawnKorath(int tileX, int tileY)
{
  CK_object *obj = CK_GetNewObj(false);

  obj->type = 23;
  obj->active = true;
  obj->posX = tileX << 8;
  obj->posY = (tileY << 8) - 128;
  obj->xDirection = US_RndT() < 128 ? 1 : -1;
  CK_SetAction(obj, CK_GetActionByName("CK5_ACT_KorathWalk1"));

  printf("Spawning Korath at %d,%d\n", tileX, tileY);
}

/*
 * Add the Obj3 functions to the function db
 */
void CK5_Obj3_SetupFunctions()
{
  // Shikadi Mine
  CK_ACT_AddFunction("CK5_MineMove", &CK5_MineMove);
  CK_ACT_AddFunction("CK5_MineMoveDotsToCenter", &CK5_MineMoveDotsToCenter);
  CK_ACT_AddFunction("CK5_MineMoveDotsToSides", &CK5_MineMoveDotsToSides);
  CK_ACT_AddFunction("CK5_MineExplode", &CK5_MineExplode);
  CK_ACT_AddFunction("CK5_MineSprCol", &CK5_MineSprCol);
  CK_ACT_AddFunction("CK5_MineShrapSprCol", &CK5_MineShrapSprCol);
  CK_ACT_AddFunction("CK5_ShrapnelTileCol", &CK5_ShrapnelTileCol);

  // Spirogrip
  CK_ACT_AddFunction("CK5_SpirogripSpin", &CK5_SpirogripSpin);
  CK_ACT_AddFunction("CK5_SpirogripFlyDraw", &CK5_SpirogripFlyDraw);

  // Korath
  CK_ACT_AddFunction("CK5_KorathDraw", &CK5_KorathDraw);
  CK_ACT_AddFunction("CK5_KorathWalk", &CK5_KorathWalk);
  CK_ACT_AddColFunction("CK5_KorathColFunc", &CK5_KorathColFunc);
}

