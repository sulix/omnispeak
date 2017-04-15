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

#include "ck_net.h"

#include "nk5_ep.h"
#include "nk_keen.h"
#include "nk_obj.h"
#include <stdio.h>

CK_EpisodeDef nk5_episode =
{
    EP_NK5,
    "NK5",
    &NK5_SetupFunctions,
    &NK5_ScanInfoLayer,
    &NK5_DefineConstants,
    NULL, // &NK5_MapMiscFlagsCheck,
    &NK5_IsPresent,
    /* .activeLimit = */ 6,
    /* .highScoreLevel = */ 15,
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
    /* .netGame = */ true,
};


// Contains some keen-5 specific functions.

// Check if all the game files are present.
bool NK5_IsPresent()
{
    // User-provided files
    if (!CA_IsFilePresent("EGAGRAPH.NK5"))
        return false;
    if (!CA_IsFilePresent("GAMEMAPS.NK5"))
        return false;
    if (!CA_IsFilePresent("AUDIO.NK5"))
        return false;

    // Omnispeak-provided files
    if (!CA_IsFilePresent("EGAHEAD.NK5"))
        return false;
    if (!CA_IsFilePresent("EGADICT.NK5"))
        return false;
    if (!CA_IsFilePresent("GFXINFOE.NK5"))
        return false;
    if (!CA_IsFilePresent("MAPHEAD.NK5"))
        return false;
    // Map header file may include the tile info
    //if (!CA_IsFilePresent("TILEINFO.CK5"))
    //	return false;
    if (!CA_IsFilePresent("AUDIODCT.NK5"))
        return false;
    if (!CA_IsFilePresent("AUDIOHHD.NK5"))
        return false;
    if (!CA_IsFilePresent("AUDINFOE.NK5"))
        return false;

    if (!CA_IsFilePresent("ACTION.NK5"))
        return false;

    // We clearly have all of the required files.
    return true;
}

// MISC Keen 5 functions

void NK5_SetupFunctions()
{
    NK_KeenSetupFunctions();
    NK_ObjSetupFunctions();
}

