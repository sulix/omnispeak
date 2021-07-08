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

//#inclucd "id_heads.h"
#include "ck_play.h"
#include "id_ca.h"
#include "id_fs.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_sd.h"
#include "id_us.h"
#include "id_vl.h"
#include "ck_def.h"
#include "ck_game.h"

#include "ck_act.h"
#include "ck_text.h"
#include "ck5_ep.h"

#include "ck_cross.h" /* For CK_Cross_SwapLE16 */

#include <stdio.h>  /* for sscanf() */
#include <stdlib.h> /* For abs() */
#include <string.h> /* For memset() */

CK_object ck_objArray[CK_MAX_OBJECTS];

CK_object *ck_freeObject;
CK_object *ck_lastObject;
int ck_numObjects;

CK_object *ck_keenObj;

static CK_object tempObj;

#ifdef CK_ENABLE_PLAYLOOP_DUMPER
FILE *ck_dumperFile;
#endif

// The rectangle within which objects are active.
int ck_activeX0Tile;
int ck_activeY0Tile;
int ck_activeX1Tile;
int ck_activeY1Tile;

void CK_KeenCheckSpecialTileInfo(CK_object *obj);

// Set to enable F10 debugging tools
bool ck_debugActive = false;

CK_GameState ck_gameState;

static bool ck_slowMotionEnabled = false;

// Switch to toggle Camera following keen.
bool ck_scrollDisabled = false;

// Set if game started with /DEMO parm
int ck_demoParm;

// invincibility switch
bool ck_godMode;

// invincibility timer
int16_t ck_invincibilityTimer;

// ScoreBox
bool ck_scoreBoxEnabled;
CK_object *ck_scoreBoxObj;

// Two button firing
bool ck_twoButtonFiring;

// Fix jerky motion
bool ck_fixJerkyMotion;

// SVGA compatibility
bool ck_svgaCompatibility;

// Gamepad
bool ck_gamePadEnabled;

// Pogo timer for two-button firing
int ck_pogoTimer;

// current frame's input state
IN_ControlFrame ck_inputFrame;

// A bunch of global variables from other modules that should be
// handled better, but are just defined here for now

extern int game_in_progress;
extern int load_game_error, ck_startingSavedGame;
extern CK_Difficulty ck_startingDifficulty;
extern CK_object *ck_scoreBoxObj;

extern int ck6_smashScreenDistance;
extern int16_t ck6_smashScreenOfs[];

void CK_CountActiveObjects()
{
	int active = 0;
	int inactive = 0;

	CK_object *current = ck_keenObj;

	while (current)
	{
		if (current->active)
			active++;
		else
			inactive++;
		current = current->next;
	}

	US_CenterWindow(18, 4);
	US_PrintF("Active Objects : %d\n", active);
	US_PrintF("Inactive Object : %d\n", inactive);
	VL_Present();
	IN_WaitButton();
}

void CK_DebugMemory()
{
	US_CenterWindow(16, 10);

	US_CPrint("Memory Usage:\n");
	US_CPrint("-------------\n");
	US_PrintF("In Use      : %dk\n", MM_UsedMemory() / 1024);
	US_PrintF("Blocks      : %d\n", MM_UsedBlocks());
	US_PrintF("Purgable    : %d\n", MM_PurgableBlocks());
	US_PrintF("GFX Mem Used: %dk\n", VL_MemUsed() / 1024);
	US_PrintF("GFX Surfaces: %d\n", VL_NumSurfaces());
	VL_Present();
	IN_WaitButton();
	//MM_ShowMemory();
}

void CK_BeginDemoRecord()
{
	// VW_SyncPages();
	US_CenterWindow(30, 3);
	US_SetPrintY(US_GetPrintY() + 3);
	US_Print("  Record a demo from level (0-21):");
	VL_Present();
	uint16_t saveX = US_GetPrintX();
	uint16_t saveY = US_GetPrintY();
	char str[4];

	if (US_LineInput(saveX, saveY, str, NULL, true, 2, 0))
	{
		int level;
		sscanf(str, "%d", &level);

		ck_gameState.currentLevel = level;
		ck_gameState.levelState = LS_AboutToRecordDemo;

		IN_DemoStartRecording(0x1000);
	}
}

void CK_EndDemoRecord()
{
	char demoFileName[] = "DEMO?.EXT";
	// VW_SyncPages();
	IN_DemoStopPlaying();
	US_CenterWindow(22, 3);

	US_SetPrintY(US_GetPrintY() + 6);

	US_Print("  Save as demo #(0-9):");

	VL_Present();

	uint16_t saveX = US_GetPrintX();
	uint16_t saveY = US_GetPrintY();
	char str[4];

	if (US_LineInput(saveX, saveY, str, NULL, true, 2, 0))
	{
		if (str[0] >= '0' && str[0] <= '9')
		{
			demoFileName[4] = str[0];
			char *fixedFileName = FS_AdjustExtension(demoFileName);
			IN_DemoSaveToFile(fixedFileName, ck_gameState.currentLevel);
		}
		IN_DemoFreeBuffer();
	}
}

void CK_SpriteTest()
{

	// VW_SyncPages();
	US_CenterWindow(30, 17);
	US_CPrint("Sprite Test");
	US_CPrint("-----------");
	int startpy = US_GetPrintY();
	int startpx = (US_GetPrintX() + 0x38) & ~7;

	int startpx_2 = startpx + 0x28;
	US_PrintF(
		"Chunk:\n"
		"Width:\n"
		"Height:\n"
		"Orgx:\n"
		"Orgy:\n"
		"Xl:\n"
		"Yl:\n"
		"Xh:\n"
		"Yh:\n"
		"Shifts:\n"
		"Mem:\n");

	int var8 = US_GetPrintY();
	int chunk = ca_gfxInfoE.offSprites;
	int shifts = 0;
	int selectedChunk = 0;
	int oldShifts = 0;

	do
	{
		// max min chunks
		if (selectedChunk != chunk || oldShifts != shifts)
		{
			if (chunk >= ca_gfxInfoE.offSprites + ca_gfxInfoE.numSprites)
			{
				chunk = ca_gfxInfoE.offSprites + ca_gfxInfoE.numSprites - 1;
			}
			else if (chunk < ca_gfxInfoE.offSprites)
			{
				chunk = ca_gfxInfoE.offSprites;
			}

			VH_SpriteTableEntry *ste = VH_GetSpriteTableEntry(chunk - ca_gfxInfoE.offSprites);
			mm_ptr_t chunk_ptr = ca_graphChunks[chunk];

			VHB_Bar(startpx, startpy, 0x28, var8 - startpy, 0xF);
			US_SetPrintX(startpx);
			US_SetPrintY(startpy);

			US_PrintF("%d\n", chunk);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->width);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->height);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->originX);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->originY);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->xl);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->yl);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->xh);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->yh);
			US_SetPrintX(startpx);
			US_PrintF("%d\n", ste->shifts);
			US_SetPrintX(startpx);

			if (chunk_ptr)
			{
				// Omnispeak: don't store preshifted sprites
				// DOS: memused = (h*w) + (shifts-1) * (h*(w+1))
				// all of this is precomputed and stored in "spritetype" struct on sprite cache in DOS
				// so the DOS game here would read this information out of the spritetype struct
				US_PrintF("%d=", ste->width * ste->height * 5);
			}
			else
			{
				US_PrintF("-----");
			}


			VHB_Bar(startpx_2, startpy, 0x6E, var8 - startpy, 0xF);
			if (chunk_ptr)
			{
				US_SetPrintX(startpx_2);
				US_SetPrintY(startpy);
				US_PrintF("Shift:%d\n", shifts);
				VHB_DrawSprite(startpx_2 + shifts * 2 + 0x10, US_GetPrintY(), chunk);
			}
			oldShifts = shifts;
			selectedChunk = chunk;

		}
		VL_Present();

		IN_WaitKey();
		IN_ScanCode sc = IN_GetLastScan();
		IN_ClearKeysDown();

		if (sc == IN_SC_LeftArrow)
		{
			if (--shifts == -1)
				shifts = 3;
		}
		else if (sc == IN_SC_RightArrow)
		{
			if (++shifts == 4)
				shifts = 0;
		}
		else if (sc == IN_SC_DownArrow)
		{
			chunk--;
		}
		else if (sc == IN_SC_PgDown)
		{
			if ((chunk -= 10) < ca_gfxInfoE.offSprites)
				chunk = ca_gfxInfoE.offSprites;
		}
		else if (sc == IN_SC_UpArrow)
		{
			chunk++;
		}
		else if (sc == IN_SC_PgUp)
		{
			if ((chunk += 10) >= ca_gfxInfoE.offSprites + ca_gfxInfoE.numSprites)
				chunk = ca_gfxInfoE.offSprites + ca_gfxInfoE.numSprites;
		}
		else if (sc == IN_SC_Escape)
		{
			break;
		}

	} while (1);
}

