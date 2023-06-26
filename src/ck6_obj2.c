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
#include "id_rf.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck6_ep.h"

#include <stdio.h>

// =========================================================================

// Nospikes
void CK6_SpawnNospike(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Nospike;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_NospikeSpawnYOffset, -0x180);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_NospikeSit0"));
	obj->user4 = CK_INT(CK6_NospikeHealth, 4);
}

void CK6_NospikeWalk(CK_object *obj)
{
	if (US_RndT() < CK_INT(CK6_NospikeSitChance, 0x10))
	{
		obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeSit0");
	}
	else if (obj->clipRects.unitY2 == ck_keenObj->clipRects.unitY2 && US_RndT() <= CK_INT(CK6_NospikeChargeChance, 0x20))
	{
		obj->xDirection = ck_keenObj->posX > obj->posX ? IN_motion_Right : IN_motion_Left;
		obj->user1 = 0;
		obj->user2 = 1;

		if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk0"))
			obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeCharge1");
		else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk1"))
			obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeCharge2");
		else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk2"))
			obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeCharge3");
		else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk3"))
			obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeCharge0");
	}
}

void CK6_NospikeCharge(CK_object *obj)
{
	if (obj->user1 == 0)
	{
		if (((ck_keenObj->clipRects.unitY2 != obj->clipRects.unitY2 ||
			     (obj->xDirection == IN_motion_Left && obj->posX < ck_keenObj->posX) ||
			     (obj->xDirection == IN_motion_Right && obj->posX > ck_keenObj->posX)) &&
			    US_RndT() < CK_INT(CK6_NospikeGiveUpChance, 0x8)) ||
			!CK_ObjectVisible(obj))
		{
			// Stop charging if nospike gets bored, or goes off screen
			obj->user2 = 0;
			if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeCharge0"))
				obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeWalk1");
			else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeCharge1"))
				obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeWalk2");
			else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeCharge2"))
				obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeWalk3");
			else if (obj->currentAction == CK_GetActionByName("CK6_ACT_NospikeCharge3"))
				obj->currentAction = CK_GetActionByName("CK6_ACT_NospikeWalk0");
		}
	}
}

