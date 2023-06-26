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

/*
 * Setup all of the functions in this file.
 */

// Fleex

void CK6_SpawnFleex(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Fleex;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_FleexSpawnYOffset, -0x280);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_FleexWalk0"));
	obj->user2 = CK_INT(CK6_FleexHealth, 4);
}

void CK6_FleexWalk(CK_object *obj)
{
	if (ck_keenObj->deltaPosX && !obj->user1)
	{
		obj->currentAction = CK_GetActionByName("CK6_ACT_FleexSearch0");
	}
	else
	{
		obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
	}
}

void CK6_FleexSearch(CK_object *obj)
{
	obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
}

void CK6_FleexCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
		CK_KillKeen();

	if (b->type == CT_Stunner)
	{
		if (--a->user2)
		{
			a->user1 = 2;
			a->visible = true;
			CK_ShotHit(b);
			if (a->currentAction == CK_GetActionByName("CK6_ACT_FleexSearch0") ||
				a->currentAction == CK_GetActionByName("CK6_ACT_FleexSearch1"))
			{
				a->xDirection = a->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_FleexWalk0"));
			}
		}
		else
		{
			CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_FleexStunned0"));
			a->velY = CK_INT(CK6_FleexDeathHopYVel, -20);
		}
	}
}

// Bobbas

void CK6_SpawnBobba(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Bobba;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_BobbaSpawnYOffset, -0x200);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BobbaJump0"));
}

void CK6_BobbaFireball(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_BOBBAFIREBALL));
}

void CK6_Bobba(CK_object *obj)
{
	if (++obj->user1 == CK_INT(CK6_BobbaHopsPerShot, 3))
	{
		// Shoot every third hop
		obj->user1 = 0;
		CK_object *shot = CK_GetNewObj(true);
		shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
		shot->type = CT6_EnemyShot;
		shot->posY = obj->posY + CK_INT(CK6_BobbaShotYOffset, 0xB0);
		shot->xDirection = obj->xDirection;
		shot->posX = obj->xDirection == IN_motion_Right ? obj->posX + CK_INT(CK6_BobbaShotXOffsetRight, 0x100) : obj->posX + CK_INT(CK6_BobbaShotXOffsetLeft, 0xB0);
		CK_SetAction(shot, CK_GetActionByName("CK6_ACT_BobbaTwinkle0"));
		shot->zLayer = PRIORITIES - 2;

		obj->currentAction = CK_GetActionByName("CK6_ACT_BobbaShoot0");
	}
	else
	{
		// Turn around if there's no where to jump
		uint16_t *tile = CA_TilePtrAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2 + 1, 1);
		for (int i = 0; i < 4; i++)
		{
			int w = CA_GetMapWidth();
			if (!TI_ForeTop(*tile) && !TI_ForeTop(*(tile - w)) && !TI_ForeTop(*(tile + w)))
			{
				obj->xDirection = -obj->xDirection;
				break;
			}

			tile += obj->xDirection;
		}

		obj->velX = obj->xDirection * CK_INT(CK6_BobbaHopXVel, 32);
		obj->velY = CK_INT(CK6_BobbaHopYVel, -32);
		SD_PlaySound(CK_SOUNDNUM(SOUND_BOBBAJUMP));
	}
}

void CK6_BobbaCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		CK_ShotHit(b);
		a->xDirection = -a->xDirection;
	}
}

void CK6_BobbaJumpDraw(CK_object *obj)
{
	if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		obj->velX = -obj->velX;
	}
	else if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		obj->velX = -obj->velX;
	}

	if (obj->bottomTI)
		obj->velY = 0;

	if (obj->topTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_BOBBALAND));
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_BobbaStand0"));
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_BobbaFireballDraw(CK_object *obj)
{

	if (obj->topTI || obj->bottomTI || obj->rightTI || obj->leftTI)
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_BobbaFireballHits0"));

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Babobbas

void CK6_SpawnBabobba(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Babobba;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_BabobbaSpawnYOffset, -0x200);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_BabobbaJump0"));
}

