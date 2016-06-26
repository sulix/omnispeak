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

#ifndef CK_DEF_H
#define CK_DEF_H

#include <stdbool.h>

#include "ck_phys.h"
#include "id_heads.h"

#define CK_MAX_OBJECTS 100

struct CK_object;

struct RF_SpriteDrawEntry;

typedef enum CK_Difficulty
{
	D_NotPlaying = 0,
	D_Easy = 1,
	D_Normal = 2,
	D_Hard = 3
} CK_Difficulty;

// see WL_DEF.H
typedef enum CK_Controldir {
        CD_north = 0,
        CD_east = 1,
        CD_south = 2,
        CD_west = 3
} CK_Controldir;

typedef enum CK_Dir {
        Dir_east = 0,
        Dir_northeast = 1,
        Dir_north = 2,
        Dir_northwest = 3,
        Dir_west = 4,
        Dir_southwest = 5,
        Dir_south = 6,
        Dir_southeast = 7,
        Dir_nodir = 8
} CK_Dir;

typedef enum CK_ClassType {
	CT_nothing = 0,
	CT_Friendly = 1,
	CT_Player = 2,
	CT_Stunner,
	CT_EnemyShot,
  CT_Item = 5,
	CT_Platform = 6,
	CT_StunnedCreature = 7,
	CT_MapFlag = 8,
	CT_Sparky = 9,
	CT_Mine = 10,
	CT_SliceStar = 11,
	CT_Robo = 12,
	CT_Spirogrip = 13,
	CT_Ampton = 14,
	CT_Volte = 16,
	CT_Spindred = 18,
	CT_Master = 19,
	CT_Shikadi = 20,
	CT_Shocksund = 21,
	CT_Sphereful = 22,
	CT_Korath = 23,
	CT_QED = 25,
} CK_ClassType;

typedef enum CK_LevelState
{
  LS_Playing = 0,             // In Level
  LS_Died = 1,                // Keen Died
  LS_LevelComplete = 2,       // Level Completed
  LS_CouncilRescued = 3,      // Rescued Council Member (Keen 4)
  LS_AboutToRecordDemo = 4,   // About to Record Demo
  LS_DestroyedQED = 15,       // Destroyed QED (Keen 5)
} CK_LevelState;

// This struct is 0x58 bytes big in Keen5
// It must be preserved if omnispeak savegames are to be compatible
// with those of vanilla keen
typedef struct CK_GameState
{
	uint16_t mapPosX;
	uint16_t mapPosY;
	uint16_t levelsDone[25];  // Number of levels complete
	int32_t keenScore;			// Keen's score. (This _is_ signed, by the looks of all the 'jl' instructions)
	int32_t nextKeenAt;			// Score keen will get a new life at.
	int16_t numShots;
	int16_t numVitalin;
	int16_t securityCard;
	int16_t word_4729C;
	int16_t fusesRemaining;
	int16_t keyGems[4];
	int16_t currentLevel;
	int16_t numLives;			// Number of lives keen has.
	CK_Difficulty difficulty;		// Difficulty level of current game
	//struct CK_object *platform;  // This was a 16-bit pointer in DOS Keen5.exe

	int levelState;				// Level State (should probably be enum)
	bool jumpCheat;				// Is the jump cheat enabled? (Not in Keen5 gamestate struct)
} CK_GameState;

extern CK_GameState ck_gameState;

typedef enum CK_ActionType
{
	AT_UnscaledOnce,			// Unscaled Motion, Thinks once.
	AT_ScaledOnce,				// Scaled Motion, Thinks once.
	AT_Frame,					// No Motion, Thinks each frame (doesn't advance action)
	AT_UnscaledFrame,			// Unscaled Motion, Thinks each frame
	AT_ScaledFrame				// Scaled Motion, Thinks each frame
} CK_ActionType;

typedef struct CK_action
{
	int16_t chunkLeft;
	int16_t chunkRight;
	CK_ActionType type;
	int16_t protectAnimation, stickToGround; 	// See KeenWiki: Galaxy Action Parameters (lemm/levelass)
	int16_t timer;
	int16_t velX, velY;
	void (*think)(struct CK_object *obj);
	void (*collide)(struct CK_object *obj, struct CK_object *other);
	void (*draw)(struct CK_object *obj);
	struct CK_action *next;
} CK_action;

typedef enum CK_objActive
{
	OBJ_INACTIVE = 0,
	OBJ_ACTIVE = 1,
	OBJ_ALWAYS_ACTIVE = 2,
	OBJ_EXISTS_ONLY_ONSCREEN = 3
} CK_objActive;

typedef enum CK_clipped
{
	CLIP_not = 0,
	CLIP_normal,
	CLIP_simple,
} CK_ClipType;