void CK6_NospikeCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		if (--a->user4 == 0)
		{
			CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_NospikeStunned0"));
		}
		else
		{
			a->xDirection = ck_keenObj->posX < a->posX ? IN_motion_Left : IN_motion_Right;
			a->user2 |= 0x400;
			a->visible = true;

			if (a->currentAction == CK_GetActionByName("CK6_ACT_NospikeSit0") ||
				a->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk0"))
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_NospikeCharge1"));
			else if (a->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk1"))
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_NospikeCharge2"));
			else if (a->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk2"))
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_NospikeCharge3"));
			else if (a->currentAction == CK_GetActionByName("CK6_ACT_NospikeWalk3"))
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_NospikeCharge0"));

			CK_ShotHit(b);
		}
	}
	else if (b->type == CT6_Nospike)
	{
		// Two charging nospikes will stun each other if they collide head on
		if ((a->user2 & 0xFF) && (b->user2 & 0xFF) && a->xDirection != b->xDirection)
		{
			a->user1 = a->user2 = a->user3 = 0;
			b->user1 = b->user2 = b->user3 = 0;
			a->user4 = b->user4 = a->type;
			CK_SetAction2(a, CK_GetActionByName("CK6_ACT_NospikeStunned0"));
			CK_SetAction2(b, CK_GetActionByName("CK6_ACT_NospikeStunned0"));
			SD_PlaySound(CK_SOUNDNUM(SOUND_NOSPIKECOLLIDE));
			a->type = b->type = CT6_StunnedCreature;
			a->velY = b->velY = CK_INT(CK6_NospikeDeathHopYVel, -24);
		}
	}
}

void CK6_NospikeFall(CK_object *obj)
{
	RF_RemoveSpriteDrawUsing16BitOffset(&(obj->user3));
}

void CK6_NospikeFallDraw(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

	// Place the question mark
	RF_AddSpriteDrawUsing16BitOffset(&(obj->user3), obj->posX + CK_INT(CK6_NospikeQuestionXOffset, 0x100), obj->posY + CK_INT(CK6_NospikeQuestionYOffset, -0x80), CK_CHUNKNUM(SPR_NOSPIKE_QUESTIONMARK), false, 3);
}

void CK6_NospikeFallDraw2(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

	if (obj->topTI)
	{
		obj->user1 = obj->user2 = obj->user3;
		obj->user4 = obj->type;
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_NospikeStunned0"));
		SD_PlaySound(CK_SOUNDNUM(SOUND_NOSPIKECOLLIDE));
		obj->type = CT6_StunnedCreature;
		obj->velY = CK_INT(CK6_NospikeDeathHopYVel, -24);
	}
}

void CK6_NospikeChargeDraw(CK_object *obj)
{
	if (obj->topTI)
	{
		obj->user1 = 0;
		if (obj->rightTI || obj->leftTI)
		{
			obj->posX -= obj->xDirection * 128;
			CK_SetAction(obj, CK_GetActionByName("CK6_ACT_NospikeSit0")); // Not Setaction2
			RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
			obj->user2 = 0;
			return;
		}
	}
	else if (++obj->user1 == CK_INT(CK6_NospikeAirHangTime, 6))
	{
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_NospikeFall0"));
	}

	if (obj->user2 & 0xFF00)
	{
		uint16_t user2 = obj->user2 & 0xFFFF; // Need to use 16-bit arithmetic here
		user2 -= 0x100;
		obj->user2 = user2;
		RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, true, obj->zLayer);
	}
	else
	{
		RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
	}
}

// Giks
void CK6_SpawnGik(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Gik;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_GikWalk0"));
}

void CK6_GikWalk(CK_object *obj)
{
	if (obj->topTI != 9)
	{
		int dx = ck_keenObj->posX - obj->posX;

		int dy = obj->clipRects.unitY2 - ck_keenObj->clipRects.unitY2;
		if (dy >= CK_INT(CK6_GikJumpKeenYMin, 0) && dy <= CK_INT(CK6_GikJumpKeenYMax, 0x400))
		{
			obj->xDirection = dx < 0 ? IN_motion_Left : IN_motion_Right;
			if (dx >= -CK_INT(CK6_GikJumpKeenXRadiusMax, 0x700) && dx <= CK_INT(CK6_GikJumpKeenXRadiusMax, 0x700) && (dx <= -CK_INT(CK6_GikJumpKeenXRadiusMin, 0x100) || dx >= CK_INT(CK6_GikJumpKeenXRadiusMin, 0x100)))
			{
				obj->velX = dx < 0 ? -CK_INT(CK6_GikJumpXVel, 40) : CK_INT(CK6_GikJumpXVel, 40);
				obj->velY = CK_INT(CK6_GikJumpYVel, -28);
				obj->currentAction = CK_GetActionByName("CK6_ACT_GikJump0");
				SD_PlaySound(CK_SOUNDNUM(SOUND_GIKJUMP));
			}
		}
	}
}

static uint16_t gikslides_r[] = {0, 7, 0, 0, 0, 3, 3, 1};
static uint16_t gikslides_l[] = {0, 7, 3, 3, 1, 0, 0, 0};

