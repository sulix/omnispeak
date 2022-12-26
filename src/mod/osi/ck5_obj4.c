/*
Commander Keen in Operation Station Inflitration
Copyright (C) 2022 Ceilick, David Gow

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

#include "id_heads.h"
#include "id_rf.h"
#include "id_us.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

#include <stdio.h>

void CK5_MultiShotDraw(CK_object *obj)
{
	if (obj->user4)
	{
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
		obj->user4--;
	}
	else
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

/* SpawnEnemyShot, but without the stuck-in-wall check. */
CK_object *OSI_SpawnFlame(int posX, int posY, CK_action *action)
{
	CK_object *new_object = CK_GetNewObj(true);

	if (!new_object)
		return NULL;

	new_object->posX = posX;
	new_object->posY = posY;
	new_object->type = CT5_EnemyShot;
	new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, action);

	
	return new_object;
}

void CK5_SpawnBigAmpton(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_BigAmpton;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - CK_INT(OSI_BigAmptonSpawnOffsetY, 0x280);// RF_TileToUnit(3) + RF_PixelToUnit(8);
	new_object->xDirection = (US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left);
	new_object->yDirection = IN_motion_Down;
	new_object->user1 = CK_INT(OSI_BigAmptonHP,3);
	new_object->user2 = 0;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_BigAmpton0"));
}

int osi_screenShakeOffset;
void CK5_BigAmptonWalk(CK_object *obj)
{

	//play tic toc sound
	if (obj->currentAction == CK_GetActionByName("CK5_ACT_BigAmpton0"))
	{
		SD_PlaySound(CK_SOUNDNUM(SND_BigAmptonWalk1));
		osi_screenShakeOffset = CK_INT(OSI_BigAmptonShakeAmt1, 10);
	}
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_BigAmpton2"))
	{
		SD_PlaySound(CK_SOUNDNUM(SND_BigAmptonWalk2));
		//TODO (OSI): ScreenShake
		osi_screenShakeOffset = CK_INT(OSI_BigAmptonShakeAmt2, 10);
	}

	if (US_RndT() < CK_INT(OSI_BigAmptonTurnChance, 0x10))
	{
		obj->xDirection = IN_motion_None;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BigAmptonTurn0"));
	}
}

/* Pick a random direction when finished turning. */
void CK5_BigAmptonChooseDir(CK_object *obj)
{
	obj->xDirection = (US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left);
}

void CK5_BigAmptonCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner && !b->user4)
	{
		if (a->xDirection == IN_motion_None || b->xDirection == IN_motion_None ||  a->xDirection == b->xDirection)
		{
			if (a->user1--)
			{
				a->user4 = CK_INT(OSI_BigAmptonShotFlashFrames, 3);
				CK_ShotHit(b);
			}
			else
			{
				SD_PlaySound(CK_SOUNDNUM(SND_BigAmptonDie));
				CK_StunCreature(a, b, CK_GetActionByName("CK5_ACT_BigAmptonStunned0"));
			}
		}
		else if (a->xDirection != b->xDirection)
		{
			b->xDirection = a->xDirection;
			b->user4 = 1; // Shot is hazardous to keen now
			SD_PlaySound(CK_SOUNDNUM(SND_KeenBulletReflect));
		}
	}

}

void CK5_BigAmptonTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		//CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BigAmptonTurn0"));
	}

	if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		//CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BigAmptonTurn0"));
	}

	if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		//CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BigAmptonTurn0"));
	}

	if (obj->user4)
	{
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
		obj->user4--;
	}
	else
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

#define ROBO_TOP_DIST 0x40
#define ROBO_FRONT_DIST 0x120
#define ROBO_BACK_DIST 0x20
#define ROBO_BOTTOM_DIST 0x30

