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
#include <stdio.h>

#include "ck_cross.h"

#include "id_in.h"
#include "id_fs.h"

/* This keeps clang's static analyzer quiet. */
#ifdef __GNUC__
#define _NORETURN __attribute__((noreturn))
#else
#define _NORETURN
#endif

// Record used to save & restore screen windows
typedef struct
{
	int x, y, w, h, px, py;
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
bool US_LineInput(uint16_t x, uint16_t y, char *buf, char *def, bool escok, uint16_t maxchars, uint16_t maxwidth);

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
int16_t US_GetPrintFont();
int8_t US_GetPrintColour();
void US_SetWindowX(int parm);
void US_SetWindowY(int parm);
void US_SetWindowW(int parm);
void US_SetWindowH(int parm);
void US_SetPrintX(int parm);
void US_SetPrintY(int parm);
void US_SetPrintFont(int16_t parm);
void US_SetPrintColour(int8_t parm);

// Common
extern bool us_noWait;	// Debug mode enabled.
extern bool us_tedLevel;      // Launching a level from TED
extern int us_tedLevelNumber; // Number of level to launch from TED

// We need to steal these from main().
extern const char **us_argv;
extern int us_argc;

void US_Startup(void);
void US_Setup(void);
void US_Shutdown(void);

// ID_US_2

// Messages passed to the callback.
typedef enum US_CardMsg
{
	US_MSG_CardEntered,  //The card has become the active card
	US_MSG_Draw,	 //Allows the callback to draw the card
	US_MSG_PostDraw,     //Called after the card is drawn, for overlays
	US_MSG_DrawItemIcon, //Draws the icon of a single CardItem
	US_MSG_DrawItem,     //Draws an item
	US_MSG_ItemEntered   //An item was triggered
} US_CardMsg;

typedef enum US_CardItemType
{
	US_ITEM_None,   //An empty CardItem to end the list
	US_ITEM_Normal, //A normal item
	US_ITEM_Radio,  //A 'radio' item, only one can be selected in a given card
	US_ITEM_Submenu //The item triggers a submenu when activated
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
	US_Comm_LoadGame = 4,
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

typedef bool (*US_CardCallbackFunc)(US_CardMsg, US_CardItem *);

typedef struct US_Card
{
	int x;
	int y;
	//int gfxChunk;
	int *gfxChunk; // converted to pointer for multiple episode support
	// An unknown int
	int unknown;
	US_CardItem *items; //Pointer to item array.
	US_CardCallbackFunc msgCallback;
	int selectedItem; //Index of selected item.
	int unk1, unk2;
} US_Card;

void US_DrawCards();
void US_RunCards();

bool US_QuickSave();
bool US_QuickLoad();

// Related
int USL_CtlDialog(const char *s1, const char *s2, const char *s3);

// A few function pointers
extern bool (*p_save_game)(FS_File handle);
extern bool (*p_load_game)(FS_File handle, bool fromMenu);
extern void (*p_exit_menu)(void);

void US_SetMenuFunctionPointers(bool (*loadgamefunc)(FS_File, bool), bool (*savegamefunc)(FS_File), void (*exitmenufunc)(void));


typedef void (*US_MeasureStringFunc)(const char *string, uint16_t *width, uint16_t *height, int16_t chunk);
typedef void (*US_DrawStringFunc)(const char *string, int x, int y, int chunk, int colour);

void US_SetPrintRoutines(US_MeasureStringFunc measure, US_DrawStringFunc draw);
// Savefiles

#define US_MAX_SAVEDGAMENAME_LEN 32
#define US_MAX_NUM_OF_SAVED_GAMES 6

typedef CK_PACKED_STRUCT(US_Savefile
{
	char id[4];
	uint16_t printXOffset;
	bool used;
	char name[US_MAX_SAVEDGAMENAME_LEN + 1];
}) US_Savefile;

extern US_Savefile us_savefiles[US_MAX_NUM_OF_SAVED_GAMES];

void US_GetSavefiles(void);

// Text-mode order screen functions
bool US_TerminalOk();
void US_PrintB8000Text(const uint8_t *textscreen, int numChars);

#endif //ID_US_H
