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

#include "id_cfg.h"
#include "id_in.h"
#include "id_mm.h"
#include "id_sd.h"
#include "id_us.h"
#include "id_vl.h"
#include "id_fs.h"
#include "ck_cross.h"

#include <stdlib.h> /* For abs() */
#include <string.h>

char nonchar_keys[] = {
	0x01, 0x0E, 0x0F, 0x1D,
	0x2A, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43,
	0x44, 0x57, 0x59, 0x46,
	0x1C, 0x36, 0x37, 0x38,
	0x47, 0x49, 0x4F, 0x51,
	0x52, 0x53, 0x45, 0x48,
	0x50, 0x4B, 0x4D, 0x00};
const char *nonchar_key_strings[] = {
	"Esc", "Bksp", "Tab", "Ctrl",
	"LShft", "Space", "CapsLk", "F1",
	"F2", "F3", "F4", "F5",
	"F6", "F7", "F8", "F9",
	"F10", "F11", "F12", "ScrlLk",
	"Enter", "RShft", "PrtSc", "Alt",
	"Home", "PgUp", "End", "PgDn",
	"Ins", "Del", "NumLk", "Up",
	"Down", "Left", "Right", ""};
const char *char_key_strings[] = {
	/*	 0	 	 1	 	 2	 	 3		 4	     5	 	 6	 	7	     */
	/*	 8	 	 9	 	 A	 	 B		 C		 D 		 E		 F	     */
	"?", "?", "1", "2", "3", "4", "5", "6", /* 0 */
	"7", "8", "9", "0", "-", "+", "?", "?",
	"Q", "W", "E", "R", "T", "Y", "U", "I", /* 1 */
	"O", "P", "[", "]", "|", "?", "A", "S",
	"D", "F", "G", "H", "J", "K", "L", ";", /* 2 */
	"\"", "?", "?", "?", "Z", "X", "C", "V",
	"B", "N", "M", ",", ".", "/", "?", "?", /* 3 */
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?", /* 4 */
	"\x0F", "?", "-", "\x15", "5", "\x11", "+", "?",
	"\x13", "?", "?", "?", "?", "?", "?", "?", /* 5 */
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?", /* 6 */
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?", /* 7 */
	"?", "?", "?", "?", "?", "?", "?", "?"};

// =========================================================================
static char in_ASCIINames[] = // Unshifted ASCII for scan codes
	{
		//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
		0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9,     // 0
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0, 'a', 's', // 1
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 0, 92, 'z', 'x', 'c', 'v',  // 2
		'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,		     // 3
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',	    // 4
		'2', '3', '0', 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			     // 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				     // 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0				     // 7
},
	    in_shiftNames[] = // Shifted ASCII for scan codes
	{
		//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
		0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9,     // 0
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0, 'A', 'S', // 1
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34, '~', 0, '|', 'Z', 'X', 'C', 'V', // 2
		'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,		     // 3
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',	    // 4
		'2', '3', '0', 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			     // 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				     // 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0				     // 7
},
	    in_specialNames[] = // ASCII for 0xe0 prefixed codes
	{
		//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0,  // 1
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 2
		0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 4
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 5
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 6
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // 7
};

// If we haven't set the default to anything, Keyboard1 is vanilla.
#ifndef DEFAULT_INPUT
#define DEFAULT_INPUT IN_ctrl_Keyboard1
#endif

bool in_Paused;
const char *in_PausedMessage = "PAUSED";
IN_ControlType in_controlType = DEFAULT_INPUT;
IN_ControlType in_backupControlType = IN_ctrl_Keyboard1;
bool in_keyStates[256];
IN_ScanCode in_lastKeyScanned = IN_SC_None;
bool in_useTextEvents = false;
char in_lastASCII;
bool in_disableJoysticks = false;
bool INL_StartJoy(int joystick);
void INL_StopJoy(int joystick);

IN_Backend *in_backend = 0;

#define IN_MAX_JOYSTICKS 2