void CK6_GikSlide(CK_object *obj)
{
	CK_PhysGravityHigh(obj);

	int topflag = obj->topTI & 7;
	uint16_t di = obj->xDirection == IN_motion_Right ? gikslides_r[topflag] : gikslides_l[topflag];
	if (obj->velX == 0 && obj->topTI)
	{
		// Hit a wall and on the ground
		obj->currentAction = CK_GetActionByName("CK6_ACT_GikEndSlide0");
	}
	else
	{
		// Slide to a halt
		int32_t lastTimeCount = SD_GetLastTimeCount();
		for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
		{
			if (di)
			{
				uint32_t ddi = di;
				if ((ddi & tickCount) == 0)
				{
					if ((obj->velX < 0 && ++obj->velX == 0) || (obj->velX > 0 && --obj->velX == 0))
					{
						obj->currentAction = CK_GetActionByName("CK6_ACT_GikEndSlide0");
						break;
					}
				}
			}
			ck_nextX += obj->velX;
		}
	}
}

void CK6_GikJumpDraw(CK_object *obj)
{
	if (obj->rightTI || obj->leftTI)
		obj->velX = 0;

	if (obj->bottomTI)
		obj->velY = 0;

	if (obj->topTI)
	{
		obj->velY = 0;
		SD_PlaySound(CK_SOUNDNUM(SOUND_GIKLAND));
		CK_SetAction2(obj, obj->currentAction->next);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_GikSlideDraw(CK_object *obj)
{
	if ((obj->rightTI && obj->velX < 0) || (obj->leftTI && obj->velX > 0))
		obj->velX = 0;

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Orbatrices
void CK6_SpawnOrbatrix(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Orbatrix;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit((tileY)) + CK_INT(CK6_OrbatrixSpawnYOffset, -0x180);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	obj->user4 = 1;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_OrbatrixFloat0"));
}

void CK6_OrbatrixFloat(CK_object *obj)
{
	if (US_RndT() < CK_INT(CK6_OrbatrixIdleChance, 0x20))
	{
		obj->currentAction = CK_GetActionByName("CK6_ACT_OrbatrixUncurl2");
	}
	else if (obj->clipRects.unitY2 == ck_keenObj->clipRects.unitY2)
	{
		int dx = ck_keenObj->posX - obj->posX;
		obj->xDirection = dx < 0 ? IN_motion_Left : IN_motion_Right;
		if (dx > -CK_INT(CK6_OrbatrixCurlKeenXRadius, 0x500) && dx < CK_INT(CK6_OrbatrixCurlKeenXRadius, 0x500))
			obj->currentAction = CK_GetActionByName("CK6_ACT_OrbatrixCurl0");
	}
}

void CK6_OrbatrixCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		CK_ShotHit(b);
		CK_SetAction2(a, CK_GetActionByName("CK6_ACT_OrbatrixUncurl2"));
	}
}

void CK6_OrbatrixDraw(CK_object *obj)
{
	obj->posY -= obj->user3;
	CK_BasicDrawFunc2(obj);
	obj->posY += obj->user3;

	obj->user3 += obj->user4 * SD_GetSpriteSync() * 4;

	if (obj->user3 > CK_INT(CK6_OrbatrixMaxFloatYVel, 0x80))
	{
		obj->user3 = CK_INT(CK6_OrbatrixMaxFloatYVel, 0x80);
		obj->user4 = -1;
	}
	else if (obj->user3 < -CK_INT(CK6_OrbatrixMaxFloatYVel, 0x80))
	{
		obj->user3 = -CK_INT(CK6_OrbatrixMaxFloatYVel, 0x80);
		obj->user4 = 1;
	}
}

void CK6_OrbatrixBounceDraw(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

	if (obj->topTI)
		obj->velY = -obj->velY;

	if (obj->topTI || obj->leftTI || obj->rightTI)
	{
		obj->velX = -obj->velX;
		SD_PlaySound(CK_SOUNDNUM(SOUND_ORBATRIXBOUNCE));

		if (obj->topTI && --obj->user1 == 0)
		{
			CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_OrbatrixUncurl0"));
			obj->user2 = CK_INT(CK6_OrbatrixRiseHeight, 0x180);
		}
	}
}

void CK6_OrbatrixCurl(CK_object *obj)
{
	if (obj->user3 >= CK_INT(CK6_OrbatrixBounceMinYVel, 0x10))
	{
		obj->velX = obj->xDirection * CK_INT(CK6_OrbatrixBounceXVel, 60);
		obj->velY = CK_INT(CK6_OrbatrixBounceYVel, -32);
		obj->posY -= obj->user3;
		obj->user1 = CK_INT(CK6_OrbatrixBounceCount, 5);
		obj->currentAction = obj->currentAction->next;
	}

	obj->visible = true;
}

void CK6_OrbatrixUncurlThink(CK_object *obj)
{
	ck_nextY = SD_GetSpriteSync() * CK_INT(CK6_OrbatrixRiseYVel, -8);
	obj->user2 += ck_nextY;
	if (obj->user2 <= 0)
	{
		ck_nextY -= obj->user2;
		obj->currentAction = obj->currentAction->next;
	}
}

void CK6_OrbatrixCol2(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		CK_ShotHit(b);
		a->velX = 0;
	}
}

