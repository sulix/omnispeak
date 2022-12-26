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

#include "id_rf.h"
#include "id_us.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

#include <stdio.h>

extern int ck_ticsThisFrame;

// Helper for Shikadi Master
bool OSI_IsObjectOnscreen(CK_object *obj)
{
	int borderSize = CK_INT(OSI_MasterOnscreenBorder, 1);
	return	((obj->clipRects.tileX2 >= RF_UnitToTile(rf_scrollXUnit) - borderSize) &&
		(obj->clipRects.tileX1 <= RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320) + borderSize) &&
		(obj->clipRects.tileY1 <= RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208) + borderSize) &&
		(obj->clipRects.tileY2 >= RF_UnitToTile(rf_scrollYUnit) - borderSize));

}

// Shikadi Mine Funcs
int CK5_Walk(CK_object *obj, CK_Controldir dir);

void CK5_SpawnMine(int tileX, int tileY)
{
	int i;
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 10; // ShikadiMine
	obj->active = OBJ_ACTIVE;
	obj->clipped = CLIP_not;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0x1F1;

	// X and Y offsets of the Dot relative to the mine
	obj->user2 = 0x100;
	obj->user3 = 0xD0;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_Mine2"));
	obj->velX = 0x100;

	for (i = 0; i < 4; i++)
		if (CK5_Walk(obj, (CK_Controldir)i))
			break;
	return;
}

/* Check a 3 x 2 square originating from (tileX, tileY)
 * If blocked, then return false
 */
int CK5_MinePathClear(int tileX, int tileY)
{

	int t, x, y;

	for (y = 0; y < 2; y++)
	{
		for (x = 0; x < 3; x++)
		{
			t = CA_TileAtPos(tileX + x, tileY + y, 1);
			if (TI_ForeTop(t) || TI_ForeBottom(t) || TI_ForeLeft(t) || TI_ForeRight(t))
				return 0; //path is blocked
		}
	}
	return 1; // didn't hit anything
}

int CK5_Walk(CK_object *obj, CK_Controldir dir)
{

	int tx, ty;

	tx = RF_UnitToTile(obj->posX + ck_nextX);
	ty = RF_UnitToTile(obj->posY + ck_nextY);

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
	// What is the point of the ordinal directions?
	// The mine only ever moves in cardinal directions.
	// Perhaps it was supposed to move in all eight?

	// EDIT: principalDir is lated passed as a CK_Controldir anyway
	CK_Dir mine_dirs[9] = {Dir_north, Dir_northwest, Dir_east, Dir_northeast,
		Dir_south, Dir_southeast, Dir_west, Dir_southwest, Dir_nodir};
	//CK_Dir mine_dirs[9] ={CD_south, CD_west, CD_north, CD_east, 6, 7, 4, 5, 8};

	int i, deltaX, deltaY;
	// These values should store CK_Controldir values (0 to 3), but 8
	// is stored in the axes (same as Dir_nodir from the CK_Dir enum).
	// Maybe there were different plans at some point.
	int closestAxis, farthestAxis, cardinalDir, principalDir;

	// Convert x and y motions into current controldirection
	if (obj->xDirection == IN_motion_Right)
		cardinalDir = CD_east;
	else if (obj->xDirection == IN_motion_Left)
		cardinalDir = CD_west;
	else if (obj->yDirection == IN_motion_Up)
		cardinalDir = CD_north;
	else if (obj->yDirection == IN_motion_Down)
		cardinalDir = CD_south;

	// Should this not be cardinalDir * 2?
	principalDir = mine_dirs[cardinalDir];

	// Determine which component (x or y) has the greatest absolute difference from keen
	// We want to move in that direction first?

	// Get position difference between Keen and Mine
	deltaX = ck_keenObj->posX - (obj->posX + ck_nextX);
	deltaY = ck_keenObj->posY - (obj->posY + ck_nextY);
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

	if (CK_Cross_abs(deltaY) > CK_Cross_abs(deltaX))
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
	if (closestAxis != Dir_nodir && CK5_Walk(obj, (CK_Controldir)closestAxis))
		return;
	if (farthestAxis != Dir_nodir && CK5_Walk(obj, (CK_Controldir)farthestAxis))
		return;
	if (CK5_Walk(obj, (CK_Controldir)cardinalDir))
		return;

	// Otherwise, look for some free space
	if (US_RndT() > 0x80)
	{
		for (i = 0; i <= 3; i++)
			if (i != principalDir)
				if (CK5_Walk(obj, (CK_Controldir)i))
					return;
	}
	else
	{
		for (i = 3; i >= 0; i--)
			if (i != principalDir)
				if (CK5_Walk(obj, (CK_Controldir)i))
					return;
	}

	// Finally, just keep going forward
	CK5_Walk(obj, (CK_Controldir)principalDir);
	return;
}

