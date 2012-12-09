#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck_act.h"
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
	ck_keenObj->active = true; // always
	ck_keenObj->visible = true;
	ck_keenObj->zLayer = 1;
	ck_keenObj->clipped = true;
	ck_keenObj->posX = (tileX << 8);
	ck_keenObj->posY = (tileY << 8) - 241;
	ck_keenObj->xDirection = direction;
	ck_keenObj->yDirection = 1;
	printf ("Keen Spawning!\n");
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
	//TODO: Something strange here? Ticks?
	if (CK_GetNumTotalTics() > ck_keenState.poleGrabTime && CK_GetNumTotalTics() - ck_keenState.poleGrabTime < 19)
		return false;

	ck_keenState.poleGrabTime = 0;

	printf ("Trying to climb  a pole\n");

	int candidateTile = CA_TileAtPos(obj->clipRects.tileXmid, ((ck_inputFrame.yDirection==-1)?((obj->clipRects.unitY1+96)>>8):(obj->clipRects.tileY2+1)), 1);

	printf("tile: %d :: %d\n", candidateTile, TI_ForeMisc(candidateTile));	

	if ((TI_ForeMisc(candidateTile) & 0x7F) == 1)
	{
		printf("Climbing a pole\n");
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

	/*if (IN_GetKeyState(IN_SC_LeftArrow)) obj->xDirection = -1;
	else obj->xDirection = 1;*/
	obj->xDirection = ck_inputFrame.xDirection;

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		obj->velX = obj->xDirection * 16;
		obj->velY = -40;
		obj->nextX = obj->nextY = 0;
		ck_keenState.jumpTimer = 18;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenJump1");
	}

	if (ck_keenState.pogoIsPressed && !ck_keenState.pogoWasPressed)
	{
		ck_keenState.pogoWasPressed = true;
		obj->velX = obj->xDirection * 16;
		obj->velY = -48;
		obj->nextX = 0;
		ck_keenState.jumpTimer = 24;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		return;
	}

	obj->nextX += ck_KeenRunXVels[obj->topTI&7] * CK_GetTicksPerFrame();
}



void CK_HandleInputOnGround(CK_object *obj)
{
	if (ck_inputFrame.xDirection)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenRun1");
		CK_KeenRunningThink(obj);
		obj->nextX = obj->xDirection * obj->currentAction->velX * (CK_GetTicksPerFrame())/4;
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
		obj->velX = 0;
		obj->velY = -48;
		obj->currentAction = CK_GetActionByName("CK_ACT_keenPogo1");
		obj->nextY = 0;
		ck_keenState.jumpTimer = 24;
		return;
	}

	if (ck_inputFrame.yDirection == -1)
	{
		if (CK_KeenTryClimbPole(obj)) return;
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
	if (ck_inputFrame.xDirection || ck_inputFrame.yDirection || ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed)
	{
		obj->user1 = obj->user2 = 0;	//Idle Time + Idle State
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

void CK_KeenLookDownThink(CK_object *obj)
{
	//Try to jump down
	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed && (obj->topTI&7) == 1)
	{
		ck_keenState.jumpWasPressed = true;

		printf("Tryin' to jump down\n");
		//If the tiles below the player are blocking on any side but the top, they cannot be jumped through
		int tile1 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
		int tile2 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2+1, 1);
		
		if (TI_ForeLeft(tile1) || TI_ForeBottom(tile1) || TI_ForeRight(tile1))
			return;

		if (TI_ForeLeft(tile2) || TI_ForeBottom(tile2) || TI_ForeRight(tile2))
			return;
		#define max(a,b) ((a>b)?a:b)

		int deltay = max(CK_GetTicksPerFrame(),4) << 4;
		printf("dy: %d\n",deltay);
	
		//TODO: Moving platforms

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
	if (IN_GetKeyState(IN_SC_LeftArrow) || IN_GetKeyState(IN_SC_RightArrow))
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_keenStowBook1");
		obj->user1 = obj->user2 = 0;
	}
}