bool OSI_RoboIsKeenNear(CK_object *obj, int searchRadius)
{
	// Find if keen is near.
	// The original Robo Red just checks if Keen is within the same Y-range,
	// but we'll check a bigger box around the robot.

	if (ck_keenObj->clipRects.unitY2 < obj->clipRects.unitY1 - CK_INT(OSI_RoboSearchTopDist, 0x40) * searchRadius)
		return false;

	if (ck_keenObj->clipRects.unitY1 > obj->clipRects.unitY2 + CK_INT(OSI_RoboSearchBottomDist, 0x20) * searchRadius)
		return false;
	
	if (obj->xDirection == IN_motion_Left)
	{
		if (ck_keenObj->clipRects.unitX2 < obj->clipRects.unitX1 - CK_INT(OSI_RoboSeachFrontDist, 0x120) * searchRadius)
			return false;

		if (ck_keenObj->clipRects.unitX1 > obj->clipRects.unitX2 + CK_INT(OSI_RoboSearchBackDist, 0x20) * searchRadius)
			return false;
	}
	else
	{
		if (ck_keenObj->clipRects.unitX2 < obj->clipRects.unitX1 - CK_INT(OSI_RoboSearchBackDist, 0x20) * searchRadius)
			return false;

		if (ck_keenObj->clipRects.unitX1 > obj->clipRects.unitX2 + CK_INT(OSI_RoboSeachFrontDist, 0x120) * searchRadius)
			return false;
	}
	
	return true;
}

// Robo Blue

void CK5_SpawnRoboBlue(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT5_Robo;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(OSI_RoboBlueSpawnOffsetY, 0x380);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_None;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_RoboBlue0"));
}

void CK5_RoboBlueMove(CK_object *obj)
{

	// Check for keen nearby (1/8 chance of firing)
	if (!(obj->posX & 0x40) && OSI_RoboIsKeenNear(obj, 0x10) && US_RndT() < 0x20)
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->user1 = CK_INT(OSI_RoboBlueNumMissiles, 4);
		obj->user2 = 1;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboBlueShoot0");
	}
	// Otherwise, (1/64) chance
	else if (!(obj->posX & 0x40) && US_RndT() < 0x04)
	{
		obj->user1 = CK_INT(OSI_RoboBlueNumMissiles, 4);
		obj->user2 = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboBlueShoot0");
	}
}
// time is number of shots

void CK5_RoboBlueCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = o1->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		o1->user1 = CK_INT(OSI_RoboBlueNumMissiles, 4);
		o1->user2 = 1;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboBlueShoot0"));
		return;
	}
	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK5_RoboBlueShoot(CK_object *obj)
{

	CK_object *new_object;
	int shotSpawnX;

	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboBlue0");

	shotSpawnX = obj->xDirection == IN_motion_Right ? obj->posX + CK_INT(OSI_RoboBlueMissileSpawnXRight, 0x280) : obj->posX  + CK_INT(OSI_RoboBlueMissileSpawnXLeft, 0);

	if ((new_object = CK5_SpawnEnemyShot(shotSpawnX, obj->posY + CK_INT(OSI_RoboBlueMissileSpawnY, 0x200), CK_GetActionByName("CK5_ACT_RoboBlueMissile0"))))
	{
		new_object->xDirection = obj->xDirection;
		new_object->velX = obj->xDirection * CK_INT(OSI_RoboBlueMissileSpeed, 50);
		new_object->zLayer = 3;
		if (!obj->user2)
			new_object->velY = obj->user1 & 1 ? -8 : 8;
		else
			new_object->velY = 0;
		new_object->user1 = obj->user2;
		SD_PlaySound(CK_SOUNDNUM(SND_BlueMissileLaunch));
		ck_nextX = obj->xDirection == IN_motion_Right ? -0x40 : 0x30;
	}
}

void CK5_RoboBlueMissileCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		CK_KillKeen();
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboBlueMissileHit0"));
	}
	else if (o2->type == CT_Stunner)
	{
		SD_PlaySound(CK_SOUNDNUM(SND_BlueMissileExplode));
		CK_ShotHit(o2);
	}
}

void CK5_RoboBlueMissileTileCol(CK_object *obj)
{

	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SND_BlueMissileExplode));
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_RoboBlueMissileHit0"));
	}
	else if (obj->velX < 0 && obj->posX > ck_keenObj->posX)
	{
		obj->velY += (obj->posY > ck_keenObj->posY) ? -1 : 1;
	}
	else if (obj->velX > 0 && obj->posX < ck_keenObj->posX)
	{
		obj->velY += (obj->posY > ck_keenObj->posY) ? -1 : 1;
	}
	

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Robo Green

void CK5_SpawnRoboGreen(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT5_Robo;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(OSI_RoboGreenSpawnOffsetY, 0x400);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_None;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_RoboGreen0"));
}

