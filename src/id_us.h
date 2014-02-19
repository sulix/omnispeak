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

#ifndef ID_US_H
#define ID_US_H

#include <stdbool.h>
#include "id_in.h"

/* This keeps clang's static analyzer quiet. */
#ifdef __GNUC__
#define _NORETURN __attribute__((noreturn))
#else
#define _NORETURN
#endif

// Record used to save & restore screen windows
typedef	struct
{
	int	x,y, w,h, px,py;
} US_WindowRec;	

// In ck_quit.c, as it may be customized by individual games.
void Quit(const char *msg) _NORETURN;

// id_us_1.c:
// Parameter Checking
int US_CheckParm(const char *parm, const char **strings);
// UI functions
void US_Print(const char *str);
void US_PrintF(const char *str, ...);
void US_PrintCentered(const char *str);
void US_CPrintLine(const char *str);
void US_CPrint(const char *str);
void US_CPrintF(const char *str, ...);
void US_ClearWindow();
void US_DrawWindow(int x, int y, int w, int h);
void US_CenterWindow(int w, int h);
void US_SaveWindow(US_WindowRec *win);
void US_RestoreWindow(US_WindowRec *win);
bool US_LineInput(int x, int y, char *buf, char *def, bool escok, int maxchars, int maxwidth);

// Random Number Generation
void US_InitRndT(bool randomize);
int US_RndT();
void US_SetRndI(int index);
int US_GetRndI();

// Getter and setter functions for print variables
int US_GetWindowX();
int US_GetWindowY();
int US_GetWindowW();
int US_GetWindowH();
int US_GetPrintX();
int US_GetPrintY();
int US_GetPrintFont();
int US_GetPrintColour();
void US_SetWindowX(int parm);
void US_SetWindowY(int parm);
void US_SetWindowW(int parm);
void US_SetWindowH(int parm);
void US_SetPrintX(int parm);
void US_SetPrintY(int parm);
void US_SetPrintFont(int parm);
void US_SetPrintColour(int parm);

// Common
extern bool us_noWait; // Debug mode enabled.
extern bool us_tedLevel; // Launching a level from TED
extern int us_tedLevelNumber; // Number of level to launch from TED

// We need to steal these from main().
extern const char **us_argv;
extern int us_argc;

void US_Startup();

// ID_US_2

// Messages passed to the callback.
typedef enum US_CardMsg
{
	US_MSG_CardEntered,	//The card has become the active card
	US_MSG_Draw,		//Allows the callback to draw the card
	US_MSG_PostDraw,	//Called after the card is drawn, for overlays
	US_MSG_DrawItemIcon,	//Draws the icon of a single CardItem
	US_MSG_DrawItem,	//Draws an item
	US_MSG_ItemEntered	//An item was triggered
} US_CardMsg;

typedef enum US_CardItemType
{
	US_ITEM_None,		//An empty CardItem to end the list
	US_ITEM_Normal,		//A normal item
	US_ITEM_Radio,		//A 'radio' item, only one can be selected in a given card
	US_ITEM_Submenu		//The item triggers a submenu when activated
} US_CardItemType;

// Used for storing flags, so not an enum (C++ doesn't like that)
typedef uint16_t US_CardItemState;
/*typedef*/ enum /*US_CardItemState*/
{
	US_IS_Checked = 0x01,
	US_IS_Selected = 0x02,
	US_IS_Disabled = 0x04,
	US_IS_Gap = 0x08
} /*US_CardItemState*/;


/*
 * Menu items can have 'commands', which cause the menu to quit, and something
 * to happen in the game.
 */
typedef enum US_CardCommand
{
	US_Comm_None = 0,
	US_Comm_ReturnToGame = 1,
	US_Comm_EndGame = 2,
	US_Comm_Quit = 3,
	US_Comm_NewEasyGame = 5,
	US_Comm_NewNormalGame = 6,
	US_Comm_NewHardGame = 7
} US_CardCommand;


struct US_Card;

typedef struct US_CardItem
{
	US_CardItemType type;
	US_CardItemState state;
	IN_ScanCode shortcutKey;
	const char *caption;
	US_CardCommand command;
	struct US_Card *subMenu;
	int x;
	int y;
} US_CardItem;

typedef bool (*US_CardCallbackFunc) (US_CardMsg, US_CardItem*);

typedef struct US_Card
{
	int x;
	int y;
	int gfxChunk;
	// An unknown int
	int unknown;
	US_CardItem *items;	//Pointer to item array.
	US_CardCallbackFunc msgCallback;
	int selectedItem;		//Index of selected item.
	int unk1, unk2;
} US_Card;

void US_DrawCards();
void US_RunCards();

// Related
int green_message_box( const char *s1, const char *s2, const char *s3 );

#endif //ID_US_H
