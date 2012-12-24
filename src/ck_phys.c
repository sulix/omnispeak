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

#include "ck_phys.h"
#include "ck_play.h"
#include "ck_def.h"
#include "id_vh.h"
#include "id_ca.h"
#include "id_us.h"

#include <stdio.h>
#include <stdlib.h> /* For abs() */

static int ck_physSlopeHeight[8][16] = 
{
	{ 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100 },
	{   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0 },
    {   0x0,   0x8,  0x10,  0x18,  0x20,  0x28,  0x30,  0x38,  0x40,  0x48,  0x50,  0x58,  0x60,  0x68,  0x70,  0x78 },
    {  0x80,  0x88,  0x90,  0x98,  0xa0,  0xa8,  0xb0,  0xb8,  0xc0,  0xc8,  0xd0,  0xd8,  0xe0,  0xe8,  0xf0,  0xf8 },
    {   0x0,  0x10,  0x20,  0x30,  0x40,  0x50,  0x60,  0x70,  0x80,  0x90,  0xa0,  0xb0,  0xc0,  0xd0,  0xe0,  0xf0 },
    {  0x78,  0x70,  0x68,  0x60,  0x58,  0x50,  0x48,  0x40,  0x38,  0x30,  0x28,  0x20,  0x18,  0x10,  0x08,  0x0  },
    {  0xF8,  0xF0,  0xE8,  0xE0,  0xD8,  0xD0,  0xC8,  0xC0,  0xB8,  0xB0,  0xA8,  0xA0,  0x98,  0x90,  0x88,  0x80 },
    {  0xF0,  0xE0,  0xD0,  0xC0,  0xB0,  0xA0,  0x90,  0x80,  0x70,  0x60,  0x50,  0x40,  0x30,  0x20,  0x10,  0x0  }
};

extern CK_object *ck_keenObj;

void CK_ResetClipRects(CK_object *obj)
{
	//NOTE: As tile rects are rarely used, keen does not calculate them here.
	int spriteNumber = obj->gfxChunk - ca_gfxInfoE.offSprites;

	VH_SpriteTableEntry ste = VH_GetSpriteTableEntry(spriteNumber);

	obj->clipRects.unitX1 = obj->posX + ste.xl;
	obj->clipRects.unitX2 = obj->posX + ste.xh;
	obj->clipRects.unitY1 = obj->posY + ste.yl;
	obj->clipRects.unitY2 = obj->posY + ste.yh;

	obj->clipRects.unitXmid = (obj->clipRects.unitX2 - obj->clipRects.unitX1)/2 + obj->clipRects.unitX1;

	obj->clipRects.tileX1 = obj->clipRects.unitX1 >> 8;
	obj->clipRects.tileX2 = obj->clipRects.unitX2 >> 8;
	obj->clipRects.tileY1 = obj->clipRects.unitY1 >> 8;
	obj->clipRects.tileY2 = obj->clipRects.unitY2 >> 8;

	obj->clipRects.tileXmid = obj->clipRects.unitXmid >> 8;
}

void CK_SetOldClipRects(CK_object *obj)
{
	obj->oldRects.unitX1 = obj->clipRects.unitX1;
	obj->oldRects.unitX2 = obj->clipRects.unitX2;
	obj->oldRects.unitY1 = obj->clipRects.unitY1;
	obj->oldRects.unitY2 = obj->clipRects.unitY2;

	obj->oldRects.unitXmid = obj->clipRects.unitXmid;

	obj->oldRects.tileX1 = obj->clipRects.tileX1;
	obj->oldRects.tileX2 = obj->clipRects.tileX2;
	obj->oldRects.tileY1 = obj->clipRects.tileY1;
	obj->oldRects.tileY2 = obj->clipRects.tileY2;

	obj->oldRects.tileXmid = obj->clipRects.tileXmid;
}

void CK_PhysUpdateX(CK_object *obj, int deltaUnitX)
{
	obj->posX += deltaUnitX;
	obj->clipRects.unitX1 += deltaUnitX;
	obj->clipRects.unitX2 += deltaUnitX;
	//obj->clipRects.unitXmid += deltaUnitX;

	//Now update the tile rect
	// WARNING: Keen _doesn't_ update tileXmid!

	obj->clipRects.tileX1 = obj->clipRects.unitX1 >> 8;
	obj->clipRects.tileX2 = obj->clipRects.unitX1 >> 8;
}