void CK_ItemCheat()
{
	int i;

	for (i = IN_SC_A; i <= IN_SC_Z; i++)
	{
		if (i != IN_SC_B && i != IN_SC_A && i != IN_SC_Z)
		{
			if (IN_GetKeyState(i))
				return;
		}
	}

	US_CenterWindow(20, 7);
	// TODO: PrintY+=2;
	US_CPrint(CK_VAR_GetStr("ck_str_itemCheat"));
	VL_Present();
	IN_WaitButton();
	//RF_Reset();
	ck_gameState.numShots = 99;
	ck_gameState.numLives++;
	ck_gameState.ep.ck5.securityCard = 1;
	ck_gameState.keyGems[0] =
		ck_gameState.keyGems[1] =
			ck_gameState.keyGems[2] =
				ck_gameState.keyGems[3] = 1;
}

void CK_SetupObjArray()
{
	for (int i = 0; i < CK_MAX_OBJECTS; ++i)
	{
		ck_objArray[i].prev = &(ck_objArray[i + 1]);
		ck_objArray[i].next = 0;
	}

	ck_objArray[CK_MAX_OBJECTS - 1].prev = 0;

	ck_freeObject = &ck_objArray[0];
	ck_lastObject = 0;
	ck_numObjects = 0;

	ck_keenObj = CK_GetNewObj(false);

	ck_scoreBoxObj = CK_GetNewObj(false);
}

CK_object *CK_GetNewObj(bool nonCritical)
{
	if (!ck_freeObject)
	{
		if (nonCritical)
		{
			//printf("Warning: No free spots in objarray! Using temp object\n");
			return &tempObj;
		}
		else
			Quit("GetNewObj: No free spots in objarray!");
	}

	CK_object *newObj = ck_freeObject;
	ck_freeObject = ck_freeObject->prev;

	//Clear any old crap out of the struct.
	memset(newObj, 0, sizeof(CK_object));

	if (ck_lastObject)
	{
		ck_lastObject->next = newObj;
	}
	newObj->prev = ck_lastObject;

	newObj->active = OBJ_ACTIVE;
	newObj->clipped = CLIP_normal;

	ck_lastObject = newObj;
	ck_numObjects++;

	return newObj;
}

void CK_RemoveObj(CK_object *obj)
{
	if (obj == ck_keenObj)
	{
		Quit("RemoveObj: Tried to remove the player!");
	}

	// TODO: Make a better spritedraw handler that
	// replaces the user int variables
	RF_RemoveSpriteDraw(&obj->sde);

	if (obj->type == CT_CLASS(StunnedCreature))
	{
		RF_RemoveSpriteDrawUsing16BitOffset(&obj->user3);
	}

	if (obj == ck_lastObject)
		ck_lastObject = obj->prev;
	else
		obj->next->prev = obj->prev;

	obj->prev->next = obj->next;
	//obj->next = 0;

	obj->prev = ck_freeObject;
	ck_freeObject = obj;
	ck_numObjects--;
}

/*** Used for saved games compatibility ***/
#define COMPAT_ORIG_OBJ_SIZE 76
uint16_t CK_ConvertObjPointerTo16BitOffset(CK_object *obj)
{
	if ((obj >= ck_objArray) && (obj < ck_objArray + CK_MAX_OBJECTS))
		return (obj - ck_objArray) * COMPAT_ORIG_OBJ_SIZE + ck_currentEpisode->objArrayOffset;
	if (obj == &tempObj)
		return ck_currentEpisode->tempObjOffset;
	return 0;
}

CK_object *CK_ConvertObj16BitOffsetToPointer(uint16_t offset)
{
	uint16_t objArrayOffset = ck_currentEpisode->objArrayOffset;
	// Original size of each object was 76 bytes
	if ((offset >= objArrayOffset) && (offset < objArrayOffset + COMPAT_ORIG_OBJ_SIZE * CK_MAX_OBJECTS))
		return ck_objArray + (offset - objArrayOffset) / COMPAT_ORIG_OBJ_SIZE;
	if (offset == ck_currentEpisode->tempObjOffset)
		return &tempObj;
	return NULL;
}

// Used for reproduction of vanilla Keen bugs; Does NOT include function pointers
static const CK_action ck_partialNullAction =
	{0, 0, AT_NullActionTypeValue, 0x6C72, 0x6E61, 0x2064, 0x2B43, 0x202B};

int16_t CK_ActionThink(CK_object *obj, int16_t time)
{
	CK_action *lastAction = obj->currentAction;
	// WORKAROUND FOR VANILLA KEEN BUG: The original code does not
	// check if lastAction != NULL. Naively accessing *lastAction
	// may lead to a crash in this case (try no-clip cheat).
	// Returning from the function in case lastAction == NULL
	// resolves this, but we try to emulate original behaviors here.
	const CK_action *action = lastAction ? lastAction : &ck_partialNullAction;

	// ThinkMethod: 2
	if (action->type == AT_Frame)
	{
		if (action->think)
		{
			if (obj->timeUntillThink)
			{
				obj->timeUntillThink--;
			}
			else
			{
				action->think(obj);
			}
		}
		return 0;
	}

	int16_t newTime = time + obj->actionTimer;

	// If we won't run out of time.
	if (action->timer > newTime || !(action->timer))
	{
		obj->actionTimer = newTime;

		if (action->type == AT_ScaledOnce || action->type == AT_ScaledFrame)
		{
			if (obj->xDirection)
			{
				//TODO: Work out what to do with the nextVelocity stuff.
				ck_nextX += action->velX * (time * obj->xDirection);
			}
			if (obj->yDirection)
			{
				ck_nextY += action->velY * (time * obj->yDirection);
			}
		}

		if (action->type == AT_UnscaledFrame || action->type == AT_ScaledFrame)
		{
			if (action->think)
			{
				if (obj->timeUntillThink)
					obj->timeUntillThink--;
				else
				{
					action->think(obj);
				}
			}
		}
		return 0;
	}

	int16_t remainingTime = action->timer - obj->actionTimer;
	newTime -= action->timer;
	obj->actionTimer = 0;

	if (action->type == AT_ScaledOnce || action->type == AT_ScaledFrame)
	{
		if (obj->xDirection)
		{
			ck_nextX += action->velX * (remainingTime * obj->xDirection);
		}
		if (obj->yDirection)
		{
			ck_nextY += action->velY * (remainingTime * obj->yDirection);
		}
	}
	else /*if (action->type == AT_UnscaledOnce || action->type == AT_UnscaledFrame)*/
	{
		if (obj->xDirection)
		{
			ck_nextX += action->velX * obj->xDirection;
		}
		if (obj->yDirection)
		{
			ck_nextY += action->velY * obj->yDirection;
		}
	}

	if (action->think)
	{
		if (obj->timeUntillThink)
			obj->timeUntillThink--;
		else
		{
			action->think(obj);
		}
	}

	// Do NOT use action variable here
	if (lastAction != obj->currentAction)
	{
		if (!obj->currentAction)
		{
			return 0;
		}
	}
	else
	{
		obj->currentAction = lastAction->next;
	}
	return newTime;
}

void CK_RunAction(CK_object *obj)
{
	int16_t oldChunk = obj->gfxChunk;

	//TODO: Check these
	//int16_t oldPosX = obj->posX;
	//int16_t oldPosY = obj->posY;

	obj->deltaPosX = obj->deltaPosY = 0;

	ck_nextX = ck_nextY = 0;

	CK_action *prevAction = obj->currentAction;

	int16_t ticsLeft = CK_ActionThink(obj, SD_GetSpriteSync());

	if (obj->currentAction != prevAction)
	{
		obj->actionTimer = 0;
		prevAction = obj->currentAction;
	}
	while (ticsLeft)
	{
		// WORKAROUND FOR VANILLA KEEN BUG: As in CK_ActionThink, the
		// original code does not check if prevAction != NULL. Naively
		// accessing *prevAction may lead to a crash in this case
		// (try no-clip cheat). Adding a check if prevAction == NULL
		// before entering the loop resolves this, but we try
		// to emulate original behaviors here.
		const CK_action *prevActionToAccess = prevAction ? prevAction : &ck_partialNullAction;

		if (prevActionToAccess->protectAnimation || prevActionToAccess->timer > ticsLeft)
		{
			ticsLeft = CK_ActionThink(obj, ticsLeft);
		}
		else
		{
			ticsLeft = CK_ActionThink(obj, prevActionToAccess->timer - 1);
		}
		// Do NOT use prevActionToAccess here
		if (obj->currentAction != prevAction)
		{
			obj->actionTimer = 0;
			prevAction = obj->currentAction;
		}
	}

	if (!prevAction)
	{
		CK_RemoveObj(obj);
		return;
	}
	if (prevAction->chunkRight)
	{
		obj->gfxChunk = (obj->xDirection > 0) ? prevAction->chunkRight : prevAction->chunkLeft;
	}
	if (obj->gfxChunk == (uint16_t)-1)
	{
		obj->gfxChunk = 0;
	}
#if 0
	if (obj->currentAction->chunkLeft && obj->xDirection <= 0) obj->gfxChunk = obj->currentAction->chunkLeft;
	if (obj->currentAction->chunkRight && obj->xDirection > 0) obj->gfxChunk = obj->currentAction->chunkRight;
#endif
	if (obj->gfxChunk != oldChunk || ck_nextX || ck_nextY || obj->topTI == 0x19)
	{
		if (obj->clipped == CLIP_simple)
			CK_PhysFullClipToWalls(obj);
		else
			CK_PhysUpdateNormalObj(obj);
	}
}

