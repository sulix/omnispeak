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

#include <stdlib.h>
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
		HelpScreens();
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

// The Fizzlefade routine
// Operates by drawing the title graphic into offscreen video memory, then
// using the hardware to copy the source to the display area, pixel-by-pixel

// The same effect can be achieved in omnispeak by drawing the title screen
// to an new surface, then copying from the surface to the screen
void CK_FizzleFade()
{
	int i;

	uint8_t bitmasks[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
	uint16_t columns1[320];
	uint16_t rows1[200];

	// Construct a multiplication table for multiples of
	for (i = 0; i < 320; i++)
		columns1[i] = i;


	// Shuffle the table entries
	for (i = 0; i < 320; i++)
	{

		// NOTE: BCC rand() implementation is capped at 0x7FFF
		int16_t var2 = (320 * (rand()&0x7FFF))/0x8000;

		uint16_t var4 = columns1[var2];

		columns1[var2] = columns1[i];

		columns1[i] = var4;
	}

	for (i = 0; i < 200; i++)
		rows1[i] = columns1[i];

	VL_SetDefaultPalette();

#if 0
	// DOS: Calculate the difference between dest and src

	if (word_46CA6)
	{
		bufferofs = word_499C7+124;
		bufferofs_0 = word_499C7;
	}
	else
	{
		bufferofs_0 = 124 + (bufferofs = word_499C7);
	}
#endif

	// VW_SetScreen(0, bufferofs_0);

	// DOS: Draw Title Bitmap offscreen
	// VW_DrawBitmap(0,0,PIC_TITLESCREEN);

	// SDL: Draw it to a new surface
	uint8_t *titleBuffer = (uint8_t *)VL_CreateSurface(320, 200);

	// FIXME: This is cached somewhere else
	CA_CacheGrChunk(PIC_TITLESCREEN);

	VH_BitmapTableEntry dimensions = VH_GetBitmapTableEntry(PIC_TITLESCREEN - ca_gfxInfoE.offBitmaps);

	VL_UnmaskedToSurface(ca_graphChunks[PIC_TITLESCREEN], titleBuffer, 0, 0, dimensions.width*8, dimensions.height);

	// int16_t copyDelta = bufferofs_0 - bufferofs;

	// Do the fizzling
	//
	for (i = 0; i < 360; i++)
	{
		int16_t var_10 = i-160;

		if (var_10 < 0)
			var_10 = 0;

		int16_t var_12 = i;

		if (var_12 >= 200)
			var_12 = 199;

		for (int16_t y = var_10; y <= var_12; y++)
		{
			// DOS:
			// uint16_t var_E = bufferofs + ylookup[y];

			for (int attempt = 0; attempt < 2; attempt++)
			{
				uint16_t x = columns1[rows1[y]];

				if (++rows1[y] == 320)
					rows1[y] = 0;

				// Here's what happens in DOS Keen, for reference
#if 0
				_SI = x % 8;

				// Set bitmask register to draw the correct pixel of the incoming data
				//outw(0x3CE, si<<8|8);

				// Point source and destinations to respective areas in vmemory
				_SI = var_E + x / 8;
				_DI = _SI + copyDelta;

				// NOTE: loop is unrolled in omnispeak
				for (plane = 0; plane < 4; plane++)
				{
					// Enable memory write to one color plane only
					outw(0x3C4, (1<<plane)<<8|2);

					// Read from the corresponding color plane
					outw(0x3CE, (1*plane)<<8|4);

					// Move the pixel
					asm mov bl, es:[si];
					asm xchg bl, es:[di];
				}
#endif

				// Now the SDL version...
				VL_SurfaceToScreen(titleBuffer, x, y, x, y, 1, 1);
				// VL_SurfaceToScreen(titleBuffer, 0, 0, 0, 0, 320, 200);

			}

		}

		VL_Present();

		// VL_WaitVBL(1);
		VL_DelayTics(1);

		IN_PumpEvents();

		if (IN_CheckAck())
			if (IN_GetLastScan() == IN_SC_F1)
				IN_SetLastScan(IN_SC_Space);

		if (IN_GetLastScan())
		{
			// Enable bitmask across all planes
			// out(0x3CE, 0xFF08);

			// Write enable all memory planes
			// out(0x3C4, 0xF02);

			// free(titleBuffer);

			return;
		}
	}

	// Title screen is now drawn, wait for a bit

	// Enable bitmask across all planes
	// out(0x3CE, 0xFF08);

	// Write enable all memory planes
	// out(0x3C4, 0xF02);

	IN_UserInput(420, false);

	// VL_DestroySurface?
	// free(titleBuffer);

}

void CK_DrawTerminator(void)
{
	// TODO: Implement all terminator functions
	// In the meantime, there's this placeholder

	bool terminator_complete = false;
	VL_ClearScreen(0);
	VL_SetScrollCoords(0,0);
#if 0
	{
		//CA_CacheGrChunk(3);
		int firsttime = SD_GetLastTimeCount();
	do
	{
		char buf[80];

		sprintf(buf, "Terminator Graphic: %d", (SD_GetTimeCount()-firsttime)/70);

		VH_DrawPropString(buf, 30, 10, 0, 15);
		VH_DrawPropString("Ends at 3", 30, 20, 0, 15);

		IN_PumpEvents();
		VL_DelayTics(35);
		VL_Present();

		if ((SD_GetTimeCount()-firsttime)/70 > 3)
			break;


	} while (!IN_GetLastScan());
	}
#endif

	// Placeholder ends here: this is now real keen code.

	// Somewhere around here, the terminator itself ends and the fizzle fade begins

	// After the terminator text has run, keys are checked
	if (!IN_GetLastScan())
	{
		; //terminator2();
	}

	if (!IN_GetLastScan())
	{
	  CK_FizzleFade();
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
		CA_CacheGrChunk(PIC_TITLESCREEN);
		VH_DrawBitmap(0, 0, PIC_TITLESCREEN);
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

uint8_t *ck_starWarsPalette;

void CK_DrawStarWars()
{
	// Keen5 sets the palette to the default one here.
	VL_ClearScreen(0);
	VL_SetScrollCoords(0,0);

	CA_SetGrPurge();
	// Cache the Star Wars font.
	CA_CacheGrChunk(5);
	// Render out the story text (to an offscreen buffer?)
	CA_CacheGrChunk(PIC_STARWARS); // Story bkg image.

	// Keen draws this to a separate surface, for fast copies.
	VH_DrawBitmap(0, 0, PIC_STARWARS);

	VL_SetPalette(ck_starWarsPalette);

	// At this point, Keen generates a set of buffers full of machine code,
	// one per line, which scale the text (from the surface mentioned above)
	// to make the "Star Wars" effect. (sub_152AE)

	StartMusic(17);


	// TODO: Implement
	// In the meantime, there's this placeholder
#if 1
	int firsttime = SD_GetLastTimeCount();
	CA_CacheGrChunk(3);
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
	StopMusic();

	VL_ClearScreen(0);
	VL_SetScrollCoords(0,0);
	VL_SetDefaultPalette();
	CA_ClearMarks();
#endif
}

void CK_ShowTitleScreen()
{
	// scrollofs = 0;
	CA_CacheGrChunk(PIC_TITLESCREEN);
	VH_DrawBitmap(0,0,PIC_TITLESCREEN);
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

	ck_gameState.currentLevel =demoMap;

	IN_DemoStartPlaying(demoBuf, demoLen);

	CK_LoadLevel(true);

	CK_PlayLoop();

	MM_FreePtr((void **)&demoBuf);
}

void CK_PlayDemo(int demoNumber)
{
	uint8_t *demoBuf;

	int demoChunk = DEMOSTART + demoNumber;

	CK_NewGame();

	CA_CacheGrChunk(demoChunk);
	demoBuf = (uint8_t *)(ca_graphChunks[demoChunk]);
	MM_SetLock(&ca_graphChunks[demoChunk], true);

	uint16_t demoMap = *demoBuf;
	demoBuf += 2;
	uint16_t demoLen = *((uint16_t *) demoBuf);
	demoBuf += 2;

	ck_gameState.currentLevel =demoMap;

	IN_DemoStartPlaying(demoBuf, demoLen);

	CK_LoadLevel(true);



	if (ck_inHighScores)
		CK_OverlayHighScores();

	CK_PlayLoop();
	IN_DemoStopPlaying();

	// We have to get rid of the demo buffer, as if we want to play it
	// again, we need a fresh copy. ID_IN modifies the buffer.
	MM_FreePtr(&ca_graphChunks[demoChunk]);
	//VW_SyncPages();
	CA_ClearMarks();

	// What should we do after playing the demo?
	CK_HandleDemoKeys();

}

/*
 * High scores
 */

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
extern void *rf_tileBuffer;
#include "id_vl_private.h"
void CK_OverlayHighScores()
{
  // Omnispeak
  int topMargin = ck_currentEpisode->ep == EP_CK4 ? 0x33 :
    ck_currentEpisode->ep == EP_CK5 ? 0x23 :
    0;

  int rightMargin = ck_currentEpisode->ep == EP_CK4 ? 0x128 :
    ck_currentEpisode->ep == EP_CK5 ? 0x118 :
    0;

  int leftMargin = ck_currentEpisode->ep == EP_CK4 ? 0x18 :
    ck_currentEpisode->ep == EP_CK5 ? 0x28 :
    0;

	RF_Reposition (0,0);

  // DOS: Set the back buffer to the master tilebuffer
  // The print routines draw to the backbuffer
	// oldbufferofs = bufferofs;
	// bufferofs = masterofs;

  // Simulate this in Omnispeak by replacing the tilebuffer surface
  // for the screen surface

  void *screen = vl_emuegavgaadapter.screen;
	vl_emuegavgaadapter.screen = rf_tileBuffer;

  if (ck_currentEpisode->ep == EP_CK5)
    US_SetPrintColour(12);

	for (int entry = 0; entry < 8; entry++)
	{
		// Print the name
		US_SetPrintY(16*entry+topMargin);
		US_SetPrintX(leftMargin);
		US_Print(ck_highScores[entry].name);

    // Keen 4: print the councilmembers rescued
    if (ck_currentEpisode->ep == EP_CK4)
    {
      US_SetPrintX(0x98);
      for (int i = 0; i < ck_highScores[entry].arg4; i++)
      {
        VH_DrawTile8(US_GetPrintX(), US_GetPrintY()+1, 0x47);
        US_SetPrintX(US_GetPrintX() + 8);
      }
    }

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
    US_SetPrintX(rightMargin-w);
		US_Print(buf);
	}

	US_SetPrintColour(15);

  // restore the backbuffer
	// bufferofs = oldbufferofs;  // DOS
	vl_emuegavgaadapter.screen = screen;

}

// Enter name if a high score has been achieved
static bool ck_highScoresDirty;
void CK_SubmitHighScore(int score, uint16_t arg_4)
{
  // Omnispeak
  int topMargin = ck_currentEpisode->ep == EP_CK4 ? 0x33 :
    ck_currentEpisode->ep == EP_CK5 ? 0x23 :
    0;

  int rightMargin = ck_currentEpisode->ep == EP_CK4 ? 0x128 :
    ck_currentEpisode->ep == EP_CK5 ? 0x118 :
    0;

  int leftMargin = ck_currentEpisode->ep == EP_CK4 ? 0x18 :
    ck_currentEpisode->ep == EP_CK5 ? 0x28 :
    0;

	int entry, entryRank;

	CK_HighScore newHighScore;
	strcpy(newHighScore.name, "");
	newHighScore.score = score;
	newHighScore.arg4 = arg_4;



	// Check if this entry made the high scores
	entryRank = -1;
	for (entry = 0; entry < 8; entry++)
	{
		if (ck_highScores[entry].score >= newHighScore.score)
			continue;

		if (newHighScore.score == ck_highScores[entry].score)
		{
			if (ck_highScores[entry].arg4 >= newHighScore.arg4)
				continue;
		}

    // Made it in!
    // Insert the new high score into the proper slot
    for (int e = 8; --e > entry; )
      memcpy(&ck_highScores[e], &ck_highScores[e-1], sizeof(newHighScore));

    memcpy(&ck_highScores[entry], &newHighScore, sizeof(newHighScore));
    entryRank = entry;
    ck_highScoresDirty = true;


    break;
	}


	if (entryRank != -1)
	{
		ck_inHighScores = true;
		ck_gameState.currentLevel = ck_currentEpisode->highScoreLevel;
		CK_LoadLevel(true);
		CK_OverlayHighScores();
    if (ck_currentEpisode->ep == EP_CK5)
      US_SetPrintColour(12);

		// FIXME: Calling these causes segfault
		RF_Refresh();
		RF_Refresh();

		US_SetPrintY(entry*16 + topMargin);
		US_SetPrintX(leftMargin);

		US_LineInput(US_GetPrintX(), US_GetPrintY(), ck_highScores[entryRank].name, 0, 1, 0x39, 0x70);

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