void CK_KeenJumpThink(CK_object *obj)
{
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
			ck_keenState.jumpTimer -= CK_GetTicksPerFrame();
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
		CK_PhysAccelHorz(obj, obj->xDirection*2, 24);
	}
	else CK_PhysDampHorz(obj);

	//Pole
	if (obj->bottomTI == 17)
	{
		obj->nextX = 0;
	}

	//TODO: Shooting

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
	if (obj->bottomTI)
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
			if (!ck_keenState.jumpTimer) // Or standing on a platform.
			{
				obj->user1 = obj->user2 = 0;	// Being on the ground is boring.
	
				//TODO: Finish these
				if (IN_GetKeyState(IN_SC_RightArrow) || IN_GetKeyState(IN_SC_LeftArrow))
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
		int temp6 = obj->clipRects.unitY1 - obj->deltaPosY;
		int temp8 = ((obj->clipRects.unitY1 - 64) & 0xFF00) + 64;
		int temp10 = (temp8 >> 8) -1;

		if (temp6 < temp8 && obj->clipRects.unitY1 >= temp8)
		{
			printf("Preparing to hang!\n");
			if (ck_inputFrame.xDirection == -1)
			{
				int tileX = obj->clipRects.tileX1 - ((obj->rightTI)?1:0);
				int tileY = temp10;
				int upperTile = CA_TileAtPos(tileX, tileY, 1);
				int lowerTile = CA_TileAtPos(tileX, tileY+1, 1);
				printf("[%d,%d]: RightTI: %d, UpperTile = %d [%d], LowerTile = %d, [%d]\n",tileX, tileY, obj->rightTI, upperTile, TI_ForeRight(upperTile), lowerTile, TI_ForeRight(lowerTile));
				if ( (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile)) &&
					TI_ForeRight(lowerTile) && TI_ForeTop(lowerTile))
				{
					obj->xDirection = -1;
					obj->clipped = false;
					obj->posX = (obj->posX & 0xFF00) + 128;
					obj->posY = (temp8 - 64);
					obj->velY = obj->deltaPosY = 0;
					CK_SetAction2(obj, CK_GetActionByName("CK_ACT_keenHang1"));
					printf("Hung left!\n");
				} else printf("Couldn't hang left!\n");
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

	//TODO: Shooting
	
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
		else
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
		if (ck_keenState.jumpTimer == 0)
		{
			obj->velY = -48;
			ck_keenState.jumpTimer = 24;
			CK_SetAction(obj, CK_GetActionByName("CK_ACT_keenPogo2"));
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenSpecialColFunc(CK_object *obj, CK_object *other)
{
	//TODO: collision with types 14,23,13?
}

void CK_KeenSpecialDrawFunc(CK_object *obj)
{
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK_KeenHangThink(CK_object *obj)
{
	printf("Just hangin'...\n");
	if (ck_inputFrame.yDirection == -1 || ck_inputFrame.xDirection == obj->xDirection)
	{
		printf("Goin' up!\n");
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
		printf("Goin' Down!\n");
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
	printf("PullThink2\n");
	obj->nextX = obj->xDirection * 128;
	obj->nextY = -128;
}

void CK_KeenPullThink3(CK_object *obj)
{
	printf("PullThink3\n");
	obj->nextY = -128;
}

void CK_KeenPullThink4(CK_object *obj)
{
	printf("PullThink4\n");
	obj->nextY = 128;
	obj->clipped = true;
	obj->zLayer = 1;
}

void CK_KeenPoleHandleInput(CK_object *obj)
{
	if (ck_inputFrame.xDirection)
		obj->xDirection = ck_inputFrame.xDirection;

	//TODO: Shooting things. *ZAP!*
	
	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = false;
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
		printf("Leaving pole (tileU = %d : %d)\n",tileUnderneath, TI_ForeMisc(tileUnderneath));
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

void CK_KeenSetupFunctions()
{
	CK_ACT_AddFunction("CK_KeenRunningThink",&CK_KeenRunningThink);
	CK_ACT_AddFunction("CK_KeenStandingThink",&CK_KeenStandingThink);
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
}
