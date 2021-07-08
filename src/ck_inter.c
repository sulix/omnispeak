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

#include "id_in.h"
#include "id_rf.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_play.h"
#include "ck_act.h"

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

	if ((ck_currentEpisode->ep != EP_CK6) && (IN_GetLastScan() == IN_SC_F1))
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

// I think this is how this works:
// We have 3 different planes moving over each other
//
// 1. The terminator line width is set to be a little bit wider than the KEEN graphic
// The Keen graphic is moved into Video memory, Once, before the scrolling starts.
// The top left of the screen is initialized to draw just to the right of the KEEN graphic,
// and it scrolls left until the Keen graphic has moved off the right edge of the screen
// KEEN is only drawn to VIDEO MEMORY plane 0
//
// 2. While this is happening, the visible part of the COMMANDER graphic is drawn
// The COMMANDER graphic itself is loaded into main memory before the scrolling routine starts,
// in 8 different shifts for efficient smooth scrolling
// COMMANDER is only drawn to VIDEO MEMORY plane 1

// 3. On top of this, the scrolling credits are drawn (again so that they don't modify the KEEN)
// in Video memory
// CREDITS are drawn to VIDEO MEMORY planes 2 and 3
//

// Once all of this is finished, the zoomout routine runs.
// It dynamically scales a bitmap that is composed of the two COMMANDER and KEEN graphics

#define TERMINATORSCREENWIDTH 248

// Pointers to the two monochrome bitmaps that are scrolled during the intro

introbmptype *ck_introKeen;
introbmptype *ck_introCommander;

// segment pointers to various graphic chunks
mm_ptr_t introbuffer;
mm_ptr_t introbuffer2;
mm_ptr_t shiftedCmdrBMPsegs[8];

