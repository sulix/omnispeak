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
#include "id_vl.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

CK_EpisodeDef osi_episode = {
	EP_CK5,
	"CK5",
	&CK5_SetupFunctions,
	&CK5_ScanInfoLayer,
	&CK5_DefineConstants,
	&CK5_MapMiscFlagsCheck,
	&CK5_IsPresent,
	/* .activeLimit = */ 6,
	/* .highScoreLevel = */ 19,
	/* .highScoreTopMargin = */ 0x23,
	/* .highScoreLeftMargin = */ 0x28,
	/* .highScoreRightMargin = */ 0x118,
	/* .endSongLevel = */ 14,
	/* .starWarsSongLevel = */ 17,
	/* .lastLevelToMarkAsDone = */ 17,
	/* .objArrayOffset = */ 0x9E6F,
	/* .tempObjOffset = */ 0xBC3B,
	/* .spriteArrayOffset = */ 0xCD50,
	/* .printXOffset = */ 0x9B9F,
	/* .animTilesOffset = */ 0xD4DC,
	/* .animTileSize = */ 4,
	/* .hasCreatureQuestion = */ false,
};

// Contains some keen-5 specific functions.

// Check if all the game files are present.
bool CK5_IsPresent()
{
	// User-provided files
	if (!FS_IsKeenFilePresent("EGAGRAPH.CK5"))
		return false;
	if (!FS_IsKeenFilePresent("GAMEMAPS.CK5"))
		return false;
	if (!FS_IsKeenFilePresent("AUDIO.CK5"))
		return false;

	// Omnispeak-provided files
	if (!FS_IsOmniFilePresent("EGAHEAD.CK5"))
		return false;
	if (!FS_IsOmniFilePresent("EGADICT.CK5"))
		return false;
	if (!FS_IsOmniFilePresent("GFXINFOE.CK5"))
		return false;
	if (!FS_IsOmniFilePresent("MAPHEAD.CK5"))
		return false;
	// Map header file may include the tile info
	//if (!FS_IsOmniFilePresent("TILEINFO.CK5"))
	//	return false;
	if (!FS_IsOmniFilePresent("AUDIODCT.CK5"))
		return false;
	if (!FS_IsOmniFilePresent("AUDIOHHD.CK5"))
		return false;
	if (!FS_IsOmniFilePresent("AUDINFOE.CK5"))
		return false;

	if (!FS_IsOmniFilePresent("ACTION.CK5"))
		return false;

	// We clearly have all of the required files.
	return true;
}

