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
#include "id_vl.h"

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
IN_ControlType in_controlType = IN_ctrl_Keyboard1;
bool in_keyStates[256];
IN_ScanCode in_lastKeyScanned = IN_SC_None;
char in_lastASCII;
bool in_disableJoysticks = false;
static bool INL_StartJoy(int joystick);
static void INL_StopJoy(int joystick);

#define IN_MAX_JOYSTICKS 2

SDL_Joystick *in_joysticks[IN_MAX_JOYSTICKS];
bool in_joystickPresent[IN_MAX_JOYSTICKS];

// SDLKey -> IN_SC
#define INL_MapKey(sdl,in_sc) case sdl: return in_sc

#if SDL_VERSION_ATLEAST(1,3,0)
static IN_ScanCode INL_SDLKeySymToScanCode(const SDL_Keysym *keySym)
{
	int sdlScanCode = keySym->scancode;
	switch (sdlScanCode)
	{
		INL_MapKey(SDL_SCANCODE_RETURN, IN_SC_Enter);
		INL_MapKey(SDL_SCANCODE_ESCAPE, IN_SC_Escape);
		INL_MapKey(SDL_SCANCODE_SPACE, IN_SC_Space);
		INL_MapKey(SDL_SCANCODE_BACKSPACE, IN_SC_Backspace);
		INL_MapKey(SDL_SCANCODE_TAB, IN_SC_Tab);
		INL_MapKey(SDL_SCANCODE_LALT, IN_SC_Alt);
		INL_MapKey(SDL_SCANCODE_RALT, IN_SC_Alt);
		INL_MapKey(SDL_SCANCODE_LCTRL, IN_SC_Control);
		INL_MapKey(SDL_SCANCODE_RCTRL, IN_SC_Control);
		INL_MapKey(SDL_SCANCODE_CAPSLOCK, IN_SC_CapsLock);
		INL_MapKey(SDL_SCANCODE_NUMLOCKCLEAR, IN_SC_NumLock);
		INL_MapKey(SDL_SCANCODE_SCROLLLOCK, IN_SC_ScrollLock);
		INL_MapKey(SDL_SCANCODE_LSHIFT, IN_SC_LeftShift);
		INL_MapKey(SDL_SCANCODE_RSHIFT, IN_SC_RightShift);
		INL_MapKey(SDL_SCANCODE_UP, IN_SC_UpArrow);
		INL_MapKey(SDL_SCANCODE_LEFT, IN_SC_LeftArrow);
		INL_MapKey(SDL_SCANCODE_RIGHT, IN_SC_RightArrow);
		INL_MapKey(SDL_SCANCODE_DOWN, IN_SC_DownArrow);
		INL_MapKey(SDL_SCANCODE_INSERT, IN_SC_Insert);
		INL_MapKey(SDL_SCANCODE_DELETE, IN_SC_Delete);
		INL_MapKey(SDL_SCANCODE_HOME, IN_SC_Home);
		INL_MapKey(SDL_SCANCODE_END, IN_SC_End);
		INL_MapKey(SDL_SCANCODE_PAGEUP, IN_SC_PgUp);
		INL_MapKey(SDL_SCANCODE_PAGEDOWN, IN_SC_PgDown);

		INL_MapKey(SDL_SCANCODE_PAUSE, IN_SC_Pause);

		INL_MapKey(SDL_SCANCODE_F1, IN_SC_F1);
		INL_MapKey(SDL_SCANCODE_F2, IN_SC_F2);
		INL_MapKey(SDL_SCANCODE_F3, IN_SC_F3);
		INL_MapKey(SDL_SCANCODE_F4, IN_SC_F4);
		INL_MapKey(SDL_SCANCODE_F5, IN_SC_F5);
		INL_MapKey(SDL_SCANCODE_F6, IN_SC_F6);
		INL_MapKey(SDL_SCANCODE_F7, IN_SC_F7);
		INL_MapKey(SDL_SCANCODE_F8, IN_SC_F8);
		INL_MapKey(SDL_SCANCODE_F9, IN_SC_F9);
		INL_MapKey(SDL_SCANCODE_F10, IN_SC_F10);

		INL_MapKey(SDL_SCANCODE_F11, IN_SC_F11);
		INL_MapKey(SDL_SCANCODE_F12, IN_SC_F12);

		INL_MapKey(SDL_SCANCODE_GRAVE, IN_SC_Grave);

		INL_MapKey(SDL_SCANCODE_1, IN_SC_One);
		INL_MapKey(SDL_SCANCODE_2, IN_SC_Two);
		INL_MapKey(SDL_SCANCODE_3, IN_SC_Three);
		INL_MapKey(SDL_SCANCODE_4, IN_SC_Four);
		INL_MapKey(SDL_SCANCODE_5, IN_SC_Five);
		INL_MapKey(SDL_SCANCODE_6, IN_SC_Six);
		INL_MapKey(SDL_SCANCODE_7, IN_SC_Seven);
		INL_MapKey(SDL_SCANCODE_8, IN_SC_Eight);
		INL_MapKey(SDL_SCANCODE_9, IN_SC_Nine);
		INL_MapKey(SDL_SCANCODE_0, IN_SC_Zero);

		INL_MapKey(SDL_SCANCODE_MINUS, IN_SC_Minus);
		INL_MapKey(SDL_SCANCODE_EQUALS, IN_SC_Equals);

		INL_MapKey(SDL_SCANCODE_LEFTBRACKET, IN_SC_LeftBracket);
		INL_MapKey(SDL_SCANCODE_RIGHTBRACKET, IN_SC_RightBracket);

		INL_MapKey(SDL_SCANCODE_SEMICOLON, IN_SC_SemiColon);
		INL_MapKey(SDL_SCANCODE_APOSTROPHE, IN_SC_SingleQuote);
		INL_MapKey(SDL_SCANCODE_BACKSLASH, IN_SC_BackSlash);

		INL_MapKey(SDL_SCANCODE_COMMA, IN_SC_Comma);
		INL_MapKey(SDL_SCANCODE_PERIOD, IN_SC_Period);
		INL_MapKey(SDL_SCANCODE_SLASH, IN_SC_Slash);

		INL_MapKey(SDL_SCANCODE_A, IN_SC_A);
		INL_MapKey(SDL_SCANCODE_B, IN_SC_B);
		INL_MapKey(SDL_SCANCODE_C, IN_SC_C);
		INL_MapKey(SDL_SCANCODE_D, IN_SC_D);
		INL_MapKey(SDL_SCANCODE_E, IN_SC_E);
		INL_MapKey(SDL_SCANCODE_F, IN_SC_F);
		INL_MapKey(SDL_SCANCODE_G, IN_SC_G);
		INL_MapKey(SDL_SCANCODE_H, IN_SC_H);
		INL_MapKey(SDL_SCANCODE_I, IN_SC_I);
		INL_MapKey(SDL_SCANCODE_J, IN_SC_J);
		INL_MapKey(SDL_SCANCODE_K, IN_SC_K);
		INL_MapKey(SDL_SCANCODE_L, IN_SC_L);
		INL_MapKey(SDL_SCANCODE_M, IN_SC_M);
		INL_MapKey(SDL_SCANCODE_N, IN_SC_N);
		INL_MapKey(SDL_SCANCODE_O, IN_SC_O);
		INL_MapKey(SDL_SCANCODE_P, IN_SC_P);
		INL_MapKey(SDL_SCANCODE_Q, IN_SC_Q);
		INL_MapKey(SDL_SCANCODE_R, IN_SC_R);
		INL_MapKey(SDL_SCANCODE_S, IN_SC_S);
		INL_MapKey(SDL_SCANCODE_T, IN_SC_T);
		INL_MapKey(SDL_SCANCODE_U, IN_SC_U);
		INL_MapKey(SDL_SCANCODE_V, IN_SC_V);
		INL_MapKey(SDL_SCANCODE_W, IN_SC_W);
		INL_MapKey(SDL_SCANCODE_X, IN_SC_X);
		INL_MapKey(SDL_SCANCODE_Y, IN_SC_Y);
		INL_MapKey(SDL_SCANCODE_Z, IN_SC_Z);

		// Keypad keys (used as num-lock is toggled *off*, more or less)

		INL_MapKey(SDL_SCANCODE_KP_DIVIDE, IN_SC_Slash);
		INL_MapKey(SDL_SCANCODE_KP_MULTIPLY, IN_KP_Multiply);
		INL_MapKey(SDL_SCANCODE_KP_MINUS, IN_KP_Minus);
		INL_MapKey(SDL_SCANCODE_KP_PLUS, IN_KP_Plus);
		INL_MapKey(SDL_SCANCODE_KP_ENTER, IN_SC_Enter);
		INL_MapKey(SDL_SCANCODE_KP_PERIOD, IN_SC_Delete);
		INL_MapKey(SDL_SCANCODE_KP_1, IN_SC_End);
		INL_MapKey(SDL_SCANCODE_KP_2, IN_SC_DownArrow);
		INL_MapKey(SDL_SCANCODE_KP_3, IN_SC_PgDown);
		INL_MapKey(SDL_SCANCODE_KP_4, IN_SC_LeftArrow);
		INL_MapKey(SDL_SCANCODE_KP_5, IN_KP_Center);
		INL_MapKey(SDL_SCANCODE_KP_6, IN_SC_RightArrow);
		INL_MapKey(SDL_SCANCODE_KP_7, IN_SC_Home);
		INL_MapKey(SDL_SCANCODE_KP_8, IN_SC_UpArrow);
		INL_MapKey(SDL_SCANCODE_KP_9, IN_SC_PgUp);
		INL_MapKey(SDL_SCANCODE_KP_0, IN_SC_Insert);

		INL_MapKey(SDL_SCANCODE_NONUSBACKSLASH, IN_SC_SecondaryBackSlash);

	default: return IN_SC_Invalid;
	}
}
#else
static IN_ScanCode INL_SDLKeySymToScanCode(const SDL_keysym *keySym)
{
	int sdlSym = keySym->sym;
	switch (sdlSym)
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
		INL_MapKey(SDLK_NUMLOCK, IN_SC_NumLock);
		INL_MapKey(SDLK_SCROLLOCK, IN_SC_ScrollLock);
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

		INL_MapKey(SDLK_LESS, IN_SC_SecondaryBackSlash);

	default: return IN_SC_Invalid;
	}
}
#endif

