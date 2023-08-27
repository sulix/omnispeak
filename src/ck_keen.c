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

#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"

// TODO: Handle multiple episodes in some way
#ifdef WITH_KEEN4
#include "ck4_ep.h"
#endif
#ifdef WITH_KEEN5
#include "ck5_ep.h"
#endif

#include "id_ca.h"
#include "id_heads.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_sd.h"
#include "id_ti.h"

#include "ck_cross.h"

// For all the shitty debug stuff  I have.
#include <stdio.h>
#include "id_vl.h"

void CK_SpawnKeen(int tileX, int tileY, int direction);
extern CK_object *ck_keenObj;

CK_keenState ck_keenState;

void CK_BasicDrawFunc1(CK_object *obj);

soundnames *ck_itemSounds;
uint16_t ck_itemPoints[] = {0, 0, 0, 0, 100, 200, 500, 1000, 2000, 5000, 0, 0, 0};
chunk_id_t ck_itemShadows[] = {
	CK_CHUNKID(SPR_BONUSGEM),
	CK_CHUNKID(SPR_BONUSGEM),
	CK_CHUNKID(SPR_BONUSGEM),
	CK_CHUNKID(SPR_BONUSGEM),
	CK_CHUNKID(SPR_BONUS100),
	CK_CHUNKID(SPR_BONUS200),
	CK_CHUNKID(SPR_BONUS500),
	CK_CHUNKID(SPR_BONUS1000),
	CK_CHUNKID(SPR_BONUS2000),
	CK_CHUNKID(SPR_BONUS5000),
	CK_CHUNKID(SPR_BONUS1UP),
	CK_CHUNKID(SPR_BONUSCLIP),
#ifdef WITH_KEEN5
	CK_CHUNKID(SPR_BONUSSECURITYCARD),
#endif
};

// Episode dependent stuff
int16_t ck_keenMoon;

extern void CK6_ToggleBigSwitch(CK_object *obj, bool p);

void CK_KeenColFunc(CK_object *a, CK_object *b)
{
	if (b->type == CT_CLASS(Item))
	{
		if ((ck_currentEpisode->ep == EP_CK4 && b->user1 > 11) ||
			(ck_currentEpisode->ep == EP_CK5 && b->user1 > 12))
			return;

		SD_PlaySound(ck_itemSounds[b->user1]);

		b->type = 1;
		b->zLayer = 3;
		b->gfxChunk = CK_LookupChunk(ck_itemShadows[b->user1]);

		CK_IncreaseScore(ck_itemPoints[b->user1]);

		//b->yDirection = -1;

		if (b->user1 < 4)
		{
			ck_gameState.keyGems[b->user1]++;
		}
		else if (b->user1 == 10)
		{
			ck_gameState.numLives++;
		}
		else if (b->user1 == 11)
		{
			ck_gameState.numShots += (ck_gameState.difficulty == D_Easy) ? 8 : 5;
		}
#ifdef WITH_KEEN5
		else if ((ck_currentEpisode->ep == EP_CK5) && (b->user1 == 12))
		{
			ck_gameState.ep.ck5.securityCard = 1;
		}
#endif
		CK_SetAction2(b, CK_GetActionByName("CK_ACT_itemNotify"));
	}
	else if (b->type == CT_CLASS(Platform)) //Platform
	{
	impossibleBullet:
		if (!ck_keenState.platform)
			CK_PhysPushY(a, b);
	}
#ifdef WITH_KEEN4
	else if (ck_currentEpisode->ep == EP_CK4)
	{
		if (b->type == CT4_Foot)
		{
			ck_gameState.levelState = LS_Foot;
		}
		else if (b->type == CT4_StunnedCreature && b->user4 == CT4_Bounder)
		{
			if (!ck_keenState.platform)
				CK_PhysPushY(a, b);
		}
		else if (b->type == CT4_Lindsey)
		{
			CK4_ShowPrincessMessage();
			CK_RemoveObj(b);
			// RF_ForceRefresh();
		}
		else if (b->type == CT4_CouncilMember)
		{
			if (a->topTI)
			{
				if (ca_mapOn == 14) // Janitor level
				{
					CK4_ShowJanitorMessage();
					//RF_ForceRefresh();
					CK_RemoveObj(b);
				}
				else
				{
					SD_PlaySound(CK_SOUNDNUM(SOUND_COUNCILSAVE));
					ck_gameState.levelState = LS_CouncilRescued;
				}
			}
		}
		else if (b->type == CT4_Bounder)
		{
			CK_PhysPushXY(a, b, false);
		}
	}
#endif
#ifdef WITH_KEEN5
	else if (ck_currentEpisode->ep == EP_CK5)
	{
	}
#endif
#ifdef WITH_KEEN6
	else if (ck_currentEpisode->ep == EP_CK6)
	{
		if (b->type == CT_Stunner)
		{
			// Reflected off of flect
			if (b->user4)
			{
				CK_ShotHit(b);
				CK_SetAction2(a, CK_GetActionByName("CK6_ACT_keenStunned0"));
			}

			if (CFG_GetConfigBool("impossibleBullet", true))
				goto impossibleBullet;
			// The legendary bug!  For some reason, the control flows to check if
			// keen is riding a platform he contacts his own stunner bullet.
		}
	}
#endif
}

int ck_KeenRunXVels[8] = {0, 0, 4, 4, 8, -4, -4, -8};

int ck_KeenPoleOffs[3] = {-8, 0, 8};

void CK_IncreaseScore(int score)
{
	ck_gameState.keenScore += score;
	if (IN_DemoGetMode() != IN_Demo_Off)
		return;
	if (ck_gameState.keenScore > ck_gameState.nextKeenAt)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_GOTEXTRALIFE));
		ck_gameState.numLives++;
		ck_gameState.nextKeenAt *= 2;
	}
}

void CK_SpawnKeen(int tileX, int tileY, int direction)
{
	ck_keenObj->type = CT_Player; //TODO: obj_keen
	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->visible = true;
	ck_keenObj->zLayer = 1;
	ck_keenObj->clipped = CLIP_normal;
	ck_keenObj->posX = RF_TileToUnit(tileX);
	ck_keenObj->posY = RF_TileToUnit(tileY) - 241;
	ck_keenObj->xDirection = direction;
	ck_keenObj->yDirection = 1;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK_ACT_keenStanding"));
}

static uint16_t emptyTile = 0;

void CK_KeenGetTileItem(int tileX, int tileY, int itemNumber)
{
	RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);
	SD_PlaySound(ck_itemSounds[itemNumber]);

	CK_IncreaseScore(ck_itemPoints[itemNumber]);

	// TODO: Handle more kinds of pick-ups

	if (itemNumber == 11)
	{
		ck_gameState.numShots += (ck_gameState.difficulty == D_Easy) ? 8 : 5;
	}

	CK_object *notify = CK_GetNewObj(true);
	notify->type = 1;
	notify->zLayer = 3;
	notify->posX = RF_TileToUnit(tileX);
	notify->posY = RF_TileToUnit(tileY);
	notify->yDirection = -1;
	notify->user2 = CK_LookupChunk(ck_itemShadows[itemNumber]);
	notify->gfxChunk = notify->user2;
	CK_SetAction(notify, CK_GetActionByName("CK_ACT_itemNotify"));
	notify->clipped = CLIP_not;
}

void CK_KeenGetTileCentilife(int tileX, int tileY)
{
	RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);
	SD_PlaySound(CK_SOUNDNUM(SOUND_GOTCENTILIFE));
	CK_SpawnCentilifeNotify(tileX, tileY);
	if (++ck_gameState.numCentilife == 100)
	{
		ck_gameState.numCentilife = 0;
		SD_PlaySound(CK_SOUNDNUM(SOUND_GOTEXTRALIFE));
		ck_gameState.numLives++;

		// Spawn a 1up shadow
		CK_object *obj = CK_GetNewObj(true);
		obj->type = CT_Friendly;
		obj->zLayer = PRIORITIES - 1;
		obj->posX = RF_TileToUnit(tileX);
		obj->posY = RF_TileToUnit(tileY);
		obj->yDirection = IN_motion_Up;
		obj->user2 = obj->gfxChunk = SPR_CENTILIFE1UPSHADOW;
		CK_SetAction(obj, CK_GetActionByName("CK_ACT_itemNotify"));
		obj->clipped = CLIP_not;
	}
}

