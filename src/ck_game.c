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

#include "id_vl.h"
#include "id_us.h"
#include "id_ca.h"
#include "id_rf.h"
#include "ck_def.h"
#include "ck_play.h"
#include "ck5_ep.h"

int ck_nextMapNumber;

/*
 * NewGame: Setup the default starting stats
 */

void CK_NewGame()
{
	// TODO: Zero the ck_gameState
	ck_gameState.nextKeenAt = 20000;
	ck_gameState.numLives = 3;
	ck_gameState.numShots = 5;
}

void CK_GameOver()
{
	US_CenterWindow(16, 3);
	US_CPrint("Game Over!");
	VL_Present();
	//TODO: Wait 4*70 tics
	IN_WaitKey();
}

//TODO: Save Game
//TODO: Load Game


//TODO: KillKeen

//TODO: Exit_menu_func

void CK_MapLevelMarkAsDone()
{
	int y, x, level, i, w, flags;
	uint16_t *pw;

	i = 0;
	pw = CA_TilePtrAtPos(0,0,2);	/* info layer */

	/* Look through the map for level-related tiles */
	for ( y = 0; y < CA_GetMapHeight(); y++ )
	{
		for ( x = 0; x < CA_GetMapWidth(); x++, pw++, i++ )
		{
			w = *pw;
			level = w & 0xFF;
			if ( level >= 1 && level <= 17 && ck_gameState.levelsDone[level] )
			{	/* Is this a level tile */
				flags = w >> 8;
				/* Set the info tile at this position to 0 */
				*pw = 0;
				if ( flags == 0xD0 )
				{	/* If this is a 'blocking' part of the level */
					/* Set the foreground tile at this position to 0 also (remove the fences) */
					CA_SetTileAtPos(x,y,1,0);
				}
				else if ( flags == 0xF0 )
				{	/* If this is the flag holder for the level */
#if 0
					if ( AZ : A7C2 == level )
						sub_339( x, y );
					else
						sub_338( x, y );
#endif
					CK_MapFlagSpawn(x,y);
				}
			}
		}
	}

}

const char *ck_levelEntryTexts[] ={
	"Keen purposefully\n"
	"wanders about the\n"
	"Omegamatic",

	"Keen investigates the\n"
	"Ion Ventilation System",

	"Keen struts through\n"
	"the Security Center",

	"Keen invades\n"
	"Defense Tunnel Vlook",

	"Keen engages\n"
	"Energy Flow Systems",

	"Keen barrels into\n"
	"Defense Tunnel Burrh",

	"Keen goes nuts in\n"
	"the Regulation\n"
	"Control Center",

	"Keen regrets entering\n"
	"Defense Tunnel Sorra",

	"Keen blows through\n"
	"the Neutrino\n"
	"Burst Injector",

	"Keen trots through\n"
	"Defense Tunnel Teln",

	"Keen breaks into\n"
	"the Brownian\n"
	"Motion Inducer",

	"Keen hurries through\n"
	"the Gravitational\n"
	"Damping Hub",

	"Keen explodes into\n"
	"the Quantum\n"
	"Explosion Dynamo",

	"Keen faces danger\n"
	"in the secret\n"
	"Korath III Base",

	"Keen will not be\n"
	"in the BWBMegarocket",

	"Keen unexplainedly\n"
	"find himself by\n"
	"theHigh Scores",

};

void CK_LoadLevel(bool doCache)
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

	//TODO: Put these in the right place.
	ck_gameState.nextKeenAt = 20000;
	ck_gameState.numLives = 3;
	ck_gameState.numShots = 5;

	CA_CacheMap(ck_currentMapNumber);
	RF_NewMap(ck_currentMapNumber);
	CA_ClearMarks();

	CK_SetupObjArray();
	CK5_ScanInfoLayer();

	if (ck_currentMapNumber == 0)
	{
		CK_MapLevelMarkAsDone(); 
	}

	RF_MarkTileGraphics();
	//MM_BombOnError();
	// CA_LoadAllSounds()

	// Cache Marked graphics and draw loading box
	if (doCache)
	{
#if 0
		if (ck_inHighScores)
		{
			CA_CacheMarks(NULL);
		}
		else
#endif
			if (IN_DemoGetMode() != IN_Demo_Off)
		{
			CA_CacheMarks("DEMO");
		}
		else if (ck_currentMapNumber == 0 && ck_keenObj->clipRects.tileY1 > 100)
		{
			/* Stepping on to korath*/
			CA_CacheMarks("Keen steps out\nonto Korath III");
		}
		else
		{
			CA_CacheMarks(ck_levelEntryTexts[ck_currentMapNumber]);
		}
	}


	// CA_CacheMarks(0);
	// Deactivate all objects
	for (CK_object *player = ck_keenObj; player; player = player->next)
	{

		if (player->active != OBJ_ALWAYS_ACTIVE)
			player->active = OBJ_INACTIVE;
	}
}


// Cache Box Routines
// These are accessed as callbacks by the caching manager

static int ck_cacheCountdownNum, ck_cacheBoxChunksPerPic, ck_cacheBoxChunkCounter;