void CK5_MineMove(CK_object *obj)
{

	int16_t deltaX, deltaY, delta, xDir, yDir;

	// Get distance to keen
	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Check if Mine should explode
	if (deltaX < 0x200 && deltaX > -0x500 && deltaY < 0x300 && deltaY > -0x50)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_MINEEXPLODE));
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDrawUsing16BitOffset(&obj->user4);
		return;
	}

	// Move the mine to the next tile boundary
	// obj->velX is used as a ticker
	// When the ticker reaches zero, check if directional change needed
	delta = SD_GetSpriteSync() * 10;
	if (obj->velX <= delta)
	{
		// Move up to the tile boundary
		ck_nextX = obj->xDirection * obj->velX;
		ck_nextY = obj->yDirection * obj->velX;
		delta -= obj->velX;
		xDir = obj->xDirection;
		yDir = obj->yDirection;

		// Switch to the changing direction action if necessary
		CK5_SeekKeen(obj);
		obj->velX = 0x100;
		if (obj->xDirection != xDir || obj->yDirection != yDir)
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_Mine1");
			return;
		}
	}

	// Tick down velX and move mine
	obj->velX -= delta;
	ck_nextX += delta * obj->xDirection;
	ck_nextY += delta * obj->yDirection;
	return;
}

void CK5_MineCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
		CK_ShotHit(o2);
	return;
}

void CK5_MineShrapCol(CK_object *o1, CK_object *o2)
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
		CK_KillKeen();
		return;
	}

	//blow up QED
	if (o2->type == 0x19)
	{
		CK5_SpawnFuseExplosion(o2->clipRects.tileX1, o2->clipRects.tileY1);
		CK5_SpawnFuseExplosion(o2->clipRects.tileX2, o2->clipRects.tileY1);
		CK5_SpawnFuseExplosion(o2->clipRects.tileX1, o2->clipRects.tileY2);
		CK5_SpawnFuseExplosion(o2->clipRects.tileX2, o2->clipRects.tileY2);
		RF_ReplaceTileBlock(CK_INT(CK5_QEDBrokenSrcX1, 0), CK_INT(CK5_QEDBrokenSrcY1, 0), CK_INT(CK5_QEDBrokenDestX1, 0x10), CK_INT(CK5_QEDBrokenDestY1, 0xB), 4, 2);
		RF_ReplaceTileBlock(CK_INT(CK5_QEDBrokenSrcX2, 4), CK_INT(CK5_QEDBrokenSrcY2, 0), CK_INT(CK5_QEDBrokenDestX2, 0x10), CK_INT(CK5_QEDBrokenDestY2, 0xD), 4, 2);
		CK5_SpawnLevelEnd();
		CK_RemoveObj(o2);
	}

	return;
}