void CK_KeenCheckSpecialTileInfo(CK_object *obj)
{
	if (ck_keenMoon == 1)
		ck_keenMoon = 0; // Yes, also set in Keen 5
	for (int y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; ++y)
	{
		for (int x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; ++x)
		{
			int specialTileInfo = (TI_ForeMisc(CA_TileAtPos(x, y, 1)) & 0x7F);
			switch (specialTileInfo)
			{
			case 0:
				break;
			case MISCFLAG_DEADLY:
				CK_KillKeen();
				break;
			case 4:
				CK_KeenGetTileCentilife(x, y);
				break;
			case MISCFLAG_GEMHOLDER0:
			case MISCFLAG_GEMHOLDER1:
			case MISCFLAG_GEMHOLDER2:
			case MISCFLAG_GEMHOLDER3:
				if (y == obj->clipRects.tileY2 && obj->topTI && obj->currentAction != CK_GetActionByName("CK_ACT_keenPlaceGem") && ck_gameState.keyGems[specialTileInfo - MISCFLAG_GEMHOLDER0])
				{
					int targetXUnit = RF_TileToUnit(x) - 64;
					if (obj->posX == targetXUnit)
					{
						ck_gameState.keyGems[specialTileInfo - MISCFLAG_GEMHOLDER0]--;
						CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenPlaceGem"));
					}
					else
					{
						obj->user1 = targetXUnit;
						obj->currentAction = CK_GetActionByName("CK_ACT_keenSlide");
					}
				}
				return;
			case 16:
				if (ck_keenMoon == 0)
					ck_keenMoon = 1; // Yes, also set in Keen 5
				break;
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
				CK_KeenGetTileItem(x, y, specialTileInfo - 17);
				/*CK_KeenGetTileItem(x,y,specialTileInfo - 21 + 4);*/
				break;
			}
		}
	}
}

void CK_KeenPressSwitchThink(CK_object *obj)
{
	uint16_t switchTarget = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 2);
	int switchTargetX = (switchTarget >> 8);
	int switchTargetY = (switchTarget & 0xFF);
	uint16_t switchTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);
	uint8_t switchMisc = TI_ForeMisc(switchTile);

	// Toggle the switch.
	uint16_t switchNextTile = switchTile + TI_ForeAnimTile(switchTile);
	RF_ReplaceTiles(&switchNextTile, 1, obj->clipRects.tileXmid, obj->clipRects.tileY1, 1, 1);
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENOUTOFAMMO));

	if (switchMisc == MISCFLAG_SWITCHBRIDGE)
	{
		for (int tileY = switchTargetY; tileY < switchTargetY + 2; ++tileY)
		{
			for (int tileX = switchTargetX - ((tileY == switchTargetY) ? 0 : 1); tileX < CA_GetMapWidth(); ++tileX)
			{
				uint16_t currentTile = CA_TileAtPos(tileX, tileY, 1);
				if (!TI_ForeAnimTile(currentTile))
					break;
				uint16_t newTile = currentTile + TI_ForeAnimTile(currentTile);
				RF_ReplaceTiles(&newTile, 1, tileX, tileY, 1, 1);
			}
		}
	}
	else
	{
		int infoPlaneInverses[8] = {2, 3, 0, 1, 6, 7, 4, 5};
		uint16_t infoPlaneValue = CA_TileAtPos(switchTargetX, switchTargetY, 2);
		// NOTE: This is only officially supported in Keen 5, though we probably will
		// want to add it for mods.
#ifdef WITH_KEEN5
		if (infoPlaneValue >= 91 && infoPlaneValue < 99)
		{
			// Invert the direction of the goplat arrow.
			infoPlaneValue = infoPlaneInverses[infoPlaneValue - 91] + 91;
		}
		else
#endif
		{
			// Insert or remove a [B] block.
			infoPlaneValue ^= 0x1F;
		}

		CA_mapPlanes[2][switchTargetY * CA_GetMapWidth() + switchTargetX] = infoPlaneValue;
	}
}

bool CK_KeenPressUp(CK_object *obj)
{
	uint8_t tileMiscFlag = TI_ForeMisc(CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1));

	// Are we pressing a switch?
	if (tileMiscFlag == MISCFLAG_SWITCHPLATON || tileMiscFlag == MISCFLAG_SWITCHPLATOFF || tileMiscFlag == MISCFLAG_SWITCHBRIDGE)
	{
		int16_t destXunit = RF_TileToUnit(obj->clipRects.tileXmid) - 64;
		if (obj->posX == destXunit)
		{
			// Flip that switch!
			obj->currentAction = CK_GetActionByName("CK_ACT_keenPressSwitch1");
		}
		else
		{
			obj->user1 = destXunit;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenSlide");
		}
		ck_keenState.keenSliding = true;
		return true;
	}

	// Are we enterting a door?
	if (tileMiscFlag == MISCFLAG_DOOR || tileMiscFlag == MISCFLAG_SECURITYDOOR)
	{
		uint16_t destUnitX = RF_TileToUnit(obj->clipRects.tileXmid) + 96;

		// If the door is two tiles wide, we want to be in the centre.
		uint8_t miscFlagLeft = TI_ForeMisc(CA_TileAtPos(obj->clipRects.tileXmid - 1, obj->clipRects.tileY1, 1));
		if (miscFlagLeft == MISCFLAG_DOOR || miscFlagLeft == MISCFLAG_SECURITYDOOR)
		{
			destUnitX -= 256;
		}

		if (obj->posX == destUnitX)
		{
			//We're at the door.

			// Is it a security door?
#ifdef WITH_KEEN5
			if (tileMiscFlag == MISCFLAG_SECURITYDOOR)
			{
				if (ck_gameState.ep.ck5.securityCard)
				{
					ck_gameState.ep.ck5.securityCard = 0;
					SD_PlaySound(CK_SOUNDNUM(SOUND_OPENSECURITYDOOR));
					CK_object *newObj = CK_GetNewObj(false);
					newObj->posX = obj->clipRects.tileXmid - 2;
					newObj->posY = obj->clipRects.tileY2 - 4;
					newObj->active = OBJ_ALWAYS_ACTIVE;
					newObj->type = 1;
					CK_SetAction(newObj, CK_GetActionByName("CK_ACT_SecurityDoorOpen"));
					obj->currentAction = CK_GetActionByName("CK_ACT_keenEnterSecurityDoor");
					obj->zLayer = 0;
					ck_keenState.keenSliding = true;
					return true;
				}
				else
				{
					SD_PlaySound(CK_SOUNDNUM(SOUND_NEEDKEYCARD));
					obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
					ck_keenState.keenSliding = true;
					return false;
				}
			}
			else
#endif
			{
				ck_invincibilityTimer = 110;
				obj->currentAction = CK_GetActionByName("CK_ACT_keenEnterDoor0");
				obj->zLayer = 0;

#ifdef WITH_KEEN5
				if ((ck_currentEpisode->ep == EP_CK5) && !CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 2))
				{
					CK5_SpawnLightning();
				}
#endif
			}
		}
		else
		{
			obj->user1 = destUnitX;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenSlide");
		}

		ck_keenState.keenSliding = true;

		return true;
	}

	// No? Return to our caller, who will handle poles/looking up.
	return false;
}

