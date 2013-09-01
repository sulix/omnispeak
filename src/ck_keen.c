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
#include "id_heads.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_ti.h"
#include "id_ca.h"

// For all the shitty debug stuff  I have.
#include <stdio.h>




void CK_SpawnKeen(int tileX, int tileY, int direction);
extern CK_object *ck_keenObj;

CK_keenState ck_keenState;

void CK_BasicDrawFunc1(CK_object *obj);

CK_action CK_ACT_itemNotify = {0, 0, 1, 0, 0, 40, 0, 8, 0, 0, CK_BasicDrawFunc1, 0};


void CK_KeenColFunc(CK_object *a, CK_object *b)
{
	if (b->type == 4)
	{
		b->type = 1;
		b->zLayer = 3;
		b->gfxChunk = b->user4;
		b->yDirection = -1;
		CK_SetAction(b, &CK_ACT_itemNotify);
	}
	else if (b->type == 6) //Platform
	{
		if (!ck_keenState.platform)
			CK_PhysPushY(a,b);
	}
}

int ck_KeenRunXVels[8] = {0, 0, 4, 4, 8, -4, -4, -8};

int ck_KeenPoleOffs[3] = {-8, 0, 8};


void CK_SpawnKeen(int tileX, int tileY, int direction)
{
	ck_keenObj->type = 0; //TODO: obj_keen
	ck_keenObj->active = OBJ_ALWAYS_ACTIVE; 
	ck_keenObj->visible = true;
	ck_keenObj->zLayer = 1;
	ck_keenObj->clipped = true;
	ck_keenObj->posX = (tileX << 8);
	ck_keenObj->posY = (tileY << 8) - 241;
	ck_keenObj->xDirection = direction;
	ck_keenObj->yDirection = 1;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK_ACT_keenStanding"));
}

static int16_t emptyTile = 0;

void CK_KeenGetTileItem(int tileX, int tileY, int itemNumber)
{
	RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);

	CK_object *notify = CK_GetNewObj(true);
	notify->type = 1;
	notify->zLayer = 3;
	notify->posX = tileX << 8;
	notify->posY = tileY << 8;
	notify->yDirection = -1;
	notify->user2 = 195 + itemNumber;
	notify->gfxChunk = notify->user2;
	CK_SetAction(notify, &CK_ACT_itemNotify);
	notify->clipped = false;
}

void CK_GetVitalin(int tileX, int tileY)
{
	CK_object *notify = CK_GetNewObj(true);
	notify->type = 1;
	notify->clipped = false;
	notify->zLayer = 3;
	notify->posX = tileX << 8;
	notify->posY = tileY << 8;
	
	CK_SetAction(notify, CK_GetActionByName("CK_ACT_VitalinNotify1"));
}

void CK_KeenGetTileVitalin(int tileX, int tileY)
{
	RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);

	CK_GetVitalin(tileX, tileY);
}

