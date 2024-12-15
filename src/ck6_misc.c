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

#include <stdio.h>
#include "id_ca.h"
#include "id_fs.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_vh.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck6_ep.h"

CK_EpisodeDef ck6_episode = {
	EP_CK6,
	"CK6",
	"EPISODE.CK6",
	&CK6_ScanInfoLayer,
	&CK6_IsPresent,
};

void CK6_SetupFunctions()
{
	CK6_Obj1_SetupFunctions();
	CK6_Obj2_SetupFunctions();
	CK6_Obj3_SetupFunctions();
	CK6_Map_SetupFunctions();
}

// Check if all the game files are present.
bool CK6_IsPresent()
{
	// User-provided files
	if (!FS_IsKeenFilePresent("EGAGRAPH.CK6"))
		return false;
	if (!FS_IsKeenFilePresent("GAMEMAPS.CK6"))
		return false;
	if (!FS_IsKeenFilePresent("AUDIO.CK6"))
		return false;

	// Omnispeak-provided files
	if (!FS_IsOmniFilePresent("EGAHEAD.CK6"))
		return false;
	if (!FS_IsOmniFilePresent("EGADICT.CK6"))
		return false;
	if (!FS_IsOmniFilePresent("GFXINFOE.CK6"))
		return false;
	if (!FS_IsOmniFilePresent("MAPHEAD.CK6"))
		return false;
	// Map header file may include the tile info
	//if (!FS_IsOmniFilePresent("TILEINFO.CK6"))
	//	return false;
	if (!FS_IsOmniFilePresent("AUDIODCT.CK6"))
		return false;
	if (!FS_IsOmniFilePresent("AUDIOHHD.CK6"))
		return false;
	if (!FS_IsOmniFilePresent("AUDINFOE.CK6"))
		return false;

	if (!FS_IsOmniFilePresent("ACTION.CK6"))
		return false;

	// We clearly have all of the required files.
	return true;
}

