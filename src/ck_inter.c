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
#include "id_in.h"
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

typedef struct introbmptypestruct {
	uint16_t height, width;
	uint16_t linestarts[200];
	uint8_t data[];
} introbmptype;

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
int  ck_currentTermPicTimeRemaining;
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
uint16_t terminatorOfs;	 	// Start of the screen buffer; the KEEN graphic is aligned to the left edge of this
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
void ZoomOutTerminator_1(uint16_t *arg0, uint16_t arg4, int leftOffset, uint16_t *arg8);
void ZoomOutTerminator(void);
void CK_FizzleFade(void);


// Caches vertically scrolling terminator pics
// and unsignedly extends each uint8_t of the pic to a uint16_t

// Presumably this is because we're drawing in a different graphics mode?
void DoubleSizeTerminatorPics(int pic)
{

	uint8_t *picptr;
	uint16_t *double_picptr;
	int width, height;
	int i, size;
	int picchunk, pictableindex;
  VH_BitmapTableEntry bmp;

	// Cache the standard-sized picture
	picchunk = *terminator_pics[pic];
	CA_CacheGrChunk(picchunk);

	// Allocate memory for the expanded picture
	// and save its dimensions
  bmp = VH_GetBitmapTableEntry(picchunk - ca_gfxInfoE.offBitmaps);

	size = bmp.width * bmp.height * 2;
	MM_GetPtr(&terminator_pic_memptrs[pic], size * 2);

	// Copy the data to the expanded buffer
	picptr = (uint8_t *)ca_graphChunks[picchunk];
	double_picptr = (uint16_t *)terminator_pic_memptrs[pic];

	for (i = 0; i < size; i++)
		*double_picptr++ = *picptr++ * 2;

	// Free the standard-sized picture
	MM_FreePtr(&ca_graphChunks[picchunk]);
}

// Returns the Y-position of the terminator credit
int AdvanceTerminatorCredit(int elapsedTime)
{

	int bx;

	switch(ck_termCreditStage)
	{

	case -1:
		// loading
		ck_currentTermPicSeg = terminator_pic_memptrs[ck_currentTerminatorCredit];
		ck_currentTermPicWidth = ck_termPicWidths[ck_currentTerminatorCredit];
		ck_currentTermPicHalfWidth = (ck_currentTermPicWidth+3)>>1;
		ck_currentTermPicHeight = ck_termPicHeights[ck_currentTerminatorCredit];
		// ck_currentTermPicNextLineDist = linewidth - (ck_currentTermPicWidth+1);
		ck_currentTermPicSize = (ck_currentTermPicWidth * ck_currentTermPicHeight) << 1;
		ck_termCreditStage++;

		ck_currentTermPicStartTime = elapsedTime;
		ck_currentTermPicTimeRemaining = 240;
		// no break, flow into case 0

	case 0:

		// Flying up from bottom of screen
		ck_currentTermPicTimeRemaining -= (elapsedTime - ck_currentTermPicStartTime)<<1;

		if (ck_currentTermPicTimeRemaining < 100)
		{
			ck_currentTermPicTimeRemaining = 100;
			ck_termCreditStage++;
		}

		ck_currentTermPicStartTime = elapsedTime;

		return ck_currentTermPicTimeRemaining - (ck_currentTermPicHeight>>1);


	case 1:

		// Pausing in the middle
		if (elapsedTime - ck_currentTermPicStartTime > 200)
		{
			ck_termCreditStage++;
			ck_currentTermPicStartTime = elapsedTime;
		}

		return 100 - (ck_currentTermPicHeight>>1);

	case 2:

		// Flying up and out of the top of the screen
		ck_currentTermPicTimeRemaining -= (elapsedTime - ck_currentTermPicStartTime)<<1;

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

		return ck_currentTermPicTimeRemaining - (ck_currentTermPicHeight>>1);

	default:
		break;

	}

		return -40;
}

