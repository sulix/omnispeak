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
#include "ck_game.h"
#include "ck_def.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_us.h"
#include "id_rf.h"
#include "id_vl.h"
#include "id_sd.h"

#include "ck_act.h"
#include "ck_text.h"
#include "ck5_ep.h"

#include <stdlib.h> /* For abs() */
#include <string.h> /* For memset() */
#include <stdio.h> /* for sscanf() */

#define max(a,b) ((a<b)?b:a)
#define min(a,b) ((a<b)?a:b)

CK_object ck_objArray[CK_MAX_OBJECTS];

CK_object *ck_freeObject;
CK_object *ck_lastObject;
int ck_numObjects;

CK_object *ck_keenObj;

static CK_object tempObj;

// The rectangle within which objects are active.
int ck_activeX0Tile;
int ck_activeY0Tile;
int ck_activeX1Tile;
int ck_activeY1Tile;


void CK_KeenCheckSpecialTileInfo(CK_object *obj);
void StartMusic(uint16_t level);
void StopMusic(void);

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

// A bunch of global variables from other modules that should be
// handled better, but are just defined here for now

extern int game_in_progress;
extern int load_game_error, ck_startingSavedGame;
extern CK_Difficulty ck_startingDifficulty;
extern CK_object *ck_scoreBoxObj;

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
	US_PrintF("Active Objects : %d", active);
	US_PrintF("Inactive Object : %d", inactive);
	VL_Present();
	IN_WaitButton();
}

void CK_DebugMemory()
{
	// HACK: US_CPrint modifies strings. So don't use
	// literals directly. (char * is NOT ok; Use char arrays.)
	static char memoryUsageString[] = "Memory Usage:";
	static char memoryUsageBarString[] = "-------------";

	US_CenterWindow(16, 10);

	US_CPrint(memoryUsageString);
	US_CPrint(memoryUsageBarString);
	US_PrintF("In Use      : %dk", MM_UsedMemory() / 1024);
	US_PrintF("Blocks      : %d", MM_UsedBlocks());
	US_PrintF("Purgable    : %d", MM_PurgableBlocks());
	US_PrintF("GFX Mem Used: %dk", VL_MemUsed() / 1024);
	US_PrintF("GFX Surfaces: %d", VL_NumSurfaces());
	VL_Present();
	IN_WaitButton();
	//MM_ShowMemory();
}

