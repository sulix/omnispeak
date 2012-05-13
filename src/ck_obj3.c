#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "id_rf.h"
#include "id_us.h"

#include <stdio.h>

// Korath Funcs

CK_action CK5_ACT_KorathWait;
void CK5_KorathDraw(CK_object *obj)
{
	if (obj->xDirection == 1 && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -1;
		obj->timeUntillThink = US_RndT()/32;
		CK_SetAction(obj, obj->currentAction);
	}
	else if (obj->xDirection == -1 && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = 1;
		obj->timeUntillThink = US_RndT()/32;
		CK_SetAction(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT()/32;
		obj->nextX = 0;
		
		CK_PhysUpdateNormalObj(obj);
		//CK_SetAction(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void CK5_KorathWalk(CK_object *obj)
{
	if (US_RndT() < 10) 
	{
		obj->nextX = 0;
		obj->xDirection = US_RndT()<128 ? 1: -1;
		obj->currentAction = &CK5_ACT_KorathWait;
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

// Korath Actions

CK_action CK5_ACT_KorathWalk2;
CK_action CK5_ACT_KorathWalk3;
CK_action CK5_ACT_KorathWalk4;

CK_action CK5_ACT_KorathWalk1 = {0x126, 0x12A, 0, 0, 1, 8, 128, 0, CK5_KorathWalk, CK5_KorathColFunc, CK5_KorathDraw, &CK5_ACT_KorathWalk2};
CK_action CK5_ACT_KorathWalk2 = {0x127, 0x12B, 0, 0, 1, 8, 128, 0, CK5_KorathWalk, CK5_KorathColFunc, CK5_KorathDraw, &CK5_ACT_KorathWalk3};
CK_action CK5_ACT_KorathWalk3 = {0x128, 0x12C, 0, 0, 1, 8, 128, 0, CK5_KorathWalk, CK5_KorathColFunc, CK5_KorathDraw, &CK5_ACT_KorathWalk4};
CK_action CK5_ACT_KorathWalk4 = {0x129, 0x12D, 0, 0, 1, 8, 128, 0, CK5_KorathWalk, CK5_KorathColFunc, CK5_KorathDraw, &CK5_ACT_KorathWalk1};

CK_action CK5_ACT_KorathWait = {0x12E, 0x12E, 0, 0, 1, 0x1E, 0, 0, 0, 0, CK5_KorathDraw, &CK5_ACT_KorathWalk1};


void CK5_SpawnKorath(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 23;
	obj->active = true;
	obj->posX = tileX << 8;
	obj->posY = (tileY << 8) - 128;
	obj->xDirection = US_RndT()<128 ? 1 : -1;	
	CK_SetAction(obj, &CK5_ACT_KorathWalk1);

	printf("Spawning Korath at %d,%d\n", tileX, tileY);
}

	