const char *IN_ControlType_Strings[] = {
	"Keyboard1",
	"Keyboard2",
	"Joystick1",
	"Joystick2",
	"Mouse",
	"All"
};

IN_KeyMapping in_kbdControls;

// In Omnispeak, which doesn't support the classic Gravis Gamepad anyway,
// we misuse in_gamepadButtons to store more buttons (fire and menu);
// this way we get persistence "for free", as the old gamepadButtons
// configuration is stored in the CONFIG.CK? files
int16_t in_gamepadButtons[4];

IN_ScanCode *in_key_controls[] = {
	&in_kbdControls.jump, &in_kbdControls.pogo, &in_kbdControls.fire,
#ifdef EXTRA_KEYBOARD_OPTIONS
	&in_kbdControls.status,
	&in_kbdControls.toggleScorebox,
#endif
#ifdef QUICKSAVE_ENABLED
	&in_kbdControls.quickSave, &in_kbdControls.quickLoad,
#endif
	&in_kbdControls.upLeft, &in_kbdControls.up, &in_kbdControls.upRight,
	&in_kbdControls.right, &in_kbdControls.downRight, &in_kbdControls.down,
	&in_kbdControls.downLeft, &in_kbdControls.left};
const int in_key_button_controls = 3 +
#ifdef EXTRA_KEYBOARD_OPTIONS
	2 +
#endif
#ifdef QUICKSAVE_ENABLED
	2 +
#endif
	0;
const int in_key_direction_controls = 8;

static bool CapsLock;

static IN_Direction in_dirTable[] = // Quick lookup for total direction
	{
		IN_dir_NorthWest, IN_dir_North, IN_dir_NorthEast,
		IN_dir_West, IN_dir_None, IN_dir_East,
		IN_dir_SouthWest, IN_dir_South, IN_dir_SouthEast};

bool in_joyAdvancedMotion = true;
int16_t in_joyDeadzonePercent = 30;
static int in_joyCachedDeadzone = 0;
static int in_joyScaledDeadzone = 0;

// the "diagonal slope" selects the sensitivity of the joystick
// regarding diagonal directions
//#define IN_JOYSTICK_DIAGONAL_SLOPE   0,1    // only diagonals (not a good idea!)
#define IN_JOYSTICK_DIAGONAL_SLOPE 1, 3 // slight preference for diagonals
//#define IN_JOYSTICK_DIAGONAL_SLOPE  29,70   // diagonals have same sensitivity as main directions
//#define IN_JOYSTICK_DIAGONAL_SLOPE   1,2    // slight preference for main directions
//#define IN_JOYSTICK_DIAGONAL_SLOPE   1,1    // no diagonals at all (not a good idea!)
static const int in_joystickDiagonalSlope[2] = {IN_JOYSTICK_DIAGONAL_SLOPE};

void IN_HandleKeyUp(IN_ScanCode sc, bool special)
{
	//Use F11 to toggle fullscreen.
	if (sc == IN_SC_F11)
		VL_ToggleFullscreen();

	in_keyStates[sc] = 0;
}

void IN_HandleKeyDown(IN_ScanCode sc, bool special)
{
	in_keyStates[sc] = true;
	in_lastKeyScanned = sc;
	char c;

	if (sc == 0xe0) // Special key prefix
		special = true;
	else if (sc == IN_SC_Pause) // Handle Pause key
		in_Paused = true;
	else
	{
		if (sc & 0x80) // Break code
		{
			sc &= 0x7f;

			// DEBUG - handle special keys: ctl-alt-delete, print scrn
			in_keyStates[sc] = false;
		}
		else // Make code
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

				if (in_keyStates[IN_SC_LeftShift] || in_keyStates[IN_SC_RightShift]) // If shifted
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
			if (!in_useTextEvents && c)
				in_lastASCII = c;
		}
	}
}

void IN_HandleTextEvent(const char *utf8Text)
{
	if (!in_useTextEvents)
		return;
	//TODO: For now, only permit ASCII.
	if (((unsigned char*)utf8Text)[0] < 0x80)
		in_lastASCII = utf8Text[0];
}

