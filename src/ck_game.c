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
#include "SDL.h"

#include "id_vl.h"
#include "id_us.h"
#include "id_ca.h"
#include "id_rf.h"
#include "ck_def.h"
#include "ck_play.h"
#include "ck_text.h"
#include "ck5_ep.h"

/*
 * NewGame: Setup the default starting stats
 */

void CK_NewGame()
{
	// TODO: Zero the ck_gameState
	memset(&ck_gameState, 0, sizeof(ck_gameState));
	ck_gameState.nextKeenAt = 20000;
	ck_gameState.numLives = 3;
	ck_gameState.numShots = 5;
	ck_gameState.currentLevel = 0;
}

void CK_GameOver()
{
	// HACK: US_CPrint modifies strings. So don't use
	// literals directly. (char * is NOT ok; Use char arrays.)
	static char gameOverString[] = "gameOverString";

	US_CenterWindow(16, 3);
	US_CPrint(gameOverString);
	VL_Present();
	//TODO: Wait 4*70 tics
	IN_WaitKey();
}

//TODO: Save Game
bool CK_SaveGame (int handle)
{
int cmplen, bufsize, i;
	CK_object *obj;
	uint8_t *buf;

	/* This saves the game */

	/* Write out Keen stats */
	ck_keenState.platform = NULL;
	if( !CA_FarWrite( handle, (uint8_t*)&ck_gameState, 86 ) )
		return false;

	bufsize = CA_GetMapWidth() * CA_GetMapHeight() * 2;
	MM_GetPtr( (mm_ptr_t *)buf, bufsize/*, 0*/ ); /* or ( &buf, (long)bufsize ); -- allocate mem */

	/* Compress and save the current level */
	for( i = 0; i < 3; i++ ) {
		cmplen = CAL_RLEWCompress( CA_TilePtrAtPos(0,0,i), bufsize, buf + 2, 0xABCD );

		/* Write the size of the compressed level */
		*((uint32_t *)buf) = cmplen;
		if( !CA_FarWrite( handle, buf, cmplen + 2 ) ) {
			/* Free the buffer and return failure */
			MM_FreePtr( (mm_ptr_t *)buf );
			return false;
		}
	}

	/* Save all the objects */
	for( obj = ck_keenObj; obj != NULL; obj = obj->next ) {
		if( !CA_FarWrite( handle, (uint8_t *)obj, 76 ) ) {
			/* Free the buffer and return failure */
			MM_FreePtr( (mm_ptr_t *)buf );
			return false;
		}
	}

	/* Free the buffer and return success */
	MM_FreePtr( (mm_ptr_t *)buf );
	return true;

}
//TODO: Load Game


//TODO: KillKeen

void CK_ExitMenu(void)
{
	CK_NewGame();
	// TODO: With this, cache message doesn't appear...
	// (nothing to cache in CA_CacheMarks)
#if 0
	ca_levelnum--;
	ca_levelbit >>= 1;
	CA_ClearMarks();
	ca_levelbit <<= 1;
	ca_levelnum++;
#endif
}

void CK_MapLevelMarkAsDone(void)
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

static int16_t ck_fadeDrawCounter;

void CK_UpdateFadeDrawing(void)
{
	ck_fadeDrawCounter++;
	if (ck_fadeDrawCounter == 2)
	{
		VL_FadeFromBlack();
		RF_SetDrawFunc(0);
		SD_SetTimeCount(SD_GetLastTimeCount());
	}
}

void CK_BeginFadeDrawing(void)
{
	VL_FadeToBlack();
	ck_fadeDrawCounter = 0;
	RF_SetDrawFunc(&CK_UpdateFadeDrawing);
}

const char *ck_levelNames[] = {
	"Omegamatic",
	"Ion Ventilation System",
	"Security Center",
	"Defense Tunnel Vlook",
	"Energy Flow Systems",
	"Defense Tunnel Burrh",
	"Regulation\nControl Center",
	"Defense Tunnel Sorra",
	"Neutrino\nBurst Injector",
	"Defense Tunnel Teln",
	"Brownian\nMotion Inducer",
	"Gravitational\nDamping Hub",
	"Quantum\nExplosion Dynamo",
	"Korath III Base",
	"BWBMegarocket",
	"High Scores",
};

// HACK: Sorry, the strings need to be in WRITABLE storage,
// because US_CPrint (temporarily) modifies them.

char ck_levelEntryText_0[] =
	"Keen purposefully\n"
	"wanders about the\n"
	"Omegamatic";

char ck_levelEntryText_1[] =
	"Keen investigates the\n"
	"Ion Ventilation System";

char ck_levelEntryText_2[] =
	"Keen struts through\n"
	"the Security Center";

char ck_levelEntryText_3[] =
	"Keen invades\n"
	"Defense Tunnel Vlook";

char ck_levelEntryText_4[] =
	"Keen engages\n"
	"Energy Flow Systems";

