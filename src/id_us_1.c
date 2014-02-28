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

#include "id_vh.h"
#include "id_vl.h"
#include "id_ca.h"
#include "id_us.h"
#include "ck_cross.h"
#include "ck_play.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h> // For strcasecmp
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>

// Check for a parameter in a list of strings
// Returns index of string found, or -1 if none

int US_CheckParm(const char *parm, const char **strings)
{
	// Strip any non-alphabetic characters from 'parm'
	while (*parm)
	{
		if (isalpha(*(parm))) break;
		parm++;
	}

	// For every string in 'strings'
	for (int i = 0; strings[i]; ++i)
	{

		if (strings[i][0] == '\0') continue;
		/* BIG FIXME FIXME FIXME
		 *
		 * strcasecmp may function differently than expected.
		 * For instance, with the Turkish locale in mind, it is
		 * possible that strcasecmp("i", "I") is non-zero.
		 * Furthermore, strcasecmp is not a part of the C99 standard.
		 */
		if (!strcasecmp(parm, strings[i])) return i;
	}
	return -1;
}


// Coords in pixels
static int16_t us_windowX;
static int16_t us_windowY;
static int16_t us_windowW;
static int16_t us_windowH;

static int16_t us_printX;
static int16_t us_printY;

static int16_t us_printFont = 0;
static int8_t us_printColour = 15;
static int16_t us_backColour = 0;

#define US_WINDOW_MAX_X 320
#define US_WINDOW_MAX_Y 200

void (*p_save_game)(FILE *handle);
void (*p_load_game)(FILE *handle);
void (*p_exit_menu)(void);

void US_SetMenuFunctionPointers(void (*loadgamefunc)(FILE *), void (*savegamefunc)(FILE *), void (*exitmenufunc)(void))
{
	p_load_game = loadgamefunc;
	p_save_game = savegamefunc;
	p_exit_menu = exitmenufunc;
}

void US_Print(const char *str)
{
	char strbuf[256];
	int sboff = 0;

	while (*str)
	{
		char ch;
		sboff = 0;
		while (true)
		{
			// TODO: Modify this (and possibly more)
			ch = *str;
			str++;
			if (ch == '\0' || ch == '\n')
			{
				strbuf[sboff] = '\0';
				break;
			}
			strbuf[sboff] = ch;
			sboff++;
		}

		uint16_t w, h;
		CA_CacheGrChunk(3); // TODO: Why is that done here?
		// TODO: Should us_printFont and us_printColour
		// be passed as arguments or not?
		VH_MeasurePropString(strbuf, &w, &h, us_printFont);
		VH_DrawPropString(strbuf, us_printX, us_printY, us_printFont, us_printColour);


		if (ch)
		{
			us_printX = us_windowX;
			us_printY += h;
		}
		else
		{
			us_printY += h;
			break;
		}
	}
}

void US_PrintF(const char *str, ...)
{
	char buf[256];
	va_list args;
	va_start(args, str);
	vsprintf(buf, str, args);
	va_end(args);
	US_Print(buf);
}

void USL_PrintInCenter(const char *str, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint16_t w, h, rw, rh, px, py;
	VH_MeasurePropString(str, &w, &h, us_printFont);
	rw = x2 - x1;
	rh = y2 - y2;
	px = x1 + (rw - w) / 2;
	py = y1 + (rh - h) / 2 + h;
	VH_DrawPropString(str, px, py, us_printFont, us_printColour);
}

void US_PrintCentered(const char *str)
{
	USL_PrintInCenter(str, us_windowX, us_windowY, us_windowX + us_windowW, us_windowY + us_windowH);
}

void US_CPrintLine(const char *str)
{
	uint16_t w, h;
	CA_CacheGrChunk(3); // TODO: What is this function call doing here?
	VH_MeasurePropString(str, &w, &h, us_printFont);
	if (w <= us_windowW)
	{
		int x = us_windowX + ((us_windowW - w) / 2);
		VH_DrawPropString(str, x, us_printY, us_printFont, us_printColour);
		us_printY += h;
	}
	else
	{
		Quit("US_CPrintLine() - String exceeds width");
	}
}

