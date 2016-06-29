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
#include "ck4_ep.h"
#include "ck5_ep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h> // For main (SDL_main) function prototype
/*
 * The 'episode' we're playing.
 */
CK_EpisodeDef *ck_currentEpisode;

int FON_MAINFONT;
int FON_WATCHFONT;

int PIC_HELPMENU;
int PIC_ARROWDIM;
int PIC_ARROWBRIGHT;
int PIC_HELPPOINTER;
int PIC_BORDERTOP;
int PIC_BORDERLEFT;
int PIC_BORDERRIGHT;
int PIC_BORDERBOTTOMSTATUS;
int PIC_BORDERBOTTOM;

int PIC_MENUCARD;
int PIC_NEWGAMECARD;
int PIC_LOADCARD;
int PIC_SAVECARD;
int PIC_CONFIGURECARD;
int PIC_SOUNDCARD;
int PIC_MUSICCARD;
int PIC_KEYBOARDCARD;
int PIC_MOVEMENTCARD;
int PIC_BUTTONSCARD;
int PIC_JOYSTICKCARD;
int PIC_OPTIONSCARD;
int PIC_PADDLEWAR;
int PIC_DEBUGCARD;

int PIC_WRISTWATCH;

int PIC_STARWARS;
int PIC_TITLESCREEN;
int PIC_COUNTDOWN5;
int PIC_COUNTDOWN4;
int PIC_COUNTDOWN0;

int MPIC_WRISTWATCHSCREEN;
int MPIC_STATUSLEFT;
int MPIC_STATUSRIGHT;

int SPR_PADDLE;
int SPR_BALL0;
int SPR_BALL1;
int SPR_BALL2;
int SPR_BALL3;

int SPR_DEMOSIGN;

int SPR_SECURITYCARD_1;
int SPR_GEM_A1;
int SPR_GEM_B1;
int SPR_GEM_C1;
int SPR_GEM_D1;
int SPR_100_PTS1;
int SPR_200_PTS1;
int SPR_500_PTS1;
int SPR_1000_PTS1;
int SPR_2000_PTS1;
int SPR_5000_PTS1;
int SPR_1UP1;
int SPR_STUNNER1;

int SPR_SCOREBOX;

int SPR_MAPKEEN_WALK1_N;
int SPR_MAPKEEN_STAND_N;
int SPR_MAPKEEN_STAND_NE;
int SPR_MAPKEEN_STAND_E;
int SPR_MAPKEEN_STAND_SE;
int SPR_MAPKEEN_WALK1_S;
int SPR_MAPKEEN_STAND_S;
int SPR_MAPKEEN_STAND_SW;
int SPR_MAPKEEN_STAND_W;
int SPR_MAPKEEN_STAND_NW;

int TEXT_HELPMENU;
int TEXT_CONTROLS;
int TEXT_STORY;
int TEXT_ABOUTID;
int TEXT_END;
int TEXT_SECRETEND;
int TEXT_ORDER;

int DEMOSTART;

