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

#include "ck_cross.h" /* For CK_Cross_SwapLE16 */

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
	US_CenterWindow(16, 10);

	US_CPrint("Memory Usage:");
	US_CPrint("-------------");
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
	US_CPrint("Cheat Option!\n\nYou just got all\nthe keys, 99 shots,\nand an extra keen!");
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

	if (obj->type == CT_CLASS(StunnedCreature))
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

void hackdraw(CK_object *me)
{
	RF_AddSpriteDraw(&(me->sde), (150 << 4), (100 << 4), 121, false, 0);
}

void CK_OverlayForegroundTile(int fgTile, int overlayTile)
{
	// Some random DOSBox memory dump, resolving crashes with NULL pointers and
	// somewhat emulating the behaviors of vanilla Keen in DOSBox (at least Keen 4)
	//
	// Note: CK_WallDebug makes sure dst is never NULL; It's src which may be NULL.
	static const uint16_t nullTile[80] =
	{
		CK_Cross_SwapLE16(0x0162), CK_Cross_SwapLE16(0x01A2), CK_Cross_SwapLE16(0x0008), CK_Cross_SwapLE16(0x0070), CK_Cross_SwapLE16(0x0008), CK_Cross_SwapLE16(0x0070), CK_Cross_SwapLE16(0x0008), CK_Cross_SwapLE16(0x0070),
		CK_Cross_SwapLE16(0x0008), CK_Cross_SwapLE16(0x0070), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x05F5), CK_Cross_SwapLE16(0x1A16), CK_Cross_SwapLE16(0x000B), CK_Cross_SwapLE16(0x1602), CK_Cross_SwapLE16(0xFF55), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1080), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x1320), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1120), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1140), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1160), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x11C0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x11E0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1200), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1240), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x12E0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x12E0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1260), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000),
		CK_Cross_SwapLE16(0x1280), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0xF0A4), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1060), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x0500), CK_Cross_SwapLE16(0xC000),
		CK_Cross_SwapLE16(0x14A0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x14C0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x20C8), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x0000), CK_Cross_SwapLE16(0x0118),
		CK_Cross_SwapLE16(0x1AAB), CK_Cross_SwapLE16(0x01A2), CK_Cross_SwapLE16(0x14E0), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1500), CK_Cross_SwapLE16(0xF000), CK_Cross_SwapLE16(0x1520), CK_Cross_SwapLE16(0xF000),
	};

	const uint16_t *src = (uint16_t*)ca_graphChunks[ca_gfxInfoE.offTiles16m + overlayTile];
	uint16_t *dst = (uint16_t*)ca_graphChunks[ca_gfxInfoE.offTiles16m + fgTile];

	src = src ? src : nullTile;

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
    if (ck_currentEpisode->ep == EP_CK4)
      ck_gameState.ep.ck4.wetsuit = 1;
    else if (ck_currentEpisode->ep == EP_CK5)
      ck_gameState.ep.ck5.securityCard = 1;
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
    CK_ShowStatusWindow();
    RF_ForceRefresh(); // Reanimate the tiles
    SD_SetLastTimeCount(SD_GetTimeCount());
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

	if ((song == -1) || (MusicMode != smm_AdLib))
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
	SD_StartMusic((MusicGroup *) CA_audio[ca_audInfoE.startMusic + song]);
}

//===========================================================================
//
// DROPDOWN STATUS WINDOW
//
//===========================================================================


#define SOUND_STATUSDOWN 34
#define SOUND_STATUSUP 35

void *ck_statusSurface;

// The status window is drawn to scratch area in the video buffer
// Then it is copied from the scratch to the screen using the RF_ Hook

void CK_DrawLongRight(int x, int y, int digits, int zerotile, int value);
void CK_DrawStatusWindow(void);
void CK_ScrollStatusWindow(void);
void CK_ShowStatusWindow(void);

// extern const char **ck_levelNames;
int ck_statusWindowYPx;
bool ck_statusDown;
unsigned statusWindowOfs;  // screen buffer


void CK_DrawLongRight(int x, int y, int digits, int zerotile, int value)
{
	char s[20];
	int len,i;


		sprintf(s, "%d", value);
	// itoa(value, s, 10);
	len = strlen(s);

	for (i = digits; i > len; i--)
	{
		VH_DrawTile8(x, y, zerotile);
		x+=8;
	}

	while (i > 0)
	{
		VH_DrawTile8(x,y,zerotile + s[len-i] - 47);
		i--;
		x+=8;
	}

}