// Think function for keen "sliding" towards a switch, keygem or door.
void CK_KeenSlide(CK_object *obj)
{
	int16_t deltaX = obj->user1 - obj->posX;
	if (deltaX < 0)
	{
		// Move left one px per tick.
		ck_nextX -= SD_GetSpriteSync() * 16;
		// If we're not at our target yet, return.
		if (ck_nextX > deltaX)
			return;
	}
	else if (deltaX > 0)
	{
		// Move right one px per tick.
		ck_nextX += SD_GetSpriteSync() * 16;
		// If we're not at our target yet, return.
		if (ck_nextX < deltaX)
			return;
	}

	// We're at our target.
	ck_nextX = deltaX;
	obj->user1 = 0;
	if (!CK_KeenPressUp(obj))
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
	}
}

void CK_KeenEnterDoor0(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK0));
}

void CK_KeenEnterDoor1(CK_object *obj)
{
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK1));
}

// Think function for entering a door.
void CK_KeenEnterDoor(CK_object *obj)
{
	uint16_t destination = CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2, 2);

#ifdef WITH_KEEN5
	if (ck_currentEpisode->ep == EP_CK5)
	{
		if (destination == 0x0000)
		{
			ck_gameState.levelState = LS_TeleportToKorath;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenEnteredDoor");
			return;
		}

		if (destination == 0xB1B1)
		{
			ck_gameState.levelState = LS_LevelComplete;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenEnteredDoor");
			return;
		}
	}
#endif
	obj->posY = RF_TileToUnit((destination & 0xFF)) - 256 + 15;
	obj->posX = RF_TileToUnit((destination >> 8));
	obj->zLayer = 1;
	obj->clipped = CLIP_not;
	CK_SetAction2(obj, obj->currentAction->next);
	obj->clipped = CLIP_normal;
	CK_CentreCamera(obj);
}

void CK_KeenPlaceGem(CK_object *obj)
{
	uint16_t oldFGTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
	uint16_t newFGTile = oldFGTile + 18;
	uint16_t target = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 2);

	int targetX = target >> 8;
	int targetY = target & 0xFF;

	RF_ReplaceTiles(&newFGTile, 1, obj->clipRects.tileXmid, obj->clipRects.tileY2, 1, 1);

	SD_PlaySound(CK_SOUNDNUM(SOUND_OPENGEMDOOR));

	CK_object *newObj = CK_GetNewObj(false);

	newObj->posX = targetX;
	newObj->posY = targetY;

	if (targetX > CA_GetMapWidth() || targetX < 2 || targetY > CA_GetMapHeight() || targetY < 2)
	{
		Quit("Keyholder points to a bad spot!");
	}

	// Calculate the height of the door.
	int doorHeight = 0;
	int doorY = targetY;
	uint16_t doorTile = CA_TileAtPos(targetX, doorY, 1);
	do
	{
		doorHeight++;
		doorY++;
	} while (CA_TileAtPos(targetX, doorY, 1) == doorTile);

	newObj->user1 = doorHeight;
	newObj->clipped = CLIP_not;
	newObj->type = CT_Friendly;
	newObj->active = OBJ_ALWAYS_ACTIVE;
	CK_SetAction(newObj, CK_GetActionByName("CK_ACT_DoorOpen1"));
}

void CK_KeenRidePlatform(CK_object *obj)
{
	// Save the platform pointer, we might be wiping it.
	CK_object *plat = ck_keenState.platform;

	if (obj->clipRects.unitX2 < plat->clipRects.unitX1 || obj->clipRects.unitX1 > plat->clipRects.unitX2)
	{
		// We've fallen off the platform horizontally.
		ck_keenState.platform = 0;
		return;
	}

	if (obj->deltaPosY < 0)
	{
		// If we've jumped off the platform.
		ck_keenState.platform = 0;
		if (plat->deltaPosY < 0)
		{
			ck_nextX = 0;
			ck_nextY = plat->deltaPosY;
			CK_PhysUpdateSimpleObj(obj);
			return;
		}
	}
	else
	{
		//Ride the platform
		ck_nextX = plat->deltaPosX;
		ck_nextY = plat->clipRects.unitY1 - obj->clipRects.unitY2 - 16;
		CK_PhysUpdateSimpleObj(obj);

		if (vl_noPan)
		{
			// Keen has a "/NOPAN" parameter which disables use of the EGA panning
			// register. In order to make platforms less ugly when this is on
			// (though not much less ugly, IMHO), clamp Keen's movement to the nearest
			// 8px, so that scrolling timing is aligned with the platform.
			// This causes a bug where you can't always walk off bounders, see:
			//   https://pckf.com/viewtopic.php?p=102264
			//   https://www.youtube.com/watch?v=cScZlYPhFnU
			obj->posX &= ~0x7F;
			obj->posX |= plat->posX & 0x7F;
		}
		else
		{
		
			// Make Keen's x-position move in tandem with the platform, so that they appear to move as a unit,
			// and (more importantly) that the platform moves in time with the camera scrolling (which is based
			// on Keen's position).
			// Technically, this probably should clear out the low bits like the /NOPAN case, so that their subpixel
			// values are identical.
			obj->posX |= plat->posX & 0x1F;
		}

		// We've hit the ceiling?
		if (obj->bottomTI)
		{
			ck_keenState.platform = 0;
			return;
		}

		// We're standing on something, don't fall down!
		obj->topTI = 0x19;
	}
}

bool CK_KeenTryClimbPole(CK_object *obj)
{
	if (SD_GetLastTimeCount() < ck_keenState.poleGrabTime)
		ck_keenState.poleGrabTime = 0;
	else if (SD_GetLastTimeCount() - ck_keenState.poleGrabTime < 19)
		return false;

	uint16_t candidateTile = CA_TileAtPos(obj->clipRects.tileXmid,
		((ck_inputFrame.yDirection == -1) ? (RF_UnitToTile(obj->clipRects.unitY1 + 96)) : (obj->clipRects.tileY2 + 1)), 1);

	if ((TI_ForeMisc(candidateTile) & 0x7F) == 1)
	{
		obj->posX = 128 + RF_TileToUnit(obj->clipRects.tileXmid - 1);
		ck_nextX = 0;
		ck_nextY = (ck_inputFrame.yDirection << 5);
		obj->clipped = CLIP_not;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleSit");
		return true;
	}
	return false;
}

void CK_HandleInputOnGround(CK_object *obj);

void CK_KeenRunningThink(CK_object *obj)
{
	if (!ck_inputFrame.xDirection)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		CK_HandleInputOnGround(obj);
		return;
	}

	obj->xDirection = ck_inputFrame.xDirection;

	if (ck_inputFrame.yDirection == -1)
	{
		if (CK_KeenTryClimbPole(obj))
			return;
		if (ck_keenState.keenSliding || CK_KeenPressUp(obj))
			return;
	}
	else if (ck_inputFrame.yDirection == 1)
	{
		if (CK_KeenTryClimbPole(obj))
			return;
	}

	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;

		if (ck_inputFrame.yDirection == -1)
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenShootUp1");
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenShoot1");
		}
		return;
	}

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENJUMP));
		obj->velX = obj->xDirection * 16;
		obj->velY = -40;
		ck_nextX = ck_nextY = 0;
		ck_keenState.jumpTimer = 18;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		// Is this the mystical 'impossible pogo'?
		// k5disasm has a 'return' here, probably a mistake?
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENJUMP));
		obj->velX = obj->xDirection * 16;
		obj->velY = -48;
		// Should this be nextY? Andy seems to think so, but lemm thinks that X is right...
		ck_nextX = 0;
		ck_keenState.jumpTimer = 24;
		return;
	}

	// Andy seems to think this is Y as well. Need to do some more disasm.
	// If this is an X vel, then surely we'd want to multiply it by the direction?
	ck_nextX += ck_KeenRunXVels[obj->topTI & 7] * SD_GetSpriteSync();

	if ((obj->currentAction->chunkLeft == CK_GetActionByName("CK_ACT_keenRun1")->chunkLeft) && !(obj->user3))
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK0));
		obj->user3 = 1;
		return;
	}

	if ((obj->currentAction->chunkLeft == CK_GetActionByName("CK_ACT_keenRun3")->chunkLeft) && !(obj->user3))
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENWALK1));
		obj->user3 = 1;
		return;
	}
	if ((obj->currentAction->chunkLeft == CK_GetActionByName("CK_ACT_keenRun2")->chunkLeft) || (obj->currentAction->chunkLeft == CK_GetActionByName("CK_ACT_keenRun4")->chunkLeft))
	{
		obj->user3 = 0;
	}
}