void CK5_PurpleAxisPlatform(CK_object *obj)
{
	uint16_t nextPosUnit, nextPosTile;

	if (ck_nextX || ck_nextY)
	{
		return;
	}
	//TODO: Implement properly.
	ck_nextX = obj->xDirection * 12 * SD_GetSpriteSync();
	ck_nextY = obj->yDirection * 12 * SD_GetSpriteSync();

	if (obj->xDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitX2 + ck_nextX;
		nextPosTile = RF_UnitToTile(nextPosUnit);
		if (obj->clipRects.tileX2 != nextPosTile && CA_TileAtPos(nextPosTile, obj->clipRects.tileY1, 2) == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			ck_nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + ck_nextX;
		nextPosTile = RF_UnitToTile(nextPosUnit);
		if (obj->clipRects.tileX1 != nextPosTile && CA_TileAtPos(nextPosTile, obj->clipRects.tileY1, 2) == 0x1F)
		{
			obj->xDirection = 1;
			//TODO: Change DeltaVelocity
			//CK_PhysUpdateX(obj, 256 - nextPosUnit&255);
			ck_nextX += (256 - nextPosUnit) & 255;
		}
	}
	else if (obj->yDirection == 1)
	{
		nextPosUnit = obj->clipRects.unitY2 + ck_nextY;
		nextPosTile = RF_UnitToTile(nextPosUnit);
		if (obj->clipRects.tileY2 != nextPosTile && CA_TileAtPos(obj->clipRects.tileX1 + 1, nextPosTile, 2) == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile - 2, 2) == 0x1F)
			{
				//Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = -1;
				//TODO: Change DeltaVelocity
				ck_nextY -= (nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + ck_nextY;
		nextPosTile = RF_UnitToTile(nextPosUnit);
		if (obj->clipRects.tileY1 != nextPosTile && CA_TileAtPos(obj->clipRects.tileX1 + 1, nextPosTile, 2) == 0x1F)
		{
			if (CA_TileAtPos(obj->clipRects.tileX1, nextPosTile + 2, 2) == 0x1F)
			{
				// Stop the platform.
				obj->visible = true;
				ck_nextY = 0;
			}
			else
			{
				obj->yDirection = 1;
				//TODO: Change DeltaVelocity
				ck_nextY += 256 - (nextPosUnit & 255);
			}
		}
	}
}

// MISC Keen 5 functions

// Teleporter Lightning Spawn

void CK5_SpawnLightning()
{
	CK_object *new_object;

	// Spawn the top lightning
	new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = RF_TileToUnit(ck_keenObj->clipRects.tileX1) - 0x80;
	new_object->posY = RF_TileToUnit(ck_keenObj->clipRects.tileY2) - 0x500;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LightningH0"));

	// Spawn the vertical lightning that covers keen
	new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = RF_TileToUnit(ck_keenObj->clipRects.tileX1);
	new_object->posY = RF_TileToUnit(ck_keenObj->clipRects.tileY1) - 0x80;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LightningV0"));

	SD_PlaySound(CK_SOUNDNUM(SOUND_UNKNOWN41));
}

// Fuse Explosion Spawn

uint8_t osi_colourFusePalette[] = {
	0x00, 56, 56, 7, 56, 7, 56, 7,
	56, 7, 7, 63, 7, 63, 63, 63, 0x00};

void CK5_SpawnFuseExplosion(int tileX, int tileY)
{
	CK_object *new_object = CK_GetNewObj(true);
	new_object->zLayer = 3;
	new_object->clipped = CLIP_not;
	new_object->type = 24;
	new_object->posX = RF_TileToUnit(tileX - 1);
	new_object->posY = RF_TileToUnit(tileY);
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_FuseExplosion0"));
	SD_PlaySound(CK_SOUNDNUM(SOUND_UNKNOWN52));
	
	if (CK_INT(OSI_FuseColourEffectLevel, 17) == ck_gameState.currentLevel)
		VL_SetPaletteAndBorderColor(osi_colourFusePalette);
}

// Level Ending Object Spawn

void CK5_SpawnLevelEnd(void)
{
	CK_object *new_object = CK_GetNewObj(false);
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_LevelEnd"));
}

// LevelEnd Behaviour
// If in the QED, end the game
// Otherwise, do the Korath Fuse message

void CK5_LevelEnd(CK_object *obj)
{
	ck_gameState.levelState = (ca_mapOn == CK_INT(CK5_QEDLevel, 12)) ? LS_DestroyedQED : LS_KorathFuse;
}

void CK5_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
	CK5_Obj1_SetupFunctions();
	CK5_Obj2_SetupFunctions();
	CK5_Obj3_SetupFunctions();
	OSI_Obj4_SetupFunctions();
	CK5_Map_SetupFunctions();
	CK_ACT_AddFunction("CK5_PurpleAxisPlatform", &CK5_PurpleAxisPlatform);
	CK_ACT_AddFunction("CK5_LevelEnd", &CK5_LevelEnd);
}

// ck_inter.c
uint8_t ck5_starWarsPalette[] = {
	0x00, 0x01, 0x18, 0x1E, 0x1F, 0x1C, 0x06, 0x07,
	0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x00};

uint8_t osi_terminator_palette1[] = {0, 2, 2, 26, 1, 1, 1, 1, 0x11, 0x11, 0x11, 0x11, 0x13, 0x13, 0x13, 0x13, 0};
uint8_t osi_terminator_palette2[] = {0, 2, 2, 26, 1, 1, 1, 1, 0x11, 0x11, 0x11, 0x11, 0x13, 0x13, 0x13, 2, 0};

// ck_keen.c

/*
soundnames ck5_itemSounds[]  = { SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM,
                               SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,
                               SOUND_GOTEXTRALIFE, SOUND_GOTSTUNNER, SOUND_GOTKEYCARD
};
*/
soundnames ck5_itemSounds[] = {19, 19, 19, 19, 8, 8, 8, 8, 8, 8, 17, 9, 55};
uint16_t ck5_itemShadows[] = {232, 232, 232, 232, 195, 196, 197, 198, 199, 200, 201, 202, 209};

// ck_map.c
int ck5_mapKeenFrames[] = {0xF7, 0x106, 0xF4, 0xFD, 0xFA, 0x100, 0xF1, 0x103};

// ck_play.c
int16_t ck5_levelMusic[] = {11, 5, 7, 9, 10, 9, 10, 9, 10, 9, 10, 3, 13, 4, 12, 2, 6, 1, 0, 8};

void CK5_DefineConstants(void)
{
	// We can't remove these constants for now, as pointers to them are
	// compiled in.
	PIC_MENUCARD = CK_CHUNKNUM(PIC_MENUCARD);
	PIC_NEWGAMECARD = CK_CHUNKNUM(PIC_NEWGAMECARD);
	PIC_LOADCARD = CK_CHUNKNUM(PIC_LOADCARD);
	PIC_SAVECARD = CK_CHUNKNUM(PIC_SAVECARD);
	PIC_CONFIGURECARD = CK_CHUNKNUM(PIC_CONFIGURECARD);
	PIC_SOUNDCARD = CK_CHUNKNUM(PIC_SOUNDCARD);
	PIC_MUSICCARD = CK_CHUNKNUM(PIC_MUSICCARD);
	PIC_KEYBOARDCARD = CK_CHUNKNUM(PIC_KEYBOARDCARD);
	PIC_MOVEMENTCARD = CK_CHUNKNUM(PIC_MOVEMENTCARD);
	PIC_BUTTONSCARD = CK_CHUNKNUM(PIC_BUTTONSCARD);
	PIC_JOYSTICKCARD = CK_CHUNKNUM(PIC_JOYSTICKCARD);
	PIC_OPTIONSCARD = CK_CHUNKNUM(PIC_OPTIONSCARD);
	PIC_PADDLEWAR = CK_CHUNKNUM(PIC_PADDLEWAR);
	PIC_DEBUGCARD = CK_CHUNKNUM(PIC_DEBUGCARD);

	// Terminator credit chunks also need setting up here.
	PIC_CREDIT1 = CK_CHUNKNUM(PIC_CREDIT1);
	PIC_CREDIT2 = CK_CHUNKNUM(PIC_CREDIT2);
	PIC_CREDIT3 = CK_CHUNKNUM(PIC_CREDIT3);
	PIC_CREDIT4 = CK_CHUNKNUM(PIC_CREDIT4);

	SPR_SECURITYCARD_1 = CK_CHUNKNUM(SPR_SECURITYCARD_1);
	SPR_GEM_A1 = CK_CHUNKNUM(SPR_GEM_A1);
	SPR_GEM_B1 = CK_CHUNKNUM(SPR_GEM_B1);
	SPR_GEM_C1 = CK_CHUNKNUM(SPR_GEM_C1);
	SPR_GEM_D1 = CK_CHUNKNUM(SPR_GEM_D1);
	SPR_100_PTS1 = CK_CHUNKNUM(SPR_100_PTS1);
	SPR_200_PTS1 = CK_CHUNKNUM(SPR_200_PTS1);
	SPR_500_PTS1 = CK_CHUNKNUM(SPR_500_PTS1);
	SPR_1000_PTS1 = CK_CHUNKNUM(SPR_1000_PTS1);
	SPR_2000_PTS1 = CK_CHUNKNUM(SPR_2000_PTS1);
	SPR_5000_PTS1 = CK_CHUNKNUM(SPR_5000_PTS1);
	SPR_1UP1 = CK_CHUNKNUM(SPR_1UP1);
	SPR_STUNNER1 = CK_CHUNKNUM(SPR_STUNNER1);

	TEXT_HELPMENU = CK_CHUNKNUM(TEXT_HELPMENU);
	TEXT_CONTROLS = CK_CHUNKNUM(TEXT_CONTROLS);
	TEXT_STORY = CK_CHUNKNUM(TEXT_STORY);
	TEXT_ABOUTID = CK_CHUNKNUM(TEXT_ABOUTID);
	TEXT_END = CK_CHUNKNUM(TEXT_END);
	TEXT_SECRETEND = CK_CHUNKNUM(TEXT_SECRETEND);
	TEXT_ORDER = CK_CHUNKNUM(TEXT_ORDER);

	CAMEIN_MUS = 0;
	LITTLEAMPTON_MUS = 1;
	THEICE_MUS = 2;
	SNOOPIN_MUS = 3;
	BAGPIPES_MUS = 4;
	WEDNESDAY_MUS = 5;
	ROCKNOSTONE_MUS = 6;
	OUTOFBREATH_MUS = 7;
	SHIKADIAIRE_MUS = 8;
	DIAMONDS_MUS = 9;
	TIGHTER_MUS = 10;
	ROBOREDROCK_MUS = 11;
	FANFARE_MUS = 12;
	BRINGEROFWAR_MUS = 13;
	LASTMUSTRACK = 14;

	// ck_inter.c
	ck_starWarsPalette = ck5_starWarsPalette;
	ck_terminator_palette1 = osi_terminator_palette1;
	ck_terminator_palette2 = osi_terminator_palette2;

	// ck_keen.c
	ck_itemSounds = ck5_itemSounds;
	ck_itemShadows = ck5_itemShadows;

	// ck_map.c
	ck_mapKeenFrames = ck5_mapKeenFrames;

	// ck_play.c
	ck_levelMusic = ck5_levelMusic;
}


void CK5_FixupVolteSpot(int tileX, int tileY)
{
	int dir;
	if (CA_TileAtPos(tileX - 1, tileY, 2) == 0x5C)
		dir = 1;
	else if (CA_TileAtPos(tileX + 1, tileY, 2) == 0x5E)
		dir = 3;
	else if (CA_TileAtPos(tileX, tileY - 1, 2) == 0x5D)
		dir = 2;
	else if (CA_TileAtPos(tileX, tileY + 1, 2) == 0x5B)
		dir = 0;
	else
		Quit("Volte did not spawn at bad spot!");

	CA_SetTileAtPos(tileX, tileY, 2, dir + 0x5B);
}

/*
 * Spawn an enemy projectile
 * Note that the behaviour is slightly different from DOS Keen
 * DOS Keen SpawnEnemyShot returns 0 if shot is spawned, or -1 otherwise
 * omnispeak CK5_SpawnEnemyShot returns pointer if succesful, NULL otherwise
 */

CK_object *CK5_SpawnEnemyShot(int posX, int posY, CK_action *action)
{
	CK_object *new_object = CK_GetNewObj(true);

	if (!new_object)
		return NULL;

	new_object->posX = posX;
	new_object->posY = posY;
	new_object->type = CT5_EnemyShot;
	new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
	CK_SetAction(new_object, action);

	if (CK_NotStuckInWall(new_object))
	{
		return new_object;
	}
	else
	{
		CK_RemoveObj(new_object);
		return NULL;
	}
}

void CK5_OpenMapTeleporter(int tileX, int tileY)
{
	uint16_t tile_array[4];
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			tile_array[y * 2 + x] = CA_TileAtPos(5 * 2 + x, y, 1);
		}
	}
	RF_ReplaceTiles(tile_array, 1, tileX, tileY, 2, 2);
}