void CK6_BabobbaSit(CK_object *obj)
{
	if (US_RndT() < CK_INT(CK6_BabobbaNapChance, 4))
	{
		// Nap time
		obj->user1 = 0;
		obj->currentAction = CK_GetActionByName("CK6_ACT_BabobbaNap0");
	}
	else if (++obj->user1 == CK_INT(CK6_BabobbaHopsPerShot, 3))
	{
		// Shoot every third hop
		obj->user1 = 0;
		CK_object *shot = CK_GetNewObj(true);
		shot->active = OBJ_EXISTS_ONLY_ONSCREEN;
		shot->type = CT6_EnemyShot;
		shot->posY = obj->posY + CK_INT(CK6_BabobbaShotYOffset, 0x40);
		shot->xDirection = obj->xDirection;
		shot->posX = obj->xDirection == IN_motion_Right ? obj->posX + CK_INT(CK6_BabobbaShotXOffsetRight, 0x100) : obj->posX + CK_INT(CK6_BabobbaShotXOffsetLeft, 0xB0);
		shot->velX = shot->xDirection * CK_INT(CK6_BabobbaShotXVel, 32);
		CK_SetAction(shot, CK_GetActionByName("CK6_ACT_BabobbaShotStart0"));
		shot->zLayer = PRIORITIES - 2;

		obj->currentAction = CK_GetActionByName("CK6_ACT_BabobbaSit1");
	}
	else
	{
		// Turn around if there's no where to jump
		uint16_t *tile = CA_TilePtrAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2 + 1, 1);
		for (int i = 0; i < 4; i++)
		{
			int w = CA_GetMapWidth();
			if (!TI_ForeTop(*tile) && !TI_ForeTop(*(tile - w)) && !TI_ForeTop(*(tile + w)))
			{
				obj->xDirection = -obj->xDirection;
				break;
			}

			tile += obj->xDirection;
		}

		obj->velX = obj->xDirection * CK_INT(CK6_BabobbaHopXVel, 24);
		obj->velY = CK_INT(CK6_BabobbaHopYVel, -32);
	}
}

void CK6_BabobbaCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_BabobbaStunned0"));
	}
}

void CK6_BabobbaNapCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
		CK_StunCreature(a, b, CK_GetActionByName("CK6_ACT_BabobbaStunned0"));
}

void CK6_BabobbaJumpDraw(CK_object *obj)
{
	if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		obj->velX = -obj->velX;
	}
	else if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		obj->velX = -obj->velX;
	}

	if (obj->bottomTI)
		obj->velY = 0;

	if (obj->topTI)
		CK_SetAction2(obj, CK_GetActionByName("CK6_ACT_BabobbaSit0"));

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_BabobbaShot(CK_object *obj)
{
	if (++obj->user1 == CK_INT(CK6_BabobbaShotDecayTime, 10))
	{
		obj->user1 = 0;
		obj->currentAction = CK_GetActionByName("CK6_ACT_BabobbaShotVanish0");
	}
}

void CK6_BabobbaShotVanish(CK_object *obj)
{
	if (++obj->user1 == CK_INT(CK6_BabobbaShotVanishTime, 5))
	{
		CK_RemoveObj(obj);
	}
}

// Blorbs

void CK6_SpawnBlorb(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Blorb;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) + CK_INT(CK6_BlorbSpawnYOffset, -0x80);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = US_RndT() < 0x80 ? IN_motion_Down : IN_motion_Up;
	obj->clipped = CLIP_simple;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_Blorb0"));
}