void US_CPrint(char *str)
{
	char lastChar;
	char *strInLine;
	while (*str)
	{
		strInLine = str;
		while (*strInLine)
		{
			if (*strInLine == '\n')
				break;
			strInLine++;
		}
		lastChar = *strInLine;
		*strInLine = '\0'; // Hence, str is not const
		US_CPrintLine(str);
		str = strInLine;
		if (lastChar)
		{
			*strInLine = lastChar; // Again not const
			str++;
		}
	}
}

void US_CPrintF(char *str, ...)
{
	char buf[256];
	va_list args;
	va_start(args, str);
	vsprintf(buf, str, args);
	va_end(args);
	US_CPrint(buf);
}

void US_ClearWindow()
{
	VL_ScreenRect(us_windowX, us_windowY, us_windowW, us_windowH, 15);
	us_printX = us_windowX;
	us_printY = us_windowY;
}

// Coords in tile8s

void US_DrawWindow(int x, int y, int w, int h)
{
	us_windowX = x * 8;
	us_windowY = y * 8;
	us_windowW = w * 8;
	us_windowH = h * 8;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "US_DrawWindow: (%d,%d)-(%d,%d)\n", x, y, w, h);

	int borderX = us_windowX - 8;
	int borderY = us_windowY - 8;
	int borderW = us_windowW + 8;
	int borderH = us_windowH + 8;

	us_printX = us_windowX;
	us_printY = us_windowY;

	US_ClearWindow();

	VH_DrawTile8M(borderX, borderY, 0); //Top Left Corner
	VH_DrawTile8M(borderX, borderY + borderH, 6); //Bottom Left Corner
	VH_DrawTile8M(borderX + borderW, borderY, 2); //Top Right Corner
	VH_DrawTile8M(borderX + borderW, borderY + borderH, 8); //Bottom Right Corner
	// Draw Horizontal sides
	for (int i = borderX + 8; i <= borderX + borderW - 8; i += 8)
	{
		VH_DrawTile8M(i, borderY, 1); //Top row
		VH_DrawTile8M(i, borderY + borderH, 7); //Bottom Row
	}
	//Draw Vertical sides
	for (int i = borderY + 8; i <= borderY + borderH - 8; i += 8)
	{
		VH_DrawTile8M(borderX, i, 3); //Left col
		VH_DrawTile8M(borderX + borderW, i, 5); //Right col
	}
}

void US_CenterWindow(int w, int h)
{
	const int maxXtile = US_WINDOW_MAX_X / 8;
	const int maxYtile = US_WINDOW_MAX_Y / 8;

	US_DrawWindow((maxXtile - w) / 2, (maxYtile - h) / 2, w, h);
}

//	US_SaveWindow() - Saves the current window parms into a record for later restoration 
void US_SaveWindow(US_WindowRec *win)
{
	win->x = US_GetWindowX();
	win->y = US_GetWindowY();
	win->w = US_GetWindowW();
	win->h = US_GetWindowH();

	win->px = US_GetPrintX();
	win->py = US_GetPrintY();
}

//	US_RestoreWindow() - Sets the current window parms to those held in the record

void US_RestoreWindow(US_WindowRec *win)
{
	US_SetWindowX(win->x);
	US_SetWindowY(win->y);
	US_SetWindowW(win->w);
	US_SetWindowH(win->h);

	US_SetPrintX(win->px);
	US_SetPrintY(win->py);
}

//	Input routines

//	USL_XORICursor() - XORs the I-bar text cursor. Used by US_LineInput()

static void USL_XORICursor(uint16_t x, uint16_t y, char *s, uint16_t cursor)
{
	//static	bool	status;		// VGA doesn't XOR...
	static char cursorStr[2] = {0x80, 0};
	char	buf[128];
	uint16_t w, h;

	strcpy(buf, s);
	buf[cursor] = '\0';
	VH_MeasurePropString(buf, &w, &h, us_printFont);
	// TODO: More changes to do here?

	US_SetPrintX(x + w - 1);
	US_SetPrintY(y);

	VH_DrawPropString(cursorStr, US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour());
#if 0

	if (status^=true)
		VH_DrawPropString("\x80", US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour());
	else
	{
		temp = us_printColour;
		us_printColour = us_backColour;
		VH_DrawPropString("\x80", US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour());
		us_printColour = temp;
	}
#endif
}

