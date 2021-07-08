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

#include <string.h> /* For memset() */

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
#include "ck4_ep.h"

CK_EpisodeDef ck4_episode = {
	EP_CK4,
	"CK4",
	&CK4_SetupFunctions,
	&CK4_ScanInfoLayer,
	&CK4_DefineConstants,
	&CK4_MapMiscFlagsCheck,
	&CK4_IsPresent,
	/* .activeLimit = */ 4,
	/* .highScoreLevel = */ 19,
	/* .highScoreTopMargin = */ 0x33,
	/* .highScoreLeftMargin = */ 0x18,
	/* .highScoreRightMargin = */ 0x128,
	/* .endSongLevel = */ 7,
	/* .starWarsSongLevel = */ 12,
	/* .lastLevelToMarkAsDone = */ 17,
	/* .objArrayOffset = */ 0xA807,
	/* .tempObjOffset = */ 0xC5D3,
	/* .spriteArrayOffset = */ 0xD622,
	/* .printXOffset = */ 0xA537,
	/* .animTilesOffset = */ 0xDDAE,
	/* .animTileSize = */ 4,
	/* .hasCreatureQuestion = */ false,
};

// Contains some keen-4 specific functions.

// Level Ending Object Spawn
void CK4_SetupFunctions()
{
	CK4_Obj1_SetupFunctions();
	CK4_Obj2_SetupFunctions();
	CK4_Obj3_SetupFunctions();
	CK4_Map_SetupFunctions();
	CK4_Misc_SetupFunctions();
}

// Check if all the game files are present.
bool CK4_IsPresent()
{
	// User-provided files
	if (!FS_IsKeenFilePresent("EGAGRAPH.CK4"))
		return false;
	if (!FS_IsKeenFilePresent("GAMEMAPS.CK4"))
		return false;
	if (!FS_IsKeenFilePresent("AUDIO.CK4"))
		return false;

	// Omnispeak-provided files
	if (!FS_IsOmniFilePresent("EGAHEAD.CK4"))
		return false;
	if (!FS_IsOmniFilePresent("EGADICT.CK4"))
		return false;
	if (!FS_IsOmniFilePresent("GFXINFOE.CK4"))
		return false;
	if (!FS_IsOmniFilePresent("MAPHEAD.CK4"))
		return false;
	// Map header file may include the tile info
	//if (!FS_IsOmniFilePresent("TILEINFO.CK4"))
	//	return false;
	if (!FS_IsOmniFilePresent("AUDIODCT.CK4"))
		return false;
	if (!FS_IsOmniFilePresent("AUDIOHHD.CK4"))
		return false;
	if (!FS_IsOmniFilePresent("AUDINFOE.CK4"))
		return false;

	if (!FS_IsOmniFilePresent("ACTION.CK4"))
		return false;

	// We clearly have all of the required files.
	return true;
}

// ck_inter.c
uint8_t ck4_starWarsPalette[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x10, 0x06, 0x07,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};

uint8_t ck4_terminator_palette1[] = {0, 0x18, 0x18, 0x07, 1, 1, 1, 1, 0x11, 0x11, 0x11, 0x11, 0x13, 0x13, 0x13, 0x13, 0};
uint8_t ck4_terminator_palette2[] = {0, 0x18, 0x18, 0x07, 1, 1, 1, 1, 0x11, 0x11, 0x11, 0x11, 0x13, 0x13, 0x13, 0x18, 0};

// ck_keen.c

soundnames ck4_itemSounds[] = {19, 19, 19, 19, 8, 8, 8, 8, 8, 8, 17, 9, 55};
uint16_t ck4_itemShadows[] = {250, 250, 250, 250, 219, 220, 221, 222, 223, 224, 225, 226};

// ck_map.c
int ck4_mapKeenFrames[] = {0x103, 0x112, 0x100, 0x109, 0x106, 0x10c, 0xFD, 0x10f};

// ck_play.c
int16_t ck4_levelMusic[] = {0, 4, 3, 3, 2, 2, 4, 3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 3, -1};

