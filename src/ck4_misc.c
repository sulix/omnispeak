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

#include "id_ca.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_vh.h"
#include "id_vl.h"
#include "ck_game.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_act.h"
#include "ck4_ep.h"
#include <stdio.h>

CK_EpisodeDef ck4_episode ={
  EP_CK4,
	"CK4",
	&CK4_SetupFunctions,
	&CK4_ScanInfoLayer,
  &CK4_DefineConstants
};


// Contains some keen-4 specific functions.

void CK4_RedAxisPlatform(CK_object *obj)
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
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			ck_nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
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
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1] == 0x1F)
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
				ck_nextY -= ( nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1] == 0x1F)
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
				ck_nextY +=  256 - (nextPosUnit & 255);
			}
		}
	}
}

void CK4_PurpleAxisPlatform(CK_object *obj)
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
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
		{
			obj->xDirection = -1;
			//TODO: Change DeltaVelocity
			ck_nextX -= (nextPosUnit & 255);
		}
	}
	else if (obj->xDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitX1 + ck_nextX;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileX1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * obj->clipRects.tileY1 + nextPosTile] == 0x1F)
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
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY2 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1 + 1] == 0x1F)
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
				ck_nextY -= ( nextPosUnit & 255);
			}
		}
	}
	else if (obj->yDirection == -1)
	{
		nextPosUnit = obj->clipRects.unitY1 + ck_nextY;
		nextPosTile = nextPosUnit >> 8;
		if (obj->clipRects.tileY1 != nextPosTile && CA_mapPlanes[2][CA_MapHeaders[ca_mapOn]->width * nextPosTile + obj->clipRects.tileX1 + 1] == 0x1F)
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
				ck_nextY +=  256 - (nextPosUnit & 255);
			}
		}
	}
}

void CK4_SpawnFallPlat(int tileX, int tileY)
{
	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_CLASS(Platform);
	new_object->active = OBJ_ALWAYS_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = tileX << 8;
	new_object->user1 = new_object->posY = tileY << 8;
	new_object->xDirection = IN_motion_None;
	new_object->yDirection = IN_motion_Down;
	new_object->clipped = CLIP_not;
	CK_SetAction(new_object, CK_GetActionByName("CK4_ACT_FallPlat0"));
}

void CK4_FallPlatSit (CK_object *obj)
{

	if (obj == ck_keenState.platform)
	{
		ck_nextY = SD_GetSpriteSync() * 16;
		obj->velY = 0;
		if ((unsigned)(obj->posY + ck_nextY - obj->user1) >= 0x80)
			obj->currentAction = CK_GetActionByName("CK4_ACT_FallPlat1");
	}
}

void CK4_FallPlatFall (CK_object *obj)
{
	uint16_t newY, newYT;

	CK_PhysGravityHigh(obj);
	newY = obj->clipRects.unitY2 + ck_nextY;
	newYT = newY >> 8;

	// Stop falling if platform hits a block
	if ((obj->clipRects.tileY2 != newYT) && (CA_TileAtPos(obj->clipRects.tileX1, newYT, 2) == 0x1F))
	{
		ck_nextY = 0xFF - (obj->clipRects.unitY2 & 0xFF);
		if (ck_keenState.platform != obj)
			obj->currentAction = CK_GetActionByName("CK4_ACT_FallPlat2");
	}
}

void CK4_FallPlatRise (CK_object *obj)
{
	if (ck_keenState.platform == obj)
	{
		obj->velY = 0;
		obj->currentAction = CK_GetActionByName("CK4_ACT_FallPlat1");
	}
	else if ((unsigned) obj->posY <= (unsigned) obj->user1)
	{
		ck_nextY = obj->user1 - obj->posY;
		obj->currentAction = CK_GetActionByName("CK4_ACT_FallPlat0");
	}
}

// Level Ending Object Spawn
void CK4_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
	CK4_Obj1_SetupFunctions();
	// CK4_Obj2_SetupFunctions();
	// CK4_Obj3_SetupFunctions();
	CK4_Map_SetupFunctions();
}

// HACK: Sorry, the strings need to be in WRITABLE storage,
// because US_CPrint (temporarily) modifies them.

// ck_game.c
char ck4_levelEntryText_0[] =
	"Keen enters the\n"
	"Shadowlands\n";

