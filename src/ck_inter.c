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

#include "ck_def.h"
#include "ck_game.h"
#include "ck_play.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"
#include "id_in.h"

/*
 * CK_INTER: Holds an assortment of screen drawing and state switching routines
 */


int ck_startingSavedGame = 0;
bool ck_inHighScores = false;
CK_Difficulty ck_startingDifficulty = D_NotPlaying;

void CK_HandleDemoKeys()
{
	if (!IN_GetLastScan())
		return;

	/*
	 * if Keen started with demo parameter
	 * ck_gameState.levelState = 5;
	 * startingDifficulty = 2;
	 * IN_ClearKeysDown();
	 * CK_NewGame(); (init_keen_stats)
	 * return;
	 */

	if (IN_GetLastScan() == IN_SC_F1)
	{
		// DoHelp();
		return;
	}

	// Otherwise, start the wristwatch menu
	US_RunCards();
	if (ck_startingDifficulty)
	{
		ck_gameState.levelState = 5;
		return;
	}

	if (!ck_startingSavedGame)
		return;

	ck_gameState.levelState = 6;
}

/*
 * Terminator Intro Text
 */

void CK_DrawTerminator(void)
{
	// TODO: Implement all terminator functions
	// In the meantime, there's this placeholder

#if 1
	{
		VL_ClearScreen(0);
		VL_SetScrollCoords(0,0);
		//CA_CacheGrChunk(3);
		int firsttime = SD_GetLastTimeCount();
	do
	{
		char buf[80];

		sprintf(buf, "Terminator Graphic: %d", (SD_GetTimeCount()-firsttime)/70);

		VH_DrawPropString(buf, 30, 10, 0, 15);
		VH_DrawPropString("Ends at 5", 30, 20, 0, 15);

		IN_PumpEvents();
		VL_DelayTics(35);
		VL_Present();

		if ((SD_GetTimeCount()-firsttime)/70 > 5)
			break;
		

	} while (!IN_GetLastScan());
	}
#endif

	int terminator_complete = 1;
	// After the terminator text has run, keys are checked

#if 0
	// Leave this out for now so we go straight to the menu
	if (!IN_GetLastScan())
		return;
#endif

	if (IN_GetLastScan() == IN_SC_F1)
	{
		// DoHelpScreen
		return;
	}

	if (!terminator_complete)
	{
		// TODO: implement

	}

	US_RunCards();
	if (ck_startingDifficulty)
	{
		ck_gameState.levelState = 5;
		return;
	}

	if (ck_startingSavedGame)
		ck_gameState.levelState = 6;
	
}

/*
 * Star Wars Story Text 
 */

void CK_DrawStarWars()
{
	// TODO: Implement
	// In the meantime, there's this placeholder

#if 1
	{
		VL_ClearScreen(0);
		VL_SetScrollCoords(0,0);
		//CA_CacheGrChunk(3);
		int firsttime = SD_GetLastTimeCount();
	do
	{
		char buf[80];

		sprintf(buf, "Star Wars: %d", (SD_GetTimeCount()-firsttime)/70);

		VH_DrawPropString(buf, 30, 10, 0, 15);
		VH_DrawPropString("Ends at 5", 30, 20, 0, 15);

		IN_PumpEvents();
		VL_DelayTics(35);
		VL_Present();

		if ((SD_GetTimeCount()-firsttime)/70 > 5)
			break;

	} while (!IN_GetLastScan());
	}
#endif
}

/*
 * FizzleFade Title Screen
 */
void CK_ShowTitleScreen()
{
	// TODO: Make this fizzle in 
	CA_CacheGrChunk(88);
	VH_DrawBitmap(0,0,88);
	VL_Present();
	IN_WaitKey();
}

//TODO: Add some demo number stuff

void CK_PlayDemoFile(const char *demoName)
{
	uint8_t *demoBuf;
	int demoFileLength;

	CK_NewGame();

	CA_LoadFile(demoName, (void **)&demoBuf, &demoFileLength);

	uint16_t demoMap = *demoBuf;
	demoBuf += 2;
	uint16_t demoLen = *((uint16_t *) demoBuf);

	ck_currentMapNumber =demoMap;
	ck_gameState.difficulty = D_Normal;

	CK_LoadLevel(true);

	ck_demoEnabled = true;


	IN_DemoStartPlaying(demoBuf, demoLen);


	CK_PlayLoop();

	MM_FreePtr((void **)&demoBuf);
}

void CK_PlayDemo(int demoNumber)
{
	uint8_t *demoBuf;

	int demoChunk = 4926 + demoNumber;

	CK_NewGame();

	CA_CacheGrChunk(demoChunk);
	demoBuf = (uint8_t *)(ca_graphChunks[demoChunk]);
	MM_SetLock(&ca_graphChunks[demoChunk], true);

	uint16_t demoMap = *demoBuf;
	demoBuf += 2;
	uint16_t demoLen = *((uint16_t *) demoBuf);
	demoBuf += 2;

	ck_currentMapNumber =demoMap;

	ck_demoEnabled = true;
	IN_DemoStartPlaying(demoBuf, demoLen);

	CK_LoadLevel(true);

	

	if (ck_inHighScores)
		CK_OverlayHighScores();

	CK_PlayLoop();

	// What should we do after playing the demo?
	CK_HandleDemoKeys();

	// We have to get rid of the demo buffer, as if we want to play it
	// again, we need a fresh copy. ID_IN modifies the buffer.
	MM_FreePtr(&ca_graphChunks[demoChunk]);

	CA_ClearMarks();
}

/*
 * High scores
 */

// Draw the high scores overtop the level
void CK_OverlayHighScores()
{

}

// Enter name if a high score has been achieved
void CK_SubmitHighScore(int score)
{
	// TODO: 

}

// Play the high score level
void CK_DoHighScores() 
{
	ck_inHighScores = true;
	IN_ClearKeysDown();
	CK_PlayDemo(4);
	ck_inHighScores = false;
}