void CK4_DefineConstants(void)
{
	FON_MAINFONT = 3;

	FON_WATCHFONT = 4;

	PIC_HELPMENU = 6;
	PIC_ARROWDIM = 47;
	PIC_ARROWBRIGHT = 48;
	PIC_HELPPOINTER = 45;
	PIC_BORDERTOP = 80;
	PIC_BORDERLEFT = 81;
	PIC_BORDERRIGHT = 82;
	PIC_BORDERBOTTOMSTATUS = 83;
	PIC_BORDERBOTTOM = 84;

	PIC_MENUCARD = 88;
	PIC_NEWGAMECARD = 89;
	PIC_LOADCARD = 90;
	PIC_SAVECARD = 91;
	PIC_CONFIGURECARD = 92;
	PIC_SOUNDCARD = 93;
	PIC_MUSICCARD = 94;
	PIC_KEYBOARDCARD = 95;
	PIC_MOVEMENTCARD = 96;
	PIC_BUTTONSCARD = 97;
	PIC_JOYSTICKCARD = 98;
	PIC_OPTIONSCARD = 99;
	PIC_PADDLEWAR = 100;
	PIC_DEBUGCARD = 109;

	PIC_WRISTWATCH = 103;
	PIC_CREDIT1 = 104;
	PIC_CREDIT2 = 105;
	PIC_CREDIT3 = 106;
	PIC_CREDIT4 = 107;

	PIC_STARWARS = 108;
	PIC_TITLESCREEN = 109;
	PIC_COUNTDOWN5 = 115;
	PIC_COUNTDOWN4 = 116;
	PIC_COUNTDOWN0 = 120;

	MPIC_WRISTWATCHSCREEN = 121;
	MPIC_STATUSLEFT = 122;
	MPIC_STATUSRIGHT = 123;

	SPR_PADDLE = 124;
	SPR_BALL0 = 125;
	SPR_BALL1 = 126;
	SPR_BALL2 = 127;
	SPR_BALL3 = 128;

	SPR_DEMOSIGN = 129;

	SPR_STARS1 = 164;

	SPR_CENTILIFE1UPSHADOW = 218;

	SPR_GEM_A1 = 242;
	SPR_GEM_B1 = 244;
	SPR_GEM_C1 = 246;
	SPR_GEM_D1 = 248;
	SPR_100_PTS1 = 227;
	SPR_200_PTS1 = 229;
	SPR_500_PTS1 = 231;
	SPR_1000_PTS1 = 233;
	SPR_2000_PTS1 = 235;
	SPR_5000_PTS1 = 237;
	SPR_1UP1 = 239;
	SPR_STUNNER1 = 251;

	SPR_SCOREBOX = 253;

	SPR_MAPKEEN_WALK1_N = 260;
	SPR_MAPKEEN_STAND_N = 262;
	SPR_MAPKEEN_STAND_NE = 277;
	SPR_MAPKEEN_STAND_E = 259;
	SPR_MAPKEEN_STAND_SE = 268;
	SPR_MAPKEEN_WALK1_S = 263;
	SPR_MAPKEEN_STAND_S = 265;
	SPR_MAPKEEN_STAND_SW = 271;
	SPR_MAPKEEN_STAND_W = 256;
	SPR_MAPKEEN_STAND_NW = 274;

	TEXT_HELPMENU = 4739;
	TEXT_CONTROLS = 4741;
	TEXT_STORY = 4740;
	TEXT_ABOUTID = 4742;
	TEXT_END = 4743;
	TEXT_SECRETEND = 4744;
	TEXT_ORDER = 4745;

	EXTERN_ORDERSCREEN = 4735;
	EXTERN_COMMANDER = 4736;
	EXTERN_KEEN = 4737;

	DEMOSTART = 4746;

	SOUND_KEENWALK0 = 0;
	SOUND_KEENWALK1 = 1;
	SOUND_KEENJUMP = 2;
	SOUND_KEENLAND = 3;
	SOUND_KEENSHOOT = 4;
	// SOUND_MINEEXPLODE = 5;
	// SOUND_SLICEBUMP = 6;
	SOUND_KEENPOGO = 7;
	SOUND_GOTITEM = 8;
	SOUND_GOTSTUNNER = 9;
	SOUND_GOTCENTILIFE = 10;
	SOUND_UNKNOWN11 = 11;
	SOUND_UNKNOWN12 = 12;
	SOUND_LEVELEXIT = 13;
	SOUND_NEEDKEYCARD = 14;
	SOUND_KEENHITCEILING = 15;
	// SOUND_SPINDREDFLYUP = 16;
	SOUND_GOTEXTRALIFE = 17;
	SOUND_OPENGEMDOOR = 18;
	SOUND_GOTGEM = 19;
	SOUND_KEENFALL = 20;
	SOUND_KEENOUTOFAMMO = 21;
	SOUND_UNKNOWN22 = 22; // SKYPESTSQUISH
	SOUND_KEENDIE = 23;
	SOUND_UNKNOWN24 = 24;
	SOUND_KEENSHOTHIT = 25;
	SOUND_UNKNOWN26 = 26;
	SOUND_SPIROSLAM = 27;
	SOUND_SPINDREDSLAM = 28;
	SOUND_ENEMYSHOOT = 29; // SMIRKY
	SOUND_ENEMYSHOTHIT = 30;
	SOUND_AMPTONWALK0 = 31;
	SOUND_AMPTONWALK1 = 32;
	SOUND_AMPTONSTUN = 33;
	SOUND_UNKNOWN34 = 34;
	SOUND_UNKNOWN35 = 35;
	SOUND_SHELLYEXPLODE = 36;
	SOUND_SPINDREDFLYDOWN = 37;
	SOUND_MASTERSHOT = 38;
	SOUND_MASTERTELE = 39;
	SOUND_POLEZAP = 40;
	SOUND_UNKNOWN41 = 41;
	SOUND_SHOCKSUNDBARK = 42;
	SOUND_FLAGFLIP = 43;
	SOUND_FLAGLAND = 44;
	SOUND_BARKSHOTDIE = 45;
	// SOUND_UNKNOWN46 = 46;
	SOUND_KEENPADDLE = 47;
	SOUND_PONGWALL = 48;
	SOUND_COMPPADDLE = 49;
	SOUND_COMPSCORE = 50;
	SOUND_KEENSCORE = 51;
	LASTSOUND = 52;

	// ck_inter.c
	ck_starWarsPalette = ck4_starWarsPalette;
	ck_terminator_palette1 = ck4_terminator_palette1;
	ck_terminator_palette2 = ck4_terminator_palette2;

	// ck_keen.c
	ck_itemSounds = ck4_itemSounds;
	ck_itemShadows = ck4_itemShadows;

	// ck_map.c
	ck_mapKeenFrames = ck4_mapKeenFrames;

	// ck_play.c
	ck_levelMusic = ck4_levelMusic;
}