void CK5_CloseMapTeleporter(int tileX, int tileY)
{
	uint16_t tile_array[4];
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 2; x++)
		{
			tile_array[y * 2 + x] = CA_TileAtPos(x, y, 1);
		}
	}
	RF_ReplaceTiles(tile_array, 1, tileX, tileY, 2, 2);
}

// TODO: Cache stuff here instead of spawner handlers
#define MAXLUMPS 46
static bool ck5_lumpsNeeded[MAXLUMPS];
typedef enum
{
	Lump_0 = 0,
	Lump_Keen = 1,
	Lump_Candy = 2,
	Lump_Marshmallow = 3,
	Lump_Cola = 4,
	Lump_Stix = 5,
	Lump_SugarStoopies = 6,
	Lump_BagOSugar = 7,
	Lump_VitalinKeg = 8,
	Lump_Gems = 9,
	Lump_Stunner = 10,
	Lump_PinkShot = 11,
	Lump_MapKeen = 12,
	Lump_ShikadiMaster = 13,
	Lump_Shikadi = 14,
	Lump_Shocksund = 15,
	Lump_Sphereful = 16,
	Lump_Sparky = 17,
	Lump_Mine = 18,
	Lump_Slicestar = 19,
	Lump_RoboRed = 20,
	Lump_Spirogrip = 21,
	Lump_Ampton = 22,
	Lump_VolteFace = 23,
	Lump_PurplePlat = 24,
	Lump_Spindred = 25,
	Lump_Shelley = 26,
	Lump_RedPlat = 27,
	Lump_UnusedRedPlat = 28,
	Lump_Keycard = 29,
	Lump_Korath = 30,
	Lump_QEDFuse = 31,
	Lump_QEDExplosion = 32,
	Lump_Teleporter = 33,
	Lump_BigAmpton = 34,
	Lump_RoboBlue = 35,
	Lump_RoboGreen = 36,
	Lump_RoboYellow = 37,
	Lump_SparkyJr = 38,
	Lump_Sparky2 = 39,
	Lump_Sparky3 = 40,
	Lump_Vlorg = 41
} CK_Lumptype;

