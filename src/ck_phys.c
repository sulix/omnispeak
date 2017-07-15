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
#include "id_rf.h"
#include "id_us.h"

#include <stdio.h>
#include <stdlib.h> /* For abs() */

static int16_t ck_physSlopeHeight[8][16] ={
	{ 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100 },
	{   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0 },
	{   0x0,   0x8,  0x10,  0x18,  0x20,  0x28,  0x30,  0x38,  0x40,  0x48,  0x50,  0x58,  0x60,  0x68,  0x70,  0x78 },
	{  0x80,  0x88,  0x90,  0x98,  0xa0,  0xa8,  0xb0,  0xb8,  0xc0,  0xc8,  0xd0,  0xd8,  0xe0,  0xe8,  0xf0,  0xf8 },
	{   0x0,  0x10,  0x20,  0x30,  0x40,  0x50,  0x60,  0x70,  0x80,  0x90,  0xa0,  0xb0,  0xc0,  0xd0,  0xe0,  0xf0 },
	{  0x78,  0x70,  0x68,  0x60,  0x58,  0x50,  0x48,  0x40,  0x38,  0x30,  0x28,  0x20,  0x18,  0x10,  0x08,  0x0  },
	{  0xF8,  0xF0,  0xE8,  0xE0,  0xD8,  0xD0,  0xC8,  0xC0,  0xB8,  0xB0,  0xA8,  0xA0,  0x98,  0x90,  0x88,  0x80 },
	{  0xF0,  0xE0,  0xD0,  0xC0,  0xB0,  0xA0,  0x90,  0x80,  0x70,  0x60,  0x50,  0x40,  0x30,  0x20,  0x10,  0x0  }
};

CK_objPhysData ck_oldRects;
CK_objPhysDataDelta ck_deltaRects;
int16_t ck_nextX;
int16_t ck_nextY;

// Present in ck6; set when keen is being moved by a gik or blooglet
bool ck_keenIgnoreVertClip;

extern CK_object *ck_keenObj;

void CK_ResetClipRects(CK_object *obj)
{
	//NOTE: As tile rects are rarely used, keen does not calculate them here.
	int spriteNumber = obj->gfxChunk - ca_gfxInfoE.offSprites;

	VH_SpriteTableEntry *ste = VH_GetSpriteTableEntry(spriteNumber);

	obj->clipRects.unitX1 = obj->posX + ste->xl;
	obj->clipRects.unitX2 = obj->posX + ste->xh;
	obj->clipRects.unitY1 = obj->posY + ste->yl;
	obj->clipRects.unitY2 = obj->posY + ste->yh;

	obj->clipRects.unitXmid = (obj->clipRects.unitX2 - obj->clipRects.unitX1) / 2 + obj->clipRects.unitX1;

	obj->clipRects.tileX1 = RF_UnitToTile(obj->clipRects.unitX1);
	obj->clipRects.tileX2 = RF_UnitToTile(obj->clipRects.unitX2);
	obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
	obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);

	obj->clipRects.tileXmid = RF_UnitToTile(obj->clipRects.unitXmid);
}

void CK_SetOldClipRects(CK_object *obj)
{
	ck_oldRects.unitX1 = obj->clipRects.unitX1;
	ck_oldRects.unitX2 = obj->clipRects.unitX2;
	ck_oldRects.unitY1 = obj->clipRects.unitY1;
	ck_oldRects.unitY2 = obj->clipRects.unitY2;

	ck_oldRects.unitXmid = obj->clipRects.unitXmid;

	ck_oldRects.tileX1 = obj->clipRects.tileX1;
	ck_oldRects.tileX2 = obj->clipRects.tileX2;
	ck_oldRects.tileY1 = obj->clipRects.tileY1;
	ck_oldRects.tileY2 = obj->clipRects.tileY2;

	ck_oldRects.tileXmid = obj->clipRects.tileXmid;
}

