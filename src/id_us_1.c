#include "id_vh.h"
#include "id_vl.h"
#include "id_ca.h"

#include <string.h>
#include <stdbool.h>

// Coords in pixels
static int us_windowX;
static int us_windowY;
static int us_windowW;
static int us_windowH;

static int us_printX;
static int us_printY;

#define US_WINDOW_MAX_X 320
#define US_WINDOW_MAX_Y 200

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
			ch = *str;
			*str++;
			if (ch == '\0' || ch == '\n')
			{
				strbuf[sboff] = '\0';
				break;
			}
			strbuf[sboff] = ch;
			sboff++;
		}

		int w, h;
		CA_CacheGrChunk(3);
		VH_MeasurePropString(strbuf, &w, &h, 3);
		VH_DrawPropString(strbuf, us_printX, us_printY, 3, 0);


		if (ch)
		{
			us_printX = us_windowX;
			us_printY += h;
		}
		else
		{
			us_printX += w;
			break;
		}
	}
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

	//printf("US_DrawWindow: (%d,%d)-(%d,%d)\n", x, y, w, h);

	int borderX = us_windowX - 8;
	int borderY = us_windowY - 8;
	int borderW = us_windowW + 8;
	int borderH = us_windowH + 8;

	us_printX = us_windowX;
	us_printY = us_windowY;

	US_ClearWindow();

	VH_DrawTile8M(borderX, borderY, 0); //Top Left Corner
	VH_DrawTile8M(borderX, borderY+borderH, 6); //Bottom Left Corner
	VH_DrawTile8M(borderX+borderW, borderY,2); //Top Right Corner
	VH_DrawTile8M(borderX+borderW, borderY+borderH,8); //Bottom Right Corner
	// Draw Horizontal sides
	for (int i = borderX+8; i <= borderX + borderW - 8; i += 8)
	{
		VH_DrawTile8M(i, borderY, 1); //Top row
		VH_DrawTile8M(i, borderY+borderH, 7); //Bottom Row
	}
	//Draw Vertical sides
	for (int i = borderY+8; i <= borderY + borderH - 8; i += 8)
	{
		VH_DrawTile8M(borderX, i, 3); //Left col
		VH_DrawTile8M(borderX+borderW, i, 5); //Right col
	}
}

void US_CenterWindow(int w, int h)
{
	const int maxXtile = US_WINDOW_MAX_X / 8;
	const int maxYtile = US_WINDOW_MAX_Y / 8;

	US_DrawWindow((maxXtile - w)/2, (maxYtile - h)/2, w, h);
}




// Random Number Generator

uint8_t us_RandomTable[256] = {
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
	120, 163, 236, 249 };

static int us_randomIndex;

void US_InitRndT(bool randomize)
{
	//TODO: Actually randomize.
	us_randomIndex = 0;
}

int US_RndT()
{
	return us_RandomTable[(us_randomIndex++)&0xff];
}

void US_SetRndI(int index)
{
	us_randomIndex = index;
}

int US_GetRndI()
{
	return us_randomIndex;
}
	