int SOUND_KEENWALK0;
int SOUND_KEENWALK1;
int SOUND_KEENJUMP;
int SOUND_KEENLAND;
int SOUND_KEENSHOOT;
int SOUND_MINEEXPLODE;
int SOUND_SLICEBUMP;
int SOUND_KEENPOGO;
int SOUND_GOTITEM;
int SOUND_GOTSTUNNER;
int SOUND_GOTCENTILIFE;
int SOUND_UNKNOWN11;
int SOUND_UNKNOWN12;
int SOUND_LEVELEXIT;
int SOUND_NEEDKEYCARD;
int SOUND_KEENHITCEILING;
int SOUND_SPINDREDFLYUP;
int SOUND_GOTEXTRALIFE;
int SOUND_OPENSECURITYDOOR;
int SOUND_GOTGEM;
int SOUND_KEENFALL;
int SOUND_KEENOUTOFAMMO;
int SOUND_UNKNOWN22;
int SOUND_KEENDIE;
int SOUND_UNKNOWN24;
int SOUND_KEENSHOTHIT;
int SOUND_UNKNOWN26;
int SOUND_SPIROSLAM;
int SOUND_SPINDREDSLAM;
int SOUND_ENEMYSHOOT;
int SOUND_ENEMYSHOTHIT;
int SOUND_AMPTONWALK0;
int SOUND_AMPTONWALK1;
int SOUND_AMPTONSTUN;
int SOUND_UNKNOWN34;
int SOUND_UNKNOWN35;
int SOUND_SHELLYEXPLODE;
int SOUND_SPINDREDFLYDOWN;
int SOUND_MASTERSHOT;
int SOUND_MASTERTELE;
int SOUND_POLEZAP;
int SOUND_UNKNOWN41;
int SOUND_SHOCKSUNDBARK;
int SOUND_FLAGFLIP;
int SOUND_FLAGLAND;
int SOUND_BARKSHOTDIE;
int SOUND_KEENPADDLE;
int SOUND_PONGWALL;
int SOUND_COMPPADDLE;
int SOUND_COMPSCORE;
int SOUND_KEENSCORE;
int SOUND_UNKNOWN51;
int SOUND_UNKNOWN52;
int SOUND_GALAXYEXPLODE;
int SOUND_GALAXYEXPLODEPRE;
int SOUND_GOTKEYCARD;
int SOUND_UNKNOWN56;
int SOUND_KEENLANDONFUSE;
int SOUND_SPARKYPREPCHARGE;
int SOUND_SPHEREFULCEILING;
int SOUND_OPENGEMDOOR;
int SOUND_SPIROFLY;
int SOUND_UNKNOWN62;
int SOUND_UNKNOWN63;
int LASTSOUND;

int CAMEIN_MUS;
int LITTLEAMPTON_MUS;
int THEICE_MUS;
int SNOOPIN_MUS;
int BAGPIPES_MUS;
int WEDNESDAY_MUS;
int ROCKNOSTONE_MUS;
int OUTOFBREATH_MUS;
int SHIKADIAIRE_MUS;
int DIAMONDS_MUS;
int TIGHTER_MUS;
int ROBOREDROCK_MUS;
int FANFARE_MUS;
int BRINGEROFWAR_MUS;
int LASTMUSTRACK;

char *STR_EXIT_TO_MAP;

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
	CA_MarkGrChunk(FON_MAINFONT);
	CA_MarkGrChunk(ca_gfxInfoE.offTiles8);
	CA_MarkGrChunk(ca_gfxInfoE.offTiles8m);
	CA_MarkGrChunk(MPIC_STATUSLEFT);
	CA_MarkGrChunk(MPIC_STATUSRIGHT);
  CA_MarkGrChunk(PIC_TITLESCREEN); // Moved from CA_Startup
	CA_CacheMarks(0);

	// Lock them chunks in memory.
	CA_LockGrChunk(FON_MAINFONT);
	MM_SetLock(&ca_graphChunks[ca_gfxInfoE.offTiles8], true);
	MM_SetLock(&ca_graphChunks[ca_gfxInfoE.offTiles8m], true);
	MM_SetLock(&ca_graphChunks[MPIC_STATUSLEFT], true);
	MM_SetLock(&ca_graphChunks[MPIC_STATUSRIGHT], true);


	// Compile the actions
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK_OBJ_SetupFunctions();
  CK_Map_SetupFunctions();
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

  // FIXME: Pick episode 5 if nothing selected
  ck_currentEpisode = &ck5_episode;

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "/EPISODE"))
		{
			// A bit of stuff from the usual demo loop
      if (argc >= i+1)
      {
        if (!strcmp(argv[i+1], "4"))
          ck_currentEpisode = &ck4_episode;
        else if (!strcmp(argv[i+1], "5"))
          ck_currentEpisode = &ck5_episode;
        else
          Quit("Unsupported episode!");
      }
		}
	}

  ck_currentEpisode->defineConstants();

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