void CK_SetDeltaClipRects(CK_object *obj)
{
	ck_deltaRects.unitX1 = obj->clipRects.unitX1 - ck_oldRects.unitX1;
	ck_deltaRects.unitY1 = obj->clipRects.unitY1 - ck_oldRects.unitY1;
	ck_deltaRects.unitX2 = obj->clipRects.unitX2 - ck_oldRects.unitX2;
	ck_deltaRects.unitY2 = obj->clipRects.unitY2 - ck_oldRects.unitY2;

	ck_deltaRects.unitXmid = obj->clipRects.unitXmid - ck_oldRects.unitXmid;

	// We don't calculate tile deltas.
}

void CK_PhysUpdateX(CK_object *obj, int16_t deltaUnitX)
{
	obj->posX += deltaUnitX;
	obj->clipRects.unitX1 += deltaUnitX;
	obj->clipRects.unitX2 += deltaUnitX;

	// It looks like only Keen 6 updates unitXmid here.
	if (ck_currentEpisode->ep == EP_CK6)
		obj->clipRects.unitXmid += deltaUnitX;

	//Now update the tile rect
	// WARNING: Keen _doesn't_ update tileXmid!

	obj->clipRects.tileX1 = RF_UnitToTile(obj->clipRects.unitX1);
	obj->clipRects.tileX2 = RF_UnitToTile(obj->clipRects.unitX2);
}

void CK_PhysUpdateY(CK_object *obj, int16_t deltaUnitY)
{
	obj->posY += deltaUnitY;
	obj->clipRects.unitY1 += deltaUnitY;
	obj->clipRects.unitY2 += deltaUnitY;

	obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
	obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);
}

void CK_PhysKeenClipDown(CK_object *obj)
{
	// Amount we're moving in each direction.
	int16_t deltaX, deltaY;
	// The part of the slope we care about (in px)
	int16_t midTileXOffset;
	// The top of the tile at Keen's feet.
	int16_t topTI;
	// Is there space above the slope?
	bool spaceAbove = false;

	// Performing some special clipping for Keen.

	// If we're moving right:
	if (obj->xDirection == 1)
	{
		// We care about the lefthand-most side of any slope we're about to touch.
		midTileXOffset = 0;
		deltaX = obj->clipRects.unitX2 - obj->clipRects.unitXmid;
		spaceAbove = (TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 - 1, 1)) == 0);
		topTI = TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2, 1));

		// If we're being blocked by something, return.
		if (TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 - 2, 1)) || TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 - 1, 1)) || TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2 - 1, 1)))
		{
			return;
		}
	}
	else // We're moving left:
	{
		// We care about the righthand-most side of the slope
		midTileXOffset = 15;
		deltaX = obj->clipRects.unitX1 - obj->clipRects.unitXmid;
		spaceAbove = (TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2 - 1, 1)) == 0);
		topTI = TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2, 1));

		// If we're being blocked, return.
		if (TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2 - 2, 1)) || TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2 - 1, 1)) || TI_ForeTop(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2 - 1, 1)))
		{
			return;
		}
	}


	// If we're about to land on something flat:
	// TODO: This doesn't make any sense. Why would we make sure we're not
	// on a slope before doing slope calculations. Something fishy, methinks.
	if (spaceAbove && (topTI == 1))
	{
		int16_t slope = ck_physSlopeHeight[(topTI & 7)][midTileXOffset];
		int16_t deltaY = RF_TileToUnit(obj->clipRects.tileY2) + slope - 1 - obj->clipRects.unitY2;
		if (deltaY <= 0 && -(ck_deltaRects.unitY2) <= deltaY)
		{
			obj->topTI = topTI;
			CK_PhysUpdateY(obj, deltaY);
			CK_PhysUpdateX(obj, deltaX);
		}
	}
}

