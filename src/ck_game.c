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

/*
 * CK_GAME: Holds the game loop and related functions.
 */

#include <stdbool.h>
#include <string.h>

#include "id_ca.h"
#include "id_fs.h"
#include "id_rf.h"
#include "id_us.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_config.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_play.h"
#include "ck_text.h"
#include "ck4_ep.h"
#include "ck5_ep.h"
#include "ck6_ep.h"

// =========================================================================

static int ck_lastLevelFinished;

// =========================================================================

// Purge stuff for endgame
void CK_EndingPurge()
{
	for (int i = ca_gfxInfoE.offSprites; i < ca_gfxInfoE.offTiles8; i++)
	{
		if (ca_graphChunks[i])
			MM_SetPurge(ca_graphChunks + i, 1);
	}

	for (int i = ca_gfxInfoE.offTiles16; i < ca_gfxInfoE.offBinaries; i++)
	{
		if (ca_graphChunks[i])
			MM_SetPurge(ca_graphChunks + i, 1);
	}
}

/*
 * NewGame: Setup the default starting stats
 */

void CK_NewGame()
{
	// TODO: Zero the ck_gameState
	memset(&ck_gameState, 0, sizeof(ck_gameState));
	ck_gameState.nextKeenAt = 20000;
	ck_gameState.numLives = 3;
	ck_gameState.numShots = 5;
	ck_gameState.currentLevel = 0;
}

void CK_GameOver()
{
	// VW_FixRefreshBuffer(); // Omnispeak TODO - This was originally called
	US_CenterWindow(16, 3);
	US_PrintCentered(CK_VAR_GetStr("ck_str_gameOver"));
	VL_Present(); // VW_UpdateScreen();
	IN_ClearKeysDown();
	IN_UserInput(4 * 70, false);
}

// OMNISPEAK - New cross-platform methods for reading/writing objects from/to saved games
bool CK_SaveObject(FILE *fp, CK_object *o)
{
	int16_t dummy = 0;
	// Convert a few enums
	int16_t activeInt = (int16_t)(o->active);
	int16_t clippedInt = (int16_t)(o->clipped);
	// BACKWARD COMPATIBILITY
	uint16_t statedosoffset = o->currentAction ? o->currentAction->compatDosPointer : 0;
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
	// Used for debugging
	uint16_t sde = RF_ConvertSpriteArrayPtrTo16BitOffset(o->sde);
	uint16_t next = CK_ConvertObjPointerTo16BitOffset(o->next);
	uint16_t prev = CK_ConvertObjPointerTo16BitOffset(o->prev);
#else
	// Just tells if "o->next" is zero or not
	int16_t isnext = o->next ? 1 : 0;
#endif
	// clang-format off
	// Now writing
	return ((FS_WriteInt16LE(&o->type, 1, fp) == 1)
	        && (FS_WriteInt16LE(&activeInt, 1, fp) == 1)
	        && (FS_WriteBoolTo16LE(&o->visible, 1, fp) == 1)
	        && (FS_WriteInt16LE(&clippedInt, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->timeUntillThink, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->posX, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->posY, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->xDirection, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->yDirection, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->deltaPosX, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->deltaPosY, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->velX, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->velY, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->actionTimer, 1, fp) == 1)
	        && (FS_WriteInt16LE(&statedosoffset, 1, fp) == 1) // BACKWARD COMPATIBILITY
	        && (FS_WriteInt16LE(&o->gfxChunk, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->zLayer, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.unitX1, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.unitY1, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.unitX2, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.unitY2, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.unitXmid, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.tileX1, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.tileY1, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.tileX2, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.tileY2, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->clipRects.tileXmid, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->topTI, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->rightTI, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->bottomTI, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->leftTI, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->user1, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->user2, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->user3, 1, fp) == 1)
	        && (FS_WriteInt16LE(&o->user4, 1, fp) == 1)
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
	        && (FS_WriteInt16LE(&sde, 1, fp) == 1)
	        && (FS_WriteInt16LE(&next, 1, fp) == 1)
	        && (FS_WriteInt16LE(&prev, 1, fp) == 1)
#else
	        // No need to write sde, prev pointers as-is,
	        // these are ignored on loading. So write dummy value.
	        // Furthermore, all we need to know about next on loading is
	        // if it's zero or not.
	        && (FS_WriteInt16LE(&dummy, 1, fp) == 1) // sde
	        && (FS_WriteInt16LE(&isnext, 1, fp) == 1) // next
	        && (FS_WriteInt16LE(&dummy, 1, fp) == 1) // prev
#endif
	);
	// clang-format on
}