void IN_SetupKbdControls()
{
	in_kbdControls.jump = CFG_GetConfigInt("in_kbd_jump", IN_SC_Control);
	in_kbdControls.pogo = CFG_GetConfigInt("in_kbd_pogo", IN_SC_Alt);
	in_kbdControls.fire = CFG_GetConfigInt("in_kbd_fire", IN_SC_Space);
#ifdef EXTRA_KEYBOARD_OPTIONS
	in_kbdControls.status = CFG_GetConfigInt("in_kbd_status", IN_SC_Enter);
	in_kbdControls.toggleScorebox = CFG_GetConfigInt("in_kbd_toggleScorebox", IN_SC_Backspace);
#endif
#ifdef QUICKSAVE_ENABLED
	in_kbdControls.quickSave = CFG_GetConfigInt("in_kbd_quickSave", IN_SC_F5);
	in_kbdControls.quickLoad = CFG_GetConfigInt("in_kbd_quickLoad", IN_SC_F9);;
#endif
	in_kbdControls.up = CFG_GetConfigInt("in_kbd_up", IN_SC_UpArrow);
	in_kbdControls.down = CFG_GetConfigInt("in_kbd_down", IN_SC_DownArrow);
	in_kbdControls.left = CFG_GetConfigInt("in_kbd_left", IN_SC_LeftArrow);
	in_kbdControls.right = CFG_GetConfigInt("in_kbd_right", IN_SC_RightArrow);
	in_kbdControls.upLeft = CFG_GetConfigInt("in_kbd_upLeft", IN_SC_Home);
	in_kbdControls.upRight = CFG_GetConfigInt("in_kbd_upRight", IN_SC_PgUp);
	in_kbdControls.downRight = CFG_GetConfigInt("in_kbd_downRight", IN_SC_PgDown);
	in_kbdControls.downLeft = CFG_GetConfigInt("in_kbd_downLeft", IN_SC_End);
}

void IN_SaveKbdControls()
{
	if (CFG_GetConfigBool("in_preferOmnispeakKeyConfig", true))
	{
		CFG_SetConfigInt("in_kbd_jump", in_kbdControls.jump);
		CFG_SetConfigInt("in_kbd_pogo", in_kbdControls.pogo);
		CFG_SetConfigInt("in_kbd_fire", in_kbdControls.fire);

		CFG_SetConfigInt("in_kbd_up", in_kbdControls.up);
		CFG_SetConfigInt("in_kbd_down", in_kbdControls.down);
		CFG_SetConfigInt("in_kbd_left", in_kbdControls.left);
		CFG_SetConfigInt("in_kbd_right", in_kbdControls.right);
		CFG_SetConfigInt("in_kbd_upLeft", in_kbdControls.upLeft);
		CFG_SetConfigInt("in_kbd_upRight", in_kbdControls.upRight);
		CFG_SetConfigInt("in_kbd_downRight", in_kbdControls.downRight);
		CFG_SetConfigInt("in_kbd_downLeft", in_kbdControls.downLeft);
	}

#ifdef EXTRA_KEYBOARD_OPTIONS
	CFG_SetConfigInt("in_kbd_status", in_kbdControls.status);
	CFG_SetConfigInt("in_kbd_toggleScorebox", in_kbdControls.toggleScorebox);
#endif
#ifdef QUICKSAVE_ENABLED
	CFG_SetConfigInt("in_kbd_quickSave", in_kbdControls.quickSave);
	CFG_SetConfigInt("in_kbd_quickLoad", in_kbdControls.quickLoad);;
#endif
}

void IN_PumpEvents()
{
	in_backend->pumpEvents();
}

