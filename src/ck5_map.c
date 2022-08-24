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

#include "id_ca.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_sd.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

#include <stdio.h>
#include <string.h>

// =========================================================================

//Thisis called from playloop

void CK5_MapMiscFlagsCheck(CK_object *keen)
{

	int midTileX, midTileY;

	if (keen->user3)
		return;

	midTileX = keen->clipRects.tileXmid;
	midTileY = ((keen->clipRects.unitY2 - keen->clipRects.unitY1) / 2 + keen->clipRects.unitY1) >> 8;

	switch (TI_ForeMisc(CA_TileAtPos(midTileX, midTileY, 1)))
	{

	case MISCFLAG_TELEPORT:
		CK_AnimateMapTeleporter(midTileX, midTileY);
		break;

	case MISCFLAG_LEFTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, 0);
		break;

	case MISCFLAG_RIGHTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, -1);
		break;
	}
}

void CK5_MapKeenTeleSpawn(int tileX, int tileY)
{

	ck_keenObj->type = 2;
	ck_keenObj->posX = RF_TileToUnit(tileX);
	ck_keenObj->posY = RF_TileToUnit(tileY);
	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->zLayer = 1;
	ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;
	ck_keenObj->user1 = 6;
	ck_keenObj->user2 = 3;
	ck_keenObj->user3 = 0;
	ck_keenObj->gfxChunk = 244;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK_ACT_MapKeenStart"));
}

//this is a move proc.. closes doors on keen

#if 0

MapKeenElevator(CK_object * obj)
{

	int var2, s, var4;

	ck_nextY = obj->yDirection * 64 * CK_GetTicksPerFrame();
	if (obj->posX != obj->user2)
	{
		ck_nextX = obj->xDirection * 12 * CK_GetTicksPerFrame();
		if (obj->xDirection == IN_motion_Right && ck_nextX + obj->posX > obj->user2 ||
				obj->xDirection == IN_motion_Left && ck_nextX + obj->posX < obj->user2)
		{
			ck_nextX = obj->user2 - obj->posX;
		}
	}
	// draw the doors closing
	for (var2 = 0; var2 <= 5; var2++)
	{
		for (var4 = 0; var4 < 2; var4++)
		{
			for (s =0; s < 2; s++)
			{
				var10[var4 * 2 + s] = CA_TileAtPos(var2 * 2 + s, var4, 1);
			}
			RF_ReplaceTiles(&var10, 1, var6, var8 - 2, 2, 2);
			RF_Refresh();
			VL_WaitVBL(8);

		}
	}

	for (var4 = 0; var4 < 32; var4++)
	{
		obj->posY += 8;
		obj->frame = ((var4 / 4) % 3) + 0xFB;
		// RF_PlaceSprite(&obj->int35, obj->posX,obj->posY,obj->frame,0,obj->zLayer);
	}

	obj->clipping = 1;
	return;
}
#endif

void CK5_MapKeenElevator(CK_object *keen)
{
	int tileX, tileY;

	// Move keen in the Y direction
	ck_nextY = keen->yDirection * 64 * SD_GetSpriteSync();

	if (keen->posX != keen->user2)
	{
		ck_nextX = keen->xDirection * 12 * SD_GetSpriteSync();
		if ((keen->xDirection == IN_motion_Right && ck_nextX + keen->posX > keen->user2) ||
			(keen->xDirection == IN_motion_Left && ck_nextX + keen->posX < keen->user2))
		{
			ck_nextX = keen->user2 - keen->posX;
		}
	}

	//1D776
	// Update hitbox
	keen->clipRects.unitX1 = keen->posX + ck_nextX;
	keen->clipRects.unitX2 = keen->clipRects.unitX1 + 0xFF;

	keen->clipRects.unitY1 = keen->posY + ck_nextY;
	keen->clipRects.unitY2 = keen->clipRects.unitY1 + 0xFF;

	// If keen has not yet hit the Y destination, keep moving
	if (keen->yDirection == IN_motion_Down)
	{
		if ((uint16_t)(keen->posY + ck_nextY) < (uint16_t)keen->user1)
			return;
	}
	else
	{
		if ((uint16_t)(keen->posY + ck_nextY) > (uint16_t)keen->user1)
			return;
	}

	// 1D7C5
	// Arrived at destination; turn travelling keen back into normal map keen
	ck_nextX = ck_nextY = 0;
	keen->posX = keen->user2;
	keen->posY = keen->user1;
	keen->zLayer = 1;
	keen->user1 = 4; // Keen faces south
	keen->user2 = 3;
	keen->user3 = 0;
	ck_keenObj->xDirection = ck_keenObj->yDirection = IN_motion_None;
	keen->currentAction = CK_GetActionByName("CK_ACT_MapKeenStart");
	keen->gfxChunk = 0xFD;
	keen->clipped = CLIP_normal;
	tileX = keen->posX >> 8;
	tileY = keen->posY >> 8;
	CK_MapCamera(keen);
	// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
	VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);

	//Draw the scorebox
	CK_UpdateScoreBox(ck_scoreBoxObj);

	RF_Refresh();
	VL_Present();
	RF_Refresh();

	keen->posY -= 0x100;
	RF_AddSpriteDraw(&keen->sde, keen->posX, keen->posY, keen->gfxChunk, false, keen->zLayer);
	SD_PlaySound(CK_SOUNDNUM(SOUND_UNKNOWN63));

	// Animate the elevator operation
	uint16_t tile_array[4];
	for (int frame = 0; frame <= 5; frame++)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				tile_array[y * 2 + x] = CA_TileAtPos(frame * 2 + x, y, 1);
			}
		}
		RF_ReplaceTiles(tile_array, 1, tileX, tileY - 2, 2, 2);
		RF_Refresh();
		//VL_DelayTics(2);
		VL_DelayTics(8); // Simulate 8 calls to VL_WaitVBL();
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	// Draw keen walking out of elevator
	for (int frame = 0; frame < 32; frame++)
	{
		keen->posY += 8;
		keen->gfxChunk = 0xFB + (frame / 4) % 3;
		RF_AddSpriteDraw(&keen->sde, keen->posX, keen->posY, keen->gfxChunk, false, keen->zLayer);
		RF_Refresh();
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	keen->clipped = CLIP_normal;
}