static bool CK_LoadObject(FILE *fp, CK_object *o)
{
	int16_t dummy;
	// Convert a few enums
	int16_t activeInt, clippedInt;
	// BACKWARD COMPATIBILITY
	uint16_t statedosoffset;
	// Just tells if "o->next" is zero or not
	int16_t isnext;
	// Now reading
	// clang-format off
	if ((FS_ReadInt16LE(&o->type, 1, fp) != 1)
	    || (FS_ReadInt16LE(&activeInt, 1, fp) != 1)
	    || (FS_ReadBoolFrom16LE(&o->visible, 1, fp) != 1)
	    || (FS_ReadInt16LE(&clippedInt, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->timeUntillThink, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->posX, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->posY, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->xDirection, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->yDirection, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->deltaPosX, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->deltaPosY, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->velX, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->velY, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->actionTimer, 1, fp) != 1)
	    || (FS_ReadInt16LE(&statedosoffset, 1, fp) != 1) // BACKWARD COMPATIBILITY
	    || (FS_ReadInt16LE(&o->gfxChunk, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->zLayer, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.unitX1, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.unitY1, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.unitX2, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.unitY2, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.unitXmid, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.tileX1, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.tileY1, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.tileX2, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.tileY2, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->clipRects.tileXmid, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->topTI, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->rightTI, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->bottomTI, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->leftTI, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->user1, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->user2, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->user3, 1, fp) != 1)
	    || (FS_ReadInt16LE(&o->user4, 1, fp) != 1)
	    // No need to read sde, prev pointers as-is,
	    // these are ignored on loading. So read dummy value.
	    // Furthermore, all we need to know about next on loading is
	    // if it's zero or not.
	    || (FS_ReadInt16LE(&dummy, 1, fp) != 1) // sde
	    || (FS_ReadInt16LE(&isnext, 1, fp) != 1) // next
	    || (FS_ReadInt16LE(&dummy, 1, fp) != 1) // prev
	)
		return false;
	// clang-format on

	o->active = (CK_objActive)activeInt;
	o->clipped = (CK_ClipType)clippedInt;
	o->currentAction = CK_LookupActionFrom16BitOffset(statedosoffset);
	// HACK: All we need to know is if next was originally NULL or not
	o->next = isnext ? o : NULL;
	return true;
}

// Similar new methods for writing/reading game state
bool CK_SaveGameState(FILE *fp, CK_GameState *state)
{
	int16_t difficultyInt = (int16_t)state->difficulty; // Convert enum
	// TODO - platform should be a part of the game state
	uint16_t platformObjOffset = CK_ConvertObjPointerTo16BitOffset(ck_keenState.platform);

	// clang-format off
	return ((FS_WriteInt16LE(&state->mapPosX, 1, fp) == 1)
	        && (FS_WriteInt16LE(&state->mapPosY, 1, fp) == 1)
	        && (FS_WriteInt16LE(state->levelsDone, sizeof(state->levelsDone)/2, fp) == sizeof(state->levelsDone)/2)
	        && (FS_WriteInt32LE(&state->keenScore, 1, fp) == 1)
	        && (FS_WriteInt32LE(&state->nextKeenAt, 1, fp) == 1)
	        && (FS_WriteInt16LE(&state->numShots, 1, fp) == 1)
	        && (FS_WriteInt16LE(&state->numCentilife, 1, fp) == 1)
	        && (((ck_currentEpisode->ep == EP_CK4)
	            && (FS_WriteInt16LE(&state->ep.ck4.wetsuit, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck4.membersRescued, 1, fp) == 1)
	         )
	         || ((ck_currentEpisode->ep == EP_CK5)
	            && (FS_WriteInt16LE(&state->ep.ck5.securityCard, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck5.word_4729C, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck5.fusesRemaining, 1, fp) == 1)
	         )
	         || ((ck_currentEpisode->ep == EP_CK6)
	            && (FS_WriteInt16LE(&state->ep.ck6.sandwich, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck6.rope, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck6.passcard, 1, fp) == 1)
	            && (FS_WriteInt16LE(&state->ep.ck6.inRocket, 1, fp) == 1)
	         )
	        )
	        && (FS_WriteInt16LE(state->keyGems, sizeof(state->keyGems)/2, fp) == sizeof(state->keyGems)/2)
	        && (FS_WriteInt16LE(&state->currentLevel, 1, fp) == 1)
	        && (FS_WriteInt16LE(&state->numLives, 1, fp) == 1)
	        && (FS_WriteInt16LE(&difficultyInt, 1, fp) == 1)
	        && (FS_WriteInt16LE(&platformObjOffset, 1, fp) == 1) // BACKWARDS COMPATIBILITY
	);
	// clang-format on
}