// Bips

void CK6_BipWalk(CK_object *obj)
{
	if (obj->clipRects.unitY2 == ck_keenObj->clipRects.unitY2)
	{
		if (ck_keenObj->clipRects.unitX1 - CK_INT(CK6_BipTurnRadius, 0x40) < obj->clipRects.unitX2)
			obj->xDirection = IN_motion_Right;

		if (ck_keenObj->clipRects.unitX2 + CK_INT(CK6_BipTurnRadius, 0x40) > obj->clipRects.unitX1)
			obj->xDirection = IN_motion_Left;
	}
	else
	{
		if (US_RndT() < CK_INT(CK6_BipStandChance, 0x10))
		{
			obj->xDirection = -obj->xDirection;
			obj->currentAction = CK_GetActionByName("CK6_ACT_BipStand0");
		}
	}
}

void CK6_BipCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player && b->deltaPosY == 0)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_BIPSQUISH));
		a->type = CT_Friendly;
		CK_SetAction2(a, CK_GetActionByName("CK6_ACT_BipSquished0"));
	}
}

void CK6_SpawnBipship(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Bipship;
	obj->active = OBJ_ACTIVE;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_BipshipSpawnYOffset, -0x180);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->velX = obj->xDirection * CK_INT(CK6_BipshipInitXVel, 20);
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BipshipFly0"));
}

