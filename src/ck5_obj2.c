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

#include "id_heads.h"
#include "id_rf.h"
#include "id_us.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

#include <stdio.h>

void CK5_SpawnSparky(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Sparky;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) + CK_INT(CK5_SparkySpawnYOffset, -0x100);
	new_object->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Sparky0"));
}

void CK5_SparkyWait(CK_object *obj)
{

	if (US_RndT() < CK_INT(CK5_SparkySearchChance, 0x40))
	{
		obj->currentAction = CK_GetActionByName("CK5_ACT_SparkySearch0");
		ck_nextX = 0;
	}
}

void CK5_SparkyPrepareCharge(CK_object *obj)
{
	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_SparkyCharge0");
}

void CK5_SparkySearchLeft(CK_object *obj)
{

	uint16_t delY = ck_keenObj->clipRects.unitY2 + 0x100 - obj->clipRects.unitY2;

	if (delY < CK_INT(CK5_SparkySearchYRadius, 0x200) && ck_keenObj->posX < obj->posX)
	{
		obj->xDirection = IN_motion_Left;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPARKYPREPCHARGE));
		obj->currentAction = CK_GetActionByName("CK5_ACT_SparkyPrepCharge0");
		obj->user1 = CK_INT(CK5_SparkyPrepChargeDelay, 3); //should this counter be multiplied by spritesync?
	}
}

void CK5_SparkySearchRight(CK_object *obj)
{

	uint16_t delY = ck_keenObj->clipRects.unitY2 + 0x100 - obj->clipRects.unitY2;

	if (delY < CK_INT(CK5_SparkySearchYRadius, 0x200) && ck_keenObj->posX > obj->posX)
	{
		obj->xDirection = IN_motion_Right;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SPARKYPREPCHARGE));
		obj->currentAction = CK_GetActionByName("CK5_ACT_SparkyPrepCharge0");
		obj->user1 = CK_INT(CK5_SparkyPrepChargeDelay, 3); //should this counter be multiplied by spritesync?
	}
}

void CK5_SparkyCharge0(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK0));
}

void CK5_SparkyCharge1(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK0));
}

void CK5_SparkyCol(CK_object *obj1, CK_object *obj2)
{

	if (obj2->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (obj2->type == CT_Stunner)
	{
		CK_StunCreature(obj1, obj2, CK_GetActionByName("CK5_ACT_SparkyStunned0"));
	}
}

void CK5_SparkyTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_SparkyTurn0"));
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_SparkyTurn0"));
	}
	else if (obj->topTI == 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_SparkyTurn0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SpawnAmpton(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Ampton;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY) + CK_INT(CK5_AmptonSpawnYOffset, -0x80);
	new_object->xDirection = (US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left);
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Ampton0"));
}

void CK5_AmptonWalk(CK_object *obj)
{

	//play tic toc sound
	if (obj->currentAction == CK_GetActionByName("CK5_ACT_Ampton0"))
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_AMPTONWALK0));
	}
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_Ampton2"))
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_AMPTONWALK1));
	}

	// if on tile boundary (i.e. sliding or computer) then skip
	// the check for a pole or a computer
	if (obj->posX & 0xFF)
	{
		int tileX = obj->clipRects.tileX1 + 1;
		int tileY = obj->clipRects.tileY1;
		int miscflags = TI_ForeMisc(CA_TileAtPos(tileX, tileY, 1)) & 0x7F;
		if (miscflags == MISCFLAG_COMPUTER)
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_AmptonComp0");
			return;
		}

		// Check for a pole
		if (miscflags == MISCFLAG_POLE)
		{
			// Don't always climb the pole
			if (US_RndT() < CK_INT(CK5_AmptonClimbPoleChance, 0xC4))
			{
				bool polebelow = ((TI_ForeMisc(CA_TileAtPos(tileX, tileY + 2, 1)) & 0x7F) == MISCFLAG_POLE);
				bool poleabove = ((TI_ForeMisc(CA_TileAtPos(tileX, tileY - 2, 1)) & 0x7F) == MISCFLAG_POLE);

				// Pick direction if there is a choice
				if (poleabove && polebelow)
				{
					if (US_RndT() < 0x80)
						poleabove = false;
					else
						polebelow = false;
				}

				//climb up
				if (poleabove)
				{
					obj->yDirection = IN_motion_Up;
					obj->currentAction = CK_GetActionByName("CK5_ACT_AmptonPole0");
					obj->clipped = CLIP_not;
					obj->timeUntillThink = 6;
					ck_nextX = 0;
					return;
				}

				// slide down
				if (polebelow)
				{
					obj->yDirection = IN_motion_Down;
					obj->currentAction = CK_GetActionByName("CK5_ACT_AmptonPole0");
					obj->clipped = CLIP_not;
					obj->timeUntillThink = 6;
					ck_nextX = 0;
					return;
				}
			}
		}
	}
}