void ScrollTerminatorCredits(uint16_t elapsedTime, uint16_t xpixel)
{
	int pelpan, var4, numrows,var10;

	// Vars are static because BP is used during ASM draw routine
	static int rowsToDraw;


	int creditY1, varA, creditY2, var12, varE;

	pelpan = xpixel & 7;
	var4 = /*terminatorOfs +*/ (xpixel>>3) + (20 - ck_currentTermPicWidth/2);

	// EGAMAPMASK(0xC);
  VL_SetMapMask(0xC);

	creditY1 = AdvanceTerminatorCredit(elapsedTime);
	creditY2 = creditY1 + ck_currentTermPicHeight;

	if (creditY2 < 0)
		creditY2 = 0;

	varA = word_46CA2[terminatorPageOn];

	// Erasing the area underneath the credit
	if (creditY2 < 200 && varA > creditY2)
	{
		numrows = varA - creditY2;
		// var10 = ylookup[creditY2] + var4;

#if 0
asm		mov es, screenseg;
asm		mov bx, linewidth;				// bx = next line distance
asm		sub bx, ck_currentTermPicHalfWidth;
asm		sub bx, ck_currentTermPicHalfWidth;
asm		mov di, var10;
asm		mov dx, numrows;
asm		mov si, ck_currentTermPicHalfWidth;
asm		xor ax, ax;


nextrow1:
asm		mov cx, si;
asm		rep stosw;
asm		add di, bx;
asm		dec dx;
asm		jnz nextrow1;
#endif

    // Omnispeak: Just draw an empty rectangle over the area
    VL_ScreenRect_PM(var4 + pelpan, 0, ck_currentTermPicHalfWidth*16, 200, 0xF);
    VL_ScreenRect_PM(var4 + pelpan, creditY1, ck_currentTermPicHalfWidth*16, numrows, 0x0);
	}

	// loc_140AB
	if (creditY2 > 200)
		creditY2 = 200;

	word_46CA2[terminatorPageOn] = creditY2;

	rowsToDraw = ck_currentTermPicHeight;
	varE = 0;

	if (creditY1 < 0)
	{
		// clipped by screen top
		varE = -creditY1 * (ck_currentTermPicWidth<<1);
		rowsToDraw += creditY1;
		creditY1 = 0;
	}
	else if (creditY1 + ck_currentTermPicHeight > 200)
	{
		// clipped by screen bot
		rowsToDraw = 200 - creditY1;
	}

#if 0

	// loc_14108
	word_499CB = varE + ck_currentTermPicSize;

	if (rowsToDraw > 0)
	{
		word_499D1 = shifttabletable[pelpan];

		word_46CB0 = ylookup[creditY1] + var4;

asm		mov bx, varE;
asm		push bp;
asm		mov bp, word_499D1;
asm		mov es, screenseg;
asm		mov ds, ck_currentTermPicSeg;
asm		mov ah, 4;
asm		mov ss:terminatorPlaneOn, ah;

nextplane:
asm		mov dx, SC_INDEX;
asm		mov al, SC_MAPMASK;
asm		out dx, ax;
asm		mov dx, ss:rowsToDraw;
asm		mov di, ss:word_46CB0;

nextrow:
asm		mov cx, ss:ck_currentTermPicWidth;
asm		xor al, al;

nextX:
asm		mov si, [bx];
asm		add bx, 2;
asm		xor ah, ah;
asm		or ax, [bp+si];
asm		stosb;
asm		mov al, ah;
asm		loop nextX;


asm		stosb;
asm		mov uint16_t ptr es:[di], 0;
asm		add di, ss:ck_currentTermPicNextLineDist;
asm		dec dx;
asm		jnz nextrow;


asm		mov bx, ss:word_499CB;
asm		mov ah, ss:terminatorPlaneOn;
asm		shl ah, 1;
asm		mov ss:terminatorPlaneOn, ah;
asm		cmp ah, 0x10;
asm		jnz nextplane;

asm		pop bp;				// Restore stack
asm		mov ax, ss;
asm		mov ds, ax;


	}

#endif
}