void CK_PhysKeenClipUp(CK_object *obj)
{
	int16_t deltaX, deltaY;
	int16_t midTileXOffset;
	int16_t bottomTI;
	bool spaceBelow;

	if (obj->xDirection == 1)
	{
		midTileXOffset = 0;
		bottomTI = TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1, 1));
		spaceBelow = (TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 + 1, 1)) == 0);
		if (TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 + 2, 1)) || TI_ForeLeft(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 + 3, 1)) || TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1 + 1, 1)))
		{
			return;
		}
	}
	else
	{
		midTileXOffset = 15;
		bottomTI = TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1, 1));
		spaceBelow = (TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1 + 1, 1)) == 0);
		if (TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1 + 2, 1)) || TI_ForeRight(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1 + 3, 1)) || TI_ForeBottom(CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1 + 1, 1)))
		{
			return;
		}
	}

	if (spaceBelow && bottomTI)
	{
		int16_t slopeAmt = ck_physSlopeHeight[bottomTI & 0x07][midTileXOffset];
		deltaY = RF_TileToUnit(obj->clipRects.tileY1 + 1) - slopeAmt - obj->clipRects.unitY1;
		if (deltaY >= 0 && (-ck_deltaRects.unitY1) >= deltaY)
		{
			obj->bottomTI = bottomTI;
			CK_PhysUpdateY(obj, deltaY);
		}
	}

}

bool CK_NotStuckInWall(CK_object *obj)
{
	for (uint16_t y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; ++y)
	{
		for (uint16_t x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; ++x)
		{
			uint16_t tile = CA_TileAtPos(x, y, 1);
			if (TI_ForeTop(tile) || TI_ForeRight(tile) || TI_ForeBottom(tile) || TI_ForeLeft(tile))
				return false;
		}
	}
	return true;
}

// Similar to CK_ResetClipRects, but checks if new action will get object stuck in wall
// Only seems to be used in CK4 in a Bluebird think function
bool CK_PreviewClipRects(CK_object *obj, CK_action *act)
{
	obj->gfxChunk = obj->xDirection < 0 ? act->chunkLeft : act->chunkRight;
	int spriteNumber = obj->gfxChunk - ca_gfxInfoE.offSprites;

	VH_SpriteTableEntry *ste = VH_GetSpriteTableEntry(spriteNumber);

	obj->clipRects.unitX1 = obj->posX + ste->xl;
	obj->clipRects.unitX2 = obj->posX + ste->xh;
	obj->clipRects.unitY1 = obj->posY + ste->yl;
	obj->clipRects.unitY2 = obj->posY + ste->yh;

	obj->clipRects.unitXmid = (obj->clipRects.unitX2 - obj->clipRects.unitX1) / 2 + obj->clipRects.unitX1;

	obj->clipRects.tileX1 = RF_UnitToTile(obj->clipRects.unitX1);
	obj->clipRects.tileX2 = RF_UnitToTile(obj->clipRects.unitX2);
	obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
	obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);

	obj->clipRects.tileXmid = RF_UnitToTile(obj->clipRects.unitXmid);

	return CK_NotStuckInWall(obj);
}