void CK_PhysUpdateY(CK_object *obj, int deltaUnitY)
{
	obj->posY += deltaUnitY;
	obj->clipRects.unitY1 += deltaUnitY;
	obj->clipRects.unitY2 += deltaUnitY;
	
	obj->clipRects.tileY1 = obj->clipRects.unitY1 >> 8;
	obj->clipRects.tileY2 = obj->clipRects.unitY2 >> 8;
}

void CK_PhysKeenClipDown(CK_object *obj)
{
	int deltaX, deltaY;
	int midTileXOffset;
	int topTI;

	
	if (obj->xDirection == 1)
	{
		midTileXOffset = 0;
		deltaX = obj->clipRects.unitX2 - obj->clipRects.unitXmid;
		topTI = TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2,1));
		if (TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2-2,1)) || TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2-1,1)) || TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 - 1,1)))
		{
			return;
		}
	}
	else
	{
		midTileXOffset = 15;
		deltaX = obj->clipRects.unitX1 - obj->clipRects.unitXmid;
		topTI = TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2, 1));
		if (TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2-2,1)) || TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2-1,1)) || TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2 - 1,1)))
		{
			return;
		}
	}
	

	if (topTI == 1)
	{
		int slope = ck_physSlopeHeight[(topTI & 7)][midTileXOffset];
		int deltaY = (obj->clipRects.tileY2 << 8) + slope - 1 - obj->clipRects.unitY2;
		if (deltaY <= 0 && -(obj->clipRects.unitY2 - obj->oldRects.unitY2) <= deltaY)
		{
			obj->topTI = topTI;
			CK_PhysUpdateY(obj, deltaY);
			CK_PhysUpdateX(obj, deltaX);
		}
	}
}

void CK_PhysKeenClipUp(CK_object *obj)
{
	int deltaX, deltaY;
	int midTileXOffset;
	int bottomTI;

	if (obj->xDirection == 1)
	{
		midTileXOffset = 0;
		bottomTI = TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1, 1));
		if (TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1+2, 1)) || TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1+3,1)) || TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1+1,1)))
		{
			return;
		}
	}
	else
	{
		midTileXOffset = 15;
		bottomTI = TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1, 1));
		if (TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1+2, 1)) || TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1+3,1)) || TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1+1,1)))
		{
			return;
		}
	}

	if (bottomTI)
	{
		int slopeAmt = ck_physSlopeHeight[bottomTI & 0x07][midTileXOffset];
		deltaY = ((obj->clipRects.tileY1+1) << 8) - slopeAmt - obj->clipRects.unitY1;
		if (deltaY >= 0 && (obj->oldRects.unitY1 - obj->clipRects.unitY1) >= deltaY)
		{
			obj->bottomTI = bottomTI;
			CK_PhysUpdateY(obj, deltaY);
		}
	}

}

bool CK_NotStuckInWall(CK_object *obj)
{
	for(int y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; ++y)
	{
		for(int x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; ++x)
		{
			int tile = CA_TileAtPos(x,y,1);
			//TODO: Bottom and left.
			if (TI_ForeTop(tile) || TI_ForeRight(tile))
				return false;
		}
	}
	return true;
}