// Does the terminator scrolling
void AnimateTerminator(void)
{

	int cmdrLeftEdgeFromScreenLeft;
	int delaytime;

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

	// EGAWRITEMODE(0);
	// EGAREADMAP(0);
	ck_introScreenWidth = ck_introKeen->width + 25 * 8;

	// EGAREADMAP(1);

	finalCmdrPosFromScreenLeft = 120 - ck_introCommander->width;
	screenWidthInPx = 320;
	finalCmdrPosFromScreenRight = finalCmdrPosFromScreenLeft - screenWidthInPx;
	maxTime = abs(finalCmdrPosFromScreenRight);

	terminatorPageOn = terminatorOfs = 0;

	// delay for a tic before starting
	SD_SetTimeCount(SD_GetTimeCount());
	while (SD_GetLastTimeCount() == SD_GetTimeCount())
		;

	SD_SetLastTimeCount(SD_GetTimeCount());

	// Update the terminator
	for (elapsedTime = 0; elapsedTime <= maxTime; elapsedTime += SD_GetSpriteSync())
	{

		// Reposition the left edge of the screen in direct proportionality to time elapsed
		xpixel = (ck_introScreenWidth*(maxTime-elapsedTime)) / maxTime;

		// Scroll the Credits graphics
		ScrollTerminatorCredits(elapsedTime, xpixel);

		elapsedCmdrScrollDist = screenWidthInPx + (finalCmdrPosFromScreenRight * elapsedTime)/ maxTime;
		elapsedCmdrScrollDist += xpixel&7;

		cmdrLeftEdgeFromScreenLeft = (elapsedCmdrScrollDist + 0x800) / 8 - 0x100;
		cmdrBMPpelpan = ((elapsedCmdrScrollDist + 0x800) & 7);

		bmpsrcseg = shiftedCmdrBMPsegs[cmdrBMPpelpan];
		cmdrLeftDrawStart = 0;

    // DOS: Move the screen start address
    // screenofs = terminatorOfs + xpixel/8;
    screenofs = xpixel/8;  // Omnispeak

     // Omnispeak: pan the screen to the appropriate spot over the KEEN surface
    VL_SetScrollCoords(xpixel, 0);

		if (cmdrLeftEdgeFromScreenLeft > 0)
		{
			// The COMMANDER graphic has not started to scroll off the left edge of the screen
			leftMarginBlackWords = (cmdrLeftEdgeFromScreenLeft+1)/2;

			if (cmdrLeftEdgeFromScreenLeft & 1)
				screenofs--;

			cmdrWords = 21 - leftMarginBlackWords;
			rightMarginBlackWords = 0;
		}
		else if (cmdrLeftEdgeFromScreenLeft > (320/8 - ck_introCommanderWidth))
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
			cmdrWords = (ck_introCommanderWidth+cmdrLeftEdgeFromScreenLeft)/2;
			rightMarginBlackWords = 21 - cmdrWords;
			cmdrLeftDrawStart -= cmdrLeftEdgeFromScreenLeft;
		}

		// loc_1445B:
		offscreenCmdrPixels = ck_introCommanderWidth - (cmdrWords<<1);
		nextRowDist = TERMINATORSCREENWIDTH - ((leftMarginBlackWords + cmdrWords + rightMarginBlackWords) << 1);

    // DOS: Only draw to Plane 1 of EGA memory
		// EGAMAPMASK(2);
    VL_SetMapMask(2);

#if 0
asm		mov di, screenofs;
asm		mov es, screenseg;
asm		mov si, cmdrLeftDrawStart;
asm		mov word_499FF, si;
asm		mov ds, bmpsrcseg;

// 		For each row
asm		mov dx, 200;

loopb0:
// Write black pixels to the left of COMMANDER
asm		xor ax, ax;
asm		mov cx, leftMarginBlackWords;
asm		rep stosw;

// Write COMMANDER bmp data
asm		mov cx, cmdrWords;
asm		rep movsw;

// Write black pixels to the right of COMMANDER
asm		xor ax, ax;
asm		mov cx, rightMarginBlackWords;
asm		rep stosw;


asm		test dx, 1;
asm		jnz odd;

even:
asm		mov si, ss:word_499FF;
asm		jmp ahead;

odd:
asm		add si, offscreenCmdrPixels;
asm		mov ss:word_499FF, si;

ahead:
asm		add di, nextRowDist;
asm		dec dx;
asm		jnz loopb0;


asm		mov ax, ss;
asm		mov ds, ax;
#endif

#if 0
// This is a rewrite of the Assembly code above
uint16_t *screenptr, *srcptr;

// screenptr =  ...
// srcptr = ...

// register shortage necessitates static declaration in DOS
// static uint16_t *word_499FF = ...;

for (int row = 200; row > 0; row--)
{
  for (int i = leftMarginBlackWords; i > 0; i--)
    *screenptr++ = 0;

  for (int i = cmdrWords; i > 0; i--)
    *screenptr++ = *srcptr++;

  for (int i = rightMarginBlackWords; i > 0; i--)
    *screenptr++ = 0;

  if (row&1)
  {
    srcptr += offscreenCmdrPixels;
    word_499FF = srcptr;
  }
  else
  {
    srcptr = word_499FF;
  }

  screenptr += nextRowDist;
}
#endif

// Omnispeak:
// Just draw the visible part of the COMMANDER surface atop the KEEN surface
int rowptr = 0;
for (int row = 0; row < 200; row++)
{

  VL_ScreenRect_PM(screenofs*8, row, leftMarginBlackWords*16, 1, 0x0);
  VL_1bppToScreen_PM((uint8_t*)bmpsrcseg+rowptr+cmdrLeftDrawStart, screenofs*8+leftMarginBlackWords*16, row, cmdrWords*16, 1, 0xF);
  VL_ScreenRect_PM(screenofs*8+(leftMarginBlackWords+cmdrWords)*16, row, rightMarginBlackWords*16, 1, 0x0);

  if (row&1)
  {
    rowptr += ck_introCommanderWidth;//offscreenCmdrPixels;
  }
  else
  {
    rowptr = rowptr;
  }

}



#if 0
		// DOS: Flip to the back page while panning the screen to the left
		// VW_SetScreen(xpixel/8 + terminatorOfs,xpixel&7);

		// Set the new back page
		if (terminatorPageOn ^=1)
			terminatorOfs = TERMINATORSCREENWIDTH/2;
		else
			terminatorOfs = 0;
#endif

      // Omnispeak: Update the screen
      VL_Present();


		IN_PumpEvents();
		// Delay
		do {
			delaytime = SD_GetTimeCount();
      SD_SetSpriteSync(delaytime - SD_GetLastTimeCount());
		} while (SD_GetSpriteSync() < 2);

  SD_SetLastTimeCount(delaytime);


		// Stop drawing if key pressed
		if (IN_CheckAck() /*IN_IsUserInput()*/ && IN_GetLastScan() == IN_SC_F1)
			IN_SetLastScan(IN_SC_Space);

		if (IN_GetLastScan())
			return;
	}

	word_499C7 = xpixel/8;

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
		// Expand a Black Run of pixels
		if ((runlength += nextword) > 7)
		{
			// write the transition byte
			*dest++ = lastbyte;

			// the leftmost pixels of the transition byte will be black
			lastbyte = 0;

			// write complete bytes
			di = runlength/8-1;
			while (di--)
				*dest++ = 0;

			// count the leftover pixels on this last byte
			runlength &= 7;
		}

		// loc_145CC
		// Check if we've hit a stop signal
		if ((nextword = *src++) == 0xFFFF)
		{
			*dest++ = lastbyte;
			*dest = 0;
			return;
		}

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
			di = runlength/8-1;
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
#if 0

