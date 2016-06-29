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

//StartSprites +
int16_t CK4_ItemSpriteChunks[] ={
	224, 226, 228, 230,
	210, 212, 214, 216, 218, 220,
	222, 233, 207
};

void CK4_PointItem(CK_object *obj)
{
	//	obj->timeUntillThink = 20;
	obj->visible = true;
	if (++obj->gfxChunk == obj->user3)
		obj->gfxChunk = obj->user2;
}

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
	new_object->type = CT_Platform;
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

// MISC Keen 5 functions

// Level Ending Object Spawn


void CK4_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
	CK_ACT_AddColFunction("CK_DeadlyCol", &CK_DeadlyCol);
	// CK4_Obj1_SetupFunctions();
	// CK4_Obj2_SetupFunctions();
	// CK4_Obj3_SetupFunctions();
	CK4_Map_SetupFunctions();
	CK_ACT_AddFunction("CK_Fall", &CK_Fall);
	CK_ACT_AddFunction("CK_Fall2", &CK_Fall2);
	CK_ACT_AddFunction("CK_Glide", &CK_Glide);
	CK_ACT_AddFunction("CK_BasicDrawFunc1", &CK_BasicDrawFunc1);
	CK_ACT_AddFunction("CK_BasicDrawFunc2", &CK_BasicDrawFunc2);
	CK_ACT_AddFunction("CK_BasicDrawFunc4", &CK_BasicDrawFunc4);
	CK_ACT_AddFunction("CK4_PointItem", &CK4_PointItem);
}

// HACK: Sorry, the strings need to be in WRITABLE storage,
// because US_CPrint (temporarily) modifies them.

// ck_game.c
char ck4_levelEntryText_0[] =
	"Keen purposefully\n"
	"wanders about the\n"
	"Omegamatic";

char ck4_levelEntryText_1[] =
	"Keen investigates the\n"
	"Ion Ventilation System";

char ck4_levelEntryText_2[] =
	"Keen struts through\n"
	"the Security Center";

char ck4_levelEntryText_3[] =
	"Keen invades\n"
	"Defense Tunnel Vlook";

char ck4_levelEntryText_4[] =
	"Keen engages\n"
	"Energy Flow Systems";

char ck4_levelEntryText_5[] =
	"Keen barrels into\n"
	"Defense Tunnel Burrh";

char ck4_levelEntryText_6[] =
	"Keen goes nuts in\n"
	"the Regulation\n"
	"Control Center";

char ck4_levelEntryText_7[] =
	"Keen regrets entering\n"
	"Defense Tunnel Sorra";

char ck4_levelEntryText_8[] =
	"Keen blows through\n"
	"the Neutrino\n"
	"Burst Injector";

char ck4_levelEntryText_9[] =
	"Keen trots through\n"
	"Defense Tunnel Teln";

char ck4_levelEntryText_10[] =
	"Keen breaks into\n"
	"the Brownian\n"
	"Motion Inducer";

char ck4_levelEntryText_11[] =
	"Keen hurries through\n"
	"the Gravitational\n"
	"Damping Hub";

char ck4_levelEntryText_12[] =
	"Keen explodes into\n"
	"the Quantum\n"
	"Explosion Dynamo";

char ck4_levelEntryText_13[] =
	"Keen faces danger\n"
	"in the secret\n"
	"Korath III Base";

char ck4_levelEntryText_14[] =
	"Keen will not be\n"
	"in the BWBMegarocket";

char ck4_levelEntryText_15[] =
	"Keen unexplainedly\n"
	"find himself by\n"
	"theHigh Scores";

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
};

const char *ck4_levelNames[] = {
	"Omegamatic",
	"Ion Ventilation System",
	"Security Center",
	"Defense Tunnel Vlook",
	"Energy Flow Systems",
	"Defense Tunnel Burrh",
	"Regulation\nControl Center",
	"Defense Tunnel Sorra",
	"Neutrino\nBurst Injector",
	"Defense Tunnel Teln",
	"Brownian\nMotion Inducer",
	"Gravitational\nDamping Hub",
	"Quantum\nExplosion Dynamo",
	"Korath III Base",
	"BWBMegarocket",
	"High Scores",
};

