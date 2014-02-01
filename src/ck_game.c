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

void CK_LoadLevel(bool unknown)
{
	if (IN_DemoGetMode != IN_Demo_Off)
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

	RF_MarkTileGraphics();
	CA_CacheMarks(0);
	// Deactivate all objects
	for (CK_object *player = ck_keenObj; player; player = player->next)
	{
		if (player->active != OBJ_ALWAYS_ACTIVE)
			player->active = OBJ_INACTIVE;
	}
}

extern int ck_startingDifficulty;
void CK_GameLoop()
{
	do
	{
		if (ck_gameState.levelState != 6)
		{
			ck_gameState.difficulty = ck_startingDifficulty;
			ck_startingDifficulty = 0;
			CK_LoadLevel(true);

			//TODO: If this didn't succeed, return to level 0.
		}

replayLevel:
		ck_scrollDisabled = false;
		//SD_WaitSoundDone();
		CK_PlayLoop();

		if (ck_gameState.levelState != 6)
		{
			//TODO: Remove all keygems from inventory
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
				SD_PlaySound(13);
				ck_gameState.levelsDone[ck_currentMapNumber] = 0;
				ck_currentMapNumber = ck_nextMapNumber;
				//word_4A16A = ck_currentMapNumber;
				// TODO: If Keen launched with /Demo switch
				// Then function returns here
			}
			break;

		case 6:
			goto replayLevel;

		case 8:
			// Quit to Dos
			// IN_ClearKeysDown();
			return;

		case 14:
			// The Korath fuse was broken
			SD_PlaySound(13);
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


		case 3:
		case 4:
		case 9:
		case 10:
		case 11:
		case 12:
			break;
		}
	}	while (true); //livesLeft >= 0

	CK_GameOver();
	//TODO: Update High Scores
}