/*
 * Spawn an enemy projectile
 * Note that the behaviour is slightly different from DOS Keen
 * DOS Keen SpawnEnemyShot returns 0 if shot is spawned, or -1 otherwise
 * omnispeak CK4_SpawnEnemyShot returns pointer if succesful, NULL otherwise
 */

CK_object *CK4_SpawnEnemyShot(int posX, int posY, CK_action *action)
{
	CK_object *new_object = CK_GetNewObj(true);

	if (!new_object)
		return NULL;

	new_object->posX = posX;
	new_object->posY = posY;
	new_object->type = CT4_EnemyShot;
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

// TODO: Cache stuff here instead of spawner handlers

#define MAXLUMPS 0x25
static bool ck4_lumpsNeeded[MAXLUMPS];
typedef enum
{
	Lump_0 = 0,
	Lump_Keen = 1,
	Lump_Soda = 2,
	Lump_Candybar = 3,
	Lump_Choc = 4,
	Lump_Jawbreaker = 5,
	Lump_Donut = 6,
	Lump_Icecream = 7,
	Lump_LifewaterFlask = 8,
	Lump_Stunner = 9,
	Lump_MapKeen = 10,
	Lump_Slug = 11,
	Lump_Mushroom = 12,
	Lump_DartShooter = 13,
	Lump_Lindsey = 14,
	Lump_FootWorm = 15,
	Lump_Smirky = 16,
	Lump_CouncilMember = 17,
	Lump_Bird = 18,
	Lump_Mimrock = 19,
	Lump_Dopefish = 20,
	Lump_Schoolfish = 21,
	Lump_Arachnut = 22,
	Lump_Skypest = 23,
	Lump_Wormmouth = 24,
	Lump_Lick = 25,
	Lump_Platform = 26,
	Lump_Bounder = 27,
	Lump_Cloud = 28,
	Lump_Berkeloid = 29,
	Lump_Gems = 30,
	Lump_Dart = 31,
	Lump_ScubaKeen = 32,
	Lump_Sprite = 33,
	Lump_Mine = 34,
	Lump_KeenMoon = 35,
	Lump_Egg = 36,
} CK_Lumptype;

static int16_t ck4_itemLumps[] =
	{
		Lump_Gems,
		Lump_Gems,
		Lump_Gems,
		Lump_Gems,
		Lump_Soda,
		Lump_Candybar,
		Lump_Choc,
		Lump_Jawbreaker,
		Lump_Donut,
		Lump_Icecream,
		Lump_LifewaterFlask,
		Lump_Stunner,
};

static int16_t ck4_lumpStarts[MAXLUMPS] =
	{
		88,
		130,
		227,
		229,
		231,
		233,
		235,
		237,
		239,
		251,
		254,
		315,
		325,
		0,
		329,
		333, // SPR_INCHWORM_R1
		338, // SPR_SMIRKY_LOOKLEFT
		356, // SPR_COUNCILWALK_R1
		367, // SPR_BIRDWALK_R1
		388, // SPR_MIMROCK
		404, // SPR_DOPEFISHR1
		421, // SPR_SCHOOLFISHL1
		425, // SPR_ARACHNUTWALK_1
		443, // SPR_SKYPESTFLYL1
		457, // wormouth
		469, // SPR_LICKR1
		484, // SPR_PLATFORM
		491, // SPR_BOUNDERL1
		498, // SPR_CLOUD1
		503, // SPR_BERKELOIDL1
		242,
		380,
		309,
		431,
		440,
		519,
		362, // SPR_EGG1
};

static int16_t ck4_lumpEnds[MAXLUMPS] =
	{
		103,
		226,
		228,
		230,
		232,
		234,
		236,
		238,
		240,
		252,
		308,
		324,
		328,
		0,
		332,
		337, // SPR_FOOT
		355, // SPR_SMIRKYSTUNNED
		361, // SPR_COUNCILPAUSE2
		379, // SPR_BIRDSTUNNED
		403, // SPR_MIMROCKSTUNNED
		420,
		424,
		429, // SPR_ARACHNUTSTUNNED
		456, // SPR_SKYPESTSQUISH
		468, // wormmouth
		483, // SPR_LICKSTUNNED
		490, // SPR_PLATFORMFLAMES_BOT2
		497, // SPR_BOUNDERSTUNNED
		502, // SPR_BOLT2
		518,
		250,
		387,
		314,
		439,
		442,
		520,
		366, // SPR_EGGBIT4
};

void CK4_ScanInfoLayer()
{

	// Reset the lumps (contiguous lists of chunks for sprites) required by
	// the level.
	memset(ck4_lumpsNeeded, 0, sizeof(ck4_lumpsNeeded));

	//TODO: Work out where to store current map number, etc.
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
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				ck4_lumpsNeeded[Lump_Keen] = true;
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				ck4_lumpsNeeded[Lump_Keen] = true;
				break;

			// ScubaKeen
			case 42:
				CK4_SpawnScubaKeen(x, y);
				CK_DemoSignSpawn();
				ck4_lumpsNeeded[Lump_ScubaKeen] = true;
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				ck4_lumpsNeeded[Lump_MapKeen] = true;
				CK_SpawnMapKeen(x, y);
				break;

			case 4:
				CK4_SpawnCouncilMember(x, y);
				ck4_lumpsNeeded[Lump_CouncilMember] = true;
				break;

			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				RF_SetScrollBlock(x, y, false);
				break;

			// Miragia
			case 33:
				CK4_SpawnMiragia(x, y);
				break;

				// Slugs
			case 44:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 43:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 22:
				ck4_lumpsNeeded[Lump_Slug] = true;
				CK4_SpawnSlug(x, y);
				break;

				// Mushrooms
			case 21:
				ck4_lumpsNeeded[Lump_Mushroom] = true;
				CK4_SpawnMushroom(x, y);
				break;

				// Birds
			case 13:
				ck4_lumpsNeeded[Lump_Egg] = true;
				ck4_lumpsNeeded[Lump_Bird] = true;
				CK4_SpawnEgg(x, y);
				break;

			case 78:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 77:
				if (ck_gameState.difficulty < D_Normal)
					break;
				ck4_lumpsNeeded[Lump_Bird] = true;
				CK4_SpawnBird(x, y);
				break;

				// Arachnuts
			case 74:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 73:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 20:
				ck4_lumpsNeeded[Lump_Arachnut] = true;
				CK4_SpawnArachnut(x, y);
				break;

			case 75:
				ck4_lumpsNeeded[Lump_KeenMoon] = true;
				break;

				// Skypest
				//
			case 46:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 45:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 8:
				ck4_lumpsNeeded[Lump_Skypest] = true;
				CK4_SpawnSkypest(x, y);
				break;

				// Wormmouth
				//
			case 52:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 51:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 7:
				ck4_lumpsNeeded[Lump_Wormmouth] = true;
				CK4_SpawnWormmouth(x, y);
				break;

				// Clouds
			case 9:
				ck4_lumpsNeeded[Lump_Cloud] = true;
				CK4_SpawnCloud(x, y);
				break;

				// Berks
			case 50:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 49:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 5:
				ck4_lumpsNeeded[Lump_Berkeloid] = true;
				CK4_SpawnBerkeloid(x, y);
				break;

				// Inchworms
			case 10:
				ck4_lumpsNeeded[Lump_FootWorm] = true;
				CK4_SpawnFoot(x, y);
				goto cachePoofs;

			case 11:
				ck4_lumpsNeeded[Lump_FootWorm] = true;
				CK4_SpawnInchworm(x, y);
				goto cachePoofs;

			cachePoofs:

				for (int i = 350; i <= 353; i++)
				{
					CA_MarkGrChunk(i);
				}
				break;

			// Bounders
			case 12:
				ck4_lumpsNeeded[Lump_Bounder] = true;
				CK4_SpawnBounder(x, y);
				break;

			// Licks
			case 48:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 47:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 14:
				ck4_lumpsNeeded[Lump_Lick] = true;
				CK4_SpawnLick(x, y);
				break;

				// Plats
			case 27:
			case 28:
			case 29:
			case 30:
				CK4_SpawnAxisPlatform(x, y, infoValue - 27);
				ck4_lumpsNeeded[Lump_Platform] = true;
				break;

			case 31:
				// B Block
				break;

			case 32:
				CK_SpawnFallPlat(x, y);
				ck4_lumpsNeeded[Lump_Platform] = true;
				break;

				//Smirkies
			case 18:
				CK4_SpawnSmirky(x, y);
				ck4_lumpsNeeded[Lump_Smirky] = true;
				break;

				//Mimrocks
			case 19:
				CK4_SpawnMimrock(x, y);
				ck4_lumpsNeeded[Lump_Mimrock] = true;
				break;

			// Dopefish
			case 88:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 87:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 15:
				ck4_lumpsNeeded[Lump_Dopefish] = true;
				CK4_SpawnDopefish(x, y);
				break;

				// Schoolfish
			case 16:
				ck4_lumpsNeeded[Lump_Schoolfish] = true;
				CK4_SpawnSchoolfish(x, y);
				break;

			// Sprites
			case 24:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 23:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 17:
				ck4_lumpsNeeded[Lump_Sprite] = true;
				CK4_SpawnSprite(x, y);
				break;

				// Mines
			case 69:
			case 70:
			case 71:
			case 72:
				ck4_lumpsNeeded[Lump_Mine] = true;
				CK4_SpawnMine(x, y, infoValue - 69);
				break;

				// Lindsey
			case 6:
				ck4_lumpsNeeded[Lump_Lindsey] = true;
				CK4_SpawnLindsey(x, y);
				break;

			case 83:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 79:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 53:
				CK4_SpawnDartGun(x, y, 0);
				ck4_lumpsNeeded[Lump_Dart] = true;
				break;

			case 84:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 80:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 54:
				CK4_SpawnDartGun(x, y, 1);
				ck4_lumpsNeeded[Lump_Dart] = true;
				break;

			case 85:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 81:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 55:
				CK4_SpawnDartGun(x, y, 2);
				ck4_lumpsNeeded[Lump_Dart] = true;
				break;

			case 86:
				if (ck_gameState.difficulty < D_Hard)
					break;
			case 82:
				if (ck_gameState.difficulty < D_Normal)
					break;
			case 56:
				CK4_SpawnDartGun(x, y, 3);
				ck4_lumpsNeeded[Lump_Dart] = true;
				break;

				// Wetsuit
			case 35:
				CK4_SpawnWetsuit(x, y);
				CA_MarkGrChunk(0x1AE);
				break;

			case 34:
				// Spawn extra stunner if Keen has low ammo
				if (ck_gameState.numShots >= 5)
					break;
				infoValue = 68;
				ck4_lumpsNeeded[Lump_Stunner] = true;
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
				ck4_lumpsNeeded[ck4_itemLumps[infoValue - 57]] = true;
				break;

			default:
				break;
			}
		}
	}

	for (CK_object *obj = ck_keenObj; obj != NULL; obj = obj->next)
	{
		if (obj->active != OBJ_ALWAYS_ACTIVE)
			obj->active = OBJ_INACTIVE;
	}

	for (int i = 0; i < MAXLUMPS; i++)
		if (ck4_lumpsNeeded[i])
			for (int j = ck4_lumpStarts[i]; j <= ck4_lumpEnds[i]; j++)
				CA_MarkGrChunk(j);
}