void CK_HandleInputOnGround(CK_object *obj)
{
	// If we're riding a platform, do it surfin' style!
	if (obj->topTI == 0x19)
	{
		// But only if such an action exists in this episode. :/
		if (CK_GetActionByName("CK_ACT_keenRidePlatform"))
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenRidePlatform");
		}
	}

	if (ck_inputFrame.xDirection)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenRun1");
		CK_KeenRunningThink(obj);
		ck_nextX = (obj->xDirection * obj->currentAction->velX * SD_GetSpriteSync()) / 4;
		return;
	}

	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		if (ck_inputFrame.yDirection == -1)
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenShootUp1");
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenShoot1");
		}
		return;
	}

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENJUMP));
		obj->velX = 0;
		obj->velY = -40;
		ck_nextY = 0;
		ck_keenState.jumpTimer = 18;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		return;
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENJUMP));
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		obj->velX = 0;
		obj->velY = -48;
		ck_nextY = 0;
		ck_keenState.jumpTimer = 24;
		return;
	}

	if (ck_inputFrame.yDirection == -1)
	{
		if (CK_KeenTryClimbPole(obj))
			return;
		if (!ck_keenState.keenSliding && CK_KeenPressUp(obj))
			return;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenLookUp1");
	}
	else if (ck_inputFrame.yDirection == 1)
	{
		// Try poles.
		if (CK_KeenTryClimbPole(obj))
			return;
		// Keen looks down.
		obj->currentAction = CK_GetActionByName("CK_ACT_keenLookDown1");
		return;
	}
}

void CK_KeenStandingThink(CK_object *obj)
{
	// If we're riding a platform, do it surfin' style!
	if (obj->topTI == 0x19)
	{
		// But only if such an action exists in this episode. :/
		if (CK_GetActionByName("CK_ACT_keenRidePlatform"))
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenRidePlatform");
		}
	}

	if (ck_inputFrame.xDirection || ck_inputFrame.yDirection || ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed || ck_keenState.shootIsPressed)
	{
		obj->user1 = obj->user2 = 0; //Idle Time + Idle State
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		CK_HandleInputOnGround(obj);

		return;
	}

	//If not on platform
	if ((obj->topTI & ~7) != 0x18)
		obj->user1 += SD_GetSpriteSync();

	if (obj->user2 == 0 && obj->user1 > 200)
	{
		obj->user2++;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenIdle");
		obj->user1 = 0;
		return;
	}

	if (obj->user2 == 1 && obj->user1 > 300)
	{
		obj->user2++;
		if ((ck_currentEpisode->ep == EP_CK4) && (ck_keenMoon == 1))
		{
			ck_keenMoon = 2;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenMoon1");
		}
		else
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_keenBored1");
		}
		obj->user1 = 0;
		return;
	}

	if (obj->user2 == 2 && obj->user1 > 700)
	{
		obj->user2++;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenOpenBook1");
		obj->user1 = 0;
	}
}

void CK_KeenLookUpThink(CK_object *obj)
{
	if (ck_inputFrame.yDirection != -1 ||
		ck_inputFrame.xDirection != 0 ||
		(ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed) ||
		(ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed) ||
		(ck_keenState.shootIsPressed))
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		CK_HandleInputOnGround(obj);
	}
}

void CK_KeenLookDownThink(CK_object *obj)
{
	//Try to jump down
	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed && (obj->topTI & 7) == 1)
	{
		ck_keenState.jumpWasPressed = true;

		//If the tiles below the player are blocking on any side but the top, they cannot be jumped through
		int tile1 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
		int tile2 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2 + 1, 1);

		if (TI_ForeLeft(tile1) || TI_ForeBottom(tile1) || TI_ForeRight(tile1))
			return;

		if (TI_ForeLeft(tile2) || TI_ForeBottom(tile2) || TI_ForeRight(tile2))
			return;

		uint16_t deltay = CK_Cross_max(SD_GetSpriteSync(), 4) << 4;

		//Moving platforms
		if (ck_keenState.platform)
			deltay += ck_keenState.platform->deltaPosY;

		ck_keenState.platform = 0;

		obj->clipRects.unitY2 += deltay;
		obj->posY += deltay;
		ck_nextX = 0;
		ck_nextY = 0;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		obj->velX = obj->velY = 0;
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENFALL));
		return;
	}

	if (ck_inputFrame.yDirection != 1 || ck_inputFrame.xDirection != 0 || (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed) || (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed))
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenLookDown4");
		return;
	}
}

// TODO: More to modify here
void CK_KeenDrawFunc(CK_object *obj)
{
	if (!obj->topTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENFALL));
		obj->velX = obj->xDirection * 8;
		obj->velY = 0;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
	}
	else if ((obj->topTI & 0xFFF8) == 8)
	{
		CK_KillKeen();
	}
	else if (obj->topTI == 0x29)
	{
		// Keen6 conveyor belt right
		ck_nextX = SD_GetSpriteSync() * 8;
		ck_nextY = 0;
		obj->user1 = 0;
		CK_PhysUpdateNormalObj(obj);
	}
	else if (obj->topTI == 0x31)
	{
		// Keen6 conveyor belt left
		ck_nextX = SD_GetSpriteSync() * -8;
		ck_nextY = 0;
		obj->user1 = 0;
		CK_PhysUpdateNormalObj(obj);
	}
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// TODO: More to modify here
void CK_KeenRunDrawFunc(CK_object *obj)
{

	if (!obj->topTI)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENFALL));
		obj->velX = obj->xDirection * 8;
		obj->velY = 0;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
	}
	else if ((obj->topTI & 0xFFF8) == 8)
	{
		CK_KillKeen();
	}
	else if (obj->topTI == 0x29)
	{
		// Keen6 conveyor belt right
		ck_nextX = SD_GetSpriteSync() * 8;
		ck_nextY = 0;
		obj->user1 = 0;
		CK_PhysUpdateNormalObj(obj);
	}
	else if (obj->topTI == 0x31)
	{
		// Keen6 conveyor belt left
		ck_nextX = SD_GetSpriteSync() * -8;
		ck_nextY = 0;
		obj->user1 = 0;
		CK_PhysUpdateNormalObj(obj);
	}

	// If we're running into a wall, we should stand still.
	if ((obj->rightTI && obj->xDirection == -1) || (obj->leftTI && obj->xDirection == 1))
	{
		obj->actionTimer = 0;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		obj->gfxChunk = (obj->xDirection == -1) ? obj->currentAction->chunkLeft : obj->currentAction->chunkRight;
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenReadThink(CK_object *obj)
{
	if (ck_inputFrame.xDirection != 0 || ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStowBook1");
		obj->user1 = obj->user2 = 0;
	}
}

void CK_KeenJumpThink(CK_object *obj)
{
	// Handle Jump Cheat
	if (ck_gameState.jumpCheat && ck_keenState.jumpIsPressed)
	{
		obj->velY = -40;
		ck_keenState.jumpTimer = 18;
		ck_keenState.jumpWasPressed = true;
	}

	if (ck_keenState.jumpTimer)
	{
		if (ck_keenState.jumpTimer <= SD_GetSpriteSync())
		{
			ck_nextY = obj->velY * ck_keenState.jumpTimer;
			ck_keenState.jumpTimer = 0;
		}
		else
		{
			ck_nextY = obj->velY * SD_GetSpriteSync();
			if (!ck_gameState.jumpCheat)
			{
				ck_keenState.jumpTimer -= SD_GetSpriteSync();
			}
		}

		// Stop moving up if we've let go of control.
		if (!ck_keenState.jumpIsPressed)
		{
			ck_keenState.jumpTimer = 0;
		}

		if (!ck_keenState.jumpTimer && obj->currentAction->next)
		{
			obj->currentAction = obj->currentAction->next;
		}
	}
	else
	{
		//TODO: Check this w/ K5Disasm and/or x-disasm
		if (ck_gameState.difficulty == D_Easy)
		{
			CK_PhysGravityMid(obj);
		}
		else // Normal or Hard
		{
			CK_PhysGravityHigh(obj);
		}

		if (obj->velY > 0 && obj->currentAction != CK_GetActionByName("CK_ACT_keenFall1") && obj->currentAction != CK_GetActionByName("CK_ACT_keenFall2"))
		{
			obj->currentAction = obj->currentAction->next;
		}
	}

	//Move horizontally
	if (ck_inputFrame.xDirection)
	{

		obj->xDirection = ck_inputFrame.xDirection;
		CK_PhysAccelHorz(obj, ck_inputFrame.xDirection * 2, 24);
	}
	else
		CK_PhysDampHorz(obj);

	//Pole
	if (obj->bottomTI == 17)
	{
		ck_nextX = 0;
		obj->velX = 0;
	}

	//Shooting
	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		switch (ck_inputFrame.yDirection)
		{
		case -1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShootUp1");
			break;
		case 0:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShoot1");
			break;
		case 1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShootDown1");
			break;
		}
		return;
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo2");
		ck_keenState.jumpTimer = 0;
		return;
	}

	if (ck_inputFrame.yDirection == -1)
	{
		CK_KeenTryClimbPole(obj);
	}
}