void CK_PhysClipVert(CK_object *obj)
{
	int16_t midTileXOffset = RF_UnitToPixel(obj->clipRects.unitXmid) & 0x0F;




	int16_t vertDisplace = -abs(ck_deltaRects.unitXmid) - ck_deltaRects.unitY2 - 16;	// Move above slope first.



	for (uint16_t y = ck_oldRects.tileY2 - 1; obj->clipRects.tileY2 >= y; ++y)
	{
		int16_t tile = CA_TileAtPos(obj->clipRects.tileXmid, y, 1);
		if (TI_ForeTop(tile))
		{

			int16_t slopeAmt = ck_physSlopeHeight[TI_ForeTop(tile)&0x07][midTileXOffset];

			int16_t objYOffset = obj->clipRects.unitY2 - (y * 256);

			int16_t dy = (slopeAmt - objYOffset) - 1;
			if ((dy < 0) && (dy >= vertDisplace))
			{
				obj->topTI = TI_ForeTop(tile);

				CK_PhysUpdateY(obj, dy); //-objYOffset+slopeAmt-1);
				return;
			}
		}
	}
	vertDisplace = abs(ck_deltaRects.unitXmid) - ck_deltaRects.unitY1 + 16;

	for (uint16_t y = ck_oldRects.tileY1 + 1; y >= obj->clipRects.tileY1; --y)
	{
		int16_t tile = CA_TileAtPos(obj->clipRects.tileXmid, y, 1);

		if (TI_ForeBottom(tile))
		{

			int16_t objYOffset = obj->clipRects.unitY1 - RF_TileToUnit(y + 1);

			int16_t slopeAmt = -ck_physSlopeHeight[TI_ForeBottom(tile)&0x07][midTileXOffset];

			if ((slopeAmt - objYOffset > 0) && ((slopeAmt - objYOffset) <= vertDisplace) && ((ck_nextY + slopeAmt - objYOffset) < 256) && ((ck_nextY + slopeAmt - objYOffset) > -256))
			{
				obj->bottomTI = TI_ForeBottom(tile);
				CK_PhysUpdateY(obj, -objYOffset + slopeAmt);
			}
		}
	}
}

void CK_PhysClipHorz(CK_object *obj)
{
	uint16_t tileY1 = obj->clipRects.tileY1;
	uint16_t tileY2 = obj->clipRects.tileY2;

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
	for (uint16_t y = tileY1; y <= tileY2; ++y)
	{
		int16_t tile = CA_TileAtPos(obj->clipRects.tileX1, y, 1);
		obj->rightTI = TI_ForeRight(tile);
		if (obj->rightTI)
		{
			// Clipping right
			// Push us right until we're no-longer intersecting.
			CK_PhysUpdateX(obj, RF_TileToUnit(obj->clipRects.tileX1 + 1) - obj->clipRects.unitX1);
			return;
		}
	}

	//Similarly for the right side (left side of instersecting tile).
	for (uint16_t y = tileY1; y <= tileY2; ++y)
	{
		int16_t tile = CA_TileAtPos(obj->clipRects.tileX2, y, 1);
		obj->leftTI = TI_ForeLeft(tile);
		if (obj->leftTI)
		{
			// Push us left until we're no-longer intersecting.
			CK_PhysUpdateX(obj, RF_TileToUnit(obj->clipRects.tileX2) - 1 - obj->clipRects.unitX2);
			return;

		}
	}
}

//TODO: Finish