void CK5_MineMoveDotsToCenter(CK_object *obj)
{

	int16_t deltaX, deltaY;
	int16_t dotOffsetX, dotOffsetY;

	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Blow up if keen is nearby
	if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300 && deltaY > -0x300)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_MINEEXPLODE));
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDrawUsing16BitOffset(&obj->user4);
		return;
	}

	obj->visible = true;
	dotOffsetX = 0x100;
	dotOffsetY = 0xD0;

	// Move Dot to the center, then change Action
	if (obj->user2 < dotOffsetX)
	{
		obj->user2 += SD_GetSpriteSync() * 4;
		if (obj->user2 >= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user2 > dotOffsetX)
	{
		obj->user2 -= SD_GetSpriteSync() * 4;
		if (obj->user2 <= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}

	// Do the same in the Y direction
	if (obj->user3 < dotOffsetY)
	{
		obj->user3 += SD_GetSpriteSync() * 4;
		if (obj->user3 >= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user3 > dotOffsetY)
	{
		obj->user3 -= SD_GetSpriteSync() * 4;
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

	int16_t deltaX, deltaY, dotOffsetX, dotOffsetY;

	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Explode if Keen is nearby
	if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300 && deltaY > -0x300)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_MINEEXPLODE));
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDrawUsing16BitOffset(&obj->user4);
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
		obj->user2 += SD_GetSpriteSync() * 4;
		if (obj->user2 >= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user2 > dotOffsetX)
	{
		obj->user2 -= SD_GetSpriteSync() * 4;
		if (obj->user2 <= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}

	// Do the same in the Y direction
	if (obj->user3 < dotOffsetY)
	{
		obj->user3 += SD_GetSpriteSync() * 4;
		if (obj->user3 >= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user3 > dotOffsetY)
	{
		obj->user3 -= SD_GetSpriteSync() * 4;
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

	CK_object *new_object;
	SD_PlaySound(CK_SOUNDNUM(SOUND_MINEEXPLODE));

	// upleft
	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = -US_RndT() / 8;
	new_object->velY = -48;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	// upright
	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = US_RndT() / 8;
	new_object->velY = -48;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = US_RndT() / 16 + 40;
	new_object->velY = -24;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = -40 - US_RndT() / 16;
	new_object->velY = -24;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = 24;
	new_object->velY = 16;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = 24;
	new_object->velY = 16;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	return;
}

void CK5_MineTileCol(CK_object *obj)
{

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
		obj->zLayer); //mine
	RF_AddSpriteDrawUsing16BitOffset(&obj->user4, obj->posX + obj->user2,
		obj->posY + obj->user3, 0x17B, 0, 2); //dot
	return;
}

void CK5_SpawnRobo(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT5_Robo;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0x400;
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_None;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_Robo0"));
}

void CK5_RoboMove(CK_object *obj)
{

	// Check for shot opportunity
	if (!(obj->posX & 0x40) && ck_keenObj->clipRects.unitY2 > obj->clipRects.unitY1 && ck_keenObj->clipRects.unitY1 < obj->clipRects.unitY2 &&
		US_RndT() < 0x10)
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->user1 = 10;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboShoot0");
	}
}
// time is number of shots

void CK5_RoboCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = o1->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		o1->user1 = 10;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboShoot0"));
		return;
	}
	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK5_RoboShoot(CK_object *obj)
{

	CK_object *new_object;
	int shotSpawnX;

	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_Robo0");

	shotSpawnX = obj->xDirection == IN_motion_Right ? obj->posX + 0x380 : obj->posX;

	if ((new_object = CK5_SpawnEnemyShot(shotSpawnX, obj->posY + 0x200, CK_GetActionByName("CK5_ACT_RoboShot0"))))
	{
		new_object->velX = obj->xDirection * 60;
		new_object->velY = obj->user1 & 1 ? -8 : 8;
		SD_PlaySound(CK_SOUNDNUM(SOUND_ENEMYSHOOT));
		ck_nextX = obj->xDirection == IN_motion_Right ? -0x40 : 0x40;
	}
}

void CK5_RoboShotCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboShotHit0"));
	}
}

void CK5_RoboShotTileCol(CK_object *obj)
{

	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_ENEMYSHOTHIT));
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_RoboShotHit0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Spirogrip Funcs

void CK5_SpawnSpirogrip(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = CT5_Spirogrip;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(CK5_SpirogripSpawnYOffset, 256);

	obj->xDirection = 1; // Right
	obj->yDirection = 1; // Down

	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SpirogripAttachDown"));
}

void CK5_SpirogripSpin(CK_object *obj)
{
	// If we're bored of spinning...
	if (US_RndT() > CK_INT(CK5_SpirogripLaunchChance, 20))
		return;

	SD_PlaySound(CK_SOUNDNUM(SOUND_SPIROFLY));

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
		//obj->currentAction = obj->currentAction->next;
		CK_SetAction2(obj, obj->currentAction->next);
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPIROSLAM));
	}
}

void CK5_SpawnSpindred(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Spindred;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - 0x80;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Spindred0"));
}

void CK5_SpindredBounce(CK_object *obj)
{
	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t timedelta = lastTimeCount - SD_GetSpriteSync(); timedelta < lastTimeCount; timedelta++)
	{
		// Every odd tic...
		if (timedelta & 1)
		{
			if (obj->yDirection == IN_motion_Down)
			{
				if (obj->velY < 0 && obj->velY >= -3)
				{
					ck_nextY += obj->velY;
					obj->velY = 0;
					return;
				}

				if ((obj->velY += 3) > 70)
					obj->velY = 70;
			}
			else
			{
				if (obj->velY > 0 && obj->velY <= 3)
				{
					ck_nextY += obj->velY;
					obj->velY = 0;
					return;
				}
				if ((obj->velY -= 3) < -70)
					obj->velY = -70;
			}
		}
		ck_nextY += obj->velY;
	}
}