void CK5_RoboGreenMove(CK_object *obj)
{

	// Check for keen nearby (1/4 chance of firing)
	if (!(obj->posX & 0x40) && OSI_RoboIsKeenNear(obj, 0x07) && US_RndT() < CK_INT(OSI_RoboGreenFireChance, 0x40))
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenShoot0");
	}
	// Otherwise, (1/32) chance of randomly stopping and thinking
	else if (!(obj->posX & 0x40) && US_RndT() < CK_INT(OSI_RoboGreenLookChance, 0x20))
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenLook0");
	}
}

void CK5_RoboGreenLookThink(CK_object *obj)
{
	// So, here's what Robo Green looks for, and thinks about.
	// - Changes direction.
	// - If keen is very near, flame immediately.
	// - If keen is slightly near, stay in that direction.
	// - Else, randomly switch direction.
	
	if (OSI_RoboIsKeenNear(obj, CK_INT(OSI_RoboGreenSearchRadius, 0x07)))
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenShoot0");
	}
	else if (OSI_RoboIsKeenNear(obj, CK_INT(OSI_RoboGreenLookRadius, 0x0B)))
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreen0");
	}
	else
	{
		obj->xDirection = -obj->xDirection;
	}
}

void CK5_RoboGreenCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = o1->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		o1->user1 = 4;
		o1->user2 = 1;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboGreenShoot0"));
		return;
	}
	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK5_RoboGreenShoot(CK_object *obj)
{

	CK_object *new_object;
	int shotSpawnX;

	shotSpawnX = obj->posX + ( (obj->xDirection == IN_motion_Right) ? CK_INT(OSI_RoboGreenFlameSpawnXRight, 0x360) : -CK_INT(OSI_RoboGreenFlameSpawnXLeft,0x280) );

	if ((new_object = OSI_SpawnFlame(shotSpawnX, obj->posY + CK_INT(OSI_RoboGreenFlameSpawnY, 0x220), CK_GetActionByName("CK5_ACT_RoboGreenFlame0"))))
	{
		new_object->xDirection = obj->xDirection;
		new_object->zLayer = 3;
		new_object->clipped = CLIP_not;
		new_object->user1 = CK_INT(OSI_RoboGreenNumFireballsBase, 4) + ((US_RndT() < 0x80) ? CK_INT(OSI_RoboGreenNumFireballsA, 2) : CK_INT(OSI_RoboGreenNumFireballsB, 1));
		new_object->user2 = CK_INT(OSI_RoboGreenFireballInterval, 0x15);
		SD_PlaySound(CK_SOUNDNUM(SND_GreenFlamer));
	}
}

void CK5_RoboGreenFlameThink(CK_object *obj)
{
	CK_object *new_object;
	int shotSpawnX;
	
	if (--obj->user2)
		return;

	obj->user2 = CK_INT(OSI_RoboGreenFireballInterval, 0x15);
	
	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenFlame3");


	shotSpawnX = (obj->xDirection == IN_motion_Right) ? obj->posX + CK_INT(OSI_RoboGreenFireballSpawnXR, 0x280) : obj->posX - CK_INT(OSI_RoboGreenFireballSpawnXL, 0);

	if ((new_object = CK5_SpawnEnemyShot(shotSpawnX, obj->posY + CK_INT(OSI_RoboGreenFireballSpawnY, 0), CK_GetActionByName("CK5_ACT_RoboGreenFireball0"))))
	{
		new_object->xDirection = obj->xDirection;
		new_object->velX = obj->xDirection * (CK_INT(OSI_RoboGreenFireballVelX, 70) - obj->user1 * 8);
		new_object->zLayer = 3;
		new_object->velY = CK_INT(OSI_RoboGreenFireballVelY, -30) + obj->user1 * 4;
		new_object->user1 = 45;
		SD_PlaySound(CK_SOUNDNUM(SND_GreenFlamePew));
	}
}


void CK5_RoboGreenFireballTileCol(CK_object *obj)
{
	// Have we hit the ground / a wall / etc?
	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		//SD_PlaySound(CK_SOUNDNUM(SOUND_ENEMYSHOTHIT));
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_RoboGreenFire0"));
	}
	
	// Fall in an arc
	CK_PhysGravityMid(obj);
	obj->velX *= 3;
	CK_PhysDampHorz(obj);
	obj->velX /= 3;

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}