//	US_LineInput() - Gets a line of user input at (x,y), the string defaults
//		to whatever is pointed at by def. Input is restricted to maxchars
//		chars or maxwidth pixels wide. If the user hits escape (and escok is
//		true), nothing is copied into buf, and false is returned. If the
//		user hits return, the current string is copied into buf, and true is
//		returned

bool US_LineInput(uint16_t x, uint16_t y, char *buf, char *def, bool escok, uint16_t maxchars, uint16_t maxwidth)
{
	bool		redraw,
		cursorvis, cursormoved,
		done, result;
	IN_ScanCode	sc;
	char	c,
		s[128], olds[128];
	uint16_t i,
	         cursor,
	         w, h,
	         len/*, temp*/;
	uint32_t lasttime;

	if (def)
		strcpy(s, def);
	else
		*s = '\0';
	*olds = '\0';
	cursor = strlen(s);
	cursormoved = redraw = true;

	cursorvis = done = false;
	lasttime = SD_GetTimeCount();

	/*
	LastASCII = key_None;
	LastScan = sc_None;
	 */
	IN_ClearKeysDown();

	while (!done)
	{
		// TODO/FIXME: Handle this in a possibly better way
		// (no busy loop, updating gfx if required, etc..)
		IN_PumpEvents();
		//CK_SetTicsPerFrame();

		if (cursorvis)
			USL_XORICursor(x, y, s, cursor);

		/*
		sc = LastScan;
		LastScan = sc_None;
		c = LastASCII;
		LastASCII = key_None;
		*/
		sc = IN_GetLastScan();
		c = IN_GetLastASCII();
		IN_ClearKeysDown();
		
		switch (sc)
		{
		case IN_SC_LeftArrow:
			if (cursor)
				cursor--;
			c = 0;
			cursormoved = true;
			break;
		case IN_SC_RightArrow:
			if (s[cursor])
				cursor++;
			c = 0;
			cursormoved = true;
			break;
		case IN_SC_Home:
			cursor = 0;
			c = 0;
			cursormoved = true;
			break;
		case IN_SC_End:
			cursor = strlen(s);
			c = 0;
			cursormoved = true;
			break;

		case IN_SC_Enter:
			strcpy(buf, s);
			done = true;
			result = true;
			c = 0;
			break;
		case IN_SC_Escape:
			if (escok)
			{
				done = true;
				result = false;
			}
			c = 0;
			break;

		case IN_SC_Backspace:
			if (cursor)
			{
				strcpy(s + cursor - 1, s + cursor);
				cursor--;
				redraw = true;
			}
			c = 0;
			cursormoved = true;
			break;
		case IN_SC_Delete:
			if (s[cursor])
			{
				strcpy(s + cursor, s + cursor + 1);
				redraw = true;
			}
			c = 0;
			cursormoved = true;
			break;

		case 0x4c:	// Keypad 5
		case IN_SC_UpArrow:
		case IN_SC_DownArrow:
		case IN_SC_PgUp:
		case IN_SC_PgDown:
		case IN_SC_Insert:
			c = 0;
			break;
		}

		if (c)
		{
			len = strlen(s);
			VH_MeasurePropString(s, &w, &h, US_GetPrintFont());

			if
				(
				 isprint(c)
				 &&	(len < 128 - 1)
				 &&	((!maxchars) || (len < maxchars))
				 &&	((!maxwidth) || (w < maxwidth))
				 )
			{
				for (i = len + 1; i > cursor; i--)
					s[i] = s[i - 1];
				s[cursor++] = c;
				redraw = true;
			}
		}

		if (redraw)
		{
			/*
			px = x;
			py = y;
			 */
			//temp = us_printColour;
			//us_printColour = us_backColour;
			VH_DrawPropString(olds, x, y, us_printFont, us_printColour);
			//us_printColour = temp;
			strcpy(olds, s);

			/*
			px = x;
			py = y;
			 */
			VH_DrawPropString(s, x, y, us_printFont, us_printColour);

			redraw = false;
		}

		if (cursormoved)
		{
			cursorvis = false;
			lasttime = SD_GetTimeCount() - 70 /*TimeCount - TickBase*/;

			cursormoved = false;
		}
		if (SD_GetTimeCount()-lasttime > 35 /*TimeCount - lasttime > TickBase / 2*/)
		{
			lasttime = SD_GetTimeCount();//TimeCount;

			cursorvis ^= true;
		}
		if (cursorvis)
			USL_XORICursor(x, y, s, cursor);

		//VW_UpdateScreen();
		VL_Present();
	}

	if (cursorvis)
		USL_XORICursor(x, y, s, cursor);
	if (!result)
	{
		/*
		px = x;
		py = y;
		*/
		VH_DrawPropString(olds, x, y, us_printFont, us_printColour);
	}

	//VW_UpdateScreen();
	VL_Present();

	IN_ClearKeysDown();
	return (result);
}
// Random Number Generator