// Dialog Functions
// These are right after ScanInfoLayer in DOS exe
void CK4_ShowPrincessMessage(void)
{
	SD_WaitSoundDone();
	StopMusic();
	CA_UpLevel();

	CA_MarkGrChunk(0x72);
	CA_MarkGrChunk(0x6F);
	CA_MarkGrChunk(0x70);
	CA_CacheMarks(0);

	// VW_SyncPages();
	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 0x72);
	US_SetPrintY(US_GetPrintY() + 6);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	US_CPrint(CK_VAR_GetStr("ck4_str_lindsaySays"));

	if (ca_mapOn == 7)
		US_CPrint(CK_VAR_GetStr("ck4_str_lindsayMessage1"));
	else
		US_CPrint(CK_VAR_GetStr("ck4_str_lindsayMessage2"));

	VL_Present(); // VW_UpdateScreen();
	SD_PlaySound(SOUND_FOOTAPPEAR);
	// VW_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();
	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 0x6F);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);

	if (ca_mapOn == 7)
		US_CPrint(CK_VAR_GetStr("ck4_str_lindsayThanks1"));
	else
		US_CPrint(CK_VAR_GetStr("ck4_str_lindsayThanks2"));

	VL_Present(); // VW_UpdateScreen();
	/// VW_WaitVBL(30);

	IN_ClearKeysDown();
	IN_WaitButton();

	VHB_DrawBitmap(US_GetWindowW() + US_GetWindowX(), US_GetWindowY(), 0x70);
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(30);

	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();

	StartMusic(ck_gameState.currentLevel);
	ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;
}