// Generate an RLE-compressed bitmap with COMMANDER KEEN separated by some space
// Except... this RLE-compressed bitmap seems to be in a slightly different format
// than those stored in the game archive
//
// Here, alternating runlengths of pixels are not specified by their length, but by their start position
//
void JoinTerminatorPics(void)
{

	// Points to the data in the stitched bmp
	uint16_t far *introBMPDataPtr;

	int i;

	// Allocate memory for the COMMANDER KEEN graphic
	MM_GetPtr(&introbuffer2, 30000);

	// Where we start writing the bitmap data
	introBMPDataPtr = MK_FP(introbuffer2, offsetof(introbmptype, data));

	// for each row
	for (i = 0; i < 200; i++)
	{
		uint16_t count, inword;
		uint16_t far *linestart;

		// Generate a pointer to this line of data
		((introbmptype far *)introbuffer2)->linestarts[i] = FP_OFF(introBMPDataPtr);

		count = 0;
		EGAREADMAP(1);

		linestart = MK_FP(ck_introCommander, ck_introCommander->linestarts[i]);
		inword = *linestart++;

		do
		{
			*introBMPDataPtr++ = count;
			count += inword;
			inword = *linestart++;

		} while (inword != 0xFFFF);

		// Add some space between the COMMANDER and the KEEN
		count += 80;

		EGAREADMAP(0);


		linestart = MK_FP(ck_introKeen, ck_introKeen->linestarts[i]);
		linestart++;
		inword = *linestart++;

		do {

			*introBMPDataPtr++ = count;
			count += inword;
			inword = *linestart++;

		} while (inword != 0xFFFF);

		*introBMPDataPtr++ = count;
		*introBMPDataPtr++ = 0xFFFF;

	}
}