/*
 * Keen enters an elevator
 */
void CK5_AnimateMapElevator(int tileX, int tileY, int dir)
{
	int unitX, unitY;
	int timer;
	int ticsx2;

	unitX = RF_TileToUnit(tileX);
	unitY = RF_TileToUnit(tileY);

	for (timer = 0; timer < 130;)
	{
		// NOTE: this function in omnispeak will calculate the new tile clipRects
		// Here, Vanilla keen calls a function which does not calculate new clipRects
		// But this is the only time that function is ever called,
		// so we can just call CK_ResetClipRects in this spot

		CK_ResetClipRects(ck_keenObj);
		CK_MapCamera(ck_keenObj);

		//Draw the scorebox
		CK_UpdateScoreBox(ck_scoreBoxObj);

		// Draw screen and delay 1/35th of a second
		RF_Refresh();
		// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
		VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);
		//VL_DelayTics(2);
		//CK_SetTicsPerFrame();
		VL_Present();

		ticsx2 = SD_GetSpriteSync() * 2;
		timer += SD_GetSpriteSync();

		// If keen arrives at the target, then he's done walking
		if (ck_keenObj->posX == unitX && ck_keenObj->posY == unitY)
			break;

		// Move Keen closer to the target on every loop
		if (ck_keenObj->posY < unitY)
		{
			ck_keenObj->posY += ticsx2;
			if (ck_keenObj->posY > unitY)
				ck_keenObj->posY = unitY;
		}
		else if (ck_keenObj->posY > unitY)
		{
			ck_keenObj->posY -= ticsx2;
			if (ck_keenObj->posY < unitY)
				ck_keenObj->posY = unitY;
		}

		if (ck_keenObj->posX < unitX)
		{
			ck_keenObj->posX += ticsx2;
			if (ck_keenObj->posX > unitX)
				ck_keenObj->posX = unitX;
		}
		else if (ck_keenObj->posX > unitX)
		{
			ck_keenObj->posX -= ticsx2;
			if (ck_keenObj->posX < unitX)
				ck_keenObj->posX = unitX;
		}

		// Draw Keen walking into target
		// TODO: Is that done by the original code?
		ck_keenObj->gfxChunk = ((SD_GetTimeCount() >> 3) % 3) + 0xF8;
		RF_AddSpriteDraw(&ck_keenObj->sde, ck_keenObj->posX, ck_keenObj->posY, ck_keenObj->gfxChunk, false, ck_keenObj->zLayer);
	}

	SD_PlaySound(CK_SOUNDNUM(SOUND_UNKNOWN63));

	// Draw the elevator operation
	uint16_t tile_array[4];
	for (int frame = 5; frame >= 0; frame--)
	{
		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				tile_array[y * 2 + x] = CA_TileAtPos(frame * 2 + x, y, 1);
			}
		}
		RF_ReplaceTiles(tile_array, 1, tileX + dir, tileY - 1, 2, 2);
		RF_Refresh();
		//VL_DelayTics(2);
		VL_DelayTics(8); // Simulate 8 calls to VL_WaitVBL();
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	// Keen is now behind the elevator door, so don't draw him
	RF_RemoveSpriteDraw(&ck_keenObj->sde);

	// Get the destination stored in the info plane
	// and store X in map units in user1; Y in user 2
	int dest_tile = CA_TileAtPos(tileX, tileY, 2);
	ck_keenObj->user2 = dest_tile / 256 * 256;
	ck_keenObj->user1 = RF_TileToUnit((dest_tile & 0x007F) + 1);

	// Move keen based on the relative location of the target
	if ((uint16_t)ck_keenObj->user1 < (uint16_t)ck_keenObj->posY)
		ck_keenObj->yDirection = IN_motion_Up;
	else
		ck_keenObj->yDirection = IN_motion_Down;

	if ((uint16_t)ck_keenObj->user2 < (uint16_t)ck_keenObj->posX)
		ck_keenObj->xDirection = IN_motion_Left;
	else
		ck_keenObj->xDirection = IN_motion_Right;

	// Keen is going to become invisible and "fly" to the destination
	ck_keenObj->clipped = CLIP_not;
	ck_keenObj->currentAction = CK_GetActionByName("CK5_ACT_MapKeenElevator");

	// Note: This should have been added
	// ck_keenObj->user3 = 1;
}

/*
 * Setup all of the functions in this file.
 */
void CK5_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK5_MapKeenElevator", &CK5_MapKeenElevator);
}