void CK5_RoboGreenBigFireThink(CK_object *obj)
{
	CK_object *new_object;
	if (--obj->user1 == 0)
	{
		int oldheight = obj->clipRects.unitY2 - obj->clipRects.unitY1;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenFire2");
		obj->user1 = 30;
		VH_SpriteTableEntry *ste = VH_GetSpriteTableEntry(obj->currentAction->chunkLeft - ca_gfxInfoE.offSprites);
		int newheight = RF_PixelToUnit(ste->height);
		obj->posY += oldheight - newheight;
	}
}

void CK5_RoboGreenFireThink(CK_object *obj)
{
	CK_object *new_object;

	if (--obj->user1 == 0)
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboGreenAsh0");
		obj->zLayer = 0;
		//obj->user1 = 50;
	}
}

// Robo Yellow

void CK5_SpawnRoboYellow(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT5_Robo;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(OSI_RoboYellowSpawnOffsetY, 0x400);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_None;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_RoboYellow0"));
}

void CK5_RoboYellowMove(CK_object *obj)
{

	// Check fx18keen nearby (1/8 chance of firing)
	if ((obj->posX & 0x40) && OSI_RoboIsKeenNear(obj, 0x20) && US_RndT() < CK_INT(OSI_RoboYellowShootChance, 0x20))
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboYellowShoot0");
	}
	// Otherwise, (1/16) chance of randomly changing speed
	else if (!(obj->posX & 0x40) && US_RndT() < CK_INT(OSI_RoboYellowSpeedChance, 0x10))
	{
		if (obj->user1 == 0)
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_RoboYellowFast0");
			obj->user1 = 1;
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_RoboYellow0");
			obj->user1 = 0;
		}
	}
}

void CK5_RoboYellowCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = o1->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		o1->user1 = 4;
		o1->user2 = 1;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboYellowShoot0"));
		return;
	}
	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void OSI_GlideAndDie(CK_object *obj)
{
	ck_nextX = obj->velX * SD_GetSpriteSync();
	ck_nextY = obj->velY * SD_GetSpriteSync();
	if (obj->user1-- == 0)
		obj->currentAction = NULL;
}

void CK5_RoboYellowShoot(CK_object *obj)
{

	CK_object *new_object;
	int shotSpawnX;

	shotSpawnX = obj->posX + ( (obj->xDirection == IN_motion_Right) ? CK_INT(OSI_RoboYellowShotOffsetXRight, 0x180) : CK_INT(OSI_RoboYellowShotOffsetXLeft, 0) );

	if ((new_object = OSI_SpawnFlame(shotSpawnX, obj->posY + CK_INT(OSI_RoboYellowShotOffsetY, 0x180), CK_GetActionByName("CK5_ACT_RoboYellowWave0"))))
	{
		new_object->xDirection = obj->xDirection;
		new_object->zLayer = 3;
		new_object->user1 = CK_INT(OSI_RoboYellowWaveLifespan, 0x50);
		new_object->velX = obj->xDirection * 30;
		new_object->clipped = CLIP_not;
		SD_PlaySound(CK_SOUNDNUM(SND_YellowShoot));
	}
	
	/* Return to being slow after firing. */
	obj->user1 = 0;
}


// Sparky Jr.
void CK5_SpawnSparkyJr(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT5_SparkyJr;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(OSI_SparkyJrSpawnOffsetY, 0x120);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_None;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SparkyJr0"));
}

void CK5_SparkyJrWalk(CK_object *obj)
{
	//CK_PhysGravityMid(obj);
	if (US_RndT() < SD_GetSpriteSync() * CK_INT(OSI_SparkyJrJumpChance, 3))
	{
		/* NOTE: Vorticons use a horribly complicated table to calculate their jump heights. */
		int jumpHeight = CK_INT(OSI_SparkyJrMinJumpHeight, 0x10);
		const int jumpHeightDiff = CK_INT(OSI_SparkyJrMaxJumpHeight, 0x30) - CK_INT(OSI_SparkyJrMinJumpHeight, 0x10);
		jumpHeight += (US_RndT() * jumpHeightDiff) >> 8;
		obj->velY = -jumpHeight;
		obj->velX = CK_INT(OSI_SparkyJrJumpSpeed, 250) * obj->xDirection;
		SD_PlaySound(CK_SOUNDNUM(SND_SparkyJrJump));
		CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SparkyJrJump0"));
	}
}