char ck4_levelEntryText_1[] =
	"Keen makes a run for\n"
	"the Border Village";

char ck4_levelEntryText_2[] =
  "Keen slips into\n"
	"Slug Village";

char ck4_levelEntryText_3[] =
	"Keen plummets into\n"
	"the The Perilous Pit";

char ck4_levelEntryText_4[] =
	"Keen plods down into\n"
	"the Cave of the\n"
	"Descendents";

char ck4_levelEntryText_5[] =
	"Keen shivers along\n"
	"the Chasm of Chills";

char ck4_levelEntryText_6[] =
	"Keen reflects upon\n"
	"entering Crystalus";

char ck4_levelEntryText_7[] =
	"Keen stumbles upon\n"
	"Hillville";

char ck4_levelEntryText_8[] =
	"Keen grits his teeth\n"
	"and enters Sand Yego";

char ck4_levelEntryText_9[] =
	"Keen disappears into\n"
	"Miragia";

char ck4_levelEntryText_10[] =
	"Keen crawls into\n"
	"Lifewater Oasis";

char ck4_levelEntryText_11[] =
	"Keen backs into the\n"
	"Pyramid of the Moons";

char ck4_levelEntryText_12[] =
  "Keen move silently in\n"
	"the Pyramid of Shadows";

char ck4_levelEntryText_13[] =
	"Keen reverently enters\n"
	"the Pyramid of the\n"
	"Gnosticene Ancients";

char ck4_levelEntryText_14[] =
	"Keen hesitantly crosses\n"
	"into the Pyramid of the\n"
	"Forbidden";

char ck4_levelEntryText_15[] =
	"Keen mucks along the\n"
	"Isle of Tar";

char ck4_levelEntryText_16[] =
	"Keen blazes across the\n"
	"Isle of Fire";

char ck4_levelEntryText_17[] =
	"Keen hopefully enters\n"
	"the Well of Wishes";

char ck4_levelEntryText_18[] =
	"Keen launches into the\n"
  "Bean-with-Bacon\n"
  "Megarocket";

char *ck4_levelEntryTexts[] = {
	ck4_levelEntryText_0,
	ck4_levelEntryText_1,
	ck4_levelEntryText_2,
	ck4_levelEntryText_3,
	ck4_levelEntryText_4,
	ck4_levelEntryText_5,
	ck4_levelEntryText_6,
	ck4_levelEntryText_7,
	ck4_levelEntryText_8,
	ck4_levelEntryText_9,
	ck4_levelEntryText_10,
	ck4_levelEntryText_11,
	ck4_levelEntryText_12,
	ck4_levelEntryText_13,
	ck4_levelEntryText_14,
	ck4_levelEntryText_15,
	ck4_levelEntryText_16,
	ck4_levelEntryText_17,
	ck4_levelEntryText_18,
};

const char *ck4_levelNames[] = {
	"Shadowlands",
  "Border Village",
	"Slug Village",
	"The Perilous Pit",
	"Cave of the Descendents",
	"Chasm of Chills",
	"Crystalus",
	"Hilville",
	"Sand Yego",
	"Miragia",
	"Lifewater Oasis",
	"Pyramid of the Moons",
	"Pyramid of Shadows",
	"Pyramid of the\nGnosticene Ancients",
	"Isle of Tar",
  "Isle of Fire",
  "Well of Wishes",
  "Bean-with-Bacon\nMegarocket"
};

soundnames ck4_itemSounds[]  = { 19, 19, 19, 19, 8,8,8,8,8,8, 17, 9, 55 };
int16_t ck4_itemShadows[] = {250, 250, 250, 250, 219, 220, 221, 222, 223, 224, 225, 226};

// ck_map.c
int ck4_mapKeenFrames[] = { 0x103, 0x112, 0x100, 0x109, 0x106, 0x10c, 0xFD, 0x10f };

// ck_play.c
int16_t ck4_levelMusic[] ={0, 4, 3, 3, 2, 2, 4, 3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 3, -1};

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

  STR_EXIT_TO_MAP = "Exit to Shadowlands";

  // ck_game.c
  ck_levelEntryTexts = ck4_levelEntryTexts;
  ck_levelNames = ck4_levelNames;

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

