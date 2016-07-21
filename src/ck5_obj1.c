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
#include "id_ca.h"
#include "id_rf.h"

#include <stdio.h>


/*
 * Spawn a "Sneak Plat".
 */
void CK5_SneakPlatSpawn(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ALWAYS_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->xDirection = 0;
	obj->yDirection = 1;
	obj->clipped = CLIP_not;

	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_sneakPlatWait"));
	CA_CacheGrChunk(obj->gfxChunk);
}

void CK5_SneakPlatThink(CK_object *obj)
{
	if (ck_keenObj->currentAction == CK_GetActionByName("CK_ACT_keenJump1"))
	{
		if (ck_keenObj->xDirection == 1)
		{
			int dist = obj->clipRects.unitX1 - ck_keenObj->clipRects.unitX2;
			if (dist > 0x400 || dist < 0) return;
		}
		else
		{
			int dist = ck_keenObj->clipRects.unitX1 - obj->clipRects.unitX2;
			if (dist > 0x400 || dist < 0) return;
		}

		int vertDist = ck_keenObj->posY - obj->posY;
		if (vertDist < -0x600 || vertDist > 0x600) return;

		obj->xDirection = ck_keenObj->xDirection;
		obj->currentAction = CK_GetActionByName("CK5_ACT_sneakPlatSneak");
	}
}

/*
 * Spawn a GoPlat
 */
void CK5_GoPlatSpawn(int tileX, int tileY, int direction, bool purple)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ALWAYS_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->xDirection = 0;
	obj->yDirection = 0;
	obj->clipped = CLIP_not;

	if (purple)
	{
		obj->posX += 0x40;
		obj->posY += 0x40;
		CK_SetAction(obj, CK_GetActionByName("CK5_ACT_purpleGoPlat"));
	}
	else
	{
		CK_SetAction(obj, CK_GetActionByName("CK5_ACT_redGoPlat"));
	}


	int mapW = CA_MapHeaders[ca_mapOn]->width;
	//int mapH = CA_MapHeaders[ca_mapOn]->height;
	CA_mapPlanes[2][tileY * mapW + tileX] = direction + 0x5B;

	obj->user1 = direction;
	obj->user2 = 256;

}

static int ck5_infoplaneArrowsX[8] ={0, 1, 0, -1, 1, 1, -1, -1};
static int ck5_infoplaneArrowsY[8] ={-1, 0, 1, 0, -1, 1, 1, -1};

void CK5_RedGoPlatThink(CK_object *obj)
{

	if (ck_nextX || ck_nextY) return;

	int16_t delta = SD_GetSpriteSync()*12;

	// Will we reach a new tile?
	if (obj->user2 > delta)
	{
		// No... keep moving in the same direction.
		obj->user2 -= delta;

		int dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		int dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
	else
	{
		// Move to next tile.
		int dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += obj->user2;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= obj->user2;
		}

		int dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += obj->user2;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= obj->user2;
		}

		int tileX = (obj->posX + ck_nextX) >> 8;
		int tileY = (obj->posY + ck_nextY) >> 8;

		obj->user1 = CA_TileAtPos(tileX, tileY, 2) - 0x5B;

		if ((obj->user1 < 0) || (obj->user1 > 8))
		{
			Quit("Goplat moved to a bad spot.");
		}

		delta -= obj->user2;
		obj->user2 = 256 - delta;

		// Move in the new direction.
		dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
}

void CK5_PurpleGoPlatThink(CK_object *obj)
{

	if (ck_nextX || ck_nextY) return;

	int16_t delta = SD_GetSpriteSync()*12;

	// Will we reach a new tile?
	if (obj->user2 > delta)
	{
		// No... keep moving in the same direction.
		obj->user2 -= delta;

		int dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		int dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
	else
	{
		// Move to next tile.
		int dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += obj->user2;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= obj->user2;
		}

		int dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += obj->user2;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= obj->user2;
		}

		int tileX = (obj->posX + ck_nextX + 0x40) >> 8;
		int tileY = (obj->posY + ck_nextY + 0x40) >> 8;

		obj->user1 = CA_TileAtPos(tileX, tileY, 2) - 0x5B;

		if ((obj->user1 < 0) || (obj->user1 > 8))
		{
			Quit("Goplat moved to a bad spot.");
		}

		delta -= obj->user2;
		obj->user2 = 256 - delta;

		// Move in the new direction.
		dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
}