//ZoomOutTerminator_1(var16, si+yBottom, leftOffset, introbuffer);
void ZoomOutTerminator_1(uint16_t far *arg0, uint16_t arg4, int leftOffset, uint16_t far *arg8)
{

	uint16_t si;


	// X coord where the zoomed bitmap is clipped by the left edge of the screen
	int leftclip;

	int dx, var8;

	uint8_t far *vidPtr; // Pointer into video memory
	uint8_t writebyte;

	unsigned pixelrun;  // The width?
	unsigned varA;

	writebyte = pixelrun = 0;

	var8 = 320-leftOffset;

	vidPtr = MK_FP(0xA000 , (ylookup[arg4] + terminatorOfs + word_499C7));

	if (leftOffset < 0)
	{
		// The zooming bitmap is clipped by the left edge of the screen
		// So find out where we start drawing it

		leftclip = -leftOffset;
		writebyte = pixelrun = 0;

		do {

			si = *arg0++;
			dx = *(arg8 + si);
			if ( dx > leftclip)
				goto loc_14852;

			si = *arg0++;
			dx = *(arg8 + si);
			if ( dx > leftclip)
				goto loc_147FB;

		} while (1);
	}
	else
	{
		// loc_147C2:
		// The zooming bitmap is not clipped by left edge of screen
		writebyte = 0;
		pixelrun = leftOffset&7;
		vidPtr += leftOffset/8;
		leftclip = 0;
		arg0++;
		goto loc_147E5;
	}


loc_14852:
	do
	{
		// Writing a run of pixels
		varA = dx - leftclip;
		leftclip = dx;

		// get the run of pixels for the first byte
		writebyte |= terminator_blackmasks[pixelrun];

		if ((pixelrun += varA) > 7)
		{

			int cx;
			// write the first byte
			*vidPtr++ = writebyte;

			// write bytes of pixels
			writebyte = 0xFF;
			for (cx = pixelrun/8 - 1; cx; cx--)
				*vidPtr++ = 0xFF; // = writebyte;

			// remaining pixels to write
			pixelrun &= 7;
		}

		if (dx > var8)
			return;

		writebyte &= terminator_lightmasks[pixelrun];

loc_147E5:
		si = *arg0++;
		dx = *(arg8 + si);

loc_147FB:

		// Writing a run of pixels
		varA = dx - leftclip;
		leftclip = dx;
		if ((pixelrun += varA)> 7)
		{

			int cx;
			// write the first byte
			*vidPtr++ = writebyte;

			// write bytes of pixels
			writebyte = 0;
			for (cx = pixelrun/8 - 1; cx; cx--)
				*vidPtr++ = 0; // = writebyte;

			// remaining pixels to write
			pixelrun &= 7;

		}

		if (dx > var8)
			return;

		if ((si = *arg0++) != 0xFFFF)
			dx = *(arg8 + si);
		else
			break;
	} while (1);

	// Write black until the end of the screen?
	varA = 320 - leftclip;
	if ((pixelrun += varA) > 7)
	{

		int cx;
		// write the first byte
		*vidPtr++ = writebyte;

		// write bytes of pixels
		writebyte = 0;
		for (cx = pixelrun/8 - 1; cx; cx--)
		       *vidPtr++ = 0; // = writebyte;

		// remaining pixels to write
		pixelrun &= 7;
		// return;
	}

	return;

}

