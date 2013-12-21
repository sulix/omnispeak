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

#include "id_us.h"
#include "id_in.h"



bool CK_US_LoadGameMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_SaveGameMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_ScoreBoxMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_TwoButtonFiringMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_FixJerkyMotionMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_SVGACompatibilityMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_ControlsMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_KeyboardMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_Joystick1MenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_Joystick2MenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_GamepadMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_US_ConfigureMenuProc(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

bool CK_PaddleWar(US_CardMsg msg, US_CardItem *item)
{
	return false;
}

// A debug menu which doesn't seem to ever appear in the game.
US_CardItem ck_us_debugMenuItems[] = {
	{ US_ITEM_Normal, 0, IN_SC_None, "DEBUG", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_debugMenu = { 0, 0, 88, 0, &ck_us_debugMenuItems, 0, 0, 0, 0 };

// Sound Menu
US_CardItem ck_us_soundMenuItems[] = {
	{ US_ITEM_Radio, 0, IN_SC_N, "NO SOUND EFFECTS", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_P, "PC SPEAKER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_Q, "QUIET ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_soundMenu = { 8, 0, 92, 0, &ck_us_soundMenuItems, 0, 0, 0, 0 };

// Music Menu
US_CardItem ck_us_musicMenuItems[] = {
	{ US_ITEM_Radio, 0, IN_SC_N, "NO MUSIC", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_musicMenu = { 8, 0, 94, 0, &ck_us_musicMenuItems, 0, 0, 0, 0 };

// New Game Menu
US_CardItem ck_us_newGameMenuItems[] = {
	{ US_ITEM_Normal, 0, IN_SC_E, "BEGIN EASY GAME", US_Comm_NewEasyGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_N, "BEGIN NORMAL GAME", US_Comm_NewNormalGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_H, "BEGIN HARD GAME", US_Comm_NewHardGame, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_newGameMenu = { 8, 0, 89, 0, &ck_us_newGameMenuItems, 0, 1, 0, 0, 0 };

// Load/Save Game Menus
US_CardItem ck_us_loadSaveMenuItems[] = {
	{ US_ITEM_Normal, 0, IN_SC_One, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Two, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Three, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Four, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Five, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Six, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_loadGameMenu = { 4, 3, 90, 0, &ck_us_loadSaveMenuItems, &CK_US_LoadGameMenuProc, 0, 0, 0 };
US_Card ck_us_saveGameMenu = { 4, 3, 90, 0, &ck_us_loadSaveMenuItems, &CK_US_SaveGameMenuProc, 0, 0, 0 };

// Dummy Menus

US_Card ck_us_scoreBoxMenu = { 0, 0, 0, 0, 0, &CK_US_ScoreBoxMenuProc, 0, 0, 0 };
US_Card ck_us_twoButtonFiringMenu = {0, 0, 0, 0, 0, &CK_US_TwoButtonFiringMenuProc, 0, 0, 0 };
US_Card ck_us_fixJerkyMotionMenu = { 0, 0, 0, 0, 0, &CK_US_FixJerkyMotionMenuProc, 0, 0, 0 };
US_Card ck_us_svgaCompatibilityMenu = { 0, 0, 0, 0, 0, &CK_US_SVGACompatibilityMenuProc, 0, 0, 0 };


// Options menu
US_CardItem ck_us_optionsMenuItems[] = {
	{ US_ITEM_Submenu, 0, IN_SC_S, "", US_Comm_None, &ck_us_scoreBoxMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_T, "", US_Comm_None, &ck_us_twoButtonFiringMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "", US_Comm_None, &ck_us_fixJerkyMotionMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_C, "", US_Comm_None, &ck_us_svgaCompatibilityMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_optionsMenu = { 8, 0, 99, 0, &ck_us_optionsMenuItems, 0, 0, 0, 0 };

// Movement Kbd Controls Menu
US_CardItem ck_us_movementMenuItems[] = {
	{ US_ITEM_Normal, 0, IN_SC_None, "UP & LEFT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "UP", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "UP & RIGHT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "RIGHT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "DOWN & RIGHT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "DOWN", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "DOWN & LEFT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_None, "LEFT", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_movementMenu = { 0, 0, 96, 0, &ck_us_movementMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0};

// Buttons Kbd Controls Menu
US_CardItem ck_us_buttonsMenuItems[] = {
	{ US_ITEM_Normal, 0, IN_SC_J, "JUMP", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_P, "POGO", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_F, "FIRE", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_buttonsMenu = { 0, 0, 97, 0, &ck_us_buttonsMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0 };

// Keyboard Menu
US_CardItem ck_us_keyboardMenuItems[] = {
	{ US_ITEM_Submenu, 0, IN_SC_M, "MOVEMENT", US_Comm_None, &ck_us_movementMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "BUTTONS", US_Comm_None, &ck_us_buttonsMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_keyboardMenu = { 8, 0, 95, 0, &ck_us_keyboardMenuItems, &CK_US_KeyboardMenuProc, 0, 0, 0 };

// Custom Menus

US_Card ck_us_joystick1Menu = { 0, 0, 98, 0, 0, &CK_US_Joystick1MenuProc, 0, 0, 0 };
US_Card ck_us_joystick2Menu = { 0, 0, 98, 0, 0, &CK_US_Joystick2MenuProc, 0, 0, 0 };
US_Card ck_us_gamepadMenu = { 0, 0, 98, 0, 0, &CK_US_GamepadMenuProc, 0, 0, 0 };

// Configure Menu
US_CardItem ck_us_configureMenuItems[] = {
	{ US_ITEM_Submenu, 0, IN_SC_S, "SOUND", US_Comm_None, &ck_us_soundMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "MENU", US_Comm_None, &ck_us_musicMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_O, "OPTIONS", US_Comm_None, &ck_us_optionsMenu, 0, 0 },
	{ US_ITEM_Submenu, US_IS_Gap, IN_SC_K, "KEYBOARD", US_Comm_None, &ck_us_keyboardMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_One, "USE JOYSTICK #1", US_Comm_None, &ck_us_joystick1Menu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_Two, "USE JOYSTICK #2", US_Comm_None, &ck_us_joystick2Menu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_G, "", US_Comm_None, &ck_us_gamepadMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_configureMenu = { 0, 0, 92, 0, &ck_us_configureMenuItems, &CK_US_ConfigureMenuProc, 0, 0, 0 };

// Paddle War!

US_Card ck_us_paddleWarMenu = { 0, 0, 0, 0, 0, &CK_PaddleWar, 0, 0, 0 };

// Main Menu
US_CardItem ck_us_mainMenuItems[] = {
	{ US_ITEM_Submenu, 0, IN_SC_N, "NEW GAME", US_Comm_None, &ck_us_newGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_L, "LOAD GAME", US_Comm_None, &ck_us_loadGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_S, "SAVE GAME", US_Comm_None, &ck_us_saveGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_C, "CONFIGURE", US_Comm_None, &ck_us_configureMenu, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_R, 0, US_Comm_ReturnToGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_E, "END GAME", US_Comm_EndGame, 0, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_P, "PADDLE WAR", US_Comm_None, &ck_us_paddleWarMenu, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Q, "QUIT", US_Comm_Quit, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_mainMenu = { 32, 4, 88, 0, &ck_us_mainMenuItems, 0, 0, 0, 0 };