void CK5_SpawnVolte(int tileX, int tileY)
{

	int dir;

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT5_Volte;
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = tileX << 8;
	new_object->posY = tileY << 8;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Volte0"));

	// Initialize Volte Direction
	// (Vanilla keen does this with far pointer arithmetic)
	if (CA_TileAtPos(tileX-1, tileY, 2) == 0x5C)
		dir = 1;
	else if (CA_TileAtPos(tileX+1, tileY, 2) == 0x5E)
		dir = 3;
	else if (CA_TileAtPos(tileX, tileY-1, 2) == 0x5D)
		dir = 2;
	else if (CA_TileAtPos(tileX, tileY+1, 2) == 0x5B)
		dir = 0;
	else
		Quit ("Volte spawned at bad spot!");  // Not present in vanilla keen

	CA_SetTileAtPos(tileX, tileY, 2, dir + 0x5B);
	new_object->user1 = dir;
	new_object->user2 = 0x100;
}


// This is very similar to the GoPlat function
// The only difference is the increased speed and the error message
void CK5_VolteMove(CK_object *obj)
{

	if (ck_nextX || ck_nextY) return;

	int16_t delta = SD_GetSpriteSync()*32;

	// Will we reach a new tile?
	if (obj->user2 > delta)
	{
		// No... keep moving in the same direction.
		obj->user2 -= delta;

		int16_t dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		int16_t dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
	else
	{
		// Move to next tile.
		int16_t dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += obj->user2;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= obj->user2;
		}

		int16_t dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += obj->user2;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= obj->user2;
		}

		int16_t tileX = (obj->posX + ck_nextX) >> 8;
		int16_t tileY = (obj->posY + ck_nextY) >> 8;

		obj->user1 = CA_TileAtPos(tileX, tileY, 2) - 0x5B;

		if ((obj->user1 < 0) || (obj->user1 > 8))
		{
			// TODO: Add printf style variable arg list to Quit()
			// and add the offending tile here
			Quit("Volte moved to a bad spot");
		}

		delta -= obj->user2;
		obj->user2 = 256 - delta;

		// Move in the new direction.
		dirX = ck5_infoplaneArrowsX[obj->user1];
		if (dirX == 1)
		{
			// Moving right.
			ck_nextX += delta;
		}
		else if (dirX == -1)
		{
			// Moving left
			ck_nextX -= delta;
		}

		dirY = ck5_infoplaneArrowsY[obj->user1];
		if (dirY == 1)
		{
			// Moving down
			ck_nextY += delta;
		}
		else if (dirY == -1)
		{
			// Moving up
			ck_nextY -= delta;
		}
	}
}

void CK5_VolteCol(CK_object *volte, CK_object *other)
{

	if (other->type == CT_Player)
	{
		CK_KillKeen();
	}
	else if (other->type == CT_Stunner)
	{ //stunner
		CK_ShotHit(other);
		CK_SetAction2(volte, CK_GetActionByName("CK5_ACT_VolteStunned"));
	}
}

/*
 * Setup all of the functions in this file.
 */
void CK5_Obj1_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_SneakPlatThink", &CK5_SneakPlatThink);
	CK_ACT_AddFunction("CK5_RedGoPlatThink", &CK5_RedGoPlatThink);
	CK_ACT_AddFunction("CK5_PurpleGoPlatThink", &CK5_PurpleGoPlatThink);

	// VolteFace
	CK_ACT_AddFunction("CK5_VolteMove", &CK5_VolteMove);
	CK_ACT_AddColFunction("CK5_VolteCol", &CK5_VolteCol);
}