void CK5_AmptonPoleClimb(CK_object *obj)
{

	int newYT;

	// Check if ampton is moving into new tile
	newYT = RF_UnitToTile(obj->clipRects.unitY2 + ck_nextY);

	if (obj->clipRects.tileY2 == newYT)
		return;

	if (obj->yDirection == IN_motion_Up)
	{
		// Going Up
		// Check if coming up through a pole hole
		int tileX = obj->clipRects.tileX1 + 1;
		int tileY = newYT;

		if (!TI_ForeTop(CA_TileAtPos(tileX, tileY, 1)) && TI_ForeTop(CA_TileAtPos(tileX, tileY + 1, 1)))
		{
			// Dismount if there are fewer than 4 pole tiles above the hole
			// or randomly, if there are more than 4
			if (((TI_ForeMisc(CA_TileAtPos(tileX, tileY - CK_INT(CK5_AmptonPoleMinHeight, 4), 1)) & 0x7F) != MISCFLAG_POLE) ||
				US_RndT() >= CK_INT(CK5_AmptonPoleDismountChance, 0x80))
			{

				// Set the ampton on the ground
				int delY = (obj->clipRects.unitY2 & 0xFF) + 1;
				obj->posY -= delY;
				obj->clipRects.unitY2 -= delY;
				obj->clipped = CLIP_normal;
				obj->currentAction = CK_GetActionByName("CK5_ACT_AmptonDismount0");
				ck_nextY = 0x10;
				obj->yDirection = IN_motion_Down;
				CK_PhysUpdateNormalObj(obj);
				obj->timeUntillThink = 4;
				return;
			}
		}
		else if ((TI_ForeMisc(CA_TileAtPos(tileX, tileY - 1, 1)) & 0x7F) != MISCFLAG_POLE)
		{
			// Hit the top of a pole; go back down
			ck_nextY = 0;
			obj->yDirection = IN_motion_Down;
			return;
		}
	}
	else
	{
		// Ampton is sliding down a pole
		int tileX = obj->clipRects.tileX1 + 1;
		int tileY = newYT;

		if (TI_ForeTop(CA_TileAtPos(tileX, tileY, 1)) && !TI_ForeTop(CA_TileAtPos(tileX, tileY - 1, 1)))
		{
			// Dismount if not landing on pole hole
			// If pole hole, randomly dismount
			if ((TI_ForeMisc(CA_TileAtPos(tileX, tileY, 1)) & 0x7F) != MISCFLAG_POLE ||
				US_RndT() >= CK_INT(CK5_AmptonPoleDismountChance, 0x80))
			{

				// Set ampton on the ground
				int delY = 0xFF - (obj->clipRects.unitY2 & 0xFF);
				obj->posY += delY;
				obj->clipRects.unitX2 += delY;
				obj->clipped = CLIP_normal;
				obj->currentAction = CK_GetActionByName("CK5_ACT_AmptonDismount0");
				ck_nextY = 0x10;
				CK_PhysUpdateNormalObj(obj);
				obj->timeUntillThink = 4;
				return;
			}
		}
		else if ((TI_ForeMisc(CA_TileAtPos(tileX, tileY, 1)) & 0x7F) != MISCFLAG_POLE)
		{
			ck_nextY = 0;
			obj->yDirection = IN_motion_Up;
			return;
		}
	}
}

void CK5_AmptonSwitch(CK_object *obj)
{
	obj->timeUntillThink = 4;
}

void CK5_AmptonCol(CK_object *obj1, CK_object *obj2)
{

	if (obj2->type == CT_Player)
	{
		if (obj1->currentAction == CK_GetActionByName("CK5_ACT_AmptonPole2"))
		{
			CK_KillKeen();
		}
		else
		{
			CK_PhysPushX(obj2, obj1);
		}
	}
	else if (obj2->type == CT_Stunner)
	{
		obj1->clipped = CLIP_normal;
		obj1->yDirection = IN_motion_Down;
		obj1->velY = 0;
		SD_PlaySound(CK_SOUNDNUM(SOUND_AMPTONSTUN));
		CK_StunCreature(obj1, obj2, CK_GetActionByName("CK5_ACT_AmptonStunned0"));
	}
}