void CK_KeenCheckSpecialTileInfo(CK_object *obj)
{
	for (int y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; ++y)
	{
		for (int x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; ++x)
		{
			int specialTileInfo =  (TI_ForeMisc(CA_TileAtPos(x,y,1)) & 0x7F);
			switch (specialTileInfo)
			{
			case 0: break;
			case 4:
				CK_KeenGetTileVitalin(x,y);
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
				CK_KeenGetTileItem(x,y,specialTileInfo - 21);
				break;
			}
		}
	}
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
			obj->nextX = 0;
			obj->nextY = plat->deltaPosY;
			CK_PhysUpdateSimpleObj(obj);
			return;
		}
	}
	else
	{
		//Ride the platform
		obj->nextX = plat->deltaPosX;
		obj->nextY = plat->clipRects.unitY1 - obj->clipRects.unitY2 - 16;
		CK_PhysUpdateSimpleObj(obj);

		//TODO: Something relating to scrolling?
		

		// WTF?
		obj->posX |= plat->posX & 0x1F;

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
	if (CK_GetNumTotalTics() > ck_keenState.poleGrabTime && CK_GetNumTotalTics() - ck_keenState.poleGrabTime < 19)
		return false;

	ck_keenState.poleGrabTime = 0;

	int candidateTile = CA_TileAtPos(obj->clipRects.tileXmid, ((ck_inputFrame.yDirection==-1)?((obj->clipRects.unitY1+96)>>8):(obj->clipRects.tileY2+1)), 1);


	if ((TI_ForeMisc(candidateTile) & 0x7F) == 1)
	{
		obj->posX = 128 + ((obj->clipRects.tileXmid - 1) << 8);
		obj->nextX = 0;
		obj->nextY = 32 * ck_inputFrame.yDirection;
		obj->clipped = false;
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
		//NOTE: HAXXX! I just added this to get demos working.
		//obj->nextX = (obj->xDirection * obj->currentAction->velX * CK_GetTicksPerFrame())/4;
		return;
	}
	
	if (ck_inputFrame.yDirection == -1)
	{
		if (CK_KeenTryClimbPole(obj)) return;
	}
	else if (ck_inputFrame.yDirection == 1)
	{
		if (CK_KeenTryClimbPole(obj)) return;
	}

	obj->xDirection = ck_inputFrame.xDirection;

	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		//TODO: Remove this, it's there so we can always actually shoot!
		ck_gameState.numShots++;

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
		obj->velX = obj->xDirection * 16;
		obj->velY = -40;
		obj->nextX = obj->nextY = 0;
		ck_keenState.jumpTimer = 18;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		// Is this the mystical 'impossible pogo'?
		// k5disasm has a 'return' here, probably a mistake?
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		obj->velX = obj->xDirection * 16;
		obj->velY = -48;
		// Should this be nextY? Andy seems to think so, but lemm thinks that X is right...
		obj->nextX = 0;
		ck_keenState.jumpTimer = 24;
		return;
	}

	// Andy seems to think this is Y as well. Need to do some more disasm.
	// If this is an X vel, then surely we'd want to multiply it by the direction?
	obj->nextX += ck_KeenRunXVels[obj->topTI&7] * CK_GetTicksPerFrame();
}



void CK_HandleInputOnGround(CK_object *obj)
{
	if (ck_inputFrame.xDirection)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenRun1");
		CK_KeenRunningThink(obj);
		obj->nextX = (obj->xDirection * obj->currentAction->velX * CK_GetTicksPerFrame())/4;
		return;
	}

	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		ck_gameState.numShots++;
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
		obj->velX = 0;
		obj->velY = -40;
		obj->nextY = 0;
		ck_keenState.jumpTimer = 18;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		return;
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		obj->velX = 0;
		obj->velY = -48;
		obj->nextY = 0;
		ck_keenState.jumpTimer = 24;
		return;
	}

	if (ck_inputFrame.yDirection == -1)
	{
		if (CK_KeenTryClimbPole(obj)) return;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenLookUp1");
	}	
	else if (ck_inputFrame.yDirection == 1)
	{
		// Try poles.
		if (CK_KeenTryClimbPole(obj)) return;
		// Keen looks down.
		obj->currentAction = CK_GetActionByName("CK_ACT_keenLookDown1");
		return;
	}
		
}

void CK_KeenStandingThink(CK_object *obj)
{
	if (ck_inputFrame.xDirection || ck_inputFrame.yDirection || ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed || ck_keenState.shootIsPressed )
	{
		obj->user1 = obj->user2 = 0;	//Idle Time + Idle State
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		CK_HandleInputOnGround(obj);

		return;
	}

	//If not on platform
	if (!obj->topTI & ~7 == 0x19)
		obj->user1 += CK_GetTicksPerFrame();

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
		obj->currentAction = CK_GetActionByName("CK_ACT_keenBored1");
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
	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed && (obj->topTI&7) == 1)
	{
		ck_keenState.jumpWasPressed = true;

		//If the tiles below the player are blocking on any side but the top, they cannot be jumped through
		int tile1 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
		int tile2 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2+1, 1);
		
		if (TI_ForeLeft(tile1) || TI_ForeBottom(tile1) || TI_ForeRight(tile1))
			return;

		if (TI_ForeLeft(tile2) || TI_ForeBottom(tile2) || TI_ForeRight(tile2))
			return;
		#define max(a,b) ((a>b)?a:b)

		int deltay = max(CK_GetTicksPerFrame(),4) << 4;
	
		//Moving platforms
		if (ck_keenState.platform)
			deltay += ck_keenState.platform->deltaPosY;

		ck_keenState.platform = 0;


		obj->clipRects.unitY2 += deltay;
		obj->posY += deltay;
		obj->nextX = 0;
		obj->nextY = 0;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		obj->velX = obj->velY = 0;
		//TODO: Sound
		return;
	}
	

	if (ck_inputFrame.yDirection != 1 || ck_inputFrame.xDirection != 0 || (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
		|| (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed))
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStanding");
		return;
	}
}	

