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

#include <SDL.h>
#include <string.h>

#define IN_MAX_JOYSTICKS 2

SDL_Joystick *in_joysticks[IN_MAX_JOYSTICKS];
bool in_joystickPresent[IN_MAX_JOYSTICKS];
static int in_joystickDeadzone = 8689;

// SDLKey -> IN_SC
#define INL_MapKey(sdl, in_sc) \
	case sdl:              \
		return in_sc

#if SDL_VERSION_ATLEAST(1, 3, 0)
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

	default:
		return IN_SC_Invalid;
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

	default:
		return IN_SC_Invalid;
	}
}
#endif

#undef INL_MapKey

static void IN_SDL_HandleSDLEvent(SDL_Event *event)
{

	IN_ScanCode sc;
	static bool special;

	switch (event->type)
	{
	case SDL_QUIT:
		Quit(0);
		break;
	case SDL_KEYDOWN:
		sc = INL_SDLKeySymToScanCode(&event->key.keysym);

		if (sc == 0xe0) // Special key prefix
			special = true;
		else
		{
			IN_HandleKeyDown(sc, special);
			special = false;
		}

		break;
	case SDL_KEYUP:
		sc = INL_SDLKeySymToScanCode(&event->key.keysym);
		IN_HandleKeyUp(sc, false);
		break;
#if SDL_VERSION_ATLEAST(2, 0, 0)
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
		if (event.type == SDL_KEYDOWN)
			break;
	}
}

void IN_SDL_Startup(bool disableJoysticks)
{
	if (!disableJoysticks)
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		int numJoys = SDL_NumJoysticks();
		for (int i = 0; i < numJoys; ++i)
			INL_StartJoy(i);
	}
}

bool IN_SDL_StartJoy(int joystick)
{
	if (joystick > SDL_NumJoysticks())
		return false;

	int joystick_id = -1;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	// On SDL2, with hotplug support, we can get hotplug events for joysticks
	// we've already got open. Check we don't have any duplicates here.
	SDL_JoystickGUID newGUID = SDL_JoystickGetDeviceGUID(joystick);
	for (int i = 0; i < IN_MAX_JOYSTICKS; ++i)
	{
		if (in_joystickPresent[i])
		{
			SDL_JoystickGUID GUID_i = SDL_JoystickGetGUID(in_joysticks[i]);
			if (!memcmp((void *)&newGUID, (void *)&GUID_i, sizeof(SDL_JoystickGUID)))
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

	return true;
}

void IN_SDL_StopJoy(int joystick)
{
	in_joystickPresent[joystick] = false;

	SDL_JoystickClose(in_joysticks[joystick]);
}

bool IN_SDL_JoyPresent(int joystick)
{
	return in_joystickPresent[joystick];
}

static int IN_ApplyDeadZone(int value)
{
	return (abs(value) <= in_joystickDeadzone) ? 0 : value;
}

void IN_SDL_JoyGetAbs(int joystick, int *x, int *y)
{
	if (x)
		*x = IN_ApplyDeadZone(SDL_JoystickGetAxis(in_joysticks[joystick], 0));
	if (y)
		*y = IN_ApplyDeadZone(SDL_JoystickGetAxis(in_joysticks[joystick], 1));
}

uint16_t IN_SDL_JoyGetButtons(int joystick)
{
	uint16_t mask = 0;
	int i, n = SDL_JoystickNumButtons(in_joysticks[joystick]);
	if (n > 16)
		n = 16;  /* the mask is just 16 bits wide anyway */
	for (i = 0;  i < n;  i++)
	{
		mask |= SDL_JoystickGetButton(in_joysticks[joystick], i) << i;
	}
	return mask;
}

void IN_SDL_JoySetDeadzone(int percent)
{
	if ((percent >= 0) && (percent <= 100))
		in_joystickDeadzone = (32768 * percent + 50) / 100;
}

const char* IN_SDL_JoyGetName(int joystick)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	return SDL_JoystickName(in_joysticks[joystick]);
#else
	return SDL_JoystickName(joystick);
#endif
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
	.joySetDeadzone = IN_SDL_JoySetDeadzone,
	.joyGetName = IN_SDL_JoyGetName,
};

IN_Backend *IN_Impl_GetBackend()
{
	return &in_sdl_backend;
}