static bool CK_LoadGameState(FILE *fp, CK_GameState *state)
{
	int16_t difficultyInt; // Convert num
	uint16_t platformObjOffset;
	// clang-format off
	if ((FS_ReadInt16LE(&state->mapPosX, 1, fp) != 1)
	    || (FS_ReadInt16LE(&state->mapPosY, 1, fp) != 1)
	    || (FS_ReadInt16LE(state->levelsDone, sizeof(state->levelsDone)/2, fp) != sizeof(state->levelsDone)/2)
	    || (FS_ReadInt32LE(&state->keenScore, 1, fp) != 1)
	    || (FS_ReadInt32LE(&state->nextKeenAt, 1, fp) != 1)
	    || (FS_ReadInt16LE(&state->numShots, 1, fp) != 1)
	    || (FS_ReadInt16LE(&state->numCentilife, 1, fp) != 1)
	    || ((ck_currentEpisode->ep == EP_CK4) &&
	       (
	        (FS_ReadInt16LE(&state->ep.ck4.wetsuit, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck4.membersRescued, 1, fp) != 1)
	     )
	    )
	    || ((ck_currentEpisode->ep == EP_CK5) &&
	       (
	        (FS_ReadInt16LE(&state->ep.ck5.securityCard, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck5.word_4729C, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck5.fusesRemaining, 1, fp) != 1)
	     )
	    )
	    || ((ck_currentEpisode->ep == EP_CK6) &&
	       (
	        (FS_ReadInt16LE(&state->ep.ck6.sandwich, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck6.rope, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck6.passcard, 1, fp) != 1)
	        || (FS_ReadInt16LE(&state->ep.ck6.inRocket, 1, fp) != 1)
	     )
	    )
	    || (FS_ReadInt16LE(state->keyGems, sizeof(state->keyGems)/2, fp) != sizeof(state->keyGems)/2)
	    || (FS_ReadInt16LE(&state->currentLevel, 1, fp) != 1)
	    || (FS_ReadInt16LE(&state->numLives, 1, fp) != 1)
	    || (FS_ReadInt16LE(&difficultyInt, 1, fp) != 1)
	    || (FS_ReadInt16LE(&platformObjOffset, 1, fp) != 1) // BACKWARDS COMPATIBILITY
	)
		return false;
	// clang-format on

	state->difficulty = (CK_Difficulty)difficultyInt;
	// TODO - platform should be a part of the game state
	ck_keenState.platform = CK_ConvertObj16BitOffsetToPointer(platformObjOffset);
	return true;
}

bool CK_SaveGame(FILE *fp)
{
	int i;
	uint16_t cmplen, bufsize;
	CK_object *obj;
	uint8_t *buf;

	/* This saves the game */

	/* Write out Keen stats */
	ck_keenState.platform = NULL;
	if (!CK_SaveGameState(fp, &ck_gameState))
		return false;

	bufsize = CA_GetMapWidth() * CA_GetMapHeight() * 2;
	MM_GetPtr((mm_ptr_t *)&buf, bufsize);

	/* Compress and save the current level */
	for (i = 0; i < 3; i++)
	{
		cmplen = CAL_RLEWCompress(CA_TilePtrAtPos(0, 0, i), bufsize, buf + 2, 0xABCD);

		/* Write the size of the compressed level */
		*((uint16_t *)buf) = cmplen;
		cmplen /= 2;
		if (FS_WriteInt16LE(buf, cmplen + 1, fp) != cmplen + 1)
		{
			/* Free the buffer and return failure */
			MM_FreePtr((mm_ptr_t *)&buf);
			return false;
		}
	}

	/* Save all the objects */
	for (obj = ck_keenObj; obj != NULL; obj = obj->next)
	{
		if (!CK_SaveObject(fp, obj))
		{
			/* Free the buffer and return failure */
			MM_FreePtr((mm_ptr_t *)&buf);
			return false;
		}
	}

	/* Free the buffer and return success */
	MM_FreePtr((mm_ptr_t *)&buf);
	return true;
}