void CK_OverlayForegroundTile(int fgTile, int overlayTile)
{
	// Some random DOSBox memory dump, resolving crashes with NULL pointers and
	// somewhat emulating the behaviors of vanilla Keen in DOSBox (at least Keen 4)
	//
	// Note: CK_WallDebug makes sure dst is never NULL; It's src which may be NULL.
	static const uint16_t nullTile[80] =
		{
			CK_Cross_SwapLE16(0x0162),
			CK_Cross_SwapLE16(0x01A2),
			CK_Cross_SwapLE16(0x0008),
			CK_Cross_SwapLE16(0x0070),
			CK_Cross_SwapLE16(0x0008),
			CK_Cross_SwapLE16(0x0070),
			CK_Cross_SwapLE16(0x0008),
			CK_Cross_SwapLE16(0x0070),
			CK_Cross_SwapLE16(0x0008),
			CK_Cross_SwapLE16(0x0070),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x05F5),
			CK_Cross_SwapLE16(0x1A16),
			CK_Cross_SwapLE16(0x000B),
			CK_Cross_SwapLE16(0x1602),
			CK_Cross_SwapLE16(0xFF55),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1080),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1320),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1120),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1140),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1160),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x11C0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x11E0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1200),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1240),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x12E0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x12E0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1260),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1280),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0xF0A4),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1060),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x0500),
			CK_Cross_SwapLE16(0xC000),
			CK_Cross_SwapLE16(0x14A0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x14C0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x20C8),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x0000),
			CK_Cross_SwapLE16(0x0118),
			CK_Cross_SwapLE16(0x1AAB),
			CK_Cross_SwapLE16(0x01A2),
			CK_Cross_SwapLE16(0x14E0),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1500),
			CK_Cross_SwapLE16(0xF000),
			CK_Cross_SwapLE16(0x1520),
			CK_Cross_SwapLE16(0xF000),
		};

	const uint16_t *src = (uint16_t *)ca_graphChunks[ca_gfxInfoE.offTiles16m + overlayTile];
	uint16_t *dst = (uint16_t *)ca_graphChunks[ca_gfxInfoE.offTiles16m + fgTile];

	src = src ? src : nullTile;

	for (int i = 0; i < 64; i++)
	{
		uint16_t row, overlayMask, overlayColor;
		row = i & 0xF;			   // Row of the tile we're on
		overlayColor = *(src + i + 0x10);  // Get the overlay color plane
		overlayMask = *(src + row);	// Get the overlay mask plane
		*(dst + row) &= overlayMask;       // Mask dest mask w/ overlay mask
		*(dst + 0x10 + i) &= overlayMask;  // Now draw the overlayTile (mask...)
		*(dst + 0x10 + i) |= overlayColor; // (... and draw color plane)
	}
}

// TODO: make this interoperable between episodes
void CK_WallDebug()
{
	// VW_SyncPages();
	US_CenterWindow(24, 3);
	US_PrintCentered("WORKING");
	VL_Present();

	// Cache the slope info foreground tiles
	for (int i = ca_gfxInfoE.offTiles16m + 0x6C; i < ca_gfxInfoE.offTiles16m + 0x6C + 16; i++)
	{
		CA_CacheGrChunk(i);
	}

	// Draw the slope info tile over each foreground tile
	for (int i = 0; i < ca_gfxInfoE.numTiles16m; i++)
	{

		if (ca_graphChunks[i + ca_gfxInfoE.offTiles16m])
		{
			int ti;

			ti = TI_ForeTop(i) & 7;
			if (ti)
				CK_OverlayForegroundTile(i, ti + 0x6B);

			ti = TI_ForeBottom(i) & 7;
			if (ti)
				CK_OverlayForegroundTile(i, ti + 0x73);

			ti = TI_ForeRight(i) & 7;
			if (ti > 1)
			{
				char buf[128];
				sprintf(buf, "CK_WallDebug: East wall other than 1:%d", i);
				Quit(buf);
			}
			else if (ti == 1)
			{
				CK_OverlayForegroundTile(i, ti + 0x72);
			}

			ti = TI_ForeLeft(i) & 7;
			if (ti > 1)
			{
				char buf[128];
				sprintf(buf, "CK_WallDebug: West wall other than 1:%d", i);
				Quit(buf);
			}
			else if (ti == 1)
			{
				CK_OverlayForegroundTile(i, ti + 0x7A);
			}
		}
	}
	// Hack to work around F10-Y not updating the screen immediately.
	RF_Reposition(rf_scrollXUnit, rf_scrollYUnit);
}

bool CK_DebugKeys()
{
	// Border colour
	if (IN_GetKeyState(IN_SC_B) && game_in_progress)
	{
		char str[4];
		uint16_t w, h;
		// VW_SyncPages();
		US_CenterWindow(0x18, 3);
		US_SetPrintY(US_GetPrintY() + 6);
		VH_MeasurePropString(" Border color (0-15):", &w, &h, US_GetPrintFont());
		uint16_t saveX = US_GetPrintX() + w;
		uint16_t saveY = US_GetPrintY();
		US_Print(" Border color (0-15):");
		VL_Present(); // VW_UpdateScreen();

		if (US_LineInput(saveX, saveY, str, NULL, true, 2, 0))
		{
			int colour;

			// Convert string into number
			sscanf(str, "%d", &colour);

			if (colour >= 0 && colour <= 15)
			{
				VL_ColorBorder(colour);
			}
		}
		return true;
	}
	if (IN_GetKeyState(IN_SC_C) && game_in_progress)
	{
		CK_CountActiveObjects();
		return true;
	}
	// TODO: Demo Recording
	if (IN_GetKeyState(IN_SC_D) && game_in_progress)
	{
		if (IN_DemoGetMode() == IN_Demo_Off)
		{
			CK_BeginDemoRecord();
		}
		else if (IN_DemoGetMode() == IN_Demo_Record)
		{
			CK_EndDemoRecord();
			ck_gameState.levelState = LS_LevelComplete;
		}
		return true;
	}

	// End Level
	if (IN_GetKeyState(IN_SC_E) && game_in_progress)
	{
		// TODO: Handle ted level differently?
		if (us_tedLevel)
			//run_ted();
			Quit(NULL);
		ck_gameState.levelState = 2;
		// Nope, no return of "true"
	}

	// God Mode
	if (IN_GetKeyState(IN_SC_G) && game_in_progress)
	{
		// VW_SyncPages();
		US_CenterWindow(12, 2);

		if (ck_godMode)
			US_PrintCentered(CK_VAR_GetStr("ck_str_godModeOff"));
		else
			US_PrintCentered(CK_VAR_GetStr("ck_str_godModeOn"));

		VL_Present();
		IN_WaitButton();
		ck_godMode = !ck_godMode;
		return true;
	}

	// Free Item Cheat
	if (IN_GetKeyState(IN_SC_I))
	{
		// VW_SyncPages();
		US_CenterWindow(12, 3);
		US_PrintCentered(CK_VAR_GetStr("ck_str_freeItems"));

		for (int i = 0; i < 4; i++)
			ck_gameState.keyGems[i]++;

		ck_gameState.numShots = 99;
		if (ck_currentEpisode->ep == EP_CK4)
			ck_gameState.ep.ck4.wetsuit = 1;
		else if (ck_currentEpisode->ep == EP_CK5)
			ck_gameState.ep.ck5.securityCard = 1;
		else if (ck_currentEpisode->ep == EP_CK6)
			ck_gameState.ep.ck6.sandwich = ck_gameState.ep.ck6.rope = ck_gameState.ep.ck6.passcard = 1;

		VL_Present();
		IN_WaitButton();
		CK_IncreaseScore(3000);
		return true;
	}

	if (IN_GetKeyState(IN_SC_J))
	{
		ck_gameState.jumpCheat = !ck_gameState.jumpCheat;
		//TODO: Something here?
		US_CenterWindow(18, 3);

		if (ck_gameState.jumpCheat)
			US_PrintCentered(CK_VAR_GetStr("ck_str_jumpCheatOn"));
		else
			US_PrintCentered(CK_VAR_GetStr("ck_str_jumpCheatOff"));

		VL_Present();
		IN_WaitButton();
		return true;
	}

	if (IN_GetKeyState(IN_SC_M))
	{
		CK_DebugMemory();
		return true;
	}

	if (IN_GetKeyState(IN_SC_N))
	{
		US_CenterWindow(18, 3);
		if (ck_keenObj->clipped)
		{
			US_PrintCentered(CK_VAR_GetStr("ck_str_noClippingOn"));
			ck_keenObj->clipped = CLIP_not;
		}
		else
		{
			US_PrintCentered(CK_VAR_GetStr("ck_str_noClippingOff"));
			ck_keenObj->clipped = CLIP_normal;
		}
		VL_Present();
		IN_WaitButton();
		return true;
	}

	// Pause

	// Slow Motion
	if (IN_GetKeyState(IN_SC_S) && game_in_progress)
	{
		ck_slowMotionEnabled = !ck_slowMotionEnabled;
		US_CenterWindow(18, 3);

		if (ck_slowMotionEnabled)
			US_PrintCentered(CK_VAR_GetStr("ck_str_slowMotionOn"));
		else
			US_PrintCentered(CK_VAR_GetStr("ck_str_slowMotionOff"));
		VL_Present();
		IN_WaitButton();
		return true;
	}

	// Sprite Test
	if (IN_GetKeyState(IN_SC_T))
	{
		CK_SpriteTest();
		return true;
	}

	// Extra Vibbles

	// Level Warp
	if (IN_GetKeyState(IN_SC_W) && game_in_progress)
	{
		char str[4];
		const char *msg = "  Warp to which level(1-18):";
		uint16_t h, w, saveX, saveY; // omnispeak hacks

		// VW_SyncPages();
		US_CenterWindow(26, 3);
		US_SetPrintY(US_GetPrintY() + 6);
		VH_MeasurePropString(msg, &w, &h, US_GetPrintFont());
		saveX = US_GetPrintX() + w;
		saveY = US_GetPrintY();
		US_Print(msg);
		VL_Present(); // VW_UpdateScreen();

		if (US_LineInput(saveX, saveY, str, NULL, true, 2, 0))
		{
			int level;

			// Convert string into number
			sscanf(str, "%d", &level);

			if (level >= 1 && level <= 18)
			{
				ck_gameState.currentLevel = level;
				ck_gameState.levelState = 4;
			}
		}
		return true;
	}

	if (IN_GetKeyState(IN_SC_Y))
	{
		CK_WallDebug();
		return true;
	}

	if (IN_GetKeyState(IN_SC_Z))
	{
		ck_gameState.numLives = 0;
		CK_KillKeen();
		// Not sure why this'd be 'false', but that's what
		// the disassembly says.
		return false;
	}

	return false;
}