extern int rf_scrollXUnit, rf_scrollYUnit;

void CK_KeenJumpDrawFunc(CK_object *obj)
{
	if (obj->rightTI && obj->xDirection == -1)
	{
		obj->velX = 0;
	}
	else if (obj->leftTI && obj->xDirection == 1)
	{
		obj->velX = 0;
	}

	// Did we hit our head on the ceiling?
	if (obj->bottomTI)
	{
		//TODO: Something to do with poles (push keen into the centre)
		if (obj->bottomTI == 17) //Pole
		{
			obj->posY -= 32;
			obj->clipRects.unitY1 -= 32;
			obj->velX = 0;
			obj->posX = RF_TileToUnit(obj->clipRects.tileXmid) - 32;
		}
		else
		{
#ifdef WITH_KEEN6
			if (obj->bottomTI == 0x21) // Bloog switches
				CK6_ToggleBigSwitch(obj, false);
#endif

			if (!ck_gameState.jumpCheat)
			{
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENHITCEILING));
				if (obj->bottomTI > 1)
				{
					obj->velY += 16;
					if (obj->velY < 0)
						obj->velY = 0;
				}
				else
				{
					obj->velY = 0;
				}
				ck_keenState.jumpTimer = 0;
			}
		}
	}

	// Have we landed?
	if (obj->topTI)
	{
		obj->deltaPosY = 0;
		//Check if deadly.
		if ((obj->topTI & ~7) == 8)
		{
			CK_KillKeen();
		}
		else
		{
			if (obj->topTI == 0x39) // Fuse
			{
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENLANDONFUSE));
			}
#ifdef WITH_KEEN6
			if (ck_currentEpisode->ep == EP_CK6 && obj->topTI == 0x21) // BigSwitch
			{
				CK6_ToggleBigSwitch(obj, true);
			}
#endif
			if (obj->topTI != 0x19 || !ck_keenState.jumpTimer) // Or standing on a platform.
			{
				obj->user1 = obj->user2 = 0; // Being on the ground is boring.

				//TODO: Finish these
				if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShoot1"))
				{
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenShoot1"));
				}
				else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShootUp1"))
				{
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenShootUp1"));
				}
				else if (ck_inputFrame.xDirection)
				{
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenRun1"));
				}
				else
				{
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenStanding"));
				}
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENLAND));
			}
		}
	}
	else if (obj->deltaPosY > 0)
	{
		// temp6 = Keen's previous upper y coord
		int temp6 = obj->clipRects.unitY1 - obj->deltaPosY;
		// temp8 = Keen's current upper y coord - 1.5 tiles, rounded to nearest tile, + 1.5 tiles
		int temp8 = ((obj->clipRects.unitY1 - 64) & 0xFF00) + 64;
		// temp10 = temp8 in tile coords, - 1
		int temp10 = RF_UnitToTile(temp8) - 1;

		// If we're moving past a tile boundary.
		if (temp6 < temp8 && obj->clipRects.unitY1 >= temp8)
		{
			// Moving left...
			if (ck_inputFrame.xDirection == -1)
			{
				int tileX = obj->clipRects.tileX1 - ((obj->rightTI) ? 1 : 0);
				int tileY = temp10;
				//VL_ScreenRect((tileX << 4) - (rf_scrollXUnit >> 4), (tileY << 4) - (rf_scrollYUnit >> 4), 16, 16, 1);
				int upperTile = CA_TileAtPos(tileX, tileY, 1);
				int lowerTile = CA_TileAtPos(tileX, tileY + 1, 1);
				if ((!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile)) &&
					TI_ForeRight(lowerTile) && TI_ForeTop(lowerTile))
				{
					obj->xDirection = -1;
					obj->clipped = CLIP_not;
					obj->posX = (obj->posX & 0xFF00) + 128;
					obj->posY = (temp8 - 64);
					obj->velY = obj->deltaPosY = 0;
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenHang1"));
				}
			}
			else if (ck_inputFrame.xDirection == 1)
			{
				int tileX = obj->clipRects.tileX2 + ((obj->leftTI) ? 1 : 0);
				int tileY = temp10;
				int upperTile = CA_TileAtPos(tileX, tileY, 1);
				int lowerTile = CA_TileAtPos(tileX, tileY + 1, 1);

				if (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile) &&
					TI_ForeLeft(lowerTile) && TI_ForeTop(lowerTile))
				{
					obj->xDirection = 1;
					obj->clipped = CLIP_not;
					obj->posX = (obj->posX & 0xFF00) + 256;
					obj->posY = (temp8 - 64);
					obj->velY = obj->deltaPosY = 0;
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenHang1"));
				}
			}
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenPogoBounceThink(CK_object *obj)
{
	obj->velY = -48;
	ck_nextY = 6 * obj->velY;
	ck_keenState.jumpTimer = 24;
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENPOGO));
}

void CK_KeenPogoThink(CK_object *obj)
{
	if (!ck_keenState.jumpTimer)
	{
		if (ck_gameState.difficulty == D_Easy)
		{
			CK_PhysGravityMid(obj);
		}
		else
		{
			CK_PhysGravityHigh(obj);
		}
	}
	else
	{
		if (ck_keenState.jumpIsPressed || ck_keenState.jumpTimer <= 9)
			CK_PhysGravityLow(obj);
		else
			CK_PhysGravityHigh(obj);

		if (ck_keenState.jumpTimer <= SD_GetSpriteSync())
			ck_keenState.jumpTimer = 0;
		else
			ck_keenState.jumpTimer -= SD_GetSpriteSync();

		if (!ck_keenState.jumpTimer && obj->currentAction->next)
			obj->currentAction = obj->currentAction->next;
	}
	if (ck_inputFrame.xDirection)
	{
		if (!obj->velX)
			obj->xDirection = ck_inputFrame.xDirection;
		CK_PhysAccelHorz(obj, ck_inputFrame.xDirection, 24);
	}
	else
	{
		ck_nextX += obj->velX * SD_GetSpriteSync();
		if (obj->velX < 0)
			obj->xDirection = -1;
		else if (obj->velX > 0)
			obj->xDirection = 1;
	}

	// Stop for poles?
	if (obj->bottomTI == 17)
	{
		ck_nextX = 0;
		obj->velX = 0;
	}

	//Shooting
	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		switch (ck_inputFrame.yDirection)
		{
		case -1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShootUp1");
			return;
		case 0:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShoot1");
			return;
		case 1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenJumpShootDown1");
			return;
		}
	}

	//Stop pogoing if Alt pressed
	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
	}
}