static int16_t ck5_itemLumps[] =
	{
		Lump_Gems,
		Lump_Gems,
		Lump_Gems,
		Lump_Gems,
		Lump_Candy,
		Lump_Marshmallow,
		Lump_Cola,
		Lump_Stix,
		Lump_SugarStoopies,
		Lump_BagOSugar,
		Lump_VitalinKeg,
		Lump_Stunner};

static int16_t ck5_lumpStarts[MAXLUMPS] =
	{
		0,
		0x6C,
		0xD2,
		0xD4,
		0xD6,
		0xD8,
		0xDA,
		0xDC,
		0xDE,
		0xE0,
		0xE9,
		0xEC,
		0xF2,
		0x130,
		0x140,
		0x151,
		0x166,
		0x16E,
		0x17A,
		0x181,
		0x183,
		0x189,
		0x195,
		0x1A1,
		0x1A6,
		0x1A8,
		0x1AC,
		0x1BE,
		0x1BF,
		0xCF,
		0x126,
		0x11B,
		0x11E,
		0x122,
		448,
		460,
		469,
		482,
		492,
		501,
		521,
		530
};

static int16_t ck5_lumpEnds[MAXLUMPS] =
	{
		0,
		0xCE,
		0xD3,
		0xD5,
		0xD7,
		0xD9,
		0xDB,
		0xDD,
		0xDF,
		0xE8,
		0xEA,
		0xF1,
		0x11A,
		0x13F,
		0x150,
		0x165,
		0x16D,
		0x179,
		0x180,
		0x182,
		0x188,
		0x194,
		0x1A0,
		0x1A5,
		0x1A7,
		0x1AB,
		0x1BD,
		0x1BE,
		0x1BF,
		0xD1,
		0x12F,
		0x11D,
		0x121,
		0x125,
		459,
		468,
		481,
		491,
		500,
		520,
		529,
		537
};