// Check non-game keys

void CK_CheckKeys()
{
	// TODO: Also check for a gamepad button when relevant

	if (vl_screenFaded)
	{
		return;
	}

	// Drop down status
#ifdef EXTRA_KEYBOARD_OPTIONS
	if (IN_GetKeyState(in_kbdControls.status))
#else
	if (IN_GetKeyState(IN_SC_Enter))
#endif
	{
		CK_ShowStatusWindow();
		RF_ForceRefresh(); // Reanimate the tiles
		SD_SetLastTimeCount(SD_GetTimeCount());
	}

	// TODO: If Paused
	if (in_Paused)
	{
		SD_MusicOff();
		// VW_SyncPages();
		US_CenterWindow(strlen(in_PausedMessage) + 2, 3);
		US_PrintCentered(in_PausedMessage);
		VL_Present(); // VW_UpdateScreen();
		IN_WaitButton();
		RF_ForceRefresh();
		in_Paused = false;
		in_PausedMessage = CK_VAR_GetStr("ck_str_paused");
		SD_MusicOn();
	}

	// HELP
	if ((ck_currentEpisode->ep != EP_CK6) && (IN_GetLastScan() == IN_SC_F1))
	{
		StopMusic();
		HelpScreens();
		StartMusic(ck_gameState.currentLevel);

		// Force scorebox redraw if it's enabled
		if (ck_scoreBoxEnabled)
		{
			ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;
		}
	}

	if (!ck_demoParm)
	{

#ifdef QUICKSAVE_ENABLED
		// Quicksave
		if (IN_GetLastScan() == in_kbdControls.quickSave)
		{
			if (US_QuickSave())
			{
				in_Paused = true;
				in_PausedMessage = CK_VAR_GetStr("ck_str_gameQuickSaved");
			}
		}

		// Quickload
		else if (IN_GetLastScan() == in_kbdControls.quickLoad)
		{
			if (US_QuickLoad())
			{
				StopMusic();
				// Force scorebox redraw if it's enabled
				if (ck_scoreBoxEnabled)
				{
					ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;
				}
				ck_gameState.levelState = 6;
				StartMusic(ck_gameState.currentLevel);
			}
			if (load_game_error)
			{
				load_game_error = 0;
				ck_gameState.levelState = 8;
			}
		}
		// clang-format off
		else
#endif

		// Go back to wristwatch
		if ((IN_GetLastScan() >= IN_SC_F2 && IN_GetLastScan() <= IN_SC_F7)
		    || IN_GetLastScan() == IN_SC_Escape
#ifndef CK_VANILLA
		    || ck_inputFrame.button3
#endif
		)
		{
			// clang-format on

			// don't re-enter menu immediately if we came here
			// via joystick button
			ck_inputFrame.button3 = false;

			// VW_SyncPages();
			StopMusic();
			US_RunCards();

			// RF_Reset();
			StartMusic(ck_gameState.currentLevel);

			// Wipe the scorebox if it got disabled
			if (!ck_scoreBoxEnabled && ck_scoreBoxObj->sde)
				RF_RemoveSpriteDraw(&ck_scoreBoxObj->sde);

			// Force scorebox redraw if it's enabled
			if (ck_scoreBoxEnabled)
			{
				ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;
			}

			IN_ClearKeysDown();

			if (ck_startingDifficulty)
				ck_gameState.levelState = 5;
			else if (!ck_startingSavedGame)
				; // RF_ResetScreen();

			if (load_game_error)
			{
				load_game_error = 0;
				ck_gameState.levelState = 8;
			}

			if (ck_startingSavedGame)
				ck_gameState.levelState = 6;

			// gameticks_2 = TimeCount
			// FIXME: Is this the right way to handle this?
			SD_SetLastTimeCount(SD_GetTimeCount());
		}
	}

	// BAT ITEM CHEAT
	if (IN_GetKeyState(IN_SC_B) && IN_GetKeyState(IN_SC_A) && IN_GetKeyState(IN_SC_T))
	{
		CK_ItemCheat();
	}

	// Debug Keys
	if (IN_GetKeyState(IN_SC_F10))
	{
		if (ck_debugActive && CK_DebugKeys())
		{
			//RF_ResetScreen();
			SD_SetLastTimeCount(SD_GetTimeCount());
		}
	}

	// TODO: CTRL+S sound

	// CTRL + Q
	if (IN_GetKeyState(IN_SC_Control) && IN_GetLastScan() == IN_SC_Q)
	{

		IN_ClearKeysDown();
		Quit(NULL);
	}
}

void CK_HandleInput()
{
	IN_ReadControls(0, &ck_inputFrame);

	if (ck_inputFrame.yDirection != -1)
		ck_keenState.keenSliding = false;

	if (!ck_gamePadEnabled || (IN_DemoGetMode() != IN_Demo_Off))
	{
		// Two button firing mode is used for demo playback
		if (!ck_twoButtonFiring && (IN_DemoGetMode() == IN_Demo_Off))
		{
			ck_keenState.jumpIsPressed = ck_inputFrame.jump;
			ck_keenState.pogoIsPressed = ck_inputFrame.pogo;
			ck_keenState.shootIsPressed = ck_inputFrame.button2;
			if (!ck_keenState.jumpIsPressed)
				ck_keenState.jumpWasPressed = false;
			if (!ck_keenState.pogoIsPressed)
				ck_keenState.pogoWasPressed = false;

			if (!ck_keenState.shootIsPressed)
				ck_keenState.shootWasPressed = false;
		}
		else
		{

			// Check for two-button firing
			if (ck_inputFrame.jump && ck_inputFrame.pogo)
			{
				ck_keenState.shootIsPressed = true;
				ck_keenState.jumpIsPressed = false;
				ck_keenState.pogoIsPressed = false;
				ck_keenState.jumpWasPressed = false;
				ck_keenState.pogoWasPressed = false;
				return;
			}

			ck_keenState.shootWasPressed = false;
			ck_keenState.shootIsPressed = false;

			if (ck_inputFrame.jump)
			{
				ck_keenState.jumpIsPressed = true;
			}
			else
			{
				ck_keenState.jumpWasPressed = false;
				ck_keenState.jumpIsPressed = false;
			}

			if (ck_inputFrame.pogo)
			{
				// Here be dragons!
				// In order to better emulate the original trilogy's controls, a delay
				// is introduced when pogoing in two-button firing.
				if (ck_pogoTimer <= 8)
				{
					ck_pogoTimer += SD_GetSpriteSync();
				}
				else
				{
					ck_keenState.pogoIsPressed = true;
				}
			}
			else
			{
				// If the player lets go of pogo, pogo immediately.
				if (ck_pogoTimer)
				{
					ck_keenState.pogoIsPressed = true;
				}
				else
				{
					ck_keenState.pogoWasPressed = false;
					ck_keenState.pogoIsPressed = false;
				}
				ck_pogoTimer = 0;
			}
		}
	}
	else
	{
		// TODO: Gravis Gamepad input handling
	}
}

