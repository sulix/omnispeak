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

/*
 * The 'episode' we're playing.
 */
CK_Episode *ck_currentEpisode;


/*
 * Shutdown all of the 'ID Engine' components
 */
void CK_ShutdownID()
{
	//TODO: Some managers don't have shutdown implemented yet
	//US
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

	US_Startup();
	//TODO: Get filenames/etc from config/episode

	// Load the core datafiles
	CA_Startup();

	// Compile the actions
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK_OBJ_SetupFunctions();
	ck_currentEpisode->setupFunctions();
	CK_ACT_LoadActions(CAL_AdjustExtension("ACTION.EXT"));

	// Setup the screen
	VL_InitScreen();
	VL_SetDefaultPalette();

	// Setup input
	IN_Startup();

	// Setup audio
	SD_Startup();

	//TODO: Read game config
	
	// Wolf loads fonts here, but we do it in CA_Startup()?
	
	RF_Startup();
}

/*
 * The Demo Loop
 * Keen (and indeed Wolf3D) have this function as the core of the game.
 * It is, in essence, a loop which runs the title/demos, and calls into the
 * main menu and game loops when they are required.
 */

static int ck_startingLevel = 0;
void CK_DemoLoop()
{
	/* FIXME: Should be called from load_config with the correct settings */
	SD_Default(true, sdm_AdLib, smm_AdLib);
	/*
	 * Commander Keen could be 'launched' from the map editor TED to test a map.
	 * This was implemented by having TED launch keen with the /TEDLEVEL xx
	 * parameter, where xx is the level number.
	 */

	if (us_tedLevel)
	{
		us_noWait = true;
		ck_currentMapNumber = us_tedLevelNumber;

		CA_LoadAllSounds();

		CK_GameLoop();
		Quit(0);
	}

	/*
	 * Handle "easy" "normal" and "hard" parameters here
	 */

	// Given we're not coming from TED, run through the demos.
	
	int demoNumber = 0;

	while (true)
	{
		// TODO: This should really be called in SetupGameLevel
		CA_LoadAllSounds();

		switch(demoNumber++)
		{
		case 0:		// Terminator scroller and Title Screen
			CK_DrawTerminator();	//TODO: Move this to an episode struct.
#if 1 //DEMO_LOOP_ENABLED
			break;
		case 1:
			CK_PlayDemo(0);
			break;
		case 2:
			// Star Wars story text
			break;
		case 3:
			CK_PlayDemo(1);
			break;
		case 4:
			// High Scores
			CK_PlayDemo(4);
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
		if (ck_gameState.levelState == 5 || ck_gameState.levelState == 6)
		{
tryagain:
			CK_GameLoop();
			// DoHighScores();
			if (ck_gameState.levelState == 5 || ck_gameState.levelState == 6)
				goto tryagain;

			// draw_title();

			// Disasm: Useless comparison of levelstate to 5 or 6 again
			goto tryagain;
		}
	}
	
	Quit("Demo loop exited!?");

}

int main(int argc, const char **argv)
{
	// Send the cmd-line args to the User Manager.
	us_argc = argc;
	us_argv = argv;

	// We want Keen 5 (for now!)
	ck_currentEpisode = &ck5_episode;

	CK_InitGame();

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i],"/DEMOFILE"))
		{
			CK_PlayDemoFile(argv[i+1]);
			Quit(0);
		}
	}	

	// Draw the ANSI "Press Key When Ready Screen" here
	CK_DemoLoop();
	CK_ShutdownID();
}