void CK5_SpindredTileCol(CK_object *obj)
{

	if (obj->bottomTI)
	{
		obj->velY = 0;
		if (obj->yDirection == IN_motion_Up)
		{
			// Reverse directions after three slams
			if (++obj->user1 == 3)
			{
				obj->user1 = 0;
				obj->velY = 68;
				obj->yDirection = -obj->yDirection;
				SD_PlaySound(CK_SOUNDNUM(SOUND_SPINDREDFLYDOWN)); // fly down
			}
			else
			{
				SD_PlaySound(CK_SOUNDNUM(SOUND_SPINDREDSLAM)); // slam once
				obj->velY = 40;
			}
		}
	}

	if (obj->topTI)
	{
		obj->velY = 0;
		if (obj->yDirection == IN_motion_Down)
		{
			if (++obj->user1 == 3)
			{ //time holds slamcount
				obj->user1 = 0;
				obj->velY = -68;
				obj->yDirection = -obj->yDirection;
				SD_PlaySound(CK_SOUNDNUM(SOUND_SPINDREDFLYUP)); // fly down
			}
			else
			{
				SD_PlaySound(CK_SOUNDNUM(SOUND_SPINDREDSLAM)); // slam once
				obj->velY = -40;
			}
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SpawnMaster(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Master;
	new_object->active = (CK_objActive)CK_INT(CK5_MasterActive, OBJ_ALWAYS_ACTIVE);
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - 0x180;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Master0"));
}

void CK5_MasterStand(CK_object *obj)
{

	if (US_RndT() < CK_INT(CK5_MasterStandChance, 0x40))
		return;
	
	if (obj->user1 == CK_INT(CK5_MasterShotsBeforeTele, 1))
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_MasterTele0");
		obj->user1 = 0;
		return;
	}

	obj->user1++;
	obj->xDirection = ck_keenObj->posX > obj->posX ? IN_motion_Right : IN_motion_Left;
	obj->currentAction = CK_GetActionByName("CK5_ACT_MasterShoot0");
}

void CK5_MasterShoot(CK_object *obj)
{

	int xPos;
	CK_object *new_object;
	
	if (!OSI_IsObjectOnscreen(obj))
		return;

	xPos = obj->xDirection == IN_motion_Right ? obj->posX : 0x100 + obj->posX;
	if ((new_object = CK5_SpawnEnemyShot(xPos, obj->posY + 0x80, CK_GetActionByName("CK5_ACT_MasterBall0"))))
	{
		new_object->velX = obj->xDirection * 48;
		new_object->velY = -16;
		// OSI: Only exists onscreen.
		new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
		SD_PlaySound(CK_SOUNDNUM(SOUND_MASTERSHOT));
	}
}

// time is shooting flag

void CK5_MasterCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = ck_keenObj->posX - o1->posX ? IN_motion_Right : IN_motion_Left;
		o1->user1 = 1;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_MasterShoot0"));
		return;
	}

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

int osi_numMasterTeleDests;
void CK5_MasterTele(CK_object *obj)
{

	int posX_0, posY_0, tx = 0, ty = 0, tries, mapXT, mapYT, tile;
	int randomDest, destNum = 0;
	int closest_x, closest_y;
	long best_dist = 32768;
	CK_object *new_object;

	posX_0 = obj->posX;
	posY_0 = obj->posY;

	// Spawn sparks going in both directions
	if (OSI_IsObjectOnscreen(obj))
	{
		if ((new_object = CK_GetNewObj(true)))
		{
			new_object->posX = obj->posX;
			new_object->posY = obj->posY;
			new_object->velX = 0x30;
			new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
			CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MasterSparks0"));
		}

		if ((new_object = CK_GetNewObj(true)))
		{
			new_object->posX = obj->posX;
			new_object->posY = obj->posY;
			new_object->velX = -0x30;
			new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
			CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MasterSparks0"));
		}

		SD_PlaySound(CK_SOUNDNUM(SOUND_MASTERTELE));
	}
	// Pick a "random" destination.
	randomDest = US_RndT() % osi_numMasterTeleDests;
	//printf("Selected random destination %d of %d\n", randomDest, osi_numMasterTeleDests);
	
	// Find a teleportation destination
	for (int y = 0; y < CA_MapHeaders[ca_mapOn]->height; ++y)
	{
		for (int x = 0; x < CA_MapHeaders[ca_mapOn]->width; ++x)
		{
			int infoValue = CA_TileAtPos(x, y, 2);
			int keenDist_x = CK_Cross_abs(ck_keenObj->posX - RF_TileToUnit(x));
			int keenDist_y = CK_Cross_abs(ck_keenObj->posY - RF_TileToUnit(y));
			
			if (
				(infoValue != 0xA1 || ck_gameState.difficulty < D_Hard) &&
				(infoValue != 0x9E || ck_gameState.difficulty < D_Normal) &&
				(infoValue != 0x9C)
			)
				continue;
			
			if (keenDist_x + keenDist_y < CK_INT(OSI_MasterDontTeleDist, 0x100))
			{
				// TODO: Handle case where this is the random selection.
				destNum++;
				continue;
			}
			
			// We've found a potential spot
			if (keenDist_x + keenDist_y < best_dist)
			{
				best_dist = keenDist_x + keenDist_y;
				closest_x = x;
				closest_y = y;
			}
			//printf("MasterTele: Considering destination %d at (%d, %d)\n", destNum, x, y);
			//printf(" Distance from keen = %d (0x%x)\n", keenDist_x, keenDist_y);
			//printf(" Random selection was %d\n", randomDest);
			if (destNum++ == randomDest)
			{
				tx = x;
				ty = y;
			}
		}
	}
	
	if (best_dist >= CK_INT(CK5_MasterTeleKeenDist, 0x400))
	{
		//printf("No destination close to Keen (closest dist = %d/0x%x), using (%d, %d) instead.\n", best_dist, best_dist, tx, ty);
		closest_x = tx;
		closest_y = ty;
	} //else
		//printf("Teleporting to Keen (dist = %d/0x%x) -> target = (%d, %d)\n", best_dist, best_dist, closest_x, closest_y);

	// We're not teleporting if we have no desitnation.
	if (!tx && !ty)
		return;
	
	// Set the new position and clipping rectangle of the master
	obj->posX = RF_TileToUnit(closest_x);
	obj->posY = RF_TileToUnit(closest_y);
	obj->clipRects.tileX1 = closest_x - 1;
	obj->clipRects.tileX2 = closest_x + 4;
	obj->clipRects.tileY1 = closest_y - 1;
	obj->clipRects.tileY2 = closest_y + 4;

	// make it through previous nested loop == succesful tele
	ck_nextX = ck_nextY = 0;
	return;
}

// This was actually found in CK_MISC.C, which would suggest it had a more
// general use  (changing action when floor is hit)
// But it ended up only being used for the master teleporting tile col
// so it goes here

void CK5_MasterTeleTileCol(CK_object *obj)
{
	if (obj->rightTI || obj->leftTI)
	{
		obj->velX = 0;
	}

	if (obj->bottomTI)
	{
		obj->velY = 0;
	}

	if (obj->topTI)
	{
		obj->velY = 0;
		if (!obj->currentAction->next)
		{
			CK_RemoveObj(obj);
			return;
		}
		else
		{
			CK_SetAction2(obj, obj->currentAction->next);
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK5_MasterBallCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		CK_RemoveObj(o1);
		return;
	}

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
		CK_RemoveObj(o1);
	}
}

void CK5_MasterBallTileCol(CK_object *obj)
{

	if (obj->rightTI || obj->leftTI)
		obj->velX = -obj->velX;
	if (obj->bottomTI)
		obj->velY = 0;
	if (obj->topTI)
	{
		//turn ball into sparks, spawn sparks in reverse direction
		if (OSI_IsObjectOnscreen(obj))
			SD_PlaySound(CK_SOUNDNUM(SOUND_MASTERSHOT));
		CK_object *new_object;
		obj->velX = 48;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_MasterSparks0"));
		
		// These disappear when offscreen.
		obj->active = OBJ_EXISTS_ONLY_ONSCREEN;
		
		if ((new_object = CK_GetNewObj(true)))
		{
			new_object->posX = obj->posX;
			new_object->posY = obj->posY;
			new_object->velX = -48;

			new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
			CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MasterSparks0"));
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_MasterSparksTileCol(CK_object *obj)
{

	if (obj->rightTI || obj->leftTI)
	{
		CK_RemoveObj(obj);
		return;
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SpawnShikadi(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Shikadi;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - 0x100;
	new_object->user2 = 4;
	new_object->xDirection = (US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left);
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShikadiStand0"));
}

#define MISCFLAG_POLE 1

extern void CK_KeenSpecialColFunc(CK_object *, CK_object *);

void CK5_ShikadiWalk(CK_object *obj)
{
	int tx, tile;

	// By casting to unsigned, we also check if the difference in
	// Y values is greater than zero
	if (ck_keenObj->currentAction->collide == &CK_KeenSpecialColFunc || (unsigned)(obj->clipRects.unitY2 - ck_keenObj->clipRects.unitY2 + 0x100) <= 0x200)
	{
		// if Keen on Pole or Keen at shikadi level , get to keen
		// walk back and forth one tile to the left or right underneath him
		if (obj->posX > ck_keenObj->posX + 0x100)
			obj->xDirection = IN_motion_Left;
		else if (obj->posX < ck_keenObj->posX - 0x100)
			obj->xDirection = IN_motion_Right;

		tx = obj->xDirection == IN_motion_Right ? obj->clipRects.tileX2 : obj->clipRects.tileX1;

		if (ck_keenObj->clipRects.tileXmid != tx)
			return; // no zapping if not below keen
	}
	else
	{
		if (US_RndT() < 0x10)
		{
			// 1/16 chance of stopping walk
			ck_nextX = 0;
			obj->currentAction = CK_GetActionByName("CK5_ACT_ShikadiStand0");
			return;
		}

		//if at tile boundary (i.e., ready for zap)
		if ((obj->posX & 0xFF) || !CK_ObjectVisible(obj))
			return;

		tx = obj->xDirection == IN_motion_Right ? obj->clipRects.tileX2 : obj->clipRects.tileX1;
	}

	// Make a pole zap
	tile = CA_TileAtPos(tx, obj->clipRects.tileY1, 1);
	//tile = *MAPSPOT(tx, obj->clipRects.tileY1, FGPLANE);
	if (TI_ForeMisc(tile) == MISCFLAG_POLE)
	{

		obj->user1 = tx;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShikadiPole0");
		ck_nextX = 0;
		SD_PlaySound(CK_SOUNDNUM(SOUND_POLEZAP));
	}
}

// int33 is white flash duration
// state = health

void CK5_ShikadiCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}

	if (o2->type == CT_Stunner)
	{
		if (--o1->user2)
		{
			o1->user3 = 2;
			o1->visible = true;
			CK_ShotHit(o2);
		}
		else
		{

			o1->velX = o1->velY = 0;
			CK_StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_ShikadiStun0"));
		}
	}
}

void CK5_ShikadiPole(CK_object *obj)
{

	int sparkX;
	CK_object *new_object;

	obj->timeUntillThink = 2;
	sparkX = obj->xDirection == IN_motion_Right ? RF_TileToUnit(obj->clipRects.tileX2) : RF_TileToUnit(obj->clipRects.tileX1);
	new_object = CK_GetNewObj(true);
	new_object->posX = sparkX;
	new_object->posY = obj->posY + 0x80;
	new_object->type = CT5_EnemyShot;
	new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_PoleZap0"));
	new_object->yDirection = obj->posY > ck_keenObj->posY ? IN_motion_Up : IN_motion_Down;
	SD_PlaySound(CK_SOUNDNUM(SOUND_POLEZAP));
}

void CK5_PoleZap(CK_object *obj)
{

	int tile;
	if (ck_nextY == 0)
	{
		// Slide up until there's no more pole
		ck_nextY = obj->yDirection * 48;
		tile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);

		if (TI_ForeMisc(tile) != MISCFLAG_POLE)
			obj->currentAction = NULL;
	}
}

void CK5_ShikadiTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

	//if flashing white
	if (obj->user3 != 0)
	{
		obj->user3--;
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{

		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
}

void CK5_SpawnShocksund(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Shocksund;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - 0x80;
	new_object->user2 = 2;
	new_object->xDirection = IN_motion_Right;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Shocksund0"));
}

void CK5_ShocksundSearch(CK_object *obj)
{

	obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;

	if (US_RndT() < 0x10)
	{
		ck_nextX = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundStand0");
		obj->user1 = 0x10;
		return;
	}

	if (obj->clipRects.unitY2 != ck_keenObj->clipRects.unitY2)
	{

		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundJump0");
		obj->velX = obj->xDirection == IN_motion_Right ? 40 : -40;
		obj->velY = -40;
	}

	if (US_RndT() < 0x80)
	{

		ck_nextX = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundShoot0");
	}
}

void CK5_ShocksundStand(CK_object *obj)
{

	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_Shocksund0");
}

void CK5_ShocksundShoot(CK_object *obj)
{
	CK_object *new_object;
	int posX = obj->xDirection == IN_motion_Right ? obj->posX + 0x70 : obj->posX;

	if ((new_object = CK5_SpawnEnemyShot(posX, obj->posY + 0x40, CK_GetActionByName("CK5_ACT_BarkShot0"))))
	{

		new_object->velX = obj->xDirection * 60;
		new_object->xDirection = obj->xDirection;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SHOCKSUNDBARK));
	}
}

void CK5_ShocksundCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}

	if (o2->type == CT_Stunner)
	{
		if (--o1->user2)
		{
			o1->user3 = 2;
			o1->visible = true;
			CK_ShotHit(o2);
		}
		else
		{
			o1->velX = o1->velY = 0;
			CK_StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_ShocksundStun0"));
		}
	}
}