void CK6_ScanInfoLayer()
{
	CA_ClearLumps();

	int mapW = CA_MapHeaders[ca_mapOn]->width;
	int mapH = CA_MapHeaders[ca_mapOn]->height;

	for (int y = 0; y < mapH; ++y)
	{
		for (int x = 0; x < mapW; ++x)
		{
			int infoValue = CA_TileAtPos(x, y, 2);
			switch (infoValue)
			{
			case 1:
				CK_SpawnKeen(x, y, 1);
				CK_DemoSignSpawn();
				CA_MarkGrChunk(CK_CHUNKNUM(SPR_SCOREBOX));
				CA_MARKLUMP(LUMP_KEEN);
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				CA_MarkGrChunk(CK_CHUNKNUM(SPR_SCOREBOX));
				CA_MARKLUMP(LUMP_KEEN);
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[175] |= ca_levelbit;
				CK_SpawnMapKeen(x, y);
				CA_MARKLUMP(LUMP_MAPKEEN);
				break;

			// Bloogs
			case 6:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 5:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 4:
				CA_MARKLUMP(LUMP_BLOOG);
				CK6_SpawnBloog(x, y);
				break;

			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			{
				int color = (infoValue - 7) % 4;
				CA_MarkLumpNeeded(CK_INT(LUMP_RBLOOGLET, -color) + color);
				if (infoValue > 10)
					CA_MARKLUMP(LUMP_KEYGEMS);
				CK6_SpawnBlooglet(x, y, infoValue - 7);
				break;
			}

			case 15:
			case 16:
				CK6_SpawnMapCliff(x, y, infoValue - 15);
				break;

			// Molly (for some reason, values 21, 22, and 23 also work.
			// (use 24 in-game)
			case 21:
			case 22:
			case 23:
			case 24:
				CA_MARKLUMP(LUMP_MOLLY);
				CK6_SpawnMolly(x, y);
				break;

			// Fleex
			case 20:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 19:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 18:
				CA_MARKLUMP(LUMP_FLEEX);
				CK6_SpawnFleex(x, y);
				break;

			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				RF_SetScrollBlock(x, y, false);
				break;

				// Platforms
			case 27:
			case 28:
			case 29:
			case 30:
				CK_SpawnAxisPlatform(x, y, infoValue - 27, false);
				CA_MARKLUMP(LUMP_PLATFORM);
				break;
			case 32:
				CK_SpawnFallPlat(x, y);
				CA_MARKLUMP(LUMP_PLATFORM);
				break;

			case 33:
				if (ck_gameState.difficulty > D_Easy)
					break;
			case 34:
				if (ck_gameState.difficulty > D_Normal)
					break;
			case 35:
				CK_SpawnStandPlatform(x, y);
				CA_MARKLUMP(LUMP_PLATFORM);
				break;

			case 36:
			case 37:
			case 38:
			case 39:
				CK_SpawnGoPlat(x, y, infoValue - 36, false);
				CA_MARKLUMP(LUMP_PLATFORM);
				CA_MARKLUMP(LUMP_BIPSQUISHED);
				break;
			case 40:
				CK_SneakPlatSpawn(x, y);
				CA_MARKLUMP(LUMP_PLATFORM);
				break;

			// Bobbas
			case 43:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 42:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 41:
				CA_MARKLUMP(LUMP_BOBBA);
				CK6_SpawnBobba(x, y);
				break;

			case 44:
			case 45:
				CK6_SpawnSatelliteLoading(x, y, infoValue - 44);
				break;

			// Nospike
			case 49:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 48:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 47:
				CA_MARKLUMP(LUMP_NOSPIKE);
				CK6_SpawnNospike(x, y);
				break;

			// Gik
			case 52:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 51:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 50:
				CA_MARKLUMP(LUMP_GIK);
				CK6_SpawnGik(x, y);
				break;

				// Turrets
			case 53:
			case 54:
			case 55:
			case 56:
				CA_MARKLUMP(LUMP_LASER);
				CK_TurretSpawn(x, y, infoValue - 53);
				break;

			case 69:
				// Spawn extra stunner if Keen has low ammo
				if (ck_gameState.numShots >= 5)
					break;
				infoValue = 68;
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63:
			case 64:
			case 65:
			case 66:
			case 67:
			case 68:
				CK_SpawnItem(x, y, infoValue - 57);
				CA_MarkLumpNeeded(CK_INTELEMENT(ck_itemLumps, infoValue - 57));
				break;

			// Orbatrices
			case 72:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 71:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 70:
				CA_MARKLUMP(LUMP_ORBATRIX);
				CK6_SpawnOrbatrix(x, y);
				break;

			// Bip
			case 75:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 74:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 73:
				CA_MARKLUMP(LUMP_BIP);
				CA_MARKLUMP(LUMP_BIPSQUISHED);
				CA_MARKLUMP(LUMP_BIPSHIP);
				CK6_SpawnBipship(x, y);
				break;

			// Flects
			case 78:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 77:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 76:
				CA_MARKLUMP(LUMP_FLECT);
				CK6_SpawnFlect(x, y);
				break;

			// Blorbs
			case 81:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 80:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 79:
				CA_MARKLUMP(LUMP_BLORB);
				CK6_SpawnBlorb(x, y);
				break;

			// Ceilicks
			case 84:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 83:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 82:
				CA_MARKLUMP(LUMP_CEILICK);
				CK6_SpawnCeilick(x, y);
				break;

			// Bloogguards
			case 87:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 86:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 85:
				CA_MARKLUMP(LUMP_BLOOGGUARD);
				CK6_SpawnBloogguard(x, y);
				break;

			// Grabbiter
			case 88:
				CK6_SpawnGrabbiter(x, y);
				break;

			// Satellite
			case 89:
				CK6_SpawnSatellite(x, y);
				break;

			// Story Items
			case 99:
				CA_MARKLUMP(LUMP_HOOK);
				CK6_SpawnRope(x, y);
				break;
			case 100:
				CA_MARKLUMP(LUMP_SANDWICH);
				CK6_SpawnSandwich(x, y);
				break;
			case 101:
				CA_MARKLUMP(LUMP_PASSCARD);
				CK6_SpawnPasscard(x, y);
				break;

			// Babobbas
			case 104:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 103:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 102:
				CA_MARKLUMP(LUMP_BABOBBA);
				CK6_SpawnBabobba(x, y);
				break;

			// Rocketship
			case 105:
			case 106:
				CK6_SpawnRocket(x, y, infoValue - 105);
				break;
			}
		}
	}

	for (CK_object *obj = ck_keenObj; obj != NULL; obj = obj->next)
	{
		if (obj->active != OBJ_ALWAYS_ACTIVE)
			obj->active = OBJ_INACTIVE;
	}

	CA_MarkAllLumps();
}