bool CK_LoadGame(FILE *fp, bool fromMenu)
{
	int i;
	uint16_t cmplen, bufsize;
	int16_t prevFuses;
	CK_object *objprev, *objnext, *moreObj;
	uint8_t *buf;

	if (!CK_LoadGameState(fp, &ck_gameState))
		return false;

	if (ck_currentEpisode->ep == EP_CK5)
		prevFuses = ck_gameState.ep.ck5.fusesRemaining;

	if (fromMenu)
	{
		// loading from menu -> load the level into the cache stack
		// entry *below* the menu
		ca_levelbit >>= 1;
		ca_levelnum--;
		CK_LoadLevel(false, false);
		/* TODO -- reimplement
		if (mmerror)
		{
			mmerror = false;
			US_CenterWindow(20, 8);
			US_SetPrintY(20);
			US_Print("Not enough memory\nto load game!");
			VL_Present(); //VW_UpdateScreen();
			IN_Ack();
			return false;
		}
		*/
		ca_levelbit <<= 1;
		ca_levelnum++;
	}
	else
	{
		// quickloading -> replace the currently cached level
		CK_LoadLevel(true, true);
	}

	bufsize = CA_GetMapWidth() * CA_GetMapHeight() * 2;
	// MM_BombOnError(true) // TODO
	MM_GetPtr((mm_ptr_t *)&buf, bufsize);
	// TODO
	/*
	MM_BombOnError(false)
	if (mmerror)
	{
		mmerror = false;
		US_CenterWindow(20, 8);
		US_SetPrintY(20);
		US_Print("Not enough memory\nto load game!");
		VL_Present(); //VW_UpdateScreen();
		IN_Ack();
		return false;
	}
*/
	/* Decompress and load the level */
	for (i = 0; i < 3; i++)
	{
		if (FS_ReadInt16LE(&cmplen, 1, fp) != 1)
		{
			MM_FreePtr((mm_ptr_t *)&buf);
			return false;
		}
		cmplen /= 2;
		if (FS_ReadInt16LE(buf, cmplen, fp) != cmplen)
		{
			MM_FreePtr((mm_ptr_t *)&buf);
			return false;
		}

		CAL_RLEWExpand(buf, CA_TilePtrAtPos(0, 0, i), bufsize, 0xABCD);
	}

	MM_FreePtr((mm_ptr_t *)&buf);

	CK_SetupObjArray();
	CK_object *newObj = ck_keenObj;

	/* Read the first object (keen) from the list */
	objprev = newObj->prev;
	objnext = newObj->next;
	if (!CK_LoadObject(fp, newObj))
		return false;
	newObj->prev = objprev;
	newObj->next = objnext;

	newObj->visible = true;
	newObj->sde = NULL;
	newObj = ck_scoreBoxObj;

	while (1)
	{
		/* Read the object from the file and put it in the list */
		objprev = newObj->prev;
		objnext = newObj->next;
		if (!CK_LoadObject(fp, newObj))
			return 0;
		moreObj = newObj->next;
		newObj->prev = objprev;
		newObj->next = objnext;

		newObj->visible = true;
		newObj->sde = NULL;

		/* Omit stale sprite draw pointers */
		if (newObj->type == CT_CLASS(StunnedCreature))
			newObj->user3 = 0;
		else if (ck_currentEpisode->ep == EP_CK4)
		{
			if (newObj->type == CT4_Platform)
				newObj->user2 = newObj->user3 = 0;
		}
		else if (ck_currentEpisode->ep == EP_CK5)
		{
			if (newObj->type == CT5_Mine)
				newObj->user4 = 0;
			else if (newObj->type == CT5_Sphereful)
				newObj->user1 = newObj->user2 = newObj->user3 = newObj->user4 = 0;
		}
		else if (ck_currentEpisode->ep == EP_CK6)
		{
			if (newObj->type == CT6_Platform)
				newObj->user3 = 0;
		}

		/* If this is the last object in the saved list, exit the loop */
		if (!moreObj)
			break;

		/* Otherwise we add a new object */
		newObj = CK_GetNewObj(false);
	}

	ck_scoreBoxObj->user1 = -1;
	ck_scoreBoxObj->user2 = -1;
	ck_scoreBoxObj->user3 = -1;
	ck_scoreBoxObj->user4 = -1;

	if (ck_currentEpisode->ep == EP_CK5)
		ck_gameState.ep.ck5.fusesRemaining = prevFuses;

	return true;
}