void CK_ItemCheat()
{
	// HACK: US_CPrint modifies strings. So don't use
	// literals directly. (char * is NOT ok; Use char arrays.)
	static char cheatOptionString[] = "Cheat Option!\n\nYou just got all\nthe keys, 99 shots,\nand an extra keen!";

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
	US_CPrint(cheatOptionString);
	VL_Present();
	IN_WaitButton();
	//RF_Reset();
	ck_gameState.numShots = 99;
	ck_gameState.numLives++;
	ck_gameState.securityCard = 1;
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
	memset(newObj, 0, sizeof (CK_object));


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

	if (obj->type == CT5_StunnedCreature)
	{
		// FIXME: This cast is bad on 64-bit platforms
		RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user3);
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

int16_t CK_ActionThink(CK_object *obj, int16_t time)
{
	CK_action *action = obj->currentAction;


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

	if (action != obj->currentAction)
	{
		if (!obj->currentAction)
		{
			return 0;
		}
	}
	else
	{
		obj->currentAction = action->next;
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
	// TODO/FIXME(?): Vanilla code doesn't check if prevAction is non-NULL,
	// but the new code may crash as a consequence (try no-clip cheat).
	while (prevAction && ticsLeft)
	{
		if (prevAction->protectAnimation || prevAction->timer > ticsLeft)
		{
			ticsLeft = CK_ActionThink(obj, ticsLeft);
		}
		else
		{
			ticsLeft = CK_ActionThink(obj, prevAction->timer - 1);
		}

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
	if (obj->gfxChunk == -1)
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

void hackdraw(CK_object *me)
{
	RF_AddSpriteDraw(&(me->sde), (150 << 4), (100 << 4), 121, false, 0);
}

void CK_OverlayForegroundTile(int fgTile, int overlayTile)
{
	uint16_t *src = (uint16_t*)ca_graphChunks[ca_gfxInfoE.offTiles16m + overlayTile];
	uint16_t *dst = (uint16_t*)ca_graphChunks[ca_gfxInfoE.offTiles16m + fgTile];

	for (int i = 0; i < 64; i++)
	{
		uint16_t row, overlayMask, overlayColor;
		row = i & 0xF;				            // Row of the tile we're on
		overlayColor = *(src + i + 0x10);	// Get the overlay color plane
		overlayMask = *(src + row);	      // Get the overlay mask plane
		*(dst + row) &= overlayMask;      // Mask dest mask w/ overlay mask
		*(dst + 0x10 + i) &= overlayMask; // Now draw the overlayTile (mask...)
		*(dst + 0x10 + i) |= overlayColor;// (... and draw color plane)
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
		uint16_t w,h;
		// VW_SyncPages();
		US_CenterWindow(0x18, 3);
		US_SetPrintY(US_GetPrintY() + 6);
		VH_MeasurePropString(" Border color (0-15):", &w, &h, US_GetPrintFont());
		uint16_t saveX = US_GetPrintX() + w;
		uint16_t saveY = US_GetPrintY();
		US_Print(" Border color (0-15):");
		VL_Present(); // VW_UpdateScreen();

		if (US_LineInput(saveX, saveY, str, NULL, true , 2, 0))
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
			US_PrintCentered("God Mode OFF");
		else
			US_PrintCentered("God Mode ON");

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
		US_PrintCentered("Free Items!");

		for (int i = 0; i < 4; i++)
			ck_gameState.keyGems[i]++;

		ck_gameState.numShots = 99;
		ck_gameState.securityCard = 1;
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
			US_PrintCentered("\nJump cheat ON");
		else
			US_PrintCentered("\nJump cheat OFF");

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
			US_PrintCentered("No clipping ON");
			ck_keenObj->clipped = CLIP_not;
		}
		else
		{
			US_PrintCentered("No clipping OFF");
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
			US_PrintCentered("Slow motion ON");
		else
			US_PrintCentered("Slow motion OFF");
		VL_Present();
		IN_WaitButton();
		return true;
	}

	// Sprite Test

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

		if (US_LineInput(saveX, saveY, str, NULL, true , 2, 0))
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
		return;

	// Drop down status
	if (IN_GetKeyState(IN_SC_Enter))
	{

	}

	// TODO: If Paused
	if (in_Paused)
	{
		SD_MusicOff();
		// VW_SyncPages();
		US_CenterWindow(8, 3);
		US_PrintCentered("PAUSED");
		VL_Present(); // VW_UpdateScreen();
		IN_WaitButton();
		// RF_ResetScreen();
		in_Paused = false;
		SD_MusicOn();
	}

	// HELP
	if (IN_GetLastScan() == IN_SC_F1)
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
		// Go back to wristwatch
		if ((IN_GetLastScan() >= IN_SC_F2 && IN_GetLastScan() <= IN_SC_F7) || IN_GetLastScan() == IN_SC_Escape)
		{

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

		// Do Boss Key
		if (IN_GetLastScan() == IN_SC_F9)
		{

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
		if (CK_DebugKeys())
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

IN_ControlFrame ck_inputFrame;

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
			if (!ck_keenState.jumpIsPressed) ck_keenState.jumpWasPressed = false;
			if (!ck_keenState.pogoIsPressed) ck_keenState.pogoWasPressed = false;

			if (!ck_keenState.shootIsPressed) ck_keenState.shootWasPressed = false;
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
			MM_SetPurge((void **) &CA_audio[ca_audInfoE.startMusic + i], 3);
}

uint16_t *ck_levelMusic;

void StartMusic(uint16_t level)
{
	if ((level >= 20) && (level != 0xFFFF))
	{
		Quit("StartMusic() - bad level number");
	}
	SD_MusicOff();
	if (ck_levelMusic[level] == 0xFFFF)
		return;
	if (MusicMode != smm_AdLib)
		return;
	MM_BombOnError(false);
	CA_CacheAudioChunk(ca_audInfoE.startMusic + ck_levelMusic[level]);
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
	SD_StartMusic((MusicGroup *) CA_audio[ca_audInfoE.startMusic + ck_levelMusic[level]]);
}

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

	if (obj->posX < (152 << 4))
		screenX = 0;
	else
		screenX = obj->posX - (152 << 4);

	if (ca_mapOn == 0)
	{
		// World Map
		if (obj->posY < (80 << 4))
			screenY = 0;
		else
			screenY = obj->posY - (80 << 4);
	}
	else
	{
		// In Level
		if (obj->clipRects.unitY2 < (140 << 4))
			screenY = 0;

		else
			screenY = obj->clipRects.unitY2 - (140 << 4);
	}

#if 0
	// TODO: Find out why this is locking the game up
	if (!ck_inHighScores)
#endif
		RF_Reposition(screenX, screenY);

	//TODO: This is 4 in Andy's disasm.
	ck_activeX0Tile = max((rf_scrollXUnit >> 8) - 6, 0);
	ck_activeX1Tile = max((rf_scrollXUnit >> 8) + (320 >> 4) + 6, 0);
	ck_activeY0Tile = max((rf_scrollYUnit >> 8) - 6, 0);
	ck_activeY1Tile = max((rf_scrollYUnit >> 8) + (208 >> 4) + 6, 0);
}

/*
 * Move the camera that follows keen on the world map
 */

void CK_MapCamera( CK_object *keen )
{
	int16_t scr_y, scr_x;

	if (ck_scrollDisabled)
		return;

	// Scroll Left, Right, or nowhere
	if ( keen->clipRects.unitX1 < rf_scrollXUnit + (144 << 4) )
		scr_x = keen->clipRects.unitX1 - (rf_scrollXUnit + (144 << 4));
	else if ( keen->clipRects.unitX2 > rf_scrollXUnit + (192 << 4) )
		scr_x = keen->clipRects.unitX2 + 16 - (rf_scrollXUnit + (192 << 4));
	else
		scr_x = 0;

	// Scroll Up, Down, or nowhere
	if ( keen->clipRects.unitY1 < rf_scrollYUnit + (80 << 4) )
		scr_y = keen->clipRects.unitY1 - (rf_scrollYUnit + (80 << 4));
	else if ( keen->clipRects.unitY2 > rf_scrollYUnit + (112 << 4) )
		scr_y = keen->clipRects.unitY2 - (rf_scrollYUnit + (112 << 4));
	else
		scr_y = 0;

	// Limit scrolling to 256 map units (1 tile)
	// And update the active boundaries of the map
	if ( scr_x != 0 || scr_y != 0 )
	{
		if ( scr_x >= 256 )
			scr_x = 255;
		else if ( scr_x <= -256 )
			scr_x = -255;

		if ( scr_y >= 256 )
			scr_y = 255;
		else if ( scr_y <= -256 )
			scr_y = -255;

		RF_SmoothScroll( scr_x, scr_y );

		/*
		 * No ScrollX1_T in omnispeak; it's computed whenever it's needed
		 * ScrollX1_T = ScrollX0_T + VIRTUAL_SCREEN_W_T;
		 * ScrollY1_T = ScrollY0_T + VIRTUAL_SCREEN_H_T;
		 */

		//TODO: This is 4 in Andy's disasm.
		ck_activeX0Tile = max((rf_scrollXUnit >> 8) - 6, 0);
		ck_activeX1Tile = max((rf_scrollXUnit >> 8) + (320 >> 4) + 6, 0);
		ck_activeY0Tile = max((rf_scrollYUnit >> 8) - 6, 0);
		ck_activeY1Tile = max((rf_scrollYUnit >> 8) + (208 >> 4) + 6, 0);
	}
}

// Run the normal camera which follows keen

void CK_NormalCamera(CK_object *obj)
{

	int16_t deltaX = 0, deltaY = 0;	// in Units

	//TODO: some unknown var must be 0
	//This var is a "ScrollDisabled flag." If keen dies, it's set so he
	// can fall out the bottom
	if (ck_scrollDisabled)
		return;

	// End level if keen makes it out either side
	if (obj->clipRects.unitX1 < rf_scrollXMinUnit || obj->clipRects.unitX2 > rf_scrollXMaxUnit + (320 << 4))
	{
		ck_gameState.levelState = 2;
		return;
	}

	// Kill keen if he falls out the bottom
	if (obj->clipRects.unitY2 > (rf_scrollYMaxUnit + (208 << 4)))
	{
		obj->posY -= obj->clipRects.unitY2 - (rf_scrollYMaxUnit + (208 << 4));
		SD_PlaySound(SOUND_KEENFALL);
		ck_godMode = false;
		CK_KillKeen();
		return;
	}


	// Keep keen's x-coord between 144-192 pixels
	if (obj->posX < (rf_scrollXUnit + (144 << 4)))
		deltaX = obj->posX - (rf_scrollXUnit + (144 << 4));

	if (obj->posX > (rf_scrollXUnit + (192 << 4)))
		deltaX = obj->posX - (rf_scrollXUnit + (192 << 4));


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
		deltaY = (-pxToMove) << 4;

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
		deltaY = pxToMove << 4;
	}

	// If we're attached to the ground, or otherwise awesome
	// do somethink inscrutible.
	if (obj->topTI || !obj->clipped || obj->currentAction == CK_GetActionByName("CK_ACT_keenHang1"))
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
			int cmpAmt = (screenYpx << 4) + rf_scrollYUnit + deltaY;
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
	if (obj->clipRects.unitY2 < (rf_scrollYUnit + deltaY + (32 << 4)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + (32 << 4));


	if (obj->clipRects.unitY2 > (rf_scrollYUnit + deltaY + (168 << 4)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + (168 << 4));

	//Don't scroll more than one tile's worth per frame.
	if (deltaX || deltaY)
	{
		if (deltaX > 255) deltaX = 255;
		else if (deltaX < -255) deltaX = -255;

		if (deltaY > 255) deltaY = 255;
		else

			if (deltaY < -255) deltaY = -255;

		// Do the scroll!
		RF_SmoothScroll(deltaX, deltaY);

		// Update the rectangle of active objects
		ck_activeX0Tile = max((rf_scrollXUnit >> 8) - 6, 0);
		ck_activeX1Tile = max((rf_scrollXUnit >> 8) + (320 >> 4) + 6, 0);
		ck_activeY0Tile = max((rf_scrollYUnit >> 8) - 6, 0);
		ck_activeY1Tile = max((rf_scrollYUnit >> 8) + (208 >> 4) + 6, 0);
	}
}



// Play a level.

int CK_PlayLoop()
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
					(currentObj->clipRects.tileX2 >= (rf_scrollXUnit >> 8) - 1) &&
					(currentObj->clipRects.tileX1 <= (rf_scrollXUnit >> 8) + (320 >> 4) + 1) &&
					(currentObj->clipRects.tileY1 <= (rf_scrollYUnit >> 8) + (208 >> 4) + 1) &&
					(currentObj->clipRects.tileY2 >= (rf_scrollYUnit >> 8) - 1))
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
						if (US_RndT() < SD_GetSpriteSync() * 2 || vl_screenFaded)
						{
							RF_RemoveSpriteDraw(&currentObj->sde);
							if (currentObj->type == CT5_StunnedCreature)
								RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & currentObj->user3);
							currentObj->active = OBJ_INACTIVE;
							continue;
						}
					}
				}
				CK_RunAction(currentObj);
			}
		}


		if (ck_keenState.platform)
			CK_KeenRidePlatform(ck_keenObj);

		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{
			// Some strange Keen4 stuff here. Ignoring for now.


			if (!currentObj->active) continue;
			for (CK_object *collideObj = currentObj->next; collideObj; collideObj = collideObj->next)
			{
				if (!collideObj->active)
					continue;

				if (	(currentObj->clipRects.unitX2 > collideObj->clipRects.unitX1) &&
						(currentObj->clipRects.unitX1 < collideObj->clipRects.unitX2) &&
						(currentObj->clipRects.unitY1 < collideObj->clipRects.unitY2) &&
						(currentObj->clipRects.unitY2 > collideObj->clipRects.unitY1) )
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
			CK_MapMiscFlagsCheck(ck_keenObj);
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
					currentObj->visible = false;	//We don't need to render it twice!
					currentObj->currentAction->draw(currentObj);
				}
			}
		}

		// Follow the player with the camera.
		if (ca_mapOn == 0)
			CK_MapCamera(ck_keenObj);
		else
			CK_NormalCamera(ck_keenObj);

		//Draw the scorebox
		CK_UpdateScoreBox(ck_scoreBoxObj);

		// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
		VL_SetScrollCoords((rf_scrollXUnit & 0xef) >> 4, (rf_scrollYUnit & 0xff) >> 4);
		RF_Refresh();



		if (ck_invincibilityTimer)
		{
			ck_invincibilityTimer -= SD_GetSpriteSync();
			if (ck_invincibilityTimer < 0)
				ck_invincibilityTimer = 0;
		}

		//TODO: Slow-mo, extra VBLs.
		if (ck_slowMotionEnabled)
		{
			VL_DelayTics(14);
			SD_SetLastTimeCount(SD_GetTimeCount());
		}
		// TODO: Extra VBLs come here
		VL_Present();
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
			ck_gameState.levelState = 15;
		}
	}
	game_in_progress = 0;
	StopMusic();
}