void CK5_SparkyJrWalkTileCol(CK_object *obj)
{
	//printf("SparkyJr WTC State %x (velY = %d, posY = 0x%d, topTI = %d)\n", obj->currentAction->compatDosPointer, obj->velY, obj->posY, obj->topTI);
	if (!obj->topTI)
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_SparkyJrJump0");
	}
	
	if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		CK_PhysUpdateNormalObj(obj);
	}
	
	if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		CK_PhysUpdateNormalObj(obj);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SparkyJrJumpTileCol(CK_object *obj)
{
	
	//printf("SparkyJr JTC State %x (velY = %d, posY = 0x%x, topTi = %d)\n", obj->currentAction->compatDosPointer, obj->velX, obj->posX, obj->topTI);
	if (obj->topTI)
	{
		obj->velY = 0;
		// Drop half a tile down, to make sure we're touching the ground when grounded.
		//obj->posY += 0x80;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_SparkyJr0"));
	}
	
	if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		obj->posX -= 8;
		obj->velX = CK_INT(OSI_SparkyJrJumpSpeed, 250) * obj->xDirection;
		CK_PhysUpdateNormalObj(obj);
	}
	
	if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		obj->posX += 8;
		obj->velX = CK_INT(OSI_SparkyJrJumpSpeed, 250) * obj->xDirection;
		CK_PhysUpdateNormalObj(obj);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SparkyJrCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		o1->velX = 0;
		CK_StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_SparkyJrStunned0"));
	}
	if (o2->type == CT_Player)
	{
		CK_StunKeen(o2, o1);
	}
}

// Sparky III
bool OSI_Sparky3IsKeenNear(CK_object *obj, int searchRadius)
{
	// Find if keen is near.
	// The original Robo Red just checks if Keen is within the same Y-range,
	// but we'll check a bigger box around the robot.

	if (ck_keenObj->clipRects.unitY2 < obj->clipRects.unitY1 - searchRadius)
		return false;

	if (ck_keenObj->clipRects.unitY1 > obj->clipRects.unitY2 + searchRadius)
		return false;
	
	if (ck_keenObj->clipRects.unitX2 < obj->clipRects.unitX1 - searchRadius)
		return false;

	if (ck_keenObj->clipRects.unitX1 > obj->clipRects.unitX2 + searchRadius)
		return false;
	
	return true;
}

void CK5_SpawnSparky3(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Sparky3;
	new_object->clipped = CLIP_simple;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) - CK_INT(OSI_Sparky3SpawnOffsetY, 0x100);
	new_object->zLayer = 1;
	new_object->user3 = CK_INT(OSI_Sparky3HoverTime, 380);
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Sparky3Hover0"));
}

void CK5_Sparky3Hover(CK_object *obj)
{
	int desiredDirX = (US_RndT() & 1) ? IN_motion_Left : IN_motion_Right;
	int desiredDirY = (US_RndT() & 1) ? IN_motion_Up : IN_motion_Down;
	int accelX = (US_RndT() >> 7) * desiredDirX;
	int accelY = (US_RndT() >> 7) * desiredDirY;
	
	if (obj->topTI)
	{
		obj->user2 = -3;
		obj->velY = -3;
	}
	else if (obj->bottomTI)
	{
		obj->user2 = 3;
		obj->velY = 3;
	}
	
	if (obj->leftTI)
	{
		obj->user1 = -3;
		obj->velX = -3;
	}
	else if (obj->rightTI)
	{
		obj->user1 = 3;
		obj->velX = 3;
	}
	
	
	if (obj->user1 > accelX)
		obj->user1--;
	else if (obj->user1 < accelX)
		obj->user1++;
	if (obj->user2 > accelY)
		obj->user2--;
	else if (obj->user2 < accelY)
		obj->user2++;
	
	obj->velX += obj->user1;
	obj->velY += obj->user2;
	
	if (obj->velX < 0)
		obj->xDirection = IN_motion_Left;
	else if (obj->velX > 0)
		obj->xDirection = IN_motion_Right;
	
	ck_nextX += obj->velX;
	ck_nextY += obj->velY;
	
	obj->user3 -= SD_GetSpriteSync();
	if (obj->user3 <= 0)
	{
		obj->velY = 0;
		obj->user3 = CK_INT(OSI_Sparky3SearchTime, 320);
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Sparky3Search0"));
	}
}