void CK5_AmptonTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_AmptonTurn0"));
	}

	if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_AmptonTurn0"));
	}

	if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_AmptonTurn0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_SpawnSlice(int tileX, int tileY, int dir)
{

	CK_object *new_object = CK_GetNewObj(false);

	new_object->type = CT5_SliceStar;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 2;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY);
	new_object->user4 = CK_INT(CK5_SliceHealth, 20);

	switch (dir)
	{

	case 0:
		new_object->xDirection = IN_motion_None;
		new_object->yDirection = IN_motion_Up;
		break;
	case 1:
		new_object->xDirection = IN_motion_Right;
		new_object->yDirection = IN_motion_None;
		break;
	case 2:
		new_object->xDirection = IN_motion_None;
		new_object->yDirection = IN_motion_Down;
		break;
	case 3:
		new_object->xDirection = IN_motion_Left;
		new_object->yDirection = IN_motion_None;
		break;
	}

	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Slice0"));
	return;
}

void CK5_SpawnSliceDiag(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_SliceStar;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 2;
	new_object->clipped = CLIP_simple;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY);
	new_object->user4 = CK_INT(CK5_SliceDiagHealth, 50); // strength

	switch (US_RndT() / 0x40)
	{

	case 0:
		new_object->xDirection = IN_motion_Left;
		new_object->yDirection = IN_motion_Up;
		break;
	case 1:
		new_object->xDirection = IN_motion_Right;
		new_object->yDirection = IN_motion_Down;
		break;
	case 2:
		new_object->xDirection = IN_motion_Left;
		new_object->yDirection = IN_motion_Down;
		break;
	case 3:
		new_object->xDirection = IN_motion_Left;
		new_object->yDirection = IN_motion_Up;
		break;
	}

	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_SliceDiag0"));
}

void CK5_SliceCol(CK_object *obj1, CK_object *obj2)
{

	if (obj2->type == CT_Player)
	{
		CK_KillKeen();
		return;
	}

	if (obj2->type == CT_Stunner)
	{
		CK_ShotHit(obj2);
		if (--obj1->user4 == 0)
			CK_SetAction(obj1, CK_GetActionByName("CK5_ACT_SliceDie0"));
	}
}

void CK5_SliceDiagTileCol(CK_object *obj)
{

	if (obj->topTI)
	{
		obj->yDirection = IN_motion_Up;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SLICEBUMP));
	}
	else if (obj->bottomTI)
	{
		obj->yDirection = IN_motion_Down;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SLICEBUMP));
	}

	if (obj->leftTI)
	{
		obj->xDirection = IN_motion_Left;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SLICEBUMP));
	}
	else if (obj->rightTI)
	{
		obj->xDirection = IN_motion_Right;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SLICEBUMP));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

void CK5_SpawnShelly(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Sparky; // Apparently, the same type as a sparky
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = RF_TileToUnit(tileX);
	new_object->posY = RF_TileToUnit(tileY);
	new_object->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Shelly0"));
}

void CK5_ShellyWait(CK_object *obj)
{

	if (ck_keenObj->clipRects.unitY1 >= obj->clipRects.unitY2)
	{
		// Jump off of ledge if Keen is a ledge below
		int xDel = ck_keenObj->clipRects.unitXmid - obj->clipRects.unitXmid;
		if (obj->xDirection == IN_motion_Right)
		{
			if (xDel > CK_INT(CK5_ShelleyDiveMinX, 0x100) && xDel < CK_INT(CK5_ShelleyDiveMaxX, 0x300))
			{
				obj->velX = CK_INT(CK5_ShelleyDiveXVel, 0x10);
				obj->velY = CK_INT(CK5_ShelleyDiveYVel, -0x18);
				obj->currentAction = CK_GetActionByName("CK5_ACT_ShellyDive0");
				ck_nextY = ck_nextX = 0;
			}
		}
		else
		{
			if (xDel < -CK_INT(CK5_ShelleyDiveMinX, 0x100) && xDel > -CK_INT(CK5_ShelleyDiveMaxX, 0x300))
			{
				obj->velX = -CK_INT(CK5_ShelleyDiveXVel, 0x10);
				obj->velY = CK_INT(CK5_ShelleyDiveYVel, -0x18);
				obj->currentAction = CK_GetActionByName("CK5_ACT_ShellyDive0");
				ck_nextY = ck_nextX = 0;
			}
		}
	}
}

void CK5_ShellyStartJump(CK_object *obj)
{

	obj->xDirection = -obj->xDirection;
}

void CK5_SpawnShellyBits(CK_object *obj)
{

	CK_object *new_object;
	if ((new_object = CK_GetNewObj(true)))
	{
		new_object->posX = obj->posX;
		new_object->posY = obj->posY;
		new_object->velX = CK_INT(CK5_ShelleyBits0XVel, 0x20);
		new_object->velY = CK_INT(CK5_ShelleyBits0YVel, -0x18);
		CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShellyBits0"));
	}

	if ((new_object = CK_GetNewObj(true)))
	{
		new_object->posX = obj->posX;
		new_object->posY = obj->posY;
		new_object->velX = CK_INT(CK5_ShelleyBits1XVel, -0x20);
		new_object->velY = CK_INT(CK5_ShelleyBits1YVel, -0x18);
		CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShellyBits1"));
	}
}

