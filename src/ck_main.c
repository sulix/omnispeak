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

#include "id_ca.h"
#include "id_fs.h"
#include "id_in.h"
#include "id_mm.h"
#include "id_rf.h"
#include "id_us.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_play.h"
#ifdef WITH_KEEN4
#include "ck4_ep.h"
#endif
#ifdef WITH_KEEN5
#include "ck5_ep.h"
#endif
#ifdef WITH_KEEN6
#include "ck6_ep.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The 'episode' we're playing.
 */
CK_EpisodeDef *ck_currentEpisode;

/*
 * Measure the containing box size of a string that spans multiple lines
 */
void CK_MeasureMultiline(const char *str, uint16_t *w, uint16_t *h)
{
	char c;
	uint16_t x, y;
	char buf[80] = {0};
	char *p;

	*h = *w = (uint16_t)0;
	p = buf; /* must be a local buffer */

	while ((c = *str++) != 0)
	{
		*p++ = c;

		if (c == '\n' || *str == 0)
		{
			VH_MeasurePropString(buf, &x, &y, US_GetPrintFont());

			*h += y;
			if (*w < x)
				*w = x;

			p = (char *)buf;
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
	VL_DestroySurface(ck_backupSurface);
	VL_DestroySurface(ck_statusSurface);
	US_Shutdown();
	SD_Shutdown();
	//IN
	RF_Shutdown();
	//VH
	VL_Shutdown();
	CA_Shutdown();

	CFG_Shutdown();
	MM_Shutdown();

#ifdef WITH_SDL
	SDL_Quit();
#endif
}

/*
 * Start the game!
 */

void CK_InitGame()
{
	// On Windows, we want to be DPI-aware
#if defined(WITH_SDL) && defined(_WIN32)
#if SDL_VERSION_ATLEAST(2,24,0)
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
#endif

	// Compile the actions
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK_OBJ_SetupFunctions();
	CK_Map_SetupFunctions();
	CK_Misc_SetupFunctions();
	ck_currentEpisode->setupFunctions();


	CK_VAR_Startup();
	CK_VAR_LoadVars("EPISODE.EXT");

	// Load the core datafiles
	CA_Startup();
	CA_InitLumps();
	// Setup saved games handling
	US_Setup();

	// Set a few Menu Callbacks
	// TODO: Finish this!
	US_SetMenuFunctionPointers(&CK_LoadGame, &CK_SaveGame, &CK_ExitMenu);
	// Set ID engine Callbacks
	ca_beginCacheBox = CK_BeginCacheBox;
	ca_updateCacheBox = CK_UpdateCacheBox;
	ca_finishCacheBox = CK_FinishCacheBox;

	// Mark some chunks we'll need.
	CA_ClearMarks();
	CA_MarkGrChunk(CK_CHUNKNUM(FON_MAINFONT));
	CA_MarkGrChunk(ca_gfxInfoE.offTiles8);
	CA_MarkGrChunk(ca_gfxInfoE.offTiles8m);
	CA_MarkGrChunk(CK_CHUNKNUM(MPIC_STATUSLEFT));
	CA_MarkGrChunk(CK_CHUNKNUM(MPIC_STATUSRIGHT));
	CA_MarkGrChunk(CK_CHUNKNUM(PIC_TITLESCREEN)); // Moved from CA_Startup
	CA_CacheMarks(0);

	// Lock them chunks in memory.
	CA_LockGrChunk(CK_CHUNKNUM(FON_MAINFONT));
	CA_LockGrChunk(ca_gfxInfoE.offTiles8);
	CA_LockGrChunk(ca_gfxInfoE.offTiles8m);
	CA_LockGrChunk(CK_CHUNKNUM(MPIC_STATUSLEFT));
	CA_LockGrChunk(CK_CHUNKNUM(MPIC_STATUSRIGHT));

	// Setup the screen
	VL_Startup();
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

	// Create a surface for the dropdown menu
	ck_statusSurface = VL_CreateSurface(RF_BUFFER_WIDTH_PIXELS, STATUS_H + 16 + 16);
	ck_backupSurface = VL_CreateSurface(RF_BUFFER_WIDTH_PIXELS, RF_BUFFER_HEIGHT_PIXELS);
}

/*
 * The Demo Loop
 * Keen (and indeed Wolf3D) have this function as the core of the game.
 * It is, in essence, a loop which runs the title/demos, and calls into the
 * main menu and game loops when they are required.
 */

extern CK_Difficulty ck_startingDifficulty;

void CK_DemoLoop()
{
	/*
	 * Commander Keen could be 'launched' from the map editor TED to test a map.
	 * This was implemented by having TED launch keen with the /TEDLEVEL xx
	 * parameter, where xx is the level number.
	 */

	if (us_tedLevel)
	{
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
	ck_gameState.levelState = LS_Playing;

	while (true)
	{
		switch (demoNumber++)
		{
		case 0: // Terminator scroller and Title Screen
			// If no pixel panning capability
			// Then the terminator screen isn't shown
			if (vl_noPan)
				CK_ShowTitleScreen();
			else
				CK_DrawTerminator();
#if 1					     //DEMO_LOOP_ENABLED
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
			CK_DoHighScores(); // High Scores
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
			if (ck_gameState.levelState == LS_ResetGame || ck_gameState.levelState == LS_LoadedGame)
			{
				CK_GameLoop();
				CK_DoHighScores();
				if (ck_gameState.levelState == LS_ResetGame || ck_gameState.levelState == LS_LoadedGame)
					continue;

				CK_ShowTitleScreen();

				if (ck_gameState.levelState == LS_ResetGame || ck_gameState.levelState == LS_LoadedGame)
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

/* Basically a set of hacks: By commenting out the relevant "define" line,
 * this code piece can be used to build a validator that checks that the
 * contents of an ACTION.EXT file match the ones from an original
 * (but unpacked) DOS executable, to find any missed difference.
 * Note that the validation of function pointers isn't precise, but it
 * is checked that different occurrences of the same function pointer in
 * a DOS EXE always map to the exact same function in the ACTION.EXT file.
 */

//#define CK_RUN_ACTION_VALIDATOR

#ifdef CK_RUN_ACTION_VALIDATOR

#ifndef CK_CROSS_IS_LITTLEENDIAN
#error "Error - Action validator is compatible with little-endian only!"
#endif

#include <stdio.h>
#include "id_str.h"

#define MAX_NUM_OF_FUNCTIONS 256


typedef enum CK_VAR_VarType
{
	VAR_Invalid,
	VAR_EOF,
	VAR_Bool,
	VAR_Int,
	VAR_String,
	VAR_IntArray,
	VAR_StringArray,
	VAR_Action,
	VAR_TOK_Include
} CK_VAR_VarType;

#ifdef CK_VAR_TYPECHECK
typedef struct CK_VAR_Variable
{
	CK_VAR_VarType type;
	void *value;
	size_t arrayLength;
} CK_VAR_Variable;
#else
#error Action validator requires typechecking to be enabled.
#endif

extern STR_Table *ck_varTable; // HACK

typedef struct
{
	uint32_t farPtr;
	void *nativePtr;
} CK_FuncPtrPair;

// Using an array for the sake of simplicity
static CK_FuncPtrPair ck_funcPtrPairsArray[MAX_NUM_OF_FUNCTIONS];
static int g_numOfFunctPtrPairs = 0;

static bool compareFunctionDOSPtrToNativePtr(uint32_t farPtr, void *nativePtr)
{
	if ((farPtr == 0) || (nativePtr == NULL))
		return ((farPtr == 0) && (nativePtr == NULL));

	int i;
	CK_FuncPtrPair *funcPtrPair = ck_funcPtrPairsArray;
	for (i = 0; i < g_numOfFunctPtrPairs; ++i, ++funcPtrPair)
		if (funcPtrPair->farPtr == farPtr)
			return (funcPtrPair->nativePtr == nativePtr);

	// First time we encounter this function, so add a mapping if there's the room
	if (i == MAX_NUM_OF_FUNCTIONS)
	{
		fprintf(stderr, "Function pointers pairs array is full: DOS pointer is %u\n", (unsigned int)farPtr);
		exit(1);
	}
	funcPtrPair->farPtr = farPtr;
	funcPtrPair->nativePtr = nativePtr;
	++g_numOfFunctPtrPairs;
	return true;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("Action validator - Usage:\n");
		printf("%s <UNPACKED.EXE> <ACTION.EXT> <EPISODENUM>\n", argv[0]);
		return 0;
	}

	switch (atoi(argv[3]))
	{
	case 4:
		ck_currentEpisode = &ck4_episode;
		break;
	case 5:
		ck_currentEpisode = &ck5_episode;
		break;
	case 6:
		ck_currentEpisode = &ck6v14e_episode;
		break;
	default:
		fprintf(stderr, "Invalid episode selected - only 4 or 5 is valid!\n");
		return 1;
	}

	FILE *exeFp = fopen(argv[1], "rb");
	if (exeFp == NULL)
	{
		fprintf(stderr, "Couldn't open DOS EXE file! %s:\n", argv[1]);
		return 1;
	}
	fseek(exeFp, 0L, SEEK_END);
	long int fSize = ftell(exeFp);
	fseek(exeFp, 0L, SEEK_SET);

	char *fileBuffer = (char *)malloc(fSize);
	if (!fileBuffer)
	{
		fprintf(stderr, "Couldn't allocate memory for DOS EXE!\n");
		fclose(exeFp);
		return 1;
	}

	fread(fileBuffer, fSize, 1, exeFp);
	fclose(exeFp);

	// We need this here
	MM_Startup();
	FS_Startup();

	// Compile the actions
	CK_VAR_Startup();
	CK_ACT_SetupFunctions();
	CK_KeenSetupFunctions();
	CK_OBJ_SetupFunctions();
	CK_Map_SetupFunctions();
	CK_Misc_SetupFunctions();
	ck_currentEpisode->setupFunctions();
	CK_VAR_LoadVars(argv[2]);

	char *exeImage = fileBuffer + 16 * (*(uint16_t *)(fileBuffer + 8));
	char *dsegBuffer = exeImage + 16 * (*(uint16_t *)(exeImage + 1)); // HUGE HACK for fetching dseg

	// HACK
	extern STR_Table *ck_varTable;
	for (int i = 0, count = 0; i < ck_varTable->size; ++i)
	{
		if (ck_varTable->arr[i].str == NULL)
			continue;

		const char *name = ck_varTable->arr[i].str;
		CK_VAR_Variable *var = (CK_VAR_Variable *)(ck_varTable->arr[i].ptr);
		if (var->type != VAR_Action)
			continue;

		CK_action *act = (CK_action *)(var->value);

		char *dataToCompare = &dsegBuffer[act->compatDosPointer];
		if (*(int16_t *)dataToCompare != act->chunkLeft)
			printf("chunkLeft mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 2) != act->chunkRight)
			printf("chunkRight mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 4) != act->type)
			printf("type mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 6) != act->protectAnimation)
			printf("protectAnimation mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 8) != act->stickToGround)
			printf("stickToGround mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 10) != act->timer)
			printf("timer mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 12) != act->velX)
			printf("velX mismatch found for action no. %d: %s\n", count, name);
		if (*(int16_t *)(dataToCompare + 14) != act->velY)
			printf("velY mismatch found for action no. %d: %s\n", count, name);
		if (!compareFunctionDOSPtrToNativePtr(*(uint32_t *)(dataToCompare + 16), (void *)(act->think)))
			printf("think mismatch found (possibly) for action no. %d: %s\n", count, name);
		if (!compareFunctionDOSPtrToNativePtr(*(uint32_t *)(dataToCompare + 20), (void *)(act->collide)))
			printf("collide mismatch found (possibly) for action no. %d: %s\n", count, name);
		if (!compareFunctionDOSPtrToNativePtr(*(uint32_t *)(dataToCompare + 24), (void *)(act->draw)))
			printf("draw mismatch found (possibly) for action no. %d: %s\n", count, name);
		if (((act->next == NULL) && (*(uint16_t *)(dataToCompare + 28) != 0)) ||
			((act->next != NULL) && (*(uint16_t *)(dataToCompare + 28) != act->next->compatDosPointer)))
			printf("next mismatch found for action no. %d: %s\n", count, name);

		++count;
	}
	return 0;
}

#else // !CK_RUN_ACTION_VALIDATOR

CK_EpisodeDef *ck_episodes[] = {
#ifdef WITH_KEEN4
	&ck4_episode,
#endif
#ifdef WITH_KEEN5
	&ck5_episode,
#endif
#ifdef WITH_KEEN6
	&ck6_episode,
#endif
	0
};

int main(int argc, char *argv[])
{
	// Send the cmd-line args to the User Manager.
	us_argc = argc;
	us_argv = (const char **)argv;

	// We need to start the filesystem code before we look
	// for any files.
	FS_Startup();

	// Can't do much without memory!
	MM_Startup();

	// Load the config file. We do this before parsing command-line args.
	CFG_Startup();

	ck_cross_logLevel = (CK_Log_Message_Class_T)CFG_GetConfigEnum("logLevel", ck_cross_logLevel_strings, CK_DEFAULT_LOG_LEVEL);

	// Default to the first episode with all files present.
	// If no episodes are found, we default to the first DEMO_LOOP_ENABLED
	// epside (usually Keen 4) in order to show the file not found messages.
	ck_currentEpisode = ck_episodes[0];
	for (int i = 0; ck_episodes[i]; ++i)
	{
		if (ck_episodes[i]->isPresent())
		{
			ck_currentEpisode = ck_episodes[i];
			break;
		}
	}

	bool isFullScreen = CFG_GetConfigBool("fullscreen", false);
	bool isAspectCorrected = CFG_GetConfigBool("aspect", true);
	bool hasBorder = CFG_GetConfigBool("border", true);
	bool isIntegerScaled = CFG_GetConfigBool("integer", false);
	bool overrideCopyProtection = CFG_GetConfigBool("ck6_noCreatureQuestion", false);
	int swapInterval = CFG_GetConfigInt("swapInterval", 1);
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
	const char *dumperFilename = NULL;
#endif

	for (int i = 1; i < argc; ++i)
	{
		if (!CK_Cross_strcasecmp(argv[i], "/EPISODE"))
		{
			// A bit of stuff from the usual demo loop
			if (argc >= i + 1)
			{
#ifdef WITH_KEEN4
				if (!strcmp(argv[i + 1], "4"))
					ck_currentEpisode = &ck4_episode;
				else
#endif
#ifdef WITH_KEEN5
				if (!strcmp(argv[i + 1], "5"))
					ck_currentEpisode = &ck5_episode;
				else
#endif
#ifdef WITH_KEEN6
				if (!strcmp(argv[i + 1], "6"))
					ck_currentEpisode = &ck6_episode;
				// For compatibility, we accept version-specific arguments for 6,
				// as this used to matter. Now, as long as the data file are
				// correct, either work.
				else if (!strcmp(argv[i + 1], "6v14"))
					ck_currentEpisode = &ck6_episode;
				else if (!strcmp(argv[i + 1], "6v15"))
					ck_currentEpisode = &ck6_episode;
				else
#endif
					Quit("Unsupported episode!");
			}
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/FULLSCREEN"))
		{
			isFullScreen = true;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/FILLED"))
		{
			isAspectCorrected = false;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/NOBORDER"))
		{
			hasBorder = false;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/INTEGER"))
		{
			isIntegerScaled = true;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/VSYNC"))
		{
			swapInterval = 1;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/NOVSYNC"))
		{
			swapInterval = 0;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/NOJOYS"))
		{
			in_disableJoysticks = true;
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/NOCOPY"))
		{
			overrideCopyProtection = true;
		}
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
		else if (!CK_Cross_strcasecmp(argv[i], "/DUMPFILE"))
		{
			if (i < argc + 1)
				dumperFilename = argv[++i]; // Yes, we increment i twice
		}
#endif
	}

	if (!ck_currentEpisode->isPresent())
	{
		Quit("Couldn't find game data files!");
	}

#ifdef CK_ENABLE_PLAYLOOP_DUMPER
	extern FILE *ck_dumperFile;
	if (dumperFilename != NULL)
	{
		ck_dumperFile = fopen(dumperFilename, "wb");
		if (ck_dumperFile == NULL)
		{
			fprintf(stderr, "Couldn't open dumper file for writing.\n");
			return 1;
		}
		printf("Writing to dump file %s\n", dumperFilename);
	}
#endif

	vl_swapInterval = swapInterval;
	VL_SetParams(isFullScreen, isAspectCorrected, hasBorder, isIntegerScaled);

	if (overrideCopyProtection)
		ck_currentEpisode->hasCreatureQuestion = false;

	CK_InitGame();

	for (int i = 1; i < argc; ++i)
	{
		if (!CK_Cross_strcasecmp(argv[i], "/DEMOFILE"))
		{
			// A bit of stuff from the usual demo loop
			ck_gameState.levelState = LS_Playing;

			CK_PlayDemoFile(argv[i + 1]);
			Quit(0);
		}
		else if (!CK_Cross_strcasecmp(argv[i], "/PLAYDEMO"))
		{
			// A bit of stuff from the usual demo loop
			ck_gameState.levelState = LS_Playing;

			CK_PlayDemo(atoi(argv[i + 1]));
			Quit(0);
		}

	}

	if (us_noWait || us_tedLevel || CFG_GetConfigBool("debugActive", false))
		ck_debugActive = true;

	// Draw the ANSI "Press Key When Ready Screen" here
	CK_DemoLoop();
	CK_ShutdownID();
#ifdef CK_ENABLE_PLAYLOOP_DUMPER
	if (ck_dumperFile)
		fclose(ck_dumperFile);
#endif
	return 0;
}

#endif // CK_RUN_ACTION_VALIDATOR