void CK_PhysUpdateNormalObj(CK_object *obj)
{
	uint16_t oldUnitX, oldUnitY;
	bool wasNotOnPlatform = false;
	oldUnitX = obj->posX;
	oldUnitY = obj->posY;

	if (obj->currentAction->stickToGround) //unknown1)
	{
		// If the object is resting on a platform
		if (obj->topTI == 0x19)
		{
			ck_nextY = 145;
		}
		else
		{
			if (ck_nextX > 0)
			{
				ck_nextY = (ck_nextX) + 16;
			}
			else
			{
				ck_nextY = -(ck_nextX) + 16;
			}
			wasNotOnPlatform = true;
		}
	}

	if (ck_nextX > 240 - 1)
	{
		ck_nextX = 240 - 1;
	}
	else if (ck_nextX < -240 + 1)
	{
		ck_nextX = -240 + 1;
	}

	if (ck_nextY > 255)
	{
		ck_nextY = 255;
	}
	else if (ck_nextY < -239)
	{
		ck_nextY = -239;
	}

	obj->posX += ck_nextX;
	obj->posY += ck_nextY;


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

			CK_SetDeltaClipRects(obj);

			CK_PhysClipVert(obj);

			//TODO: Handle keen NOT being pushed.
			if (obj == ck_keenObj && (ck_currentEpisode->ep != EP_CK6 || !ck_keenIgnoreVertClip))
			{
				if (!obj->topTI && ck_deltaRects.unitY2 > 0)
				{
					CK_PhysKeenClipDown(obj);
				}
				if (!obj->bottomTI && ck_deltaRects.unitY1 < 0)
				{
					CK_PhysKeenClipUp(obj);
				}
			}

			CK_PhysClipHorz(obj);

			// Present only in the Keen 6 EXE - Handle the case Keen is pushed towards a wall (with right/left key) while standing on a sloped floor (see levels 6-8).
			if ((ck_currentEpisode->ep == EP_CK6) && (obj == ck_keenObj) && ((obj->topTI & 7) > 1) && (obj->rightTI || obj->leftTI))
			{
				// Based on code from CK_PhysClipVert
				int16_t midTileXOffset = RF_UnitToPixel(obj->clipRects.unitXmid) & 0x0F;

				for (uint16_t y = ck_oldRects.tileY2; obj->clipRects.tileY2 + 1 >= y; ++y)
				{
					int16_t tile = CA_TileAtPos(obj->clipRects.tileXmid, y, 1);
					if (TI_ForeTop(tile))
					{
						int16_t slopeAmt = ck_physSlopeHeight[TI_ForeTop(tile)&0x07][midTileXOffset];
						int16_t objYOffset = obj->clipRects.unitY2 - (y * 256);
						int16_t dy = (slopeAmt - objYOffset) - 1;

						obj->topTI = TI_ForeTop(tile);
						CK_PhysUpdateY(obj, dy); //-objYOffset+slopeAmt-1);
						return;
					}
				}
			}
		}

		//TODO: Something strange about reseting if falling?

		if (!obj->topTI && wasNotOnPlatform)
		{
			obj->posX = oldUnitX + ck_nextX;
			obj->posY = oldUnitY;
			CK_ResetClipRects(obj);
		}

		obj->deltaPosX += obj->posX - oldUnitX;
		obj->deltaPosY += obj->posY - oldUnitY;
	}

}


//TODO: Finish Implementing!!!!

void CK_PhysFullClipToWalls(CK_object *obj)
{
	uint16_t oldUnitX = obj->posX;
	uint16_t oldUnitY = obj->posY;

	if (ck_nextX > 239)
		ck_nextX = 239;
	else if (ck_nextX < -239)
		ck_nextX = -239;
	if (ck_nextY > 239)
		ck_nextY = 239;
	else if (ck_nextY < -239)
		ck_nextY = -239;

	obj->posX += ck_nextX;
	obj->posY += ck_nextY;
	obj->visible = true;

	//TODO: Verify object class (need callback to episode struct)
  int16_t delX, delY;

  switch (ck_currentEpisode->ep)
  {
    case EP_CK4:
      if (obj->type == CT4_Schoolfish)
      {
        delX = 0x100;
        delY = 0x80;
      }
      else if (obj->type == CT4_Dopefish)
      {
        delX = 0x580;
        delY = 0x400;
      }
      else if (obj->type == CT4_Bird)
      {
        delX = 0x400;
        delY = 0x200;
      }
      else if (obj->type == CT_Player)
      {
        // Scubakeen
        delX = 0x280;
        delY = 0x180;
      }
      break;
    case EP_CK5:
      if (obj->type == CT5_SliceStar || obj->type == CT5_Sphereful)
      {
        delX = delY = 512;
      }
      else
      {
        goto badobjclass;
      }
      break;

    case EP_CK6:
      if (obj->type == CT6_Blorb)
      {
        delX = delY = 0x200;
      }
      else
      {
        goto badobjclass;
      }
      break;

    default:
      break;

badobjclass:
		Quit("FullClipToWalls: Bad obclass");
  }


	obj->clipRects.unitX2 = obj->posX + delX;
	obj->clipRects.unitX1 = obj->posX;
	obj->clipRects.unitY1 = obj->posY;
	obj->clipRects.unitY2 = obj->posY + delY;
	obj->clipRects.tileX1 = RF_UnitToTile(obj->clipRects.unitX1);
	obj->clipRects.tileX2 = RF_UnitToTile(obj->clipRects.unitX2);
	obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
	obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);
	//Reset the tile clipping vars.
	obj->topTI = 0;
	obj->bottomTI = 0;
	obj->leftTI = 0;
	obj->rightTI = 0;

	if (!CK_NotStuckInWall(obj))
	{
		CK_PhysUpdateX(obj, -ck_nextX);
		if (CK_NotStuckInWall(obj))
		{
			if (ck_nextX > 0)
				obj->leftTI = 1;
			else
				obj->rightTI = 1;
		}
		else
		{
			if (ck_nextY > 0)
				obj->topTI = 1;
			else
				obj->bottomTI = 1;

			CK_PhysUpdateX(obj, ck_nextX);
			CK_PhysUpdateY(obj, -ck_nextY);
			if (!CK_NotStuckInWall(obj))
			{
				CK_PhysUpdateX(obj, -ck_nextX);
				if (ck_nextX > 0)
					obj->leftTI = 1;
				else
					obj->rightTI = 1;
			}
		}
	}
	obj->deltaPosX += (obj->posX - oldUnitX);
	obj->deltaPosY += (obj->posY - oldUnitY);

	CK_ResetClipRects(obj);
}