void CK4_SpawnAxisPlatform(int tileX, int tileY, int direction, bool purple)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ALWAYS_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;

	switch (direction)
	{
	case 0:
		obj->xDirection = 0;
		obj->yDirection = -1;
		break;
	case 1:
		obj->xDirection = 1;
		obj->yDirection = 0;
		break;
	case 2:
		obj->xDirection = 0;
		obj->yDirection = 1;
		break;
	case 3:
		obj->xDirection = -1;
		obj->yDirection = 0;
		break;
	}

	if (purple)
	{
		obj->posX += 0x40;
		obj->posY += 0x40;
		CK_SetAction(obj, CK_GetActionByName("CK4_act_purpleAxisPlatform"));
	}
	else
	{

		CK_SetAction(obj, CK_GetActionByName("CK4_act_redAxisPlatform"));
	}
	// TODO: These should *not* be done here.
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

void CK4_SpawnRedStandPlatform(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 6;
	obj->active = OBJ_ACTIVE;
	obj->zLayer = 0;
	obj->posX = tileX << 8;
	obj->posY = obj->user1 = tileY << 8;
	obj->xDirection = 0;
	obj->yDirection = 1;
	obj->clipped = CLIP_not;
	CK_SetAction(obj, CK_GetActionByName("CK4_ACT_redStandPlatform"));
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}


// TODO: Cache stuff here instead of spawner handlers

