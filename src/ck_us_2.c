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

#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "ck_config.h"

#include "id_fs.h"
#include "id_in.h"
#include "id_sd.h"
#include "id_us.h"
#include "id_vl.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "ck_play.h"

#define US_MAX_JOYSTICK_NAME_LENGTH 28

#define US_MAX_JOYSTICKS 2

void CK_US_SetKeyBinding(US_CardItem *item, int which_control);
void CK_US_SetJoyBinding(US_CardItem *item, IN_JoyConfItem which_control);

bool CK_US_ScoreBoxMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	ck_scoreBoxEnabled = !ck_scoreBoxEnabled;
	USL_CtlDialog((ck_scoreBoxEnabled ? "Score box now on" : "Score box now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_TwoButtonFiringMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	ck_twoButtonFiring = !ck_twoButtonFiring;
	USL_CtlDialog((ck_twoButtonFiring ? "Two-button firing now on" : "Two-button firing now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_FixJerkyMotionMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	ck_fixJerkyMotion = !ck_fixJerkyMotion;
	USL_CtlDialog((ck_fixJerkyMotion ? "Jerky motion fix enabled" : "Jerky motion fix disabled"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_SVGACompatibilityMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	ck_svgaCompatibility = !ck_svgaCompatibility;
	USL_CtlDialog((ck_svgaCompatibility ? "SVGA compatibility now on" : "SVGA compatibility now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

#ifdef EXTRA_GRAPHICS_OPTIONS

bool CK_US_FullscreenMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	VL_ToggleFullscreen();
	// Save the config option.
	CFG_SetConfigBool("fullscreen", vl_isFullScreen);
	USL_CtlDialog((vl_isFullScreen ? "Fullscreen mode enabled" : "Windowed mode enabled"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_AspectCorrectMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	VL_ToggleAspect();
	CFG_SetConfigBool("aspect", vl_isAspectCorrected);
	USL_CtlDialog((vl_isAspectCorrected ? "Aspect ratio correction now on" : "Aspect ratio correction now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_BorderMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	VL_ToggleBorder();
	CFG_SetConfigBool("border", vl_hasOverscanBorder);
	USL_CtlDialog((vl_hasOverscanBorder ? "Overscan border now on" : "Overscan border now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

bool CK_US_IntegerMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	VL_ToggleInteger();
	CFG_SetConfigBool("integer", vl_isIntegerScaled);
	USL_CtlDialog((vl_isIntegerScaled ? "Integer scaling now on" : "Integer scaling now off"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

#endif

#ifdef EXTRA_JOYSTICK_OPTIONS

bool CK_US_JoyMotionModeMenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return false;

	in_joyAdvancedMotion = !in_joyAdvancedMotion;
	USL_CtlDialog((in_joyAdvancedMotion ? "Motion mode set to modern" : "Motion mode set to classic"), "Press any key", NULL);
	CK_US_UpdateOptionsMenus();
	return true;
}

#endif

bool CK_US_ControlsMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_KeyboardMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_Joystick1MenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_Joystick2MenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_GamepadMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_JoyConfMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_ConfigureMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_PaddleWar(US_CardMsg msg, US_CardItem *item);

// A debug menu which doesn't seem to ever appear in the game.
US_CardItem ck_us_debugMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_None, "DEBUG", US_Comm_None, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_debugMenu = {0, 0, &PIC_DEBUGCARD, 0, ck_us_debugMenuItems, 0, 0, 0, 0};

// Sound Menu
US_CardItem ck_us_soundMenuItems[] = {
	{US_ITEM_Radio, 0, IN_SC_N, "NO SOUND EFFECTS", US_Comm_None, 0, 0, 0},
	{US_ITEM_Radio, 0, IN_SC_P, "PC SPEAKER", US_Comm_None, 0, 0, 0},
	{US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0},
	{US_ITEM_Radio, 0, IN_SC_Q, "QUIET ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_soundMenu = {8, 0, &PIC_SOUNDCARD, 0, ck_us_soundMenuItems, 0, 0, 0, 0};

// Music Menu
US_CardItem ck_us_musicMenuItems[] = {
	{US_ITEM_Radio, 0, IN_SC_N, "NO MUSIC", US_Comm_None, 0, 0, 0},
	{US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_musicMenu = {8, 0, &PIC_MUSICCARD, 0, ck_us_musicMenuItems, 0, 0, 0, 0};

// New Game Menu
US_CardItem ck_us_newGameMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_E, "BEGIN EASY GAME", US_Comm_NewEasyGame, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_N, "BEGIN NORMAL GAME", US_Comm_NewNormalGame, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_H, "BEGIN HARD GAME", US_Comm_NewHardGame, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_newGameMenu = {8, 0, &PIC_NEWGAMECARD, 0, ck_us_newGameMenuItems, 0, 1, 0, 0 /*, 0*/};

// Load/Save Game Menus
US_CardItem ck_us_loadSaveMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_One, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Two, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Three, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Four, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Five, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Six, 0, US_Comm_None, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

extern US_Card *us_currentCard;
void CK_US_DrawSavegameItemBorder(US_CardItem *item)
{

	int c;

	/* Set the item's position */
	item->y = us_currentCard->y + 60;
	item->y += (item - ck_us_loadSaveMenuItems) * 11;

	/* Choose an appropriate color */
	US_SetPrintColour((item->state & US_IS_Selected) ? 2 : 10);
	c = US_GetPrintColour() ^ 8;

	/* Draw the rectangle */
	VH_HLine(item->x, item->x + 148, item->y, c);
	VH_HLine(item->x, item->x + 148, item->y + 9, c);
	VH_VLine(item->y, item->y + 9, item->x, c);
	VH_VLine(item->y, item->y + 9, item->x + 148, c);
}

extern int load_game_error, ck_startingSavedGame;
extern US_CardCommand us_currentCommand;
extern bool command_confirmed;
const char *US_GetSavefileName(int index);
void USL_HandleError(int error);
void USL_LoadSaveMessage(const char *s1, const char *s2);
void USL_SetMenuFooter(void);
int USL_ConfirmComm(US_CardCommand command);

static bool US_LoadMain(int i, bool fromMenu)
{
	int error = 0;
	FS_File fp;
	const char *fname;
	US_Savefile *e;

	e = &us_savefiles[i];
	fname = US_GetSavefileName(i);
	fp = FS_OpenUserFile(fname);
	if (FS_IsFileValid(fp))
	{
		// Omnispeak - reading US_Savefile fields one-by-one
		// for cross-platform support
		uint8_t padding; // One byte of struct padding
		if ((FS_Read(e->id, sizeof(e->id), 1, fp) == 1) &&
			(FS_ReadInt16LE(&e->printXOffset, 1, fp) == 1) &&
			(FS_ReadBoolFrom16LE(&e->used, 1, fp) == 1) &&
			(FS_Read(e->name, sizeof(e->name), 1, fp) == 1) &&
			(FS_Read(&padding, sizeof(padding), 1, fp) == 1))
		{
			if (p_load_game && !(*p_load_game)(fp, fromMenu))
			{
				load_game_error = 1;
				USL_HandleError(error = errno);
			}
		}
		else
		{
			USL_HandleError(error = errno);
		}
		FS_CloseFile(fp);
	}
	else
	{
		USL_HandleError(error = errno);
	}

	if (error)
	{
		return false;
	}
	else
	{
		ck_startingSavedGame = 1;
		e->used = 1;
		in_Paused = 1;
		in_PausedMessage = "GAME LOADED";
		return true;
	}
}

bool US_QuickLoad(void)
{
	return US_LoadMain(US_MAX_NUM_OF_SAVED_GAMES - 1, false);
}

void load_savegame_item(US_CardItem *item)
{
	if (USL_ConfirmComm(US_Comm_LoadGame))
	{
		int i = item - ck_us_loadSaveMenuItems;
		USL_LoadSaveMessage("Loading", us_savefiles[i].name);
		if (!US_LoadMain(i, true))
		{ /* is this condition right? */
			load_game_error = 1;
			us_currentCommand = US_Comm_None; /* ? last command ? */
			command_confirmed = 0;		  /* ? command success state ? */
		}
		US_DrawCards();
	}
}

bool CK_US_LoadGameMenuProc(US_CardMsg msg, US_CardItem *item)
{
	int result, i;

	result = 0;

	switch (msg)
	{
	case US_MSG_CardEntered:
		if (getenv("UID"))
			US_GetSavefiles();

		for (i = 0; i < US_MAX_NUM_OF_SAVED_GAMES; i++)
		{
			if (us_savefiles[i].used)
				ck_us_loadSaveMenuItems[i].state &= ~US_IS_Disabled;
			else
				ck_us_loadSaveMenuItems[i].state |= US_IS_Disabled;
		}
		break;

	case US_MSG_DrawItemIcon:
		CK_US_DrawSavegameItemBorder(item);
		result = 1;
		break;

	case US_MSG_DrawItem:
		CK_US_DrawSavegameItemBorder(item);

		/* Draw the caption */
		VH_Bar(item->x + 1, item->y + 2, 146, 7, 8);
		i = item - ck_us_loadSaveMenuItems;
		if (us_savefiles[i].used)
			US_SetPrintX(item->x + 2);
		else
			US_SetPrintX(item->x + 60);

		US_SetPrintY(item->y + 2);
		VH_DrawPropString(us_savefiles[i].used ? us_savefiles[i].name : "Empty", US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour());
		result = 1;
		break;

	case US_MSG_ItemEntered:
		load_savegame_item(item);
		result = 1;
		break;

	default:
		break;
	}

	return result;
}

extern int game_unsaved;
extern const char *footer_str[3];

static bool US_SaveMain(int i)
{
	int n, error = 0;
	FS_File fp;
	const char *fname;
	US_Savefile *e;

	e = &us_savefiles[i];
	fname = US_GetSavefileName(i);
	fp = FS_CreateUserFile(fname);
	if (FS_IsFileValid(fp))
	{
		// Omnispeak - writing US_Savefile fields one-by-one
		// for cross-platform support
		uint8_t padding = 0; // One byte of struct padding
		if ((FS_Write(e->id, sizeof(e->id), 1, fp) == 1) &&
			(FS_WriteInt16LE(&e->printXOffset, 1, fp) == 1) &&
			(FS_WriteBoolTo16LE(&e->used, 1, fp) == 1) &&
			(FS_Write(e->name, sizeof(e->name), 1, fp) == 1) &&
			(FS_Write(&padding, sizeof(padding), 1, fp) == 1))
		//if ( write( handle, e, sizeof ( SAVEFILE_ENTRY ) ) == sizeof ( SAVEFILE_ENTRY ) )
		{
			if (p_save_game && !(n = (*p_save_game)(fp)))
				USL_HandleError(error = errno);
		}
		else
		{
			error = (errno == 2) ? 8 : errno;
			USL_HandleError(error);
		}
		FS_CloseFile(fp);
	}
	else
	{
		error = (errno == 2) ? 8 : errno;
		USL_HandleError(error);
	}

	/* Delete the file if an error occurred */
	if (error)
	{
		remove(fname);
		e->used = 0;
		return false;
	}
	else
	{
		game_unsaved = 0;
		return true;
	}
}

#ifdef QUICKSAVE_ENABLED

bool US_QuickSave(void)
{
	US_Savefile *e;
	e = &us_savefiles[US_MAX_NUM_OF_SAVED_GAMES - 1];
	e->printXOffset = ck_currentEpisode->printXOffset;
	strcpy(e->name, "QuickSave");
	e->used = 1;
	return US_SaveMain(US_MAX_NUM_OF_SAVED_GAMES - 1);
}

#endif

void save_savegame_item(US_CardItem *item)
{
	int i, n;
	US_Savefile *e;

	footer_str[2] = "Type name";
	footer_str[1] = "Enter accepts";
	US_DrawCards();

	i = item - ck_us_loadSaveMenuItems;
	e = &us_savefiles[i];

	/* Prompt the user to enter a name */
	US_SetPrintColour(2);
	//fontcolour = 2;
	VH_Bar(item->x + 1, item->y + 2, 146, 7, 8);
	e->printXOffset = ck_currentEpisode->printXOffset;
	n = US_LineInput(item->x + 2, item->y + 2, e->name, (e->used ? e->name : NULL), 1, 32, 138);

	/* If they entered no name, give a default */
	if (strlen(e->name) == 0)
		strcpy(e->name, "Untitled");

	/* If the input was not canceled */
	if (n != 0)
	{
		e->used = 1;
		USL_LoadSaveMessage("Saving", e->name);
		US_SaveMain(i);
	}
	USL_SetMenuFooter();
}

bool CK_US_SaveGameMenuProc(US_CardMsg msg, US_CardItem *item)
{
	int i;

	switch (msg)
	{
	case US_MSG_CardEntered:
		if (getenv("UID"))
			US_GetSavefiles();

		/* Enable all the entries */
		for (i = 0; i < US_MAX_NUM_OF_SAVED_GAMES; i++)
			ck_us_loadSaveMenuItems[i].state &= ~US_IS_Disabled;

		return false;

	case US_MSG_ItemEntered:
		save_savegame_item(item);
		return true;

	default:
		return CK_US_LoadGameMenuProc(msg, item);
	}
}

US_Card ck_us_loadGameMenu = {4, 3, &PIC_LOADCARD, 0, ck_us_loadSaveMenuItems, &CK_US_LoadGameMenuProc, 0, 0, 0};
US_Card ck_us_saveGameMenu = {4, 3, &PIC_SAVECARD, 0, ck_us_loadSaveMenuItems, &CK_US_SaveGameMenuProc, 0, 0, 0};

// Dummy Menus

US_Card ck_us_scoreBoxMenu = {0, 0, 0, 0, 0, &CK_US_ScoreBoxMenuProc, 0, 0, 0};
US_Card ck_us_twoButtonFiringMenu = {0, 0, 0, 0, 0, &CK_US_TwoButtonFiringMenuProc, 0, 0, 0};
US_Card ck_us_fixJerkyMotionMenu = {0, 0, 0, 0, 0, &CK_US_FixJerkyMotionMenuProc, 0, 0, 0};
US_Card ck_us_svgaCompatibilityMenu = {0, 0, 0, 0, 0, &CK_US_SVGACompatibilityMenuProc, 0, 0, 0};
#ifdef EXTRA_GRAPHICS_OPTIONS
US_Card ck_us_fullscreenMenu = {0, 0, 0, 0, 0, &CK_US_FullscreenMenuProc, 0, 0, 0};
US_Card ck_us_aspectCorrectMenu = {0, 0, 0, 0, 0, &CK_US_AspectCorrectMenuProc, 0, 0, 0};
US_Card ck_us_borderMenu = {0, 0, 0, 0, 0, &CK_US_BorderMenuProc, 0, 0, 0};
US_Card ck_us_integerMenu = {0, 0, 0, 0, 0, &CK_US_IntegerMenuProc, 0, 0, 0};
#endif
#ifdef EXTRA_JOYSTICK_OPTIONS
US_Card ck_us_joyMotionModeMenu = {0, 0, 0, 0, 0, &CK_US_JoyMotionModeMenuProc, 0, 0, 0};
#endif
// Options menu
US_CardItem ck_us_optionsMenuItems[] = {
	{US_ITEM_Submenu, 0, IN_SC_S, "", US_Comm_None, &ck_us_scoreBoxMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_T, "", US_Comm_None, &ck_us_twoButtonFiringMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_M, "", US_Comm_None, &ck_us_fixJerkyMotionMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_C, "", US_Comm_None, &ck_us_svgaCompatibilityMenu, 0, 0},
#ifdef EXTRA_GRAPHICS_OPTIONS
	{US_ITEM_Submenu, 0, IN_SC_F, "", US_Comm_None, &ck_us_fullscreenMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_A, "", US_Comm_None, &ck_us_aspectCorrectMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_B, "", US_Comm_None, &ck_us_borderMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_I, "", US_Comm_None, &ck_us_integerMenu, 0, 0},
#endif
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_optionsMenu = {8, 0, &PIC_OPTIONSCARD, 0, ck_us_optionsMenuItems, 0, 0, 0, 0};

// Movement Kbd Controls Menu
US_CardItem ck_us_movementMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_None, "UP & LEFT", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "UP", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "UP & RIGHT", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "RIGHT", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "DOWN & RIGHT", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "DOWN", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "DOWN & LEFT", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_None, "LEFT", US_Comm_None, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_movementMenu = {0, 0, &PIC_MOVEMENTCARD, 0, ck_us_movementMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0};

// Buttons Kbd Controls Menu
US_CardItem ck_us_buttonsMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_J, "JUMP", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_P, "POGO", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_F, "FIRE", US_Comm_None, 0, 0, 0},
#ifdef EXTRA_KEYBOARD_OPTIONS
	{US_ITEM_Normal, 0, IN_SC_I, "STATUS", US_Comm_None, 0, 0, 0},
#endif
#ifdef QUICKSAVE_ENABLED
	{US_ITEM_Normal, 0, IN_SC_S, "QUICKSAVE", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_L, "QUICKLOAD", US_Comm_None, 0, 0, 0},
#endif
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_buttonsMenu = {0, 0, &PIC_BUTTONSCARD, 0, ck_us_buttonsMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0};

// Keyboard Menu
US_CardItem ck_us_keyboardMenuItems[] = {
	{US_ITEM_Submenu, 0, IN_SC_M, "MOVEMENT", US_Comm_None, &ck_us_movementMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_M, "BUTTONS", US_Comm_None, &ck_us_buttonsMenu, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_keyboardMenu = {8, 0, &PIC_KEYBOARDCARD, 0, ck_us_keyboardMenuItems, &CK_US_KeyboardMenuProc, 0, 0, 0};

// Custom Menus

US_Card ck_us_joystick1Menu = {0, 0, &PIC_JOYSTICKCARD, 0, 0, &CK_US_Joystick1MenuProc, 0, 0, 0};
US_Card ck_us_joystick2Menu = {0, 0, &PIC_JOYSTICKCARD, 0, 0, &CK_US_Joystick2MenuProc, 0, 0, 0};
US_Card ck_us_gamepadMenu = {0, 0, &PIC_JOYSTICKCARD, 0, 0, &CK_US_GamepadMenuProc, 0, 0, 0};

#ifdef EXTRA_JOYSTICK_OPTIONS
// Joystick Config Menu
US_CardItem ck_us_joyconfMenuItems[] = {
	{US_ITEM_Normal, 0, IN_SC_J, "JUMP", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_P, "POGO", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_F, "FIRE", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_M, "MENU", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_S, "STATUS", US_Comm_None, 0, 0, 0},
#ifdef QUICKSAVE_ENABLED
	{US_ITEM_Normal, 0, IN_SC_L, "QUICKLOAD", US_Comm_None, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Q, "QUICKSAVE", US_Comm_None, 0, 0, 0},
#endif
	{US_ITEM_Normal, 0, IN_SC_D, "DEAD ZONE", US_Comm_None, 0, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_J, "", US_Comm_None, &ck_us_joyMotionModeMenu, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_joyconfMenu = {0, 0, &PIC_BUTTONSCARD, 0, ck_us_joyconfMenuItems, &CK_US_JoyConfMenuProc, 0, 0, 0};
#endif

// Configure Menu
US_CardItem ck_us_configureMenuItems[] = {
	{US_ITEM_Submenu, 0, IN_SC_S, "SOUND", US_Comm_None, &ck_us_soundMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_M, "MUSIC", US_Comm_None, &ck_us_musicMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_O, "OPTIONS", US_Comm_None, &ck_us_optionsMenu, 0, 0},
	{US_ITEM_Submenu, US_IS_Gap, IN_SC_K, "KEYBOARD", US_Comm_None, &ck_us_keyboardMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_One, "USE JOYSTICK #1", US_Comm_None, &ck_us_joystick1Menu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_Two, "USE JOYSTICK #2", US_Comm_None, &ck_us_joystick2Menu, 0, 0},
//{ US_ITEM_Submenu, 0, IN_SC_G, "", US_Comm_None, &ck_us_gamepadMenu, 0, 0 },
#ifdef EXTRA_JOYSTICK_OPTIONS
	{US_ITEM_Submenu, 0, IN_SC_J, "JOYSTICK CONFIGURATION", US_Comm_None, &ck_us_joyconfMenu, 0, 0},
#endif
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_configureMenu = {0, 0, &PIC_CONFIGURECARD, 0, ck_us_configureMenuItems, &CK_US_ConfigureMenuProc, 0, 0, 0};

// Paddle War!

US_Card ck_us_paddleWarMenu = {0, 0, 0, 0, 0, &CK_PaddleWar, 0, 0, 0};

// Main Menu
US_CardItem ck_us_mainMenuItems[] = {
	{US_ITEM_Submenu, 0, IN_SC_N, "NEW GAME", US_Comm_None, &ck_us_newGameMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_L, "LOAD GAME", US_Comm_None, &ck_us_loadGameMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_S, "SAVE GAME", US_Comm_None, &ck_us_saveGameMenu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_C, "CONFIGURE", US_Comm_None, &ck_us_configureMenu, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_R, NULL, US_Comm_ReturnToGame, 0, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_E, "END GAME", US_Comm_EndGame, 0, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_P, "PADDLE WAR", US_Comm_None, &ck_us_paddleWarMenu, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Q, "QUIT", US_Comm_Quit, 0, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0}};

US_Card ck_us_mainMenu = {32, 4, &PIC_MENUCARD, 0, ck_us_mainMenuItems, 0, 0, 0, 0};

extern US_Card *us_currentCard;

bool CK_US_ControlsMenuProc(US_CardMsg msg, US_CardItem *item)
{
	int which_control;
	int result = 0;
	int print_x, print_y;

	which_control = (us_currentCard == &ck_us_movementMenu) ? (item - ck_us_movementMenuItems) + in_key_button_controls : (item - ck_us_buttonsMenuItems);

	switch (msg)
	{
	case US_MSG_CardEntered:
		// game_controller[0] = CTRL_KEYBOARD;
		break;

	case US_MSG_DrawItem:

		// Draw the item Icon and the key's name
		VH_Bar(75, item->y, 159, 8, 8);
		USL_DrawCardItemIcon(item);

		US_SetPrintColour((item->state & US_IS_Selected) ? 2 : 10);
		print_x = item->x + 8;
		print_y = item->y + 1;
		VH_DrawPropString(item->caption, print_x, print_y, 1, US_GetPrintColour());

		// Draw the outer green bo
		VH_Bar(item->x + 90, item->y, 40, 8, US_GetPrintColour() ^ 8);
		VH_Bar(item->x + 91, item->y + 1, 38, 6, 8);

		print_x = item->x + 96;
		print_y = item->y + 1;
		VH_DrawPropString(IN_GetScanName(*in_key_controls[which_control]), print_x, print_y, 1, US_GetPrintColour());
		result = 1;
		break;

	case US_MSG_ItemEntered:
		CK_US_ControlsMenuProc(US_MSG_DrawItem, item);
		CK_US_SetKeyBinding(item, which_control);
		US_DrawCards();
		result = 1;
		break;

	default:
		break;
	}

	return result;
}

void CK_US_SetKeyBinding(US_CardItem *item, int which_control)
{
	bool cursor = false;
	uint32_t lasttime = 0;
	IN_ControlFrame state;
	char k;
	int i, used;

	// TODO: Should be global variables (as in vanilla Keen 5)?
	char last_scan = 0;
	US_SetPrintColour(2);

	IN_ClearKeysDown();

	/* Prompt the user to press a key */
	do
	{
		//CK_SetTicsPerFrame();
		IN_PumpEvents();

		/* Flicker the cursor */
		if (SD_GetTimeCount() >= lasttime)
		{
			/* time_count */
			cursor = !cursor;

			/* Draw the rectangle */
			VH_Bar(item->x + 90, item->y, 40, 8, US_GetPrintColour() ^ 8);
			VH_Bar(item->x + 91, item->y + 1, 38, 6, 8);

			/* Draw the cursor */
			if (cursor)
				VH_DrawTile8(item->x + 106, item->y, 100);

			//VW_UpdateScreen();
			lasttime = SD_GetTimeCount() + 35; /* time_count */
			VL_Present();
		}

		/* A button push will cancel the key selection */
		IN_ReadControls(0, &state);
		last_scan = IN_GetLastScan();
		while (state.jump || state.pogo)
		{
			VL_Yield(); // Keep CPU usage low
			IN_PumpEvents();
			IN_ReadControls(0, &state);
			last_scan = IN_SC_Escape;
		}

		/* Disallow left shift for some reason */
		// disable();
		if (IN_GetLastScan() == IN_SC_LeftShift)
			last_scan = 0;
		// enable();

	} while ((k = last_scan) == 0);

	/* If they didn't cancel the process with ESC */
	if (last_scan != IN_SC_Escape)
	{
		used = 0;
		i = 0;

		/* Make sure the key they chose is not already used */
		for (i = 0; i < in_key_button_controls + in_key_direction_controls; i++)
		{
			/* Don't check the one we're setting */
			if (i != which_control)
			{
				if (*(in_key_controls[i]) == k)
				{
					used = 1;
					break;
				}
			}
		}

		if (used)
			USL_CtlDialog("Key already used", "Press any key", NULL);
		else
			*(in_key_controls[which_control]) = k;
	}

	IN_ClearKeysDown();
}

bool CK_US_KeyboardMenuProc(US_CardMsg msg, US_CardItem *item)
{
	// Set keyboard as game controller if this menu is entered
	if (msg == US_MSG_CardEntered)
	{
		IN_SetControlType(0, IN_ctrl_Keyboard1);
		CK_US_UpdateOptionsMenus();
	}
	return false;
}

bool CK_US_Joystick1MenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg == US_MSG_CardEntered)
	{
		IN_SetControlType(0, IN_ctrl_Joystick1);
		USL_CtlDialog("USING JOYSTICK #1", "PRESS ANY KEY", 0);
		CK_US_UpdateOptionsMenus();
		return true;
	}
	return false;
}

bool CK_US_Joystick2MenuProc(US_CardMsg msg, US_CardItem *item)
{
	if (msg == US_MSG_CardEntered)
	{
		IN_SetControlType(0, IN_ctrl_Joystick2);
		USL_CtlDialog("USING JOYSTICK #2", "PRESS ANY KEY", 0);
		CK_US_UpdateOptionsMenus();
		return true;
	}
	return false;
}

bool CK_US_GamepadMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

#ifdef EXTRA_JOYSTICK_OPTIONS
bool CK_US_JoyConfMenuProc(US_CardMsg msg, US_CardItem *item)
{
	IN_JoyConfItem which_control;
	int value;
	char str[8], *spos;
	int print_x, print_y;
	static const int8_t deadzone_values[] = {
		0, 5, 10, 15, 20, 25, 30, 35, 40, 50, 60, 70, 80, 90, -1};

	if (!item || item == &ck_us_joyconfMenuItems[(int)IN_joy_modern])
		return false; // no special handling for the motion mode option

	which_control = (IN_JoyConfItem)(item - ck_us_joyconfMenuItems);
	value = IN_GetJoyConf(which_control);

	switch (msg)
	{
	case US_MSG_DrawItem:

		// Draw the item Icon and the key's name
		VH_Bar(75, item->y, 159, 8, 8);
		USL_DrawCardItemIcon(item);

		US_SetPrintColour((item->state & US_IS_Selected) ? 2 : 10);
		print_x = item->x + 8;
		print_y = item->y + 1;
		VH_DrawPropString(item->caption, print_x, print_y, 1, US_GetPrintColour());

		// Draw the outer green bo
		VH_Bar(item->x + 90, item->y, 40, 8, US_GetPrintColour() ^ 8);
		VH_Bar(item->x + 91, item->y + 1, 38, 6, 8);

		// construct the value string
		spos = str;
		if ((which_control != IN_joy_deadzone) && (value < 0))
		{
			*spos++ = 'N';
			*spos++ = 'o';
			*spos++ = 'n';
			*spos++ = 'e';
		}
		else
		{
			if (which_control != IN_joy_deadzone)
			{
				*spos++ = 'B';
				*spos++ = 't';
				*spos++ = 'n';
				*spos++ = ' ';
			}
			if (value >= 10)
				*spos++ = '0' + (value / 10);
			*spos++ = '0' + (value % 10);
			if (which_control == IN_joy_deadzone)
				*spos++ = '%';
		}
		*spos++ = '\0';

		print_x = item->x + 96;
		print_y = item->y + 1;
		VH_DrawPropString(str, print_x, print_y, 1, US_GetPrintColour());
		return true;

	case US_MSG_ItemEntered:
		if (which_control == IN_joy_deadzone)
		{
			int i;
			for (i = 0; (deadzone_values[i] >= 0) && (deadzone_values[i] <= value); i++)
				;
			value = deadzone_values[(deadzone_values[i] < 0) ? 0 : i];
			IN_SetJoyConf(which_control, value);
		}
		else
			CK_US_SetJoyBinding(item, which_control);

		US_DrawCards();
		return true;

	default:
		break;
	}

	return false;
}

void CK_US_SetJoyBinding(US_CardItem *item, IN_JoyConfItem which_control)
{
	bool cursor = false;
	uint32_t lasttime = 0;
	uint16_t button_mask = 0;
	IN_ScanCode last_scan = IN_SC_None;

	US_SetPrintColour(2);
	IN_ClearKeysDown();

	/* Prompt the user to press a button */
	while (1)
	{
		IN_PumpEvents();

		/* Flicker the cursor */
		if (SD_GetTimeCount() >= lasttime)
		{
			/* time_count */
			cursor = !cursor;

			/* Draw the rectangle */
			VH_Bar(item->x + 90, item->y, 40, 8, US_GetPrintColour() ^ 8);
			VH_Bar(item->x + 91, item->y + 1, 38, 6, 8);

			/* Draw the cursor */
			if (cursor)
				VH_DrawTile8(item->x + 106, item->y, 100);

			//VW_UpdateScreen();
			lasttime = SD_GetTimeCount() + 35; /* time_count */
			VL_Present();
		}

		/* any key cancels the selection */
		last_scan = IN_GetLastScan();
		if (last_scan != 0)
		{
			break;
		}

		/* poll joysticks */
		for (int i = 0; i < US_MAX_JOYSTICKS; i++)
		{
			if (IN_JoyPresent(i))
			{
				button_mask = IN_GetJoyButtonsDB(i);
				if (button_mask)
					break;
			}
		}
		if (button_mask != 0)
			break;
		VL_Present();
	}

	/* assign the joystick button */
	if (last_scan == IN_SC_Backspace)
		IN_SetJoyConf(which_control, -1); /* Backspace = unassign */
	else if (button_mask)
	{
		int bit = 0;
		while ((button_mask & 1) == 0)
		{
			bit++;
			button_mask >>= 1;
		}
		IN_SetJoyConf(which_control, bit);
	}

	/* wait until all joystick buttons have been released */
	while (1)
	{
		IN_PumpEvents();
		button_mask = 0;
		for (int i = 0; i < US_MAX_JOYSTICKS; i++)
		{
			if (IN_JoyPresent(i))
				button_mask |= IN_GetJoyButtonsDB(i);
		}
		if (button_mask == 0)
			break;
		VL_Present();
	}

	IN_ClearKeysDown();
}
#endif

bool CK_US_ConfigureMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

void USL_DrawPaddleWarScore(int16_t keen_score, int16_t comp_score)
{
	// NOTE: This is modified a little from the original
	// exe in order to align the text and to set the proper font and color

	int16_t print_color = 2;
	int16_t print_y = 52;
	uint16_t w, h;

	//uint16_t old_print_font = US_GetPrintFont();
	//uint16_t old_print_color = US_GetPrintColour();

	//US_SetPrintFont(4);
	US_SetPrintColour(print_color);

	// Draw keen's score
	int print_x = 80;
	VH_Bar(print_x, print_y, 42, 6, 8);
	const char *keenString = "KEEN:";
	VH_MeasurePropString(keenString, &w, &h, 1);
	VH_DrawPropString(keenString, print_x, print_y, 1, print_color);
	US_SetPrintX(print_x + w);
	US_SetPrintY(print_y);
	US_PrintF("%u", keen_score);

	// Draw Comp score
	print_x = 182;
	VH_Bar(print_x, print_y, 50, 6, 8);
	const char *compString = "COMP:";
	VH_MeasurePropString(compString, &w, &h, 1);
	VH_DrawPropString(compString, print_x, print_y, 1, print_color);

	US_SetPrintX(print_x + w);
	US_SetPrintY(print_y);
	US_PrintF("%u", comp_score);

	//US_SetPrintFont(old_print_font);
	//US_SetPrintColour(old_print_color);
}

void USL_PlayPaddleWar(void)
{
	int16_t ball_visible, new_round, y_bounce, done, keen_won_last, comp_move_counter;
	int16_t ball_y, keen_x, comp_x, bounce_point, ball_real_x, ball_real_y;
	int16_t old_keen_x, old_comp_x, old_ball_x, old_ball_y, keen_score, comp_score;
	int16_t speedup_delay, ball_x_speed;
	int32_t start_delay;
	uint32_t lasttime, timediff;
	IN_ControlFrame status;
	int16_t ball_y_speed, ball_x;

	keen_x = comp_x = 148;
	ball_real_x = ball_real_y = ball_y_speed = 0;
	old_ball_x = old_comp_x = old_keen_x = 78;
	old_ball_y = 62;
	keen_score = comp_score = 0;
	USL_DrawPaddleWarScore(0, 0);
	comp_move_counter = 0;
	y_bounce = 0;
	new_round = 1;
	done = 0;
	keen_won_last = 0;
	lasttime = SD_GetTimeCount();

	do
	{
		// Delay Processing
		// TODO/FIXME: Better handling of this in the future
		while ((timediff = SD_GetTimeCount() - lasttime) == 0)
		{
			// The original code waits in a busy loop.
			// Bad idea for new code.
			// TODO: What about checking for input/graphics/other status?
			VL_Yield();
			//CK_SetTicsPerFrame();
			//IN_PumpEvents();
			//IN_ReadControls(0, &status );
		}
		lasttime = SD_GetTimeCount();
		if (timediff > 4)
		{
			timediff = 4;
		}

		// Move the game elements
		while (timediff-- && !done && (IN_GetLastScan() != IN_SC_Escape))
		{
			IN_PumpEvents();
			IN_ReadControls(0, &status);
			// Move Keen's paddle
			if (status.xDirection < 0 || IN_GetKeyState(IN_SC_LeftArrow))
			{
				if (keen_x > 78)
				{
					keen_x -= 2;
				}
			}
			else if (status.xDirection > 0 || IN_GetKeyState(IN_SC_RightArrow))
			{
				if (keen_x < 219)
					keen_x += 2;
			}

			// Start a new round if there was a point
			if (new_round)
			{
				ball_visible = 0;
				start_delay = 70;
				speedup_delay = 10;
				new_round = 0;

				/* Erase the ball */
				VH_Bar(old_ball_x, old_ball_y, 6, 5, 8);
			}

			if (ball_visible && (comp_move_counter++ % 3) != 0)
			{
				ball_x = ball_real_x / 4;
				if ((ball_x & 1) == 0)
					ball_x += US_RndT() & 1;

				// Move computer paddle to the ball
				if (comp_x + 6 < ball_x && comp_x < 219)
					comp_x++;
				else if (comp_x + 6 > ball_x && comp_x > 78)
					comp_x--;
			}

			if (ball_visible == 0)
			{
				if (--start_delay == 0)
				{
					ball_visible = 1;
					ball_x_speed = 1 - (US_RndT() % 3);
					ball_y_speed = 3;
					if (keen_won_last)
						ball_y_speed = -ball_y_speed;
					ball_real_x = 612;
					ball_real_y = 396;
				}
			}

			// Wait until the ball has been served
			if (!ball_visible)
			{
				continue;
			}

			// Bounce ball off of side wall
			if ((ball_real_x + ball_x_speed) / 4 > 228 || (ball_real_x + ball_x_speed) / 4 < 78)
			{
				SD_PlaySound(SOUND_PONGWALL);
				ball_x_speed = -ball_x_speed;
			}

			// Move ball in the X direction
			ball_real_x += ball_x_speed;

			// Check if computer scores a point
			if ((ball_real_y + ball_y_speed) / 4 > 137)
			{
				new_round = 1;
				keen_won_last = 0;
				comp_score++;
				SD_PlaySound(SOUND_COMPSCORE);
				USL_DrawPaddleWarScore(keen_score, comp_score);
				if (comp_score == 21)
				{
					USL_CtlDialog("You lost!", "Press any key", NULL);
					done = 1;
					continue;
				}
			}
			// Check if Keen scores a point
			else if ((ball_real_y + ball_y_speed) / 4 < 62)
			{
				new_round = 1;
				keen_won_last = 1;
				keen_score++;
				SD_PlaySound(SOUND_KEENSCORE); /* play_sound */
				USL_DrawPaddleWarScore(keen_score, comp_score);
				if (keen_score == 21)
				{
					USL_CtlDialog("You won!", "Press any key", NULL);
					done = 1;
					continue;
				}
			}

			ball_real_y += ball_y_speed;
			ball_x = ball_real_x / 4;
			ball_y = ball_real_y / 4;

			if (!new_round)
			{
				// Check if ball hits comp paddle
				if (ball_y_speed < 0 && ball_y >= 66 && ball_y < 69 && (comp_x - 5) <= ball_x && (comp_x + 11) > ball_x)
				{
					bounce_point = comp_x;
					y_bounce = 1;
					SD_PlaySound(SOUND_COMPPADDLE);
				}
				else if (ball_y_speed > 0 && ball_y >= 132 && ball_y < 135 && (keen_x - 5) <= ball_x && (keen_x + 11) > ball_x)
				{
					if (ball_y_speed / 4 < 3)
					{
						if (--speedup_delay == 0)
						{
							ball_y_speed++;
							speedup_delay = 10;
						}
					}
					bounce_point = keen_x;
					y_bounce = 1;
					SD_PlaySound(SOUND_KEENPADDLE);
				}

				if (y_bounce)
				{
					ball_y_speed = -ball_y_speed;
					ball_x_speed = (ball_x + 5 - bounce_point) / 2 - 4; /* or / 4? */
					if (ball_x_speed == 0)
						ball_x_speed--;
					y_bounce = 0;
				}
			}
		}

		if (ball_visible)
		{
			VH_Bar(old_ball_x, old_ball_y, 6, 5, 8);
			old_ball_x = ball_x;
			old_ball_y = ball_y;

			/* draw_sprite? */
			// Omnispeak doesn't emulate sprite shifts properly, so paddle war
			// (which works around this by having two sprites, one shifted one
			// pixel over) ends up shifted an extra pixel sometimes. By ignoring
			// the lower bit of ball_x here, we emulate the shift behaviour we need.
			VH_DrawSprite(ball_x & ~1, ball_y, (ball_x & 1) ? SPR_BALL1 : SPR_BALL0);
		}

		// Draw Computer Paddle
		VH_Bar(old_comp_x - 3, 66, 16, 3, 8);
		old_comp_x = comp_x;
		VH_DrawSprite(comp_x, 66, SPR_PADDLE);

		// Draw Keen paddle
		VH_Bar(old_keen_x - 3, 135, 16, 3, 8);
		old_keen_x = keen_x;
		VH_DrawSprite(keen_x, 135, SPR_PADDLE);

		//sub_658();
		VL_Present();

	} while ((IN_GetLastScan() != IN_SC_Escape) && !done);

	IN_ClearKeysDown();
}

bool CK_PaddleWar(US_CardMsg msg, US_CardItem *item)
{
	if (msg != US_MSG_CardEntered)
		return 0;

	/* Draw the watch */
	VH_DrawBitmap(0, 0, PIC_WRISTWATCH);

	/* Draw the PaddleWar title */
	VH_DrawBitmap(130, 48, PIC_PADDLEWAR);

	/* Draw a line above the playing area */
	VH_HLine(77, 231, 60, 10);

	/* Draw a line below the playing area */
	VH_HLine(77, 231, 143, 10);

	/* Play the game */
	USL_PlayPaddleWar();
	return 1;
}

#ifdef EXTRA_JOYSTICK_OPTIONS
void CK_US_SetJoystickName(US_CardItem *item, int joystick)
{
	static char str[US_MAX_JOYSTICKS][US_MAX_JOYSTICK_NAME_LENGTH + 1];
	char *pos = str[joystick];
	const char *name = IN_GetJoyName(joystick);
	if (name)
	{
		strcpy(pos, "USE ");
		pos += 4;
		if ((strlen(name) + 4) > US_MAX_JOYSTICK_NAME_LENGTH)
		{
			int n = US_MAX_JOYSTICK_NAME_LENGTH - 7;
			memcpy(pos, name, n);
			pos += n;
			strcpy(pos, "...");
		}
		else
		{
			strcpy(pos, name);
		}
	}
	else
		sprintf(pos, "USE JOYSTICK #%d", joystick + 1);
	item->caption = str[joystick];
}
#endif
void CK_US_UpdateOptionsMenus(void)
{

	ck_us_optionsMenuItems[0].caption = ck_scoreBoxEnabled ? "SCORE BOX (ON)" : "SCORE BOX (OFF)";
	ck_us_optionsMenuItems[1].caption = ck_twoButtonFiring ? "TWO-BUTTON FIRING (ON)" : "TWO-BUTTON FIRING (OFF)";
	ck_us_optionsMenuItems[2].caption = ck_fixJerkyMotion ? "FIX JERKY MOTION (ON)" : "FIX JERKY MOTION (OFF)";
	ck_us_optionsMenuItems[3].caption = ck_svgaCompatibility ? "SVGA COMPATIBILITY (ON)" : "SVGA COMPATIBILITY (OFF)";
#ifdef EXTRA_GRAPHICS_OPTIONS
	ck_us_optionsMenuItems[4].caption = vl_isFullScreen ? "FULLSCREEN (ON)" : "FULLSCREEN (OFF)";
	ck_us_optionsMenuItems[5].caption = vl_isAspectCorrected ? "CORRECT ASPECT RATIO (ON)" : "CORRECT ASPECT RATIO (OFF)";
	ck_us_optionsMenuItems[6].caption = vl_hasOverscanBorder ? "OVERSCAN BORDER (ON)" : "OVERSCAN BORDER (OFF)";
	ck_us_optionsMenuItems[7].caption = vl_isIntegerScaled ? "INTEGER SCALING (ON)" : "INTEGER SCALING (OFF)";
#endif
#ifdef EXTRA_JOYSTICK_OPTIONS
	ck_us_joyconfMenuItems[(int)IN_joy_modern].caption = in_joyAdvancedMotion ? "MOTION MODE (MODERN)" : "MOTION MODE (CLASSIC)";
#endif

	// Disable Two button firing selection if required
	ck_us_buttonsMenuItems[2].state &= ~US_IS_Disabled;
	if (ck_twoButtonFiring)
		ck_us_buttonsMenuItems[2].state |= US_IS_Disabled;

	if (IN_JoyPresent(0))
		ck_us_configureMenuItems[4].state &= ~US_IS_Disabled;
	else
		ck_us_configureMenuItems[4].state |= US_IS_Disabled;
	if (IN_JoyPresent(1))
		ck_us_configureMenuItems[5].state &= ~US_IS_Disabled;
	else
		ck_us_configureMenuItems[5].state |= US_IS_Disabled;
#ifdef EXTRA_JOYSTICK_OPTIONS
	CK_US_SetJoystickName(&ck_us_configureMenuItems[4], 0);
	CK_US_SetJoystickName(&ck_us_configureMenuItems[5], 1);
	if (IN_JoyPresent(0) || IN_JoyPresent(1))
		ck_us_configureMenuItems[6].state &= ~US_IS_Disabled;
	else
		ck_us_configureMenuItems[6].state |= US_IS_Disabled;
#endif

		/* Set up the gamepad menu item */
#if 0
	ck_us_configureMenuItems[6].state |= US_IS_Disabled;
	if ( in_controlType == IN_ctrl_Joystick1 || in_controlType == IN_ctrl_Joystick2 )
		ck_us_configureMenuItems[6].state &= ~US_IS_Disabled;
	ck_us_configureMenuItems[6].caption = gamepad ? "USE GRAVIS GAMEPAD (ON)" : "USE GRAVIS GAMEPAD (OFF)";
#endif
}

typedef struct
{
	const char *name;
	int sprite;
	int xofs;
	int yofs;
} CK_CreatureType;

static int currentCreature = -1;

CK_CreatureType ck6_creatures[] = {
	{"Bip", 269, -2, 0},
	{"Babobba", 288, 0, 0},
	{"Blorb", 399, -2, 0},
	{"Gik", 387, -1, 0},
	{"Ceilick", 246, 0, 0},
	{"Blooglet", 351, -2, 0},
	{"Blooguard", 254, -3, -1},
	{"Flect", 317, -1, 0},
	{"Bobba", 405, -2, 0},
	{"Nospike", 298, -2, 0},
	{"Orbatrix", 335, -2, 1},
	{"Fleex", 239, -2, 0},
};

bool CK6_CreatureQuestion()
{
	static bool alreadyPassed = 0;

	if (alreadyPassed)
		return true;

	int var2 = 0;
	if (currentCreature == -1)
	{
		time_t t;
		struct tm *tt;
		time(&t);
		tt = localtime(&t);
		currentCreature = (tt->tm_hour + tt->tm_mday) % 12;
	}

	CA_UpLevel();
	CK_CreatureType creature = ck6_creatures[currentCreature];
	CA_ClearMarks();
	CA_MarkGrChunk(creature.sprite);
	CA_CacheMarks(NULL);

	VH_Bar(0, 0, 320, 200, 8);
	VH_SpriteTableEntry *sprite = VH_GetSpriteTableEntry(creature.sprite - ca_gfxInfoE.offSprites);
	int w = sprite->width;
	int h = sprite->height;

	US_CenterWindow(30, (h + 41) / 8 + 1);
	US_SetPrintY(US_GetWindowY() + 2);
	US_CPrint("What is the name of this creature?");
	int x = US_GetWindowX() + (US_GetWindowW() - w) / 2 + (creature.xofs * 8);
	int y = US_GetWindowY() + 15;

	if (creature.sprite == 246)
		y++;
	else
		y += creature.yofs * 3;

	VH_DrawSprite(x, y, creature.sprite);

	y = US_GetWindowY() + US_GetWindowH() - 0x10;
	int varC = 100;
	x = US_GetWindowX() + (US_GetWindowW() - 100) / 2;
	VH_Bar(x, y, 100, 14, 0);
	VH_Bar(x + 1, y + 1, varC - 2, 12, 15);
	x += 2;
	y += 2;
	varC -= 8;
	VL_Present();

	char buf[16];
	if (US_LineInput(x, y, buf, NULL, true, 16, varC))
	{
		var2 = 1;
		// In the disassembly, a loop which appears to do a  case-insensitve strcmp
		if (CK_Cross_strcasecmp(buf, creature.name))
		{
			VH_Bar(0, 0, 320, 200, 8);
			US_CenterWindow(35, 5);
			US_SetPrintY(US_GetPrintY() + 11);
			US_CPrint("Sorry, that's not quite right.");
			US_CPrint("Please check your manual and try again.");
			VL_Present();
			IN_WaitButton();
			var2 = 0;
		}
	}

	VH_Bar(0, 0, 320, 200, 8);
	CA_DownLevel();
	alreadyPassed = var2;
	return var2;
}