//TODO: KillKeen

void CK_ExitMenu(void)
{
	CK_NewGame();
	ca_levelnum--;
	ca_levelbit >>= 1;
	CA_ClearMarks();
	ca_levelbit <<= 1;
	ca_levelnum++;
}

void CK_MapLevelMarkAsDone(void)
{
	int y, x, level, i, w, flags;
	uint16_t *pw;

	i = 0;
	pw = CA_TilePtrAtPos(0, 0, 2); /* info layer */

	/* Look through the map for level-related tiles */
	for (y = 0; y < CA_GetMapHeight(); y++)
	{
		for (x = 0; x < CA_GetMapWidth(); x++, pw++, i++)
		{
			w = *pw;
			level = w & 0xFF;
			if (level >= 1 && level <= ck_currentEpisode->lastLevelToMarkAsDone && ck_gameState.levelsDone[level])
			{ /* Is this a level tile */
				flags = w >> 8;
				/* Set the info tile at this position to 0 */
				*pw = 0;
				if (flags == 0xD0)
				{ /* If this is a 'blocking' part of the level */
					/* Set the foreground tile at this position to 0 also (remove the fences) */
					CA_SetTileAtPos(x, y, 1, 0);
				}
				else if (flags == 0xF0)
				{ /* If this is the flag holder for the level */
					if (ck_currentEpisode->ep != EP_CK5 && ck_lastLevelFinished == level)
						CK_FlippingFlagSpawn(x, y);
					else
						CK_MapFlagSpawn(x, y);
				}
			}
		}
	}
}

static int16_t ck_fadeDrawCounter;

void CK_UpdateFadeDrawing(void)
{
	ck_fadeDrawCounter++;
	if (ck_fadeDrawCounter == 2)
	{
		VL_FadeFromBlack();
		RF_SetDrawFunc(0);
		SD_SetTimeCount(SD_GetLastTimeCount());
	}
}

void CK_BeginFadeDrawing(void)
{
	VL_FadeToBlack();
	ck_fadeDrawCounter = 0;
	RF_SetDrawFunc(&CK_UpdateFadeDrawing);
}

void CK_LoadLevel(bool doCache, bool silent)
{
	if (IN_DemoGetMode() != IN_Demo_Off)
	{
		// If we're recording or playing back a demo, the game needs
		// to be deterministic. Seed the RNG at 0 and set difficulty
		// to normal.
		US_InitRndT(false);
		ck_gameState.difficulty = D_Normal;
	}
	else
	{
		US_InitRndT(true);
	}

	CA_CacheMap(ck_gameState.currentLevel);
	RF_NewMap();
	CA_ClearMarks();

	CK_SetupObjArray(); // This is done inside ScanInfoLayer in CK4
	ck_currentEpisode->scanInfoLayer();

	if (ca_mapOn == 0)
	{
		CK_MapLevelMarkAsDone();
	}

	RF_MarkTileGraphics();
	//MM_BombOnError();
	CA_LoadAllSounds();

	// Cache Marked graphics and draw loading box
	if (doCache)
	{
		if (ck_inHighScores || silent)
		{
			CA_CacheMarks(NULL);
		}
		else if (IN_DemoGetMode() != IN_Demo_Off)
		{
			CA_CacheMarks(CK_VAR_GetStr("ck_str_demo"));
		}
		else if (ck_currentEpisode->ep == EP_CK5 && ca_mapOn == 0 && ck_keenObj->clipRects.tileY1 > 100)
		{
			/* Stepping on to korath*/
			CA_CacheMarks(CK_VAR_GetStr("ck5_str_korath3"));
		}
		else
		{
			CA_CacheMarks(CK_VAR_GetStringByNameAndIndex("ck_str_levelEntryText", ca_mapOn));
		}
	}

	// CA_CacheMarks(0);
	if (doCache && !silent)
		CK_BeginFadeDrawing();
}

