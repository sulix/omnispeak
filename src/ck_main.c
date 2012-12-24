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

/*
 * Shutdown all of the 'ID Engine' components
 */
void CK_ShutdownID()
{
	//TODO: Some managers don't have shutdown implemented yet
	//US
	//SD
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

	// Compile the actions
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK5_SetupFunctions();
	CK_ACT_LoadActions("ACTION.CK5");

	// Setup the screen
	VL_InitScreen();
	VL_SetDefaultPalette();

	// Setup input
	IN_Startup();

	//TODO: Read game config
	
	// Wolf loads fonts here, but we do it in CA_Startup()?
	
	RF_Startup();
}

void CK_ShowTitleScreen()
{
	CA_CacheGrChunk(88);
	VH_DrawBitmap(0,0,88);
	VL_Present();
	IN_WaitKey();
}

void CK_HandleDemoKeys()
{
	// If a button has been pressed
	if (IN_GetLastScan() != IN_SC_None)
	{
		// Before the menu is implemented, make any key start the game.

		ck_gameState.levelState = 5;
		ck_currentMapNumber = 1;

		//TODO: If LastScan == F1, show help

		//TODO: When we have menus, handle them here

		// If we've started a game...
		if (ck_gameState.difficulty != D_NotPlaying)
		{
			ck_gameState.levelState = 5;
		}
	}
}


bool ck_tedlaunched = false;
bool ck_nowait = false;

/*
 * The Demo Loop
 * Keen (and indeed Wolf3D) have this function as the core of the game.
 * It is, in essence, a loop which runs the title/demos, and calls into the
 * main menu and game loops when they are required.
 */

void CK_DemoLoop()
{
	/*
	 * Commander Keen could be 'launched' from the map editor TED to test a map.
	 * This was implemented by having TED launch keen with the /TEDLEVEL xx
	 * parameter, where xx is the level number.
	 */

	//TODO: Work out where these vars come from
	if (ck_tedlaunched)
	{
		ck_nowait = true;
		//ck_currentMapNumber = ck_tedlevel;

		//CK_GameLoop();
		Quit(0);
	}

	// Given we're not coming from TED, run through the demos.
	
	int demoNumber = 0;

	while (true)
	{
		switch(demoNumber++)
		{
		case 0:		// Terminator scroller and Title Screen
			CK_ShowTitleScreen();	//TODO: Move this to an episode struct.
#ifdef DEMO_LOOP_ENABLED
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
			CK_GameLoop();
		}
	}
	
	Quit("Demo loop exited!?");

}

int main()
{
	CK_InitGame();
	CK_DemoLoop();
	CK_ShutdownID();
}