void CK_DrawStatusWindow(void)
{
	int si, i, oldcolor;


	// Draw the backdrop
	int var2 = 64;
	int di = 16;
	int statusWidth = STATUS_W-8;
	int statusHeight = STATUS_H-8;


	VH_DrawTile8(var2, di, 54);
	VH_DrawTile8(var2, di+statusHeight, 60);
	si = var2 + 8;

	while (var2 + statusWidth - 8 >= si)
	{

		VH_DrawTile8(si, di, 55);
		VH_DrawTile8(si, di + statusHeight, 61);
		si += 8;
	}

	VH_DrawTile8(si, di, 56);
	VH_DrawTile8(si, di + statusHeight, 62);

	si = di + 8;

	while (di + statusHeight - 8 >= si)
	{

		VH_DrawTile8(var2, si, 57);
		VH_DrawTile8(var2+statusWidth, si, 59);
		si += 8;
	}

	VH_Bar(72, 24, 176, 136, 7);

	// Print the stuff
	oldcolor = US_GetPrintColour();

	// Level
  uint16_t strW, strH;
  char str[256];
	US_SetPrintY(28);
	US_SetWindowX(80);
	US_SetWindowW(160);
	US_SetPrintColour(15);
	US_CPrint("LOCATION");
	VH_Bar(79, 38, 162, 20, 15);
  strcpy(str, ck_levelNames[ca_mapOn]);
	CK_MeasureMultiline(str, &strW, &strH);
	US_SetPrintY ((20 - strH)/2 + 40 - 2);
	US_CPrint(str);

	// Score
	US_SetPrintY(61);
	US_SetWindowX(80);
	US_SetWindowW(64);
	US_SetPrintColour(15);
	US_CPrint("SCORE");

	VH_Bar(79, 71, 66, 10, 0);
	CK_DrawLongRight(80, 72, 8, 41, ck_gameState.keenScore);

	// Extra man
	US_SetPrintY(61);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_CPrint("EXTRA");
	VH_Bar(175, 71, 66, 10, 0);
	CK_DrawLongRight(176, 72, 8, 41, ck_gameState.nextKeenAt);

  // Episode-dependent field
  switch (ck_currentEpisode->ep)
  {
    case EP_CK4:
      US_SetPrintY(85);
      US_SetWindowX(80);
      US_SetWindowW(64);
      US_CPrint("RESCUED");

      VH_Bar(79, 95, 66, 10, 0);
      for (int i = 0; i < ck_gameState.ep.ck4.membersRescued; i++) {
        VH_DrawTile8(80 + 8 * i, 96, 40);
      }
      break;

    case EP_CK5:
      US_SetPrintY(91);
      US_SetPrintX(80);
      US_Print("KEYCARD");

      VH_Bar(135, 90, 10, 10, 0);
      if (ck_gameState.ep.ck5.securityCard)
        VH_DrawTile8(136, 91, 40);
      break;
  }

	// Difficulty
	US_SetPrintY(85);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_SetPrintColour(15);
	US_CPrint("LEVEL");
	VH_Bar(175, 95, 66, 10, 15);

	US_SetPrintY(96);
	US_SetWindowX(176);
	US_SetWindowW(64);
	US_SetPrintColour(15);

	switch (ck_gameState.difficulty)
	{
	case 1:
		US_CPrint("Easy");
		break;
	case 2:
		US_CPrint("Normal");
		break;
	case 3:
		US_CPrint("Hard");
		break;
	}

	// Key gems
	US_SetPrintX(80);
	US_SetPrintY(112);
	US_SetPrintColour(15);
	US_Print("KEYS");

	VH_Bar(119, 111, 34, 10, 0);

	for (i = 0; i < 4; i++)
	{
		if (ck_gameState.keyGems[i])
			VH_DrawTile8(120+i*8, 112, 36+i);
	}

	// AMMO
	US_SetPrintX(176);
	US_SetPrintY(112);
	US_Print("AMMO");
	VH_Bar(215, 111, 26, 10, 0);
	CK_DrawLongRight(216, 112, 3, 41, ck_gameState.numShots);

	// Lives
	US_SetPrintX(80);
	US_SetPrintY(128);
	US_Print("KEENS");
	VH_Bar(127, 127, 18, 10, 0);
	CK_DrawLongRight(128, 128, 2, 41, ck_gameState.numLives);

	// Lifeups
	US_SetPrintX(176);
	US_SetPrintY(128);
  switch (ck_currentEpisode->ep)
  {
    case EP_CK4:
      US_Print("DROPS");
      break;
    case EP_CK5:
      US_Print("VITALIN");
      break;
  }
	VH_Bar(224, 127, 16, 10, 0);
	CK_DrawLongRight(224, 128, 2, 41, ck_gameState.numCentilife);

  // Episode-dependent field
  int addX = 0;
  switch (ck_currentEpisode->ep)
  {

    case EP_CK4:

      // Wetsuit
      VH_Bar(79, 143, 66, 10, 15);

      US_SetPrintY(144);
      US_SetWindowX(80);
      US_SetWindowW(64);
      US_SetPrintColour(15);
      US_CPrint(ck_gameState.ep.ck4.wetsuit ? "Wetsuit" : "???");

      addX = 5;

    case EP_CK5:

      for (int y = 0; y < 2; y++)
        for (int x = 0; x < 10; x++)
          VH_DrawTile8(120 + 8 * (x + addX), 140 + 8 * y, 72 + y*10+x);

      break;
  }

	US_SetPrintColour(oldcolor);
}