void CK6_BipShotDraw(CK_object *obj)
{
	if (obj->topTI || obj->bottomTI || obj->rightTI || obj->leftTI)
		CK_RemoveObj(obj);
	else
		RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_BipshipTurn(CK_object *obj)
{
	CK_PhysAccelHorz(obj, obj->xDirection, CK_INT(CK6_BipshipXAccel, 20));
}

void CK6_BipshipFly(CK_object *obj)
{
	CK_PhysAccelHorz(obj, obj->xDirection, CK_INT(CK6_BipshipXAccel, 20));
	int xdir = obj->xDirection;
	int ycheck = ck_keenObj->clipRects.unitY2 + CK_INT(CK6_BipshipSearchYOffset, 0x100) - obj->clipRects.unitY2;
	if (ycheck <= CK_INT(CK6_BipshipSearchYDiameter, 0x200) && ycheck >= 0)
	{
		// Fire!!
		xdir = (ck_keenObj->posX < obj->posX) ? IN_motion_Left : IN_motion_Right;
		if (obj->xDirection == xdir && US_RndT() < SD_GetSpriteSync() * 2)
		{
			SD_PlaySound(CK_SOUNDNUM(SOUND_KEENSHOOT));
			CK_object *shot = CK_GetNewObj(true);
			shot->type = CT6_EnemyShot;
			shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
			shot->zLayer = PRIORITIES - 3;
			if (obj->xDirection == IN_motion_Right)
			{
				shot->posX = obj->posX + CK_INT(CK6_BipshipShotXOffsetRight, 0x100);
				shot->velX = CK_INT(CK6_BipshipShotXVel, 64);
			}
			else
			{
				shot->posX = obj->posX + CK_INT(CK6_BipshipShotXOffsetLeft, 0);
				shot->velX = -CK_INT(CK6_BipshipShotXVel, 64);
			}

			shot->posY = obj->posY + CK_INT(CK6_BipshipShotYOffset, 0xA0);
			shot->velY = CK_INT(CK6_BipshipShotYVel, 16);
			CK_SetAction(shot, CK_GetActionByName("CK6_ACT_BipShot0"));
		}
	}

	int startx = obj->clipRects.tileXmid + 2 * xdir;
	int y = obj->clipRects.tileY1;
	uint16_t *tile = CA_TilePtrAtPos(startx, y, 1);

	for (; y <= obj->clipRects.tileY2; y++, tile += CA_GetMapWidth())
	{
		if (TI_ForeLeft(*tile) || TI_ForeRight(*tile))
		{
			xdir = -xdir;
			goto checkTurn;
		}
	}

	// And turn around at ledge-ends
	if (!TI_ForeTop(*tile))
		xdir = -xdir;

checkTurn:

	if (obj->xDirection != xdir)
	{
		obj->xDirection = xdir;
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_BipshipTurn0"));
	}
}

void CK6_BipshipCrash(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_BIPSHIPCRASH));

	// Spawn smoke
	CK_object *smoke = CK_GetNewObj(true);
	smoke->type = CT_Friendly;
	smoke->active = OBJ_ACTIVE;
	smoke->zLayer = PRIORITIES - 2;
	smoke->posX = obj->posX;
	smoke->posY = obj->posY + CK_INT(CK6_BipshipCrashSmokeYOffset, -0x180);
	CK_SetAction(smoke, CK_GetActionByName("CK6_ACT_BipshipSmoke0"));
	smoke->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;

	CK_object *bip = CK_GetNewObj(true);
	bip->type = CT6_Bip;
	bip->active = OBJ_ACTIVE;
	bip->zLayer = PRIORITIES - 4;
	bip->posX = obj->posX;
	bip->posY = obj->posY + CK_INT(CK6_BipCrashYOffset, -0x80);
	bip->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	CK_SetAction(bip, CK_GetActionByName("CK6_ACT_BipStand0"));
}

void CK6_BipshipCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		CK_ShotHit(b);
		CK_SetAction2(a, CK_GetActionByName("CK6_ACT_BipshipHit0"));
	}
}

// Flects

void CK6_SpawnFlect(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Flect;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_FlectSpawnYOffset, -0x100);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_FlectWalk0"));
}

void CK6_FlectTurn(CK_object *obj)
{
	if (ck_keenObj->posX < obj->posX)
	{
		if (obj->xDirection == IN_motion_Left)
		{
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectWalk0");
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");
			obj->xDirection = IN_motion_Left;
		}
	}
	else
	{
		if (obj->xDirection == IN_motion_Right)
		{
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectWalk0");
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");
			obj->xDirection = IN_motion_Right;
		}
	}
}

void CK6_FlectWalk(CK_object *obj)
{
	if (ck_keenObj->posX < obj->posX && obj->xDirection == IN_motion_Right)
	{
		if (obj->xDirection == IN_motion_Left)
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");

		obj->xDirection = IN_motion_Left;
	}

	if (ck_keenObj->posX > obj->posX && obj->xDirection == IN_motion_Left)
	{
		if (obj->xDirection == IN_motion_Right)
			obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn1");

		obj->xDirection = IN_motion_Right;
	}

	if (US_RndT() < CK_INT(CK6_FlectTurnChance, 0x20))
		obj->currentAction = CK_GetActionByName("CK6_ACT_FlectTurn0");
}