uint8_t terminator_blackmasks[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
uint8_t terminator_lightmasks[] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
uint8_t *ck_terminator_palette1;
uint8_t *ck_terminator_palette2;

// chunk #'s for the vertically scrolling pics
int *terminator_pics[4] = {&PIC_CREDIT1, &PIC_CREDIT2, &PIC_CREDIT3, &PIC_CREDIT4};
// storage for the vertically scrolling pics
mm_ptr_t terminator_pic_memptrs[4];

// Width of the COMMANDER graphic, in BYTES
int ck_introCommanderWidth;

int ck_introScreenWidth;

// type-identified variables
int ck_currentTermPicTimeRemaining;
int ck_currentTermPicStartTime;

// Which terminator credit image we're drawing
int ck_currentTerminatorCredit;

// What action the terminator credit is currently doing (loading/flying up/pausing/flying out)
int ck_termCreditStage;
// int rowsToDraw;

unsigned *word_499D1;
mm_ptr_t ck_currentTermPicSeg;

unsigned word_499CB;
// unknown vairables

uint16_t word_46CA2[2];
uint16_t terminatorPageOn; // always 0 or 1? Maybe a pageflip thing?
uint16_t word_46CB0;

uint16_t word_499C7;
uint16_t terminatorOfs; // Start of the screen buffer; the KEEN graphic is aligned to the left edge of this
uint8_t terminatorPlaneOn;
uint16_t ck_currentTermPicNextLineDist;
uint16_t ck_currentTermPicSize;
uint16_t ck_currentTermPicHeight;
uint16_t ck_currentTermPicHalfWidth;
uint16_t ck_currentTermPicWidth;
uint16_t ck_termPicHeights[5];
uint16_t ck_termPicWidths[5];

void DoubleSizeTerminatorPics(int pic);
int AdvanceTerminatorCredit(int elapsedTime);
void ScrollTerminatorCredits(uint16_t elapsedTime, uint16_t xpixel);
void AnimateTerminator(void);
void TerminatorExpandRLE(uint16_t *src, uint8_t *dest);
void JoinTerminatorPics(void);
void ZoomOutTerminator_1(uint16_t *rleData, uint16_t arg4, int leftOffset, uint16_t scaleFactor);
void ZoomOutTerminator(void);
void CK_FizzleFade(void);

// Returns the Y-position of the terminator credit
int AdvanceTerminatorCredit(int elapsedTime)
{

	int picchunk;
	VH_BitmapTableEntry *bmp;

	switch (ck_termCreditStage)
	{

	case -1:
		// loading
		picchunk = *terminator_pics[ck_currentTerminatorCredit];
		CA_CacheGrChunk(picchunk);
		bmp = VH_GetBitmapTableEntry(picchunk - ca_gfxInfoE.offBitmaps);
		ck_currentTermPicSeg = ca_graphChunks[picchunk];
		ck_currentTermPicWidth = bmp->width; // This is width in EGA bytes (1/8 px)
		ck_currentTermPicHalfWidth = (ck_currentTermPicWidth + 3) >> 1;
		ck_currentTermPicHeight = bmp->height;
		// The size in bytes of the pic.
		ck_currentTermPicSize = (ck_currentTermPicWidth * ck_currentTermPicHeight);
		ck_termCreditStage++;

		ck_currentTermPicStartTime = elapsedTime;
		ck_currentTermPicTimeRemaining = 240;
		// no break, flow into case 0

	case 0:

		// Flying up from bottom of screen
		ck_currentTermPicTimeRemaining -= (elapsedTime - ck_currentTermPicStartTime) << 1;

		if (ck_currentTermPicTimeRemaining < 100)
		{
			ck_currentTermPicTimeRemaining = 100;
			ck_termCreditStage++;
		}

		ck_currentTermPicStartTime = elapsedTime;

		return ck_currentTermPicTimeRemaining - (ck_currentTermPicHeight >> 1);

	case 1:

		// Pausing in the middle
		if (elapsedTime - ck_currentTermPicStartTime > 200)
		{
			ck_termCreditStage++;
			ck_currentTermPicStartTime = elapsedTime;
		}

		return 100 - (ck_currentTermPicHeight >> 1);

	case 2:

		// Flying up and out of the top of the screen
		ck_currentTermPicTimeRemaining -= (elapsedTime - ck_currentTermPicStartTime) << 1;

		if (ck_currentTermPicTimeRemaining < -40)
		{
			ck_currentTermPicTimeRemaining = -40;

			// Are there any more terminator credit images?
			if (++ck_currentTerminatorCredit < 4)
				ck_termCreditStage = -1;
			else
				ck_termCreditStage = 3;
		}

		ck_currentTermPicStartTime = elapsedTime;

		return ck_currentTermPicTimeRemaining - (ck_currentTermPicHeight >> 1);

	default:
		break;
	}

	return -40;
}

void ScrollTerminatorCredits(uint16_t elapsedTime, uint16_t xpixel)
{
	// Vars are static because BP is used during ASM draw routine
	static int rowsToDraw;

	int creditY1, picStartOffset;

	int picX = (xpixel) + (160 - ck_currentTermPicWidth * 4);

	VL_SetMapMask(0xC);

	creditY1 = AdvanceTerminatorCredit(elapsedTime);

	// Erasing the area underneath the credit
	static int oldY2 = 0;
	int creditY2 = creditY1 + ck_currentTermPicHeight;
	// The original assembly code here cleared a word at a time, hence the
	// rounding here.
	int termPicClearWidth = ((ck_currentTermPicWidth + 3) / 2) * 16;

	if (creditY2 < 0)
		creditY2 = 0;

	if (creditY2 < 200 && oldY2 > creditY2)
	{
		int rowsToClear = oldY2 - creditY2;
		// Omnispeak: Just draw an empty rectangle over the area
		VL_ScreenRect_PM(picX, creditY2, termPicClearWidth, rowsToClear, 0x0);
	}

	if (creditY2 > 200)
		creditY2 = 200;

	oldY2 = creditY2;

	rowsToDraw = ck_currentTermPicHeight;
	picStartOffset = 0;

	if (creditY1 < 0)
	{
		// clipped by screen top
		picStartOffset = -creditY1 * (ck_currentTermPicWidth);
		rowsToDraw += creditY1;
		creditY1 = 0;
	}
	else if (creditY1 + ck_currentTermPicHeight > 200)
	{
		// clipped by screen bot
		rowsToDraw = 200 - creditY1;
	}

	if (rowsToDraw > 0)
	{
		uint8_t *plane_1 = (uint8_t *)(ck_currentTermPicSeg) + picStartOffset;
		uint8_t *plane_2 = plane_1 + ck_currentTermPicWidth * ck_currentTermPicHeight;

		VL_SetMapMask(0x4);
		VL_1bppToScreen_PM(plane_1, picX, creditY1, ck_currentTermPicWidth * 8, rowsToDraw, 0xF);
		VL_SetMapMask(0x8);
		VL_1bppToScreen_PM(plane_2, picX, creditY1, ck_currentTermPicWidth * 8, rowsToDraw, 0xF);
	}
}

// Does the terminator scrolling
void AnimateTerminator(void)
{

	int cmdrLeftEdgeFromScreenLeft;
	int delaytime;

	bool joyButtonPressedNow = false;
	bool joyButtonPressedPrev = false;

	// At the end of the terminator scrolling, left edge of the commander graphic is
	// this many pixels from the Left/Right edge of the Visible screen
	// (i.e., a negative value means an offset to the left)
	int finalCmdrPosFromScreenLeft;
	int finalCmdrPosFromScreenRight;

	// How far it's currently scrolled
	int elapsedCmdrScrollDist;

	// How many pixels the left edge of the screen is away from the left edge of the KEEN image
	int xpixel;

	// start of the visible area in the screen buffer
	unsigned screenofs;

	int elapsedTime;

	// The pixel modulo 8 that the left edge of the scrolling CMDR graphic is aligned to,
	// relative to the left edge of the buffer
	int cmdrBMPpelpan;

	// Which of the shifted "COMMANDER" graphics to use
	mm_ptr_t bmpsrcseg;

	// The distance from the left margin of the COMMANDER graphic that
	// should be drawn, measured in BYTES (px * 8)
	int cmdrLeftDrawStart;

	// variables of unknown type
	int screenWidthInPx, maxTime, leftMarginBlackWords, cmdrWords, rightMarginBlackWords, offscreenCmdrPixels, nextRowDist;

	ck_introScreenWidth = ck_introKeen->width + 25 * 8;

	finalCmdrPosFromScreenLeft = 120 - ck_introCommander->width;
	screenWidthInPx = 320;
	finalCmdrPosFromScreenRight = finalCmdrPosFromScreenLeft - screenWidthInPx;
	maxTime = abs(finalCmdrPosFromScreenRight);

	terminatorPageOn = terminatorOfs = 0;

	// delay for a tic before starting
	SD_SetLastTimeCount(SD_GetTimeCount());
	while (SD_GetLastTimeCount() == SD_GetTimeCount())
		VL_Yield(); // Keep CPU usage low

	SD_SetLastTimeCount(SD_GetTimeCount());

	// Update the terminator
	for (elapsedTime = 0; elapsedTime <= maxTime; elapsedTime += SD_GetSpriteSync())
	{

		// Reposition the left edge of the screen in direct proportionality to time elapsed
		xpixel = (ck_introScreenWidth * (maxTime - elapsedTime)) / maxTime;

		// Scroll the Credits graphics
		ScrollTerminatorCredits(elapsedTime, xpixel);

		elapsedCmdrScrollDist = screenWidthInPx + (finalCmdrPosFromScreenRight * elapsedTime) / maxTime;
		elapsedCmdrScrollDist += xpixel & 7;

		cmdrLeftEdgeFromScreenLeft = (elapsedCmdrScrollDist + 0x800) / 8 - 0x100;
		cmdrBMPpelpan = ((elapsedCmdrScrollDist + 0x800) & 7);

		bmpsrcseg = shiftedCmdrBMPsegs[cmdrBMPpelpan];
		cmdrLeftDrawStart = 0;

		// DOS: Move the screen start address
		// screenofs = terminatorOfs + xpixel/8;
		screenofs = xpixel / 8; // Omnispeak

		// Omnispeak: pan the screen to the appropriate spot over the KEEN surface
		VL_SetScrollCoords(xpixel, 0);

		if (cmdrLeftEdgeFromScreenLeft > 0)
		{
			// The COMMANDER graphic has not started to scroll off the left edge of the screen
			leftMarginBlackWords = (cmdrLeftEdgeFromScreenLeft + 1) / 2;

			if (cmdrLeftEdgeFromScreenLeft & 1)
				screenofs--;

			cmdrWords = 21 - leftMarginBlackWords;
			rightMarginBlackWords = 0;
		}
		else if (cmdrLeftEdgeFromScreenLeft > (42 - ck_introCommanderWidth))
		{
			// The COMMANDER graphic is clipped on both left and right edges
			leftMarginBlackWords = 0;
			cmdrWords = 21; // 42 * 8 pixels = 336
			rightMarginBlackWords = 0;
			cmdrLeftDrawStart -= cmdrLeftEdgeFromScreenLeft;
		}
		else
		{
			// The COMMANDER graphic is only clipped by the left side of the screen
			leftMarginBlackWords = 0;
			cmdrWords = (ck_introCommanderWidth + cmdrLeftEdgeFromScreenLeft) / 2;
			rightMarginBlackWords = 21 - cmdrWords;
			cmdrLeftDrawStart -= cmdrLeftEdgeFromScreenLeft;
		}

		offscreenCmdrPixels = ck_introCommanderWidth - (cmdrWords << 1);
		nextRowDist = TERMINATORSCREENWIDTH - ((leftMarginBlackWords + cmdrWords + rightMarginBlackWords) << 1);

		// Only draw to Plane 1 of EGA memory
		VL_SetMapMask(2);

		// Omnispeak:
		// Just draw the visible part of the COMMANDER surface atop the KEEN surface
		int rowptr = 0;
		for (int row = 0; row < 200; row++)
		{
			VL_ScreenRect_PM(screenofs * 8, row, leftMarginBlackWords * 16, 1, 0x0);
			VL_1bppToScreen_PM((uint8_t *)bmpsrcseg + rowptr + cmdrLeftDrawStart, screenofs * 8 + leftMarginBlackWords * 16, row, cmdrWords * 16, 1, 0xF);
			VL_ScreenRect_PM(screenofs * 8 + (leftMarginBlackWords + cmdrWords) * 16, row, rightMarginBlackWords * 16, 1, 0x0);

			if (row & 1)
			{
				rowptr += ck_introCommanderWidth;
			}
		}

		// In DOS, we handle the double-buffering here.

		// Update the screen
		VL_Present();

		IN_PumpEvents();
		// Delay
		do
		{
			delaytime = SD_GetTimeCount();
			SD_SetSpriteSync(delaytime - SD_GetLastTimeCount());
		} while (SD_GetSpriteSync() < 2);

		SD_SetLastTimeCount(delaytime);

		// Stop drawing if key pressed
		if (IN_CheckAck() /*IN_IsUserInput()*/ && IN_GetLastScan() == IN_SC_F1)
			IN_SetLastScan(IN_SC_Space);

		joyButtonPressedPrev = joyButtonPressedNow;
		joyButtonPressedNow = (IN_JoyPresent(0) && IN_GetJoyButtonsDB(0)) || (IN_JoyPresent(1) && IN_GetJoyButtonsDB(1));
		if (joyButtonPressedPrev && !joyButtonPressedNow)
			IN_SetLastScan(IN_SC_Space);

		if (IN_GetLastScan())
			return;
	}

	word_499C7 = xpixel / 8;
}

// RLE-Expands one line of monochrome terminator BMP data
// Each source line is a sequence of WORD data, terminated with an 0xFFFF flag
// Each line of BMP data is a sequence of uint16_t values that encodes alternating runs of black and white pixels,
//  always starting with BLACK pixels

void TerminatorExpandRLE(uint16_t *src, uint8_t *dest)
{
	// nextword = next source word
	// runlength = number of pixels of one color remaining, in one byte, before transition to the alternate color
	// lastbyte = byte to write at the transition between black and white
	uint16_t runlength, di, nextword;
	uint8_t lastbyte;

	runlength = lastbyte = 0;

	while ((nextword = *src++) != 0xFFFF)
	{
		nextword = CK_Cross_SwapLE16(nextword);
		// Expand a Black Run of pixels
		if ((runlength += nextword) > 7)
		{
			// write the transition byte
			*dest++ = lastbyte;

			// the leftmost pixels of the transition byte will be black
			lastbyte = 0;

			// write complete bytes
			di = runlength / 8 - 1;
			while (di--)
				*dest++ = 0;

			// count the leftover pixels on this last byte
			runlength &= 7;
		}

		// Check if we've hit a stop signal
		if ((nextword = *src++) == 0xFFFF)
		{
			*dest++ = lastbyte;
			*dest = 0;
			return;
		}
		nextword = CK_Cross_SwapLE16(nextword);

		// the lowest bits in this byte will be black (zero)
		// so we OR the right most remaining bits so that they are drawn white
		lastbyte |= terminator_blackmasks[runlength];

		// Expand a Light Run of pixels
		if ((runlength += nextword) > 7)
		{
			// write the transition byte
			*dest++ = lastbyte;

			// the leftmost pixels of the transition byte will be light
			lastbyte = 0xFF;

			// write runs of 8 light pixels
			di = runlength / 8 - 1;
			while (di--)
				*dest++ = 0xFF;

			// count the leftover light pixels on this last byte
			runlength &= 7;
		}

		// the lowest bites in this byte will be light ( set)
		// so we AND the rightmost remaining bits so that they are drawn black
		lastbyte &= terminator_lightmasks[runlength];
	}

	// write the final byte, and one padding byte of black
	*dest++ = lastbyte;
	*dest = 0;
}

// Generate an RLE-compressed bitmap with COMMANDER KEEN separated by some space
// Except... this RLE-compressed bitmap seems to be in a slightly different format
// than those stored in the game archive
//
// Here, alternating runlengths of pixels are not specified by their length, but by their start position
//
void JoinTerminatorPics(void)
{

	// Points to the data in the stitched bmp
	uint16_t *introBMPDataPtr;

	int i;

	// Allocate memory for the COMMANDER KEEN graphic
	MM_GetPtr(&introbuffer2, 30000);

	// Where we start writing the bitmap data
	introBMPDataPtr = (uint16_t *)((introbmptype *)(introbuffer2))->data;

	// for each row
	for (i = 0; i < 200; i++)
	{
		uint16_t count, inword;
		uint16_t *linestart;

		// Generate a pointer to this line of data
		((introbmptype *)introbuffer2)->linestarts[i] = (uint16_t)((uint8_t *)(introBMPDataPtr) - (uint8_t *)(introbuffer2));

		count = 0;

		linestart = (uint16_t *)((uint8_t *)ck_introCommander + ck_introCommander->linestarts[i]);
		inword = *linestart++;

		do
		{
			*introBMPDataPtr++ = count;
			count += inword;
			inword = *linestart++;

		} while (inword != 0xFFFF);

		// Add some space between the COMMANDER and the KEEN
		count += 80;

		linestart = (uint16_t *)((uint8_t *)(ck_introKeen) + ck_introKeen->linestarts[i]);
		linestart++;
		inword = *linestart++;

		do
		{

			*introBMPDataPtr++ = count;
			count += inword;
			inword = *linestart++;

		} while (inword != 0xFFFF);

		*introBMPDataPtr++ = count;
		*introBMPDataPtr++ = 0xFFFF;
	}
}

// Originally this function accepted a table of precomputed scaled values.
// Here we do the scaling ourselves, with 'scaleFactor' being
void ZoomOutTerminator_1(uint16_t *rleData, uint16_t arg4, int leftOffset, uint16_t scaleFactor)
{

	uint16_t inputOffset, scaledOffset;

	// X coord where the zoomed bitmap is clipped by the left edge of the screen
	int runBegin;

	unsigned runLength;

	int rightOffset = 320 - leftOffset;

	int vidOffset = 0;

	if (leftOffset < 0)
	{
		// The zooming bitmap is clipped by the left edge of the screen
		// So find out where we start drawing it

		runBegin = -leftOffset;

		do
		{

			inputOffset = *rleData++;
			scaledOffset = (inputOffset * scaleFactor) / 256;
			if (scaledOffset > runBegin)
			{
				goto writeWhiteRun;
			}

			inputOffset = *rleData++;
			scaledOffset = (inputOffset * scaleFactor) / 256;
			if (scaledOffset > runBegin)
			{
				goto writeBlackRun;
			}

		} while (1);
	}
	else
	{
		// loc_147C2:
		// The zooming bitmap is not clipped by left edge of screen
		runBegin = 0;
		rleData++;
		vidOffset = leftOffset;
		goto loc_147E5;
	}

writeWhiteRun:
	do
	{
		// Writing a run of pixels
		runLength = scaledOffset - runBegin;
		VL_ScreenRect(vidOffset, arg4, runLength, 1, 0xF);
		runBegin = scaledOffset;
		vidOffset += runLength;

		if (scaledOffset > rightOffset)
			return;

	loc_147E5:
		inputOffset = *rleData++;
		scaledOffset = (inputOffset * scaleFactor) / 256;

	writeBlackRun:

		// Writing a run of pixels
		runLength = scaledOffset - runBegin;
		VL_ScreenRect(vidOffset, arg4, runLength, 1, 0x0);
		runBegin = scaledOffset;
		vidOffset += runLength;

		if (scaledOffset > rightOffset)
			return;

		if ((inputOffset = *rleData++) != 0xFFFF)
			scaledOffset = (inputOffset * scaleFactor) / 256;
		else
			break;
	} while (1);

	// Write black until the end of the screen?
	runLength = 320 - vidOffset;
	VL_ScreenRect(vidOffset, arg4, runLength, 1, 0);

	return;
}

// The COMMANDER and KEEN RLE-Encoded bitmaps are first joined together into one big
// RLE-encoded bitmap, which is then scaled and translated to its final position.

// NOTE: I think that this still has errors, which are visible when setting maxTime to a large value
// But... it looks good enough at normal game speed
void ZoomOutTerminator(void)
{
	long startingLeftOffset;
	unsigned yBottom, scaledHeight;
	unsigned var20;
	unsigned var1C, si;

	// The px offset of the left edge of the bitmap from the left edge of the screen
	// (i.e., negative number means graphic is clipped by left edge of screen
	int leftOffset;

	unsigned elapsedTime, maxTime;
	uint16_t *var16;

	// finalHeight looks like final Height?
	uint16_t newTime, scaleFactor, finalHeight, varC, varE;

	// Set the palette
	VL_SetPaletteAndBorderColor(ck_terminator_palette2);

	// The starting (negative) offset of the COMMANDER graphic from the left edge of the screen
	startingLeftOffset = 120 - ck_introCommander->width;

	JoinTerminatorPics();

	scaleFactor = 256;
	finalHeight = 33;
	var20 = 200;
	elapsedTime = 1;
	maxTime = 30; // Set to large value to slow down the zoom

	// elapsedTime seems to be a timer, maxTime is the max time
	while (elapsedTime <= maxTime)
	{

		if (elapsedTime == maxTime)
		{
			// We're done
			scaleFactor = finalHeight;
			leftOffset = 0;
			yBottom = 4;
		}
		else
		{
			// These casts to long don't actually exist in disasm, but are required for
			// testing large values of maxtime
			scaleFactor = 256 - (((long)(256 - finalHeight) * (long)elapsedTime) / (long)maxTime);

			leftOffset = (startingLeftOffset * (long)(maxTime - elapsedTime)) / (long)maxTime;

			yBottom = (long)(elapsedTime * 4) / (long)maxTime;
		}

		if (elapsedTime == maxTime)
			leftOffset = 0;
		else
			leftOffset = ((long)(maxTime - elapsedTime) * startingLeftOffset) / (long)maxTime;

		scaledHeight = (200 * scaleFactor) >> 8;
		varC = 0;
		varE = 0x10000L / scaleFactor;

		if (yBottom > 0)
			VL_ScreenRect(0, 0, 320, yBottom, 0);

		// Draw each line to the screen
		for (si = 0; si < ((200 * scaleFactor) >> 8); si++)
		{
			var16 = (uint16_t *)((uint8_t *)introbuffer2 + ((introbmptype *)introbuffer2)->linestarts[varC >> 8]);
			ZoomOutTerminator_1(var16, si + yBottom, leftOffset, scaleFactor);
			varC += varE;
		}

		var1C = scaledHeight + yBottom;
		if (var20 > var1C)
		{
			VL_ScreenRect(0, var1C, 320, var20 - var1C + 1, 0);
			var20 = var1C;
		}

		IN_PumpEvents();
		VL_Present();

		newTime = SD_GetTimeCount();
		SD_SetSpriteSync(newTime - SD_GetLastTimeCount());
		SD_SetLastTimeCount(newTime);
		if (SD_GetSpriteSync() > 8)
			SD_SetSpriteSync(8);

		if (elapsedTime == maxTime)
			break;

		elapsedTime += SD_GetSpriteSync();

		if (elapsedTime > maxTime)
			elapsedTime = maxTime;

		if (/*IN_IsUserInput() &&*/ IN_GetLastScan() == IN_SC_F1)
			IN_SetLastScan(IN_SC_Space);

		if (IN_GetLastScan())
			// return;// should this be break instead? Want to free intro buffers!
			break;
	}

	MM_FreePtr(&introbuffer2);
}

// The Fizzlefade routine
// Operates by drawing the title graphic into offscreen video memory, then
// using the hardware to copy the source to the display area, pixel-by-pixel

// The same effect can be achieved in omnispeak by drawing the title screen
// to an new surface, then copying from the surface to the screen
void CK_FizzleFade()
{
	int i;

	uint16_t columns1[320];
	uint16_t rows1[200];

	// Construct a multiplication table for multiples of
	for (i = 0; i < 320; i++)
		columns1[i] = i;

	// Shuffle the table entries
	for (i = 0; i < 320; i++)
	{

		// NOTE: BCC rand() implementation is capped at 0x7FFF
		int16_t var2 = (320 * (rand() & 0x7FFF)) / 0x8000;

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
	VL_SetScrollCoords(0, 0);

	// DOS: Draw Title Bitmap offscreen
	// VW_DrawBitmap(0,0,PIC_TITLESCREEN);

	// SDL: Draw it to a new surface
	uint8_t *titleBuffer = (uint8_t *)VL_CreateSurface(320, 200);

	// FIXME: This is cached somewhere else
	CA_CacheGrChunk(PIC_TITLESCREEN);

	VH_BitmapTableEntry *dimensions = VH_GetBitmapTableEntry(PIC_TITLESCREEN - ca_gfxInfoE.offBitmaps);

	VL_UnmaskedToSurface(ca_graphChunks[PIC_TITLESCREEN], titleBuffer, 0, 0, dimensions->width * 8, dimensions->height);

	// Do the fizzling
	//
	for (i = 0; i < 360; i++)
	{
		int16_t var_10 = i - 160;

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
				// We use VL_ScreenRect here instead of VL_SurfaceToScreen() because
				// the EGA backend requires VL_SurfaceToScreen()'s x coordinate be divisble by 8.
				int pixelValue = VL_SurfacePGet(titleBuffer, x, y);
				VL_ScreenRect(x, y, 1, 1, pixelValue);
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

			VL_DestroySurface(titleBuffer);

			return;
		}
	}

	// Title screen is now drawn, wait for a bit

	// Enable bitmask across all planes
	// out(0x3CE, 0xFF08);

	// Write enable all memory planes
	// out(0x3C4, 0xF02);

	IN_UserInput(420, false);

	VL_DestroySurface(titleBuffer);
}

void CK_DrawTerminator(void)
{

	unsigned cmdrWidthX100;

	uint16_t *srcptr;
	uint8_t *destptr;

	int cmdrLineStarts[200];

	bool terminator_complete = false;

	VL_ResizeScreen(TERMINATORSCREENWIDTH * 8, 200);
	VL_ClearScreen(0);

	// Cache Intro Bitmaps
	CA_CacheGrChunk(PIC_TITLESCREEN);
	CA_CacheGrChunk(EXTERN_COMMANDER);
	CA_CacheGrChunk(EXTERN_KEEN);

	ck_introKeen = (introbmptype *)ca_graphChunks[EXTERN_KEEN];
	ck_introCommander = (introbmptype *)ca_graphChunks[EXTERN_COMMANDER];

	// Only writing to plane 0 of display memory
	VL_SetMapMask(1);

	// Because "KEEN" needs to end up on the right side of the screen,
	// an extra padding amount is added to the left side of the Keen graphic
	ck_introScreenWidth = ck_introKeen->width + 25 * 8;

	// Set the screen to the right of the "KEEN" graphic
	VL_SetScrollCoords(ck_introScreenWidth + 1, 0);

	// Copy each line of the KEEN graphic into video memory, accounting for the padding amount
	{
		uint8_t *destbuf = (uint8_t *)calloc((ck_introScreenWidth + 7) / 8 + 1, sizeof(uint8_t));
		for (int i = 0; i < 200; i++)
		{
			// Left margin of KEEN is 25*8 = 200 pixels

			// Omnispeak: RLE-Expand into temp buffer, then copy into video memory
			srcptr = (uint16_t *)((uint8_t *)ck_introKeen + ck_introKeen->linestarts[i]);
			TerminatorExpandRLE(srcptr, destbuf + 25);
			VL_1bppToScreen_PM(destbuf, 0, i, ck_introScreenWidth, 1, 0xF);
		}

		free(destbuf);
	}

	// In DOS, we copy the KEEN graphic to the second page, in Omnispeak
	// we only render to one page, so we don't do anything.

	// Allocate memory for 8 shifts of the COMMANDER graphic
	// Notice that there are only 100 rows of memory, which means that
	// each EVEN row of source graphics is duplicated

	// The most shifted version of the image will shifted 7px, taking
	// at most one extra byte. We + 7 px and then divide by 8 (rounding
	// down). We actually want to round up, though, so we add 1 to the
	// result.
	ck_introCommanderWidth = ((ck_introCommander->width + 7) / 8) + 1;
	// We then want to round up to the nearest 16-bit word.
	ck_introCommanderWidth = (ck_introCommanderWidth + 1) & 0xFFFE;

	cmdrWidthX100 = ck_introCommanderWidth * 100;
	for (int i = 0; i < 8; i++)
		MM_GetPtr(&shiftedCmdrBMPsegs[i], cmdrWidthX100);

	ck_introKeen = (introbmptype *)ca_graphChunks[EXTERN_KEEN];
	ck_introCommander = (introbmptype *)ca_graphChunks[EXTERN_COMMANDER];

	// Decompress the RLE-encoded "Commander" bitmap.
	for (int i = 0; i < 100; i++)
	{
		cmdrLineStarts[2 * i] = cmdrLineStarts[2 * i + 1] = i * ck_introCommanderWidth;

		srcptr = (uint16_t *)((uint8_t *)ck_introCommander + ck_introCommander->linestarts[i * 2]);
		destptr = (uint8_t *)shiftedCmdrBMPsegs[0] + cmdrLineStarts[2 * i];
		TerminatorExpandRLE(srcptr, destptr);
	}

	// Looks like we're making 8 shifts of the "Commander" graphic
	for (int i = 1; i < 8; i++)
	{
		uint8_t *last = (uint8_t *)shiftedCmdrBMPsegs[i - 1];
		uint8_t *next = (uint8_t *)shiftedCmdrBMPsegs[i];

		int c = 0;
		for (int j = 0; j < cmdrWidthX100; j++)
		{
			uint8_t d = *last++;
			*next++ = (d >> 1) | c;
			c = (d << 7);
		}
	}

	// Set the terminator palette
	ck_terminator_palette2[16] = vl_border_color;
	ck_terminator_palette1[16] = vl_border_color;

	VL_SetPaletteAndBorderColor(ck_terminator_palette1);

	// Do the terminator
	ck_currentTermPicStartTime = ck_currentTerminatorCredit = 0;
	ck_termCreditStage = -1;
	AnimateTerminator();

	// Omnispeak
	VL_ResizeScreen(21 * 16, 14 * 16);

	VL_SetScrollCoords(0, 0);

	// After the terminator text has run, keys are checked
	if (!IN_GetLastScan())
	{
		ZoomOutTerminator();
	}

	if (!IN_GetLastScan())
	{
		CK_FizzleFade();
		terminator_complete = true;
	}

	// Free the COMMANDER and KEEN bitmaps
	MM_SetPurge(ca_graphChunks + EXTERN_COMMANDER, 3);
	MM_SetPurge(ca_graphChunks + EXTERN_KEEN, 3);

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
	if ((ck_currentEpisode->ep != EP_CK6) && (IN_GetLastScan() == IN_SC_F1))
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

		VL_SetScrollCoords(0, 0);
		VL_Present();
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

const char *ck_storyText;

// Utilities for converting to the units used by the Star Wars scroller:
// 1/2048 of a pixel.
static const unsigned int SWunitsPerPixel = 2048;
static const unsigned int log2_SWunitsPerPixel = 11;
#define CK_SWunitsToPixels(sw) ((sw) >> log2_SWunitsPerPixel)
#define CK_PixelsToSWunits(px) ((px) << log2_SWunitsPerPixel)

// Table that adds a row-displacement when selecting a line of the master text image to draw,
// given a row on visible screen as the index
// Because the table values are skewed, this causes the text to bunch up at the top of the screen.
uint16_t ck_SWScreenRowToMasterRow[200];

// Table giving the distance (in SWunits) between two screen pixels in master pixel coordinates.
uint16_t ck_SWRowPixelDistance[200];

// Table giving the width of each row in screen pixels.
uint16_t ck_SWRowWidthInScreenPx[200];

// The total height of the master text, in master pixels.
int ck_starWarsTotalHeight;

// The master text screen for the Star Wars scroller.
void *ck_starWarsTextSurface;

// This replaces CompileSWUpdate in Keen (BuildScalers in CKSRCMOD).
// As omnispeak is not architecture specific, instead of generating scaling
// functions for each row, we'll simply determine the scaling parameters.
void CK_PrepareSWUpdate()
{
	// TODO: This is where BuildBitTables() would normally happen.
	// I still need to work out what that actually does, so leaving it for
	// now.

	// This is the width of the trapezoid at the bottom of the screen.
	uint32_t trapezoidWidth = CK_PixelsToSWunits(320);

	// This is the change in width per scanline.
	// The value here is such that the top of the screen will by ~40px.
	uint32_t delTrapWidth = 0xB33;

	// This is the Y-coordinate of where the current screen row maps to on
	// the master SW text surface. (In SWunits.)
	uint32_t currentMasterRow = 0;

	for (int row = 199; row >= 0; --row)
	{
		// Width of the row in pixels. We want this to be even, hence
		// the / 2 * 2 shenanigans.
		uint32_t rowWidth = CK_SWunitsToPixels(trapezoidWidth / 2) * 2;

		ck_SWRowWidthInScreenPx[row] = rowWidth;

		ck_SWScreenRowToMasterRow[row] = CK_SWunitsToPixels(currentMasterRow);

		// This is the distance between successive pixels (in SWunits).
		// It is also used as the distance between this row and the one above it.
		uint32_t rowDistance = CK_PixelsToSWunits(336) / rowWidth;

		ck_SWRowPixelDistance[row] = rowDistance;

		currentMasterRow += rowDistance;

		// Shrink the scale distance for the next line.
		trapezoidWidth -= delTrapWidth;
	}
}

// Converts a string from ASCII to Star Wars font indices.
// Note that the DOS version acted on strings in-place. For omnispeak, we write
// into another string. (It can still be used in place if input == output).
void CK_TranslateString(const char *input, char *output)
{
	while (*input)
	{
		char c = *input;
		if (c >= 'A' && c <= 'Z')
			c -= 33;
		else if (c >= 'a' && c <= 'z')
			c -= 39;
		else if (c == '.')
			c = 0x54;
		else if (c == ',')
			c = 0x55;
		else if (c == '-')
			c = 0x56;
		else if (c == '"')
			c = 0x57;
		else if (c == ' ')
			c = 0x58;
		else if (c == '!')
			c = 0x59;
		else if (c == '\'')
			c = 0x5A;
		else if (c != '\n')
			c = 0x54;
		*output = c;
		input++;
		output++;
	}
}

// This is PrintStarWars in CKSRCMOD.
// We print the story text to an offscreen buffer, which we'll later scale
// invidual rows from to produce the final trapezoidal text.
void CK_DrawSWText()
{
	char currentTextLine[81];
	const char *storyTextIndex = CK_VAR_GetStr("ck_str_storyText");

	// We want to render to a 336px wide texture.
	US_SetWindowX(0);
	US_SetWindowW(336);

	// We start from row 1, as row 0 is used as an empty row for rendering
	// otherwise out-of-bounds rows.
	US_SetPrintY(1);
	US_SetPrintColour(0xF);

	// TODO: Calculate the height of the text. Keen will happily use as much
	// video memory as is required, but here we guess that 1024 px will be
	// sufficient. (It is for the original games). Eventually, we'll measure
	// the number of newlines to calculate this.
	ck_starWarsTextSurface = VL_CreateSurface(336, 1024);

	// We want to render to this offscreen buffer.
	void *oldScreen = VL_SetScreen(ck_starWarsTextSurface);
	VL_ClearScreen(0);

	// Because we need to draw more than a screen-height's worth of text, we
	// can't naïvely use US_CPrint(), as that uses the buffered drawing by
	// default, which only has a screen-sized buffer. The original game
	// works around this by changing the buffer offset, but this seems
	// simpler, particularly since we don't allow similar things with our
	// graphics surfaces.
	US_SetPrintRoutines(NULL, VH_DrawPropString);

	while (*storyTextIndex)
	{
		char *lineIndex = currentTextLine;
		char ch;

		// Extract one line of text.
		do
		{
			ch = *storyTextIndex;
			*lineIndex = ch;
			if (ch)
				storyTextIndex++;
			lineIndex++;
		} while (ch && ch != '\n');
		// ...and null-terminate it.
		*lineIndex = '\0';

		// Translate from ASCII to SW font indices.
		CK_TranslateString(currentTextLine, currentTextLine);

		// And print it centered in its line.
		US_CPrint(currentTextLine);

		// Note that the original game resets its buffer offset here
		// and incrementally calculates the height. We just let
		// US_CPrint() increase the PrintY coordinate for us.
	}

	// Save off the total height of the scroller.
	ck_starWarsTotalHeight = US_GetPrintY();
	US_SetPrintY(0);
	US_SetPrintRoutines(NULL, NULL);

	// Restore the screen.
	VL_SetScreen(oldScreen);
}

// This is StarWarsLoop in CKSRCMOD.
void CK_ScrollSWText()
{
	SD_SetLastTimeCount(0);
	SD_SetTimeCount(0);
	SD_SetSpriteSync(0);
	// We draw the text on plane 4.
	VL_SetMapMask(8);

	uint16_t scrollDistance = 0;

	while (scrollDistance <= ck_starWarsTotalHeight + 400)
	{
		// Update rows from the bottom.

		for (int row = 199; row >= 0; --row)
		{
			int masterRowToDraw = scrollDistance - ck_SWScreenRowToMasterRow[row];

			// If it's out of range, set it to our reserved blank row 0.
			if (masterRowToDraw < 0 || masterRowToDraw >= ck_starWarsTotalHeight)
				masterRowToDraw = 0;

			int rowStart = 160 - ck_SWRowWidthInScreenPx[row] / 2;
			int rowEnd = 160 + ck_SWRowWidthInScreenPx[row] / 2;

			// The x-coordinate of the current pixel in the offscreen
			// text buffer. Measured in SWunits.
			int masterX = 0;

			// Start drawing.
			// For each screen (destination) pixel, we calulate the
			// "master" pixel coordinates in the offscreen buffer,
			// read the pixel from it, and then mask it to the
			// screen in plane 4.
			for (int screenX = rowStart; screenX < rowEnd; ++screenX)
			{
				int masterPixelX = CK_SWunitsToPixels(masterX);
				int pixelValue = VL_SurfacePGet(ck_starWarsTextSurface, masterPixelX, masterRowToDraw);

				VL_ScreenRect_PM(screenX, row, 1, 1, pixelValue);
				masterX += ck_SWRowPixelDistance[row];
			}
		}

		IN_PumpEvents();
		VL_Present();

		uint32_t newTime = SD_GetTimeCount();
		SD_SetSpriteSync(SD_GetSpriteSync() + newTime - SD_GetLastTimeCount());
		SD_SetLastTimeCount(newTime);

		if (SD_GetSpriteSync() > 20)
			SD_SetSpriteSync(20);

		scrollDistance += SD_GetSpriteSync() / 4;
		SD_SetSpriteSync(SD_GetSpriteSync() % 4);

		if (IN_GetLastScan() == IN_SC_F1)
			IN_SetLastScan(IN_SC_Space);

		if (IN_GetLastScan())
			return;
	}
}

void CK_DrawStarWars()
{
	// Keen5 sets the palette to the default one here.
	VL_ClearScreen(0);
	VL_SetScrollCoords(0, 0);

	CA_SetGrPurge();
	// Cache and set the Star Wars font.
	CA_CacheGrChunk(5);
	US_SetPrintFont(2);
	// Render out the story text (to an offscreen buffer?)
	CK_DrawSWText();
	// Restore the font.
	US_SetPrintFont(0);

	CA_CacheGrChunk(PIC_STARWARS); // Story bkg image.

	// Keen draws this to a separate surface, for fast copies.
	VH_DrawBitmap(0, 0, PIC_STARWARS);

	VL_SetPalette(ck_starWarsPalette);

	// At this point, Keen generates a set of buffers full of machine code,
	// one per line, which scale the text (from the surface mentioned above)
	// to make the "Star Wars" effect. (BuildScalers/CompileSWUpdate)

	// Instead, we just precalculate the various scaling factors.
	CK_PrepareSWUpdate();

	// ...and scroll the text!
	if (!IN_GetLastScan())
	{
		StartMusic(ck_currentEpisode->starWarsSongLevel);
		CK_ScrollSWText();
		StopMusic();
	}

	VL_DestroySurface(ck_starWarsTextSurface);

	VL_ClearScreen(0);
	VL_SetScrollCoords(0, 0);
	VL_SetDefaultPalette();
	CA_ClearMarks();

	CK_HandleDemoKeys();
}

void CK_ShowTitleScreen()
{
	// scrollofs = 0;
	CA_CacheGrChunk(PIC_TITLESCREEN);
	VH_DrawBitmap(0, 0, PIC_TITLESCREEN);
	// Draw to offscreen buffer and copy?
	// VW_SetScreen(0,bufferofs_0);
	VL_SetScrollCoords(0, 0);
	// VWL_ScreenToScreen(bufferofs, bufferofs_0, 42, 224);
	VL_Present();
	IN_UserInput(420, false);
	CA_ClearMarks();
	CK_HandleDemoKeys();
	VL_Present();
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
	uint16_t demoLen = CK_Cross_SwapLE16(*((uint16_t *)demoBuf));
	demoBuf += 2;

	ck_gameState.currentLevel = demoMap;

	IN_DemoStartPlaying(demoBuf, demoLen);

	CK_LoadLevel(true, false);

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
	uint16_t demoLen = CK_Cross_SwapLE16(*((uint16_t *)demoBuf));
	demoBuf += 2;

	ck_gameState.currentLevel = demoMap;

	IN_DemoStartPlaying(demoBuf, demoLen);

	CK_LoadLevel(true, false);

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
void CK_OverlayHighScores()
{
	// Omnispeak
	int topMargin = ck_currentEpisode->highScoreTopMargin;
	int rightMargin = ck_currentEpisode->highScoreRightMargin;
	int leftMargin = ck_currentEpisode->highScoreLeftMargin;

	RF_Reposition(0, 0);

	// DOS: Set the back buffer to the master tilebuffer
	// The print routines draw to the backbuffer
	// oldbufferofs = bufferofs;
	// bufferofs = masterofs;

	// Simulate this in Omnispeak by replacing the tilebuffer surface
	// for the screen surface

	void *screen = VL_SetScreen(rf_tileBuffer);

	US_SetPrintRoutines(NULL, VH_DrawPropString);

	if (ck_currentEpisode->ep == EP_CK5)
		US_SetPrintColour(12);

	for (int entry = 0; entry < 8; entry++)
	{
		// Print the name
		US_SetPrintY(16 * entry + topMargin);
		US_SetPrintX(leftMargin);
		US_Print(ck_highScores[entry].name);

		// Keen 4: print the councilmembers rescued
		if (ck_currentEpisode->ep == EP_CK4)
		{
			US_SetPrintX(0x98);
			for (int i = 0; i < ck_highScores[entry].arg4; i++)
			{
				VH_DrawTile8(US_GetPrintX(), US_GetPrintY() + 1, 0x47);
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
		US_SetPrintX(rightMargin - w);
		US_Print(buf);
	}

	US_SetPrintRoutines(NULL, NULL);
	US_SetPrintColour(15);

	// restore the backbuffer
	VL_SetScreen(screen);
}

// Enter name if a high score has been achieved
static bool ck_highScoresDirty;
void CK_SubmitHighScore(int score, uint16_t arg_4)
{
	// Omnispeak
	int topMargin = ck_currentEpisode->highScoreTopMargin;
	int leftMargin = ck_currentEpisode->highScoreLeftMargin;

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
		for (int e = 8; --e > entry;)
			memcpy(&ck_highScores[e], &ck_highScores[e - 1], sizeof(newHighScore));

		memcpy(&ck_highScores[entry], &newHighScore, sizeof(newHighScore));
		entryRank = entry;
		ck_highScoresDirty = true;

		break;
	}

	if (entryRank != -1)
	{
		ck_inHighScores = true;
		ck_gameState.currentLevel = ck_currentEpisode->highScoreLevel;
		CK_LoadLevel(true, false);
		CK_OverlayHighScores();
		if (ck_currentEpisode->ep == EP_CK5)
			US_SetPrintColour(12);

		// FIXME: Calling these causes segfault
		RF_Refresh();
		RF_Refresh();

		US_SetPrintY(entry * 16 + topMargin);
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