#undef INL_MapKey

IN_KeyMapping in_kbdControls;

int16_t in_gamepadButtons[4];

IN_ScanCode *key_controls[] ={
	&in_kbdControls.jump, &in_kbdControls.pogo, &in_kbdControls.fire,
	&in_kbdControls.upLeft, &in_kbdControls.up, &in_kbdControls.upRight,
	&in_kbdControls.right, &in_kbdControls.downRight, &in_kbdControls.down,
	&in_kbdControls.downLeft, &in_kbdControls.left
};

static	bool		CapsLock;

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

		sc = INL_SDLKeySymToScanCode(&event->key.keysym);
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
					if (sc == IN_SC_CapsLock)
					{
						CapsLock ^= true;
						// DEBUG - make caps lock light work
					}

					if (in_keyStates[IN_SC_LeftShift] || in_keyStates[IN_SC_RightShift])	// If shifted
					{
						c = in_shiftNames[sc];
						if ((c >= 'A') && (c <= 'Z') && CapsLock)
							c += 'a' - 'A';
					}
					else
					{
						c = in_ASCIINames[sc];
						if ((c >= 'a') && (c <= 'z') && CapsLock)
							c -= 'a' - 'A';
					}
				}
				if (c)
					in_lastASCII = c;
			}

			special = false;
		}


		break;
	case SDL_KEYUP:
		//Use F11 to toggle fullscreen.
		if (event->key.keysym.sym == SDLK_F11)
			VL_ToggleFullscreen();

		sc = INL_SDLKeySymToScanCode(&event->key.keysym);
		in_keyStates[sc] = 0;
		break;