// The COMMANDER and KEEN RLE-Encoded bitmaps are first joined together into one big
// RLE-encoded bitmap, which is then scaled and translated to its final position.

// NOTE: I think that this still has errors, which are visible when setting maxTime to a large value
// But... it looks good enough at normal game speed
void ZoomOutTerminator(void)
{
	long startingLeftOffset;
	unsigned yBottom, var18;
	unsigned var20[2];
	unsigned var1C, si;

	// The px offset of the left edge of the bitmap from the left edge of the screen
	// (i.e., negative number means graphic is clipped by left edge of screen
	int leftOffset;

	unsigned elapsedTime, maxTime;
	uint16_t far* var16;

	// finalHeight looks like final Height?
	uint16_t newTime, var8, finalHeight, varC, varE;


	// Set the palette
asm	mov     ax, ds;
asm	mov	es, ax;
asm	mov	dx, offset ck_terminator_palette1;
asm	mov	ax, 0x1002;
asm 	int	0x10;

	EGAREADMAP(1);

	// The starting (negative) offset of the COMMANDER graphic from the left edge of the screen
	startingLeftOffset = 120 - ck_introCommander->width;

	JoinTerminatorPics();


	MM_GetPtr((memptr *)&introbuffer, 5000);

	var8 = 256;
	finalHeight = 33;
	var20[0] = var20[1] = 200;
	elapsedTime = 1;
	maxTime = 30; //3000; // Set to large value to slow down the zoom


	// elapsedTime seems to be a timer, maxTime is the max time
	while (elapsedTime <= maxTime)
	{

		if (elapsedTime == maxTime)
		{
			// We're done
			var8 = finalHeight;
			leftOffset = 0;
			yBottom = 4;
		}
		else
		{
			// These casts to long don't actually exist in disasm, but are required for
			// testing large values of maxtime
			var8 = 256 - (((long)(256 - finalHeight) * (long)elapsedTime) / (long)maxTime);

			leftOffset = (startingLeftOffset * (long)(maxTime-elapsedTime))/ (long)maxTime;

			yBottom = (long)(elapsedTime*4)/(long)maxTime;
		}



		// Generate a table of 2500 multiples of var8 * 256
asm		xor    ax, ax;
asm		xor    dx, dx;
asm		mov    cx, 2500;
asm		mov    bx, var8;
asm		mov    es, introbuffer;
asm		xor    di, di;

loop1:

asm		mov    es:[di], ah;
asm		inc    di;
asm		mov    es:[di], dl;
asm		inc    di;
asm		add    ax, bx;
asm		adc    dx, 0;
asm		loop   loop1;

		if (elapsedTime == maxTime)
			leftOffset = 0;
		else
			leftOffset = ((long)(maxTime-elapsedTime) * startingLeftOffset) / (long)maxTime;


		var18 = *((uint16_t far *)introbuffer + 200);
		varC =0 ;
		varE = 0x10000L / var8;
		bufferofs = terminatorOfs + word_499C7;

		if (yBottom > 0)
			VW_Bar(0,0,320,yBottom, 0);


		EGAWRITEMODE(0);
		EGAMAPMASK(0xF);

		// Draw each line to the screen
		for (si = 0; si < var18; si++)
		{
			var16 = MK_FP(introbuffer2, ((introbmptype far*)introbuffer2)->linestarts[varC>>8]);
			ZoomOutTerminator_1(var16, si+yBottom, leftOffset, introbuffer);
			varC += varE;
		}

		bufferofs = terminatorOfs + word_499C7;
		var1C = var18 + yBottom;
		if (var20[terminatorPageOn] > var1C)
		{
			VW_Bar(0, var1C, 320,var20[terminatorPageOn],0);
			var20[terminatorPageOn] = var1C;
		}

		// Flip the page
		VW_SetScreen(terminatorOfs + word_499C7, 0);
		if (terminatorPageOn ^= 1)
			terminatorOfs = TERMINATORSCREENWIDTH/2;
		else
			terminatorOfs = 0;

		// loc_14B4F;
		newTime = TimeCount;
		tics = newTime-lasttimecount;
		lasttimecount = newTime;

		if (tics > 8)
			tics = 8;

		if (elapsedTime == maxTime)
			break;

		elapsedTime += tics;

		if (elapsedTime > maxTime)
			elapsedTime = maxTime;


		if (IN_IsUserInput() && LastScan == sc_F1)
			LastScan = sc_Space;

		if (LastScan)
			// return;// should this be break instead? Want to free intro buffers!
			break;

	}

	MM_FreePtr(&introbuffer);
	MM_FreePtr(&introbuffer2);
}
#endif


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
  VL_SetScrollCoords(0,0);

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

