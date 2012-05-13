#ifndef ID_IN_H
#define ID_IN_H

#include <stdint.h>
#include <stdbool.h>
//
// These scancodes match keen (and wolf3d)'s numbering, such that
// patches and config files may be compatible.
typedef enum IN_ScanCode
{
	IN_SC_None = 0x00,
	IN_SC_Invalid = 0xff,
	IN_SC_Escape = 0x01,
	IN_SC_One = 0x02,
	IN_SC_Two = 0x03,
	IN_SC_Three = 0x04,
	IN_SC_Four = 0x05,
	IN_SC_Five = 0x06,
	IN_SC_Six = 0x07,
	IN_SC_Seven = 0x08,
	IN_SC_Eight = 0x09,
	IN_SC_Nine = 0x0a,
	IN_SC_Zero = 0x0b,
	IN_SC_Minus = 0x0c,
	IN_SC_Equals = 0x0d,
	IN_SC_Backspace = 0x0e,
	IN_SC_Tab = 0x0f,
	IN_SC_Enter = 0x1c,
	IN_SC_Control = 0x1d,
	IN_SC_LeftShift = 0x2a,
	IN_SC_RightShift = 0x36,
	IN_SC_Alt = 0x38,
	IN_SC_Space = 0x39,
	IN_SC_CapsLock = 0x3a,
	IN_SC_UpArrow = 0x48,
	IN_SC_LeftArrow = 0x4b,
	IN_SC_RightArrow = 0x4d,
	IN_SC_DownArrow = 0x50,
	IN_SC_Home = 0x47,
	IN_SC_End = 0x4f,
	IN_SC_PgUp = 0x49,
	IN_SC_PgDown = 0x51,
	IN_SC_Insert = 0x52,
	IN_SC_Delete = 0x53,
	
	IN_SC_F1 = 0x3b,
	IN_SC_F2 = 0x3c,
	IN_SC_F3 = 0x3d,
	IN_SC_F4 = 0x3e,
	IN_SC_F5 = 0x40,
	IN_SC_F6 = 0x41,
	IN_SC_F7 = 0x42,
	IN_SC_F8 = 0x43,
	IN_SC_F9 = 0x44,
	IN_SC_F10 = 0x45,
	IN_SC_F11 = 0x57,
	IN_SC_F12 = 0x59,

	IN_SC_A = 0x1a,
	IN_SC_B = 0x30,
	IN_SC_C = 0x2e,
	IN_SC_D = 0x20,
	IN_SC_E = 0x12,
	IN_SC_F = 0x21,
	IN_SC_G = 0x22,
	IN_SC_H = 0x23,
	IN_SC_I = 0x17,
	IN_SC_J = 0x24,
	IN_SC_K = 0x25,
	IN_SC_L = 0x26,
	IN_SC_M = 0x32,
	IN_SC_N = 0x31,
	IN_SC_O = 0x18,
	IN_SC_P = 0x19,
	IN_SC_Q = 0x10,
	IN_SC_R = 0x13,
	IN_SC_S = 0x1f,
	IN_SC_T = 0x14,
	IN_SC_U = 0x16,
	IN_SC_V = 0x2f,
	IN_SC_W = 0x11,
	IN_SC_X = 0x2d,
	IN_SC_Y = 0x15,
	IN_SC_Z = 0x2c

	//IN_KP_Enter = 0x0d,
	//IN_KP_Escape = 0x1b,
	//IN_KP_Space = 0x20,
	//IN_KP_Backspace = 0x
} IN_ScanCode;

// See wolf3d ID_IN.H
typedef enum IN_DemoMode
{
	IN_Demo_Off,
	IN_Demo_Record,
	IN_Demo_Playback,
	IN_Demo_PlayDone
} IN_DemoMode;

typedef struct IN_KeyMapping
{
	IN_ScanCode jump;
	IN_ScanCode pogo;
	IN_ScanCode upLeft;
	IN_ScanCode up;
	IN_ScanCode upRight;
	IN_ScanCode left;
	IN_ScanCode right;
	IN_ScanCode downLeft;
	IN_ScanCode down;
	IN_ScanCode downRight;
} IN_KeyMapping;

typedef struct IN_ControlFrame
{
	bool jump, pogo;
	int xDirection;
	int yDirection;
} IN_ControlFrame;

void IN_PumpEvents();
void IN_WaitKey();
bool IN_GetKeyState(IN_ScanCode scanCode);
void IN_Startup();
void IN_DemoStartPlaying(uint8_t *data, int len);
void IN_DemoStopPlaying();
bool IN_DemoIsPlaying();
void IN_ReadControls(int player, IN_ControlFrame *controls);
#endif