#ifdef WITH_KEEN5
void CK_KeenBreakFuse(int x, int y)
{
	CK5_SpawnFuseExplosion(x, y);
	if (!(--ck_gameState.ep.ck5.fusesRemaining))
	{
		CK5_SpawnLevelEnd();
	}

	uint16_t brokenFuseTiles[] = {0, 0};
	brokenFuseTiles[0] = CA_TileAtPos(0, 0, 1);
	brokenFuseTiles[1] = CA_TileAtPos(0, 1, 1);

	RF_ReplaceTiles(brokenFuseTiles, 1, x, y, 1, 2);
}
#endif

void CK_KeenPogoDrawFunc(CK_object *obj)
{
	if (obj->rightTI && obj->xDirection == -1)
	{
		obj->velX = 0;
	}
	else if (obj->leftTI && obj->xDirection == 1)
	{
		obj->velX = 0;
	}

	if (obj->bottomTI)
	{

		if (obj->bottomTI == 17) //Pole
		{
			obj->posY -= 32;
			obj->clipRects.unitY1 -= 32;
			obj->velX = 0;
			obj->posX = RF_TileToUnit(obj->clipRects.tileXmid) - 32;
		}
		else
		{
#ifdef WITH_KEEN6
			if (obj->bottomTI == 0x21) // Bloog switches
				CK6_ToggleBigSwitch(obj, false);
#endif

			if (!ck_gameState.jumpCheat)
			{
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENHITCEILING));

				if (obj->bottomTI > 1)
				{
					obj->velY += 16;
					if (obj->velY < 0)
						obj->velY = 0;
				}
				else
					obj->velY = 0;

				ck_keenState.jumpTimer = 0;
			}
		}
	}

	// Houston, we've landed!
	if (obj->topTI)
	{
		obj->deltaPosY = 0;
		//Check if deadly.
		if ((obj->topTI & ~7) == 8)
		{
			CK_KillKeen();
		}
		else
		{
#ifdef WITH_KEEN6
			if (ck_currentEpisode->ep == EP_CK6 && obj->topTI == 0x21) // BigSwitch
			{
				CK6_ToggleBigSwitch(obj, true);
			}
#endif
#ifdef WITH_KEEN5
			if (obj->topTI == 0x39) // Fuse
			{
				if (obj->velY >= 0x30)
				{
					CK_KeenBreakFuse(obj->clipRects.tileXmid, obj->clipRects.tileY2);
					RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
					return;
				}
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENLANDONFUSE));
			}
#endif
			if (obj->topTI != 0x19 || ck_keenState.jumpTimer == 0)
			{
				obj->velY = -48;
				ck_keenState.jumpTimer = 24;
				SD_PlaySound(CK_SOUNDNUM(SOUND_KEENPOGO));
				CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenPogo2"));
			}
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenSpecialColFunc(CK_object *obj, CK_object *other)
{
	if (other->type == CT_CLASS(Platform))
	{
		// NOTE: Keen 6 fixes the "pole foreground bug", where Keen can
		// get stuck with a zLayer of 3 if ledge-grabbing is interrupted.
		// See:
		//     https://pckf.com/viewtopic.php?t=12901
		if (CFG_GetConfigBool("ck_fixPlatformZLayer", (ck_currentEpisode->ep == EP_CK6)))
			obj->zLayer = 1;
		obj->clipped = CLIP_normal;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
		obj->velX = 0;
		obj->velY = 0;
		CK_PhysPushY(obj, other);
	}
#ifdef WITH_KEEN4
	else if ((ck_currentEpisode->ep == EP_CK4) &&
		((other->type == CT4_Mushroom) || (other->type == CT4_Arachnut) || (other->type == CT4_Berkeloid)))
	{
		CK_KillKeen();
	}
#endif
	else if (
#ifdef WITH_KEEN4
		((ck_currentEpisode->ep == EP_CK4) && (other->type == CT4_Bounder)) ||
#endif
#ifdef WITH_KEEN5
		((ck_currentEpisode->ep == EP_CK5) && ((other->type == CT5_Ampton) || (other->type == CT5_Korath))) ||
#endif
#ifdef WITH_KEEN6
		((ck_currentEpisode->ep == EP_CK6) && ((other->type == CT6_Gik) || (other->type == CT6_Flect) || (other->type == CT6_Blooglet))) ||
#endif
		false)
	{
		obj->zLayer = 1;
		obj->clipped = CLIP_normal;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
		obj->velX = 0;
		obj->velY = 0;

		// Keen's yDirection needs to be Down (1) when on the ground, or
		// we can hit _nasty_ issues when going through a door, leading to
		// a visual glitch (walking down into the door) and an exploit
		// (the door's target is read from the wrong tile, becoming 0000 in
		// most cases, leading to finishing the level, and possibly going to
		// the secret level in Keen 5).
		// See:
		//   https://keenwiki.shikadi.net/wiki/Keen_5_Bugs#Ampton_Pole_Push_Door_Trick
		//   https://keenwiki.shikadi.net/wiki/Keen_5_Bugs#Shelley.2FDoor_Trick
		//   https://keenwiki.shikadi.net/wiki/Keen_4_Bugs#Bounder_Pole_Door_Glitch
		if (!CFG_GetConfigBool("ck_poleDoorGlitch", true))
			obj->yDirection = IN_motion_Down;
#ifdef WITH_KEEN4
		if (ck_currentEpisode->ep == EP_CK4)
			CK_PhysPushXY(obj, other, false);
#endif
#ifdef WITH_KEEN6
		if (ck_currentEpisode->ep == EP_CK6)
			CK_PhysPushY(obj, other);
#endif
	}
}

void CK_KeenSpecialDrawFunc(CK_object *obj)
{
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// Used by CK6
void CK_KeenJumpDownThink(CK_object *obj)
{
	obj->clipped = CLIP_normal;
}

void CK_KeenHangThink(CK_object *obj)
{
	if (ck_inputFrame.yDirection == -1 || ck_inputFrame.xDirection == obj->xDirection)
	{
		uint16_t tile;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPull1");

		if (obj->xDirection == 1)
		{
			tile = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 - 1, 1);
			ck_nextY = -256;
		}
		else
		{
			tile = CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1 - 1, 1);
			ck_nextY = -128;
		}

		if (!(TI_ForeMisc(tile) & 0x80))
			obj->zLayer = 3;
	}
	else if (ck_inputFrame.yDirection == 1 || (ck_inputFrame.xDirection && ck_inputFrame.xDirection != obj->xDirection))
	{
		// Drop down.
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		obj->clipped = CLIP_normal;
	}
}

void CK_KeenPullThink1(CK_object *obj)
{
	if (obj->xDirection == 1)
		ck_nextX = 128;
	else
		ck_nextY = -128;
}

void CK_KeenPullThink2(CK_object *obj)
{
	ck_nextX = obj->xDirection * 128;
	ck_nextY = -128;
}

void CK_KeenPullThink3(CK_object *obj)
{
	ck_nextY = -128;
}

void CK_KeenPullThink4(CK_object *obj)
{
	obj->clipped = CLIP_normal;
	obj->zLayer = 1;
	ck_nextY = 128;
}