void StopMusic(void)
{
	int16_t i;
	SD_MusicOff();
	for (i = 0; i < LASTMUSTRACK; i++)

		if (CA_audio[ca_audInfoE.startMusic + i])
			MM_SetPurge((void **)&CA_audio[ca_audInfoE.startMusic + i], 3);
}

int16_t *ck_levelMusic;

void StartMusic(int16_t level)
{
	int16_t song;

	if ((level >= 20) && (level != -1))
	{
		Quit("StartMusic() - bad level number");
	}
	SD_MusicOff();

	// NOTE: For buffer overflow emulation in Keen 5,
	// consider assigning in_kbdControls.fire as the song number
	if ((level == -1) && (ck_currentEpisode->ep == EP_CK4))
		song = 5;
	else
		song = ck_levelMusic[level];

	if ((song == -1) || (SD_GetMusicMode() != smm_AdLib))
		return;
	MM_BombOnError(false);
	CA_CacheAudioChunk(ca_audInfoE.startMusic + song);
	MM_BombOnError(true);
	// TODO: FINISH THIS!
#if 0
	if (mmerror)
	{
		int16_t faded;
		mmerror = false;
		if (DemoMode)
			return;
		US_CenterWindow(20, 8);
		window_print_y += 20;
		window_print("Insufficient memory\nfor background music!");
		VM_UpdateScreen();
		faded = screen_faded;
		if (faded)
			VM_SetDefaultPalette();
		IN_ClearKeysDown();
		IN_UserInput(210, 0, 0);

		if (faded)
			VW_FadeToBlack();
	}
#endif
	SD_StartMusic((SD_MusicTrack *)CA_audio[ca_audInfoE.startMusic + song]);
}

//===========================================================================
//
// DROPDOWN STATUS WINDOW
//
//===========================================================================

#define SOUND_STATUSDOWN 34
#define SOUND_STATUSUP 35

void *ck_statusSurface;
void *ck_backupSurface;

// The status window is drawn to scratch area in the video buffer
// Then it is copied from the scratch to the screen using the RF_ Hook

void CK_DrawLongRight(int x, int y, int digits, int zerotile, int value);
void CK_DrawStatusWindow(void);
void CK_ScrollStatusWindow(void);
void CK_ShowStatusWindow(void);

int ck_statusWindowYPx;
bool ck_statusDown;
unsigned statusWindowOfs; // screen buffer

void CK_DrawLongRight(int x, int y, int digits, int zerotile, int value)
{
	char s[20];
	int len, i;

	sprintf(s, "%d", value);
	// itoa(value, s, 10);
	len = strlen(s);

	for (i = digits; i > len; i--)
	{
		VHB_DrawTile8(x, y, zerotile);
		x += 8;
	}

	while (i > 0)
	{
		VHB_DrawTile8(x, y, zerotile + s[len - i] - 47);
		i--;
		x += 8;
	}
}

void CK_DrawStatusWindow(void)
{
	int si, i, oldcolor;

	// Draw the backdrop
	int var2 = 64;
	int di = 16;
	int statusWidth = STATUS_W - 8;
	int statusHeight = STATUS_H - 8;

	VHB_DrawTile8(var2, di, 54);
	VHB_DrawTile8(var2, di + statusHeight, 60);
	si = var2 + 8;

	while (var2 + statusWidth - 8 >= si)
	{

		VHB_DrawTile8(si, di, 55);
		VHB_DrawTile8(si, di + statusHeight, 61);
		si += 8;
	}

	VHB_DrawTile8(si, di, 56);
	VHB_DrawTile8(si, di + statusHeight, 62);

	si = di + 8;

	while (di + statusHeight - 8 >= si)
	{

		VHB_DrawTile8(var2, si, 57);
		VHB_DrawTile8(var2 + statusWidth, si, 59);
		si += 8;
	}

	VHB_Bar(72, 24, 176, 136, 7);

	// Print the stuff
	oldcolor = US_GetPrintColour();

	// Level
	uint16_t strW, strH;
	char str[256];
	US_SetPrintY(28);
	US_SetWindowX(80);
	US_SetWindowW(160);
	US_SetPrintColour(15);
	US_CPrint(CK_VAR_GetStr("ck_str_statusLocation"));
	VHB_Bar(79, 38, 162, 20, 15);
	strcpy(str, CK_VAR_GetStringByNameAndIndex("ck_str_levelName", ca_mapOn));
	CK_MeasureMultiline(str, &strW, &strH);
	US_SetPrintY((20 - strH) / 2 + 40 - 2);
	US_CPrint(str);

	// Score
	US_SetPrintY(61);
	US_SetWindowX(80);
	US_SetWindowW(64);
	US_SetPrintColour(15);
	US_CPrint(CK_VAR_GetStr("ck_str_statusScore"));

	VHB_Bar(79, 71, 66, 10, 0);
	CK_DrawLongRight(80, 72, 8, 41, ck_gameState.keenScore);

	// Extra man
	US_SetPrintY(61);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_CPrint(CK_VAR_GetStr("ck_str_statusExtra"));
	VHB_Bar(175, 71, 66, 10, 0);
	CK_DrawLongRight(176, 72, 8, 41, ck_gameState.nextKeenAt);

	// Episode-dependent field
	switch (ck_currentEpisode->ep)
	{
	case EP_CK4:
		US_SetPrintY(85);
		US_SetWindowX(80);
		US_SetWindowW(64);
		US_CPrint(CK_VAR_GetStr("ck4_str_statusRescued"));

		VHB_Bar(79, 95, 66, 10, 0);
		for (int i = 0; i < ck_gameState.ep.ck4.membersRescued; i++)
		{
			VHB_DrawTile8(80 + 8 * i, 96, 40);
		}
		break;

	case EP_CK5:
		US_SetPrintY(91);
		US_SetPrintX(80);
		US_Print(CK_VAR_GetStr("ck5_str_statusKeycard"));

		VHB_Bar(135, 90, 10, 10, 0);
		if (ck_gameState.ep.ck5.securityCard)
			VHB_DrawTile8(136, 91, 40);
		break;

	case EP_CK6:
		US_SetPrintX(80);
		US_SetPrintY(96);
		US_Print(CK_VAR_GetStr("ck6_str_statusItems"));
		VHB_Bar(127, 95, 26, 10, 0);

		if (ck_gameState.ep.ck6.sandwich == 1)
			VHB_DrawTile8(128, 96, 2);
		else
			VHB_DrawTile8(128, 96, 1);

		if (ck_gameState.ep.ck6.rope == 1)
			VHB_DrawTile8(136, 96, 4);
		else
			VHB_DrawTile8(136, 96, 3);

		if (ck_gameState.ep.ck6.passcard == 1)
			VHB_DrawTile8(144, 96, 6);
		else
			VHB_DrawTile8(144, 96, 5);
		break;
	
	default:
		Quit("No episode set!");
	}

	// Difficulty
	US_SetPrintY(85);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_SetPrintColour(15);
	US_CPrint(CK_VAR_GetStr("ck_str_statusLevel"));
	VHB_Bar(175, 95, 66, 10, 15);

	US_SetPrintY(96);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_SetPrintColour(15);

	switch (ck_gameState.difficulty)
	{
	case D_Easy:
		US_CPrint(CK_VAR_GetStr("ck_str_statusEasy"));
		break;
	case D_Normal:
		US_CPrint(CK_VAR_GetStr("ck_str_statusNormal"));
		break;
	case D_Hard:
		US_CPrint(CK_VAR_GetStr("ck_str_statusHard"));
		break;
	default:
		Quit("No difficulty set!");
	}

	// Key gems
	US_SetPrintX(80);
	US_SetPrintY(112);
	US_SetPrintColour(15);
	US_Print(CK_VAR_GetStr("ck_str_statusKeys"));

	VHB_Bar(119, 111, 34, 10, 0);

	for (i = 0; i < 4; i++)
	{
		if (ck_gameState.keyGems[i])
			VHB_DrawTile8(120 + i * 8, 112, 36 + i);
	}

	// AMMO
	US_SetPrintX(176);
	US_SetPrintY(112);
	US_Print(CK_VAR_GetStr("ck_str_statusAmmo"));
	VHB_Bar(215, 111, 26, 10, 0);
	CK_DrawLongRight(216, 112, 3, 41, ck_gameState.numShots);

	// Lives
	US_SetPrintX(80);
	US_SetPrintY(128);
	US_Print(CK_VAR_GetStr("ck_str_statusLives"));
	VHB_Bar(127, 127, 18, 10, 0);
	CK_DrawLongRight(128, 128, 2, 41, ck_gameState.numLives);

	// Lifeups
	US_SetPrintX(176);
	US_SetPrintY(128);
	US_Print(CK_VAR_GetStr("ck_str_statusCentilives"));
	VHB_Bar(224, 127, 16, 10, 0);
	CK_DrawLongRight(224, 128, 2, 41, ck_gameState.numCentilife);

	// Episode-dependent field
	int addX = 0;
	switch (ck_currentEpisode->ep)
	{

	case EP_CK4:

		// Wetsuit
		VHB_Bar(79, 143, 66, 10, 15);

		US_SetPrintY(144);
		US_SetWindowX(80);
		US_SetWindowW(64);
		US_SetPrintColour(15);
		US_CPrint(ck_gameState.ep.ck4.wetsuit ? CK_VAR_GetStr("ck4_str_statusWetsuit") : CK_VAR_GetStr("ck4_str_statusNoWetsuit"));

		addX = 5;

	case EP_CK5:
	case EP_CK6:

		for (int y = 0; y < 2; y++)
			for (int x = 0; x < 10; x++)
				VHB_DrawTile8(120 + 8 * (x + addX), 140 + 8 * y, 72 + y * 10 + x);

		break;
	default:
		Quit("No episode set!");
	}

	US_SetPrintColour(oldcolor);
}

