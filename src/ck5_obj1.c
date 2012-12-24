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
#include "id_rf.h"

#include <stdio.h>

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

	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_turretWait"));
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

	CK_SetAction(shot, CK_GetActionByName("CK5_ACT_turretShot1"));

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
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_turretShotHit1"));
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->currentAction->chunkLeft,false, 0);

}

/*
 * Setup all of the functions in this file.
 */
void CK5_Obj1_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_TurretShoot",&CK5_TurretShoot);
	CK_ACT_AddFunction("CK5_Glide",&CK5_Glide);
	CK_ACT_AddColFunction("CK5_TurretShotCol",&CK5_TurretShotCol);
	CK_ACT_AddFunction("CK5_TurretShotDraw",&CK5_TurretShotDraw);
}
