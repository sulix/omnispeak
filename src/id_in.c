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
#include "id_sd.h"
#include "id_us.h"

#include <string.h>
#include <SDL.h>

char nonchar_keys[] ={
	0x01, 0x0E, 0x0F, 0x1D,
	0x2A, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43,
	0x44, 0x57, 0x59, 0x46,
	0x1C, 0x36, 0x37, 0x38,
	0x47, 0x49, 0x4F, 0x51,
	0x52, 0x53, 0x45, 0x48,
	0x50, 0x4B, 0x4D, 0x00
};
const char *nonchar_key_strings[] ={
	"Esc",		"Bksp",		"Tab",		"Ctrl",
	"LShft",	"Space",	"CapsLk",	"F1",
	"F2",		"F3",		"F4",		"F5",
	"F6",		"F7", 		"F8", 		"F9",
	"F10",		"F11",		"F12",		"ScrlLk",
	"Enter",	"RShft",	"PrtSc",	"Alt",
	"Home",		"PgUp",		"End",		"PgDn",
	"Ins",		"Del",		"NumLk",	"Up",
	"Down",		"Left",		"Right",	""
};
const char *char_key_strings[] ={
	/*	 0	 	 1	 	 2	 	 3		 4	     5	 	 6	 	7	     */
	/*	 8	 	 9	 	 A	 	 B		 C		 D 		 E		 F	     */
	"?",	"?",	"1",	"2",	"3",	"4",	"5",	"6",	/* 0 */
	"7",	"8",	"9",	"0",	"-",	"+",	"?",	"?",
	"Q",	"W",	"E",	"R",	"T",	"Y",	"U",	"I",	/* 1 */
	"O",	"P",	"[",	"]",	"|",	"?",	"A",	"S",
	"D",	"F",	"G",	"H",	"J",	"K",	"L",	";",	/* 2 */
	"\"",	"?",	"?",	"?",	"Z",	"X",	"C",	"V",
	"B",	"N",	"M",	",",	".",	"/",	"?",	"?",	/* 3 */
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",	/* 4 */
	"\x0F",	"?",	"-",	"\x15",	"5",	"\x11",	"+",	"?",
	"\x13",	"?",	"?",	"?",	"?",	"?",	"?",	"?",	/* 5 */
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",	/* 6 */
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?",	/* 7 */
	"?",	"?",	"?",	"?",	"?",	"?",	"?",	"?"
};

// =========================================================================
static	char in_ASCIINames[] =		// Unshifted ASCII for scan codes
{
	//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	0  , 27 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8  , 9  ,	// 0
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13 , 0  , 'a', 's',	// 1
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39 , '`', 0  , 92 , 'z', 'x', 'c', 'v',	// 2
	'b', 'n', 'm', ',', '.', '/', 0  , '*', 0  , ' ', 0  , 0  , 0  , 0  , 0  , 0  ,	// 3
	0  , 0  , 0  , 0  , 0  , 0  , 0  , '7', '8', '9', '-', '4', '5', '6', '+', '1',	// 4
	'2', '3', '0', 127, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 5
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 6
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0		// 7
},
in_shiftNames[] =		// Shifted ASCII for scan codes
{
	//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	0  , 27 , '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8  , 9  ,	// 0
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13 , 0  , 'A', 'S',	// 1
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34 , '~', 0  , '|', 'Z', 'X', 'C', 'V',	// 2
	'B', 'N', 'M', '<', '>', '?', 0  , '*', 0  , ' ', 0  , 0  , 0  , 0  , 0  , 0  ,	// 3
	0  , 0  , 0  , 0  , 0  , 0  , 0  , '7', '8', '9', '-', '4', '5', '6', '+', '1',	// 4
	'2', '3', '0', 127, 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 5
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 6
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0   	// 7
},
in_specialNames[] =	// ASCII for 0xe0 prefixed codes
{
	//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 0
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 13 , 0  , 0  , 0  ,	// 1
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 2
	0  , 0  , 0  , 0  , 0  , '/', 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 3
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 4
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 5
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,	// 6
	0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0   	// 7
};