// Cache Box Routines
// These are accessed as callbacks by the caching manager

static int ck_cacheCountdownNum, ck_cacheBoxChunksPerPic, ck_cacheBoxChunkCounter;

void CK_BeginCacheBox(const char *title, int numChunks)
{
	uint16_t w, h;

	// Vanilla keen checks if > 2kb free here
	// If not, it doesn't cache all of the keen counting down graphics
	// But not necessary for omnispeak
#if 0
	if (MM_TotalFree()) > 2048)
	{
		ck_cacheCountdownNum = 0;
	}
	else
	{
		ck_cacheCountdownNum = 5;
	}
#endif

	ck_cacheCountdownNum = 0;

	// Cache the Keen countdown graphics
	for (int i = 0; i < 6; i++)
	{
		// TODO: Episode independence
		CA_CacheGrChunk(PIC_COUNTDOWN5 + i);
		ca_graphChunkNeeded[PIC_COUNTDOWN5 + i] &= ~ca_levelbit;

		// If a pic can't be cached, forget updating the hand pics
		// by setting the countdown counter at 5
		if (!ca_graphChunks[PIC_COUNTDOWN5 + i])
		{
			// mmerror = 0;
			ck_cacheCountdownNum = 5;
			break;
		}

		MM_SetPurge(ca_graphChunks + PIC_COUNTDOWN5 + i, 3);
	}

	US_CenterWindow(26, 8);

	if (ca_graphChunks[PIC_COUNTDOWN5])
		VHB_DrawBitmap(US_GetWindowX(), US_GetWindowY(), PIC_COUNTDOWN5);
	else
		ck_cacheCountdownNum = 5;

	ca_graphChunkNeeded[PIC_COUNTDOWN5] &= ~ca_levelbit;
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	// Omnispeak FIXME: Start printX at the right spot
	// The following line is not present in Keen 5
	US_SetPrintX(US_GetWindowX());
	CK_MeasureMultiline(title, &w, &h);
	US_SetPrintY(US_GetPrintY() + (US_GetWindowH() - h) / 2 - 4);
	US_CPrint(title);
	VL_Present();

	ck_cacheBoxChunkCounter = ck_cacheBoxChunksPerPic = numChunks / 6;

	if (!ck_cacheBoxChunksPerPic && !ck_cacheCountdownNum)
	{
		ck_cacheCountdownNum = 5;
		if (ca_graphChunks[PIC_COUNTDOWN0])
		{

			VHB_DrawBitmap(US_GetWindowX() - 24, US_GetWindowY() + 40, PIC_COUNTDOWN0);
		}

		VL_Present();
	}
}

void CK_UpdateCacheBox()
{
	ck_cacheBoxChunkCounter--;

	if (ck_cacheBoxChunkCounter == 0 && ck_cacheCountdownNum <= 4)
	{

		ck_cacheBoxChunkCounter = ck_cacheBoxChunksPerPic;
		if (ca_graphChunks[PIC_COUNTDOWN4 + ck_cacheCountdownNum])
			VHB_DrawBitmap(US_GetWindowX() - 24, US_GetWindowY() + 40, PIC_COUNTDOWN4 + ck_cacheCountdownNum);
		VL_Present();
		// Because loading is VERY fast on omnispeak, add artificial delay
		int loadingDelay = CFG_GetConfigInt("loadingDelay", 10);
		if (loadingDelay)
			VL_DelayTics(loadingDelay);
		ck_cacheCountdownNum++;
	}
}

void CK_FinishCacheBox()
{
}