void CK_KeenDeathThink(CK_object *obj)
{
	CK_PhysGravityMid(obj);
	ck_nextX = obj->velX * SD_GetSpriteSync();
	if (!CK_ObjectVisible(obj))
	{
		ck_gameState.levelState = LS_Died;
	}
}

void CK_KillKeen()
{
	CK_object *obj = ck_keenObj;
	if (ck_invincibilityTimer || ck_godMode || (obj->currentAction == CK_GetActionByName("CK_ACT_keenNot")))
	{
		return;
	}

	ck_keenMoon = 0; // Yes, also set in Keen 5
	ck_invincibilityTimer = 30;
	ck_scrollDisabled = true;
	obj->clipped = CLIP_not;
	obj->zLayer = 3;
	if ((ck_currentEpisode->ep == EP_CK4) && ca_mapOn == 17)
	{
		if (US_RndT() < 0x80)
		{
			CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_KeenSwimDie0"));
		}
		else
		{
			CK_SetAction2(obj, CK_GetActionByName("CK4_ACT_KeenSwimDie1"));
		}
	}
	else
	{
		if (US_RndT() < 0x80)
		{
			CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenDie0"));
		}
		else
		{
			CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenDie1"));
		}
	}

	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENDIE));

	obj->velY = -40;
	obj->velX = 16;
}

void CK_KeenPoleHandleInput(CK_object *obj)
{
	if (ck_inputFrame.xDirection)
		obj->xDirection = ck_inputFrame.xDirection;

	//Shooting things. *ZAP!*
	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;

		switch (ck_inputFrame.yDirection)
		{
		case -1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleShootUp1");
			break;
		case 0:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleShoot1");
			break;
		case 1:
			obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleShootDown1");
			break;
		}
	}

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENJUMP));
		obj->velX = ck_KeenPoleOffs[ck_inputFrame.xDirection + 1];
		obj->velY = -20;
		obj->clipped = CLIP_normal;
		ck_keenState.jumpTimer = 10;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		obj->yDirection = 1;
		ck_keenState.poleGrabTime = SD_GetLastTimeCount();
	}
	return;
}

void CK_KeenPoleDownThink(CK_object *obj);

void CK_KeenPoleSitThink(CK_object *obj)
{
	//TODO: Support climb up/down
	if (ck_inputFrame.yDirection == 1)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleDown1");
		obj->yDirection = 1;
		CK_KeenPoleDownThink(obj);
		return;
	}
	else if (ck_inputFrame.yDirection == -1)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleUp1");
		obj->yDirection = -1;
		//CK_KeenPoleUpThink(obj);
		return;
	}

	// When keen is at ground level, allow dismounting using left/right.
	if (ck_inputFrame.xDirection)
	{
		int groundTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2 + 1, 1);
		if (TI_ForeTop(groundTile))
		{
			obj->velX = 0;
			obj->velY = 0;
			obj->clipped = CLIP_normal;
			ck_keenState.jumpTimer = 0;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
			obj->yDirection = 1;
			SD_PlaySound(CK_SOUNDNUM(SOUND_KEENFALL));
			return;
		}
	}

	//TODO: Pole input func
	CK_KeenPoleHandleInput(obj);
}

void CK_KeenPoleUpThink(CK_object *obj)
{
	int topTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);

	if ((TI_ForeMisc(topTile) & 127) != 1)
	{
		ck_nextY = 0;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleSit");
		CK_KeenPoleHandleInput(obj);
		return;
	}

	if (ck_inputFrame.yDirection == 0)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleSit");
		obj->yDirection = 0;
		//CK_KeenPoleSitThink(obj);
	}
	else if (ck_inputFrame.yDirection == 1)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleDown1");
		obj->yDirection = 1;
		CK_KeenPoleDownThink(obj);
	}

	CK_KeenPoleHandleInput(obj);
}

void CK_KeenPoleDownThink(CK_object *obj)
{

	int tileUnderneath = CA_TileAtPos(obj->clipRects.tileXmid, RF_UnitToTile(obj->clipRects.unitY2 - 64), 1);

	if ((TI_ForeMisc(tileUnderneath) & 127) != 1)
	{
		// We're no longer on a pole.
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENFALL));
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		ck_keenState.jumpTimer = 0;
		obj->velX = ck_KeenPoleOffs[ck_inputFrame.xDirection + 1];
		obj->velY = 0;
		obj->clipped = CLIP_normal;
		obj->clipRects.tileY2 -= 1; //WTF?
		return;
	}

	if (ck_inputFrame.yDirection == 0)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleSit");
		obj->yDirection = 0;
	}
	else if (ck_inputFrame.yDirection == -1)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPoleUp1");
		obj->yDirection = -1;
	}

	CK_KeenPoleHandleInput(obj);
}

void CK_KeenPoleDownDrawFunc(CK_object *obj)
{
	// Check if keen is trying to climb through the floor.
	// It's quite a strange clipping error if he succeeds.

	int groundTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);

	if (TI_ForeTop(groundTile) == 1)
	{
		int yReset = -(obj->clipRects.unitY2 & 255) - 1;
		obj->posY += yReset;
		obj->clipRects.unitY2 += yReset;
		obj->clipRects.tileY2 += -1;
		obj->clipped = CLIP_normal;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenLookDown1"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// Defined for a seemingly unused state
void CK_KeenSetClipped(CK_object *obj)
{
	obj->clipped = CLIP_normal;
}

// Shooting

void CK_SpawnShot(int x, int y, int direction)
{
	if (!ck_gameState.numShots)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_KEENOUTOFAMMO));
		return;
	}

	ck_gameState.numShots--;

	CK_object *shot = CK_GetNewObj(true);
	shot->posX = x;
	shot->posY = y;
	shot->zLayer = 0;
	shot->type = CT_Stunner; // TODO: obj_stunner
	shot->active = OBJ_ALWAYS_ACTIVE;

	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENSHOOT));

	switch (direction)
	{
	case 0:
		shot->xDirection = 0;
		shot->yDirection = -1;
		break;
	case 2:
		shot->xDirection = 1;
		shot->yDirection = 0;
		break;
	case 4:
		shot->xDirection = 0;
		shot->yDirection = 1;
		break;
	case 6:
		shot->xDirection = -1;
		shot->yDirection = 0;
		break;
	default:
		Quit("SpawnShot: Bad dir!");
	}

	CK_SetAction(shot, CK_GetActionByName("CK_ACT_keenShot1"));

	// This assists when Keen may shoot "through" a blooglet (see last demo in Keen 6 v1.5)
	if (ck_currentEpisode->ep == EP_CK6)
	{
		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
			if (currentObj->active &&
				(shot->clipRects.unitX2 > currentObj->clipRects.unitX1) &&
				(shot->clipRects.unitX1 < currentObj->clipRects.unitX2) &&
				(shot->clipRects.unitY1 < currentObj->clipRects.unitY2) &&
				(shot->clipRects.unitY2 > currentObj->clipRects.unitY1) &&
				currentObj->currentAction->collide)
				currentObj->currentAction->collide(currentObj, shot);
	}
}

void CK_ShotHit(CK_object *obj)
{
	//TODO: Implement obj_ classes.
	obj->type = CT_Friendly;
	CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenShotHit1"));
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENSHOTHIT));
}