bool in_Paused;
bool in_keyStates[256];
IN_ScanCode in_lastKeyScanned = IN_SC_None;
char in_lastASCII;

// SDLK -> IN_SC
#define INL_MapKey(sdl,in_sc) case sdl: return in_sc

IN_ScanCode INL_SDLKToScanCode(int sdlKey)
{
	switch (sdlKey)
	{
		INL_MapKey(SDLK_RETURN, IN_SC_Enter);
		INL_MapKey(SDLK_ESCAPE, IN_SC_Escape);
		INL_MapKey(SDLK_SPACE, IN_SC_Space);
		INL_MapKey(SDLK_BACKSPACE, IN_SC_Backspace);
		INL_MapKey(SDLK_TAB, IN_SC_Tab);
		INL_MapKey(SDLK_LALT, IN_SC_Alt);
		INL_MapKey(SDLK_RALT, IN_SC_Alt);
		INL_MapKey(SDLK_LCTRL, IN_SC_Control);
		INL_MapKey(SDLK_RCTRL, IN_SC_Control);
		INL_MapKey(SDLK_CAPSLOCK, IN_SC_CapsLock);
#if SDL_VERSION_ATLEAST(1,3,0)
		INL_MapKey(SDLK_NUMLOCKCLEAR, IN_SC_NumLock);
		INL_MapKey(SDLK_SCROLLLOCK, IN_SC_ScrollLock);
#else
		INL_MapKey(SDLK_NUMLOCK, IN_SC_NumLock);
		INL_MapKey(SDLK_SCROLLOCK, IN_SC_ScrollLock);
#endif
		INL_MapKey(SDLK_LSHIFT, IN_SC_LeftShift);
		INL_MapKey(SDLK_RSHIFT, IN_SC_RightShift);
		INL_MapKey(SDLK_UP, IN_SC_UpArrow);
		INL_MapKey(SDLK_LEFT, IN_SC_LeftArrow);
		INL_MapKey(SDLK_RIGHT, IN_SC_RightArrow);
		INL_MapKey(SDLK_DOWN, IN_SC_DownArrow);
		INL_MapKey(SDLK_INSERT, IN_SC_Insert);
		INL_MapKey(SDLK_DELETE, IN_SC_Delete);
		INL_MapKey(SDLK_HOME, IN_SC_Home);
		INL_MapKey(SDLK_END, IN_SC_End);
		INL_MapKey(SDLK_PAGEUP, IN_SC_PgUp);
		INL_MapKey(SDLK_PAGEDOWN, IN_SC_PgDown);

		INL_MapKey(SDLK_PAUSE, IN_SC_Pause);

		INL_MapKey(SDLK_F1, IN_SC_F1);
		INL_MapKey(SDLK_F2, IN_SC_F2);
		INL_MapKey(SDLK_F3, IN_SC_F3);
		INL_MapKey(SDLK_F4, IN_SC_F4);
		INL_MapKey(SDLK_F5, IN_SC_F5);
		INL_MapKey(SDLK_F6, IN_SC_F6);
		INL_MapKey(SDLK_F7, IN_SC_F7);
		INL_MapKey(SDLK_F8, IN_SC_F8);
		INL_MapKey(SDLK_F9, IN_SC_F9);
		INL_MapKey(SDLK_F10, IN_SC_F10);

		INL_MapKey(SDLK_F11, IN_SC_F11);
		INL_MapKey(SDLK_F12, IN_SC_F12);

		INL_MapKey(SDLK_BACKQUOTE, IN_SC_Grave);

		INL_MapKey(SDLK_1, IN_SC_One);
		INL_MapKey(SDLK_2, IN_SC_Two);
		INL_MapKey(SDLK_3, IN_SC_Three);
		INL_MapKey(SDLK_4, IN_SC_Four);
		INL_MapKey(SDLK_5, IN_SC_Five);
		INL_MapKey(SDLK_6, IN_SC_Six);
		INL_MapKey(SDLK_7, IN_SC_Seven);
		INL_MapKey(SDLK_8, IN_SC_Eight);
		INL_MapKey(SDLK_9, IN_SC_Nine);
		INL_MapKey(SDLK_0, IN_SC_Zero);

		INL_MapKey(SDLK_MINUS, IN_SC_Minus);
		INL_MapKey(SDLK_EQUALS, IN_SC_Equals);

		INL_MapKey(SDLK_LEFTBRACKET, IN_SC_LeftBracket);
		INL_MapKey(SDLK_RIGHTBRACKET, IN_SC_RightBracket);

		INL_MapKey(SDLK_SEMICOLON, IN_SC_SemiColon);
		INL_MapKey(SDLK_QUOTE, IN_SC_SingleQuote);
		INL_MapKey(SDLK_BACKSLASH, IN_SC_BackSlash);

		INL_MapKey(SDLK_COMMA, IN_SC_Comma);
		INL_MapKey(SDLK_PERIOD, IN_SC_Period);
		INL_MapKey(SDLK_SLASH, IN_SC_Slash);

		INL_MapKey(SDLK_a, IN_SC_A);
		INL_MapKey(SDLK_b, IN_SC_B);
		INL_MapKey(SDLK_c, IN_SC_C);
		INL_MapKey(SDLK_d, IN_SC_D);
		INL_MapKey(SDLK_e, IN_SC_E);
		INL_MapKey(SDLK_f, IN_SC_F);
		INL_MapKey(SDLK_g, IN_SC_G);
		INL_MapKey(SDLK_h, IN_SC_H);
		INL_MapKey(SDLK_i, IN_SC_I);
		INL_MapKey(SDLK_j, IN_SC_J);
		INL_MapKey(SDLK_k, IN_SC_K);
		INL_MapKey(SDLK_l, IN_SC_L);
		INL_MapKey(SDLK_m, IN_SC_M);
		INL_MapKey(SDLK_n, IN_SC_N);
		INL_MapKey(SDLK_o, IN_SC_O);
		INL_MapKey(SDLK_p, IN_SC_P);
		INL_MapKey(SDLK_q, IN_SC_Q);
		INL_MapKey(SDLK_r, IN_SC_R);
		INL_MapKey(SDLK_s, IN_SC_S);
		INL_MapKey(SDLK_t, IN_SC_T);
		INL_MapKey(SDLK_u, IN_SC_U);
		INL_MapKey(SDLK_v, IN_SC_V);
		INL_MapKey(SDLK_w, IN_SC_W);
		INL_MapKey(SDLK_x, IN_SC_X);
		INL_MapKey(SDLK_y, IN_SC_Y);
		INL_MapKey(SDLK_z, IN_SC_Z);

		// Keypad keys (used as num-lock is toggled *off*, more or less)

		INL_MapKey(SDLK_KP_DIVIDE, IN_SC_Slash);
		INL_MapKey(SDLK_KP_MULTIPLY, IN_KP_Multiply);
		INL_MapKey(SDLK_KP_MINUS, IN_KP_Minus);
		INL_MapKey(SDLK_KP_PLUS, IN_KP_Plus);
		INL_MapKey(SDLK_KP_ENTER, IN_SC_Enter);
		INL_MapKey(SDLK_KP_PERIOD, IN_SC_Delete);
#if SDL_VERSION_ATLEAST(1,3,0)
		INL_MapKey(SDLK_KP_1, IN_SC_End);
		INL_MapKey(SDLK_KP_2, IN_SC_DownArrow);
		INL_MapKey(SDLK_KP_3, IN_SC_PgDown);
		INL_MapKey(SDLK_KP_4, IN_SC_LeftArrow);
		INL_MapKey(SDLK_KP_5, IN_KP_Center);
		INL_MapKey(SDLK_KP_6, IN_SC_RightArrow);
		INL_MapKey(SDLK_KP_7, IN_SC_Home);
		INL_MapKey(SDLK_KP_8, IN_SC_UpArrow);
		INL_MapKey(SDLK_KP_9, IN_SC_PgUp);
		INL_MapKey(SDLK_KP_0, IN_SC_Insert);
#else
		INL_MapKey(SDLK_KP1, IN_SC_End);
		INL_MapKey(SDLK_KP2, IN_SC_DownArrow);
		INL_MapKey(SDLK_KP3, IN_SC_PgDown);
		INL_MapKey(SDLK_KP4, IN_SC_LeftArrow);
		INL_MapKey(SDLK_KP5, IN_KP_Center);
		INL_MapKey(SDLK_KP6, IN_SC_RightArrow);
		INL_MapKey(SDLK_KP7, IN_SC_Home);
		INL_MapKey(SDLK_KP8, IN_SC_UpArrow);
		INL_MapKey(SDLK_KP9, IN_SC_PgUp);
		INL_MapKey(SDLK_KP0, IN_SC_Insert);
#endif
	default: return IN_SC_Invalid;
	}
}