#if SDL_VERSION_ATLEAST(2,0,0)
	case SDL_JOYDEVICEADDED:
		INL_StartJoy(event->jdevice.which);
		break;
	case SDL_JOYDEVICEREMOVED:
		for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
		{
			if (SDL_JoystickInstanceID(in_joysticks[i]) == event->jdevice.which)
				INL_StopJoy(i);
		}
		break;
#endif
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

	#endif
		for (int i = 0; i < IN_MAX_JOYSTICKS; i++)
		{
			if (IN_JoyPresent(i))
			{
				if (IN_GetJoyButtonsDB(i))
				{
					while (IN_GetJoyButtonsDB(i))
					{
						IN_PumpEvents();
						VL_Present();
					}
					return;
				}
			}
		}
		VL_Present();
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
	if (!in_disableJoysticks)
		SDL_Init(SDL_INIT_JOYSTICK);
	for (int i = 0; i < 256; ++i)
		in_keyStates[i] = 0;

	// Set the default kbd controls.
	INL_SetupKbdControls();
	
	// Setup any existing joysticks.
	if (!in_disableJoysticks)
	{
		int numJoys = SDL_NumJoysticks();
		for (int i = 0; i < numJoys; ++i)
			INL_StartJoy(i);
	}
}

// TODO: IMPLEMENT!
void IN_Default(bool gotit,int16_t inputChoice)
{
	in_controlType = (IN_ControlType)inputChoice;
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

void CK_US_UpdateOptionsMenus( void );

static bool INL_StartJoy(int joystick)
{
	if (in_disableJoysticks)
		return false;
	
	if (joystick > SDL_NumJoysticks())
		return false;
	
	int joystick_id = -1;
	
#if SDL_VERSION_ATLEAST(2,0,0)
	// On SDL2, with hotplug support, we can get hotplug events for joysticks
	// we've already got open. Check we don't have any duplicates here.
	SDL_JoystickGUID newGUID = SDL_JoystickGetDeviceGUID(joystick);
	for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
	{
		if (in_joystickPresent[i])
		{
			SDL_JoystickGUID GUID_i = SDL_JoystickGetGUID(in_joysticks[i]);
			if (!memcmp((void*)&newGUID, (void*)&GUID_i, sizeof(SDL_JoystickGUID)))
				return false;
		}
	}
#endif
	
	// Find an available joystick ID.
	for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
	{
		if (!in_joystickPresent[i])
		{
			joystick_id = i;
			break;
		}
	}
	
	if (joystick_id == -1)
		return false;
	
	in_joysticks[joystick_id] = SDL_JoystickOpen(joystick);
	
	in_joystickPresent[joystick_id] = true;
	
	CK_US_UpdateOptionsMenus();
	
	return true;
}

static void INL_StopJoy(int joystick)
{
	in_joystickPresent[joystick] = false;
	
	SDL_JoystickClose(in_joysticks[joystick]);

	// If the joystick is unplugged, switch to keyboard immediately.
	if (in_controlType == IN_ctrl_Joystick1 + joystick)
		in_controlType = IN_ctrl_Keyboard1;
	
	void CK_US_UpdateOptionsMenus();
	
}

bool IN_JoyPresent(int joystick)
{
	return in_joystickPresent[joystick];
}

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define IN_DO_DEADZONE(val, zone) (((val)*(val) < (zone)*(zone))?0:val) 

void IN_GetJoyAbs(int joystick, int *x, int *y)
{
	// We apply the XInput (Xbox 360 controller)'s right analogue stick's deadzone,
	// as it's one of the worst in common use.
	if (x)
		*x = IN_DO_DEADZONE (SDL_JoystickGetAxis(in_joysticks[joystick], 0), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	if (y)
		*y = IN_DO_DEADZONE (SDL_JoystickGetAxis(in_joysticks[joystick], 1), XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
}

uint16_t IN_GetJoyButtonsDB(int joystick)
{
	int button0 = SDL_JoystickGetButton(in_joysticks[joystick], 0);
	int button1 = SDL_JoystickGetButton(in_joysticks[joystick], 1);
	return (button0) | (button1 << 1);
}

void IN_SetControlType(int player, IN_ControlType type)
{
	in_controlType = type;
}

void IN_ReadCursor(IN_Cursor *cursor)
{
	cursor->button0 = false;
	cursor->button1 = false;
	cursor->xMotion = IN_motion_None;
	cursor->yMotion = IN_motion_None;
	if (in_controlType == IN_ctrl_Joystick1 || in_controlType == IN_ctrl_Joystick2)
	{
		int joy = in_controlType - IN_ctrl_Joystick1;
		int rawX, rawY;
		IN_GetJoyAbs(joy, &rawX, &rawY);
		cursor->xMotion = IN_motion_None;
		if (rawX < 0)
			cursor->xMotion = IN_motion_Left;
		else if (rawX > 0)
			cursor->xMotion = IN_motion_Right;
		cursor->yMotion = IN_motion_None;
		if (rawY < 0)
			cursor->yMotion = IN_motion_Up;
		else if (rawY > 0)
			cursor->yMotion = IN_motion_Down;
		
		uint16_t buttons = IN_GetJoyButtonsDB(joy);
		cursor->button0 = buttons & 1;
		cursor->button1 = buttons & 2;
	}
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
		if (in_controlType == IN_ctrl_Keyboard1 || in_controlType == IN_ctrl_Keyboard2)
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
		}
		else if (in_controlType == IN_ctrl_Joystick1 || in_controlType == IN_ctrl_Joystick2)
		{
			int joy = in_controlType - IN_ctrl_Joystick1;
			int rawX, rawY;
			IN_GetJoyAbs(joy, &rawX, &rawY);
			controls->xDirection = IN_motion_None;
			if (rawX < 0)
				controls->xDirection = IN_motion_Left;
			else if (rawX > 0)
				controls->xDirection = IN_motion_Right;
			controls->yDirection = IN_motion_None;
			if (rawY < 0)
				controls->yDirection = IN_motion_Up;
			else if (rawY > 0)
				controls->yDirection = IN_motion_Down;
			
			uint16_t buttons = IN_GetJoyButtonsDB(joy);
			controls->jump = buttons & 1;
			controls->pogo = buttons & 2;
		}

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
#endif

	for (int i = 0; i < IN_MAX_JOYSTICKS; i++)
	{
		if (IN_JoyPresent(i))
			if (IN_GetJoyButtonsDB(i))
				di = 1;
	}


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
		VL_Present();
	} while (SD_GetTimeCount() - lasttime < tics);

	return false;
}