const char *IN_GetScanName(IN_ScanCode scan)
{
	char *p;
	const char **ps;

	p = nonchar_keys;
	ps = nonchar_key_strings;

	while (*p)
	{
		if (*p == scan)
			return *ps;

		ps++;
		p++;
	}
	if (scan < (int)(sizeof(char_key_strings) / sizeof(char_key_strings[0])))
		return char_key_strings[scan];
	else
		return "?";
}

void IN_WaitKey()
{
	in_backend->waitKey();
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

void IN_StartTextInput(const char *reason, const char *existing)
{
	if (in_backend->startTextInput)
		in_backend->startTextInput(reason, existing);
}

void IN_StopTextInput()
{
	if (in_backend->stopTextInput)
		in_backend->stopTextInput();
}

void IN_Startup(void)
{
	in_backend = IN_Impl_GetBackend();
	for (int i = 0; i < 256; ++i)
		in_keyStates[i] = 0;

#ifdef CK_VANILLA
	in_useTextEvents = CFG_GetConfigBool("in_useTextEvents", false);
#else
	in_useTextEvents = CFG_GetConfigBool("in_useTextEvents", in_backend->supportsTextEvents);
#endif

	if (in_useTextEvents && !in_backend->supportsTextEvents)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Requested text event support, but backend doesn't support them!\n");
		in_useTextEvents = false;
	}

	// Set the default kbd controls.
	IN_SetupKbdControls();

	in_backend->startup(in_disableJoysticks);
}

// TODO: IMPLEMENT!
void IN_Default(bool gotit, int16_t inputChoice)
{
#ifdef FORCE_DEFAULT_INPUT
	inputChoice = DEFAULT_INPUT;
#endif
	in_controlType = (IN_ControlType)inputChoice;
	if (inputChoice != IN_ctrl_All)
	    in_backupControlType = (IN_ControlType)inputChoice;
}

uint8_t *in_demoBuf;
int in_demoPtr;
int in_demoBytes;
IN_DemoMode in_demoState;

bool IN_DemoStartRecording(int bufferSize)
{
	if (!bufferSize)
		return false;
	MM_GetPtr((mm_ptr_t *)&in_demoBuf, bufferSize);

	in_demoState = IN_Demo_Record;
	in_demoBytes = bufferSize & ~1;
	in_demoPtr = 0;

	in_demoBuf[0] = 0;
	in_demoBuf[1] = 0;

	return true;
}

void IN_DemoStartPlaying(uint8_t *data, int len)
{
	in_demoBuf = data;
	in_demoBytes = len;
	in_demoPtr = 0;
	in_demoState = IN_Demo_Playback;
}