unsigned cmdrWidthX100;

uint16_t *srcptr;
uint8_t *destptr;


int cmdrLineStarts[200];


bool terminator_complete = false;
#if 0
CA_SetAllPurge();

// Prepare screen with black palette
colors[0][16] = bordercolor;
_ES=FP_SEG(&colors[0]);
_DX=FP_OFF(&colors[0]);
_AX=0x1002;
geninterrupt(0x10);
#endif

  VL_ClearScreen(0);

#if 0
VW_SetLineWidth(TERMINATORSCREENWIDTH);
#endif
VL_ResizeScreen(TERMINATORSCREENWIDTH * 8, 200);


// Cache Intro Bitmaps
CA_CacheGrChunk(PIC_TITLESCREEN);
CA_CacheGrChunk(EXTERN_COMMANDER);
CA_CacheGrChunk(EXTERN_KEEN);


ck_introKeen = (introbmptype *)ca_graphChunks[EXTERN_KEEN];
ck_introCommander = (introbmptype *)ca_graphChunks[EXTERN_COMMANDER];

// DOS: Only writing to plane 0 of display memory
// EGAMAPMASK(1);

// Omnispeak: Replicate that in software
VL_SetMapMask(1);

// Because "KEEN" needs to end up on the right side of the screen,
// an extra padding amount is added to the left side of the Keen graphic
ck_introScreenWidth = ck_introKeen->width + 25 * 8;

#if 0
// Set the screen to the right of the "KEEN" graphic
VW_SetScreen(ck_introScreenWidth/8 + 1, 0);
#endif
// Omnispeak: do the same
VL_SetScrollCoords(ck_introScreenWidth+1, 0);