void CK_BeginCacheBox (char *title, int numChunks)
{
	int totalfree;
	int w, h;

	// Vanilla keen checks if > 2kb free here
	// If not, it doesn't cache all of the keen counting down graphics
	// But not necessary for omnispeak
#if 0
	if ((totalfree = MM_TotalFree()) > 2048)
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
		CA_CacheGrChunk(92 + i);
		ca_graphChunkNeeded[92 + i] &= ~ca_levelbit;

		// If a pic can't be cached, forget updating the hand pics
		// by setting the countdown counter at 5
		if (!ca_graphChunks[92 + i])
		{
			// mmerror = 0;
			ck_cacheCountdownNum = 5;
			break;
		}

		MM_SetPurge(ca_graphChunks + 92 + i, 3);
	}

	US_CenterWindow(26, 8);

	if (ca_graphChunks[92])
		VH_DrawBitmap(US_GetWindowX(), US_GetWindowY(), 92);
	else
		ck_cacheCountdownNum = 5;

	ca_graphChunkNeeded[92] &= !ca_levelbit;
	US_SetWindowW(US_GetWindowW() - 0x30);
	US_SetWindowX(US_GetWindowX() + 0x30);
	// Omnispeak FIXME: Start printX at the right spot
	// The following line is not present in Keen 5
	US_SetPrintX(US_GetWindowX());
	CK_MeasureMultiline(title, &w, &h);
	US_SetPrintY(US_GetPrintY() + (US_GetWindowH() - h) / 2 - 4);
	US_Print(title);
	VL_Present();

	ck_cacheBoxChunkCounter = ck_cacheBoxChunksPerPic = numChunks / 6;

	if (!ck_cacheBoxChunksPerPic && !ck_cacheCountdownNum)
	{
		ck_cacheCountdownNum = 5;
		if (ca_graphChunks[97])
		{

			VH_DrawBitmap(US_GetWindowX() - 24, US_GetWindowY() + 40, 97);
		}

		VL_Present();
	}
}

void CK_UpdateCacheBox()
{
	ck_cacheBoxChunkCounter--;

	if ( ck_cacheBoxChunkCounter == 0 && ck_cacheCountdownNum <= 4 )
	{

		ck_cacheBoxChunkCounter = ck_cacheBoxChunksPerPic;
		if ( ca_graphChunks[93 + ck_cacheCountdownNum] )
			VH_DrawBitmap( US_GetWindowX() - 24, US_GetWindowY() + 40, 93 + ck_cacheCountdownNum);
		VL_Present();
		// Because loading is VERY fast on omnispeak, add artificial delay
		VL_DelayTics(10);
AZ:
		ck_cacheCountdownNum++;
	}
}

void CK_FinishCacheBox()
{

}

extern CK_Difficulty ck_startingDifficulty;

void CK_GameLoop()
{
	do
	{
		if (ck_gameState.levelState != 6)
		{
resetDifficultyAndLevel:
			ck_gameState.difficulty = ck_startingDifficulty;
			ck_startingDifficulty = D_NotPlaying;
loadLevel:
			CK_LoadLevel(true);

			//TODO: If this didn't succeed, return to level 0.
		}

replayLevel:
		ck_scrollDisabled = false;
		SD_WaitSoundDone();
		CK_PlayLoop();

		if (ck_gameState.levelState != 6)
		{
			memset(ck_gameState.keyGems, 0, sizeof (ck_gameState.keyGems));
			// TODO: This is probably (but not necessarily) Keen 5 specific
			ck_gameState.securityCard = 0;
		}

		//TODO: Some TED launching stuff


		switch (ck_gameState.levelState)
		{
		case 1:
			//Kill Keen
			break;

		case 2:
		case 7:
		case 13:
			if (ck_currentMapNumber == 0)
			{
				// US_CenterWindow(8, 0x1A);
				// window_print_y += 0x19;
				// window_print("One Moment");

				// This is an omnispeak hack
				// because we can't change ck_currentMapNumber from within
				// CK_ScanForLevelEntry
				ck_currentMapNumber = ck_nextMapNumber;
			}
			else
			{
				//We've won, return to main map.
				//TODO: Mark level as done (and more)
				SD_PlaySound(SOUND_LEVELEXIT);
				ck_gameState.levelsDone[ck_currentMapNumber] = 1;
				ck_nextMapNumber = 0;
				ck_currentMapNumber = ck_nextMapNumber;
				//word_4A16A = ck_currentMapNumber;
				// TODO: If Keen launched with /Demo switch
				// Then function returns here
			}
			break;

		case 5:
			goto resetDifficultyAndLevel;

		case 6:
			goto replayLevel;

		case 8:
			// Quit to Dos
			// IN_ClearKeysDown();
			return;

		case 14:
			// The Korath fuse was broken
			SD_PlaySound(SOUND_LEVELEXIT);
			//word_4A16A = ck_currentMapNumber;
			ck_gameState.levelsDone[ck_currentMapNumber] = 14;
			// TODO: Fuse Message goes here
			ck_currentMapNumber = 0;
			break;

		case 15:
			// The QED was destroyed
			/*
			 * purge_chunks()
			 * RF_Reset();
			 * VW_SyncPages();
			 * win_game();
			 * loadHiscores(score)
			 */
			return;

			// Warping level
			// This code is added for omnispeak so that the warp functionality works
			// Case 4 normally switches to default
		case 4:
			ck_currentMapNumber = ck_nextMapNumber;
			break;


		case 3:
		case 9:
		case 10:
		case 11:
		case 12:
			break;
		}
		goto loadLevel; //livesLeft >= 0
	}	while (true);

	CK_GameOver();
	//TODO: Update High Scores
}