void CK6_ToggleBigSwitch(CK_object *obj, bool dir)
{

	// Replace switch tiles
	int ty = dir ? obj->clipRects.tileY2 : obj->clipRects.tileY1 - 2;
	int tx = obj->clipRects.tileX1 - 1;

	uint16_t *infoTile = CA_TilePtrAtPos(tx + 1, ty + 1, 2);

	while (!*infoTile)
	{
		tx++;
		infoTile++;
	}

	uint16_t *fgTile = CA_TilePtrAtPos(tx, ty, 1);

	uint16_t tile_array[6];
	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			tile_array[2 * y + x] = *fgTile + TI_ForeAnimTile(*fgTile);
			fgTile++;
		}

		fgTile += CA_GetMapWidth() - 2;
	}

	RF_ReplaceTiles(tile_array, 1, tx, ty, 2, 3);

	// Apply the switch effect
	infoTile = CA_TilePtrAtPos(tx + 1, ty + 1, 2);
	int destX = *infoTile >> 8;
	int destY = *infoTile & 0xFF;
	SD_PlaySound(CK_SOUNDNUM(SOUND_KEENOUTOFAMMO));

	infoTile = CA_TilePtrAtPos(destX, destY, 2);

	if (*infoTile >= 0x5B && *infoTile < 0x5B + 8)
	{
		// Toggle a goplat arrow
		static uint16_t infoPlaneInverses[8] = {2, 3, 0, 1, 6, 7, 4, 5};
		*infoTile = infoPlaneInverses[(*infoTile - 0x5B)] + 0x5B;
	}
	else
	{
		fgTile = CA_TilePtrAtPos(destX, destY, 1);
		int miscValue = TI_ForeMisc(*fgTile) & 0x7F;

		if (miscValue == MISCFLAG_ACTIVEZAPPER)
		{
			uint16_t start = CA_TileAtPos(0, 0, 1);
			uint16_t mid = CA_TileAtPos(1, 0, 1);
			uint16_t end = CA_TileAtPos(2, 0, 1);

			RF_ReplaceTiles(&start, 1, destX, destY, 1, 1);
			destY++;

			while (TI_ForeMisc(CA_TileAtPos(destX, destY, 1)) == MISCFLAG_DEADLY)
			{
				RF_ReplaceTiles(&mid, 1, destX, destY, 1, 1);
				destY++;
			}

			RF_ReplaceTiles(&end, 1, destX, destY, 1, 1);
		}
		else if (miscValue == MISCFLAG_INACTIVEZAPPER)
		{
			uint16_t start = CA_TileAtPos(3, 0, 1);
			uint16_t mid = CA_TileAtPos(4, 0, 1);
			uint16_t end = CA_TileAtPos(5, 0, 1);

			RF_ReplaceTiles(&start, 1, destX, destY, 1, 1);
			destY++;

			while (TI_ForeMisc(CA_TileAtPos(destX, destY, 1)) != MISCFLAG_INACTIVEZAPPER)
			{
				RF_ReplaceTiles(&mid, 1, destX, destY, 1, 1);
				destY++;
			}

			RF_ReplaceTiles(&end, 1, destX, destY, 1, 1);
		}
		else if (miscValue == MISCFLAG_BRIDGE)
		{
			for (int y = destY; y < destY + 2; ++y)
			{
				for (int x = destX - ((y == destY) ? 0 : 1); x < CA_GetMapWidth(); ++x)
				{
					uint16_t currentTile = CA_TileAtPos(x, y, 1);
					if (!TI_ForeAnimTile(currentTile))
						break;
					uint16_t newTile = currentTile + TI_ForeAnimTile(currentTile);
					RF_ReplaceTiles(&newTile, 1, x, y, 1, 1);
				}
			}
		}
		else
		{
			// Toggle a B block
			*infoTile ^= 0x1F;
		}
	}
}

// Story item dialogues

#define SOUND_STORYITEM 0x2D
void CK6_ShowGetSandwich()
{
	SD_WaitSoundDone();
	SD_PlaySound(CK_SOUNDNUM(SOUND_STORYITEM));
	CA_UpLevel();
	CA_CacheGrChunk(CK_CHUNKNUM(PIC_KEENTALK1));

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), CK_CHUNKNUM(PIC_KEENTALK1));
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);
	US_CPrint(CK_STRING(ck6_str_getSandwich));
	VH_UpdateScreen();

	VL_DelayTics(30); // VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
	ck_gameState.ep.ck6.sandwich = true;
}

void CK6_ShowGetRope()
{
	SD_WaitSoundDone();
	SD_PlaySound(CK_SOUNDNUM(SOUND_STORYITEM));
	CA_UpLevel();
	CA_CacheGrChunk(CK_CHUNKNUM(PIC_KEENTALK1));

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), CK_CHUNKNUM(PIC_KEENTALK1));
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);
	US_CPrint(CK_STRING(ck6_str_getRope));
	VH_UpdateScreen();

	VL_DelayTics(30); // VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
	ck_gameState.ep.ck6.rope = true;
}

void CK6_ShowGetPasscard()
{
	SD_WaitSoundDone();
	SD_PlaySound(CK_SOUNDNUM(SOUND_STORYITEM));
	CA_UpLevel();
	CA_CacheGrChunk(CK_CHUNKNUM(PIC_KEENTALK1));

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), CK_CHUNKNUM(PIC_KEENTALK1));
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 4);
	US_CPrint(CK_STRING(ck6_str_getPasscard));
	VH_UpdateScreen();

	VL_DelayTics(30); // VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
	ck_gameState.ep.ck6.passcard = true;
}