#undef INL_MapKey

IN_KeyMapping in_kbdControls;

int16_t in_gamepadButtons[4];

IN_ScanCode *key_controls[] ={
	&in_kbdControls.jump, &in_kbdControls.pogo, &in_kbdControls.fire,
	&in_kbdControls.upLeft, &in_kbdControls.up, &in_kbdControls.upRight,
	&in_kbdControls.right, &in_kbdControls.downRight, &in_kbdControls.down,
	&in_kbdControls.downLeft, &in_kbdControls.left
};

static IN_Direction	in_dirTable[] =		// Quick lookup for total direction
{
	IN_dir_NorthWest,	IN_dir_North,	IN_dir_NorthEast,
	IN_dir_West,		IN_dir_None,	IN_dir_East,
	IN_dir_SouthWest,	IN_dir_South,	IN_dir_SouthEast
};

static void INL_HandleSDLEvent(SDL_Event *event)
{

	IN_ScanCode sc;
	static bool special;
	char c;

	switch (event->type)
	{
	case SDL_QUIT:
		Quit(0);
		break;
	case SDL_KEYDOWN:

		sc = INL_SDLKToScanCode(event->key.keysym.sym);
		in_keyStates[sc] = 1;
		in_lastKeyScanned = sc;

		if (sc == 0xe0)		// Special key prefix
			special = true;
		else if (sc == IN_SC_Pause)	// Handle Pause key
			in_Paused = true;
		else
		{
			if (sc & 0x80)	// Break code
			{
				sc &= 0x7f;

				// DEBUG - handle special keys: ctl-alt-delete, print scrn
				in_keyStates[sc] = false;
			}
			else			// Make code
			{
				/*
				LastCode = CurCode;
				CurCode = LastScan = sc;
				Keyboard[sc] = true;
				 */

				if (special)
					c = in_specialNames[sc];
				else
				{
#if 0
					if (sc == sc_CapsLock)
					{
						// CapsLock ^= true;
						// DEBUG - make caps lock light work
					}
#endif

					if (in_keyStates[IN_SC_LeftShift] || in_keyStates[IN_SC_RightShift])	// If shifted
					{
						c = in_shiftNames[sc];
#if 0
						if ((c >= 'A') && (c <= 'Z') && CapsLock)
							c += 'a' - 'A';
#endif
					}
					else
					{
						c = in_ASCIINames[sc];
#if 0
						if ((c >= 'a') && (c <= 'z') && CapsLock)
							c -= 'a' - 'A';
#endif
					}
				}
				if (c)
					in_lastASCII = c;
			}

			special = false;
		}


		break;
	case SDL_KEYUP:
		sc = INL_SDLKToScanCode(event->key.keysym.sym);
		in_keyStates[sc] = 0;
		break;
	}
}