// Copy each line of the KEEN graphic into video memory, accounting for the padding amount
{
uint8_t *destbuf = (uint8_t *)calloc(ck_introScreenWidth/8+1, sizeof(uint8_t));
for (int i = 0; i < 200; i++)
{
  //srcptr = MK_FP(ck_introKeen,  ck_introKeen->linestarts[i]);
  // destptr = MK_FP(0xA000 , ylookup[i] + 25); // Left margin of KEEN is 25*8 = 200 pixels

  // Omnispeak: RLE-Expand into temp buffer, then copy into video memory
  srcptr = (uint16_t*)((uint8_t *)ck_introKeen + ck_introKeen->linestarts[i]);
  TerminatorExpandRLE(srcptr, destbuf + 25);
  VL_1bppToScreen_PM(destbuf, 0, i, ck_introScreenWidth, 1, 0xF);
}

free (destbuf);
}

// Copy the KEEN graphic to the second page
// Omnispeak: Don't need to do this
// VW_ScreenToScreen(0,TERMINATORSCREENWIDTH/2,109,200);

// DOS: Allocate memory for 8 shifts of the COMMANDER graphic
// Notice that there are only 100 rows of memory, which means that
// each EVEN row of source graphics is duplicated
ck_introCommanderWidth = (ck_introCommander->width+7)>>3;
ck_introCommanderWidth = (ck_introCommanderWidth+3)&0xFFFE;

cmdrWidthX100 = ck_introCommanderWidth * 100;
for (int i = 0; i < 8; i++)
  MM_GetPtr(&shiftedCmdrBMPsegs[i],cmdrWidthX100);

ck_introKeen = (introbmptype *)ca_graphChunks[EXTERN_KEEN];
ck_introCommander = (introbmptype *)ca_graphChunks[EXTERN_COMMANDER];

for (int i = 0; i < 100; i++)
{
  cmdrLineStarts[2*i] = cmdrLineStarts[2*i+1] = i * ck_introCommanderWidth;

  //Dos
  //srcptr = MK_FP(ck_introCommander, ck_introCommander->linestarts[2*i]);
  //destptr = MK_FP(shiftedCmdrBMPsegs[0],cmdrLineStarts[2*i]);

  srcptr = (uint16_t*)((uint8_t *)ck_introCommander + ck_introCommander->linestarts[i*2]);
  destptr = (uint8_t*)shiftedCmdrBMPsegs[0] + cmdrLineStarts[2*i];
  TerminatorExpandRLE(srcptr, destptr);
}


// loc_150A5
// Looks like we're making 8 shifts of the "Commander" graphic
#if 0
for (j = 1; j < 8; j++)
{
memptr var10 = shiftedCmdrBMPsegs[j-1];
memptr var12 = shiftedCmdrBMPsegs[j];

asm		mov ds, var10;
asm		mov es, var12;
asm		mov cx, cmdrWidthX100;
asm		clc;
asm		xor si, si;
asm		xor di, di;

nextb:
asm		lodsb;
asm		rcr al, 1;
asm		stosb;
asm		loop nextb;

asm		mov ax, ss;
asm		mov ds, ax;
}
#endif

for (int i = 1; i < 8; i++)
{
  uint8_t* last = (uint8_t*)shiftedCmdrBMPsegs[i -1];
  uint8_t* next = (uint8_t*)shiftedCmdrBMPsegs[i];

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

for (int i = 0; i < 4; i++)
  DoubleSizeTerminatorPics(i);



// Do the terminator
ck_currentTermPicStartTime = ck_currentTerminatorCredit = 0;
ck_termCreditStage = -1;
AnimateTerminator();

// Omnispeak
VL_ResizeScreen(21 * 16, 14 * 16);


#if 0

  for (int i = 0; i < 4; i++)
    MM_FreePtr(&terminator_pic_memptrs[i]);

  for (j = 0; j < 8; j++)
    MM_FreePtr(&shiftedCmdrBMPsegs[j]);



	// After the terminator text has run, keys are checked
	if (!IN_GetLastScan())
	{
		; // ZoomOutTerminator();
	}
#endif

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

    //VW_SetScreen(bufferofs, 0);
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
  VL_SetScrollCoords(0,0);
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