void CK4_ShowJanitorMessage(void)
{
	SD_WaitSoundDone();
	CA_UpLevel();
	CA_MarkGrChunk(0x6E);
	CA_MarkGrChunk(0x6F);
	CA_MarkGrChunk(0x71);
	CA_CacheMarks(0);

	// VW_SyncPages();
	StartMusic(0xFFFF);

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 0x6E);
	US_SetPrintY(US_GetPrintY() + 6);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	US_CPrint(CK_VAR_GetStr("ck4_str_janitor1"));
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 0x6E);
	US_SetPrintY(US_GetPrintY() + 6);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	US_CPrint(CK_VAR_GetStr("ck4_str_janitor2"));
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 0x6F);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);
	US_CPrint(CK_VAR_GetStr("ck4_str_janitor3"));
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 0x6E);
	US_SetPrintY(US_GetPrintY() + 6);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	US_CPrint(CK_VAR_GetStr("ck4_str_janitor4"));
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 0x6F);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x28, US_GetWindowY() + 0x18, 0x71);
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();

	StopMusic();
	CA_DownLevel();
	StartMusic(ck_gameState.currentLevel);
}

void CK4_ShowCantSwimMessage(void)
{
	SD_WaitSoundDone();
	CA_UpLevel();
	CA_CacheGrChunk(0x6F);
	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 0x6F);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 6);
	US_CPrint(CK_VAR_GetStr("ck4_str_cantSwim"));
	VL_Present(); // VW_UpdateScreen();
	// VL_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	CA_DownLevel();
}