void CK_KeenDrawFunc(CK_object *obj)
{
	if (!obj->topTI)
	{
		obj->velX = obj->xDirection * 8;
		obj->velY = 0;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
	}
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenRunDrawFunc(CK_object *obj)
{

	if (!obj->topTI)
	{
		obj->velX = obj->xDirection * 8;
		obj->velY = 0;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenFall1"));
		ck_keenState.jumpTimer = 0;
	}

	if ((obj->rightTI && obj->xDirection == -1) || (obj->leftTI && obj->xDirection == 1))
	{
		obj->timeUntillThink = 0;
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
		if (ck_keenState.jumpTimer <= CK_GetTicksPerFrame())
		{
			obj->nextY = obj->velY * ck_keenState.jumpTimer;
			ck_keenState.jumpTimer = 0;
		}
		else
		{
			obj->nextY = obj->velY * CK_GetTicksPerFrame();
			if (!ck_gameState.jumpCheat)
			{
				ck_keenState.jumpTimer -= CK_GetTicksPerFrame();
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
		else	// Normal or Hard
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
		CK_PhysAccelHorz(obj, ck_inputFrame.xDirection*2, 24);
	}
	else CK_PhysDampHorz(obj);

	//Pole
	if (obj->bottomTI == 17)
	{
		obj->nextX = 0;
	}

	//Shooting
	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		ck_gameState.numShots = 5;
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
	if (obj->leftTI && obj->xDirection == 1)
	{
		obj->velX = 0;
	}

	// Did we hit our head on the ceiling?
	if (obj->bottomTI && !ck_gameState.jumpCheat)
	{
		//TODO: Something to do with poles (push keen into the centre)
		if (obj->bottomTI == 17)	//Pole
		{
			obj->posY -= 32;
			obj->clipRects.unitY1 -= 32;
			obj->velX = 0;
			obj->posX = (obj->clipRects.tileXmid << 8) - 32;
		}
		else
		{
			//TODO: sounds
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

	// Have we landed?
	if (obj->topTI)
	{
		obj->deltaPosY = 0;
		//Check if deadly.
		if (obj->topTI & ~7 == 8)
		{
			//TODO: Kill Keen
		}
		else
		{
			//TODO: Check if fuse.
			if (obj->topTI != 0x19 || !ck_keenState.jumpTimer) // Or standing on a platform.
			{
				obj->user1 = obj->user2 = 0;	// Being on the ground is boring.
	
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
		int temp10 = (temp8 >> 8) - 1 ;

		// If we're moving past a tile boundary.
		if (temp6 < temp8 && obj->clipRects.unitY1 >= temp8)
		{
			// Moving left...
			if (ck_inputFrame.xDirection == -1)
			{
				int tileX = obj->clipRects.tileX1 - ((obj->rightTI)?1:0);
				int tileY = temp10;
				//VL_ScreenRect((tileX << 4) - (rf_scrollXUnit >> 4), (tileY << 4) - (rf_scrollYUnit >> 4), 16, 16, 1);
				int upperTile = CA_TileAtPos(tileX, tileY, 1);
				int lowerTile = CA_TileAtPos(tileX, tileY+1, 1);
				if ( (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile)) &&
					TI_ForeRight(lowerTile) && TI_ForeTop(lowerTile))
				{
					obj->xDirection = -1;
					obj->clipped = false;
					obj->posX = (obj->posX & 0xFF00) + 128;
					obj->posY = (temp8 - 64);
					obj->velY = obj->deltaPosY = 0;
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenHang1"));
				}
			}
			else if (ck_inputFrame.xDirection == 1)
			{
				int tileX = obj->clipRects.tileX2 + ((obj->leftTI)?1:0);
				int tileY = temp10;
				int upperTile = CA_TileAtPos(tileX, tileY, 1);
				int lowerTile = CA_TileAtPos(tileX, tileY+1, 1);

				if (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile) &&
					TI_ForeLeft(lowerTile) && TI_ForeTop(lowerTile))
				{
					obj->xDirection = 1;
					obj->clipped = false;
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
	obj->nextY = 6 * obj->velY;
	ck_keenState.jumpTimer = 24;
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
		if (ck_keenState.jumpIsPressed || ck_keenState.jumpTimer <= 9) CK_PhysGravityLow(obj);
		else CK_PhysGravityHigh(obj);

		if (ck_keenState.jumpTimer <= CK_GetTicksPerFrame()) ck_keenState.jumpTimer = 0;
		else ck_keenState.jumpTimer -= CK_GetTicksPerFrame();

		if (!ck_keenState.jumpTimer && obj->currentAction->next) obj->currentAction = obj->currentAction->next;
	}
	if (ck_inputFrame.xDirection)
	{
		if (!obj->velX)
			obj->xDirection = ck_inputFrame.xDirection;
		CK_PhysAccelHorz(obj, ck_inputFrame.xDirection, 24);
	}
	else
	{
		obj->nextX += obj->velX * CK_GetTicksPerFrame();
		if (obj->velX < 0) obj->xDirection = -1;
		else if (obj->velX > 0) obj->xDirection = 1;
	}

	// Stop for poles?
	if (obj->bottomTI == 17)
	{
		obj->nextX = 0;
		obj->velX = 0;
	}

	//Shooting
	if (ck_keenState.shootIsPressed && !ck_keenState.shootWasPressed)
	{
		ck_keenState.shootWasPressed = true;
		ck_gameState.numShots = 5;
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
	}
	
	//Stop pogoing if Alt pressed
	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
	}
}

void CK_KeenPogoDrawFunc(CK_object *obj)
{
	if (obj->rightTI && obj->xDirection == -1)
		obj->velX = 0;
	if (obj->leftTI && obj->xDirection == 1)
		obj->velX = 0;

	if (obj->bottomTI)
	{
		
		if (obj->bottomTI == 17)	//Pole
		{
			obj->posY -= 32;
			obj->clipRects.unitY1 -= 32;
			obj->velX = 0;
			obj->posX = (obj->clipRects.tileXmid << 8) - 32;
		}
		else if (!ck_gameState.jumpCheat)
		{
			//TODO: PlaySound

			if (obj->bottomTI > 1)
			{
				obj->velY += 16;
				if (obj->velY < 0) obj->velY = 0;
			}
			else obj->velY = 0;
		
			ck_keenState.jumpTimer = 0;
		}
	}

	// Houston, we've landed!
	if (obj->topTI)
	{
		obj->deltaPosY = 0;
		//TODO: Deadly surfaces and fuse breakage.
		if (obj->topTI != 0x19 || ck_keenState.jumpTimer == 0)
		{
			obj->velY = -48;
			ck_keenState.jumpTimer = 24;
			CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenPogo2"));
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenSpecialColFunc(CK_object *obj, CK_object *other)
{
	//TODO: collision with types 14,23?
	if (other->type == 6)
	{
		obj->clipped = true;
		CK_PhysUpdateSimpleObj(obj);
		ck_keenState.jumpTimer = 0;
		obj->deltaPosX = 0;
		obj->deltaPosY = 0;
		CK_PhysPushY(obj,other);
		return;
	}

}

void CK_KeenSpecialDrawFunc(CK_object *obj)
{
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenHangThink(CK_object *obj)
{
	if (ck_inputFrame.yDirection == -1 || ck_inputFrame.xDirection == obj->xDirection)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPull1");

		obj->clipped = false;

		if(obj->xDirection == 1)
		{
			obj->nextY = -256;
		}
		else
		{
			obj->nextY = -128;
		}
		//TODO: Set keen->zlayer 3

		//if (obj->xDirection == 1)
		//{
			

	}
	else if (ck_inputFrame.yDirection == 1 || (ck_inputFrame.xDirection && ck_inputFrame.xDirection != obj->xDirection))
	{
		// Drop down.
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		obj->clipped = true;
	}
}

void CK_KeenPullThink1(CK_object *obj)
{
	if (obj->xDirection == 1)
		obj->nextX = 128;
	else
		obj->nextY = -128;
}

void CK_KeenPullThink2(CK_object *obj)
{
	obj->nextX = obj->xDirection * 128;
	obj->nextY = -128;
}

void CK_KeenPullThink3(CK_object *obj)
{
	obj->nextY = -128;
}

void CK_KeenPullThink4(CK_object *obj)
{
	obj->clipped = true;
	obj->zLayer = 1;
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
		//TODO: Play A sound!
		obj->velX = ck_KeenPoleOffs[ck_inputFrame.xDirection+1];
		obj->velY = -20;
		obj->clipped = true;
		ck_keenState.jumpTimer = 10;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
		obj->yDirection = 1;
		ck_keenState.poleGrabTime = CK_GetNumTotalTics();
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

	// Whenn keen is at ground level, allow dismounting using left/right.
	if (ck_inputFrame.xDirection)
	{
		int groundTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2+1, 1);
		if (TI_ForeTop(groundTile))
		{
			obj->velX = 0;
			obj->velY = 0;
			obj->clipped = true;
			ck_keenState.jumpTimer = 0;
			obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
			obj->yDirection = 1;
			//TODO: Sound
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
		obj->nextY = 0;
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

	int tileUnderneath = CA_TileAtPos(obj->clipRects.tileXmid, (obj->clipRects.unitY2-64) >> 8, 1);

	if ((TI_ForeMisc(tileUnderneath) & 127) != 1)
	{
		// We're no longer on a pole.
		//TODO: Play sound 20
		obj->currentAction = CK_GetActionByName("CK_ACT_keenFall1");
		ck_keenState.jumpTimer = 0;
		obj->velX = ck_KeenPoleOffs[ck_inputFrame.xDirection + 1];
		obj->velY = 0;
		obj->clipped = true;
		obj->clipRects.tileY2 -= 1;	//WTF?
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
		obj->clipped = true;
		CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenLookDown1"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// Shooting

void CK_SpawnShot(int x, int y, int direction)
{
	if (!ck_gameState.numShots)
	{
		//TODO: Play out-of-ammo sound
		return;
	}

	ck_gameState.numShots--;

	CK_object *shot = CK_GetNewObj(true);
	shot->posX = x;
	shot->posY = y;
	shot->zLayer = 0;
	shot->type = 4; // TODO: obj_stunner
	shot->active = true;
	
	switch(direction)
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
		Quit("SpawnShot: bad dir!");
	}

	CK_SetAction(shot, CK_GetActionByName("CK_ACT_keenShot1"));
}

void CK_ShotHit(CK_object *obj)
{
	//TODO: Implement obj_ classes.
	CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenShotHit1"));
}

void CK_ShotColFunc(CK_object *obj, CK_object *other)
{
	//TODO: Maybe do something.
}

void CK_ShotThink(CK_object *obj)
{
	//TODO: Kill things which are offscreen.
}

void CK_ShotDrawFunc(CK_object *obj)
{
	//TODO: Force the bullet through pole-holes.
	
	if (obj->topTI || obj->bottomTI || obj->leftTI || obj->rightTI)
	{
		CK_ShotHit(obj);
	}
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_SimpleDrawFunc(CK_object *obj)
{
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
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShoot2"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 256, obj->posY + 32, 2);
		}
		else
		{
			//TODO: There's no '-128' in the keen5 binary.
			//However, there clearly should be one, and for whatever
			//reason, it's not working without it. (Shot is invisible?)
			CK_SpawnShot(obj->posX - 128, obj->posY + 32, 6);
		}
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShootDown2"))
	{
		CK_SpawnShot(obj->posX + 128, obj->posY + 288, 4);
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenJumpShootUp2"))
	{
		CK_SpawnShot(obj->posX + 80, obj->posY - 160, 0);
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenShootUp1"))
	{
		CK_SpawnShot(obj->posX + 80, obj->posY - 160, 0);
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShoot1"))
	{
		if (obj->xDirection == 1)
		{
			CK_SpawnShot(obj->posX + 256, obj->posY + 64, 2);
		}
		else
		{
			CK_SpawnShot(obj->posX + 128, obj->posY + 64, 6);
		}
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShootUp1"))
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
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenPoleShootDown1"))
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

void CK_KeenFall(CK_object *obj)
{
	CK_PhysGravityHigh(obj);
	obj->nextX = obj->velX * CK_GetTicksPerFrame();
}

void CK_KeenSetupFunctions()
{
	CK_ACT_AddFunction("CK_KeenRunningThink",&CK_KeenRunningThink);
	CK_ACT_AddFunction("CK_KeenStandingThink",&CK_KeenStandingThink);
	CK_ACT_AddFunction("CK_KeenLookUpThink",&CK_KeenLookUpThink);
	CK_ACT_AddFunction("CK_KeenLookDownThink",&CK_KeenLookDownThink);
	CK_ACT_AddFunction("CK_KeenDrawFunc",&CK_KeenDrawFunc);
	CK_ACT_AddFunction("CK_KeenRunDrawFunc",&CK_KeenRunDrawFunc);
	CK_ACT_AddFunction("CK_KeenReadThink",&CK_KeenReadThink);
	CK_ACT_AddFunction("CK_KeenJumpThink",&CK_KeenJumpThink);
	CK_ACT_AddFunction("CK_KeenJumpDrawFunc",&CK_KeenJumpDrawFunc);
	CK_ACT_AddFunction("CK_KeenPogoThink",&CK_KeenPogoThink);
	CK_ACT_AddFunction("CK_KeenPogoBounceThink",&CK_KeenPogoBounceThink);
	CK_ACT_AddFunction("CK_KeenPogoDrawFunc",&CK_KeenPogoDrawFunc);
	CK_ACT_AddFunction("CK_KeenSpecialDrawFunc",&CK_KeenSpecialDrawFunc);
	CK_ACT_AddColFunction("CK_KeenSpecialColFunc",&CK_KeenSpecialColFunc);
	CK_ACT_AddFunction("CK_KeenHangThink",&CK_KeenHangThink);
	CK_ACT_AddFunction("CK_KeenPullThink1",&CK_KeenPullThink1);
	CK_ACT_AddFunction("CK_KeenPullThink2",&CK_KeenPullThink2);
	CK_ACT_AddFunction("CK_KeenPullThink3",&CK_KeenPullThink3);
	CK_ACT_AddFunction("CK_KeenPullThink4",&CK_KeenPullThink4);
	CK_ACT_AddFunction("CK_KeenPoleSitThink",&CK_KeenPoleSitThink);
	CK_ACT_AddFunction("CK_KeenPoleUpThink",&CK_KeenPoleUpThink);
	CK_ACT_AddFunction("CK_KeenPoleDownThink",&CK_KeenPoleDownThink);
	CK_ACT_AddFunction("CK_KeenPoleDownDrawFunc",&CK_KeenPoleDownDrawFunc);
	CK_ACT_AddColFunction("CK_KeenColFunc",&CK_KeenColFunc);

	CK_ACT_AddFunction("CK_KeenSpawnShot", &CK_KeenSpawnShot);
	CK_ACT_AddFunction("CK_ShotThink", &CK_ShotThink);
	CK_ACT_AddFunction("CK_ShotDrawFunc", &CK_ShotDrawFunc);
	CK_ACT_AddColFunction("CK_ShotColFunc", &CK_ShotColFunc);
	CK_ACT_AddFunction("CK_SimpleDrawFunc",&CK_SimpleDrawFunc);
	CK_ACT_AddFunction("CK_KeenFall",&CK_KeenFall);
}