void CK5_ShocksundGroundTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{

		//if facing ck_keenObj, jump towards him, else turn around at edge
		if ((obj->xDirection == IN_motion_Right && ck_keenObj->posX > obj->posX) ||
			(obj->xDirection == IN_motion_Left && ck_keenObj->posX < obj->posX))
		{
			CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_ShocksundJump0"));
			obj->velX = obj->xDirection == IN_motion_Right ? 40 : -40;
			obj->velY = -40;
		}
		else
		{
			obj->posX -= obj->deltaPosX;
			obj->xDirection = -obj->xDirection;
			obj->timeUntillThink = US_RndT() / 32;
			CK_SetAction2(obj, obj->currentAction);
		}
	}

	//if flashing white
	if (obj->user3)
	{
		obj->user3--; //flash counter
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{

		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
}

void CK5_ShocksundJumpTileCol(CK_object *obj)
{

	if (obj->topTI)
	{
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Shocksund0"));
	}
	if (obj->user3)
	{
		obj->user3--; //flash counter
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{

		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
}

void CK5_BarkShotCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
		return;
	}

	if (o2->type == CT_Stunner)
	{

		CK_ShotHit(o2);
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
	}
}

void CK5_BarkShotTileCol(CK_object *obj)
{

	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_BARKSHOTDIE));

		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SpawnSphereful(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Sphereful;
	new_object->clipped = CLIP_simple;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - 0x100;
	new_object->zLayer = 1;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Sphereful0"));
}