void CK4_ShowWetsuitMessage(void)
{
	SD_WaitSoundDone();
	CA_UpLevel();
	CA_MarkGrChunk(0x6F);
	CA_MarkGrChunk(0x70);
	CA_CacheMarks(0);
	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 0x6F);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);
	US_CPrint(CK_VAR_GetStr("ck4_str_gotWetsuit"));
	VL_Present(); // VW_UpdateScreen();
	// VL_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();

	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW(), US_GetWindowY(), 0x70);
	VL_Present(); // VW_UpdateScreen();
	// VL_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();

	CA_DownLevel();
}

void CK4_ShowCouncilMessage(void)
{
	SD_WaitSoundDone();
	CA_UpLevel();

	CA_MarkGrChunk(0x6E);
	CA_MarkGrChunk(0x6F);
	CA_MarkGrChunk(0x70);
	CA_CacheMarks(0);
	StartMusic(-1);

	// VW_SyncPages();

	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 110);
	US_SetPrintY(US_GetPrintY() + 6);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);

	if (ca_mapOn == 17)
	{
		// Underwater level
		US_CPrint(CK_VAR_GetStr("ck4_str_councilThanks2"));
	}
	else
	{
		US_CPrint(CK_VAR_GetStr("ck4_str_councilThanks"));
	}

	VL_Present();
	// VL_WaitVBL(60);
	IN_ClearKeysDown();
	IN_WaitButton();
	US_CenterWindow(26, 8);
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 111);
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetPrintY(US_GetPrintY() + 12);
	US_CPrint(CK_VAR_GetStringByNameAndIndex("ck4_str_councilMessage",ck_gameState.ep.ck4.membersRescued));
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(30);
	IN_ClearKeysDown();
	IN_WaitButton();
	VHB_DrawBitmap(US_GetWindowX() + US_GetWindowW(), US_GetWindowY(), 112);
	VL_Present(); // VW_UpdateScreen();
	// VW_WaitVBL(30);
	IN_WaitButton();
	ck_gameState.ep.ck4.membersRescued++;
	CA_DownLevel();
	StopMusic();
}