void CK_PhysClipVert(CK_object *obj)
{
	int midTileXOffset = (obj->clipRects.unitXmid >> 4) & 0x0F;



	
	int vertDisplace = -abs(obj->clipRects.unitXmid - obj->oldRects.unitXmid) - obj->clipRects.unitY2 + obj->oldRects.unitY2 - 16;	// Move above slope first.



	for (int y = obj->oldRects.tileY2-1; obj->clipRects.tileY2 >= y; ++y)
	{
		int tile = CA_TileAtPos(obj->clipRects.tileXmid, y, 1);
		if (TI_ForeTop(tile))
		{

			int slopeAmt = ck_physSlopeHeight[TI_ForeTop(tile)&0x07][midTileXOffset];

			int objYOffset = obj->clipRects.unitY2 - (y*256);

			int dy = (slopeAmt - objYOffset) - 1;
			if ((dy < 0) && (dy >= vertDisplace))
			{
				obj->topTI = TI_ForeTop(tile);

				CK_PhysUpdateY(obj,dy);//-objYOffset+slopeAmt-1);
				return;
			}
		}
	}
	//TODO: Ceiling collision.

	vertDisplace = abs(obj->clipRects.unitXmid - obj->oldRects.unitXmid) + obj->oldRects.unitY1 - obj->clipRects.unitY1 + 16;

	for (int y = obj->oldRects.tileY1+1; y >= obj->clipRects.tileY1; --y)
	{
		int tile = CA_TileAtPos(obj->clipRects.tileXmid,y,1);
		
		if (TI_ForeBottom(tile))
		{

			int objYOffset = obj->clipRects.unitY1 - ((y+1) << 8);

			int slopeAmt = -ck_physSlopeHeight[TI_ForeBottom(tile)&0x07][midTileXOffset];

			if ((slopeAmt - objYOffset > 0) && ((slopeAmt - objYOffset) <= vertDisplace) && ((obj->nextY +slopeAmt-objYOffset) < 256) && ((obj->nextY + slopeAmt - objYOffset) > -256))
			{
				obj->bottomTI = TI_ForeBottom(tile);
				CK_PhysUpdateY(obj,-objYOffset+slopeAmt);
			}
		}
	}
}
	

void CK_PhysClipHorz(CK_object *obj)
{
	int tileY1 = obj->clipRects.tileY1;
	int tileY2 = obj->clipRects.tileY2;

	//TODO: Increment on topFlags, bottomFlags.
	if (obj->topTI > 1)	//If we're on a slope
	{
		tileY2--;	//Collide horizontally
	}
	if (obj->bottomTI > 1)
	{
		tileY1++;
	}


	//Check if our left side is intersecting with a wall.
	for (int y = tileY1; y <= tileY2; ++y)
	{
		int tile = CA_TileAtPos(obj->clipRects.tileX1,y,1);
		obj->rightTI = TI_ForeRight(tile);
		if (obj->rightTI)
		{
			// Clipping right
			// Push us right until we're no-longer intersecting.
			CK_PhysUpdateX(obj, ((obj->clipRects.tileX1 + 1) << 8) - obj->clipRects.unitX1);
			return;
		}
	}

	//Similarly for the right side (left side of instersecting tile).
	for (int y = tileY1; y <= tileY2; ++y)
	{
		int tile = CA_TileAtPos(obj->clipRects.tileX2,y,1);
		obj->leftTI = TI_ForeLeft(tile);
		if (obj->leftTI)
		{
			// Push us left until we're no-longer intersecting.
			CK_PhysUpdateX(obj, (obj->clipRects.tileX2 << 8) - 1 - obj->clipRects.unitX2);
			return;
		
		}
	}
}

//TODO: Finish
void CK_PhysUpdateNormalObj(CK_object *obj)
{
	int oldUnitX, oldUnitY;
	bool wasNotOnPlatform = false;
	oldUnitX = obj->posX;
	oldUnitY = obj->posY;

	if (obj->currentAction->stickToGround) //unknown1)
	{
		// If the object is resting on a platform
		if (obj->topTI == 0x19)
		{
			obj->velY = 145;
		}
		else
		{
			if (obj->nextX > 0)
			{
				obj->nextY = (obj->nextX) + 16;
			}
			else
			{
				obj->nextY = -(obj->nextX) + 16;
			}
			wasNotOnPlatform = true;
		}
	}

	if (obj->nextX > 240-1)
	{
		obj->nextX = 240-1;
	}
	else if (obj->nextX < -240+1)
	{
		obj->nextX = -240+1;
	}

	if (obj->nextY > 255)
	{
		obj->nextY = 255;
	}
	else if (obj->nextY < -239)
	{
		obj->nextY = -239;
	}

	obj->posX += obj->nextX;
	obj->posY += obj->nextY;


	obj->visible = true;

	if (obj->gfxChunk)
	{

		CK_SetOldClipRects(obj);

		CK_ResetClipRects(obj);

		//Reset the tile clipping vars.
		obj->topTI = 0;
		obj->bottomTI = 0;
		obj->leftTI = 0;
		obj->rightTI = 0;

		if (obj->clipped)
		{
			//NOTE: Keen calculates a clip-rect delta here.

			CK_PhysClipVert(obj);

			//TODO: Handle keen NOT being pushed.
			if (obj == ck_keenObj)
			{
				if (!obj->topTI && obj->clipRects.unitY2 > obj->oldRects.unitY2)
				{
					CK_PhysKeenClipDown(obj);
				}
				if (!obj->bottomTI && obj->clipRects.unitY1 < obj->oldRects.unitY1)
				{
					CK_PhysKeenClipUp(obj);
				}
			}

			CK_PhysClipHorz(obj);
		}
	
		//TODO: Something strange about reseting if falling?

		if (!obj->topTI && wasNotOnPlatform)
		{
			obj->posX = oldUnitX + obj->nextX;
			obj->posY = oldUnitY;
			CK_ResetClipRects(obj);
		}

		obj->deltaPosX += obj->posX - oldUnitX;
		obj->deltaPosY += obj->posY - oldUnitY;
	}
	
}