void CK6_BlorbDraw(CK_object *obj)
{
	if (obj->topTI)
	{
		obj->yDirection = IN_motion_Up;
		SD_PlaySound(CK_SOUNDNUM(SOUND_BLORBBOUNCE));
	}
	else if (obj->bottomTI)
	{
		obj->yDirection = IN_motion_Down;
		SD_PlaySound(CK_SOUNDNUM(SOUND_BLORBBOUNCE));
	}

	if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		SD_PlaySound(CK_SOUNDNUM(SOUND_BLORBBOUNCE));
	}
	else if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		SD_PlaySound(CK_SOUNDNUM(SOUND_BLORBBOUNCE));
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK6_SpawnCeilick(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT6_Ceilick;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->clipped = CLIP_not;
	obj->posX = RF_TileToUnit(tileX);
	obj->user1 = obj->posY = RF_TileToUnit(tileY);
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_CeilickWait0"));
}

void CK6_Ceilick(CK_object *obj)
{
	if (ck_keenObj->posY - obj->posY < CK_INT(CK6_CeilickSearchHeight, 0x280) &&  // <- relies on unsignedness of posY
		ck_keenObj->posY - obj->posY >= 0 && // this line not in dos version
		obj->clipRects.unitX2 + CK_INT(CK6_CeilickSearchXRadius, 0x10) > ck_keenObj->clipRects.unitX1 &&
		obj->clipRects.unitX1 - CK_INT(CK6_CeilickSearchXRadius, 0x10) < ck_keenObj->clipRects.unitX2)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_CEILICKATTACK));
		obj->currentAction = CK_GetActionByName("CK6_ACT_CeilickStrike00");
	}
}

void CK6_CeilickDescend(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_CEILICKLAUGH));
}

void CK6_CeilickStunned(CK_object *obj)
{
	obj->visible = true;
}

void CK6_CeilickCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		a->posY = a->user1;
		CK_ShotHit(b);
		a->user1 = a->user2 = a->user3 = 0;
		a->user4 = a->type;
		CK_SetAction2(a, CK_GetActionByName("CK6_ACT_CeilickStunned0"));
		a->type = CT6_StunnedCreature;
	}
}

void CK6_Obj3_SetupFunctions()
{

	CK_ACT_AddFunction("CK6_FleexWalk", &CK6_FleexWalk);
	CK_ACT_AddFunction("CK6_FleexSearch", &CK6_FleexSearch);
	CK_ACT_AddColFunction("CK6_FleexCol", &CK6_FleexCol);

	CK_ACT_AddFunction("CK6_BobbaFireball", &CK6_BobbaFireball);
	CK_ACT_AddFunction("CK6_Bobba", &CK6_Bobba);
	CK_ACT_AddColFunction("CK6_BobbaCol", &CK6_BobbaCol);
	CK_ACT_AddFunction("CK6_BobbaJumpDraw", &CK6_BobbaJumpDraw);
	CK_ACT_AddFunction("CK6_BobbaFireballDraw", &CK6_BobbaFireballDraw);

	CK_ACT_AddFunction("CK6_BabobbaSit", &CK6_BabobbaSit);
	CK_ACT_AddColFunction("CK6_BabobbaCol", &CK6_BabobbaCol);
	CK_ACT_AddColFunction("CK6_BabobbaNapCol", &CK6_BabobbaNapCol);
	CK_ACT_AddFunction("CK6_BabobbaJumpDraw", &CK6_BabobbaJumpDraw);
	CK_ACT_AddFunction("CK6_BabobbaShot", &CK6_BabobbaShot);
	CK_ACT_AddFunction("CK6_BabobbaShotVanish", &CK6_BabobbaShotVanish);

	CK_ACT_AddFunction("CK6_Ceilick", &CK6_Ceilick);
	CK_ACT_AddFunction("CK6_CeilickDescend", &CK6_CeilickDescend);
	CK_ACT_AddFunction("CK6_CeilickStunned", &CK6_CeilickStunned);
	CK_ACT_AddColFunction("CK6_CeilickCol", &CK6_CeilickCol);

	CK_ACT_AddFunction("CK6_BlorbDraw", &CK6_BlorbDraw);
}