extern int osi_numMasterTeleDests;

void CK5_ScanInfoLayer()
{
	osi_numMasterTeleDests = 0;
	//TODO: Work out where to store current map number, etc.
	int mapW = CA_MapHeaders[ca_mapOn]->width;
	int mapH = CA_MapHeaders[ca_mapOn]->height;

	ck_gameState.ep.ck5.fusesRemaining = 0;

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
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				ck5_lumpsNeeded[Lump_Keen] = true;
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				ck5_lumpsNeeded[Lump_Keen] = true;
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				ck5_lumpsNeeded[Lump_MapKeen] = true;
				if (ck_gameState.levelState != LS_TeleportToKorath)
					CK_SpawnMapKeen(x, y);
				break;

			case 6:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 5:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 4:
				CK5_SpawnSparky(x, y);
				ck5_lumpsNeeded[Lump_Sparky] = true;
				break;

			case 9:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 8:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 7:
				CK5_SpawnMine(x, y);
				//CK5_SpawnVlorg(x, y);
				ck5_lumpsNeeded[Lump_Mine] = true;
				break;

			case 12:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 11:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 10:
				CK5_SpawnSlice(x, y, CD_north);
				ck5_lumpsNeeded[Lump_Slicestar] = true;
				break;

			case 15:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 14:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 13:
				CK5_SpawnRobo(x, y);
				ck5_lumpsNeeded[Lump_RoboRed] = true;
				break;

			case 18:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 17:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 16:
				CK5_SpawnSpirogrip(x, y);
				ck5_lumpsNeeded[Lump_Spirogrip] = true;
				break;

			case 21:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 20:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 19:
				CK5_SpawnSliceDiag(x, y);
				ck5_lumpsNeeded[Lump_Slicestar] = true;
				break;

			case 24:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 23:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 22:
				CK5_SpawnSlice(x, y, CD_east);
				ck5_lumpsNeeded[Lump_Slicestar] = true;
				break;

			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				//RF_SetScrollBlock(x, y, false);
				if (ck_gameState.levelState == LS_TeleportToKorath)
					CK5_MapKeenTeleSpawn(x, y);
				break;
			case 27:
			case 28:
			case 29:
			case 30:
				CK_SpawnAxisPlatform(x, y, infoValue - 27, false);
				ck5_lumpsNeeded[Lump_RedPlat] = true;
				break;
			case 32:
				CK_SpawnFallPlat(x, y);
				ck5_lumpsNeeded[Lump_RedPlat] = true;
				break;

			case 33:
				if (ck_gameState.difficulty > D_Easy)
					break;
			case 34:
				if (ck_gameState.difficulty > D_Normal)
					break;
			case 35:
				CK_SpawnStandPlatform(x, y);
				ck5_lumpsNeeded[Lump_RedPlat] = true;
				break;

			case 36:
			case 37:
			case 38:
			case 39:
				CK_SpawnGoPlat(x, y, infoValue - 36, false);
				ck5_lumpsNeeded[Lump_RedPlat] = true;
				break;
			case 40:
				CK_SneakPlatSpawn(x, y);
				ck5_lumpsNeeded[Lump_RedPlat] = true;
				break;
			case 41:
				if (ck_gameState.currentLevel == CK_INT(CK5_QEDLevel, 12))
				{
					ck_gameState.ep.ck5.fusesRemaining = 4;
					CK5_QEDSpawn(x, y);
				}
				else
				{
					ck_gameState.ep.ck5.fusesRemaining++;
				}
				ck5_lumpsNeeded[Lump_QEDFuse] = true;
				break;
			case 44:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 43:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 42:
				CK5_SpawnAmpton(x, y);
				ck5_lumpsNeeded[Lump_Ampton] = true;
				break;

			case 53:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 49:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 45:
				CK_TurretSpawn(x, y, 0);
				ck5_lumpsNeeded[Lump_PinkShot] = true;
				break;

			case 54:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 50:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 46:
				CK_TurretSpawn(x, y, 1);
				ck5_lumpsNeeded[Lump_PinkShot] = true;
				break;

			case 55:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 51:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 47:
				CK_TurretSpawn(x, y, 2);
				ck5_lumpsNeeded[Lump_PinkShot] = true;
				break;

			case 56:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 52:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 48:
				CK_TurretSpawn(x, y, 3);
				ck5_lumpsNeeded[Lump_PinkShot] = true;
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
				ck5_lumpsNeeded[ck5_itemLumps[infoValue - 57]] = true;
				break;
			case 70:
				CK_SpawnItem(x, y, infoValue - 58); // Omegamatic Keycard
				ck5_lumpsNeeded[Lump_Keycard] = true;
				break;

			case 73:
				if (ck_gameState.difficulty < D_Hard)
				{
					CK5_FixupVolteSpot(x, y);
					break;
				}
			case 72:
				if (ck_gameState.difficulty < D_Normal)
				{
					CK5_FixupVolteSpot(x, y);
					break;
				}
			case 71:
				CK5_SpawnVolte(x, y);
				ck5_lumpsNeeded[Lump_VolteFace] = true;
				break;

			case 76:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 75:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 74:
				CK5_SpawnShelly(x, y);
				ck5_lumpsNeeded[Lump_Shelley] = true;
				break;

			case 79:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 78:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 77:
				CK5_SpawnSpindred(x, y);
				ck5_lumpsNeeded[Lump_Spindred] = true;
				break;

			case 80:
			case 81:
			case 82:
			case 83:
				CK_SpawnGoPlat(x, y, infoValue - 80, true);
				ck5_lumpsNeeded[Lump_PurplePlat] = true;
				break;
			case 84:
			case 85:
			case 86:
			case 87:
				CK_SpawnAxisPlatform(x, y, infoValue - 84, true);
				ck5_lumpsNeeded[Lump_PurplePlat] = true;
				break;

			case 90:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 89:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 88:
				CK5_SpawnMaster(x, y);
				ck5_lumpsNeeded[Lump_ShikadiMaster] = true;
				break;

			case 101:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 100:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 99:
				CK5_SpawnShikadi(x, y);
				ck5_lumpsNeeded[Lump_Shikadi] = true;
				break;

			case 104:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 103:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 102:
				CK5_SpawnShocksund(x, y);
				ck5_lumpsNeeded[Lump_Shocksund] = true;
				break;

			case 107:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 106:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 105:
				CK5_SpawnSphereful(x, y);
				ck5_lumpsNeeded[Lump_Sphereful] = true;
				break;

			case 124:
				CK5_SpawnKorath(x, y);
				ck5_lumpsNeeded[Lump_Korath] = true;
				break;
			case 125:
				// TODO: Signal about teleportation (caching)
				ck5_lumpsNeeded[Lump_Teleporter] = true;
				break;
			case 134:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 133:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 132:
				CK5_SpawnBigAmpton(x, y);
				ck5_lumpsNeeded[Lump_BigAmpton] = true;
				break;
			case 137:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 136:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 135:
				ck5_lumpsNeeded[Lump_Vlorg] = true;
				CK5_SpawnVlorg(x, y);
				break;
			case 0x83:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x82:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x81:
				CK5_SpawnRoboGreen(x, y);
				ck5_lumpsNeeded[Lump_RoboGreen] = true;
				break;
			case 0x80:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x7F:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x7E:
				CK5_SpawnRoboBlue(x, y);
				ck5_lumpsNeeded[Lump_RoboBlue] = true;
				break;
			case 0x8C:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x8B:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x8A:
				CK5_SpawnRoboYellow(x, y);
				ck5_lumpsNeeded[Lump_RoboYellow] = true;
				break;
			case 0x8F:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x8E:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x8D:
				CK5_SpawnSparkyJr(x, y);
				ck5_lumpsNeeded[Lump_SparkyJr] = true;
				break;
			case 0x97:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x96:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x95:
				CK5_SpawnSparky2(x, y);
				ck5_lumpsNeeded[Lump_Sparky2] = true;
				break;
			case 0x9A:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x99:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x98:
				CK5_SpawnSparky3(x, y);
				ck5_lumpsNeeded[Lump_Sparky3] = true;
				break;
			case 0xA1:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 0x9E:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 0x9C:
				osi_numMasterTeleDests++;
				break;
			}
		}
	}

	for (CK_object *obj = ck_keenObj; obj != NULL; obj = obj->next)
	{
		if (obj->active != OBJ_ALWAYS_ACTIVE)
			obj->active = OBJ_INACTIVE;
	}
	// TODO: Some more stuff (including opening elevator after breaking fuses)

	// Mark all of the chunks for needed lumps.
	for (int i = 0; i < MAXLUMPS; i++)
		if (ck5_lumpsNeeded[i])
			for (int j = ck5_lumpStarts[i]; j <= ck5_lumpEnds[i]; j++)
				CA_MarkGrChunk(j);

	if (ck_gameState.currentLevel == 0)
	{
		int keenYTilePos = ck_keenObj->posY >> 8;

		// The top of the lower shaft is opened if you're above its entrance or
		// on Korath III.
		if (ck_gameState.levelsDone[CK_INT(CK5_MapTeleLevelNeeded1, 4)] &&
			ck_gameState.levelsDone[CK_INT(CK5_MapTeleLevelNeeded2, 6)] &&
			ck_gameState.levelsDone[CK_INT(CK5_MapTeleLevelNeeded3, 8)] &&
			ck_gameState.levelsDone[CK_INT(CK5_MapTeleLevelNeeded4, 10)] &&
			(keenYTilePos < CK_INT(CK5_MapTele1TopOpenAboveY, 75) || keenYTilePos > CK_INT(CK5_MapTele1TopOpenBelowY,100)))
		{
			CK5_CloseMapTeleporter(CK_INT(CK5_MapTele1BottomX, 24), CK_INT(CK5_MapTele1BottomY, 76));
			CK5_OpenMapTeleporter(CK_INT(CK5_MapTele1TopX, 22), CK_INT(CK5_MapTele1TopY, 55));
		}
		else
		{
			CK5_OpenMapTeleporter(CK_INT(CK5_MapTele1BottomX, 24), CK_INT(CK5_MapTele1BottomY, 76));
			CK5_CloseMapTeleporter(CK_INT(CK5_MapTele1TopX, 22), CK_INT(CK5_MapTele1TopY, 55));
		}

		// Unlock the bottom entrance to the upper shaft if we're below
		if (keenYTilePos > CK_INT(CK5_MapTele2BottomOpenBelowY, 39))
		{
			CK5_OpenMapTeleporter(CK_INT(CK5_MapTele2BottomX, 26), CK_INT(CK5_MapTele2BottomY, 55));
			CK5_CloseMapTeleporter(CK_INT(CK5_MapTele2TopX, 24), CK_INT(CK5_MapTele2TopY, 30));
		}

		// Unlock the top elevator when we're at the top or on Korath III.
		if (keenYTilePos < CK_INT(CK5_MapTele2TopOpenAboveY, 39) || keenYTilePos > CK_INT(CK5_MapTele2TopOpenBelowY, 100))
		{
			CK5_OpenMapTeleporter(CK_INT(CK5_MapTele2TopX, 24), CK_INT(CK5_MapTele2TopY, 30));
			CK5_CloseMapTeleporter(CK_INT(CK5_MapTele2BottomX, 26), CK_INT(CK5_MapTele2BottomY, 55));
		}
	}
}

