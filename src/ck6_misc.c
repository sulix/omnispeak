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
#include "ck6_ep.h"
#include <stdio.h>

CK_EpisodeDef ck6_episode ={
  EP_CK6,
	"CK6",
	&CK6_SetupFunctions,
	&CK6_ScanInfoLayer,
  &CK6_DefineConstants,
  &CK6_MapMiscFlagsCheck,
  4,
  15,
  14,
  0x75CE,
  0x939E,
};


// Contains some keen-5 specific functions.

//StartSprites +
int16_t CK6_ItemSpriteChunks[] ={
	224, 226, 228, 230,
	210, 212, 214, 216, 218, 220,
	222, 233, 207
};

void CK6_SetupFunctions()
{
	//Quick hack as we haven't got a deadly function yet
#if 0
	CK6_Obj1_SetupFunctions();
	CK6_Obj2_SetupFunctions();
	CK6_Obj3_SetupFunctions();
	CK6_Map_SetupFunctions();
#endif
}

// ck_inter.c
uint8_t ck6_starWarsPalette[] = {
	0x00, 0x01, 0x18, 0x1E, 0x1F, 0x1C, 0x06, 0x07,
	0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x00};

// ck_game.c
const char *ck6_levelEntryTexts[] = {
	"Keen purposefully\n"
	"wanders about the\n"
	"Omegamatic",

	"Keen investigates the\n"
	"Ion Ventilation System",

	"Keen struts through\n"
	"the Security Center",

	"Keen invades\n"
	"Defense Tunnel Vlook",

	"Keen engages\n"
	"Energy Flow Systems",

	"Keen barrels into\n"
	"Defense Tunnel Burrh",

	"Keen goes nuts in\n"
	"the Regulation\n"
	"Control Center",

	"Keen regrets entering\n"
	"Defense Tunnel Sorra",

	"Keen blows through\n"
	"the Neutrino\n"
	"Burst Injector",

	"Keen trots through\n"
	"Defense Tunnel Teln",

	"Keen breaks into\n"
	"the Brownian\n"
	"Motion Inducer",

	"Keen hurries through\n"
	"the Gravitational\n"
	"Damping Hub",

	"Keen explodes into\n"
	"the Quantum\n"
	"Explosion Dynamo",

	"Keen faces danger\n"
	"in the secret\n"
	"Korath III Base",

	"Keen will not be\n"
	"in the BWBMegarocket",

	"Keen unexplainedly\n"
	"find himself by\n"
	"theHigh Scores",
};