void IN_DemoStopPlaying()
{
	if (in_demoState == IN_Demo_Record && in_demoPtr != 0)
		in_demoPtr += 2;

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

void IN_DemoFreeBuffer()
{
	if (in_demoBuf)
		MM_FreePtr((mm_ptr_t *)&in_demoBuf);
}

void IN_DemoSaveToFile(const char *fileName, uint16_t mapNumber)
{
	uint16_t demoSize = in_demoPtr;
	FS_File demoFile = FS_CreateUserFile(fileName);
	if (!demoFile)
		Quit("Couldn't open demo file for writing!");
	FS_WriteInt16LE(&mapNumber, 1, demoFile);
	FS_WriteInt16LE(&demoSize, 1, demoFile);

	FS_Write(in_demoBuf, in_demoPtr, 1, demoFile);

	FS_CloseFile(demoFile);
}

void IN_ClearKeysDown()
{
	in_lastKeyScanned = IN_SC_None;
	in_lastASCII = IN_KP_None;
	memset(in_keyStates, 0, sizeof(in_keyStates));
}

void IN_ClearKey(IN_ScanCode key)
{
	if (in_lastKeyScanned == key)
		in_lastKeyScanned = IN_SC_None;

	in_keyStates[key] = false;
}

void CK_US_UpdateOptionsMenus(void);

bool INL_StartJoy(int joystick)
{
	if (in_disableJoysticks)
		return false;

	in_backend->joyStart(joystick);

	CK_US_UpdateOptionsMenus();

	return true;
}

void INL_StopJoy(int joystick)
{
	in_backend->joyStop(joystick);

	// If the joystick is unplugged, switch to default immediately.
	if (in_controlType == IN_ctrl_Joystick1 + joystick)
		in_controlType = DEFAULT_INPUT;

	CK_US_UpdateOptionsMenus();
}

bool IN_JoyPresent(int joystick)
{
	return in_backend->joyPresent(joystick);
}

void IN_GetJoyAbs(int joystick, int *x, int *y)
{
	in_backend->joyGetAbs(joystick, x, y);
}

uint16_t IN_GetJoyButtonsDB(int joystick)
{
	return in_backend->joyGetButtons(joystick);
}

void IN_SetControlType(int player, IN_ControlType type)
{
#ifdef FORCE_DEFAULT_INPUT
	type = DEFAULT_INPUT;
#endif
	in_controlType = type;

	// Save the control type if we need to
	if (CFG_GetConfigEnum("in_controlType", IN_ControlType_Strings, -1) != -1)
	{
		CFG_SetConfigEnum("in_controlType", IN_ControlType_Strings, in_controlType);
	}
}

void IN_GetJoyMotion(int joystick, IN_Motion *p_x, IN_Motion *p_y)
{
	int valX, valY, resX, resY, signX, signY;

	// update the pre-computed deadzone threshold
	if (in_joyDeadzonePercent != in_joyCachedDeadzone)
	{
		in_joyCachedDeadzone = in_joyDeadzonePercent;
		in_joyScaledDeadzone = (in_backend->joyAxisMax - in_backend->joyAxisMin) * in_joyCachedDeadzone / 200;
		in_joyScaledDeadzone *= in_joyScaledDeadzone;
	}

	// get raw data from joystick and re-center it
	IN_GetJoyAbs(joystick, &valX, &valY);
	valX -= (in_backend->joyAxisMin + in_backend->joyAxisMax) / 2;
	valY -= (in_backend->joyAxisMin + in_backend->joyAxisMax) / 2;

	// Now "quantize" the raw joystick position into one of the nine
	// discrete positions we need in the game (center / neutral, four main
	// directions, four diagonals).
#ifdef EXTRA_JOYSTICK_OPTIONS
	if (in_joyAdvancedMotion)
	{
		// "Advanced" (or "modern") quantization: Apply a circular
		// "deadzone" at the center. The remaining coordinates are
		// split into eight radial segments, whereby the size of the
		// segments for the main directions versus the diagonals can be
		// tuned using the slope parameters at the beginning of this
		// file. (This could easily be turned into an option later on.)

		// extract the quadrant and map values into the upper-right quadrant
		signX = valX >> 31;
		signY = valY >> 31;
		// We need to clamp these at 32767 because:
		// - -32768 is a valid int16_t, which (e.g.) SDL returns
		// - the deadzone calculation below will overflow if both values
		//   are 32768, causing the deadzone check to fail if the stick
		//   is all the way to the top-left.
		valX = CK_Cross_min(CK_Cross_abs(valX), 32767);
		valY = CK_Cross_min(CK_Cross_abs(valY), 32767);

		// check against the deadzone first
		if ((valX * valX + valY * valY) <= in_joyScaledDeadzone)
		{
			resX = resY = 0;
		}
		else
		{
			// not in deadzone -> classify into main horizontal,
			// main vertical or diagonal directions
			resX = (valY * in_joystickDiagonalSlope[0] > valX * in_joystickDiagonalSlope[1]) ? 0 : 1;
			resY = (valX * in_joystickDiagonalSlope[0] > valY * in_joystickDiagonalSlope[1]) ? 0 : 1;
		}

		// flip the result back into the proper quadrant
		resX = (resX ^ signX) - signX;
		resY = (resY ^ signY) - signY;
	}
	else
#endif
	{
		// "Simple" (or "classic") quantization: Apply the deadzone
		// separately for each component and map every non-dead
		// value to its direction
		resX = ((valX * valX) <= in_joyScaledDeadzone) ? 0 : valX;
		resY = ((valY * valY) <= in_joyScaledDeadzone) ? 0 : valY;
	}

	// finally, map the values to the IN_Motion constants
	if (p_x)
	{
		if (resX < 0)
			*p_x = IN_motion_Left;
		else if (resX > 0)
			*p_x = IN_motion_Right;
		else
			*p_x = IN_motion_None;
	}
	if (p_y)
	{
		if (resY < 0)
			*p_y = IN_motion_Up;
		else if (resY > 0)
			*p_y = IN_motion_Down;
		else
			*p_y = IN_motion_None;
	}
}

void IN_ReadCursor(IN_Cursor *cursor)
{
	bool forceJoyMenu = CFG_GetConfigBool("in_forceJoyMenu", CK_NEW_FEATURE_DEFAULT);
	cursor->button0 = false;
	cursor->button1 = false;
	cursor->xMotion = IN_motion_None;
	cursor->yMotion = IN_motion_None;
	if (forceJoyMenu || in_controlType == IN_ctrl_Joystick1 || in_controlType == IN_ctrl_Joystick2 || in_controlType == IN_ctrl_All)
	{
		int joy = in_controlType - IN_ctrl_Joystick1;
		if (joy < 0 || joy > 1)
			joy = 0;
		IN_GetJoyMotion(joy, &cursor->xMotion, &cursor->yMotion);

		uint16_t buttons = IN_GetJoyButtonsDB(joy);
#ifdef EXTRA_JOYSTICK_OPTIONS
		cursor->button0 = IN_GetJoyButtonFromMask(buttons, IN_joy_jump);
		cursor->button1 = IN_GetJoyButtonFromMask(buttons, IN_joy_pogo);
#else
		cursor->button0 = buttons & 1;
		cursor->button1 = (buttons >> 1) & 1;
#endif
	}
}

void IN_ReadControls(int player, IN_ControlFrame *controls)
{
	controls->xDirection = IN_motion_None;
	controls->yDirection = IN_motion_None;
	controls->jump = false;
	controls->pogo = false;
	controls->button2 = false;
	controls->button3 = false;

	if (in_demoState == IN_Demo_Playback)
	{
		uint8_t ctrlByte = in_demoBuf[in_demoPtr + 1];
		controls->yDirection = (IN_Motion)((ctrlByte & 3) - 1);
		controls->xDirection = (IN_Motion)(((ctrlByte >> 2) & 3) - 1);
		controls->jump = (ctrlByte >> 4) & 1;
		controls->pogo = (ctrlByte >> 5) & 1;

		// Delay for n frames.
		if ((--in_demoBuf[in_demoPtr]) == 0)
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
		if (in_controlType == IN_ctrl_Joystick1 || in_controlType == IN_ctrl_Joystick2 || in_controlType == IN_ctrl_All)
		{
			int joy = in_controlType - IN_ctrl_Joystick1;

			if (joy < 0 || joy > 1)
				joy = 0;

			IN_GetJoyMotion(joy, &controls->xDirection, &controls->yDirection);

			uint16_t buttons = IN_GetJoyButtonsDB(joy);
#ifdef EXTRA_JOYSTICK_OPTIONS
			controls->jump = IN_GetJoyButtonFromMask(buttons, IN_joy_jump);
			controls->pogo = IN_GetJoyButtonFromMask(buttons, IN_joy_pogo);
			controls->button2 = IN_GetJoyButtonFromMask(buttons, IN_joy_fire);
			controls->button3 = IN_GetJoyButtonFromMask(buttons, IN_joy_menu);
#else
			controls->jump = buttons & 1;
			controls->pogo = (buttons >> 1) & 1;
			controls->button2 = 0;
			controls->button3 = 0;
#endif
		}
		if (in_controlType == IN_ctrl_Keyboard1 || in_controlType == IN_ctrl_Keyboard2 || in_controlType == IN_ctrl_All)
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

			if (IN_GetKeyState(in_kbdControls.up))
				controls->yDirection = IN_motion_Up;
			else if (IN_GetKeyState(in_kbdControls.down))
				controls->yDirection = IN_motion_Down;

			if (IN_GetKeyState(in_kbdControls.left))
				controls->xDirection = IN_motion_Left;
			else if (IN_GetKeyState(in_kbdControls.right))
				controls->xDirection = IN_motion_Right;

			if (IN_GetKeyState(in_kbdControls.jump))
				controls->jump = true;
			if (IN_GetKeyState(in_kbdControls.pogo))
				controls->pogo = true;
			// TODO: Handle fire input separately? (e.g. two-button firing)
			if (IN_GetKeyState(in_kbdControls.fire))
				controls->button2 = true;
		}

		controls->dir = in_dirTable[3 * (controls->yDirection + 1) + controls->xDirection + 1];
	}

	if (in_demoState == IN_Demo_Record)
	{
		uint8_t ctrlByte = 0;
		ctrlByte |= (controls->yDirection + 1);
		ctrlByte |= (controls->xDirection + 1) << 2;
		ctrlByte |= (controls->jump) << 4;
		ctrlByte |= (controls->pogo) << 5;

		// If the controls haven't changed…
		if ((in_demoBuf[in_demoPtr + 1] == ctrlByte) &&
			// and we have room left…
			(in_demoBuf[in_demoPtr] < 254) &&
			// and it isn't the first frame
			(in_demoPtr != 0))
		{
			// The current input lasts for another frame.
			in_demoBuf[in_demoPtr]++;
		}
		else
		{
			// We have new input, record it.

			// Use the first slot if it is empty.
			if (in_demoPtr || in_demoBuf[in_demoPtr])
				in_demoPtr += 2;

			if (in_demoPtr >= in_demoBytes)
				Quit("Demo buffer overflow");

			// One frame…
			in_demoBuf[in_demoPtr] = 1;
			// with these controls:
			in_demoBuf[in_demoPtr + 1] = ctrlByte;
		}
	}
}