void CK5_ShellyCol(CK_object *obj1, CK_object *obj2)
{

	CK_object *new_object;

	// Player, Stunners, and Enemy shots destroy shelly
	if (obj2->type == CT_Player)
	{
		CK_PhysPushX(obj2, obj1);
		//don't detonate if not inside shelley
		if (ck_keenObj->clipRects.unitXmid < obj1->clipRects.unitX1 || ck_keenObj->clipRects.unitXmid > obj1->clipRects.unitX2)
		{
			return;
		}
	}
	else if (obj2->type == CT_Stunner)
	{
		CK_ShotHit(obj2);
	}
	else if (obj2->type == CT5_EnemyShot)
	{
		CK_RemoveObj(obj2);
	}
	else
	{
		return;
	}

	// destroy old shelly and spawn explosion
	SD_PlaySound(CK_SOUNDNUM(SOUND_SHELLYEXPLODE));
	if (!obj1->topTI)
		CK_SetAction2(obj1, CK_GetActionByName("CK5_ACT_ShellyDieAir"));
	else
		CK_SetAction2(obj1, CK_GetActionByName("CK5_ACT_ShellyDieGround"));

	if ((new_object = CK_GetNewObj(true)))
	{
		new_object->posX = obj1->posX;
		new_object->posY = obj1->posY;
		CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShellyExplode0"));
	}
}

void CK5_ShellyGroundTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
	}
	else if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_ShellyWait0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	return;
}

void CK5_ShellyAirTileCol(CK_object *obj)
{

	if (obj->rightTI || obj->leftTI)
	{
		obj->velX = 0;
	}

	if (obj->topTI)
	{
		CK_object *new_object;
		SD_PlaySound(CK_SOUNDNUM(SOUND_SHELLYEXPLODE));
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_ShellyDieAir"));
		if ((new_object = CK_GetNewObj(true)))
		{
			new_object->posX = obj->posX;
			new_object->posY = obj->posY;
			CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShellyExplode0"));
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_Obj2_SetupFunctions(void)
{

	// Sparky
	CK_ACT_AddFunction("CK5_SparkyWait", &CK5_SparkyWait);
	CK_ACT_AddFunction("CK5_SparkyPrepareCharge", &CK5_SparkyPrepareCharge);
	CK_ACT_AddFunction("CK5_SparkySearchLeft", &CK5_SparkySearchLeft);
	CK_ACT_AddFunction("CK5_SparkySearchRight", &CK5_SparkySearchRight);
	CK_ACT_AddFunction("CK5_SparkyCharge0", &CK5_SparkyCharge0);
	CK_ACT_AddFunction("CK5_SparkyCharge1", &CK5_SparkyCharge1);
	CK_ACT_AddColFunction("CK5_SparkyCol", &CK5_SparkyCol);
	CK_ACT_AddFunction("CK5_SparkyTileCol", &CK5_SparkyTileCol);

	CK_ACT_AddFunction("CK5_AmptonWalk", &CK5_AmptonWalk);
	CK_ACT_AddFunction("CK5_AmptonPoleClimb", &CK5_AmptonPoleClimb);
	CK_ACT_AddFunction("CK5_AmptonSwitch", &CK5_AmptonSwitch);
	CK_ACT_AddColFunction("CK5_AmptonCol", &CK5_AmptonCol);
	CK_ACT_AddFunction("CK5_AmptonTileCol", &CK5_AmptonTileCol);

	CK_ACT_AddColFunction("CK5_SliceCol", &CK5_SliceCol);
	CK_ACT_AddFunction("CK5_SliceDiagTileCol", &CK5_SliceDiagTileCol);

	CK_ACT_AddFunction("CK5_ShellyWait", &CK5_ShellyWait);
	CK_ACT_AddFunction("CK5_ShellyStartJump", &CK5_ShellyStartJump);
	CK_ACT_AddFunction("CK5_SpawnShellyBits", &CK5_SpawnShellyBits);
	CK_ACT_AddColFunction("CK5_ShellyCol", &CK5_ShellyCol);
	CK_ACT_AddFunction("CK5_ShellyGroundTileCol", &CK5_ShellyGroundTileCol);
	CK_ACT_AddFunction("CK5_ShellyAirTileCol", &CK5_ShellyAirTileCol);
}