bool CK_TryAgainMenu()
{
	uint16_t w, h;
	int y1, y2, sel;
	int y, clr;

	char buf[80];

	/* Copy and measure the level name */
	strcpy(buf, CK_VAR_GetStringByNameAndIndex("ck_str_levelName", ca_mapOn));
	CK_MeasureMultiline(buf, &w, &h);

	/* Take away all gems */
	memset(ck_gameState.keyGems, 0, sizeof(ck_gameState.keyGems));

	/* If lives remain, see if they want to try this level again */
	if (--ck_gameState.numLives >= 0)
	{
		//VW_SyncPages();
		US_CenterWindow(20, 8);
		US_SetPrintY(US_GetPrintY() + 3);
		US_CPrint(CK_VAR_GetStr("ck_str_tryAgainIntro"));
		y1 = US_GetPrintY() + 22;

		/* Center the level name vertically */
		if (h < 15)
			US_SetPrintY(US_GetPrintY() + 4);
		US_CPrint(buf);

		US_SetPrintY(y1 + 2);
		US_CPrint(CK_VAR_GetStr("ck_str_tryAgainTryAgain"));
		US_SetPrintY(US_GetPrintY() + 4);
		y2 = US_GetPrintY() - 2;
		US_CPrint(CK_VAR_GetStr("ck_str_exitToMap"));

		IN_ClearKeysDown();
		sel = 0;
		while (1)
		{
			IN_PumpEvents();

			/* Decide which selection to draw */
			if (sel != 0)
				y = y2;
			else
				y = y1;

			/* Choose a color to draw it in */
			if ((SD_GetTimeCount() >> 4) & 1)
				clr = 12;
			else
				clr = 1;

			/* And draw the selection box */
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y, clr);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 1, clr);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 12, clr);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 13, clr);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + 4, clr);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + 5, clr);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 4, clr);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 5, clr);
			VL_Present();

			/* Erase the box for next time */
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y, 15);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 1, 15);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 12, 15);
			VHB_HLine(US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 13, 15);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + 4, 15);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + 5, 15);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 4, 15);
			VHB_VLine(y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 5, 15);

			/* If they press Esc, they want to go back to the Map */
			if (IN_GetLastScan() == IN_SC_Escape)
			{
				ck_gameState.currentLevel = 0;
				IN_ClearKeysDown();
				return false;
			}

			IN_ReadControls(0, &ck_inputFrame);
			if (ck_inputFrame.jump || ck_inputFrame.pogo || IN_GetLastScan() == IN_SC_Enter || IN_GetLastScan() == IN_SC_Space)
			{
				/* If they want to go back to the Map, set the current level to zero */
				if (sel != 0)
					ck_gameState.currentLevel = 0;
				return false;
			}

			if (ck_inputFrame.yDirection == -1 || IN_GetLastScan() == IN_SC_UpArrow)
			{
				sel = 0;
			}
			else if (ck_inputFrame.yDirection == 1 || IN_GetLastScan() == IN_SC_DownArrow)
			{
				sel = 1;
			}

#ifdef QUICKSAVE_ENABLED
			if (IN_GetLastScan() == in_kbdControls.quickLoad)
			{
				// quickload, but fall back to map screen on failure
				ck_gameState.currentLevel = 0;
				return US_QuickLoad();
			}
#endif
		} /* while */
	}
	return false;
}

extern CK_Difficulty ck_startingDifficulty;