void CK_ScrollStatusWindow(void)
{
	int dx, dy, sx, sy;
	int height;

	int scrX = VL_GetScrollX() & 8;
	int scrY = VL_GetScrollY() % 16;
	int statusWindowXPx = STATUS_X + scrX;

	// Check if there's some game visible behind the top of the window.
	if (ck_statusWindowYPx > STATUS_H)
	{
		// The bit of tilemap behind the status window would need to be
		// redrawn after the top border of the status window scrolled on to the screen
		VL_SurfaceToScreen(ck_statusSurface,
					statusWindowXPx, 0,
					statusWindowXPx, 0,
					STATUS_W, ck_statusWindowYPx);

		// MPic atop the statusbox
		VH_DrawMaskedBitmap(136 + scrX, (ck_statusWindowYPx - STATUS_BOTTOM) + scrY,
				MPIC_STATUSRIGHT);
		
		// Set up the position for the status box.
		height = STATUS_H;
		sx = statusWindowXPx;
		sy = STATUS_Y + scrY;
		dx = statusWindowXPx;
		dy = ck_statusWindowYPx - STATUS_H + scrY;
	}
	else
	{
		// If the status window is not unfurled, its height is equal to
		// the y-position of its bottom.
		height = ck_statusWindowYPx;

		dx = statusWindowXPx;
		dy = scrY;
		sx = statusWindowXPx;
		sy = STATUS_BOTTOM - height + scrY;
	}

	if (height > 0)
	{
		VL_SurfaceToScreen(ck_statusSurface,
				dx, dy,
				sx, sy,
				STATUS_W, height);
	}

	// Draw the tile map underneath the scrolling status box
	if (ck_statusDown)
	{
		// Coming back up, need to redraw the map back underneath
		height = STATUS_BOTTOM - ck_statusWindowYPx;
		
		VL_SurfaceToScreen(ck_backupSurface,
				statusWindowXPx, ck_statusWindowYPx + scrY,
				statusWindowXPx, ck_statusWindowYPx + scrY,
				STATUS_W, height);

		// Draw underneath the left masked pic
		height = ck_statusWindowYPx;

		if (height > 0)
		{
			VL_SurfaceToScreen(ck_statusSurface,
					40 + scrX, 0,
					40 + scrX, 0,
					24, height);
		}

	}
	else
	{
		// Going down
		height = ck_statusWindowYPx - 72;

		// Reset the area under the masked pic to the left
		if (height > 0)
		{
			VL_SurfaceToScreen(ck_statusSurface, 40 + scrX % 16, 0, 40 + scrX % 16, 0, 24, height);
		}
	}

	if (ck_statusWindowYPx >= 72)
	{
		VH_DrawMaskedBitmap(40 + scrX, ck_statusWindowYPx - STATUS_BOTTOM + scrY,
				MPIC_STATUSLEFT);
	}
}

extern void RFL_SetupOnscreenAnimList();
#include "id_vl_private.h"
void CK_ShowStatusWindow(void)
{

	// int oldBufferofs;

	US_SetWindowX(0);
	US_SetWindowW(320);
	US_SetWindowY(0);
	US_SetWindowH(200);

	// This function is called when enter pressed; check for A+2 to enable debug mode
	if (IN_GetKeyState(IN_SC_A) && IN_GetKeyState(IN_SC_Two))
	{
		US_CenterWindow(20, 2);
		US_SetPrintY(US_GetPrintY() + 2);
		US_Print(CK_VAR_GetStr("ck_str_debugKeysActive"));
		VL_Present(); //VW_UpdateScreen();
		IN_WaitButton();
		ck_debugActive = true;
	}

	RF_Refresh();

	// Clear out all animating tiles
	RFL_SetupOnscreenAnimList();

	// Draw the status window to scratch area in the buffer
#if 0
	oldBufferofs = bufferofs;
	bufferofs = statusWindowOfs = RF_FindFreeBuffer();
	VW_ScreenToScreen(displayofs, displayofs, 44, 224);
	VW_ScreenToScreen(displayofs, masterofs, 44, 224);
	VW_ScreenToScreen(displayofs, bufferofs, 44, 168);
#endif

	// Omnispeak: make a surface and set it as the screen, temporarily,
	// in order to use the VH_ functions for drawing
	void *screen = vl_emuegavgaadapter.screen;
	VL_SurfaceToSurface(screen, ck_statusSurface, 0, 0, 0, 0, RF_BUFFER_WIDTH_PIXELS, STATUS_H + 16);
	// And save a copy of the screen as-is to a backup surface as well.
	VL_SurfaceToSurface(screen, ck_backupSurface, 0, 0, 0, 0, RF_BUFFER_WIDTH_PIXELS, RF_BUFFER_HEIGHT_PIXELS);
	VL_SetScreen(ck_statusSurface);
	//int oldScrollX = VL_GetScrollX();
	//int oldScrollY = VL_GetScrollY();
	CK_DrawStatusWindow();
	//VL_SetScrollCoords(oldScrollX, oldScrollY);
	VL_SetScreen(screen);
	// bufferofs = oldBufferofs;
	RF_Refresh();

	// Scroll the window down
	SD_PlaySound(SOUND_STATUSDOWN);
	ck_statusWindowYPx = 16;
	ck_statusDown = false;
	RF_SetDrawFunc(CK_ScrollStatusWindow);
	do
	{
		RF_Refresh();
		if (ck_statusWindowYPx == 168)
			break;

		if ((ck_statusWindowYPx += SD_GetSpriteSync() * 8) <= 168)
			continue;

		ck_statusWindowYPx = 168;

	} while (1);

	RF_Refresh();
	RF_SetDrawFunc(NULL);
	IN_ClearKeysDown();
	IN_WaitButton(); //IN_Ack();

	// Scroll the window up
	SD_PlaySound(SOUND_STATUSUP);
	ck_statusWindowYPx -= 16;
	ck_statusDown = true;
	RF_SetDrawFunc(CK_ScrollStatusWindow);

	do
	{
		RF_Refresh();
		if (ck_statusWindowYPx == 0)
			break;

		if ((ck_statusWindowYPx -= SD_GetSpriteSync() * 8) >= 0)
			continue;

		ck_statusWindowYPx = 0;

	} while (1);

	RF_SetDrawFunc(NULL);

	ck_scoreBoxObj->posX = 0;
}

// ===========================================================================

extern int rf_scrollXUnit;
extern int rf_scrollYUnit;

// The intended y-coordinate of the bottom of the keen sprite
// in pixels from the top of the screen.
static uint16_t screenYpx;

// Centre the camera on the given object.