void CK_ShotThink(CK_object *shot)
{
	// Stun things which are offscreen.
	if ((shot->clipRects.tileX2 < RF_UnitToTile(rf_scrollXUnit)) ||
		(shot->clipRects.tileY2 < RF_UnitToTile(rf_scrollYUnit)) ||
		(shot->clipRects.tileX1 > RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320)) ||
		(shot->clipRects.tileY1 > RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208)))
	{
		if ((shot->clipRects.tileX2 + 10 < RF_UnitToTile(rf_scrollXUnit)) ||
			(shot->clipRects.tileX1 - 10 > RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320)) ||
			(shot->clipRects.tileY2 + 6 < RF_UnitToTile(rf_scrollYUnit)) ||
			(shot->clipRects.tileY1 - 6 > RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208)))
		{
			CK_RemoveObj(shot);
			return;
		}
		for (CK_object *currentObj = ck_keenObj->next; currentObj; currentObj = currentObj->next)
		{
			if (currentObj->active ||
				(shot->clipRects.unitX2 <= currentObj->clipRects.unitX1) ||
				(shot->clipRects.unitX1 >= currentObj->clipRects.unitX2) ||
				(shot->clipRects.unitY1 >= currentObj->clipRects.unitY2) ||
				(shot->clipRects.unitY2 <= currentObj->clipRects.unitY1))
				continue;
			if (currentObj->currentAction->collide)
			{
				currentObj->currentAction->collide(currentObj, shot);
				currentObj->visible = true;
				currentObj->active = OBJ_ACTIVE;
			}
			if (shot->type == CT_Nothing)
				return;
		}
	}
}

void CK_ShotDrawFunc(CK_object *obj)
{
	uint16_t t;

	// shoot down through a pole hole
	if (obj->topTI == 1 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
	{
		t = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 - 1, 1);
		if (TI_ForeTop(t) == 0x11)
		{
			obj->topTI = 0x11;
			obj->posX += 0x100 - (obj->posX & 0xFF);
		}
	}
	// move into pole hole before making contact
	else if (obj->topTI == 0x11 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
	{
		obj->posX &= 0xFF00;
	}

	// shoot through pole hole upwards
	if (obj->bottomTI == 1 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
	{
		t = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 + 1, 1);
		if (TI_ForeBottom(t) == 0x11)
		{
			obj->bottomTI = 0x11;
			obj->posX += 0x100 - (obj->posX & 0xFF);
		}
	}
	// move into pole hole whilst travelling upwards
	else if (obj->bottomTI == 0x11 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
	{
		obj->posX &= 0xFF00;
	}

	// if hit any other type of object, die
	if (obj->topTI != 0x11 && obj->bottomTI != 0x11)
	{
		if (obj->topTI || obj->bottomTI || obj->rightTI || obj->leftTI)
		{
			CK_ShotHit(obj);
		}
	}
	else
	// correct for pole hole passage
	{
		ck_nextY = obj->currentAction->velY * SD_GetSpriteSync() * obj->yDirection;
		obj->posY += ck_nextY;
		obj->clipRects.unitY1 += ck_nextY;
		obj->clipRects.unitY2 += ck_nextY;
		obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
		obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);
	}
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenSpawnShot(CK_object *obj)
{
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenShoot1"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 256, obj->posY + 64, 2);
		}
		else
		{
			CK_SpawnShot(obj->posX - 128, obj->posY + 64, 6);
		}
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShoot2"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 256, obj->posY + 32, 2);
		}
		else
		{
			CK_SpawnShot(obj->posX, obj->posY + 32, 6);
		}
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShootDown2"))
	{
		CK_SpawnShot(obj->posX + 128, obj->posY + 288, 4);
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShootUp2"))
	{
		CK_SpawnShot(obj->posX + 80, obj->posY - 160, 0);
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenShootUp1"))
	{
		CK_SpawnShot(obj->posX + 80, obj->posY - 160, 0);
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShoot1"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 256, obj->posY + 64, 2);
		}
		else
		{
			CK_SpawnShot(obj->posX - 128, obj->posY + 64, 6);
		}
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShootUp1"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 96, obj->posY + 64, 0);
		}
		else
		{
			CK_SpawnShot(obj->posX + 192, obj->posY + 64, 0);
		}
	}
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShootDown1"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 96, obj->posY + 384, 4);
		}
		else
		{
			CK_SpawnShot(obj->posX + 192, obj->posY + 384, 4);
		}
	}
}

void CK_KeenSetupFunctions()
{
	CK_ACT_AddFunction("CK_KeenSlide", &CK_KeenSlide);
	CK_ACT_AddFunction("CK_KeenEnterDoor0", &CK_KeenEnterDoor0);
	CK_ACT_AddFunction("CK_KeenEnterDoor1", &CK_KeenEnterDoor1);
	CK_ACT_AddFunction("CK_KeenEnterDoor", &CK_KeenEnterDoor);
	CK_ACT_AddFunction("CK_KeenPlaceGem", &CK_KeenPlaceGem);
	CK_ACT_AddFunction("CK_KeenRunningThink", &CK_KeenRunningThink);
	CK_ACT_AddFunction("CK_KeenStandingThink", &CK_KeenStandingThink);
	CK_ACT_AddFunction("CK_HandleInputOnGround", &CK_HandleInputOnGround);
	CK_ACT_AddFunction("CK_KeenLookUpThink", &CK_KeenLookUpThink);
	CK_ACT_AddFunction("CK_KeenLookDownThink", &CK_KeenLookDownThink);
	CK_ACT_AddFunction("CK_KeenPressSwitchThink", &CK_KeenPressSwitchThink);
	CK_ACT_AddFunction("CK_KeenDrawFunc", &CK_KeenDrawFunc);
	CK_ACT_AddFunction("CK_KeenRunDrawFunc", &CK_KeenRunDrawFunc);
	CK_ACT_AddFunction("CK_KeenReadThink", &CK_KeenReadThink);
	CK_ACT_AddFunction("CK_KeenJumpThink", &CK_KeenJumpThink);
	CK_ACT_AddFunction("CK_KeenJumpDrawFunc", &CK_KeenJumpDrawFunc);
	CK_ACT_AddFunction("CK_KeenPogoThink", &CK_KeenPogoThink);
	CK_ACT_AddFunction("CK_KeenPogoBounceThink", &CK_KeenPogoBounceThink);
	CK_ACT_AddFunction("CK_KeenPogoDrawFunc", &CK_KeenPogoDrawFunc);
	CK_ACT_AddFunction("CK_KeenSpecialDrawFunc", &CK_KeenSpecialDrawFunc);
	CK_ACT_AddColFunction("CK_KeenSpecialColFunc", &CK_KeenSpecialColFunc);
	CK_ACT_AddFunction("CK_KeenHangThink", &CK_KeenHangThink);
	CK_ACT_AddFunction("CK_KeenPullThink1", &CK_KeenPullThink1);
	CK_ACT_AddFunction("CK_KeenPullThink2", &CK_KeenPullThink2);
	CK_ACT_AddFunction("CK_KeenPullThink3", &CK_KeenPullThink3);
	CK_ACT_AddFunction("CK_KeenPullThink4", &CK_KeenPullThink4);
	CK_ACT_AddFunction("CK_KeenPoleSitThink", &CK_KeenPoleSitThink);
	CK_ACT_AddFunction("CK_KeenPoleUpThink", &CK_KeenPoleUpThink);
	CK_ACT_AddFunction("CK_KeenPoleDownThink", &CK_KeenPoleDownThink);
	CK_ACT_AddFunction("CK_KeenJumpDownThink", &CK_KeenJumpDownThink);
	CK_ACT_AddFunction("CK_KeenPoleDownDrawFunc", &CK_KeenPoleDownDrawFunc);
	CK_ACT_AddFunction("CK_KeenSetClipped", &CK_KeenSetClipped);
	CK_ACT_AddColFunction("CK_KeenColFunc", &CK_KeenColFunc);
	CK_ACT_AddFunction("CK_KeenDeathThink", &CK_KeenDeathThink);
	CK_ACT_AddFunction("CK_KeenSpawnShot", &CK_KeenSpawnShot);
	CK_ACT_AddFunction("CK_ShotThink", &CK_ShotThink);
	CK_ACT_AddFunction("CK_ShotDrawFunc", &CK_ShotDrawFunc);
}
