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
#include "id_sd.h"
#include "ck_play.h"

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

bool CK_US_ControlsMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_KeyboardMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_Joystick1MenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_Joystick2MenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_GamepadMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_US_ConfigureMenuProc(US_CardMsg msg, US_CardItem *item);
bool CK_PaddleWar(US_CardMsg msg, US_CardItem *item);

// A debug menu which doesn't seem to ever appear in the game.
US_CardItem ck_us_debugMenuItems[] ={
	{ US_ITEM_Normal, 0, IN_SC_None, "DEBUG", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_debugMenu ={ 0, 0, 88, 0, ck_us_debugMenuItems, 0, 0, 0, 0 };

// Sound Menu
US_CardItem ck_us_soundMenuItems[] ={
	{ US_ITEM_Radio, 0, IN_SC_N, "NO SOUND EFFECTS", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_P, "PC SPEAKER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_Q, "QUIET ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_soundMenu ={ 8, 0, 72, 0, ck_us_soundMenuItems, 0, 0, 0, 0 };

// Music Menu
US_CardItem ck_us_musicMenuItems[] ={
	{ US_ITEM_Radio, 0, IN_SC_N, "NO MUSIC", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Radio, 0, IN_SC_A, "ADLIB/SOUNDBLASTER", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_musicMenu ={ 8, 0, 73, 0, ck_us_musicMenuItems, 0, 0, 0, 0 };

// New Game Menu
US_CardItem ck_us_newGameMenuItems[] ={
	{ US_ITEM_Normal, 0, IN_SC_E, "BEGIN EASY GAME", US_Comm_NewEasyGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_N, "BEGIN NORMAL GAME", US_Comm_NewNormalGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_H, "BEGIN HARD GAME", US_Comm_NewHardGame, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_newGameMenu ={ 8, 0, 68, 0, ck_us_newGameMenuItems, 0, 1, 0, 0/*, 0*/ };

// Load/Save Game Menus
US_CardItem ck_us_loadSaveMenuItems[] ={
	{ US_ITEM_Normal, 0, IN_SC_One, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Two, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Three, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Four, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Five, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Six, 0, US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_loadGameMenu ={ 4, 3, 69, 0, ck_us_loadSaveMenuItems, &CK_US_LoadGameMenuProc, 0, 0, 0 };
US_Card ck_us_saveGameMenu ={ 4, 3, 70, 0, ck_us_loadSaveMenuItems, &CK_US_SaveGameMenuProc, 0, 0, 0 };

// Dummy Menus

US_Card ck_us_scoreBoxMenu ={ 0, 0, 0, 0, 0, &CK_US_ScoreBoxMenuProc, 0, 0, 0 };
US_Card ck_us_twoButtonFiringMenu ={0, 0, 0, 0, 0, &CK_US_TwoButtonFiringMenuProc, 0, 0, 0 };
US_Card ck_us_fixJerkyMotionMenu ={ 0, 0, 0, 0, 0, &CK_US_FixJerkyMotionMenuProc, 0, 0, 0 };
US_Card ck_us_svgaCompatibilityMenu ={ 0, 0, 0, 0, 0, &CK_US_SVGACompatibilityMenuProc, 0, 0, 0 };


// Options menu
US_CardItem ck_us_optionsMenuItems[] ={
	{ US_ITEM_Submenu, 0, IN_SC_S, "", US_Comm_None, &ck_us_scoreBoxMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_T, "", US_Comm_None, &ck_us_twoButtonFiringMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "", US_Comm_None, &ck_us_fixJerkyMotionMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_C, "", US_Comm_None, &ck_us_svgaCompatibilityMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_optionsMenu ={ 8, 0, 78, 0, ck_us_optionsMenuItems, 0, 0, 0, 0 };

// Movement Kbd Controls Menu
US_CardItem ck_us_movementMenuItems[] ={
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

US_Card ck_us_movementMenu ={ 0, 0, 75, 0, ck_us_movementMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0};

// Buttons Kbd Controls Menu
US_CardItem ck_us_buttonsMenuItems[] ={
	{ US_ITEM_Normal, 0, IN_SC_J, "JUMP", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_P, "POGO", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_F, "FIRE", US_Comm_None, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_buttonsMenu ={ 0, 0, 76, 0, ck_us_buttonsMenuItems, &CK_US_ControlsMenuProc, 0, 0, 0 };

// Keyboard Menu
US_CardItem ck_us_keyboardMenuItems[] ={
	{ US_ITEM_Submenu, 0, IN_SC_M, "MOVEMENT", US_Comm_None, &ck_us_movementMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "BUTTONS", US_Comm_None, &ck_us_buttonsMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_keyboardMenu ={ 8, 0, 74, 0, ck_us_keyboardMenuItems, &CK_US_KeyboardMenuProc, 0, 0, 0 };

// Custom Menus

US_Card ck_us_joystick1Menu ={ 0, 0, 77, 0, 0, &CK_US_Joystick1MenuProc, 0, 0, 0 };
US_Card ck_us_joystick2Menu ={ 0, 0, 77, 0, 0, &CK_US_Joystick2MenuProc, 0, 0, 0 };
US_Card ck_us_gamepadMenu ={ 0, 0, 77, 0, 0, &CK_US_GamepadMenuProc, 0, 0, 0 };

// Configure Menu
US_CardItem ck_us_configureMenuItems[] ={
	{ US_ITEM_Submenu, 0, IN_SC_S, "SOUND", US_Comm_None, &ck_us_soundMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_M, "MUSIC", US_Comm_None, &ck_us_musicMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_O, "OPTIONS", US_Comm_None, &ck_us_optionsMenu, 0, 0 },
	{ US_ITEM_Submenu, US_IS_Gap, IN_SC_K, "KEYBOARD", US_Comm_None, &ck_us_keyboardMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_One, "USE JOYSTICK #1", US_Comm_None, &ck_us_joystick1Menu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_Two, "USE JOYSTICK #2", US_Comm_None, &ck_us_joystick2Menu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_G, "", US_Comm_None, &ck_us_gamepadMenu, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_configureMenu ={ 0, 0, 71, 0, ck_us_configureMenuItems, &CK_US_ConfigureMenuProc, 0, 0, 0 };

// Paddle War!

US_Card ck_us_paddleWarMenu ={ 0, 0, 0, 0, 0, &CK_PaddleWar, 0, 0, 0 };

// Main Menu
US_CardItem ck_us_mainMenuItems[] ={
	{ US_ITEM_Submenu, 0, IN_SC_N, "NEW GAME", US_Comm_None, &ck_us_newGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_L, "LOAD GAME", US_Comm_None, &ck_us_loadGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_S, "SAVE GAME", US_Comm_None, &ck_us_saveGameMenu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_C, "CONFIGURE", US_Comm_None, &ck_us_configureMenu, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_R, NULL, US_Comm_ReturnToGame, 0, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_E, "END GAME", US_Comm_EndGame, 0, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_P, "PADDLE WAR", US_Comm_None, &ck_us_paddleWarMenu, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_Q, "QUIT", US_Comm_Quit, 0, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, 0, US_Comm_None, 0, 0, 0 }
};

US_Card ck_us_mainMenu ={ 32, 4, 67, 0, ck_us_mainMenuItems, 0, 0, 0, 0 };

extern US_Card *us_currentCard;
extern IN_ScanCode *key_controls[];

bool CK_US_ControlsMenuProc(US_CardMsg msg, US_CardItem *item)
{
	int which_control;
	int result = 0;
	int print_x, print_y;
	int fontcolour;

	which_control = (us_currentCard == &ck_us_movementMenu) ?
		(item - ck_us_movementMenuItems) + 3 : (item - ck_us_buttonsMenuItems);

	switch ( msg )
	{
	case US_MSG_CardEntered:
		// game_controller[0] = CTRL_KEYBOARD;
		break;

	case US_MSG_DrawItem:

		// Draw the item Icon and the key's name
		VH_Bar( 75, item->y, 159, 8, 8 );
		USL_DrawCardItemIcon( item );

		fontcolour = (item->state & US_IS_Selected) ? 10 : 2;
		print_x = item->x + 8;
		print_y = item->y + 1;
		VH_DrawPropString( item->caption, print_x, print_y, 4, fontcolour );

		// Draw the outer green box
		VH_Bar( item->x + 90, item->y, 40, 8, fontcolour);
		VH_Bar( item->x + 91, item->y + 1, 38, 6, 8 );

		print_x = item->x + 96;
		print_y = item->y + 1;
		VH_DrawPropString( IN_GetScanName( *key_controls[which_control] ), print_x, print_y, 4, fontcolour );
		result = 1;
		break;

	case US_MSG_ItemEntered:
		CK_US_ControlsMenuProc( US_MSG_DrawItem, item );
		set_key_control( item, which_control );
		US_DrawCards();
		result = 1;
		break;
	}

	return result;
}

void set_key_control( US_CardItem *item, int which_control )
{
	long lasttime;
	int cursor;
	IN_ControlFrame state;
	char k;
	int i, used;

	cursor = 0;
	lasttime = 0;
	char last_scan = 0;
	int fontcolour = 2;

	IN_ClearKeysDown();

	/* Prompt the user to press a key */
	do
	{
		CK_SetTicsPerFrame();
		IN_PumpEvents();

		/* Flicker the cursor */
		if ( CK_GetNumTotalTics() >= lasttime )
		{
			/* time_count */
			cursor = !cursor;

			/* Draw the rectangle */
			VH_Bar( item->x + 90, item->y, 40, 8, fontcolour ^ 8 );
			VH_Bar( item->x + 91, item->y + 1, 38, 6, 8 );

			/* Draw the cursor */
			if ( cursor )
				VH_DrawTile8( item->x + 106, item->y, 100 );

			//VW_UpdateScreen();
			lasttime = CK_GetNumTotalTics() + 35;	/* time_count */
			VL_Present();
		}

		/* A button push will cancel the key selection */
		IN_ReadControls(0, &state );
		last_scan = IN_GetLastScan();
		while ( state.jump || state.pogo )
		{
			IN_ReadControls(0, &state );
			last_scan = IN_SC_Escape;
		}

		/* Disallow left shift for some reason */
		// disable();
		if ( IN_GetLastScan() == IN_SC_LeftShift )
			last_scan = 0;
		// enable();

	} while ( (k = last_scan) == 0 );

	/* If they didn't cancel the process with ESC */
	if ( last_scan != IN_SC_Escape )
	{
		used = 0;
		i = 0;

		/* Make sure the key they chose is not already used */
		for ( i = 0; i < 11; i++ )
		{
			/* Don't check the one we're setting */
			if ( i != which_control )
			{
				if ( *(key_controls[i]) == k )
				{
					used = 1;
					break;
				}
			}
		}

		if ( used )
			green_message_box( "Key already used", "Press any key", NULL );
		else
			*(key_controls[ which_control ]) = k;
	}

	IN_ClearKeysDown();
}

bool CK_US_KeyboardMenuProc(US_CardMsg msg, US_CardItem *item)
{
	// Set keyboard as game controller if this menu is entered
	if ( msg == US_MSG_CardEntered )
	{
		// TODO: Implement gamepad and joystick support
#if 0
		game_controller[0] = CTRL_KEYBOARD;
		gamepad = 0;
		update_options_menus();
#endif 
	}
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

void show_paddlewar_score( int keen_score, int comp_score )
{
	// NOTE: This is modified a little from the original
	// exe in order to align the text and to set the proper font and color

	int print_color = 10;
	int print_y = 52;
	int w, h;

	int old_print_font = US_GetPrintFont();
	int old_print_color = US_GetPrintColour();

	US_SetPrintFont(4);
	US_SetPrintColour(print_color);

	// Draw keen's score
	int print_x = 80;
	VH_Bar(print_x, print_y, 42, 6, 8);
	char *keenString = "KEEN:";
	VH_MeasurePropString(keenString, &w, &h, 4);
	VH_DrawPropString(keenString, print_x, print_y, 4, print_color);
	US_SetPrintX(print_x + w);
	US_SetPrintY(print_y);
	US_PrintF("%u", keen_score);

	// Draw Comp score
	print_x = 182;
	VH_Bar( print_x, print_y, 50, 6, 8 );
	char *compString = "COMP:";
	VH_MeasurePropString(compString, &w, &h, 4);
	VH_DrawPropString(compString, print_x, print_y, 4, print_color );

	US_SetPrintX(print_x + w);
	US_SetPrintY(print_y);
	US_PrintF("%u", comp_score);

	US_SetPrintFont(old_print_font);
	US_SetPrintColour(old_print_color);
}

void paddlewar( void )
{
	int ball_visible, new_round, y_bounce, done, keen_won_last, comp_move_counter;
	int ball_y, keen_x, comp_x, bounce_point, ball_real_x, ball_real_y;
	int old_keen_x, old_comp_x, old_ball_x, old_ball_y, keen_score, comp_score;
	int speedup_delay, ball_x_speed;
	long start_delay, lasttime, timediff;
	IN_ControlFrame status;
	int ball_y_speed, ball_x;

	keen_x = comp_x = 148;
	ball_real_x = ball_real_y = ball_y_speed = 0;
	old_ball_x = old_comp_x = old_keen_x = 78;
	old_ball_y = 62;
	keen_score = comp_score = 0;
	show_paddlewar_score( 0, 0 );
	comp_move_counter = 0;
	y_bounce = 0;
	new_round = 1;
	done = 0;
	keen_won_last = 0;
	lasttime = CK_GetNumTotalTics();

	do
	{
		// Delay Processing
		while ( (timediff = CK_GetNumTotalTics() - lasttime) == 0 )
		{
			CK_SetTicsPerFrame();
			IN_PumpEvents();
			IN_ReadControls(0, &status );
		}
		lasttime = CK_GetNumTotalTics();
		if ( timediff > 4 )
		{
			timediff = 4;
		}

		// Move the game elements
		while ( timediff-- && !done && (IN_GetLastScan() != IN_SC_Escape) )
		{
			// Move Keen's paddle
			if ( status.xDirection < 0 || IN_GetKeyState(IN_SC_LeftArrow) )
			{
				if ( keen_x > 78 )
				{
					keen_x -= 2;
				}
			}
			else if ( status.xDirection > 0 || IN_GetKeyState(IN_SC_RightArrow) )
			{
				if ( keen_x < 219 )
					keen_x += 2;
			}

			// Start a new round if there was a point
			if ( new_round )
			{
				ball_visible = 0;
				start_delay = 70;
				speedup_delay = 10;
				new_round = 0;

				/* Erase the ball */
				VH_Bar( old_ball_x, old_ball_y, 6, 5, 8 );
			}

			if ( ball_visible && (comp_move_counter++ % 3) != 0 )
			{
				ball_x = ball_real_x / 4;
				if ( ball_x & 1 == 0 )
					ball_x += US_RndT() & 1;

				// Move computer paddle to the ball
				if ( comp_x + 6 < ball_x && comp_x < 219 )
					comp_x++;
				else if ( comp_x + 6 > ball_x && comp_x > 78 )
					comp_x--;
			}

			if ( ball_visible == 0  )
			{
				if ( --start_delay == 0 )
				{
					ball_visible = 1;
					ball_x_speed = 1 - (US_RndT() % 3);
					ball_y_speed = 3;
					if ( keen_won_last )
						ball_y_speed = -ball_y_speed;
					ball_real_x = 612;
					ball_real_y = 396;
				}
			}

			// Wait until the ball has been served
			if ( !ball_visible)
			{
				continue;
			}

			// Bounce ball off of side wall
			if ( (ball_real_x + ball_x_speed) / 4 > 228 || (ball_real_x + ball_x_speed) / 4 < 78 )
			{
				SD_PlaySound(SOUND_UNKNOWN47);
				ball_x_speed = -ball_x_speed;
			}

			// Move ball in the X direction
			ball_real_x += ball_x_speed;

			// Check if computer scores a point
			if ( (ball_real_y + ball_y_speed) / 4 > 137 )
			{
				new_round = 1;
				keen_won_last = 0;
				comp_score++;
				SD_PlaySound(SOUND_UNKNOWN49);
				show_paddlewar_score( keen_score, comp_score );
				if ( comp_score == 21 )
				{
					green_message_box( "You lost!", "Press any key", NULL );
					done = 1;
					continue;
				}
			}
				// Check if Keen scores a point
			else if ( (ball_real_y + ball_y_speed) / 4 < 62 )
			{
				new_round = 1;
				keen_won_last = 1;
				keen_score++;
				SD_PlaySound(SOUND_UNKNOWN50);	/* play_sound */
				show_paddlewar_score( keen_score, comp_score );
				if ( keen_score == 21 )
				{
					green_message_box( "You won!", "Press any key", NULL );
					done = 1;
					continue;
				}
			}

			ball_real_y += ball_y_speed;
			ball_x = ball_real_x / 4;
			ball_y = ball_real_y / 4;

			if ( !new_round )
			{
				// Check if ball hits comp paddle
				if ( ball_y_speed < 0 && ball_y >= 66 && ball_y < 69 && (comp_x - 5) <= ball_x && (comp_x + 11) > ball_x )
				{
					bounce_point = comp_x;
					y_bounce = 1;
					SD_PlaySound(SOUND_UNKNOWN48);
				}
				else if ( ball_y_speed > 0 && ball_y >= 132 && ball_y < 135 && (keen_x - 5) <= ball_x && (keen_x + 11) > ball_x )
				{
					if ( ball_y_speed / 4 < 3 )
					{
						if ( --speedup_delay == 0 )
						{
							ball_y_speed++;
							speedup_delay = 10;
						}
					}
					bounce_point = keen_x;
					y_bounce = 1;
					SD_PlaySound(SOUND_UNKNOWN46);
				}

				if ( y_bounce )
				{
					ball_y_speed = -ball_y_speed;
					ball_x_speed = (ball_x + 5 - bounce_point) / 2 - 4;	/* or / 4? */
					if ( ball_x_speed == 0 )
						ball_x_speed--;
					y_bounce = 0;
				}
			}
		}


		if ( ball_visible )
		{
			VH_Bar( old_ball_x, old_ball_y, 6, 5, 8 );
			old_ball_x = ball_x;
			old_ball_y = ball_y;

			/* draw_sprite? */
			// Omnispeak doesn't emulate sprite shifts properly, so paddle war
			// (which works around this by having two sprites, one shifted one
			// pixel over) ends up shifted an extra pixel sometimes. By ignoring
			// the lower bit of ball_x here, we emulate the shift behaviour we need.
			VH_DrawSprite( ball_x & ~1, ball_y, (ball_x & 1) ? 104 : 103 );
		}

		// Draw Computer Paddle
		VH_Bar( old_comp_x - 3, 66, 16, 3, 8 );
		old_comp_x = comp_x;
		VH_DrawSprite( comp_x, 66, 102 );

		// Draw Keen paddle
		VH_Bar( old_keen_x - 3, 135, 16, 3, 8 );
		old_keen_x = keen_x;
		VH_DrawSprite( keen_x, 135, 102 );

		//sub_658();
		VL_Present();

	}  while ( (IN_GetLastScan() != IN_SC_Escape) && !done );

	IN_ClearKeysDown();
}

bool CK_PaddleWar(US_CardMsg msg, US_CardItem *item)
{
	if ( msg != US_MSG_CardEntered )
		return 0;

	/* Draw the watch */
	VH_DrawBitmap( 0, 0, 82 );

	/* Draw the PaddleWar title */
	VH_DrawBitmap( 130, 48, 79 );

	/* Draw a line above the playing area */
	VH_HLine( 77, 231, 60, 10 );

	/* Draw a line below the playing area */
	VH_HLine( 77, 231, 143, 10 );

	/* Play the game */
	paddlewar();
	return 1;
}