void CK6_FlectCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_PhysPushX(b, a);
	}
	else if (b->type == CT_Stunner)
	{
		if (b->xDirection == IN_motion_None)
		{
			CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_FlectStunned0"));
		}
		else if (a->xDirection != b->xDirection)
		{
			b->xDirection = a->xDirection;
			b->user4 = 1; // Shot is hazardous to keen now
			SD_PlaySound(CK_SOUNDNUM(SOUND_FLECTSHOT));
		}
	}
}

void CK6_FlectDraw(CK_object *obj)
{
	// Hit wall walking right; turn around and go left
	if (obj->xDirection == IN_motion_Right && obj->leftTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	// Hit wall walking left; turn around and go right
	else if (obj->xDirection == IN_motion_Left && obj->rightTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	// Walked off of ledge; turn around
	else if (obj->topTI == 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		CK_SetAction2(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Setup all of the functions in this file.
 */
void CK6_Obj2_SetupFunctions()
{

	CK_ACT_AddFunction("CK6_NospikeWalk", &CK6_NospikeWalk);
	CK_ACT_AddFunction("CK6_NospikeCharge", &CK6_NospikeCharge);
	CK_ACT_AddColFunction("CK6_NospikeCol", &CK6_NospikeCol);
	CK_ACT_AddFunction("CK6_NospikeFall", &CK6_NospikeFall);
	CK_ACT_AddFunction("CK6_NospikeFallDraw", &CK6_NospikeFallDraw);
	CK_ACT_AddFunction("CK6_NospikeFallDraw2", &CK6_NospikeFallDraw2);
	CK_ACT_AddFunction("CK6_NospikeChargeDraw", &CK6_NospikeChargeDraw);

	CK_ACT_AddFunction("CK6_GikWalk", &CK6_GikWalk);
	CK_ACT_AddFunction("CK6_GikSlide", &CK6_GikSlide);
	CK_ACT_AddFunction("CK6_GikJumpDraw", &CK6_GikJumpDraw);
	CK_ACT_AddFunction("CK6_GikSlideDraw", &CK6_GikSlideDraw);

	CK_ACT_AddFunction("CK6_OrbatrixFloat", &CK6_OrbatrixFloat);
	CK_ACT_AddColFunction("CK6_OrbatrixCol", &CK6_OrbatrixCol);
	CK_ACT_AddFunction("CK6_OrbatrixDraw", &CK6_OrbatrixDraw);
	CK_ACT_AddFunction("CK6_OrbatrixBounceDraw", &CK6_OrbatrixBounceDraw);
	CK_ACT_AddFunction("CK6_OrbatrixCurl", &CK6_OrbatrixCurl);
	CK_ACT_AddFunction("CK6_OrbatrixUncurlThink", &CK6_OrbatrixUncurlThink);
	CK_ACT_AddColFunction("CK6_OrbatrixCol2", &CK6_OrbatrixCol2);

	CK_ACT_AddFunction("CK6_BipWalk", &CK6_BipWalk);
	CK_ACT_AddColFunction("CK6_BipCol", &CK6_BipCol);
	CK_ACT_AddFunction("CK6_BipShotDraw", &CK6_BipShotDraw);
	CK_ACT_AddFunction("CK6_BipshipTurn", &CK6_BipshipTurn);
	CK_ACT_AddFunction("CK6_BipshipFly", &CK6_BipshipFly);
	CK_ACT_AddFunction("CK6_BipshipCrash", &CK6_BipshipCrash);
	CK_ACT_AddColFunction("CK6_BipshipCol", &CK6_BipshipCol);

	CK_ACT_AddFunction("CK6_FlectTurn", &CK6_FlectTurn);
	CK_ACT_AddFunction("CK6_FlectWalk", &CK6_FlectWalk);
	CK_ACT_AddColFunction("CK6_FlectCol", &CK6_FlectCol);
	CK_ACT_AddFunction("CK6_FlectDraw", &CK6_FlectDraw);
}