void IN_WaitButton()
{
	// Zero all of the input
	IN_PumpEvents();

	in_keyStates[in_lastKeyScanned] = 0;

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

// Must be in-sync with IN_JoyConfItem
const char *IN_JoyConfNames[] = {
	"in_joy_jump",
	"in_joy_pogo",
	"in_joy_fire",
	"in_joy_menu",
	"in_joy_status",
	"in_joy_quickload",
	"in_joy_quicksave",
	"in_joy_deadzone",
	"in_joy_modern"
};

int IN_GetJoyConf(IN_JoyConfItem item)
{
	return CFG_GetConfigInt(IN_JoyConfNames[item], item);
}

void IN_SetJoyConf(IN_JoyConfItem item, int value)
{
	CFG_SetConfigInt(IN_JoyConfNames[item], value);
}

bool IN_GetJoyButtonFromMask(uint16_t mask, IN_JoyConfItem btn)
{
	int btn_id = IN_GetJoyConf(btn);
	return (btn_id < 0) ? 0 : ((mask >> btn_id) & 1);
}

bool IN_IsJoyButtonDown(IN_JoyConfItem btn)
{
	int joy = in_controlType - IN_ctrl_Joystick1;
	if (joy < 0 || joy >= IN_MAX_JOYSTICKS) joy = 0;
	uint16_t mask = IN_GetJoyButtonsDB(joy);
	return IN_GetJoyButtonFromMask(mask, btn);
}

const char *IN_GetJoyName(int joystick)
{
	return (in_backend->joyPresent(joystick) && in_backend->joyGetName)
		? in_backend->joyGetName(joystick)
		: NULL;
}

const char *IN_GetJoyButtonName(int joystick, int button)
{
	return (in_backend->joyPresent(joystick) && in_backend->joyGetButtonName)
		? in_backend->joyGetButtonName(joystick, button)
		: NULL;
}