char ck_levelEntryText_5[] =
	"Keen barrels into\n"
	"Defense Tunnel Burrh";

char ck_levelEntryText_6[] =
	"Keen goes nuts in\n"
	"the Regulation\n"
	"Control Center";

char ck_levelEntryText_7[] =
	"Keen regrets entering\n"
	"Defense Tunnel Sorra";

char ck_levelEntryText_8[] =
	"Keen blows through\n"
	"the Neutrino\n"
	"Burst Injector";

char ck_levelEntryText_9[] =
	"Keen trots through\n"
	"Defense Tunnel Teln";

char ck_levelEntryText_10[] =
	"Keen breaks into\n"
	"the Brownian\n"
	"Motion Inducer";

char ck_levelEntryText_11[] =
	"Keen hurries through\n"
	"the Gravitational\n"
	"Damping Hub";

char ck_levelEntryText_12[] =
	"Keen explodes into\n"
	"the Quantum\n"
	"Explosion Dynamo";

char ck_levelEntryText_13[] =
	"Keen faces danger\n"
	"in the secret\n"
	"Korath III Base";

char ck_levelEntryText_14[] =
	"Keen will not be\n"
	"in the BWBMegarocket";

char ck_levelEntryText_15[] =
	"Keen unexplainedly\n"
	"find himself by\n"
	"theHigh Scores";

char *ck_levelEntryTexts[] = {
	ck_levelEntryText_0,
	ck_levelEntryText_1,
	ck_levelEntryText_2,
	ck_levelEntryText_3,
	ck_levelEntryText_4,
	ck_levelEntryText_5,
	ck_levelEntryText_6,
	ck_levelEntryText_7,
	ck_levelEntryText_8,
	ck_levelEntryText_9,
	ck_levelEntryText_10,
	ck_levelEntryText_11,
	ck_levelEntryText_12,
	ck_levelEntryText_13,
	ck_levelEntryText_14,
	ck_levelEntryText_15,
};

#if 0
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
#endif

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
#if 0
	//TODO: Put these in the right place.
	ck_gameState.nextKeenAt = 20000;
	ck_gameState.numLives = 3;
	ck_gameState.numShots = 5;
#endif
	CA_CacheMap(ck_gameState.currentLevel);
	RF_NewMap();
	CA_ClearMarks();

	CK_SetupObjArray();
	CK5_ScanInfoLayer();

	if (ca_mapOn == 0)
	{
		CK_MapLevelMarkAsDone();
	}

	RF_MarkTileGraphics();
	//MM_BombOnError();
	CA_LoadAllSounds();

	// Cache Marked graphics and draw loading box
	if (doCache)
	{
		// HACK: US_CPrint modifies strings. So don't use
		// literals directly. (char * is NOT ok; Use char arrays.)
		static char demoString[] = "DEMO";
		static char korathString[] = "Keen steps out\nonto Korath III";
		if (ck_inHighScores)
		{
			CA_CacheMarks(NULL);
		}
		else if (IN_DemoGetMode() != IN_Demo_Off)
		{
			CA_CacheMarks(demoString);
		}
		else if (ca_mapOn == 0 && ck_keenObj->clipRects.tileY1 > 100)
		{
			/* Stepping on to korath*/
			CA_CacheMarks(korathString);
		}
		else
		{
			CA_CacheMarks(ck_levelEntryTexts[ca_mapOn]);
		}
	}


	// CA_CacheMarks(0);
	CK_BeginFadeDrawing();
}


// Cache Box Routines
// These are accessed as callbacks by the caching manager

static int ck_cacheCountdownNum, ck_cacheBoxChunksPerPic, ck_cacheBoxChunkCounter;