//TODO: Finish Implementing!!!!
void CK_FullClipToWalls(CK_object *obj)
{
	int oldUnitX = obj->posX;
	int oldUnitY = obj->posY;

	obj->visible = true;

	//TODO: Verify object class (need callback to episode struct)
	// Error msg: "FullClipToWalls: Bad obclass"

}

void CK_PhysUpdateSimpleObj(CK_object *obj)
{
	int oldUnitX, oldUnitY;
	oldUnitX = obj->posX;
	oldUnitY = obj->posY;


	obj->posX += obj->nextX;
	obj->posY += obj->nextY;


	obj->visible = true;

	if (obj->gfxChunk)
	{

		CK_SetOldClipRects(obj);

		CK_ResetClipRects(obj);

		//Reset the tile clipping vars.
		obj->topTI = 0;
		obj->bottomTI = 0;
		obj->leftTI = 0;
		obj->rightTI = 0;

		if (obj->clipped)
		{
			CK_PhysClipVert(obj);
			CK_PhysClipHorz(obj);
		}

		obj->deltaPosX += obj->posX - oldUnitX;
		obj->deltaPosY += obj->posY - oldUnitY;
	}
	
}

void CK_PhysPushX(CK_object *pushee, CK_object *pusher)
{
	int deltaPosX = pusher->deltaPosX - pushee->deltaPosX;
	int deltaX_1 = pusher->clipRects.unitX2 - pushee->clipRects.unitX1;
	int deltaX_2 = pushee->clipRects.unitX2 - pusher->clipRects.unitX1;

	// Push object to the right
	if ((deltaX_1 > 0) && (deltaX_1 <= deltaPosX))
	{
		pushee->nextX = deltaX_1;
		if (pushee->currentAction->stickToGround)
			pushee->nextY = -deltaX_1 + 16;
		CK_PhysUpdateNormalObj(pushee);
		pushee->rightTI = 1;
		return;
	}

	// Push object to the left
	if (deltaX_2 > 0 && deltaX_2 >= -deltaPosX)
	{
		pushee->nextX = -deltaX_2;
		if (pushee->currentAction->stickToGround)
			pushee->nextY = deltaX_2 + 16;
		CK_PhysUpdateNormalObj(pushee);
		pushee->leftTI = 1;
		return;
	}
}

void CK_PhysPushY(CK_object *pushee, CK_object *pusher)
{
	int deltaDeltaY = pushee->deltaPosY - pusher->deltaPosY;
	int deltaClipY = pushee->clipRects.unitY2 - pusher->clipRects.unitY1;

	if (deltaClipY >= 0 && deltaClipY < deltaDeltaY)
	{
		//If the pushee is keen, set ck_keenState.currentPlatform to pusher
		// (I'm not sure I like this)
		if (pushee == ck_keenObj)
			ck_keenState.platform = pusher;

		pushee->nextY = -deltaClipY;
		bool pusheeSticksToGround = pushee->currentAction->stickToGround;
		pushee->currentAction->stickToGround = false;
		CK_PhysUpdateNormalObj(pushee);
		pushee->currentAction->stickToGround = pusheeSticksToGround;

		if (!pushee->bottomTI)
			pushee->topTI = 0x19; // Platform?
	}
}

