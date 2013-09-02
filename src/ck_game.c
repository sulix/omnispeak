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

/*
 * NewGame: Setup the default starting stats
 */

void CK_NewGame()
{
	//TODO: Implement
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

	//TODO: Put this in the right place.
	ck_gameState.numShots = 5;
	
	CA_CacheMap(ck_currentMapNumber);
	RF_NewMap(ck_currentMapNumber);

	CK_SetupObjArray();
	CK5_ScanInfoLayer();
	
	// Deactivate all objects
	for (CK_object *player = ck_keenObj; player; player = player->next)
	{
		if (player->active != OBJ_ALWAYS_ACTIVE)
			player->active = OBJ_INACTIVE;
	}
}


void CK_GameLoop()
{
	do
	{
		if (ck_gameState.levelState != 6)
		{
			CK_LoadLevel(true);

			//TODO: If this didn't succeed, return to level 0.
		}

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
			//We've won, return to main map.
			//TODO: Play sound 13
			ck_currentMapNumber = 0;
			break;
		}
	}
	while (true); //livesLeft >= 0
	
	CK_GameOver();
	//TODO: Update High Scores
}