// The string may be temporarily modified by US_CPrint, hence it's non-const.
void CK_BeginCacheBox (char *title, int numChunks)
{
	int totalfree;
	uint16_t w, h;

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
	US_CPrint(title);
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

void CK_TryAgainMenu()
{
	uint16_t w, h;
	int y1, y2, sel;
	int y, clr;

	char buf[80];

	// HACK: US_CPrint modifies strings. So don't use
	// literals directly. (char * is NOT ok; Use char arrays.)
	static char youDidntMakeItPastString[] = "You didn't make it past";
	static char tryAgainString[] = "Try Again";
	static char exitToArmageddonString[] = "Exit to Armageddon";

	/* Copy and measure the level name */
	strcpy( buf, ck_levelNames[ca_mapOn] );
	CK_MeasureMultiline( buf, &w, &h );

	/* Take away all gems */
	memset( ck_gameState.keyGems, 0, sizeof (ck_gameState.keyGems) );

	/* If lives remain, see if they want to try this level again */
	if ( --ck_gameState.numLives >= 0 )
	{
		//VW_SyncPages();
		US_CenterWindow( 20, 8 );
		US_SetPrintY(US_GetPrintY() + 3);
		US_CPrint(youDidntMakeItPastString);
		y1 = US_GetPrintY() + 22;

		/* Center the level name vertically */
		if ( h < 15 )
			US_SetPrintY(US_GetPrintY() + 4);
		US_CPrint(buf);

		US_SetPrintY(y1 + 2);
		US_CPrint(tryAgainString);
		US_SetPrintY(US_GetPrintY() + 4);
		y2 = US_GetPrintY() - 2;
		US_CPrint(exitToArmageddonString);

		IN_ClearKeysDown();
		sel = 0;
		while ( 1 )
		{
			SDL_Delay(1);
			IN_PumpEvents();

			/* Decide which selection to draw */
			if ( sel != 0 )
				y = y2;
			else
				y = y1;

			/* Choose a color to draw it in */
			if ( (SD_GetTimeCount() >> 4) & 1 )
				clr = 12;
			else
				clr = 1;

			/* And draw the selection box */
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y, clr );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 1, clr );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 12, clr );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 13, clr );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + 4, clr );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + 5, clr );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 4, clr );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 5, clr );
			VL_Present();

			/* Erase the box for next time */
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y, 15 );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 1, 15 );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 12, 15 );
			VH_HLine( US_GetWindowX() + 4, US_GetWindowX() + US_GetWindowW() - 4, y + 13, 15 );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + 4, 15 );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + 5, 15 );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 4, 15 );
			VH_VLine( y + 1, y + 11, US_GetWindowX() + US_GetWindowW() - 5, 15 );

			/* If they press Esc, they want to go back to the Map */
			if ( IN_GetLastScan() == IN_SC_Escape )
			{
				ck_gameState.currentLevel = 0;
				IN_ClearKeysDown();
				return;
			}

			IN_ReadControls( 0, &ck_inputFrame );
			if ( ck_inputFrame.jump || ck_inputFrame.pogo || IN_GetLastScan() == IN_SC_Enter || IN_GetLastScan() == IN_SC_Space )
			{
				/* If they want to go back to the Map, set the current level to zero */
				if ( sel != 0 )
					ck_gameState.currentLevel = 0;
				return;
			}

			if ( ck_inputFrame.yDirection == -1 || IN_GetLastScan() == IN_SC_UpArrow )
			{
				sel = 0;
			}
			else if ( ck_inputFrame.yDirection == 1 || IN_GetLastScan() == IN_SC_DownArrow )
			{
				sel = 1;
			}
		} /* while */
	}
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
			CK_TryAgainMenu();
			//ck_gameState.currentLevel = ck_nextMapNumber;
			break;

		case 2:
		case 7:
		case 13:
			if (ca_mapOn == 0)
			{
				// US_CenterWindow(8, 0x1A);
				// window_print_y += 0x19;
				// window_print("One Moment");

				// This is an omnispeak hack
				// because we can't change ck_gameState.currentLevel
				// from within CK_ScanForLevelEntry
				// UPDATE (Mar 6 2014): Not the case anymore
				//ck_gameState.currentLevel = ck_nextMapNumber;
			}
			else
			{
				//We've won, return to main map.
				//TODO: Mark level as done (and more)
				SD_PlaySound(SOUND_LEVELEXIT);
				ck_gameState.currentLevel = 0;
				//word_4A16A = ca_mapOn;
				ck_gameState.levelsDone[ca_mapOn] = 1;
				// TODO: If Keen launched with /Demo switch
				// Then function returns here based on ca_mapOn
			}
			break;

		case 5:
			goto resetDifficultyAndLevel;

		case 6:
			goto replayLevel;

		case 8:
			// Quit to Dos
			IN_ClearKeysDown();
			return;

		case 14:
			// The level has been ended by fuse destruction
			SD_PlaySound(SOUND_LEVELEXIT);
			//word_4A16A = ca_mapOn;
			ck_gameState.levelsDone[ca_mapOn] = 14;
			CK5_FuseMessage();
			ck_gameState.currentLevel = 0;
			break;

		case 15:
			// The QED was destroyed
			/*
			 * purge_chunks()
			 * RF_Reset();
			 * VW_SyncPages();
			 */
			help_endgame();
#if 0
			CK_SubmitHighScore(ck_gameState.keenScore, 0);
#endif
			return;
#if 0
			// Warping level
			// This code is added for omnispeak so that the warp functionality works
			// Case 4 normally switches to default
			// UPDATE (Mar 6 2014): Not needed anymore.
		case 4:
			ck_gameState.currentLevel = ck_nextMapNumber;
			break;
#endif


		case 3:
		case 9:
		case 10:
		case 11:
		case 12:
			break;
		}

		if (ck_gameState.numLives < 0)
			break;

		goto loadLevel; //livesLeft >= 0
	}	while (true);

#if 0
	// Keen4/6 simple game over
	CK_GameOver();
#endif

	// Keen 5: Blow up the galaxy
	CK5_ExplodeGalaxy();

	//TODO: Update High Scores
#if 0
	CK_SubmitHighScore(ck_gameState.keenScore, 0);
#endif
}