bool ck_hasCreatureQuestion = false;
bool CK6_CreatureQuestion();
void CK_GameLoop()
{
	if (ck_currentEpisode->hasCreatureQuestion)
	{
		// if !demoParm
		if (!CK6_CreatureQuestion())
		{
			ck_startingSavedGame = false;
			ck_startingDifficulty = D_NotPlaying;
			return;
		}
	}

	do
	{
		if (ck_gameState.levelState != 6)
		{
		resetDifficultyAndLevel:
			ck_gameState.difficulty = ck_startingDifficulty;
			ck_startingDifficulty = D_NotPlaying;
		loadLevel:
			CK_LoadLevel(true, false);

			//TODO: If this didn't succeed, return to level 0.
		}

	replayLevel:
		ck_scrollDisabled = false;
		SD_WaitSoundDone();
		CK_PlayLoop();

		if (ck_gameState.levelState != 6)
		{
			memset(ck_gameState.keyGems, 0, sizeof(ck_gameState.keyGems));
			// TODO: This is probably (but not necessarily) Keen 5 specific
			if (ck_currentEpisode->ep == EP_CK5)
				ck_gameState.ep.ck5.securityCard = 0;
		}

		//TODO: Some TED launching stuff

		ck_lastLevelFinished = -1;
		switch (ck_gameState.levelState)
		{
		case LS_Died: //1
			if (CK_TryAgainMenu())
				goto replayLevel;
			//ck_gameState.currentLevel = ck_nextMapNumber;
			break;

		case LS_Foot:
			if (ck_currentEpisode->ep == EP_CK6)
			{
				IN_ClearKeysDown();
				break;
			}
		case LS_LevelComplete: // 2:
		levelcomplete:
		case 13:
			if (ca_mapOn == 0)
			{
				// US_CenterWindow(8, 0x1A);
				// window_print_y += 0x19;
				// window_print("One Moment");

				// This is an omnispeak hack
				// because we can't change ck_gameState.currentLevel
				// from within CK_ScanForLevelEntry
				// UPDATE (Mar 6 2014): Not the case anymore
				//ck_gameState.currentLevel = ck_nextMapNumber;
			}
			else
			{
				//We've won, return to main map.
				//TODO: Mark level as done (and more)
				SD_PlaySound(SOUND_LEVELEXIT);
				ck_gameState.currentLevel = 0;
				ck_lastLevelFinished = ca_mapOn;
				ck_gameState.levelsDone[ca_mapOn] = 1;
				// TODO: If Keen launched with /Demo switch
				// Then function returns here based on ca_mapOn
			}
			break;

		case 5:
			goto resetDifficultyAndLevel;

		case 6:
			goto replayLevel;

		case 8:
			// Quit to Dos
			IN_ClearKeysDown();
			return;

		// Episode Specific Level Endings
		case LS_CouncilRescued: // 3
			if (ck_currentEpisode->ep == EP_CK4)
			{
				if (ca_mapOn)
					SD_PlaySound(SOUND_LEVELEXIT);

				ck_lastLevelFinished = ca_mapOn;
				ck_gameState.levelsDone[ca_mapOn] = 1;
				CK4_ShowCouncilMessage();

				if (ck_gameState.ep.ck4.membersRescued == 8)
				{
					// Game won
					CK_EndingPurge();
					// RF_Reset();
					// VW_SyncPages();
					help_endgame();
					CK_SubmitHighScore(ck_gameState.keenScore, ck_gameState.ep.ck4.membersRescued);
					return;
				}
				else
				{
					// Back to map
					ck_gameState.currentLevel = 0;
				}
			}
			break;

		case LS_Sandwich:
			CK6_ShowGetSandwich();
			goto levelcomplete;
		case LS_Rope:
			CK6_ShowGetRope();
			goto levelcomplete;
		case LS_Passcard:
			CK6_ShowGetPasscard();
			goto levelcomplete;
		case LS_Molly:
			// Game won
			CK_EndingPurge();
			// RF_Reset();
			// VW_SyncPages();
			help_endgame();
			goto highscores;

		case 14:
			if (ck_currentEpisode->ep == EP_CK5)
			{
				// The level has been ended by fuse destruction
				SD_PlaySound(SOUND_LEVELEXIT);
				ck_lastLevelFinished = ca_mapOn;
				ck_gameState.levelsDone[ca_mapOn] = 14;
				CK5_FuseMessage();
				ck_gameState.currentLevel = 0;
			}
			break;

		case LS_DestroyedQED: //15:
			// The QED was destroyed
			/*
			 * purge_chunks()
			 * RF_Reset();
			 * VW_SyncPages();
			 */
			help_endgame();
#if 0
			CK_SubmitHighScore(ck_gameState.keenScore, 0);
#endif
			return;
#if 0
			// Warping level
			// This code is added for omnispeak so that the warp functionality works
			// Case 4 normally switches to default
			// UPDATE (Mar 6 2014): Not needed anymore.
		case 4:
			ck_gameState.currentLevel = ck_nextMapNumber;
			break;
#endif
		}

		if (ck_gameState.numLives < 0)
			break;

		goto loadLevel; //livesLeft >= 0
	} while (true);

	// Keen 5: Blow up the galaxy
	if (ck_currentEpisode->ep == EP_CK5)
	{
		CK5_ExplodeGalaxy();
	}
	else
	{
		CK_GameOver();
	}

	//TODO: Update High Scores
highscores:
	if (ck_currentEpisode->ep == EP_CK4)
	{
		CK_SubmitHighScore(ck_gameState.keenScore, ck_gameState.ep.ck4.membersRescued);
	}
	else if (ck_currentEpisode->ep == EP_CK5)
	{
		CK_SubmitHighScore(ck_gameState.keenScore, 0);
	}
	else if (ck_currentEpisode->ep == EP_CK6)
	{
		int complete = 0;
		for (int i = 0; i < 25; i++)
		{
			if (ck_gameState.levelsDone[i])
				complete++;
		}
		CK_SubmitHighScore(ck_gameState.keenScore, complete);
	}
}