void CK5_ExplodeGalaxy()
{
	// purge_chunks()
	VL_SetScrollCoords(0, 0);

	VL_FadeToBlack();
	CA_CacheGrChunk(98);   // Game Over Pic

	VL_ClearScreen(0);

	StartMusic(CK_INT(CK5_GameOverMusicLevel, 18));

	VH_DrawBitmap(32, 80, 98);
	VL_Present();

	IN_UserInput(24 * 70, false);

	StopMusic();
}

// Fuse Explosion Message

extern uint8_t ca_levelbit;
extern uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS];

void CK5_FuseMessage()
{
	SD_WaitSoundDone();

	// Cache the Keen thumbs up pic
	CA_UpLevel();
	ca_graphChunkNeeded[0x5A] |= ca_levelbit;
	ca_graphChunkNeeded[0x5B] |= ca_levelbit;
	CA_CacheMarks(0);

	// VW_SyncPages();

	// Draw Keen Talking
	US_CenterWindow(0x1A, 8);
	US_SetWindowW(US_GetWindowW() - 0x30);
	VHB_DrawBitmap(US_GetWindowW() + US_GetWindowX(), US_GetWindowY(), 0x5A);
	US_SetPrintY(US_GetPrintY() + 0xC);

	if (ck_gameState.currentLevel == CK_INT(CK5_SecretFuseLevel, 0xD))
		US_CPrint(CK_STRING(ck5_str_fuseDestroyedKorath));//"I wonder what that\nfuse was for....\n");
	else
		US_CPrint(CK_STRING(ck5_str_fuseDestroyed));//("One of the four\nmachines protecting the\nmain elevator shaft--\ntoast!\n");

	// VW_UpdateScreen();
	VL_Present();
	// VW_WaitVBL(30);

	IN_ClearKeysDown();
	// TODO: Add Joystick compatability here
	// IN_WaitForButton();
	IN_WaitButton();

	// Draw the Keen Thumbs Up Pic
	VHB_DrawBitmap(US_GetWindowW() + US_GetWindowX(), US_GetWindowY(), 0x5B);
	VL_Present();
	// VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
	// StopMusic();
}

void OSI_ShowGravityWarning()
{
	//VW_SyncPages()
	US_CenterWindow(0x1A, 4);
	US_SetPrintY(US_GetPrintY()+0xC);
	US_CPrint(CK_STRING(osi_lowGravityWarning));
	
	VL_Present();
	IN_ClearKeysDown();
	IN_WaitButton();
}