const char *ck6_levelNames[] = {
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

// ck_keen.c

/*
soundnames ck6_itemSounds[]  = { SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM, SOUND_GOTGEM,
                               SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,SOUND_GOTITEM,
                               SOUND_GOTEXTRALIFE, SOUND_GOTSTUNNER, SOUND_GOTKEYCARD
};
*/
soundnames ck6_itemSounds[]  = { 19, 19, 19, 19, 8,8,8,8,8,8, 17, 9, 55 };
uint16_t ck6_itemShadows[] = {232, 232, 232, 232, 195, 196, 197,  198,  199,  200, 201, 202, 209};

// ck_map.c
int ck6_mapKeenFrames[] = { 0xF7, 0x106, 0xF4, 0xFD, 0xFA, 0x100, 0xF1, 0x103 };

// ck_play.c
int16_t ck6_levelMusic[] ={11, 5, 7, 9, 10, 9, 10, 9, 10, 9, 10, 3, 13, 4, 12, 2, 6, 1, 0, 8};

void CK6_DefineConstants(void)
{
  FON_MAINFONT = 3;
  FON_WATCHFONT = 4;

  PIC_HELPMENU = 6;
  PIC_ARROWDIM = 26;
  PIC_ARROWBRIGHT = 27;
  PIC_HELPPOINTER = 24;
  PIC_BORDERTOP = 28;
  PIC_BORDERLEFT = 29;
  PIC_BORDERRIGHT = 30;
  PIC_BORDERBOTTOMSTATUS = 31;
  PIC_BORDERBOTTOM = 32;

  PIC_MENUCARD = 11;
  PIC_NEWGAMECARD = 12;
  PIC_LOADCARD = 13;
  PIC_SAVECARD = 14;
  PIC_CONFIGURECARD = 15;
  PIC_SOUNDCARD = 16;
  PIC_MUSICCARD = 17;
  PIC_KEYBOARDCARD = 18;
  PIC_MOVEMENTCARD = 19;
  PIC_BUTTONSCARD = 20;
  PIC_JOYSTICKCARD = 21;
  PIC_OPTIONSCARD = 22;
  PIC_PADDLEWAR = 23;
  // PIC_DEBUGCARD = 32;

  PIC_WRISTWATCH = 26;
  PIC_CREDIT1 = 29;
  PIC_CREDIT2 = 30;
  PIC_CREDIT3 = 31;
  PIC_CREDIT4 = 32;

  PIC_STARWARS = 33;
  PIC_TITLESCREEN = 34;
  PIC_COUNTDOWN5 = 92;
  PIC_COUNTDOWN4 = 93;
  PIC_COUNTDOWN0 = 97;

  MPIC_WRISTWATCHSCREEN = 43;
  MPIC_STATUSLEFT = 44;
  MPIC_STATUSRIGHT = 45;

  SPR_PADDLE = 46;
  SPR_BALL0 = 47;
  SPR_BALL1 = 48;
  SPR_BALL2 = 49;
  SPR_BALL3 = 50;

  SPR_DEMOSIGN = 51;

  SPR_STARS1 = 86;

  SPR_CENTILIFE1UPSHADOW = 146;

  SPR_SECURITYCARD_1 = 207;
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

  SPR_SCOREBOX = 175;

  SPR_MAPKEEN_WALK1_N = 190;
  SPR_MAPKEEN_STAND_N = 192;
  SPR_MAPKEEN_STAND_NE = 207;
  SPR_MAPKEEN_STAND_E = 189;
  SPR_MAPKEEN_STAND_SE = 198;
  SPR_MAPKEEN_WALK1_S = 193;
  SPR_MAPKEEN_STAND_S = 195;
  SPR_MAPKEEN_STAND_SW = 201;
  SPR_MAPKEEN_STAND_W = 186;
  SPR_MAPKEEN_STAND_NW = 204;

  // TEXT_HELPMENU = 4914;
  // TEXT_CONTROLS = 4915;
  // TEXT_STORY = 4916;
  // TEXT_ABOUTID = 4917;
  TEXT_END = 5550;
  // TEXT_SECRETEND = 4919;
  // TEXT_ORDER = 4920;

  EXTERN_ORDERSCREEN = 5551;
  EXTERN_COMMANDER = 5552;
  EXTERN_KEEN = 5553;

  DEMOSTART = 5555;

  SOUND_KEENWALK0 = 0;
  SOUND_KEENWALK1 = 1;
  SOUND_KEENJUMP = 2;
  SOUND_KEENLAND = 3;
  SOUND_KEENSHOOT = 4;
  SOUND_MINEEXPLODE = 5;
  SOUND_SLICEBUMP = 6;
  SOUND_KEENPOGO = 7;
  SOUND_GOTITEM = 8;
  SOUND_GOTSTUNNER = 9;
  SOUND_GOTCENTILIFE = 10;
  SOUND_UNKNOWN11 = 11;
  SOUND_UNKNOWN12 = 12;
  SOUND_LEVELEXIT = 13;
  SOUND_NEEDKEYCARD = 14;
  SOUND_KEENHITCEILING = 15;
  SOUND_SPINDREDFLYUP = 16;
  SOUND_GOTEXTRALIFE = 17;
  SOUND_OPENSECURITYDOOR = 18;
  SOUND_GOTGEM = 19;
  SOUND_KEENFALL = 20;
  SOUND_KEENOUTOFAMMO = 21;
  SOUND_UNKNOWN22 = 22;
  SOUND_KEENDIE = 23;
  SOUND_UNKNOWN24 = 24;
  SOUND_KEENSHOTHIT = 25;
  SOUND_UNKNOWN26 = 26;
  SOUND_SPIROSLAM = 27;
  SOUND_SPINDREDSLAM = 28;
  SOUND_ENEMYSHOOT = 29;
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
  //SOUND_UNKNOWN43 = 43;
  //SOUND_UNKNOWN44 = 44;
  SOUND_BARKSHOTDIE = 45;
  SOUND_KEENPADDLE = 46;
  SOUND_PONGWALL = 47;
  SOUND_COMPPADDLE = 48;
  SOUND_COMPSCORE = 49;
  SOUND_KEENSCORE = 50;
  SOUND_UNKNOWN51 = 51;
  SOUND_UNKNOWN52 = 52;
  SOUND_GALAXYEXPLODE = 53;
  SOUND_GALAXYEXPLODEPRE = 54;
  SOUND_GOTKEYCARD = 55;
  SOUND_UNKNOWN56 = 56;
  SOUND_KEENLANDONFUSE = 57;
  SOUND_SPARKYPREPCHARGE = 58;
  SOUND_SPHEREFULCEILING = 59;
  SOUND_OPENGEMDOOR = 60;
  SOUND_SPIROFLY = 61;
  SOUND_UNKNOWN62 = 62;
  SOUND_UNKNOWN63 = 63;
  LASTSOUND = 64;

  STR_EXIT_TO_MAP = "Exit to Armageddon";

  // ck_inter.c
  ck_starWarsPalette = ck6_starWarsPalette;

  // ck_game.c
  ck_levelEntryTexts = ck6_levelEntryTexts;
  ck_levelNames = ck6_levelNames;

  // ck_keen.c
  ck_itemSounds = ck6_itemSounds;
  ck_itemShadows = ck6_itemShadows;

  // ck_map.c
  ck_mapKeenFrames = ck6_mapKeenFrames;

  // ck_play.c
  ck_levelMusic = ck6_levelMusic;

}

/*
 * Spawn an enemy projectile
 * Note that the behaviour is slightly different from DOS Keen
 * DOS Keen SpawnEnemyShot returns 0 if shot is spawned, or -1 otherwise
 * omnispeak CK6_SpawnEnemyShot returns pointer if succesful, NULL otherwise
 */

#if 0
CK_object *CK6_SpawnEnemyShot(int posX, int posY, CK_action *action)
{
	CK_object *new_object = CK_GetNewObj(true);

	if (!new_object)
		return NULL;

	new_object->posX = posX;
	new_object->posY = posY;
	new_object->type = CT6_EnemyShot;
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
#endif

void CK6_SpawnAxisPlatform(int tileX, int tileY, int direction, bool purple)
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
		CK_SetAction(obj, CK_GetActionByName("CK6_ACT_purpleAxisPlatform"));
	}
	else
	{

		CK_SetAction(obj, CK_GetActionByName("CK_ACT_AxisPlatform"));
	}
	// TODO: These should *not* be done here.
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

void CK6_SpawnRedStandPlatform(int tileX, int tileY)
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
	CK_SetAction(obj, CK_GetActionByName("CK6_ACT_redStandPlatform"));
	obj->gfxChunk = obj->currentAction->chunkLeft;
	CA_CacheGrChunk(obj->gfxChunk);
	CK_ResetClipRects(obj);
}

// TODO: Cache stuff here instead of spawner handlers
void CK6_ScanInfoLayer()
{

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
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				break;
			case 2:
				CK_SpawnKeen(x, y, -1);
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				break;

			case 3:
				CK_DemoSignSpawn();
				ca_graphChunkNeeded[0xEB] |= ca_levelbit;
				if (ck_gameState.levelState != LS_TeleportToKorath)
					CK_SpawnMapKeen(x, y);
				break;

#if 0
			case 25:
				RF_SetScrollBlock(x, y, true);
				break;
			case 26:
				RF_SetScrollBlock(x, y, false);
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
				break;
			case 70:
				CK_SpawnItem(x, y, infoValue - 58); // Omegamatic Keycard
				break;
#endif
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

