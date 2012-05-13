#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "id_rf.h"

#include <stdio.h>

void CK_BasicDrawFunc1(CK_object *obj);
void CK5_TurretShoot(CK_object *obj);
void CK5_Glide(CK_object *obj);
void CK5_TurretShotCol(CK_object *me, CK_object *other);
void CK5_TurretShotDraw(CK_object *obj);


CK_action CK5_ACT_turretShot2, CK5_ACT_turretShot3, CK5_ACT_turretShot4;
CK_action CK5_ACT_turretShot1 = {0xEC, 0xEC, AT_UnscaledFrame, 0, 0, 8, 0, 0, CK5_Glide, CK5_TurretShotCol, CK5_TurretShotDraw, &CK5_ACT_turretShot2};
CK_action CK5_ACT_turretShot2 = {0xED, 0xED, AT_UnscaledFrame, 0, 0, 8, 0, 0, CK5_Glide, CK5_TurretShotCol, CK5_TurretShotDraw, &CK5_ACT_turretShot3};
CK_action CK5_ACT_turretShot3 = {0xEE, 0xEE, AT_UnscaledFrame, 0, 0, 8, 0, 0, CK5_Glide, CK5_TurretShotCol, CK5_TurretShotDraw, &CK5_ACT_turretShot4};
CK_action CK5_ACT_turretShot4 = {0xEF, 0xEF, AT_UnscaledFrame, 0, 0, 8, 0, 0, CK5_Glide, CK5_TurretShotCol, CK5_TurretShotDraw, &CK5_ACT_turretShot1};


CK_action CK5_ACT_turretShotHit2;
CK_action CK5_ACT_turretShotHit1 = {0xF0, 0xF0, AT_UnscaledOnce, 0, 0, 10, 0, 0, 0, 0, &CK_BasicDrawFunc1, &CK5_ACT_turretShotHit2};
CK_action CK5_ACT_turretShotHit2 = {0xF1, 0xF1, AT_UnscaledOnce, 0, 0, 10, 0, 0, 0, 0, &CK_BasicDrawFunc1, 0};

CK_action CK5_ACT_turretShoot;
CK_action CK5_ACT_turretWait = {0, 0, 0, 0, 0, 0x78, 0, 0, 0, 0, 0, &CK5_ACT_turretShoot};
CK_action CK5_ACT_turretShoot = {0, 0, 0, 1, 0, 1, 0, 0, &CK5_TurretShoot, 0, 0, &CK5_ACT_turretWait};

void CK5_TurretSpawn(int tileX, int tileY, int direction)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 0;
	obj->gfxChunk = 0;
	obj->active = true;
	obj->clipRects.tileX1 = obj->clipRects.tileX2 = tileX;
	obj->clipRects.tileY1 = obj->clipRects.tileY2 = tileY;

	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->clipRects.unitX1 = obj->clipRects.unitX2 = tileX << 8;
	obj->clipRects.unitX2 = obj->clipRects.unitY2 = tileY << 8;
	
	obj->user1 = direction;

	CK_SetAction(obj, &CK5_ACT_turretWait);
}

void CK5_TurretShoot(CK_object *obj)
{
	CK_object *shot = CK_GetNewObj(true);

	shot->type = 0;	//TurretShot
	shot->active = true;	//3
	//shot->clipped = true;
	shot->posX = obj->posX;
	shot->posY = obj->posY;

	printf("Shooting!\n");
	switch(obj->user1)
	{
	case 0:
		shot->velX = 0; shot->velY = -64; break;
	case 1:
		shot->velX = 64; shot->velY = 0; break;
	case 2:
		shot->velX = 0; shot->velY = 64; break;
	case 3:
		shot->velX = -64; shot->velY = 0; break;
	}

	CK_SetAction(shot,&CK5_ACT_turretShot1);

	//CK_SetAction(obj, &CK5_ACT_turretWait);
}

void CK5_Glide(CK_object *obj)
{
	obj->nextX = obj->velX;
	obj->nextY = obj->velY;
}

void CK5_TurretShotCol(CK_object *me, CK_object *other)
{
	//TODO: Kill Keen
}

void CK5_TurretShotDraw(CK_object *obj)
{
	if (obj->topTI || obj->bottomTI || obj->leftTI || obj->rightTI)
	{
		printf("Shot Hit\n");
		//obj->clipped=false;
		CK_SetAction2(obj, &CK5_ACT_turretShotHit1);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->currentAction->chunkLeft,false, 0);

}