void CK_ScrollStatusWindow(void)
{
	int dest, height, source;
  int dx, dy, sx, sy;

  int scrX = VL_GetScrollX();
  int scrY = VL_GetScrollY();

	if (ck_statusWindowYPx > 152)
	{
    // In DOS keen, the bit of tilemap behind the status window would need to be
    // redrawn after the top border of the status window scrolled on to the screen
#if 0
		height = ck_statusWindowYPx - 152;
		source = statusWindowOfs + panadjust + 8;
		dest = bufferofs + panadjust + 8;
		VW_ScreenToScreen(source, dest, 24, height);
#endif

    // MPic atop the statusbox
		// VW_ClipDrawMPic((pansx + 136)/8, pansy - (16-height), STATUSTOPPICM);
		height = 152;
		sx = 64; sy = 16; // source = statusWindowOfs + panadjust + 0x408;
    dx = 64; dy = ck_statusWindowYPx - height; // dest = bufferofs + panadjust + (height << 6) + 8;

    VH_DrawMaskedBitmap(136 + scrX % 16, 16-dy + scrY % 16, MPIC_STATUSRIGHT);
	}
	else
	{
		// source = statusWindowOfs + panadjust + ((152-ck_statusWindowYPx)<<6) + 0x408;
		// dest = bufferofs + panadjust + 8;

		height = ck_statusWindowYPx;

    dx = 64;
    dy = 0;
    sx = 64;
    sy = 16 + STATUS_H - height;

	}

	if (height > 0)
  {
		//VW_ScreenToScreen(source, dest, 24, height);
    VL_SurfaceToScreen(ck_statusSurface, dx + scrX % 16, dy + scrY % 16, sx, sy, STATUS_W, height);
  }

	// Draw the tile map underneath the scrolling status box
#if 0
	if (ck_statusDown)
	{
		// Coming back up, need to redraw the map back underneath
		height = 168 - ck_statusWindowYPx;
		source = masterofs + panadjust + (ck_statusWindowYPx << 6) + 8;
		dest = bufferofs + panadjust + (ck_statusWindowYPx << 6) + 8;
		VW_ScreenToScreen(source, dest, 24, height);

		// Draw underneath the left masked pic
		height = ck_statusWindowYPx;
		source = statusWindowOfs + panadjust + 8 - 3;
		dest = bufferofs + panadjust + 8 - 3;

		if (height > 0)
			VW_ScreenToScreen(source, dest, 3, height);

	}
	else
	{
		// Going down
		height = ck_statusWindowYPx - 72;

		if (height > 0)
		{
			source = statusWindowOfs + panadjust + 8 - 3;
			dest = bufferofs + panadjust + 8 - 3;
			// if (height > 0) // ??
				VW_ScreenToScreen(source, dest, 3, height);
		}
	}
#endif

	if (ck_statusWindowYPx >= 72)
		//VW_ClipDrawMPic((pansx + 40)/8, pansy + ck_statusWindowYPx - 168, STATUSLEFTPICM);
    VH_DrawMaskedBitmap(40 + scrX % 16, ck_statusWindowYPx - 168 + scrY % 16, MPIC_STATUSLEFT);

  VL_Present();//VW_UpdateScreen();
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
		US_SetPrintY(US_GetPrintY()+2);
		US_Print("Debug keys active");
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
	vl_emuegavgaadapter.screen = ck_statusSurface;
	CK_DrawStatusWindow();
	vl_emuegavgaadapter.screen = screen;
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


		if ((ck_statusWindowYPx += SD_GetSpriteSync()*8) <= 168)
			continue;

		ck_statusWindowYPx = 168;

	} while (1);

	RF_Refresh();
	RF_SetDrawFunc(NULL);
	IN_ClearKeysDown();
	IN_WaitButton();//IN_Ack();


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


		if ((ck_statusWindowYPx -= SD_GetSpriteSync()*8) >= 0)
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
							if (currentObj->type == CT_CLASS(StunnedCreature))
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
					currentObj->visible = false;	//We don't need to render it twice!
					currentObj->currentAction->draw(currentObj);
				}
			}
		}

		// Follow the player with the camera.
		if (ca_mapOn == 0 || ck_currentEpisode->ep == EP_CK4 && ca_mapOn == 17)
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
      if (ck_currentEpisode->ep == EP_CK4)
      {
        ck_gameState.ep.ck4.membersRescued = 7;
        ck_gameState.levelState = LS_CouncilRescued;
      }
      else if (ck_currentEpisode->ep == EP_CK5)
      {
        ck_gameState.levelState = 15;
      }
    }
  }
	game_in_progress = 0;
	StopMusic();
}
