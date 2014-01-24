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
	D_NotPlaying,
	D_Easy,
	D_Normal,
	D_Hard
} CK_Difficulty;

// see WL_DEF.H
typedef enum CK_Controldir {
        CD_north,
        CD_east,
        CD_south,
        CD_west
} CK_Controldir;

typedef enum CK_Dir {
        Dir_east,
        Dir_northeast,
        Dir_north,
        Dir_northwest,
        Dir_west,
        Dir_southwest,
        Dir_south,
        Dir_southeast,
        Dir_nodir
} CK_Dir;

typedef enum CK_ClassType {

	CT_nothing,
	CT_Friendly,
  CT_Player = 2,
  CT_Stunner,
  CT_EnemyShot,
	CT_Platform = 6,
  CT_StunnedCreature = 7,
  CT_Sparky = 9,
  CT_Mine = 10,
  CT_SliceStar = 11,
  CT_Robo = 12,
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

typedef struct CK_GameState
{
	CK_Difficulty difficulty;		// Difficulty level of current game
	int levelState;				// Level State (should probably be enum)
						// Values:
						// 0 - In Level
						// 1 - Keen Died
						// 2 - Level Completed
						// 3 - Rescued Council Member (Keen 4)
						// 4 - About to Record Demo
						// 5 - ???
						// 6 - ???
						// 15 - Destroyed QED (Keen 5)
	int16_t numShots;
	int32_t keenScore;			// Keen's score. (This _is_ signed, by the looks of all the 'jl' instructions)
	int32_t nextKeenAt;			// Score keen will get a new life at.
	int16_t numLives;			// Number of lives keen has.
	bool jumpCheat;				// Is the jump cheat enabled?
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
	int chunkLeft;
	int chunkRight;
	CK_ActionType type;
	int protectAnimation, stickToGround; 	// See KeenWiki: Galaxy Action Parameters (lemm/levelass)
	int timer;
	int velX, velY;
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
	CLIP_not,
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

	CK_objPhysData oldRects;

	CK_objPhysData deltaRects;

	int nextX;
	int nextY;

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
extern int ck_currentMapNumber;
void CK_SpawnKeen(int tileX, int tileY, int direction);

void CK_HandleDemoKeys();
void CK_KeenRidePlatform(CK_object *obj);
void CK_KeenSetupFunctions();

extern CK_object *ck_keenObj;

#endif