void CK_PhysUpdateSimpleObj(CK_object *obj)
{
	uint16_t oldUnitX, oldUnitY;
	oldUnitX = obj->posX;
	oldUnitY = obj->posY;


	obj->posX += ck_nextX;
	obj->posY += ck_nextY;


	obj->visible = true;

	if (obj->gfxChunk)
	{

		CK_SetOldClipRects(obj);

		CK_ResetClipRects(obj);

		//Reset the tile clipping vars.
		//obj->topTI = 0;
		//obj->bottomTI = 0;
		//obj->leftTI = 0;
		//obj->rightTI = 0;

		if (obj->clipped)
		{
			CK_SetDeltaClipRects(obj);
			CK_PhysClipVert(obj);
			CK_PhysClipHorz(obj);
		}

		obj->deltaPosX += obj->posX - oldUnitX;
		obj->deltaPosY += obj->posY - oldUnitY;
	}

}

void CK_PhysPushX(CK_object *pushee, CK_object *pusher)
{
	int16_t deltaPosX = pusher->deltaPosX - pushee->deltaPosX;
	int16_t deltaX_1 = pusher->clipRects.unitX2 - pushee->clipRects.unitX1;
	int16_t deltaX_2 = pushee->clipRects.unitX2 - pusher->clipRects.unitX1;

	// Push object to the right
	if ((deltaX_1 > 0) && (deltaX_1 <= deltaPosX))
	{
		ck_nextX = deltaX_1;
		if (pushee->currentAction->stickToGround)
			ck_nextY = -deltaX_1 + 16;
		CK_PhysUpdateNormalObj(pushee);
		pushee->rightTI = 1;
		return;
	}

	// Push object to the left
	if ((deltaX_2 > 0) && (-deltaPosX >= deltaX_2))
	{
		ck_nextX = -deltaX_2;
		if (pushee->currentAction->stickToGround)
			ck_nextY = deltaX_2 + 16;
		CK_PhysUpdateNormalObj(pushee);
		pushee->leftTI = 1;
		return;
	}
}

void CK_PhysPushY(CK_object *pushee, CK_object *pusher)
{
	int16_t deltaDeltaY = pushee->deltaPosY - pusher->deltaPosY;
	int16_t deltaClipY = pushee->clipRects.unitY2 - pusher->clipRects.unitY1;

	if ((deltaClipY >= 0) && (deltaClipY <= deltaDeltaY))
	{
		//If the pushee is keen, set ck_keenState.currentPlatform to pusher
		// (I'm not sure I like this)
		if (pushee == ck_keenObj)
			ck_keenState.platform = pusher;

		ck_nextY = -deltaClipY;
		bool pusheeSticksToGround = pushee->currentAction->stickToGround;
		pushee->currentAction->stickToGround = false;
		CK_PhysUpdateNormalObj(pushee);
		pushee->currentAction->stickToGround = pusheeSticksToGround;

		if (!pushee->bottomTI)
			pushee->topTI = 0x19; // Platform?
	}
}