void CK_CentreCamera(CK_object *obj)
{
	uint16_t screenX, screenY;

	screenYpx = 140;

	if (obj->posX < RF_PixelToUnit(152))
		screenX = 0;
	else
		screenX = obj->posX - RF_PixelToUnit(152);

	if (ca_mapOn == 0)
	{
		// World Map
		if (obj->posY < RF_PixelToUnit(80))
			screenY = 0;
		else
			screenY = obj->posY - RF_PixelToUnit(80);
	}
	else
	{
		// In Level
		if (obj->clipRects.unitY2 < RF_PixelToUnit(140))
			screenY = 0;

		else
			screenY = obj->clipRects.unitY2 - RF_PixelToUnit(140);
	}

	// TODO: Find out why this is locking the game up
	if (!ck_inHighScores)
		RF_Reposition(screenX, screenY);

	//TODO: This is 4 in Andy's disasm.
	ck_activeX0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) - ck_currentEpisode->activeLimit, 0);
	ck_activeX1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320) + ck_currentEpisode->activeLimit, 0);
	ck_activeY0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) - ck_currentEpisode->activeLimit, 0);
	ck_activeY1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208) + ck_currentEpisode->activeLimit, 0);
}

/*
 * Move the camera that follows keen on the world map
 */

void CK_MapCamera(CK_object *keen)
{
	int16_t scr_y, scr_x;

	if (ck_scrollDisabled)
		return;

	// Scroll Left, Right, or nowhere
	if (keen->clipRects.unitX1 < rf_scrollXUnit + RF_PixelToUnit(144))
		scr_x = keen->clipRects.unitX1 - (rf_scrollXUnit + RF_PixelToUnit(144));
	else if (keen->clipRects.unitX2 > rf_scrollXUnit + RF_PixelToUnit(192))
		scr_x = keen->clipRects.unitX2 + 16 - (rf_scrollXUnit + RF_PixelToUnit(192));
	else
		scr_x = 0;

	// Scroll Up, Down, or nowhere
	if (keen->clipRects.unitY1 < rf_scrollYUnit + RF_PixelToUnit(80))
		scr_y = keen->clipRects.unitY1 - (rf_scrollYUnit + RF_PixelToUnit(80));
	else if (keen->clipRects.unitY2 > rf_scrollYUnit + RF_PixelToUnit(112))
		scr_y = keen->clipRects.unitY2 - (rf_scrollYUnit + RF_PixelToUnit(112));
	else
		scr_y = 0;

	// Limit scrolling to 256 map units (1 tile)
	// And update the active boundaries of the map
	if (scr_x != 0 || scr_y != 0)
	{
		if (scr_x >= 256)
			scr_x = 255;
		else if (scr_x <= -256)
			scr_x = -255;

		if (scr_y >= 256)
			scr_y = 255;
		else if (scr_y <= -256)
			scr_y = -255;

		RF_SmoothScroll(scr_x, scr_y);

		/*
		 * No ScrollX1_T in omnispeak; it's computed whenever it's needed
		 * ScrollX1_T = ScrollX0_T + VIRTUAL_SCREEN_W_T;
		 * ScrollY1_T = ScrollY0_T + VIRTUAL_SCREEN_H_T;
		 */

		//TODO: This is 4 in Andy's disasm.
		ck_activeX0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) - ck_currentEpisode->activeLimit, 0);
		ck_activeX1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320) + ck_currentEpisode->activeLimit, 0);
		ck_activeY0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) - ck_currentEpisode->activeLimit, 0);
		ck_activeY1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208) + ck_currentEpisode->activeLimit, 0);
	}
}

// Run the normal camera which follows keen

void CK_NormalCamera(CK_object *obj)
{

	int16_t deltaX = 0, deltaY = 0; // in Units

	//TODO: some unknown var must be 0
	//This var is a "ScrollDisabled flag." If keen dies, it's set so he
	// can fall out the bottom
	if (ck_scrollDisabled)
		return;

	// End level if keen makes it out either side
	if (obj->clipRects.unitX1 < rf_scrollXMinUnit || obj->clipRects.unitX2 > rf_scrollXMaxUnit + RF_PixelToUnit(320))
	{
		ck_gameState.levelState = 2;
		return;
	}

	// Kill keen if he falls out the bottom
	if (obj->clipRects.unitY2 > (rf_scrollYMaxUnit + RF_PixelToUnit(208)))
	{
		obj->posY -= obj->clipRects.unitY2 - (rf_scrollYMaxUnit + RF_PixelToUnit(208));
		SD_PlaySound(SOUND_KEENFALL);
		ck_godMode = false;
		CK_KillKeen();
		return;
	}

	// Keep keen's x-coord between 144-192 pixels
	if (obj->posX < (rf_scrollXUnit + RF_PixelToUnit(144)))
		deltaX = obj->posX - (rf_scrollXUnit + RF_PixelToUnit(144));

	if (obj->posX > (rf_scrollXUnit + RF_PixelToUnit(192)))
		deltaX = obj->posX - (rf_scrollXUnit + RF_PixelToUnit(192));

	// Keen should be able to look up and down.
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenLookUp2"))
	{
		int16_t pxToMove;
		if (screenYpx + SD_GetSpriteSync() > 167)
		{
			// Keen should never be so low on the screen that his
			// feet are more than 167 px from the top.
			pxToMove = 167 - screenYpx;
		}
		else
		{
			// Move 1px per tick.
			pxToMove = SD_GetSpriteSync();
		}
		screenYpx += pxToMove;
		deltaY = RF_PixelToUnit(-pxToMove);
	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenLookDown3"))
	{
		int16_t pxToMove;
		if (screenYpx - SD_GetSpriteSync() < 33)
		{
			// Keen should never be so high on the screen that his
			// feet are fewer than 33 px from the top.
			pxToMove = screenYpx - 33;
		}
		else
		{
			// Move 1px/tick.
			pxToMove = SD_GetSpriteSync();
		}
		screenYpx -= pxToMove;
		deltaY = RF_PixelToUnit(pxToMove);
	}

	// If we're attached to the ground, or otherwise awesome
	// do somethink inscrutible.
	if (ck_currentEpisode->ep == EP_CK6 && ck6_smashScreenDistance)
	{
		int16_t dx, ax;

		ax = ck6_smashScreenOfs[ck6_smashScreenDistance] + obj->clipRects.unitY2;
		deltaY += (dx - ax); // Undefined behaviour here
	}
	else if (obj->topTI || !obj->clipped || obj->currentAction == CK_GetActionByName("CK_ACT_keenHang1"))
	{
		if (obj->currentAction != CK_GetActionByName("CK_ACT_keenPull1") &&
			obj->currentAction != CK_GetActionByName("CK_ACT_keenPull2") &&
			obj->currentAction != CK_GetActionByName("CK_ACT_keenPull3") &&
			obj->currentAction != CK_GetActionByName("CK_ACT_keenPull4"))
		{
			deltaY += obj->deltaPosY;

			//TODO: Something hideous
			// TODO: Convert to 16-bit once the rest is converted
			// (due to unsigned vs signed mess)
			int cmpAmt = RF_PixelToUnit(screenYpx) + rf_scrollYUnit + deltaY;
			if (cmpAmt != obj->clipRects.unitY2)
			{
				int oneDiff, otherDiff;
				if (obj->clipRects.unitY2 < cmpAmt)
					oneDiff = cmpAmt - obj->clipRects.unitY2;
				else
					oneDiff = obj->clipRects.unitY2 - cmpAmt;

				// TODO: Unsigned shift left,
				// followed by a signed shift right...
				otherDiff = (signed)((unsigned)oneDiff << 4) >> 7;
				if (otherDiff > 48)
					otherDiff = 48;
				otherDiff *= SD_GetSpriteSync();
				if (otherDiff < 16)
				{
					if (oneDiff < 16)
						otherDiff = oneDiff;
					else
						otherDiff = 16;
				}
				if (obj->clipRects.unitY2 < cmpAmt)
					deltaY -= otherDiff;
				else
					deltaY += otherDiff;
#if 0
				int16_t adjAmt = (((screenYpx << 4) + rf_scrollYUnit + deltaY - obj->clipRects.unitY2));
				int16_t adjAmt2 = abs(adjAmt / 8);

				adjAmt2 = (adjAmt2 <= 48) ? adjAmt2 : 48;

				if (adjAmt > 0)
					deltaY -= adjAmt2;
				else
					deltaY += adjAmt2;
#endif
			}
		}
	}
	else
	{
		// Reset to 140px.
		screenYpx = 140;
	}

	// Scroll the screen to keep keen between 33 and 167 px.
	if (obj->clipRects.unitY2 < (rf_scrollYUnit + deltaY + RF_PixelToUnit(32)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + RF_PixelToUnit(32));

	if (obj->clipRects.unitY2 > (rf_scrollYUnit + deltaY + RF_PixelToUnit(168)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + RF_PixelToUnit(168));

	//Don't scroll more than one tile's worth per frame.
	if (deltaX || deltaY)
	{
		if (deltaX > 255)
			deltaX = 255;
		else if (deltaX < -255)
			deltaX = -255;

		if (deltaY > 255)
			deltaY = 255;
		else

			if (deltaY < -255)
			deltaY = -255;

		// Do the scroll!
		RF_SmoothScroll(deltaX, deltaY);

		// Update the rectangle of active objects
		ck_activeX0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) - ck_currentEpisode->activeLimit, 0);
		ck_activeX1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320) + ck_currentEpisode->activeLimit, 0);
		ck_activeY0Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) - ck_currentEpisode->activeLimit, 0);
		ck_activeY1Tile = CK_Cross_max(RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208) + ck_currentEpisode->activeLimit, 0);
	}
}

