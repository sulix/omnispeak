#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck_act.h"
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
		obj->currentAction = CK_GetActionByName("CK5_ACT_KorathWait");
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

/*
 * Add the Obj3 functions to the function db
 */
void CK5_Obj3_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_KorathDraw",&CK5_KorathDraw);
	CK_ACT_AddFunction("CK5_KorathWalk",&CK5_KorathWalk);
	CK_ACT_AddColFunction("CK5_KorathColFunc",&CK5_KorathColFunc);
}

// Korath Actions


void CK5_SpawnKorath(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 23;
	obj->active = true;
	obj->posX = tileX << 8;
	obj->posY = (tileY << 8) - 128;
	obj->xDirection = US_RndT()<128 ? 1 : -1;	
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_KorathWalk1"));

	printf("Spawning Korath at %d,%d\n", tileX, tileY);
}

	