void CK4_SpawnKeenBubble(CK_object *);

// Scuba Keen
void CK4_SpawnScubaKeen(int tileX, int tileY)
{
	ck_keenObj->type = CT_Player;
	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->zLayer = PRIORITIES - 3;
	ck_keenObj->posX = RF_TileToUnit(tileX);
	ck_keenObj->posY = RF_TileToUnit(tileY);
	ck_keenObj->xDirection = IN_motion_Right;
	ck_keenObj->yDirection = IN_motion_Down;
	ck_keenObj->clipped = CLIP_simple;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK4_ACT_KeenSwim0"));
}

void CK4_SpawnKeenBubble(CK_object *obj)
{
	obj->user4 = 0;
	CK_object *bubble = CK_GetNewObj(true);
	bubble->posX = obj->xDirection == IN_motion_Left ? obj->posX : obj->posX + 0x180;

	bubble->posY = obj->posY;
	bubble->type = CT_Friendly;
	bubble->zLayer = PRIORITIES - 1;
	bubble->active = OBJ_EXISTS_ONLY_ONSCREEN;
	bubble->clipped = CLIP_not;
	bubble->velY = -24;
	bubble->velX = 4;

	switch (US_RndT() / 64)
	{
	case 0:
		CK_SetAction(bubble, CK_GetActionByName("CK4_ACT_Bubble0"));
		break;

	case 1:
		CK_SetAction(bubble, CK_GetActionByName("CK4_ACT_Bubble1"));
		break;

	case 2:
		CK_SetAction(bubble, CK_GetActionByName("CK4_ACT_Bubble2"));
		break;

	case 3:
		CK_SetAction(bubble, CK_GetActionByName("CK4_ACT_Bubble3"));
		break;
	}

	SD_PlaySound(SOUND_KEENBUBBLE);
}