/*
 * This random number generator simply outputs these numbers, in order
 * from a random index. This makes behaviour predictable during demos, and
 * ensures a uniform (enough) distribution.
 */
uint8_t us_RandomTable[256] ={
	0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66,
	74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36,
	95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188,
	52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224,
	149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242,
	145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0,
	175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235,
	25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113,
	94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75,
	136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196,
	135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113,
	80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241,
	24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224,
	145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95,
	28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226,
	71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36,
	17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106,
	197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136,
	120, 163, 236, 249
};

static int us_randomIndex;

// Seed the random number generator.

void US_InitRndT(bool randomize)
{
	if (randomize)
	{
		us_randomIndex = time(0);
	}
	else
	{
		us_randomIndex = 0;
	}
}

// Get a random integer in the range 0-255

int US_RndT()
{
	return us_RandomTable[(us_randomIndex++)&0xff];
}

// Set the random seed (index)

void US_SetRndI(int index)
{
	us_randomIndex = index;
}

// Get the random seed (index)

int US_GetRndI()
{
	return us_randomIndex;
}


// Common

/*
 * US_Startup
 * Startup routine for the 'User Manager'
 */

static const char *us_parmStrings[] ={"TEDLEVEL", "NOWAIT", "\0", NULL};

bool us_noWait; // Debug mode enabled.
bool us_tedLevel; // Launching a level from TED
int us_tedLevelNumber; // Number of level to launch from TED

// We need to steal these from main().
const char **us_argv;
int us_argc;

void US_Startup()
{
	// Initialize the random number generator (to the current time).
	US_InitRndT(true);

	// Check command line args.
	for (int i = 1; i < us_argc; ++i)
	{
		int parmIdx = US_CheckParm(us_argv[i], us_parmStrings);
		switch (parmIdx)
		{
		case 0:
			us_tedLevelNumber = atoi(us_argv[i + 1]);
			if (us_tedLevelNumber > -1)
				us_tedLevel = true;
			break;
		case 1:
			us_noWait = true;
			break;
		}
	}

}

// Getter and setter functions for print variables

int US_GetWindowX()
{
	return us_windowX;
}

int US_GetWindowY()
{
	return us_windowY;
}

int US_GetWindowW()
{
	return us_windowW;
}

int US_GetWindowH()
{
	return us_windowH;
}

int US_GetPrintX()
{
	return us_printX;
}

int US_GetPrintY()
{
	return us_printY;
}

int16_t US_GetPrintFont()
{
	return us_printFont;
}

int8_t US_GetPrintColour()
{
	return us_printColour;
}

void US_SetWindowX(int parm)
{
	us_windowX = parm;
}

void US_SetWindowY(int parm)
{
	us_windowY = parm;
}

void US_SetWindowW(int parm)
{
	us_windowW = parm;
}

void US_SetWindowH(int parm)
{
	us_windowH = parm;
}

void US_SetPrintX(int parm)
{
	us_printX = parm;
}

void US_SetPrintY(int parm)
{
	us_printY = parm;
}

void US_SetPrintFont(int16_t parm)
{
	us_printFont = parm;
}

void US_SetPrintColour(int8_t parm)
{
	us_printColour = parm;
}