void CK_PhysPushXY(CK_object *passenger, CK_object *platform, bool squish)
{
  int16_t dx = platform->deltaPosX - passenger->deltaPosX;
  ck_nextX = ck_nextY = 0;
  int16_t dLeft = platform->clipRects.unitX2 - passenger->clipRects.unitX1;
  int16_t dRight = passenger->clipRects.unitX2 - platform->clipRects.unitX1;

  //Push in all four directions

  if (dLeft > 0 && dx+1 >= dLeft)
  {
    ck_nextX = dLeft;
    passenger->velX = 0;
    CK_PhysUpdateSimpleObj(passenger);

    if (squish && passenger->leftTI)
      CK_KillKeen();

    passenger->rightTI = 1;
    return;
  }

  if (dRight > 0 && -dx+1 >= dRight)
  {
    ck_nextX = -dRight;
    passenger->velX = 0;
    CK_PhysUpdateSimpleObj(passenger);

    if (squish && passenger->rightTI)
      CK_KillKeen();

    passenger->leftTI = 1;
    return;
  }

  int16_t dy = passenger->deltaPosY - platform->deltaPosY;
  int16_t dTop = platform->clipRects.unitY2 - passenger->clipRects.unitY1;
  int16_t dBottom = passenger->clipRects.unitY2 - platform->clipRects.unitY1;

  if (dBottom >= 0 && dBottom <= dy)
  {
     if (passenger == ck_keenObj)
       ck_keenState.platform = platform;

     ck_nextY = -dBottom;
     CK_PhysUpdateSimpleObj(passenger);

     if (squish && passenger->bottomTI)
       CK_KillKeen();

     // Riding the platform, unless passenger hits its head
     if (!passenger->bottomTI)
       passenger->topTI = 0x19;

     return;
  }

  if (dTop >= 0 && dTop <= dy)
  {
    ck_nextY = dTop;
    CK_PhysUpdateNormalObj(passenger);
    if (squish && passenger->topTI)
      CK_KillKeen();

    passenger->bottomTI = 0x19;
    return;
  }
}

void CK_SetAction(CK_object *obj, CK_action *act)
{
	obj->currentAction = act;

	if (act->chunkRight && obj->xDirection > 0) obj->gfxChunk = act->chunkRight;
	else if (act->chunkLeft) obj->gfxChunk = act->chunkLeft;

	if (obj->gfxChunk == (uint16_t)-1) obj->gfxChunk = 0;

	ck_nextX = 0;
	ck_nextY = 0;

	//TODO: Support clipped enums.
	CK_ClipType oldClipping = obj->clipped;
	obj->clipped = CLIP_not;
	CK_PhysUpdateNormalObj(obj);
	obj->clipped = oldClipping;

	//TODO: Handle platforms
	if (obj->clipped == CLIP_simple)
		CK_PhysFullClipToWalls(obj);
	else if (obj->clipped == CLIP_normal)
		CK_PhysUpdateNormalObj(obj);
}

void CK_SetAction2(CK_object *obj, CK_action *act)
{
	obj->currentAction = act;
	obj->actionTimer = 0;

	if (act->chunkRight)
	{
		obj->gfxChunk = (obj->xDirection > 0) ? act->chunkRight : act->chunkLeft;
	}
#if 0
	if (act->chunkRight && obj->xDirection > 0) obj->gfxChunk = act->chunkRight;
	else if (act->chunkLeft) obj->gfxChunk = act->chunkLeft;
#endif
	if (obj->gfxChunk == (uint16_t)-1) obj->gfxChunk = 0;

	obj->visible = true;
	ck_nextX = 0;
	ck_nextY = 0;


	if (obj->topTI != 0x19)
		CK_PhysUpdateNormalObj(obj);

}