void CK5_Sparky3Search(CK_object *obj)
{
	obj->velX = 0;
	obj->velY -= ((obj->posY & 0xFF) >> 5) - 4;
	//printf("posY = 0x%x, posY adj = %d, velY = %d\n", obj->posY, ((obj->posY & 0xFF) >> 5) - 4, obj->velY);
	ck_nextY = obj->velY;
	//printf("Search!\n");
	
	if (OSI_Sparky3IsKeenNear(obj, CK_INT(OSI_Sparky3SearchRadius, 0x500)))
	{
		obj->velY = 0;
		obj->user1 = ck_keenObj->posX - obj->posX;
		obj->user2 = ck_keenObj->posY - obj->posY;
		obj->user3 = CK_Cross_abs(obj->user1) + CK_Cross_abs(obj->user2);
		int sqrlength = obj->user1 * obj->user1 + obj->user2 + obj->user2;
		while (sqrlength > CK_INT(OSI_Sparky3ChargeSqrSpeed, 16384))
		{
			obj->user1 >>= 1;
			obj->user2 >>= 1;
			sqrlength = obj->user1 * obj->user1 + obj->user2 + obj->user2;
		}
		if (obj->user1 < 0)
			obj->xDirection = IN_motion_Left;
		else if (obj->user1 > 0)
			obj->xDirection = IN_motion_Right;
		//printf("Found Keen! vector = (%d, %d)\n", obj->user1, obj->user2);
		SD_PlaySound(CK_SOUNDNUM(SND_SparkyIIICharge));
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Sparky3Charge0"));
	}
	else
	{
		obj->user3 -= SD_GetSpriteSync();
		// Every 128 frames, look the other way.
		obj->xDirection = (obj->user3 & 0x80) ? IN_motion_Left : IN_motion_Right;
		
		if (obj->user3 <= 0)
		{
			obj->user1 = obj->user2 = 0;
			obj->user3 = CK_INT(OSI_Sparky3HoverTime, 280);
			CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Sparky3Hover0"));
		}
	}
}

void CK5_Sparky3Charge(CK_object *obj)
{
	ck_nextX += obj->user1;
	ck_nextY += obj->user2;
	obj->user3 -= CK_Cross_abs(obj->user1) + CK_Cross_abs(obj->user2);
	if (obj->user3 <= 0)
	{
		obj->user1 = obj->user2 = 0;
		obj->user3 = CK_INT(OSI_Sparky3HoverTime, 280);
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Sparky3Hover0"));
	}
}

