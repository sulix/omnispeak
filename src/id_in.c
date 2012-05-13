#include "id_in.h"
#include "id_us.h"
#include <SDL/SDL.h>

bool in_keyStates[256];

// SDLK -> IN_SC
#define INL_MapKey(sdl,in_sc) case sdl: return in_sc

IN_ScanCode INL_SDLKToScanCode(int sdlKey)
{
	switch(sdlKey)
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
		INL_MapKey(SDLK_F1, IN_SC_F1);
		INL_MapKey(SDLK_F2, IN_SC_F2);
		INL_MapKey(SDLK_F3, IN_SC_F3);
		INL_MapKey(SDLK_F4, IN_SC_F4);
		INL_MapKey(SDLK_F5, IN_SC_F5);
		INL_MapKey(SDLK_F6, IN_SC_F6);
		INL_MapKey(SDLK_F7, IN_SC_F7);
		
		INL_MapKey(SDLK_F9, IN_SC_F9);
		INL_MapKey(SDLK_F10, IN_SC_F10);
		INL_MapKey(SDLK_F11, IN_SC_F11);
		INL_MapKey(SDLK_F12, IN_SC_F12);
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
		default: return IN_SC_Invalid;
	}
}

#undef INL_MapKey

static IN_KeyMapping in_kbdControls;

static void INL_HandleSDLEvent(SDL_Event *event)
{
	IN_ScanCode sc;
	switch (event->type)
	{
	case SDL_QUIT:
		Quit(0);
		break;
	case SDL_KEYDOWN:
		sc = INL_SDLKToScanCode(event->key.keysym.sym);
		in_keyStates[sc] = 1;
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
	in_kbdControls.up = IN_SC_UpArrow;
	in_kbdControls.down = IN_SC_DownArrow;
	in_kbdControls.left = IN_SC_LeftArrow;
	in_kbdControls.right = IN_SC_RightArrow;
	//TODO: Diagonals
}

void IN_PumpEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
		INL_HandleSDLEvent(&event);
}

void IN_WaitKey()
{
	SDL_Event event;
	while (SDL_WaitEvent(&event))
	{
		INL_HandleSDLEvent(&event);
		if(event.type == SDL_KEYDOWN)
			break;
	}
}

bool IN_GetKeyState(IN_ScanCode scanCode)
{
	return in_keyStates[scanCode];
}

void IN_Startup()
{
	for (int i = 0; i < 256; ++i)
		in_keyStates[i] = 0;

	// Set the default kbd controls.
	INL_SetupKbdControls();
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

bool IN_DemoIsPlaying()
{
	return in_demoState == IN_Demo_Playback;
}

void IN_ReadControls(int player, IN_ControlFrame *controls)
{
	controls->xDirection = 0;
	controls->yDirection = 0;
	controls->jump = false;
	controls->pogo = false;

	if (in_demoState == IN_Demo_Playback)
	{
		uint8_t ctrlByte = in_demoBuf[in_demoPtr + 1];
		controls->yDirection = (ctrlByte & 3) - 1;
		controls->xDirection = ((ctrlByte >> 2) & 3) - 1;
		controls->jump = (ctrlByte >> 4) & 1;
		controls->pogo = (ctrlByte >> 5) & 1;

		printf("Demo --- ctrl byte %d, timer byte %d at offset %d of %d\n", ctrlByte, in_demoBuf[in_demoPtr], in_demoPtr, in_demoBytes);

		printf("Got a demo ctrl byte %d: xd = %d, yd= %d, jmp = %d, pgo = %d\n", ctrlByte, controls->xDirection, controls->yDirection, controls->jump, controls->pogo);

		// Delay for n frames.
		if (in_demoBuf[in_demoPtr])
			(in_demoBuf[in_demoPtr])--;

		if (!in_demoBuf[in_demoPtr])
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
		if (IN_GetKeyState(in_kbdControls.jump)) controls->jump = true;
		if (IN_GetKeyState(in_kbdControls.pogo)) controls->pogo = true;

		if (IN_GetKeyState(in_kbdControls.up)) controls->yDirection = -1;
		else if (IN_GetKeyState(in_kbdControls.down)) controls->yDirection = 1;

		if (IN_GetKeyState(in_kbdControls.left)) controls->xDirection = -1;
		else if (IN_GetKeyState(in_kbdControls.right)) controls->xDirection = 1;
	}

	//TODO: Record this inputFrame
}