static void INL_SetupKbdControls()
{
	in_kbdControls.jump = IN_SC_Control;
	in_kbdControls.pogo = IN_SC_Alt;
	in_kbdControls.fire = IN_SC_Space;
	in_kbdControls.up = IN_SC_UpArrow;
	in_kbdControls.down = IN_SC_DownArrow;
	in_kbdControls.left = IN_SC_LeftArrow;
	in_kbdControls.right = IN_SC_RightArrow;
	in_kbdControls.upLeft = IN_SC_Home;
	in_kbdControls.upRight = IN_SC_PgUp;
	in_kbdControls.downRight = IN_SC_PgDown;
	in_kbdControls.downLeft = IN_SC_End;
}

void IN_PumpEvents()
{
	SDL_Event event;
	in_lastKeyScanned = IN_SC_None;
	while (SDL_PollEvent(&event))
		INL_HandleSDLEvent(&event);
}

const char *IN_GetScanName ( IN_ScanCode scan)
{
	char *p;
	const char **ps;

	p = nonchar_keys;
	ps = nonchar_key_strings;

	while ( *p)
	{
		if ( *p == scan )
			return *ps;

		ps++;
		p++;
	}
	return char_key_strings [scan];
}

void IN_WaitKey()
{
	SDL_Event event;
	while (SDL_WaitEvent(&event))
	{
		INL_HandleSDLEvent(&event);
		if (event.type == SDL_KEYDOWN)
			break;
	}
}