void CK5_SpherefulBounce(CK_object *obj)
{
	obj->visible = true;

	if (ck_nextX || ck_nextY)
		return;

	int32_t lastTimeCount = SD_GetLastTimeCount();
	// Bounce around, and home in on Keen
	for (int32_t ticks_0 = lastTimeCount - SD_GetSpriteSync(); ticks_0 < lastTimeCount; ticks_0++)
	{
		if (!(ticks_0 & 0xF))
		{
			if (obj->velY < 8)
				obj->velY++;

			if (ck_keenObj->posX > obj->posX && obj->velX < 8)
				obj->velX++;

			if (ck_keenObj->posX < obj->posX && obj->velX > -8)
				obj->velX--;
		}
		ck_nextY += obj->velY;
		ck_nextX += obj->velX;
	}
}

void CK5_SpherefulTileCol(CK_object *obj)
{
	int16_t zLayer;
	uint16_t time, diamond_chunk;
	static uint16_t diamondpos[16] = {0x180, 0x171, 0x148, 0x109, 0x0C0, 0x077, 0x03A, 0x00F,
		0x000, 0x00F, 0x03A, 0x077, 0x0C0, 0x109, 0x148, 0x171};

	// Make the sphereful bounce off of walls
	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = -obj->velX;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPHEREFULCEILING));
	}

	if (obj->bottomTI)
	{
		obj->velY = -obj->velY;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPHEREFULCEILING));
	}

	if (obj->topTI)
	{
		obj->velY = -obj->velY;
		if (ck_keenObj->posY < obj->posY)
			obj->velY--;
		else
			obj->velY++;

		if (obj->velY > -4)
			obj->velY = -4;
		else if (obj->velY < -12)
			obj->velY = -12;
		/* FIXME: Any better name for (shared) sound? */
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPINDREDFLYUP));
	}

	// First draw the sphereful
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);

	//draw the diamonds
	time = (SD_GetLastTimeCount() >> 3) & 0xF;

	diamond_chunk = (time >> 2) + 362;
	zLayer = (time >= 8) ? 2 : 0;

	//topleft
	RF_AddSpriteDrawUsing16BitOffset(&obj->user1, obj->posX + diamondpos[time], obj->posY + diamondpos[time], diamond_chunk, 0, zLayer);

	//topright
	RF_AddSpriteDrawUsing16BitOffset(&obj->user2, obj->posX + 0x180 - diamondpos[time], obj->posY + diamondpos[time], diamond_chunk, 0, zLayer);

	time = (time + 8) & 0xF;
	zLayer = (time >= 8) ? 2 : 0;

	//botleft
	RF_AddSpriteDrawUsing16BitOffset(&obj->user3, obj->posX + diamondpos[time], obj->posY + diamondpos[time], diamond_chunk, 0, zLayer);

	//botright
	RF_AddSpriteDrawUsing16BitOffset(&obj->user4, obj->posX + 0x180 - diamondpos[time], obj->posY + diamondpos[time], diamond_chunk, 0, zLayer);
}