#define MAXLUMPS 0x25
static bool ck4_lumpsNeeded[MAXLUMPS];
typedef enum
{
  Lump_0 = 0,
  Lump_Slug = 11,
  Lump_Mushroom = 12,
  Lump_CouncilMember = 17,
  Lump_Bird = 18,
  Lump_Arachnut = 22,
  Lump_Skypest = 23,
  Lump_Egg = 36,
} CK_Lumptype;

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
  333,
  338,
  356,// SPR_COUNCILWALK_R1
  367,  // SPR_BIRDWALK_R1
  388,
  404,
  421,
  425,  // SPR_ARACHNUTWALK_1
  443,  // SPR_SKYPESTFLYL1
  457,
  469,
  484,
  491,
  498,
  503,
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
  337,
  355,
  361, // SPR_COUNCILPAUSE2
  379, // SPR_BIRDSTUNNED
  403,
  420,
  424,
  429, // SPR_ARACHNUTSTUNNED
  456, // SPR_SKYPESTSQUISH
  468,
  483,
  490,
  497,
  502,
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

  memset(ck4_lumpsNeeded, sizeof(ck4_lumpsNeeded), 0);

  //TODO: Work out where to store current map number, etc.
	int mapW = CA_MapHeaders[ca_mapOn]->width;
	int mapH = CA_MapHeaders[ca_mapOn]->height;

	for (int y = 0; y < mapH; ++y)
	{
		for (int x = 0; x < mapW; ++x)
		{
			int infoValue = CA_mapPlanes[2][y * mapW + x];
			switch (infoValue)
			{
			case 1:
				CK_SpawnKeen(x, y, 1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[SPR_SCOREBOX] |= ca_levelbit;
				if (ck_gameState.levelState != 13)
					CK_SpawnMapKeen(x, y);
				break;

      case 4:
        CK4_SpawnCouncilMember(x, y);
        ck4_lumpsNeeded[Lump_CouncilMember] = true;
        break;


			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
#if 0
			case 27:
			case 28:
			case 29:
			case 30:
				CK4_SpawnAxisPlatform(x, y, infoValue - 27, false);
				break;
			case 32:
				CK4_SpawnFallPlat(x, y);
				break;

			case 33:
				if (ck_gameState.difficulty > D_Easy) break;
			case 34:
				if (ck_gameState.difficulty > D_Normal) break;
			case 35:
				CK4_SpawnRedStandPlatform(x, y);
				break;

#endif

        // Slugs
      case 44:
        if (ck_gameState.difficulty < D_Hard) break;
      case 43:
        if (ck_gameState.difficulty < D_Normal) break;
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
        CK4_SpawnEgg(x, y);
        break;

      case 78:
        if (ck_gameState.difficulty < D_Hard) break;
      case 77:
        if (ck_gameState.difficulty < D_Normal) break;
        ck4_lumpsNeeded[Lump_Bird] = true;
        CK4_SpawnBird(x, y);
        break;

        // Arachnuts
      case 74:
        if (ck_gameState.difficulty < D_Hard) break;
      case 73:
        if (ck_gameState.difficulty < D_Normal) break;
      case 20:
        ck4_lumpsNeeded[Lump_Arachnut] = true;
        CK4_SpawnArachnut(x, y);
        break;

        // Skypest
        //
      case 46:
        if (ck_gameState.difficulty < D_Hard) break;
      case 45:
        if (ck_gameState.difficulty < D_Normal) break;
      case 8:
        ck4_lumpsNeeded[Lump_Skypest] = true;
        CK4_SpawnSkypest(x, y);
        break;





      case 33:
        CK4_SpawnMiragia(x, y);
        break;

			case 34:
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
				break;
#if 0

			case 84:
			case 85:
			case 86:
			case 87:
				CK4_SpawnAxisPlatform(x, y, infoValue - 84, true);
				break;
#endif

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

	if (ck_gameState.currentLevel == 0)
	{
		int keenYTilePos = ck_keenObj->posY >> 8;

	}

  for (int i = 0; i < MAXLUMPS; i++)
    if (ck4_lumpsNeeded[i])
      for (int j = ck4_lumpStarts[i]; j <= ck4_lumpEnds[i]; i++)
        CA_CacheGrChunk(j);
}

// Dialog Functions
// These are right after ScanInfoLayer in DOS exe
void CK4_ShowPrincessMessage(void)
{
}

void CK4_ShowJanitorMessage(void)
{
}

void CK4_ShowCantSwimMessage(void)
{
}

void CK4_ShowWetsuitMessage(void)
{
}

char *ck4_councilMessages[] = {
  "No sweat, oh guardian\n"
    "of wisdom!",

  "Sounds like a plan,\n"
    "bearded one!",

  "No problemo.",

  "Great.  You known, you\n"
    "look a lot like the\n"
    "last guy I rescued...",

  "Good idea, Gramps.",

  "May the road rise\n"
    "to meet your feet,\n"
    "Mr. Member.",

  "Wise plan of action,\n"
    "your ancientness.",

  "You're the last one,\n"
    "fella. Let's both\n"
    "get back to the\n"
    "Oracle chamber!"
};

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
  VH_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 110);
  US_SetPrintY(US_GetPrintY() + 6);
  US_SetWindowW(US_GetWindowW() - 0x30);
  US_SetWindowX(US_GetWindowX() + 0x30);

  if (ca_mapOn == 17)
  {
     // Underwater level
    US_CPrint("Ggoh thig you sogh mg\n"
              "fgor regscuing mgge!\n"
              "I'gll regur tgo the\n"
              "Goracle chagber\n"
              "igmediatggely. Blub.");
  }
  else
  {
    US_CPrint("Oh thank you so much\n"
              "for rescuing me!\n"
              "I'll return to the\n"
              "Oracle chamber\n"
              "immediately.");
  }

  VL_Present();
  // VL_WaitVBL(60);
  IN_ClearKeysDown();
  IN_WaitButton();
  US_CenterWindow(26, 8);
  VH_DrawBitmap(US_GetWindowX() + US_GetWindowW() - 0x30, US_GetWindowY(), 111);
  US_SetWindowW(US_GetWindowW() - 0x30);
  US_SetPrintY(US_GetPrintY() + 12);
  US_CPrint(ck4_councilMessages[ck_gameState.ep.ck4.membersRescued]);
  VL_Present(); // VW_UpdateScreen();
  // VW_WaitVBL(30);
  IN_ClearKeysDown();
  IN_WaitButton();
  VH_DrawBitmap(US_GetWindowX() + US_GetWindowW(), US_GetWindowY(), 112);
  VL_Present(); // VW_UpdateScreen();
  // VW_WaitVBL(30);
  IN_WaitButton();
  ck_gameState.ep.ck4.membersRescued++;
  CA_DownLevel();
  StopMusic();

}

// Scuba Keen
void CK4_KeenSwim(CK_object *obj)
{
}

void CK4_KeenSwimFast(CK_object *obj)
{
}

void CK4_KeenSwimCol(CK_object *a, CK_object *b)
{
}

void CK4_KeenSwimDraw(CK_object *obj)
{
}