//void IN_WaitASCII() {}

void IN_WaitForButtonPress()
{

	while (1)
	{
		IN_PumpEvents();

		if (in_lastKeyScanned)
		{
			in_keyStates[in_lastKeyScanned] = 0;

			if (in_lastKeyScanned == in_lastKeyScanned)
				in_lastKeyScanned = 0;

			in_lastKeyScanned = 0;
			return;
		}

	#if 0
		if (MousePresent)
		{
			if (INL_GetMouseButtons())
				while (INL_GetMouseButtons())
					;

			return;
		}

		for (int i = 0; i < 2; i++)
		{
			if (JoyPresent[i] || Gamepad)
			{
				if (IN_GetJoyButtonsDB(i))
				{
					while (IN_GetJoyButtonsDB(i))
						;
					return;
				}
			}
		}
	#endif
	}
}

bool IN_GetKeyState(IN_ScanCode scanCode)
{
	return in_keyStates[scanCode];
}

IN_ScanCode IN_GetLastScan(void)
{
	return in_lastKeyScanned;
}

void IN_SetLastScan(IN_ScanCode scanCode)
{
	in_lastKeyScanned = scanCode;
}

char IN_GetLastASCII(void)
{
	return in_lastASCII;
}

void IN_SetLastASCII(char c)
{
  in_lastASCII = c;
}

void IN_Startup(void)
{
	for (int i = 0; i < 256; ++i)
		in_keyStates[i] = 0;

	// Set the default kbd controls.
	INL_SetupKbdControls();
}

// TODO: IMPLEMENT!
void IN_Default(bool gotit,int16_t inputChoice)
{
}

uint8_t *in_demoBuf;
int in_demoPtr;
int in_demoBytes;
IN_DemoMode in_demoState;

void IN_DemoStartPlaying(uint8_t *data, int len)
{
	in_demoBuf = data;
	in_demoBytes = len;
	in_demoPtr = 0;
	in_demoState = IN_Demo_Playback;
}

void IN_DemoStopPlaying()
{
	in_demoState = IN_Demo_Off;
}

#if 0
bool IN_DemoIsPlaying()
{
	return in_demoState == IN_Demo_Playback;
}
#endif

IN_DemoMode IN_DemoGetMode()
{
	return in_demoState;
}

void IN_ClearKeysDown()
{
	in_lastKeyScanned = IN_SC_None;
	in_lastASCII = IN_KP_None;
	memset (in_keyStates, 0, sizeof (in_keyStates));
}