void CK_SetAction(CK_object *obj, CK_action *act)
{
	obj->currentAction = act;

	if (act->chunkRight && obj->xDirection > 0) obj->gfxChunk = act->chunkRight;
	else if (act->chunkLeft) obj->gfxChunk = act->chunkLeft;

	if (obj->gfxChunk == -1) obj->gfxChunk = 0;

	obj->nextX = 0;
	obj->nextY = 0;

	//TODO: Support clipped enums.
	bool oldClipping = obj->clipped;
	obj->clipped = false;
	CK_PhysUpdateNormalObj(obj);
	obj->clipped = oldClipping;

	//TODO: Handle platforms
	if (obj->clipped)
		CK_PhysUpdateNormalObj(obj);
}

void CK_SetAction2(CK_object *obj, CK_action *act)
{
	obj->currentAction = act;

	if (act->chunkRight && obj->xDirection > 0) obj->gfxChunk = act->chunkRight;
	else if (act->chunkLeft) obj->gfxChunk = act->chunkLeft;

	if (obj->gfxChunk == -1) obj->gfxChunk = 0;

	obj->visible = true;
	obj->nextX = 0;
	obj->nextY = 0;


	if (!obj->topTI == 0x19)
		CK_PhysUpdateNormalObj(obj);

}


void CK_PhysGravityHigh(CK_object *obj)
{
	long numTics = CK_GetNumTotalTics();
	int tics = CK_GetTicksPerFrame();
	for (long ticCount = numTics-tics; ticCount < numTics; ++ticCount)
	{
		// Every odd tic...
		if (ticCount & 1)
		{
			if (obj->velY < 0 && obj->velY > -4)
			{
				obj->nextY += obj->velY;
				obj->velY = 0;
				return;
			}
			obj->velY += 4;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		obj->nextY += obj->velY;
	}
}

void CK_PhysGravityMid(CK_object *obj)
{
	long numTics = CK_GetNumTotalTics();
	int tics = CK_GetTicksPerFrame();
	for (long ticCount = numTics-tics; ticCount < numTics; ++ticCount)
	{
		// Every odd tic...
		if (ticCount & 1)
		{
			if (obj->velY < 0 && obj->velY > -3)
			{
				obj->nextY += obj->velY;
				obj->velY = 0;
				return;
			}
			obj->velY += 3;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		obj->nextY += obj->velY;
	}
}

void CK_PhysGravityLow(CK_object *obj)
{
	long numTics = CK_GetNumTotalTics();
	int tics = CK_GetTicksPerFrame();
	for (long ticCount = numTics-tics; ticCount < numTics; ++ticCount)
	{
		// TODO: recheck this condition
		//if ((ticCount & 3) == 1)
		if ((ticCount?0:1) & 3)
		{
			obj->velY += 1;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		obj->nextY += obj->velY;
	}
}

void CK_PhysDampHorz(CK_object *obj)
{
	long numTics = CK_GetNumTotalTics();
	int tics = CK_GetTicksPerFrame();
	bool movingLeft = (obj->velX < 0);
	for (long ticCount = numTics-tics; ticCount < numTics; ++ticCount)
	{
		// Every odd tic...
		if (ticCount & 1)
		{
			if (movingLeft)
			{
				obj->velX++;
			}
			else
			{
				obj->velX--;
			}
			if ((obj->velX < 0) != movingLeft)
			{
				obj->velX = 0;
			}
		}
		obj->nextX += obj->velX;
	}
}

void CK_PhysAccelHorz(CK_object *obj, int accX, int velLimit)
{
	long numTics = CK_GetNumTotalTics();
	int tics = CK_GetTicksPerFrame();
	bool isNegative = (obj->velX < 0);
	for (long ticCount = numTics-tics; ticCount < numTics; ++ticCount)
	{
		// Every odd tic...
		if (ticCount & 1)
		{
			obj->velX += accX;
			if ((obj->velX < 0) != isNegative)
			{
				isNegative = (obj->velX < 0);
				obj->xDirection = isNegative?-1:1;
			}

			if (obj->velX > velLimit)
			{
				obj->velX = velLimit;
			}
			else if (obj->velX < -velLimit)
			{
				obj->velX = -velLimit;
			}
		}
		obj->nextX += obj->velX;
	}
}