// Korath Funcs

void CK5_KorathWalk(CK_object *obj)
{
	if (US_RndT() < 10)
	{

		ck_nextX = 0;
		obj->xDirection = US_RndT() < 128 ? 1 : -1;
		obj->currentAction = CK_GetActionByName("CK5_ACT_KorathWait");
	}
}

void CK5_KorathColFunc(CK_object *obj, CK_object *other)
{
	//if keen and pushable
	if (other->type == CT_Player && other->currentAction->collide)
	{

		CK_PhysPushX(other, obj);
	}
	else if (other->type == CT_Stunner)
	{
		CK_StunCreature(obj, other, CK_GetActionByName("CK5_ACT_KorathStun0"));
	}
}

void CK5_SpawnKorath(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);

	obj->type = 23;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 128;
	obj->xDirection = US_RndT() < 128 ? 1 : -1;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_KorathWalk1"));
}

void CK5_QEDSpawn(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_QED;
	new_object->active = OBJ_ACTIVE;
	new_object->clipRects.tileX1 = tileX;
	new_object->clipRects.tileY1 = tileY;
	new_object->clipRects.tileX2 = new_object->clipRects.tileX1 + 1;
	new_object->clipRects.tileY2 = new_object->clipRects.tileY1 + 1;
	new_object->clipRects.unitX1 = RF_TileToUnit(tileX) - 0x10;
	new_object->clipRects.unitY1 = RF_TileToUnit(tileY) - 0x10;
	new_object->clipRects.unitX2 = new_object->clipRects.unitX1 + 0x220;
	new_object->clipRects.unitY2 = new_object->clipRects.unitY1 + 0x220;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_QED"));
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
	CK_ACT_AddColFunction("CK5_MineCol", &CK5_MineCol);
	CK_ACT_AddColFunction("CK5_MineShrapCol", &CK5_MineShrapCol);
	CK_ACT_AddFunction("CK5_MineTileCol", &CK5_MineTileCol);

	// Robo Red
	CK_ACT_AddFunction("CK5_RoboMove", &CK5_RoboMove);
	CK_ACT_AddColFunction("CK5_RoboCol", &CK5_RoboCol);
	CK_ACT_AddFunction("CK5_RoboShoot", &CK5_RoboShoot);
	CK_ACT_AddColFunction("CK5_RoboShotCol", &CK5_RoboShotCol);
	CK_ACT_AddFunction("CK5_RoboShotTileCol", &CK5_RoboShotTileCol);

	// Spirogrip
	CK_ACT_AddFunction("CK5_SpirogripSpin", &CK5_SpirogripSpin);
	CK_ACT_AddFunction("CK5_SpirogripFlyDraw", &CK5_SpirogripFlyDraw);

	// Spindred
	CK_ACT_AddFunction("CK5_SpindredBounce", &CK5_SpindredBounce);
	CK_ACT_AddFunction("CK5_SpindredTileCol", &CK5_SpindredTileCol);

	// Shikadi Master
	CK_ACT_AddFunction("CK5_MasterStand", &CK5_MasterStand);
	CK_ACT_AddFunction("CK5_MasterShoot", &CK5_MasterShoot);
	CK_ACT_AddColFunction("CK5_MasterCol", &CK5_MasterCol);
	CK_ACT_AddFunction("CK5_MasterTele", &CK5_MasterTele);
	CK_ACT_AddFunction("CK5_MasterTeleTileCol", &CK5_MasterTeleTileCol);
	CK_ACT_AddColFunction("CK5_MasterBallCol", &CK5_MasterBallCol);
	CK_ACT_AddFunction("CK5_MasterBallTileCol", &CK5_MasterBallTileCol);
	CK_ACT_AddFunction("CK5_MasterSparksTileCol", &CK5_MasterSparksTileCol);

	// Shikadi
	CK_ACT_AddFunction("CK5_ShikadiWalk", &CK5_ShikadiWalk);
	CK_ACT_AddColFunction("CK5_ShikadiCol", &CK5_ShikadiCol);
	CK_ACT_AddFunction("CK5_ShikadiPole", &CK5_ShikadiPole);
	CK_ACT_AddFunction("CK5_PoleZap", &CK5_PoleZap);
	CK_ACT_AddFunction("CK5_ShikadiTileCol", &CK5_ShikadiTileCol);
	CK_ACT_AddFunction("CK5_ShocksundSearch", &CK5_ShocksundSearch);
	CK_ACT_AddFunction("CK5_ShocksundStand", &CK5_ShocksundStand);
	CK_ACT_AddFunction("CK5_ShocksundShoot", &CK5_ShocksundShoot);
	CK_ACT_AddColFunction("CK5_ShocksundCol", &CK5_ShocksundCol);
	CK_ACT_AddFunction("CK5_ShocksundGroundTileCol", &CK5_ShocksundGroundTileCol);
	CK_ACT_AddFunction("CK5_ShocksundJumpTileCol", &CK5_ShocksundJumpTileCol);
	CK_ACT_AddColFunction("CK5_BarkShotCol", &CK5_BarkShotCol);
	CK_ACT_AddFunction("CK5_BarkShotTileCol", &CK5_BarkShotTileCol);

	// Sphereful
	CK_ACT_AddFunction("CK5_SpherefulBounce", &CK5_SpherefulBounce);
	CK_ACT_AddFunction("CK5_SpherefulTileCol", &CK5_SpherefulTileCol);

	// Korath
	CK_ACT_AddFunction("CK5_KorathWalk", &CK5_KorathWalk);
	CK_ACT_AddColFunction("CK5_KorathColFunc", &CK5_KorathColFunc);

	// QED
}