const char *nk5_levelEntryTexts[] = {
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

const char *nk5_levelNames[] = {
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
soundnames nk5_itemSounds[]  = { 19, 19, 19, 19, 8,8,8,8,8,8, 17, 9, 55 };
uint16_t nk5_itemShadows[] = {763, 763, 763, 763, 726, 727, 728,  729,  730,  731, 732, 733, 740};

// ck_play.c
int16_t nk5_levelMusic[] ={11, 5, 7, 9, 10, 9, 10, 9, 10, 9, 10, 3, 13, 4, 12, 2, 6, 1, 0, 8};

void NK5_DefineConstants(void)
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

    PIC_MENUCARD = 67;
    PIC_NEWGAMECARD = 68;
    PIC_LOADCARD = 69;
    PIC_SAVECARD = 70;
    PIC_CONFIGURECARD = 71;
    PIC_SOUNDCARD = 72;
    PIC_MUSICCARD = 73;
    PIC_KEYBOARDCARD = 74;
    PIC_MOVEMENTCARD = 75;
    PIC_BUTTONSCARD = 76;
    PIC_JOYSTICKCARD = 77;
    PIC_OPTIONSCARD = 78;
    PIC_PADDLEWAR = 79;
    PIC_DEBUGCARD = 88;

    PIC_WRISTWATCH = 82;
    PIC_CREDIT1 = 83;
    PIC_CREDIT2 = 84;
    PIC_CREDIT3 = 85;
    PIC_CREDIT4 = 86;

    PIC_STARWARS = 87;
    PIC_TITLESCREEN = 88;
    PIC_COUNTDOWN5 = 92;
    PIC_COUNTDOWN4 = 93;
    PIC_COUNTDOWN0 = 97;

    MPIC_WRISTWATCHSCREEN = 99;
    MPIC_STATUSLEFT = 100;
    MPIC_STATUSRIGHT = 101;

    SPR_PADDLE = 30;
    SPR_BALL0 = 31;
    SPR_BALL1 = 32;
    SPR_BALL2 = 33;
    SPR_BALL3 = 34;

    SPR_DEMOSIGN = 0x6B;

    SPR_STARS1 = 143;

    SPR_CENTILIFE1UPSHADOW = 732;

    SPR_BOMB = 738;
    SPR_GEM_A1 = 755;
    SPR_GEM_B1 = 757;
    SPR_GEM_C1 = 759;
    SPR_GEM_D1 = 761;
    SPR_100_PTS1 = 741;
    SPR_200_PTS1 = 743;
    SPR_500_PTS1 = 745;
    SPR_1000_PTS1 = 747;
    SPR_2000_PTS1 = 749;
    SPR_5000_PTS1 = 751;
    SPR_1UP1 = 753;
    SPR_STUNNER1 = 764;

    SPR_SCOREBOX = 766;

    TEXT_HELPMENU = 4914;
    TEXT_CONTROLS = 4915;
    TEXT_STORY = 4916;
    TEXT_ABOUTID = 4917;
    TEXT_END = 4918;
    TEXT_SECRETEND = 4919;
    TEXT_ORDER = 4920;

    EXTERN_ORDERSCREEN = 4921;
    EXTERN_COMMANDER = 4922;
    EXTERN_KEEN = 4923;

    DEMOSTART = 4926;

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

    LASTMUSTRACK = 14;

    // ck_game.c
    ck_levelEntryTexts = nk5_levelEntryTexts;
    ck_levelNames = nk5_levelNames;

    // ck_keen.c
    ck_itemSounds = nk5_itemSounds;
    ck_itemShadows = nk5_itemShadows;

    // ck_play.c
    ck_levelMusic = nk5_levelMusic;
}

/*
 * Spawn an enemy projectile
 * Note that the behaviour is slightly different from DOS Keen
 * DOS Keen SpawnEnemyShot returns 0 if shot is spawned, or -1 otherwise
 * omnispeak CK5_SpawnEnemyShot returns pointer if succesful, NULL otherwise
 */

CK_object *NK5_SpawnEnemyShot(int posX, int posY, CK_action *action)
{
#if 0
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
#endif
}

void NK5_ScanInfoLayer()
{
    // TODO: Work out where to store current map number, etc.
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
                    NK_AddSpawnPoint(x, y, 1, 0);
                    break;
                case 2:
                    NK_AddSpawnPoint(x, y, -1, 0);
                    break;
                case 3:
                    NK_AddSpawnPoint(x, y, -1, 1);
                    break;
                case 4:
                    NK_AddSpawnPoint(x, y, -1, 1);
                    break;

                case 25:
                    RF_SetScrollBlock(x, y, true);
                    break;
                case 26:
                    RF_SetScrollBlock(x, y, false);
                    break;

                case 27:
                case 28:
                case 29:
                case 30:
                    // NK_AxisPlatSpawn(x, y, infoValue - 27, 0);
                    break;

                case 32:
                    // NK_FallPlatSpawn(x, y);
                    break;

                case 33:
                case 34:
                case 35:
                    // NK_StandPlatSpawn(x, y);
                    break;

                case 36:
                case 37:
                case 38:
                case 39:
                    // NK_GoPlatSpawn(x, y, infoValue - 36, 0);
                    break;

                case 40:
                    // NK_SneakPlatSpawn(x, y);
                    break;

                case 53: 
                case 49:
                case 45: 
                    // NK_AutoGunSpawn(x, y, 0); 
                    break;

                case 54:
                case 50:
                case 46:
                    // NK_AutoGunSpawn(x, y, 1);  
                    break;

                case 55:
                case 51:
                case 47: 
                    // NK_AutoGunSpawn(x, y, 2); 
                    break;

                case 56:
                case 52:
                case 48:
                    // NK_AutoGunSpawn(x, y, 3); 
                    break;

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
                    NK_SpawnItem(x, y, infoValue - 57);
                    break;

                case 70:
                    NK_SpawnItem(x, y, infoValue - 58);
                    break;

                case 80:
                case 81:
                case 82:
                case 83:
                    // NK_GoPlatSpawn(x, y, infoValue - 80, 1);
                    break;

                case 84:
                case 85:
                case 86:
                case 87:
                    // NK_AxisPlatSpawn(x, y, infoValue - 84, 1);
                    break;

                default:
                    break;
            }
        }
    }

    // Cache all the sprites
    for (int i = 0; i < ca_gfxInfoE.numSprites; i++)
        CA_MarkGrChunk(ca_gfxInfoE.offSprites + i);

    // Spawn Players
    for (int i = 0; i < net_state->numPlayers; i++)
    {
        NK_SpawnKeen(i, i % net_state->numSpawn);
        net_state->playerStates[i].numShots = 99;
    }
}

