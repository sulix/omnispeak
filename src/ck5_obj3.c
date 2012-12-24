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
#include "id_us.h"

#include <stdio.h>

// Spirogrip Funcs

void CK5_SpawnSpirogrip(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);
	
	obj->type = 13; // Spirogrip
	obj->active = true;
	obj->posX = (tileX << 8);
	obj->posY = (tileY << 8) - 256;

	obj->xDirection = -1; // Left
	obj->yDirection = 1; //Down

	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SpirogripSpin1"));
}

void CK5_SpirogripSpin(CK_object *obj)
{
	// If we're bored of spinning...
	if (US_RndT() > 20) return;

	// TODO: Play sound (0x3D)
	
	// and we're in the right direction, fly!
	if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin1"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyUp");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin3"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyRight");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin5"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyDown");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin7"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyLeft");

}

void CK5_SpirogripFlyDraw(CK_object *obj)
{
	// Draw the sprite
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);

	// Check if we've collided with a tile
	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		obj->currentAction = obj->currentAction->next;
		//TODO: Play sound (0x1B)
	}
}
// Korath Funcs

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

/*
 * Add the Obj3 functions to the function db
 */
void CK5_Obj3_SetupFunctions()
{
	// Spirogrip
	CK_ACT_AddFunction("CK5_SpirogripSpin", &CK5_SpirogripSpin);
	CK_ACT_AddFunction("CK5_SpirogripFlyDraw", &CK5_SpirogripFlyDraw);

	// Korath
	CK_ACT_AddFunction("CK5_KorathDraw",&CK5_KorathDraw);
	CK_ACT_AddFunction("CK5_KorathWalk",&CK5_KorathWalk);
	CK_ACT_AddColFunction("CK5_KorathColFunc",&CK5_KorathColFunc);
}

	