void CK5_Sparky3TileCol(CK_object *obj)
{
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_Sparky3Col(CK_object *obj1, CK_object *obj2)
{
	if (obj2->type == CT_Stunner)
	{
		obj1->velX = 0;
		obj1->clipped = CLIP_normal;
		CK_StunCreature(obj1, obj2, CK_GetActionByName("CK5_ACT_Sparky3Stunned0"));
	}
	if (obj2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK5_SpawnVlorg(int tileX, int tileY)
{
	int i;
	CK_object *obj = CK_GetNewObj(false);

	obj->type = CT5_Vlorg; // ShikadiMine
	obj->active = OBJ_ACTIVE;
	obj->clipped = CLIP_not;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - CK_INT(OSI_VlorgSpawnOffsetY, 0x200);
	obj->zLayer = 3;
	
	obj->user1 = CK_INT(OSI_VlorgSoundTimer, 70);

	// X and Y offsets of the Dot relative to the mine
	obj->user2 = CK_INT(OSI_VlorgEyeXOffset, 0xB0);
	obj->user3 = CK_INT(OSI_VlorgEyeYOffset, 0xB0);
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_VlorgChase0"));

	return;
}

void CK5_VlorgMove(CK_object *obj)
{

	int16_t deltaX, deltaY, delta, xDir, yDir;
	int16_t soundDist;

	// Get distance to keen
	deltaX = ck_keenObj->posX - obj->posX;
	deltaY = ck_keenObj->posY - obj->posY;
	
	soundDist = (CK_Cross_abs(deltaX) + CK_Cross_abs(deltaY)) >> 7;
	soundDist = CK_Cross_max(soundDist, CK_INT(OSI_VlorgSoundMinDistance, 9));
	//printf("soundDist = %d\n", soundDist);
	
	if ((deltaX < 0 && ck_keenObj->xDirection == IN_motion_Right)
		|| (deltaX > 0 && ck_keenObj->xDirection == IN_motion_Left))
	{
		if (CK_INT(OSI_VlorgHidingSoundTimer, 4))
		{
			obj->user1 -= SD_GetSpriteSync();
			if (obj->user1 <= 0)
			{
				SD_PlaySound(CK_SOUNDNUM(SND_Vlorg));
				obj->user1 = CK_INT(OSI_VlorgHidingSoundTimer, 4) * soundDist;
			}
		}
		obj->velX = 0;
		obj->velY = 0;
		ck_nextX = 0;
		ck_nextY = 0;
		return;
	}
	
	/* If the sound timer is greater than it should be, reset it and play a sound immediately.
	 * This ensures that we start playing at the faster rate immediately when turning around.
	 */
	
	if (obj->user1 > CK_INT(OSI_VlorgSoundTimer, 2) * soundDist)
		obj->user1 = 0;
	else
		obj->user1 -= SD_GetSpriteSync();
	
	if (obj->user1 <= 0)
	{
		SD_PlaySound(CK_SOUNDNUM(SND_Vlorg));
		obj->user1 = CK_INT(OSI_VlorgSoundTimer, 2) * soundDist;
	}
	
	if (CK_Cross_abs(deltaX) > CK_Cross_abs(deltaY))
	{
		obj->velX = deltaX;
		obj->velY = deltaY;
		while (CK_Cross_abs(obj->velX) > CK_INT(OSI_VlorgMaxSpeedLongComponent, 12) * SD_GetSpriteSync())
		{
			obj->velX = (obj->velX / 2);
			obj->velY = (obj->velY / 2);
		}
	}
	else
	{
		obj->velX = deltaX;
		obj->velY = deltaY;
		while (CK_Cross_abs(obj->velY) > CK_INT(OSI_VlorgMaxSpeedLongComponent, 12) * SD_GetSpriteSync())
		{
			obj->velX = (obj->velX / 2);
			obj->velY = (obj->velY / 2);
		}
	}
	
	if (CK_Cross_abs(deltaY) > (3 * CK_Cross_abs(deltaX)))
	{
		obj->xDirection = IN_motion_None;
		if (deltaY > 0)
			obj->yDirection = IN_motion_Down;
		else
			obj->yDirection = IN_motion_Up;
	}
	else
	{
		obj->yDirection = IN_motion_None;
		if (deltaX > 0)
			obj->xDirection = IN_motion_Right;
		else
			obj->xDirection = IN_motion_Left;
	}

	//printf("VlorgMove: deltaX = %x, deltaY = %x, velX = %d, velY = %d\n", deltaX, deltaY, obj->velX, obj->velY);
	
	ck_nextX += obj->velX;
	ck_nextY += obj->velY;
	return;
}

void CK5_VlorgCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		RF_RemoveSpriteDrawUsing16BitOffset(&o1->user4);
		o1->velX = 0; /* We use velX as a timer, so we don't want to shoot off to the right! */
		
		/* We want to take the eye's position when we're stunned, so add it now. */
		o1->posX += o1->user2;
		o1->posY += o1->user3;
		CK_StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_VlorgStunned0"));
		o1->clipped = CLIP_normal;
		//CK_ShotHit(o2);
	}
	else if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
	return;
}

void CK5_VlorgDraw(CK_object *obj)
{
	int eyeChunk = CK_CHUNKNUM(SPR_VLORGSTUNNED);
	if (obj->xDirection == IN_motion_Left)
		eyeChunk = CK_CHUNKNUM(SPR_VLORGLEFT);
	else if (obj->yDirection == IN_motion_Up)
		eyeChunk = CK_CHUNKNUM(SPR_VLORGUP);
	else if (obj->xDirection == IN_motion_Right)
		eyeChunk = CK_CHUNKNUM(SPR_VLORGRIGHT);
	else if (obj->yDirection == IN_motion_Down)
		eyeChunk = CK_CHUNKNUM(SPR_VLORGDOWN);

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
		obj->zLayer); //vlorg
	RF_AddSpriteDrawUsing16BitOffset(&obj->user4, obj->posX + obj->user2,
		obj->posY + obj->user3, eyeChunk, 0, 3); //dot
	return;
}


void CK_LethalCol(CK_object *a, CK_object *b);
void CK_BasicDrawFunc1(CK_object *a);

void CK_NullCol(CK_object *a, CK_object *b)
{
	if (CK_INT(OSI_RoboGreenFlameVisible, 1))
		CK_LethalCol(a,b);

}