void CK4_KeenSwim(CK_object *obj)
{
	int movingLeft = obj->velX < 0 ? 1 : 0;
	int movingUp = obj->velY < 4 ? 1 : 0;

	if ((obj->user4 += SD_GetSpriteSync()) > 60)
		CK4_SpawnKeenBubble(obj);

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		if (ck_inputFrame.xDirection)
			obj->velX = ck_inputFrame.xDirection * 18;
		if (ck_inputFrame.yDirection)
			obj->velY = ck_inputFrame.yDirection * 18;

		if (obj->currentAction->next)
			obj->currentAction = obj->currentAction->next;
	}

	if (ck_inputFrame.xDirection)
		obj->xDirection = ck_inputFrame.xDirection;

	int32_t lastTimeCount = SD_GetLastTimeCount();
	int32_t tic = lastTimeCount - SD_GetSpriteSync();
	while (tic < lastTimeCount)
	{
		if (!(tic & 7))
		{
			int dx;
			if (obj->velX > 12)
				dx = -3;
			else if (obj->velX > 0)
				dx = -1;
			else if (obj->velX > -12)
				dx = 1;
			else
				dx = 3;

			dx += 2 * ck_inputFrame.xDirection;
			obj->velX += dx;

			if (!ck_inputFrame.xDirection)
			{
				int stillMovingLeft = obj->velX < 0 ? 1 : 0;

				if (stillMovingLeft != movingLeft)
					obj->velX = 0;
			}

			int dy;
			if (obj->velY > 12)
				dy = -3;
			else if (obj->velY > 4)
				dy = -1;
			else if (obj->velY > -12)
				dy = 1;
			else
				dy = 3;

			dy += 2 * ck_inputFrame.yDirection;
			obj->velY += dy;

			if (!ck_inputFrame.yDirection)
			{
				if (obj->velY > 4 && movingUp)
					obj->velY = 0;
			}
		}

		ck_nextX += obj->velX;
		ck_nextY += obj->velY;
		tic++;
	}
}

void CK4_KeenSwimFast(CK_object *obj)
{
	if ((obj->user4 += SD_GetSpriteSync()) > 60)
		CK4_SpawnKeenBubble(obj);

	if (ck_keenState.jumpIsPressed && !ck_keenState.jumpWasPressed)
	{
		ck_keenState.jumpWasPressed = true;
		obj->velX = ck_inputFrame.xDirection * 18;

		if (ck_inputFrame.yDirection)
			obj->velY = ck_inputFrame.yDirection * 18;
	}

	if (obj->currentAction == CK_GetActionByName("CK4_ACT_KeenSwimFast0"))
		obj->currentAction = CK_GetActionByName("CK4_ACT_KeenSwimFast1");
	else
		obj->currentAction = CK_GetActionByName("CK4_ACT_KeenSwimFast0");

	ck_nextX += obj->velX * SD_GetSpriteSync();
	ck_nextY += obj->velY * SD_GetSpriteSync();

	if (ck_nextX > 0)
		obj->xDirection = IN_motion_Right;
	else if (ck_nextX < 0)
		obj->xDirection = IN_motion_Left;

	ck_nextY += SD_GetSpriteSync() * 4;
}

void CK4_KeenSwimCol(CK_object *a, CK_object *b)
{
	if (b->type == CT4_Item)
	{
		if (b->user1 > 11)
			return;

		SD_PlaySound(ck_itemSounds[b->user1]);

		b->type = CT_Friendly;
		b->zLayer = PRIORITIES - 1;
		b->gfxChunk = ck_itemShadows[b->user1];

		CK_IncreaseScore(ck_itemPoints[b->user1]);

		//b->yDirection = -1;

		if (b->user1 < 4)
		{
			ck_gameState.keyGems[b->user1]++;
		}
		else if (b->user1 == 10)
		{
			ck_gameState.numLives++;
		}
		else if (b->user1 == 11)
		{
			ck_gameState.numShots += (ck_gameState.difficulty == D_Easy) ? 8 : 5;
		}
		CK_SetAction2(b, CK_GetActionByName("CK_ACT_itemNotify"));
	}
	else if (b->type == CT4_CouncilMember)
	{
		ck_gameState.levelState = LS_CouncilRescued;
	}
}

void CK4_KeenSwimDraw(CK_object *obj)
{
	if (obj->rightTI || obj->leftTI)
		obj->velX = 0;

	if ((obj->topTI && obj->velY > 0) || (obj->bottomTI && obj->velY < 0))
		obj->velY = 0;

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// ===========================================================================

void CK4_Misc_SetupFunctions()
{
	CK_ACT_AddFunction("CK4_KeenSwim", &CK4_KeenSwim);
	CK_ACT_AddFunction("CK4_KeenSwimFast", &CK4_KeenSwimFast);
	CK_ACT_AddColFunction("CK4_KeenSwimCol", &CK4_KeenSwimCol);
	CK_ACT_AddFunction("CK4_KeenSwimDraw", &CK4_KeenSwimDraw);
}