/*
soundnames ck4_itemSounds[]  = { SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM,
                               SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,
                               SOUND_GOTEXTRALIFE, SOUND_GOTSTUNNER,
};
*/
soundnames ck4_itemSounds[]  = { 19, 19, 19, 19, 8,8,8,8,8,8, 17, 9, 55 };
int16_t ck4_itemShadows[] = {250, 250, 250, 250, 219, 220, 221, 222, 223, 224, 225, 226};

// ck_map.c
int ck4_mapKeenFrames[] = { 0x103, 0x112, 0x100, 0x109, 0x106, 0x10c, 0xFD, 0x10f };

// ck_play.c
uint16_t ck4_levelMusic[] ={0, 4, 3, 3, 2, 2, 4, 3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 3, -1};

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

  /*
  SPR_GEM_A1 = 224;
  SPR_GEM_B1 = 226;
  SPR_GEM_C1 = 228;
  SPR_GEM_D1 = 230;
  SPR_100_PTS1 = 210;
  SPR_200_PTS1 = 212;
  SPR_500_PTS1 = 214;
  SPR_1000_PTS1 = 216;
  SPR_2000_PTS1 = 218;
  SPR_5000_PTS1 = 220;
  SPR_1UP1 = 222;
  SPR_STUNNER1 = 233;
  */

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
  TEXT_CONTROLS = 4740;
  TEXT_STORY = 4741;
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
  SOUND_GOTVITALIN = 10;
  SOUND_UNKNOWN11 = 11;
  SOUND_UNKNOWN12 = 12;
  SOUND_LEVELEXIT = 13;
  SOUND_NEEDKEYCARD = 14;
  SOUND_KEENHITCEILING = 15;
  // SOUND_SPINDREDFLYUP = 16;
  SOUND_GOTEXTRALIFE = 17;
  SOUND_OPENSECURITYDOOR = 18;
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
//   SOUND_UNKNOWN52 = 52;
//   SOUND_GALAXYEXPLODE = 53;
//   SOUND_GALAXYEXPLODEPRE = 54;
//   SOUND_GOTKEYCARD = 55;
//   SOUND_UNKNOWN56 = 56;
//   SOUND_KEENLANDONFUSE = 57;
//   SOUND_SPARKYPREPCHARGE = 58;
//   SOUND_SPHEREFULCEILING = 59;
//   SOUND_OPENGEMDOOR = 60;
//   SOUND_SPIROFLY = 61;
//   SOUND_UNKNOWN62 = 62;
//   SOUND_UNKNOWN63 = 63;
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
	new_object->type = CT_EnemyShot;
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

void CK4_SpawnItem(int tileX, int tileY, int itemNumber)
{

	CK_object *obj = CK_GetNewObj(false);

	obj->clipped = CLIP_not;
	//obj->active = OBJ_ACTIVE;
	obj->zLayer = 2;
	obj->type = CT_Item;	//OBJ_ITEM
	obj->posX = tileX << 8;
	obj->posY = tileY << 8;
	obj->yDirection = -1;
	obj->user1 = itemNumber;
	obj->gfxChunk = CK4_ItemSpriteChunks[itemNumber];
	obj->user2 = obj->gfxChunk;
	obj->user3 = obj->gfxChunk + 2;
	CK_SetAction(obj, CK_GetActionByName("CK4_act_item") );
	// TODO: Wrong place to cache?
	CA_CacheGrChunk(obj->gfxChunk);
	CA_CacheGrChunk(obj->gfxChunk + 1);
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
void CK4_ScanInfoLayer()
{

	//TODO: Work out where to store current map number, etc.
	int mapW = CA_MapHeaders[ca_mapOn]->width;
	int mapH = CA_MapHeaders[ca_mapOn]->height;

	ck_gameState.fusesRemaining = 0;

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
				CK4_SpawnItem(x, y, infoValue - 57);
				break;
			case 70:
				CK4_SpawnItem(x, y, infoValue - 58); // Omegamatic Keycard
				break;

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
	// TODO: Some more stuff (including opening elevator after breaking fuses)

	if (ck_gameState.currentLevel == 0)
	{
		int keenYTilePos = ck_keenObj->posY >> 8;

	}
}
