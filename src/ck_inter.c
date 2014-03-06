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
#include "id_rf.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"
#include "id_in.h"

#include <string.h>

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

	bool terminator_complete = false;
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

	// Placeholder ends here: this is now real keen code.

	// Somewhere around here, the terminator itself ends and the fizzle fade begins

	// After the terminator text has run, keys are checked
	if (!IN_GetLastScan())
	{
		// One of terminator2 or terminator1 is likely the fizzlefade
		; //terminator2();
	}

	if (!IN_GetLastScan())
	{
		// terminator1();
		terminator_complete = true;
	}

	// Free the COMMANDER and KEEN bitmaps
	// MM_SetPurge(4922, 3);
	// MM_SetPurge(4923, 3);

	// Restore video mode to normal
	VL_ClearScreen(0);
	// VW_SetLineWidth(0x40);
	VL_SetDefaultPalette();
	// RF_Reset();
	CA_ClearMarks();

	// Handle any keypresses
	if (!IN_GetLastScan())
		return;

	// Go to help screen
	if (IN_GetLastScan() == IN_SC_F1)
	{
		HelpScreens();
		return;
	}

	if (!terminator_complete)
	{
		// RF_Reset();

		// Display Title Screen
		CA_CacheGrChunk(88);
		VH_DrawBitmap(0, 0, 88);
		VL_Present();// ORIGINAL: VW_SetScreen(bufferofs, 0);
		IN_WaitButton();
		CA_ClearMarks();

		// TODO: If started with /DEMO PARM
#if 0
		if (DemoSwitch)
		{
			ck_gameState.levelState = 5;
			ck_startingDifficulty = D_Normal;
			IN_ClearKeysDown();
			CK_NewGame();
			return;
		}
#endif

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

void CK_ShowTitleScreen()
{
	// scrollofs = 0;
	CA_CacheGrChunk(88);
	VH_DrawBitmap(0,0,88);
	// Draw to offscreen buffer and copy?
	// VW_SetScreen(0,bufferofs_0);
	// VWL_ScreenToScreen(bufferofs, bufferofs_0, 42, 224);
	VL_Present();
	IN_UserInput(420, false);
	CA_ClearMarks();
	CK_HandleDemoKeys();
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
	demoBuf += 2;

	ck_currentMapNumber =demoMap;
	ck_gameState.difficulty = D_Normal;
	ck_demoEnabled = true;

	CK_LoadLevel(true);



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

	

#if 0
	if (ck_inHighScores)
		CK_OverlayHighScores();
#endif

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

typedef struct CK_HighScore
{
	char name[58];
	uint32_t score;
	uint16_t arg4;
} CK_HighScore;

CK_HighScore ck_highScores[8] =
{
	{"Id Software - '91", 10000, 0},
	{"", 10000, 0},
	{"Jason Blochowiak", 10000, 0},
	{"Adrian Carmack", 10000, 0},
	{"John Carmack", 10000, 0},
	{"Tom Hall", 10000, 0},
	{"John Romero", 10000, 0},
	{"", 10000, 0},
};

// Draw the high scores overtop the level
void CK_OverlayHighScores()
{

	RF_Reposition (0,0);
	// var8 = bufferofs;
	// word_49EEE = bufferofs;

	US_SetPrintColour(12);

	for (int entry = 0; entry < 8; entry++)
	{
		// Print the name
		US_SetPrintY(16*entry+0x23);
		US_SetPrintX(0x28);
		US_Print(ck_highScores[entry].name);

		// Print the score, right aligned in the second
		// column of the table
		char buf[0x10];
		sprintf(buf, "%d", ck_highScores[entry].score);

		// Convert it to high score numbers?
		for (char *c = buf; *c; c++)
		{
			*c += 0x51;
		}

		// Align it
		uint16_t w, h;
		VH_MeasurePropString(buf, &w, &h, US_GetPrintFont());
		US_SetPrintX(0x118-w);
		US_Print(buf);
	}

	US_SetPrintColour(15);
	// bufferofs = var8;

}

// Enter name if a high score has been achieved
static bool word_49EE1;
void CK_SubmitHighScore(int score, uint16_t arg_4)
{

	int entry, var6, var4;

	CK_HighScore newHighScore;
	strcpy(newHighScore.name, "");
	newHighScore.score = score;
	newHighScore.arg4 = arg_4;
	


	// Check if this entry made the high scores
	var6 = -1;
	for (entry = 0; entry < 8; entry++)
	{
		if (ck_highScores[entry].score < newHighScore.score)
			continue;

		if (newHighScore.score == ck_highScores[entry].score)
		{
			if (ck_highScores[entry].arg4 >= newHighScore.arg4)
				continue;
		}
	}
		
	// Insert the new high score into the proper slot
	for (var4 = 8; --var4 > entry; )
		memcpy(&ck_highScores[var4], &ck_highScores[var4-1], 64);

	memcpy(&ck_highScores[entry], &newHighScore, 64);
	var6 = entry;
	word_49EE1 = true;

	if (var6 != -1)
	{
		ck_inHighScores = true;
		ck_currentMapNumber = 15;
		CK_LoadLevel(true);
		CK_OverlayHighScores();
		US_SetPrintColour(12);

#if 0
		// FIXME: Calling these causes segfault
		RF_Refresh();
		RF_Refresh();
#endif

		US_SetPrintY(entry*16 + 0x23);
		US_SetPrintX(0x28);
		
		US_LineInput(US_GetPrintX(), US_GetPrintY(), ck_highScores[var6].name, 0, 1, 0x39, 0x70);

		ck_inHighScores = false;
	}

	US_SetPrintColour(15);
}

// Play the high score level
void CK_DoHighScores() 
{
	ck_inHighScores = true;
	IN_ClearKeysDown();
	CK_PlayDemo(4);
	ck_inHighScores = false;
}