bool CK_ObjectVisible(CK_object *obj)
{
	// TODO: Use ScrollX0_T,  ScrollX1_T and co. directly?
	if (obj->clipRects.tileX2 < RF_UnitToTile(rf_scrollXUnit)
		|| obj->clipRects.tileY2 < RF_UnitToTile(rf_scrollYUnit)
		|| obj->clipRects.tileX1 > (RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320))
		|| obj->clipRects.tileY1 > (RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208)))
	{
		return false;
	}

	return true;
}

void CK_PhysGravityHigh(CK_object *obj)
{
	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
	{
		// Every odd tic...
		if (tickCount & 1)
		{
			if (obj->velY < 0 && obj->velY >= -4)
			{
				ck_nextY += obj->velY;
				obj->velY = 0;
				return;
			}
			obj->velY += 4;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		ck_nextY += obj->velY;
	}
}

void CK_PhysGravityMid(CK_object *obj)
{
	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
	{
		// Every odd tic...
		if (tickCount & 1)
		{
			if (obj->velY < 0 && obj->velY >= -3)
			{
				ck_nextY += obj->velY;
				obj->velY = 0;
				return;
			}
			obj->velY += 3;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		ck_nextY += obj->velY;
	}
}

void CK_PhysGravityLow(CK_object *obj)
{
	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
	{
		// TODO: recheck this condition
		//if ((tickCount & 3) == 1)
		if ((tickCount?0:1) & 3)
		//if (tickCount == 0)	// This condition is seriously fucked up.
		{
			obj->velY += 1;
			if (obj->velY > 70)
			{
				obj->velY = 70;
			}
		}
		ck_nextY += obj->velY;
	}
}

void CK_PhysDampHorz(CK_object *obj)
{
	bool movingLeft = (obj->velX < 0);
	int16_t xAdj;
	if (obj->velX > 0)
		xAdj = -1;
	else if (obj->velX < 0)
		xAdj = 1;
	else
		xAdj = 0;

	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
	{
		// Every odd tic...
		if (tickCount & 1)
		{
			obj->velX += xAdj;
			if ((obj->velX < 0) != movingLeft)
			{
				obj->velX = 0;
			}
		}
		ck_nextX += obj->velX;
	}
}

void CK_PhysAccelHorz(CK_object *obj, int16_t accX, int16_t velLimit)
{
	bool isNegative = (obj->velX < 0);
	int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
	{
		// Every odd tic...
		if (tickCount & 1)
		{
			obj->velX += accX;
			if ((obj->velX < 0) != isNegative)
			{
				isNegative = (obj->velX < 0);
				obj->xDirection = isNegative ? -1 : 1;
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
		ck_nextX += obj->velX;
	}
}

void CK_PhysAccelHorz2(CK_object *obj, int16_t accX, int16_t velLimit)
{
  int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
  {
		// Every odd tic...
		if (tickCount & 1)
		{
			obj->velX += accX;

			if (obj->velX > velLimit)
			{
				obj->velX = velLimit;
			}
			else if (obj->velX < -velLimit)
			{
				obj->velX = -velLimit;
			}
		}
		ck_nextX += obj->velX;
	}
}

void CK_PhysAccelVert1(CK_object *obj, int16_t accY, int16_t velLimit)
{
  int32_t lastTimeCount = SD_GetLastTimeCount();
	for (int32_t tickCount = lastTimeCount - SD_GetSpriteSync(); tickCount < lastTimeCount; tickCount++)
  {
		// Every odd tic...
		if (tickCount & 1)
		{
			obj->velY += accY;

			if (obj->velY > velLimit)
			{
				obj->velY = velLimit;
			}
			else if (obj->velY < -velLimit)
			{
				obj->velY = -velLimit;
			}
		}
		ck_nextY += obj->velY;
	}
}