// Play a level.

void CK_PlayLoop()
{
	StartMusic(ck_gameState.currentLevel);
	ck_pogoTimer = 0;
	memset(&ck_keenState, 0, sizeof(ck_keenState));
	ck_invincibilityTimer = 0;
	ck_scrollDisabled = false;
	ck_keenState.jumpWasPressed = ck_keenState.pogoWasPressed = ck_keenState.shootWasPressed = false;
	game_in_progress = 1;
	// If this is nonzero, the level will quit immediately.
	ck_gameState.levelState = 0;

	//ck_keenState.pogoTimer = ck_scrollDisabled = ck_keenState.invincibilityTimer = 0;

	CK_CentreCamera(ck_keenObj);

	// Note that this appears in CK_LoadLevel as well
	if (IN_DemoGetMode() != IN_Demo_Off)
		US_InitRndT(false);
	else
		US_InitRndT(true);

	SD_SetSpriteSync(3);
	SD_SetLastTimeCount(3);
	SD_SetTimeCount(3);

	while (ck_gameState.levelState == 0)
	{

		IN_PumpEvents();
		CK_HandleInput();

		// Set, unset active objects.
		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{

			if (!currentObj->active &&
				(currentObj->clipRects.tileX2 >= RF_UnitToTile(rf_scrollXUnit) - 1) &&
				(currentObj->clipRects.tileX1 <= RF_UnitToTile(rf_scrollXUnit) + RF_PixelToTile(320) + 1) &&
				(currentObj->clipRects.tileY1 <= RF_UnitToTile(rf_scrollYUnit) + RF_PixelToTile(208) + 1) &&
				(currentObj->clipRects.tileY2 >= RF_UnitToTile(rf_scrollYUnit) - 1))
			{
				currentObj->active = OBJ_ACTIVE;
				currentObj->visible = true;
			}
			if (currentObj->active)
			{
				if ((currentObj->clipRects.tileX2 < ck_activeX0Tile) ||
					(currentObj->clipRects.tileX1 > ck_activeX1Tile) ||
					(currentObj->clipRects.tileY1 > ck_activeY1Tile) ||
					(currentObj->clipRects.tileY2 < ck_activeY0Tile))
				{
					//TODO: Add an Episode callback. Ep 4 requires
					// type 33 to remove int33 (Andy's decomp)
					// For Ep 5 that's type 7
					if (currentObj->active == OBJ_EXISTS_ONLY_ONSCREEN)
					{
						CK_RemoveObj(currentObj);
						continue;
					}
					else if (currentObj->active != OBJ_ALWAYS_ACTIVE)
					{
						if (US_RndT() < SD_GetSpriteSync() * 2 || vl_screenFaded || ck_startingSavedGame)
						{
							RF_RemoveSpriteDraw(&currentObj->sde);
							if (currentObj->type == CT_CLASS(StunnedCreature))
								RF_RemoveSpriteDrawUsing16BitOffset(&currentObj->user3);
							currentObj->active = OBJ_INACTIVE;
							continue;
						}
					}
				}
				CK_RunAction(currentObj);
			}
		}
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
		if (ck_dumperFile)
		{
			bool CK_SaveObject(FILE * fp, CK_object * o);
			bool CK_SaveGameState(FILE * fp, CK_GameState * state);

			uint32_t timecountToDump = SD_GetTimeCount();
			FS_WriteInt32LE(&timecountToDump, 1, ck_dumperFile);
			CK_SaveGameState(ck_dumperFile, &ck_gameState);
			for (CK_object *currentObj = &ck_objArray[0]; currentObj != &ck_objArray[CK_MAX_OBJECTS]; ++currentObj)
				CK_SaveObject(ck_dumperFile, currentObj);
		}
#endif

		if (ck_keenState.platform)
			CK_KeenRidePlatform(ck_keenObj);

		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{
			// Some strange Keen4 stuff here. Ignoring for now.

			if (!currentObj->active)
				continue;
			for (CK_object *collideObj = currentObj->next; collideObj; collideObj = collideObj->next)
			{
				if (!collideObj->active)
					continue;

				if ((currentObj->clipRects.unitX2 > collideObj->clipRects.unitX1) &&
					(currentObj->clipRects.unitX1 < collideObj->clipRects.unitX2) &&
					(currentObj->clipRects.unitY1 < collideObj->clipRects.unitY2) &&
					(currentObj->clipRects.unitY2 > collideObj->clipRects.unitY1))
				{
					if (currentObj->currentAction->collide)
						currentObj->currentAction->collide(currentObj, collideObj);
					if (collideObj->currentAction->collide)
						collideObj->currentAction->collide(collideObj, currentObj);
				}
			}
		}

		//TODO: If world map and keen4, check wetsuit.

		//TODO: If not world map, check keen -> item-tile collision.

		if (ca_mapOn == 0)
			ck_currentEpisode->mapMiscFlagsCheck(ck_keenObj);
		else
			CK_KeenCheckSpecialTileInfo(ck_keenObj);

		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{
			if (currentObj->active)
			{
				if (currentObj->clipRects.tileY2 >= (CA_GetMapHeight() - 1))
				{
					if (currentObj->type == CT_Player)
					{
						// Kill Keen if he exits the bottom of the map.
						ck_gameState.levelState = 1;
						continue;
					}
					else
					{
						CK_RemoveObj(currentObj);
						continue;
					}
				}
				if (currentObj->visible && currentObj->currentAction->draw)
				{
					currentObj->visible = false; //We don't need to render it twice!
					currentObj->currentAction->draw(currentObj);
				}
			}
		}

		// Follow the player with the camera.
		if (ca_mapOn == 0 || (ck_currentEpisode->ep == EP_CK4 && ca_mapOn == 17))
			CK_MapCamera(ck_keenObj);
		else
			CK_NormalCamera(ck_keenObj);

		//Draw the scorebox
		CK_UpdateScoreBox(ck_scoreBoxObj);

		if (ck_startingSavedGame)
			ck_startingSavedGame = 0;

		// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
		VL_SetScrollCoords(RF_UnitToPixel(rf_scrollXUnit & 0xef), RF_UnitToPixel(rf_scrollYUnit & 0xff));
		RF_Refresh();

		if (ck_invincibilityTimer)
		{
			ck_invincibilityTimer -= SD_GetSpriteSync();
			if (ck_invincibilityTimer < 0)
				ck_invincibilityTimer = 0;
		}

		if (ck_currentEpisode->ep == EP_CK6 && ck6_smashScreenDistance)
		{
			if ((ck6_smashScreenDistance -= SD_GetSpriteSync()) < 0)
				ck6_smashScreenDistance = 0;
		}

		//TODO: Slow-mo, extra VBLs.
		if (ck_slowMotionEnabled)
		{
			VL_DelayTics(14);
			SD_SetLastTimeCount(SD_GetTimeCount());
		}
		// TODO: Extra VBLs come here
		// If we've finished playing back our demo, or the player presses a key,
		// exit the playloop.
		if (IN_DemoGetMode() == IN_Demo_Playback)
		{
			if (!vl_screenFaded && (IN_GetLastScan() != IN_SC_None))
			{
				ck_gameState.levelState = 2;
				if (IN_GetLastScan() != IN_SC_F1)
					IN_SetLastScan(IN_SC_Space);
			}
		}
		else if (IN_DemoGetMode() == IN_Demo_PlayDone)
		{
			ck_gameState.levelState = 2;
		}
		else
		{
			CK_CheckKeys();
		}
		// End-Of-Game cheat
		if (IN_GetKeyState(IN_SC_E) && IN_GetKeyState(IN_SC_N) && IN_GetKeyState(IN_SC_D))
		{
			if (ck_currentEpisode->ep == EP_CK4)
			{
				ck_gameState.ep.ck4.membersRescued = 7;
				ck_gameState.levelState = LS_CouncilRescued;
			}
			else if (ck_currentEpisode->ep == EP_CK5)
			{
				ck_gameState.levelState = LS_DestroyedQED;
			}
			else if (ck_currentEpisode->ep == EP_CK6)
			{
				ck_gameState.levelState = LS_Molly;
			}
		}
	}
	game_in_progress = 0;
	StopMusic();
}