void CK_NullFn(CK_object *a)
{
	if (CK_INT(OSI_RoboGreenFlameVisible, 1))
		CK_BasicDrawFunc1(a);
}


void OSI_Obj4_SetupFunctions(void)
{
	//Common
	CK_ACT_AddFunction("CK5_MultiShotDraw", &CK5_MultiShotDraw);

	//Big Ampton
	CK_ACT_AddFunction("CK5_BigAmptonWalk", &CK5_BigAmptonWalk);
	CK_ACT_AddFunction("CK5_BigAmptonChooseDir", &CK5_BigAmptonChooseDir);
	CK_ACT_AddColFunction("CK5_BigAmptonCol", &CK5_BigAmptonCol);
	CK_ACT_AddFunction("CK5_BigAmptonTileCol", &CK5_BigAmptonTileCol);

	//Robo Blue
	CK_ACT_AddFunction("CK5_RoboBlueMove", &CK5_RoboBlueMove);
	CK_ACT_AddFunction("CK5_RoboBlueShoot", &CK5_RoboBlueShoot);
	CK_ACT_AddColFunction("CK5_RoboBlueCol", &CK5_RoboBlueCol);
	CK_ACT_AddColFunction("CK5_RoboBlueMissileCol", &CK5_RoboBlueMissileCol);
	CK_ACT_AddFunction("CK5_RoboBlueMissileTileCol", &CK5_RoboBlueMissileTileCol);
	
	//Robo Green
	CK_ACT_AddFunction("CK5_RoboGreenMove", &CK5_RoboGreenMove);
	CK_ACT_AddFunction("CK5_RoboGreenLookThink", &CK5_RoboGreenLookThink);
	CK_ACT_AddFunction("CK5_RoboGreenShoot", &CK5_RoboGreenShoot);
	CK_ACT_AddColFunction("CK5_RoboGreenCol", &CK5_RoboGreenCol);
	CK_ACT_AddFunction("CK5_RoboGreenFlameThink", &CK5_RoboGreenFlameThink);
	CK_ACT_AddFunction("CK5_RoboGreenFireballTileCol", &CK5_RoboGreenFireballTileCol);
	CK_ACT_AddFunction("CK5_RoboGreenBigFireThink", &CK5_RoboGreenBigFireThink);
	CK_ACT_AddFunction("CK5_RoboGreenFireThink", &CK5_RoboGreenFireThink);
	{
		CK_ACT_AddColFunction("OSI_FlameLethalCol", CK_NullCol);
		CK_ACT_AddFunction("OSI_FlameBasicDrawFunc1", CK_NullFn);
	}

	
	//Robo Yellow
	CK_ACT_AddFunction("CK5_RoboYellowMove", &CK5_RoboYellowMove);
	CK_ACT_AddColFunction("CK5_RoboYellowCol", &CK5_RoboYellowCol);
	CK_ACT_AddFunction("CK5_RoboYellowShoot", &CK5_RoboYellowShoot);
	
	//Sparky Jr
	CK_ACT_AddFunction("CK5_SparkyJrWalk", &CK5_SparkyJrWalk);
	CK_ACT_AddFunction("CK5_SparkyJrWalkTileCol", &CK5_SparkyJrWalkTileCol);
	CK_ACT_AddFunction("CK5_SparkyJrJumpTileCol", &CK5_SparkyJrJumpTileCol);
	CK_ACT_AddColFunction("CK5_SparkyJrCol", &CK5_SparkyJrCol);
	
	//Sparky 3
	CK_ACT_AddFunction("CK5_Sparky3Hover", &CK5_Sparky3Hover);
	CK_ACT_AddFunction("CK5_Sparky3Search", &CK5_Sparky3Search);
	CK_ACT_AddFunction("CK5_Sparky3Charge", &CK5_Sparky3Charge);
	CK_ACT_AddColFunction("CK5_Sparky3Col", &CK5_Sparky3Col);
	CK_ACT_AddFunction("CK5_Sparky3TileCol", &CK5_Sparky3TileCol);
	
	//Vlorg
	CK_ACT_AddFunction("CK5_VlorgMove", &CK5_VlorgMove);
	CK_ACT_AddColFunction("CK5_VlorgCol", &CK5_VlorgCol);
	CK_ACT_AddFunction("CK5_VlorgDraw", &CK5_VlorgDraw);
	
	CK_ACT_AddFunction("OSI_GlideAndDie", &OSI_GlideAndDie);
}