typedef struct CK_object
{
	int type;
	CK_objActive active;
	bool visible;
	CK_ClipType clipped;
	int timeUntillThink;
	int posX;
	int posY;
	int xDirection;
	int yDirection;
	int deltaPosX;
	int deltaPosY;
	int velX;
	int velY;
	int actionTimer;
	CK_action *currentAction;
	int gfxChunk;
	int zLayer;

	CK_objPhysData clipRects;

	// In real keen, these are shared, so we're moving them out.
#if 0
	CK_objPhysData oldRects;

	CK_objPhysData deltaRects;

	int nextX;
	int nextY;
#endif

	//TileInfo for surrounding tiles.
	int topTI, bottomTI, leftTI, rightTI;


	struct RF_SpriteDrawEntry *sde;

	intptr_t user1;
	intptr_t user2;
	intptr_t user3;
	intptr_t user4;

	struct CK_object *next;
	struct CK_object *prev;
} CK_object;

extern CK_objPhysData ck_oldrects;
extern CK_objPhysData ck_deltarects;
extern int ck_nextX;
extern int ck_nextY;

typedef struct CK_keenState
{
	int jumpTimer;
	int poleGrabTime;
	bool jumpIsPressed;
	bool jumpWasPressed;
	bool pogoIsPressed;
	bool pogoWasPressed;
	bool shootIsPressed;
	bool shootWasPressed;

	bool keenSliding;
	CK_object *platform;
} CK_keenState;

extern CK_keenState ck_keenState;
extern IN_ControlFrame ck_inputFrame;

typedef struct CK_HighScore
{
	char name[58];
	uint32_t score;
	uint16_t arg4;
} CK_HighScore;

extern CK_HighScore ck_highScores[8];

void CK_SpawnKeen(int tileX, int tileY, int direction);
void CK_ShotHit(CK_object *obj);

void CK_HandleDemoKeys();
void CK_KeenRidePlatform(CK_object *obj);
void CK_KeenSetupFunctions();

void StartMusic(uint16_t level);
void StopMusic(void);

void CK_NewGame(void);
void CK_ExitMenu(void);

/* ck_inter.c */
extern bool ck_inHighScores;

void CK_DrawTerminator(void);
void CK_DrawStarWars(void);

void CK_ShowTitleScreen();

void CK_PlayDemoFile(const char *demoName);

void CK_OverlayHighScores();
void CK_SubmitHighScore(int score, uint16_t arg4);
void CK_DoHighScores();

/* ck_keen.c */
soundnames *ck_itemSounds;
uint16_t *ck_itemShadows;

void CK_IncreaseScore(int score);
void CK_KillKeen();

extern CK_object *ck_keenObj;

void CK_OBJ_SetupFunctions();

/* ck_main.c */
void CK_MeasureMultiline(const char *str, uint16_t *w, uint16_t *h);

/* ck_misc.c */
void CK_Fall(CK_object *obj);
void CK_Fall2(CK_object *obj);
void CK_Glide(CK_object *obj);
void CK_BasicDrawFunc1(CK_object *obj);
void CK_BasicDrawFunc2(CK_object *obj);
void CK_BasicDrawFunc4(CK_object *obj);
void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act);
void CK_DeadlyCol(CK_object *o1, CK_object *o2);

/* ck_text.c */
void HelpScreens(void);

/* ck_us_2.c */
void CK_US_UpdateOptionsMenus();


// Constants
extern int16_t FON_MAINFONT;

extern int16_t PIC_STARWARS;
extern int16_t PIC_TITLESCREEN;
extern int16_t PIC_COUNTDOWN5;
extern int16_t PIC_COUNTDOWN4;
extern int16_t PIC_COUNTDOWN0;

extern int16_t MPIC_STATUSLEFT;
extern int16_t MPIC_STATUSRIGHT;

extern int16_t SPR_DEMOSIGN;

extern int16_t SPR_SECURITYCARD_1;
extern int16_t SPR_GEM_A1;
extern int16_t SPR_GEM_B1;
extern int16_t SPR_GEM_C1;
extern int16_t SPR_GEM_D1;
extern int16_t SPR_100_PTS1;
extern int16_t SPR_200_PTS1;
extern int16_t SPR_500_PTS1;
extern int16_t SPR_1000_PTS1;
extern int16_t SPR_2000_PTS1;
extern int16_t SPR_5000_PTS1;
extern int16_t SPR_1UP1;
extern int16_t SPR_STUNNER1;

extern int16_t SPR_SCOREBOX;

