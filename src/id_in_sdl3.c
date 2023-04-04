/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2018 Omnispeak Authors

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

#include <SDL3/SDL.h>
#include <string.h>

#define IN_MAX_JOYSTICKS 2

SDL_Joystick *in_joysticks[IN_MAX_JOYSTICKS];
SDL_Gamepad *in_controllers[IN_MAX_JOYSTICKS];
bool in_joystickPresent[IN_MAX_JOYSTICKS];
bool in_joystickHasHat[IN_MAX_JOYSTICKS];

// SDLKey -> IN_SC
#define INL_MapKey(sdl, in_sc) \
	case sdl:              \
		return in_sc

static IN_ScanCode INL_SDLKeySymToScanCode(const SDL_KeyboardEvent *keySym)
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

	default:
		return IN_SC_Invalid;
	}
}

#undef INL_MapKey

static void IN_SDL_HandleSDLEvent(SDL_Event *event)
{

	IN_ScanCode sc;
	static bool special;

	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		Quit(0);
		break;
	case SDL_EVENT_KEY_DOWN:
		sc = INL_SDLKeySymToScanCode(&event->key);

		if (sc == 0xe0) // Special key prefix
			special = true;
		else
		{
			IN_HandleKeyDown(sc, special);
			special = false;
		}

		break;
	case SDL_EVENT_KEY_UP:
		sc = INL_SDLKeySymToScanCode(&event->key);
		IN_HandleKeyUp(sc, false);
		break;
	case SDL_EVENT_TEXT_INPUT:
		IN_HandleTextEvent(event->text.text);
		break;
	case SDL_EVENT_JOYSTICK_ADDED:
		INL_StartJoy(event->jdevice.which);
		break;
	case SDL_EVENT_JOYSTICK_REMOVED:
		for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
		{
			if (SDL_GetJoystickID(in_joysticks[i]) == event->jdevice.which)
				INL_StopJoy(i);
		}
		break;
	}
}

void IN_SDL_PumpEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
		IN_SDL_HandleSDLEvent(&event);
}

void IN_SDL_WaitKey()
{
	SDL_Event event;
	while (SDL_WaitEvent(&event))
	{
		IN_SDL_HandleSDLEvent(&event);
		if (event.type == SDL_EVENT_KEY_DOWN)
			break;
	}
}

void IN_SDL_Startup(bool disableJoysticks)
{
	if (!disableJoysticks)
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		int numJoys;
		const SDL_JoystickID *joys = SDL_GetJoysticks(&numJoys);
		for (int i = 0; i < numJoys; ++i)
			INL_StartJoy(joys[i]);
	}
}

bool IN_SDL_StartJoy(int joystick)
{
	int joystick_id = -1;

	// On SDL2, with hotplug support, we can get hotplug events for joysticks
	// we've already got open. Check we don't have any duplicates here.
	SDL_GUID newGUID = SDL_GetJoystickGUIDForID(joystick);
	for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
	{
		if (in_joystickPresent[i])
		{
			SDL_GUID GUID_i = SDL_GetJoystickGUID(in_joysticks[i]);
			if (!memcmp((void *)&newGUID, (void *)&GUID_i, sizeof(SDL_GUID)))
				return false;
		}
	}

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

	in_joysticks[joystick_id] = SDL_OpenJoystick(joystick);

	in_joystickPresent[joystick_id] = true;

	in_joystickHasHat[joystick_id] = (SDL_GetNumJoystickHats(in_joysticks[joystick_id]) > 0);

	if (SDL_IsGamepad(joystick))
	{
		in_controllers[joystick_id] = SDL_OpenGamepad(joystick);
	}

	return true;
}

void IN_SDL_StopJoy(int joystick)
{
	in_joystickPresent[joystick] = false;
	if (in_controllers[joystick])
		SDL_CloseGamepad(in_controllers[joystick]);

	SDL_CloseJoystick(in_joysticks[joystick]);
}

bool IN_SDL_JoyPresent(int joystick)
{
	return in_joystickPresent[joystick];
}

void IN_SDL_JoyGetAbs(int joystick, int *x, int *y)
{
	int value_x = SDL_GetJoystickAxis(in_joysticks[joystick], 0);
	int value_y = SDL_GetJoystickAxis(in_joysticks[joystick], 1);
#if !defined(CK_VANILLA)
	static const int HAT = 32000;
	if (in_joystickHasHat[joystick]) {
		switch (SDL_GetJoystickHat(in_joysticks[joystick], 0)) {
			case SDL_HAT_LEFTUP:     value_x = -HAT;  value_y = -HAT;  break;
			case SDL_HAT_UP:                          value_y = -HAT;  break;
			case SDL_HAT_RIGHTUP:    value_x = +HAT;  value_y = -HAT;  break;
			case SDL_HAT_LEFT:       value_x = -HAT;                   break;
			case SDL_HAT_RIGHT:      value_x = +HAT;                   break;
			case SDL_HAT_LEFTDOWN:   value_x = -HAT;  value_y = +HAT;  break;
			case SDL_HAT_DOWN:                        value_y = +HAT;  break;
			case SDL_HAT_RIGHTDOWN:  value_x = +HAT;  value_y = +HAT;  break;
			default:                                                   break;
		}
	}
#endif
	if (x)
		*x = value_x;
	if (y)
		*y = value_y;
}

uint16_t IN_SDL_JoyGetButtons(int joystick)
{
	uint16_t mask = 0;
	int i, n = SDL_GetNumJoystickButtons(in_joysticks[joystick]);
	if (n > 16)
		n = 16;  /* the mask is just 16 bits wide anyway */
	for (i = 0;  i < n;  i++)
	{
		if (in_controllers[joystick])
			mask |= SDL_GetGamepadButton(in_controllers[joystick], (SDL_GamepadButton)i) << i;
		else
		mask |= SDL_GetJoystickButton(in_joysticks[joystick], i) << i;
	}
	return mask;
}

const char* IN_SDL_JoyGetName(int joystick)
{
	return SDL_GetJoystickName(in_joysticks[joystick]);
}

const char* IN_SDL_JoyGetButtonName(int joystick, int index)
{
	if (in_controllers[joystick])
	{
		return SDL_GetGamepadStringForButton((SDL_GamepadButton)index);
	}
	return NULL;
}

extern SDL_Window *vl_sdl3_window;

void IN_SDL_StartTextInput(const char *reason, const char *oldText)
{
	SDL_StartTextInput(vl_sdl3_window);
}

void IN_SDL_StopTextInput()
{
	SDL_StopTextInput(vl_sdl3_window);
}

IN_Backend in_sdl_backend = {
	.startup = IN_SDL_Startup,
	.shutdown = 0,
	.pumpEvents = IN_SDL_PumpEvents,
	.waitKey = IN_SDL_WaitKey,
	.joyStart = IN_SDL_StartJoy,
	.joyStop = IN_SDL_StopJoy,
	.joyPresent = IN_SDL_JoyPresent,
	.joyGetAbs = IN_SDL_JoyGetAbs,
	.joyGetButtons = IN_SDL_JoyGetButtons,
	.joyGetName = IN_SDL_JoyGetName,
	.joyGetButtonName = IN_SDL_JoyGetButtonName,
	.startTextInput = IN_SDL_StartTextInput,
	.stopTextInput = IN_SDL_StopTextInput,
	.joyAxisMin = -32768,
	.joyAxisMax =  32767,
	.supportsTextEvents = true,
};

IN_Backend *IN_Impl_GetBackend()
{
	return &in_sdl_backend;
}
