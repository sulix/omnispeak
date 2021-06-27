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
#include "ck4_ep.h"

#include <stdio.h>

// =========================================================================

// Miragia

void CK4_SpawnMiragia(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT_Friendly;
	obj->active = OBJ_ALWAYS_ACTIVE;
	obj->posX = tileX;
	obj->posY = tileY;
	obj->currentAction = CK_GetActionByName("CK4_ACT_Miragia0");
}

void CK4_Miragia0(CK_object *obj)
{
	if ((ck_keenObj->clipRects.tileX2 >= obj->posX) &&
		(ck_keenObj->clipRects.tileX1 <= obj->posX + 5) &&
		(ck_keenObj->clipRects.tileY1 >= obj->posY) &&
		(ck_keenObj->clipRects.tileY2 <= obj->posY + 4))
	{
		obj->currentAction = CK_GetActionByName("CK4_ACT_Miragia7");
	}
	else
	{
		RF_ReplaceTileBlock(8, 60, obj->posX, obj->posY, 6, 4);
	}
}

void CK4_Miragia1(CK_object *obj) { RF_ReplaceTileBlock(14, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia2(CK_object *obj) { RF_ReplaceTileBlock(20, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia3(CK_object *obj) { RF_ReplaceTileBlock(26, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia4(CK_object *obj) { RF_ReplaceTileBlock(20, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia5(CK_object *obj) { RF_ReplaceTileBlock(14, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia6(CK_object *obj) { RF_ReplaceTileBlock(8, 60, obj->posX, obj->posY, 6, 4); }
void CK4_Miragia7(CK_object *obj) { RF_ReplaceTileBlock(2, 60, obj->posX, obj->posY, 6, 4); }

void CK4_SpawnCouncilMember(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_CouncilMember;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 369;
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_CouncilWalk0"));
}

void CK4_CouncilWalk(CK_object *obj)
{
	if (SD_GetSpriteSync() << 3 > US_RndT())
		obj->currentAction = CK_GetActionByName("CK4_ACT_CouncilPause");
}

// Slugs

void CK4_SpawnSlug(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_Slug;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 2;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0x71;
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_SlugMove0"));
}

void CK4_SlugMove(CK_object *obj)
{
	if (US_RndT() < 0x10)
	{
		obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
		obj->currentAction = CK_GetActionByName("CK4_ACT_SlugSliming0");
		SD_PlaySound(SOUND_SLUGSLIME);
	}
}

void CK4_SlugSlime(CK_object *obj)
{
	CK_object *slime = CK_GetNewObj(true);
	slime->type = CT_Friendly;
	slime->active = OBJ_EXISTS_ONLY_ONSCREEN;
	slime->zLayer = PRIORITIES - 4;
	slime->posX = obj->posX;
	slime->posY = obj->clipRects.unitY2 - 0x80;
	CK_SetAction(slime, CK_GetActionByName("CK4_ACT_SlugSlime0"));
}

void CK4_SlugCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		CK_StunCreature(a, b, CK_GetActionByName(US_RndT() < 0x80 ? "CK4_ACT_SlugStunned0" : "CK4_ACT_SlugStunned1"));
		a->velY = -24;
		a->velX = a->xDirection * 8;
	}
}

// Mad Mushrooms

void CK4_SpawnMushroom(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_Mushroom;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit((tileY)) - 0xF1;
	obj->xDirection = IN_motion_Right;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Mushroom0"));
}

void CK4_MushroomMove(CK_object *obj)
{
	obj->xDirection = ck_keenObj->posX > obj->posX ? IN_motion_Right : IN_motion_Left;
	CK_PhysGravityMid(obj);
}

void CK4_MushroomCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		CK_ShotHit(b);
	}
	else if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK4_MushroomDraw(CK_object *obj)
{
	if (obj->bottomTI)
	{
		obj->velY = 0;
	}

	if (obj->topTI)
	{
		obj->velY = 0;
		if (++obj->user1 == 3) // Jump counter
		{
			obj->user1 = 0;
			obj->velY = -68;
			SD_PlaySound(SOUND_MUSHROOMLEAP);
		}
		else
		{
			SD_PlaySound(SOUND_MUSHROOMHOP);
			obj->velY = -40;
		}
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Bluebird

void CK4_SpawnEgg(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_Egg;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 2;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0x71;
	obj->xDirection = IN_motion_Right;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_Egg0"));
}

void CK4_BirdRecover(CK_object *obj)
{
	obj->type = CT4_Bird;
}

void CK4_SpawnBird(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(true);
	obj->type = CT4_Bird;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 2;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0xF1;
	obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_BirdHatched0"));
}

void CK4_EggCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player || b->type == CT_Stunner)
	{
		if (b->type == CT_Stunner)
			CK_ShotHit(b);

		a->type = CT_Friendly;
		a->active = OBJ_EXISTS_ONLY_ONSCREEN;
		CK_SetAction(a, CK_GetActionByName("CK4_ACT_Eggshell0"));

		// Spawn bird
		CK_object *bird = CK_GetNewObj(true);
		bird->type = CT4_Bird;
		bird->active = OBJ_ACTIVE;
		bird->posX = a->posX;
		bird->posY = a->posY - 0x80;
		bird->xDirection = a->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
		bird->yDirection = IN_motion_Down;
		CK_SetAction(bird, CK_GetActionByName("CK4_ACT_BirdHatched0"));

		// Shell bits
		CK_object *shell = CK_GetNewObj(true);
		shell->type = CT_Friendly;
		shell->active = OBJ_EXISTS_ONLY_ONSCREEN;
		shell->posX = a->posX;
		shell->posY = a->posY;
		shell->velX = -28;
		shell->velY = -40;
		CK_SetAction(shell, CK_GetActionByName("CK4_ACT_EggshellBitA0"));

		shell = CK_GetNewObj(true);
		shell->type = CT_Friendly;
		shell->active = OBJ_EXISTS_ONLY_ONSCREEN;
		shell->posX = a->posX;
		shell->posY = a->posY;
		shell->velX = 28;
		shell->velY = -40;
		CK_SetAction(shell, CK_GetActionByName("CK4_ACT_EggshellBitB0"));

		shell = CK_GetNewObj(true);
		shell->type = CT_Friendly;
		shell->active = OBJ_EXISTS_ONLY_ONSCREEN;
		shell->posX = a->posX;
		shell->posY = a->posY;
		shell->velX = 0;
		shell->velY = -56;
		CK_SetAction(shell, CK_GetActionByName("CK4_ACT_EggshellBitB0"));
	}
}

void CK4_BirdWalk(CK_object *obj)
{
	obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
	if (obj->clipRects.unitY2 >= ck_keenObj->clipRects.unitY2 + 0x300 &&
		ck_keenObj->topTI &&
		CK_PreviewClipRects(obj, CK_GetActionByName("CK4_ACT_BirdFly0")))
	{
		obj->currentAction = CK_GetActionByName("CK4_ACT_BirdFly0");
		obj->clipped = CLIP_simple;
		obj->velY = -8;
		obj->user1 = 0;
	}
}

void CK4_BirdFly(CK_object *obj)
{
	if (obj->user1)
		obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;

	CK_PhysAccelHorz2(obj, obj->xDirection, 16);

	if (obj->posY < ck_keenObj->posY)
		CK_PhysAccelVert1(obj, 1, 16);
	else
		CK_PhysAccelVert1(obj, -1, 16);
}

void CK4_BirdCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (b->type == CT_Stunner)
	{
		a->velX = 0;
		a->clipped = CLIP_normal;
		CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_BirdStunned0"));
	}
}

void CK4_BirdHatchedCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		a->velX = 0;
		CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_BirdStunned0"));
	}
}

void CK4_BirdDraw(CK_object *obj)
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

	if (!obj->topTI)
	{
		obj->velY = -16;
		obj->clipped = CLIP_simple;
		CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_BirdFly0"));
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_BirdLandingDraw(CK_object *obj)
{
	if (obj->topTI)
		CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_BirdWalk0"));

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_EggshellDraw(CK_object *obj)
{
	if (obj->topTI)
		obj->velX = obj->velY = 0;

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK4_BirdFlyingDraw(CK_object *obj)
{
	if (obj->bottomTI || obj->topTI)
		if (!obj->user1)
			obj->user1++;

	if ((obj->rightTI && obj->velX < 0) || (obj->leftTI && obj->velX > 0))
	{
		obj->velX = 0;
		obj->xDirection = -obj->xDirection;
	}

	if (obj->topTI == 1 && ck_keenObj->clipRects.unitY2 - obj->clipRects.unitY2 < 0x80)
	{
		// Attempt to land, and remain flying if can't
		CK_action *act = obj->currentAction;
		obj->clipped = CLIP_normal;
		CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_BirdWalk0"));
		ck_nextX = 0;
		ck_nextY = 0x80;
		CK_PhysUpdateSimpleObj(obj);
		if (!obj->topTI)
		{
			obj->clipped = CLIP_simple;
			CK_SetAction2(obj, act);
		}
		// Shouldn't we draw here?
		return;
	}

	if (!obj->bottomTI && !obj->topTI)
		obj->user1 = 0;

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Arachnut

void CK4_SpawnArachnut(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_Arachnut;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY) - 0x171;
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_ArachnutWalk0"));
}

void CK4_ArachnutSearch(CK_object *obj)
{
	obj->xDirection = obj->posX < ck_keenObj->posX ? IN_motion_Right : IN_motion_Left;
}

void CK4_ArachnutCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_ArachnutStunned0"));
	}
	else if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK4_ArachnutStunnedCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Stunner)
	{
		CK_StunCreature(a, b, CK_GetActionByName("CK4_ACT_ArachnutStunned0"));
	}
}

// Skypests

void CK4_SpawnSkypest(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT4_Skypest;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = PRIORITIES - 4;
	obj->posX = RF_TileToUnit(tileX);
	obj->posY = RF_TileToUnit(tileY);
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = US_RndT() < 0x80 ? IN_motion_Down : IN_motion_Up;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_SkypestFly0"));
}

void CK4_SkypestFly(CK_object *obj)
{
	if (US_RndT() < SD_GetSpriteSync())
		obj->xDirection = -obj->xDirection;

	if (obj->yDirection == IN_motion_Up && US_RndT() < SD_GetSpriteSync())
		obj->yDirection = IN_motion_Down;

	if (obj->yDirection == IN_motion_Down && US_RndT() < SD_GetSpriteSync() * 2)
		obj->yDirection = -obj->yDirection;

	CK_PhysAccelHorz(obj, obj->xDirection, 20);
	CK_PhysAccelVert1(obj, obj->yDirection, 20);
}

void CK4_SkypestAirCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		CK_KillKeen();
	}
	// There's no "else" in the original Keen 4 EXE so, so let's omit it for
	// the sake of consistency, even if there's no change to behaviors
	if (b->type == CT_Stunner)
	{
		if (b->xDirection == IN_motion_Right)
			a->velX = 20;
		else if (b->xDirection == IN_motion_Left)
			a->velX = -20;
		else if (b->yDirection == IN_motion_Down)
			a->velY = 20;
		else if (b->yDirection == IN_motion_Up)
			a->velY = -20;

		CK_ShotHit(b);
	}
}

void CK4_SkypestGroundCol(CK_object *a, CK_object *b)
{
	if (b->currentAction == CK_GetActionByName("CK_ACT_keenPogo1") ||
		b->currentAction == CK_GetActionByName("CK_ACT_keenPogo2") ||
		b->currentAction == CK_GetActionByName("CK_ACT_keenPogo3"))
	{
		CK_SetAction2(a, CK_GetActionByName("CK4_ACT_SkypestSquish0"));
		SD_PlaySound(SOUND_SKYPESTSQUISH);
		a->type = CT_Friendly;
	}
}

void CK4_SkypestTakeoff(CK_object *obj)
{
	obj->yDirection = IN_motion_Up;
	obj->velY = -16;
	ck_nextY = -144;
}

void CK4_SkypestDraw(CK_object *obj)
{
	if (obj->bottomTI)
	{
		obj->velY = 8;
		obj->yDirection = IN_motion_Down;
	}

	if (obj->topTI && !obj->rightTI && !obj->leftTI)
	{
		obj->posY += 0x80;
		CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_SkypestPreen0"));
	}

	if (obj->leftTI)
	{
		obj->velX = 0;
		obj->xDirection = IN_motion_Left;
	}

	if (obj->rightTI)
	{
		obj->velX = 0;
		obj->xDirection = IN_motion_Right;
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Setup all of the functions in this file.
 */
void CK4_Obj1_SetupFunctions()
{
	CK_ACT_AddFunction("CK4_Miragia0", &CK4_Miragia0);
	CK_ACT_AddFunction("CK4_Miragia1", &CK4_Miragia1);
	CK_ACT_AddFunction("CK4_Miragia2", &CK4_Miragia2);
	CK_ACT_AddFunction("CK4_Miragia3", &CK4_Miragia3);
	CK_ACT_AddFunction("CK4_Miragia4", &CK4_Miragia4);
	CK_ACT_AddFunction("CK4_Miragia5", &CK4_Miragia5);
	CK_ACT_AddFunction("CK4_Miragia6", &CK4_Miragia6);
	CK_ACT_AddFunction("CK4_Miragia7", &CK4_Miragia7);

	CK_ACT_AddFunction("CK4_CouncilWalk", &CK4_CouncilWalk);

	CK_ACT_AddFunction("CK4_SlugMove", &CK4_SlugMove);
	CK_ACT_AddFunction("CK4_SlugSlime", &CK4_SlugSlime);
	CK_ACT_AddColFunction("CK4_SlugCol", &CK4_SlugCol);

	CK_ACT_AddFunction("CK4_MushroomMove", &CK4_MushroomMove);
	CK_ACT_AddColFunction("CK4_MushroomCol", &CK4_MushroomCol);
	CK_ACT_AddFunction("CK4_MushroomDraw", &CK4_MushroomDraw);

	CK_ACT_AddFunction("CK4_BirdRecover", &CK4_BirdRecover);
	CK_ACT_AddColFunction("CK4_EggCol", &CK4_EggCol);
	CK_ACT_AddFunction("CK4_BirdWalk", &CK4_BirdWalk);
	CK_ACT_AddFunction("CK4_BirdFly", &CK4_BirdFly);
	CK_ACT_AddColFunction("CK4_BirdCol", &CK4_BirdCol);
	CK_ACT_AddColFunction("CK4_BirdHatchedCol", &CK4_BirdHatchedCol);
	CK_ACT_AddFunction("CK4_BirdDraw", &CK4_BirdDraw);
	CK_ACT_AddFunction("CK4_BirdLandingDraw", &CK4_BirdLandingDraw);
	CK_ACT_AddFunction("CK4_EggshellDraw", &CK4_EggshellDraw);
	CK_ACT_AddFunction("CK4_BirdFlyingDraw", &CK4_BirdFlyingDraw);

	CK_ACT_AddFunction("CK4_ArachnutSearch", &CK4_ArachnutSearch);
	CK_ACT_AddColFunction("CK4_ArachnutCol", &CK4_ArachnutCol);
	CK_ACT_AddColFunction("CK4_ArachnutStunnedCol", &CK4_ArachnutStunnedCol);

	CK_ACT_AddFunction("CK4_SkypestFly", &CK4_SkypestFly);
	CK_ACT_AddColFunction("CK4_SkypestAirCol", &CK4_SkypestAirCol);
	CK_ACT_AddColFunction("CK4_SkypestGroundCol", &CK4_SkypestGroundCol);
	CK_ACT_AddFunction("CK4_SkypestTakeoff", &CK4_SkypestTakeoff);
	CK_ACT_AddFunction("CK4_SkypestDraw", &CK4_SkypestDraw);
}