void IN_ReadControls(int player, IN_ControlFrame *controls)
{
	controls->xDirection = IN_motion_None;
	controls->yDirection = IN_motion_None;
	controls->jump = false;
	controls->pogo = false;
	controls->button2 = false;

	if (in_demoState == IN_Demo_Playback)
	{
		uint8_t ctrlByte = in_demoBuf[in_demoPtr + 1];
		controls->yDirection = (IN_Motion) ((ctrlByte & 3) - 1);
		controls->xDirection = (IN_Motion) (((ctrlByte >> 2) & 3) - 1);
		controls->jump = (ctrlByte >> 4) & 1;
		controls->pogo = (ctrlByte >> 5) & 1;

		// Delay for n frames.
		if ((--in_demoBuf[in_demoPtr]) == 0 )
		{
			in_demoPtr += 2;
			if (in_demoPtr >= in_demoBytes)
				in_demoState = IN_Demo_PlayDone;
		}
	}
	else if (in_demoState == IN_Demo_PlayDone)
	{
		Quit("Demo playback exceeded");
	}
	else
	{
		if (IN_GetKeyState(in_kbdControls.upLeft))
		{
			controls->xDirection = IN_motion_Left;
			controls->yDirection = IN_motion_Up;
		}
		else if (IN_GetKeyState(in_kbdControls.upRight))
		{
			controls->xDirection = IN_motion_Right;
			controls->yDirection = IN_motion_Up;
		}
		else if (IN_GetKeyState(in_kbdControls.downLeft))
		{
			controls->xDirection = IN_motion_Left;
			controls->yDirection = IN_motion_Down;
		}
		else if (IN_GetKeyState(in_kbdControls.downRight))
		{
			controls->xDirection = IN_motion_Right;
			controls->yDirection = IN_motion_Down;
		}

		if (IN_GetKeyState(in_kbdControls.up)) controls->yDirection = IN_motion_Up;
		else if (IN_GetKeyState(in_kbdControls.down)) controls->yDirection = IN_motion_Down;

		if (IN_GetKeyState(in_kbdControls.left)) controls->xDirection = IN_motion_Left;
		else if (IN_GetKeyState(in_kbdControls.right)) controls->xDirection = IN_motion_Right;

		if (IN_GetKeyState(in_kbdControls.jump)) controls->jump = true;
		if (IN_GetKeyState(in_kbdControls.pogo)) controls->pogo = true;
		// TODO: Handle fire input separately? (e.g. two-button firing)
		if (IN_GetKeyState(in_kbdControls.fire)) controls->button2 = true;

		controls->dir = in_dirTable[3 * (controls->yDirection + 1) + controls->xDirection + 1];
	}

	//TODO: Record this inputFrame
}

void IN_WaitButton()
{
	// Zero all of the input
	IN_PumpEvents();

	in_keyStates[in_lastKeyScanned] = 0;

	if (in_lastKeyScanned == in_lastKeyScanned)
		in_lastKeyScanned = 0;

	in_lastKeyScanned = 0;

#if 0
	if (MousePresent)
	{
		while (INL_GetMouseButtons())
			;
	}

	for (int i = 0; i < 2; i++)
	{
		if (JoyPresent[i] || Gamepad)
			while (IN_GetJoyButtonsDB(i))
				;
	}
#endif

	// Now wait for a button press
	IN_WaitForButtonPress();

}

// TODO: Handle Joy/Mouse
int IN_CheckAck()
{
	int di;

	di = IN_GetLastScan();

#if 0
	if (MousePresent && INL_GetMouseButtons())
		di = 1;

	for (int i = 0; i < 2; i++)
	{
		if (JoyPresent[i] || Gamepad)
			if (INL_GetJoyButtons)
				di = 1;
	}

#endif

	return di;

}

bool IN_UserInput(int tics, bool waitPress)
{
	int lasttime = SD_GetTimeCount();

	do
	{
		IN_PumpEvents();

		if (IN_CheckAck())
		{
			if (waitPress)
				IN_WaitForButtonPress();

			return true;

		}
	} while (SD_GetTimeCount() - lasttime < tics);

	return false;
}
