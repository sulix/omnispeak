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

#include "id_us.h"
#include "id_mm.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_rf.h"
#include "ck_def.h"
#include "ck_play.h"
#include "ck_game.h"
#include "ck_act.h"
#include "ck5_ep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h> // For main (SDL_main) function prototype
/*
 * The 'episode' we're playing.
 */
CK_Episode *ck_currentEpisode;

/*
 * Measure the containing box size of a string that spans multiple lines
 */
void CK_MeasureMultiline(const char *str, uint16_t *w, uint16_t *h)
{
	char c;
	uint16_t x, y;
	char buf[80];
	char *p;

	*h = *w = (uint16_t) 0;
	p = buf;	/* must be a local buffer */

	while ( (c = *str++) != 0 )
	{
		*p++ = c;

		if ( c == '\n' || *str == 0 )
		{
			VH_MeasurePropString( buf, &x, &y, US_GetPrintFont() );

			*h += y;
			if ( *w < x )
				*w = x;

			p = (char *) buf;
			// Shouldn't buf be cleared so that a newline is not read over by
			// VH_MeasurePropString?
		}
	}
}

/*
 * Shutdown all of the 'ID Engine' components
 */
void CK_ShutdownID(void)
{
	//TODO: Some managers don't have shutdown implemented yet
	US_Shutdown();
	SD_Shutdown();
	//IN
	//RF
	//VH
	//VL
	//CA
	MM_Shutdown();
}

/*
 * Start the game!
 */

void CK_InitGame()
{
	// Can't do much without memory!
	MM_Startup();

	//TODO: Get filenames/etc from config/episode

	// Load the core datafiles
	CA_Startup();

	// Set a few Menu Callbacks
	// TODO: Finish this!
	US_SetMenuFunctionPointers(NULL /*Load*/, NULL /*Save*/, &CK_ExitMenu);
	// Set ID engine Callbacks
	ca_beginCacheBox = CK_BeginCacheBox;
	ca_updateCacheBox = CK_UpdateCacheBox;
	ca_finishCacheBox = CK_FinishCacheBox;

	// Mark some chunks we'll need.
	CA_ClearMarks();
	CA_MarkGrChunk(3);
	CA_MarkGrChunk(0x1C0);
	CA_MarkGrChunk(0x1C1);
	CA_MarkGrChunk(0x64);
	CA_MarkGrChunk(0x65);
  CA_MarkGrChunk(88); // Moved from CA_Startup
	CA_CacheMarks(0);

	// Lock them chunks in memory.
	CA_LockGrChunk(3);
	MM_SetLock(&ca_graphChunks[448], true);
	MM_SetLock(&ca_graphChunks[449], true);
	MM_SetLock(&ca_graphChunks[0x64], true);
	MM_SetLock(&ca_graphChunks[0x65], true);


	// Compile the actions
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK_OBJ_SetupFunctions();
	ck_currentEpisode->setupFunctions();
	CK_ACT_LoadActions("ACTION.EXT");

	// Setup the screen
	VL_InitScreen();
	// TODO: Palette initialization should be done in the terminator code
	VL_SetDefaultPalette();

	// Setup input
	IN_Startup();

	// Setup audio
	SD_Startup();

	US_Startup();

	// Wolf loads fonts here, but we do it in CA_Startup()?

	RF_Startup();

	VL_ColorBorder(3);
	VL_ClearScreen(0);
	VL_Present();
}

/*
 * The Demo Loop
 * Keen (and indeed Wolf3D) have this function as the core of the game.
 * It is, in essence, a loop which runs the title/demos, and calls into the
 * main menu and game loops when they are required.
 */

extern CK_Difficulty ck_startingDifficulty;
static int ck_startingLevel = 0;

void CK_DemoLoop()
{
	/*
	 * Commander Keen could be 'launched' from the map editor TED to test a map.
	 * This was implemented by having TED launch keen with the /TEDLEVEL xx
	 * parameter, where xx is the level number.
	 */

	if (us_tedLevel)
	{
		us_noWait = true; // ?

		CK_NewGame();
		CA_LoadAllSounds();
		ck_gameState.currentLevel = us_tedLevelNumber;
		ck_startingDifficulty = D_Normal;

		// TODO: Support selecting difficulty via an extra command line
		// argument ("easy", "normal", "hard")

		CK_GameLoop();
		Quit(0); // run_ted
	}

	/*
	 * Handle "easy" "normal" and "hard" parameters here
	 */

	// Given we're not coming from TED, run through the demos.

	int demoNumber = 0;
	ck_gameState.levelState = 0;

	while (true)
	{
		switch (demoNumber++)
		{
		case 0:		// Terminator scroller and Title Screen
#if 0
			// If no pixel panning capability
			// Then the terminator screen isn't shown
			if (NoPan)
				CK_ShowTitleScreen();
			else
#endif
				CK_DrawTerminator();	//TODO: Move this to an episode struct.
#if 1 //DEMO_LOOP_ENABLED
			break;
		case 1:
			CK_PlayDemo(0);
			break;
		case 2:
			// Star Wars story text
			CK_DrawStarWars();
			break;
		case 3:
			CK_PlayDemo(1);
			break;
		case 4:
			CK_DoHighScores();// High Scores
			// CK_PlayDemo(4);
			break;
		case 5:
			CK_PlayDemo(2);
			break;
		case 6:
			CK_PlayDemo(3);
#else
			CK_HandleDemoKeys();
#endif
			demoNumber = 0;
			break;
		}

		// Game Loop
		while (1)
		{
			if (ck_gameState.levelState == 5 || ck_gameState.levelState == 6)
			{
				CK_GameLoop();
				CK_DoHighScores();
				if (ck_gameState.levelState == 5 || ck_gameState.levelState == 6)
					continue;

				CK_ShowTitleScreen();

				if (ck_gameState.levelState == 5 || ck_gameState.levelState == 6)
					continue;
			}
			else
			{
				break;
			}
		}
	}

	Quit("Demo loop exited!?");

}

int main(int argc, char *argv[])
{
	// Send the cmd-line args to the User Manager.
	us_argc = argc;
	us_argv = (const char **) argv;

	// We want Keen 5 (for now!)
	ck_currentEpisode = &ck5_episode;

	CK_InitGame();

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "/DEMOFILE"))
		{
			// A bit of stuff from the usual demo loop
			ck_gameState.levelState = 0;

			CK_PlayDemoFile(argv[i + 1]);
			Quit(0);
		}
	}

	// Draw the ANSI "Press Key When Ready Screen" here
	CK_DemoLoop();
	CK_ShutdownID();
}