extern int16_t SPR_MAPKEEN_WALK1_N;
extern int16_t SPR_MAPKEEN_STAND_N;
extern int16_t SPR_MAPKEEN_STAND_NE;
extern int16_t SPR_MAPKEEN_STAND_E;
extern int16_t SPR_MAPKEEN_STAND_SE;
extern int16_t SPR_MAPKEEN_WALK1_S;
extern int16_t SPR_MAPKEEN_STAND_S;
extern int16_t SPR_MAPKEEN_STAND_SW;
extern int16_t SPR_MAPKEEN_STAND_W;
extern int16_t SPR_MAPKEEN_STAND_NW;

extern int16_t DEMOSTART;

extern int16_t SOUND_KEENWALK0;
extern int16_t SOUND_KEENWALK1;
extern int16_t SOUND_KEENJUMP;
extern int16_t SOUND_KEENLAND;
extern int16_t SOUND_KEENSHOOT;
extern int16_t SOUND_MINEEXPLODE;
extern int16_t SOUND_SLICEBUMP;
extern int16_t SOUND_KEENPOGO;
extern int16_t SOUND_GOTITEM;
extern int16_t SOUND_GOTSTUNNER;
extern int16_t SOUND_GOTVITALIN;
extern int16_t SOUND_UNKNOWN11;
extern int16_t SOUND_UNKNOWN12;
extern int16_t SOUND_LEVELEXIT;
extern int16_t SOUND_NEEDKEYCARD;
extern int16_t SOUND_KEENHITCEILING;
extern int16_t SOUND_SPINDREDFLYUP;
extern int16_t SOUND_GOTEXTRALIFE;
extern int16_t SOUND_OPENSECURITYDOOR;
extern int16_t SOUND_GOTGEM;
extern int16_t SOUND_KEENFALL;
extern int16_t SOUND_KEENOUTOFAMMO;
extern int16_t SOUND_UNKNOWN22;
extern int16_t SOUND_KEENDIE;
extern int16_t SOUND_UNKNOWN24;
extern int16_t SOUND_KEENSHOTHIT;
extern int16_t SOUND_UNKNOWN26;
extern int16_t SOUND_SPIROSLAM;
extern int16_t SOUND_SPINDREDSLAM;
extern int16_t SOUND_ENEMYSHOOT;
extern int16_t SOUND_ENEMYSHOTHIT;
extern int16_t SOUND_AMPTONWALK0;
extern int16_t SOUND_AMPTONWALK1;
extern int16_t SOUND_AMPTONSTUN;
extern int16_t SOUND_UNKNOWN34;
extern int16_t SOUND_UNKNOWN35;
extern int16_t SOUND_SHELLYEXPLODE;
extern int16_t SOUND_SPINDREDFLYDOWN;
extern int16_t SOUND_MASTERSHOT;
extern int16_t SOUND_MASTERTELE;
extern int16_t SOUND_POLEZAP;
extern int16_t SOUND_UNKNOWN41;
extern int16_t SOUND_SHOCKSUNDBARK;
extern int16_t SOUND_UNKNOWN43;
extern int16_t SOUND_UNKNOWN44;
extern int16_t SOUND_BARKSHOTDIE;
extern int16_t SOUND_UNKNOWN46;
extern int16_t SOUND_UNKNOWN47;
extern int16_t SOUND_UNKNOWN48;
extern int16_t SOUND_UNKNOWN49;
extern int16_t SOUND_UNKNOWN50;
extern int16_t SOUND_UNKNOWN51;
extern int16_t SOUND_UNKNOWN52;
extern int16_t SOUND_GALAXYEXPLODE;
extern int16_t SOUND_GALAXYEXPLODEPRE;
extern int16_t SOUND_GOTKEYCARD;
extern int16_t SOUND_UNKNOWN56;
extern int16_t SOUND_KEENLANDONFUSE;
extern int16_t SOUND_SPARKYPREPCHARGE;
extern int16_t SOUND_SPHEREFULCEILING;
extern int16_t SOUND_OPENGEMDOOR;
extern int16_t SOUND_SPIROFLY;
extern int16_t SOUND_UNKNOWN62;
extern int16_t SOUND_UNKNOWN63;
extern int16_t LASTSOUND;

extern int16_t CAMEIN_MUS;
extern int16_t LITTLEAMPTON_MUS;
extern int16_t THEICE_MUS;
extern int16_t SNOOPIN_MUS;
extern int16_t BAGPIPES_MUS;
extern int16_t WEDNESDAY_MUS;
extern int16_t ROCKNOSTONE_MUS;
extern int16_t OUTOFBREATH_MUS;
extern int16_t SHIKADIAIRE_MUS;
extern int16_t DIAMONDS_MUS;
extern int16_t TIGHTER_MUS;
extern int16_t ROBOREDROCK_MUS;
extern int16_t FANFARE_MUS;
extern int16_t BRINGEROFWAR_MUS;
extern int16_t LASTMUSTRACK;

extern char *STR_EXIT_TO_MAP;
#endif
