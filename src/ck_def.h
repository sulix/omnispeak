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

#include "id_heads.h"
#include "ck_ep.h"
#include "ck_phys.h"

#define MISCFLAG_POLE 1
#define MISCFLAG_DOOR 2
#define MISCFLAG_DEADLY 3
#define MISCFLAG_SWITCHPLATON 5
#define MISCFLAG_SWITCHPLATOFF 6
#define MISCFLAG_GEMHOLDER0 7
#define MISCFLAG_GEMHOLDER1 8
#define MISCFLAG_GEMHOLDER2 9
#define MISCFLAG_GEMHOLDER3 10
#define MISCFLAG_SWITCHBRIDGE 15
#define MISCFLAG_BRIDGE 18

#define MISCFLAG_ACTIVEZAPPER 19
#define MISCFLAG_TELEPORT 20
#define MISCFLAG_INACTIVEZAPPER 30
#define MISCFLAG_COMPUTER 31
#define MISCFLAG_SECURITYDOOR 32
#define MISCFLAG_LEFTELEVATOR 33
#define MISCFLAG_RIGHTELEVATOR 34

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
typedef enum CK_Controldir
{
	CD_north = 0,
	CD_east = 1,
	CD_south = 2,
	CD_west = 3
} CK_Controldir;

typedef enum CK_Dir
{
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

typedef enum CK_ClassType
{
	CT_Nothing = 0,
	CT_Friendly = 1,
	CT_Player = 2,
	CT_Stunner = 3,

	CT4_Item = 4,
	CT4_Slug = 5,
	CT4_CouncilMember = 6,
	CT4_7 = 7,
	CT4_Egg = 8,
	CT4_Mushroom = 9,
	CT4_Arachnut = 10,
	CT4_Skypest = 11,
	CT4_Wormmouth = 12,
	CT4_Cloud = 13,
	CT4_Berkeloid = 14,
	CT4_Bounder = 15,
	CT4_Inchworm = 16,
	CT4_Foot = 17, // what was this?
	CT4_Lick = 18,
	CT4_Mimrock = 19,
	CT4_Platform = 20,
	CT4_Dopefish = 21,
	CT4_Schoolfish = 22,
	CT4_Sprite = 23,
	CT4_Lindsey = 24,
	CT4_Bolt = 25,
	CT4_Smirky = 26,
	CT4_Bird = 27,
	CT4_0x1C = 28,
	CT4_0x1D = 29,
	CT4_Wetsuit = 30,
	CT4_EnemyShot = 31,
	CT4_Mine = 32,
	CT4_StunnedCreature = 33,
	CT4_MapFlag = 34,
	CT4_Turret = 1, // to make the CT_CLASS macro work

	CT5_EnemyShot = 4,
	CT5_Item = 5,
	CT5_Platform = 6,
	CT5_StunnedCreature = 7,
	CT5_MapFlag = 8,
	CT5_Sparky = 9,
	CT5_Mine = 10,
	CT5_SliceStar = 11,
	CT5_Robo = 12,
	CT5_Spirogrip = 13,
	CT5_Ampton = 14,
	CT5_Turret = 15,
	CT5_Volte = 16,
	CT5_0x11 = 17,
	CT5_Spindred = 18,
	CT5_Master = 19,
	CT5_Shikadi = 20,
	CT5_Shocksund = 21,
	CT5_Sphereful = 22,
	CT5_Korath = 23,
	CT5_QED = 25,

	CT6_EnemyShot = 4,
	CT6_Item = 5,
	CT6_Platform = 6,
	CT6_Bloog = 7,
	CT6_Blooglet = 8,
	CT6_Fleex = 10,
	CT6_Molly = 12,
	CT6_Babobba = 13,
	CT6_Bobba = 14,
	CT6_Nospike = 16,
	CT6_Gik = 17,
	CT6_Turret = 18,
	CT6_Orbatrix = 19,
	CT6_Bip = 20,
	CT6_Flect = 21,
	CT6_Blorb = 22,
	CT6_Ceilick = 23,
	CT6_Bloogguard = 24,
	CT6_Bipship = 26,
	CT6_Sandwich = 27,
	CT6_Rope = 28,
	CT6_Passcard = 29,
	CT6_StunnedCreature = 25,
	CT6_Grabbiter = 30,
	CT6_Rocket = 31,
	CT6_MapCliff = 32,
	CT6_Satellite = 33,
	CT6_SatelliteLoading = 34,
	CT6_MapFlag = 35,
} CK_ClassType;

#define CT_CLASS(type) \
	(ck_currentEpisode->ep == EP_CK4 ? CT4_##type : (ck_currentEpisode->ep == EP_CK5 ? CT5_##type : (ck_currentEpisode->ep == EP_CK6 ? CT6_##type : CT_Nothing)))

typedef enum CK_MiscFlag
{
	MF_Nil,
	MF_Pole,
	MF_Door,
	MF_Deadly,
	MF_Centilife,
	MF_SwitchOff,
	MF_SwitchOn,
	MF_KeyholderA,
	MF_KeyholderB,
	MF_KeyholderC,
	MF_KeyholderD,
	MF_WaterN,
	MF_WaterE,
	MF_WaterS,
	MF_WaterW,
	MF_BridgeSwitch = 15,
	MF_Moon = 16,
	MF_PathArrow = 17,
	MF_Bridge = 18,
	MF_ZapperOn = 19,
	MF_Teleporter = 20,
	MF_Points100,
	MF_Points200,
	MF_Points500,
	MF_Points1000,
	MF_Points2000,
	MF_Points5000,
	MF_1UP,
	MF_Stunner,
	MF_ZapperOff = 30,
	MF_AmptonComputer = 31,
	MF_KeycardDoor = 32,
	MF_MapElevatorLeft = 33,
	MF_MapElevatorRight = 34
} CK_MiscFlag;

typedef enum CK_LevelState
{
	LS_Playing = 0,		  // In Level
	LS_Died = 1,		  // Keen Died
	LS_LevelComplete = 2,     // Level Completed
	LS_CouncilRescued = 3,    // Rescued Council Member (Keen 4)
	LS_AboutToRecordDemo = 4, // About to Record Demo
	LS_Foot = 7,		  // Keen exited level by touching foot (keen 4)
	LS_Sandwich = 9,	  // Keen exied level by getting items (Keen 6)
	LS_Rope = 10,
	LS_Passcard = 11,
	LS_Molly = 12,		  // Keen rescues Molly (Keen 6)
	LS_TeleportToKorath = 13, // Keen teleported to Korath III Base (Keen 5)
	LS_DestroyedQED = 15,     // Destroyed QED (Keen 5)
} CK_LevelState;

// This struct is 0x58 bytes big in Keen5
// It must be preserved (or at least negotiated) if omnispeak savegames are to be compatible
// with those of vanilla keen
typedef struct CK_GameState
{
	uint16_t mapPosX;
	uint16_t mapPosY;
	uint16_t levelsDone[25]; // Number of levels complete
	int32_t keenScore;       // Keen's score. (This _is_ signed, by the looks of all the 'jl' instructions)
	int32_t nextKeenAt;      // Score keen will get a new life at.
	int16_t numShots;
	int16_t numCentilife;

	// The episode-specific variables come here
	// They were probably conditionally compiled in the DOS version
	// so that the Gamestate struct is variably sized between eps.
	union {
		struct
		{
			int16_t wetsuit;
			int16_t membersRescued;
		} ck4;

		struct
		{
			int16_t securityCard;
			int16_t word_4729C;
			int16_t fusesRemaining;
		} ck5;

		struct
		{
			int16_t sandwich;
			int16_t rope; // 1 == collected, 2 == deployed on cliff
			int16_t passcard;
			int16_t inRocket; // true if flying
		} ck6;
	} ep;

	int16_t keyGems[4];
	int16_t currentLevel;
	int16_t numLives;	 // Number of lives keen has.
	CK_Difficulty difficulty; // Difficulty level of current game
	//struct CK_object *platform;  // This was a 16-bit pointer in DOS Keen5.exe

	int levelState; // Level State (should probably be enum)
	bool jumpCheat; // Is the jump cheat enabled? (Not in Keen5 gamestate struct)
} CK_GameState;

extern CK_GameState ck_gameState;

typedef enum CK_ActionType
{
	AT_UnscaledOnce,  // Unscaled Motion, Thinks once.
	AT_ScaledOnce,    // Scaled Motion, Thinks once.
	AT_Frame,	 // No Motion, Thinks each frame (doesn't advance action)
	AT_UnscaledFrame, // Unscaled Motion, Thinks each frame
	AT_ScaledFrame,   // Scaled Motion, Thinks each frame

	AT_NullActionTypeValue = 0x6F42 // FIXME: Ugly hack used for filling ck_play.c:ck_partialNullAction
} CK_ActionType;

typedef struct CK_action
{
	int16_t chunkLeft;
	int16_t chunkRight;
	CK_ActionType type;
	int16_t protectAnimation, stickToGround; // See KeenWiki: Galaxy Action Parameters (lemm/levelass)
	int16_t timer;
	int16_t velX, velY;
	void (*think)(struct CK_object *obj);
	void (*collide)(struct CK_object *obj, struct CK_object *other);
	void (*draw)(struct CK_object *obj);
	struct CK_action *next;
	// Omnispeak - backwards compatibility:
	// Given an instance of this type, stores what would be
	// the 16-bit offset pointer in the dseg while using the
	// original 16-bit DOS executable (corresponding version).
	uint16_t compatDosPointer;
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
	int16_t type;
	CK_objActive active;
	bool visible;
	CK_ClipType clipped;
	uint16_t timeUntillThink;
	uint16_t posX;
	uint16_t posY;
	int16_t xDirection;
	int16_t yDirection;
	int16_t deltaPosX;
	int16_t deltaPosY;
	int16_t velX;
	int16_t velY;
	int16_t actionTimer;
	CK_action *currentAction;
	uint16_t gfxChunk;
	int16_t zLayer;

	CK_objPhysData clipRects;

	//TileInfo for surrounding tiles.
	int16_t topTI, rightTI, bottomTI, leftTI;

	struct RF_SpriteDrawEntry *sde;

	// *** OMNISPEAK - NOTES ABOUT USAGE OF USER VARIABLES: ***
	//
	// The user variables are of type of int16_t for compatibility with
	// saved games, as well, as to be useful with dumper-enabled builds.
	//
	// This is the case even if a user field stores a pointer.
	//
	// If you want to store a pointer in a user field,
	// an appropriate conversion shall be made. A few
	// examples of functions that can be used:
	// - RF_AddSpriteDrawUsing16BitOffset
	// - RF_RemoveSpriteDrawUsing16BitOffset
	// - CK_ConvertObjPointerTo16BitOffset
	// - CK_ConvertObj16BitOffsetToPointer

	int16_t user1, user2, user3, user4;

	struct CK_object *next;
	struct CK_object *prev;
} CK_object;

extern CK_objPhysData ck_oldrects;
extern CK_objPhysDataDelta ck_deltarects;
extern int16_t ck_nextX;
extern int16_t ck_nextY;

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

void StartMusic(int16_t level);
void StopMusic(void);

void CK_NewGame(void);
void CK_ExitMenu(void);

/* ck_inter.c */
extern int ck_startingSavedGame;
extern bool ck_inHighScores;
extern uint8_t *ck_starWarsPalette;
extern uint8_t *ck_terminator_palette1;
extern uint8_t *ck_terminator_palette2;

typedef struct introbmptypestruct
{
	uint16_t height, width;
	uint16_t linestarts[200];
	uint8_t data[];
} introbmptype;

void CK_DrawTerminator(void);
void CK_DrawStarWars(void);

void CK_ShowTitleScreen();

void CK_PlayDemoFile(const char *demoName);

void CK_OverlayHighScores();
void CK_SubmitHighScore(int score, uint16_t arg4);
void CK_DoHighScores();

/* ck_game.c */
bool CK_SaveGame(FILE *fp);
bool CK_LoadGame(FILE *fp, bool fromMenu);

/* ck_keen.c */
extern soundnames *ck_itemSounds;
extern uint16_t ck_itemPoints[];
extern uint16_t *ck_itemShadows;

void CK_IncreaseScore(int score);
void CK_KillKeen();

extern CK_object *ck_keenObj;

void CK_OBJ_SetupFunctions();

/* ck_main.c */
void CK_MeasureMultiline(const char *str, uint16_t *w, uint16_t *h);
void CK_ShutdownID();

/* ck_map.c */
extern int *ck_mapKeenFrames;

void CK_DemoSignSpawn();
void CK_UpdateScoreBox(CK_object *scorebox);
void CK_SpawnMapKeen(int tileX, int tileY);
void CK_AnimateMapTeleporter(int tileX, int tileY);
void CK_MapFlagSpawn(int tileX, int tileY);
void CK_FlippingFlagSpawn(int tileX, int tileY);
void CK_Map_SetupFunctions();

/* ck_misc.c */
void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act);
void CK_Misc_SetupFunctions(void);
void CK_Glide(CK_object *obj);
void CK_BasicDrawFunc2(CK_object *obj);

/* ck_obj.c */
void CK_SpawnItem(int tileX, int tileY, int itemNumber);
void CK_SpawnCentilifeNotify(int tileX, int tileY);
void CK_SpawnAxisPlatform(int tileX, int tileY, int direction, bool purple);
void CK_SpawnFallPlat(int tileX, int tileY);
void CK_SpawnStandPlatform(int tileX, int tileY);
void CK_SpawnGoPlat(int tileX, int tileY, int direction, bool purple);
void CK_GoPlatThink(CK_object *obj);
void CK_SneakPlatSpawn(int tileX, int tileY);
void CK_TurretSpawn(int tileX, int tileY, int direction);
/*** Used for saved games compatibility ***/
uint16_t CK_ConvertObjPointerTo16BitOffset(CK_object *obj);
CK_object *CK_ConvertObj16BitOffsetToPointer(uint16_t offset);

/* ck_text.c */
void HelpScreens(void);

/* ck_us_2.c */
void CK_US_UpdateOptionsMenus();

// Constants
extern int FON_MAINFONT;
extern int FON_WATCHFONT;

extern int PIC_HELPMENU;
extern int PIC_ARROWDIM;
extern int PIC_ARROWBRIGHT;
extern int PIC_HELPPOINTER;
extern int PIC_BORDERTOP;
extern int PIC_BORDERLEFT;
extern int PIC_BORDERRIGHT;
extern int PIC_BORDERBOTTOMSTATUS;
extern int PIC_BORDERBOTTOM;

extern int PIC_MENUCARD;
extern int PIC_NEWGAMECARD;
extern int PIC_LOADCARD;
extern int PIC_SAVECARD;
extern int PIC_CONFIGURECARD;
extern int PIC_SOUNDCARD;
extern int PIC_MUSICCARD;
extern int PIC_KEYBOARDCARD;
extern int PIC_MOVEMENTCARD;
extern int PIC_BUTTONSCARD;
extern int PIC_JOYSTICKCARD;
extern int PIC_OPTIONSCARD;
extern int PIC_PADDLEWAR;
extern int PIC_DEBUGCARD;

extern int PIC_WRISTWATCH;
extern int PIC_CREDIT1;
extern int PIC_CREDIT2;
extern int PIC_CREDIT3;
extern int PIC_CREDIT4;

extern int PIC_STARWARS;
extern int PIC_TITLESCREEN;
extern int PIC_COUNTDOWN5;
extern int PIC_COUNTDOWN4;
extern int PIC_COUNTDOWN0;

extern int MPIC_WRISTWATCHSCREEN;
extern int MPIC_STATUSLEFT;
extern int MPIC_STATUSRIGHT;

extern int SPR_PADDLE;
extern int SPR_BALL0;
extern int SPR_BALL1;
extern int SPR_BALL2;
extern int SPR_BALL3;

extern int SPR_DEMOSIGN;

extern int SPR_STARS1;

extern int SPR_CENTILIFE1UPSHADOW;

extern int SPR_SECURITYCARD_1;
extern int SPR_GEM_A1;
extern int SPR_GEM_B1;
extern int SPR_GEM_C1;
extern int SPR_GEM_D1;
extern int SPR_100_PTS1;
extern int SPR_200_PTS1;
extern int SPR_500_PTS1;
extern int SPR_1000_PTS1;
extern int SPR_2000_PTS1;
extern int SPR_5000_PTS1;
extern int SPR_1UP1;
extern int SPR_STUNNER1;

extern int SPR_SCOREBOX;

extern int SPR_MAPKEEN_WALK1_N;
extern int SPR_MAPKEEN_STAND_N;
extern int SPR_MAPKEEN_STAND_NE;
extern int SPR_MAPKEEN_STAND_E;
extern int SPR_MAPKEEN_STAND_SE;
extern int SPR_MAPKEEN_WALK1_S;
extern int SPR_MAPKEEN_STAND_S;
extern int SPR_MAPKEEN_STAND_SW;
extern int SPR_MAPKEEN_STAND_W;
extern int SPR_MAPKEEN_STAND_NW;

extern int TEXT_HELPMENU;
extern int TEXT_CONTROLS;
extern int TEXT_STORY;
extern int TEXT_ABOUTID;
extern int TEXT_END;
extern int TEXT_SECRETEND;
extern int TEXT_ORDER;

extern int EXTERN_ORDERSCREEN;
extern int EXTERN_KEEN;
extern int EXTERN_COMMANDER;

extern int DEMOSTART;

extern int SOUND_KEENWALK0;
extern int SOUND_KEENWALK1;
extern int SOUND_KEENJUMP;
extern int SOUND_KEENLAND;
extern int SOUND_KEENSHOOT;
extern int SOUND_MINEEXPLODE;
extern int SOUND_SLICEBUMP;
extern int SOUND_KEENPOGO;
extern int SOUND_GOTITEM;
extern int SOUND_GOTSTUNNER;
extern int SOUND_GOTCENTILIFE;
extern int SOUND_UNKNOWN11;
extern int SOUND_UNKNOWN12;
extern int SOUND_LEVELEXIT;
extern int SOUND_NEEDKEYCARD;
extern int SOUND_KEENHITCEILING;
extern int SOUND_SPINDREDFLYUP;
extern int SOUND_GOTEXTRALIFE;
extern int SOUND_OPENSECURITYDOOR;
extern int SOUND_GOTGEM;
extern int SOUND_KEENFALL;
extern int SOUND_KEENOUTOFAMMO;
extern int SOUND_UNKNOWN22;
extern int SOUND_KEENDIE;
extern int SOUND_UNKNOWN24;
extern int SOUND_KEENSHOTHIT;
extern int SOUND_UNKNOWN26;
extern int SOUND_SPIROSLAM;
extern int SOUND_SPINDREDSLAM;
extern int SOUND_ENEMYSHOOT;
extern int SOUND_ENEMYSHOTHIT;
extern int SOUND_AMPTONWALK0;
extern int SOUND_AMPTONWALK1;
extern int SOUND_AMPTONSTUN;
extern int SOUND_UNKNOWN34;
extern int SOUND_UNKNOWN35;
extern int SOUND_SHELLYEXPLODE;
extern int SOUND_SPINDREDFLYDOWN;
extern int SOUND_MASTERSHOT;
extern int SOUND_MASTERTELE;
extern int SOUND_POLEZAP;
extern int SOUND_UNKNOWN41;
extern int SOUND_SHOCKSUNDBARK;
extern int SOUND_FLAGFLIP;
extern int SOUND_FLAGLAND;
extern int SOUND_BARKSHOTDIE;
extern int SOUND_KEENPADDLE;
extern int SOUND_PONGWALL;
extern int SOUND_COMPPADDLE;
extern int SOUND_COMPSCORE;
extern int SOUND_KEENSCORE;
extern int SOUND_UNKNOWN48;
extern int SOUND_UNKNOWN49;
extern int SOUND_UNKNOWN50;
extern int SOUND_UNKNOWN51;
extern int SOUND_UNKNOWN52;
extern int SOUND_GALAXYEXPLODE;
extern int SOUND_GALAXYEXPLODEPRE;
extern int SOUND_GOTKEYCARD;
extern int SOUND_UNKNOWN56;
extern int SOUND_KEENLANDONFUSE;
extern int SOUND_SPARKYPREPCHARGE;
extern int SOUND_SPHEREFULCEILING;
extern int SOUND_OPENGEMDOOR;
extern int SOUND_SPIROFLY;
extern int SOUND_UNKNOWN62;
extern int SOUND_UNKNOWN63;
extern int LASTSOUND;

extern int CAMEIN_MUS;
extern int LITTLEAMPTON_MUS;
extern int THEICE_MUS;
extern int SNOOPIN_MUS;
extern int BAGPIPES_MUS;
extern int WEDNESDAY_MUS;
extern int ROCKNOSTONE_MUS;
extern int OUTOFBREATH_MUS;
extern int SHIKADIAIRE_MUS;
extern int DIAMONDS_MUS;
extern int TIGHTER_MUS;
extern int ROBOREDROCK_MUS;
extern int FANFARE_MUS;
extern int BRINGEROFWAR_MUS;
extern int LASTMUSTRACK;

#endif
