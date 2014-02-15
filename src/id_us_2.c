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

/*
 * ID_US_2 handles the main menu in keen.
 * The system has a stack of 'US_Card's, each of which contain an array of
 * menu items. Each card has a callback, which handles messages.
 * 
 * NOTE: The original Keen game used a callback to handle string printing
 * and measurement, presumably so that printing routines could be swapped
 * on the fly.  Here, we are just calling VH_DrawPropString and VH_MeasurePropString 
 * directly.
 */

#include <stdbool.h>
// Need this for NULL
#include <stddef.h>
// For strcpy, strcat
#include <string.h>

#include "id_heads.h"
#include "id_us.h"
#include "id_in.h"
#include "id_vh.h"
#include "id_vl.h"
#include "id_sd.h"
#include "ck_def.h"
#include "ck_text.h"

void USL_DrawCardItemIcon(US_CardItem *item);
void set_key_control( US_CardItem *item, int which_control );
#include "ck_us_2.c"

// Card stack can have at most 7 cards
#define US_MAX_CARDSTACK 7

extern CK_Difficulty ck_startingDifficulty;
int game_unsaved, game_in_progress, quit_to_dos, load_game_error;
int fontnumber, fontcolour;

void (*p_exit_menu)(void) ;

// The last selected command.
US_CardCommand us_currentCommand = US_Comm_None;
bool command_confirmed;

static int us_cardStackIndex;
US_Card *us_currentCard;
US_Card *us_cardStack[US_MAX_CARDSTACK];

US_CardItem new_game_menu_items[] ={
	{ US_ITEM_Normal, 0, IN_SC_E, "BEGIN EASY GAME", US_Comm_NewEasyGame, NULL, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_N, "BEGIN NORMAL GAME", US_Comm_NewNormalGame, NULL, 0, 0 },
	{ US_ITEM_Normal, 0, IN_SC_H, "BEGIN HARD GAME", US_Comm_NewHardGame, NULL, 0, 0 },
	{ US_ITEM_None, 0, IN_SC_None, NULL,             US_Comm_None,  NULL, 0, 0 },
};

US_Card new_game_menu ={ 8, 0, 68, 0, new_game_menu_items, NULL, 0, 0, 0 };

US_CardItem main_menu_items[] ={
	{ US_ITEM_Submenu, 0, IN_SC_N,    "NEW GAME",   US_Comm_None, &new_game_menu, 0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_L,    "LOAD GAME",  US_Comm_None, /*load_game_menu*/NULL,  0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_S,    "SAVE GAME",  US_Comm_None, /*save_game_menu*/NULL,  0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_C,    "CONFIGURE",  US_Comm_None, /*&configure_menu*/NULL,  0, 0 },
	{ US_ITEM_Normal,  0, IN_SC_R,    NULL,         US_Comm_ReturnToGame, NULL, 0, 0 },
	{ US_ITEM_Normal,  0, IN_SC_E,    "END GAME",   US_Comm_EndGame, NULL,            0, 0 },
	{ US_ITEM_Submenu, 0, IN_SC_P,    "PADDLE WAR", US_Comm_None, /*paddlewar_menu*/NULL, 0, 0 },
	{ US_ITEM_Normal,  0, IN_SC_Q,    "QUIT",       US_Comm_Quit, NULL,  0, 0 },
	{ US_ITEM_None,    0, IN_SC_None, NULL,         US_Comm_None,  NULL,            0, 0 }
};

//AS:00A2
US_Card main_menu ={ 32, 4, 67, 0, main_menu_items, NULL, 0, 0, 0 };

void US_SetupCards(US_Card *initial)
{
	us_cardStackIndex = 0;
	us_cardStack[0] = initial;
	us_currentCard = initial;
}

US_Card *USL_PopCard()
{
	// If the stack is empty, we're in trouble.
	if (!us_cardStackIndex)
		return 0;

	--us_cardStackIndex;

	us_currentCard = us_cardStack[us_cardStackIndex];
	return us_currentCard;
}

void USL_PushCard(US_Card *card)
{
	if (us_cardStackIndex >= US_MAX_CARDSTACK)
		return;

	++us_cardStackIndex;
	us_currentCard = card;
	us_cardStack[us_cardStackIndex] = card;
}

void USL_DrawCardItemIcon(US_CardItem *item)
{
	// Send a US_MSG_DrawItemIcon message to the card.
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_DrawItemIcon, item))
		return;

	int gfxChunk;

	if (item->state & US_IS_Disabled)
	{
		if (item->state & US_IS_Selected)
			gfxChunk = 92 + 5;
		else
			gfxChunk = 92 + 4;
	}
	else
	{
		if ((item->type == US_ITEM_Radio) && !(item->state & US_IS_Checked))
		{
			if (item->state & US_IS_Selected)
				gfxChunk = 92 + 3;
			else
				gfxChunk = 92 + 2;
		}
		else
		{
			if (item->state & US_IS_Selected)
				gfxChunk = 92 + 1;
			else
				gfxChunk = 92 + 0;
		}
	}

	VH_DrawTile8(item->x, item->y, gfxChunk);
}

void USL_DrawCardItem(US_CardItem *item)
{
	// Send a US_MSG_DrawItemIcon message to the card.
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_DrawItem, item))
		return;

	// Gray out the area underneath the text
	VH_Bar(75, item->y, 159, 8, 8);

	USL_DrawCardItemIcon(item);

	int fontcolour = 10;
	if ( !(item->state & US_IS_Selected) ||  (item->state & US_IS_Disabled))
		fontcolour = 2;

	VH_DrawPropString(item->caption, item->x + 8, item->y + 1, 4, fontcolour);
}

const char *footer_str[3];

void USL_DrawMenuFooter( void )
{
	int w, h;

	fontcolour = 10;

	/* "Arrows move" */
	VH_DrawPropString( footer_str[2], 78, 135, 4, fontcolour );

	/* "Enter selects" */
	VH_MeasurePropString( footer_str[1], &w, &h, 4 );
	VH_DrawPropString( footer_str[1], 230 - w, 135, 4, fontcolour );

	/* "Esc to quit/back out" */
	VH_MeasurePropString( footer_str[0], &w, &h, 4 );
	VH_DrawPropString( footer_str[0], 74 + ((160 - w) / 2), 135 + h + 1, 4, fontcolour );

	fontcolour = 0;

	VH_HLine( 77, 231, 133, 10 );
}

void USL_DrawCard()
{
	// Run the custom draw procedure if it exists
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_Draw, 0))
		return;

	// Draw the header (if any))
	if (us_currentCard->gfxChunk)
	{
		VH_HLine(77, 231, 55, 10);
		VH_DrawBitmap(80, 48, us_currentCard->gfxChunk);
	}

	// Draw the Footer
	USL_DrawMenuFooter();

	// Draw each card item
	if (us_currentCard->items)
	{
		int itemX = us_currentCard->x + 74;
		if (itemX % 8)
			itemX += (8 - itemX % 8);

		int itemY = us_currentCard->y + 60;

		for (int itemIndex = 0; us_currentCard->items[itemIndex].type != US_ITEM_None; ++itemIndex)
		{
			US_CardItem *item = &(us_currentCard->items[itemIndex]);

			if (item->state & US_IS_Gap)
				itemY += 8;

			item->x = itemX;
			item->y = itemY;

			USL_DrawCardItem(item);
			itemY += 8;
		}

		if (us_currentCard->msgCallback)
			us_currentCard->msgCallback(US_MSG_PostDraw, 0);
	}
}

void US_DrawCards()
{
	// Draw the watch screen and the active card
	if (us_currentCard->items || us_currentCard->gfxChunk)
	{
		VH_DrawBitmap(0, 0, 82);
		USL_DrawCard();
	}

	VL_SetScrollCoords(0, 0);
	VL_Present();
}

void center_watch_window( int w, int h, int *x, int *y )
{
	VH_DrawMaskedBitmap( 74, 48, 99 );

	/* Calculate the position */
	*x = 74 + (160 - w) / 2;
	*y = 48 + (102 - h) / 2;

	/* Fill the background */
	VH_Bar( *x, *y, w + 1, h + 1, 8 );

	/* Draw the border */
	VH_HLine( *x - 1, *x + w + 1, *y - 1, 10 );
	VH_HLine( *x - 1, *x + w + 1, *y + h + 1, 10 );
	VH_VLine( *y - 1, *y + h + 1, *x - 1, 10 );
	VH_VLine( *y - 1, *y + h + 1, *x + w + 1, 10 );
}

void load_save_message( char *s1, char *s2 )
{
	int x, y, w2, h, w1;
	int window_w;
	int print_x, print_y;
	char buf[36];


	strcpy( buf, "'" );
	strcat( buf, s2 );
	strcat( buf, "'" );

	VH_MeasurePropString( s1, &w1, &h, 4 );
	VH_MeasurePropString( buf, &w2, &h, 4 );

	window_w = (w1 > w2) ? w1 : w2;
	window_w += 6;
	center_watch_window( window_w, (h * 2) + 2, &x, &y );

	print_y = y + 2;
	print_x = x + (window_w - w1) / 2;
	VH_DrawPropString( s1, print_x, print_y, 4, 10 );

	print_y += h;
	print_x = x + (window_w - w2) / 2;
	VH_DrawPropString( buf, print_x, print_y, 4, 10 );

	// VW_UpdateScreen();
	VL_Present();
}

int green_message_box( char *s1, char *s2, char *s3 )
{
	int w1, w2, w3, h, sh;
	int window_w, x, y;
	IN_ControlFrame state;
	char k;

	int print_x, print_y;

	/* Find the lengths of the strings */
	VH_MeasurePropString( s1, &w1, &h, 4 );
	VH_MeasurePropString( s2, &w2, &h, 4 );
	if ( s3 )
		VH_MeasurePropString( s3, &w3, &h, 4 );
	else
		w3 = 0;

	/* Work out the window size and position and draw it */
	window_w = (w1 > w2) ? ((w1 > w3) ? w1 : w3) : w2;
	window_w += 7;
	sh = h;
	h *= (s3 ? 5 : 4);
	center_watch_window( window_w, h, &x, &y );

	/* Print the message */
	// fontcolour = 2;
	print_x = x + (window_w - w1) / 2;
	print_y = y + sh + 1;
	VH_DrawPropString( s1, print_x, print_y, 4, 10 );

	print_y += (sh * 2) - 1;
	VH_HLine( x + 3, x + window_w - 3, print_y, 10 );

	/* Print the OK prompt */
	print_y += 2;
	//fontcolour = 10;
	print_x = x + (window_w - w2) / 2;
	VH_DrawPropString( s2, print_x, print_y, 4, 2 );

	/* Print the third string ( if any ) */
	print_y += sh;
	if ( s3 )
	{
		print_x = x + (window_w - w3) / 2;
		VH_DrawPropString( s3, print_x, print_y, 4, 2 );
	}

	// VW_UpdateScreen();
	VL_Present();

	/* Wait for button1 or a key to be pressed */
	IN_ClearKeysDown();
	do
	{
		k = IN_SC_None;
		IN_PumpEvents();
		IN_ReadControls(0,  &state );
		if ( state.jump )
		{
			k = IN_SC_Y;
		}
		else if (state.pogo)
		{
			k = IN_SC_Escape;
		}
		else
		{
			k = IN_GetLastScan();
		}
	} while ( k == IN_SC_None );

	/* Wait for the button to be released */
	do
	{
		IN_PumpEvents();
		IN_ReadControls(0, &state );
	} while ( state.jump || state.pogo );
	IN_ClearKeysDown();

	US_DrawCards();

	/* Return true or false based on the user's choice */
	if ( k == IN_SC_Y )
		return 1;
	else
		return 0;
}

int USL_ConfirmComm( US_CardCommand command )
{
	int result;
	char *s1, *s2, *s3;
	int ask_user;

	if ( command == US_Comm_None )
		Quit( "USL_ConfirmComm() - empty comm" );	/* quit */

	result = 1;
	ask_user = 0;
	s3 = "ESC TO BACK OUT";
	switch ( command )
	{
	case US_Comm_EndGame:
		s1 = "REALLY END CURRENT GAME?";
		s2 = "PRESS Y TO END IT";
		if ( game_in_progress && game_unsaved )
			ask_user = 1;
		break;

	case US_Comm_Quit:
		s1 = "REALLY QUIT?";
		s2 = "PRESS Y TO QUIT";
		ask_user = 1;
		break;

	case 4:
		s1 = "YOU'RE IN A GAME";
		s2 = "PRESS Y TO LOAD GAME";
		if ( game_in_progress && game_unsaved )
			ask_user = 1;
		break;

	case US_Comm_NewEasyGame:
	case US_Comm_NewNormalGame:
	case US_Comm_NewHardGame:
		s1 = "YOU'RE IN A GAME";
		s2 = "PRESS Y FOR NEW GAME";
		if ( game_in_progress && game_unsaved )
			ask_user = 1;
		break;
	}
	// TODO: implement green_message_box
	//result = ask_user ? green_message_box( s1, s2, s3 ) : 1;
	result = 1;
	if ( result )
	{
		us_currentCommand = command;
		command_confirmed = 1;
	}
	return result;
}


#if 0
char *error_message_table[] ={
	syserrlist[EZERO], syserrlist[EINVFNC], syserrlist[ENOFILE],
	syserrlist[ENOPATH], syserrlist[EMFILE], syserrlist[EACCES],
	syserrlist[EACCES], syserrlist[EBADF], syserrlist[EBADF],
	syserrlist[ECONTR], syserrlist[ECONTR], syserrlist[ENOMEM],
	syserrlist[ENOMEM], syserrlist[EINVMEM], syserrlist[EINVENV],
	syserrlist[EINVFMT], syserrlist[EINVACC], syserrlist[EINVDAT],
	NULL, syserrlist[ENODEV], syserrlist[ECURDIR],
	syserrlist[ENOTSAM], syserrlist[ENMFILE], syserrlist[INVAL],
	syserrlist[E2BIG], syserrlist[ENOEXEC], syserrlist[EXDEV],
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, NULL, NULL,
	NULL, syserrlist[EDOM], syserrlist[ERANGE],
	syserrlist[EEXIST]
};

void error_message( int error )
{
	char buf[64];

	strcpy( buf, "Error: " );
	if ( error == 0 )
		strcat( buf, "Unknown" );
	else if ( error == 8 )
		strcat( buf, "Disk is Full" );
	else if ( error == 11 )
		strcat( buf, "File is Incomplete" );
	else
		strcat( error_message_table[error] );

	sub_653();
	green_message_box( buf, "PRESS ANY KEY", 0 );
	sub_658();
	clear_keybuf();
	wait_for_button();
	sub_652();
	sub_658();
}

void update_options_menus( void )
{
	options_menu_items[0].caption = score_box ? "SCORE BOX (ON)" : "SCORE BOX (OFF)";
	options_menu_items[1].caption = two_button_firing ? "TWO-BUTTON FIRING (ON)" : "TWO-BUTTON FIRING (OFF)";
	options_menu_items[2].caption = fix_jerky_motion ? "FIX JERKY MOTION (ON)" : "FIX JERKY MOTION (OFF)";
	options_menu_items[3].caption = svga_comp ? "SVGA COMPATIBILITY (ON)" : "SVGA COMPATIBILITY (OFF)";

	buttons_menu_items[2].state &= ~US_Is_Disabled;
	if ( two_button_firing )
		buttons_menu_items[2].state |= US_Is_Disabled;

	/* Set up the gamepad menu item */
	configure_menu_items[6].state |= US_Is_Disabled;
	if ( game_controllers[0] == CTRL_JOYSTICK1 || game_controllers[0] == CTRL_JOYSTICK2 )
		configure_menu_items[6].state &= ~US_Is_Disabled;
	configure_menu_items[6].caption = gamepad ? "USE GRAVIS GAMEPAD (ON)" : "USE GRAVIS GAMEPAD (OFF)";
}

/*   U.0926  55                                   push    bp */
int score_box_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg != MP_ENTER_MENU )
		return 0;

	score_box = !score_box;
	green_message_box( (score_box ? "Score box now on" : "Score box now off"), "Press any key", NULL );
	update_options_menus();
	return 1;
}

/*   U.095F  55                                   push    bp */
int fix_jerky_motion_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg != MP_ENTER_MENU )
		return 0;

	fix_jerky_motion = !fix_jerky_motion;
	green_message_box( (fix_jerky_motion ? "Jerky motion fix enabled" : "Jerky motion fix disabled"),
										"Press any key", NULL );
	update_options_menus();
	return 1;
}

/*   U.0998  55                                   push    bp */
int svga_comp_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg != MP_ENTER_MENU )
		return 0;

	svga_comp = !svga_comp;
	green_message_box( (svga_comp ? "SVGA compatibility now on" : "SVGA compatibility now off"),
										"Press any key", NULL );
	update_options_menus();
	return 1;
}

/*   U.09D1  55                                   push    bp */
int two_button_firing_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg != MP_ENTER_MENU )
		return 0;

	two_button_firing = !two_button_firing;
	green_message_box( (two_button_firing ? "Two-button firing now on" : "Two-button firing now off"),
										"Press any key", NULL );
	update_options_menus();
	return 1;
}

int configure_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg == MP_DRAW_MENU_AFTER )
	{
		VH_MeasurePropString( "CONTROL: ", &local1, &local2 );
		_SI = local1;
		VH_MeasurePropString( control_type_strings[ game_controllers[0] ], &local1, &local2 );
		_SI += local1;
		print_y = 132 - local_2;
		print_x = 74 + (160 - _SI) / 2;
		fontcolour = 10;
		VH_DrawPropString( "CONTROL: " );
		VH_DrawPropString( control_type_strings[ game_controllers[0] ] );
	}
	item++;
	return 0;	/* return value is irrelevant for MP_DRAW_MENU_AFTER */
}

void set_key_control( US_CardItem far *item, int which_control )
{
	long lasttime;
	int cursor;
	CONTROLLER state;
	byte k;
	int i, used;

	cursor = 0;
	lasttime = 0;
	last_scan = 0;
	fontcolour = 2;

	/* Prompt the user to press a key */
	do
	{
		if ( AZ : A53D >= lasttime )
		{	/* time_count */
			cursor = !cursor;

			/* Draw the rectangle */
			VH_Bar( item->x + 90, item->y, 40, 8, fontcolour ^ 8 );
			VH_Bar( item->x + 91, item->y + 1, 38, 6, 8 );

			/* Draw the cursor */
			if ( cursor )
				sub_659( item->x + 106, item->y, 100 );

			sub_658();
			lasttime = AZ : A53D + 35;	/* time_count */
		}

		/* A button push will cancel the key selection */
		get_controller_stats( &state );
		while ( state.button1 || status.button2 )
		{
			get_controller_status( &state );
			last_scan = KEY_ESC;
		}

		/* Disallow left shift for some reason */
		disable();
		if ( last_scan == KEY_LSHIFT )
			last_scan = 0;
		enable();

	} while ( (k = last_scan) == 0 );

	/* If they didn't cancel the process with ESC */
	if ( local1a != KEY_ESC )
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

	clear_keybuf();
}

/*   U.0BF1  55                                   push    bp */
int keyboard_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg == MP_ENTER_MENU )
	{
		game_controller[0] = CTRL_KEYBOARD;
		gamepad = 0;
		update_options_menus();
	}
	return 0;
}

int controls_menu_proc( MENU_PROC_MSG msg, US_CardItem *item )
{
	int which_control;
	int result = 0;

	which_control = (us_currentCard == movement_menu) ?
		(item - movement_menu_items) / sizeof ( US_CardItem ) + 3 :
		(item - buttons_menu_items) / sizeof ( US_CardItem );

	switch ( msg )
	{
	case MP_ENTER_MENU:
		game_controller[0] = CTRL_KEYBOARD;
		break;

	case MP_DRAW_ITEM:
		VH_Bar( 75, param3->y, 159, 8, 8 );
		draw_menu_item_icon( param3 );

		fontcolour = (item->state & US_Is_Selected) ? 10 : 2;
		print_x = item->x + 8;
		print_y = item->y + 1;
		VH_DrawPropString( item->caption );

		VH_Bar( item->x + 90, item->y, 40, 8, fontcolour ^ 8 );
		VH_Bar( item->x + 91, item->y + 1, 38, 6, 8 );

		print_x = item->x + 96;
		print_y = item->y + 1;
		VH_DrawPropString( get_key_string( *key_controls[which_control] ) );
		result = 1;
		break;

	case US_MSG_ItemEntered:
		controls_menu_proc( MP_DRAW_ITEM, item );
		set_key_control( item, which_control );
		US_DrawCards();
		result = 1;
		break;
	}

	return result;
}

void print_joystick_prompt( char *s1, char *s2 )
{
	int w, h;

	VH_MeasurePropString( s1, &w, &h );
	print_x = 74 + (160 - w) / 2;
	print_y = 116;

	VH_Bar( 75, print_y, 158, h * 2, 8 );

	fontcolour = 2;
	VH_DrawPropString( s1 );

	print_y += h;
	VH_MeasurePropString( s2, &w, &h );
	print_x = 74 + (160 - w) / 2;
	VH_DrawPropString( s2 );
}

int get_joystick_calibration_point( int which, int button, int cx, int cy, int *jx, int *jy )
{
	long lasttime;
	int cursor;

	/* Wait for the buttons to be released */
	while ( joystick_buttons_debounced( which ))
	{
		/* Let the user cancel if they want */
		if ( last_scan == KEY_ESC )
		{
			return 0;
		}
	}

	cursor = 0;
	lasttime = 0;

	/* Wait for the appropriate button to be pressed */
	while ( !(joystick_buttons_debounced( which ) & (1 << button)) )
	{

		/* Draw flashing cursor */
		if ( time_count > lasttime )
		{
			cursor = !cursor
				lasttime = time_count + 35;
			sub_659( cx, cy, 92 + cursor );
			sub_658();
		}

		/* Let the user cancel if they want */
		if ( last_scan == KEY_ESC )
			return 0;
	}

	/* Get the joystick position */
	joystick_abs_pos( which, jx, jy );
	return 1;
}

int calibrate_joystick( int which )
{
	int x, y;
	int max_x, max_y, min_x, min_y;

	footer_str[1] = footer_str[2] = "";
	footer_str[0] = "ESC to back out"
		US_DrawCards();

	x = 134;
	y = 67;

	/* Draw joystick diagram */
	VH_DrawBitmap( x, y, 102 );
	print_joystick_prompt( "Move Joystick to upper left", "and press button #1" );

	/* Draw arrow */
	sub_659( x + 24, y + 8, 98 );
	/* Draw button 1 & 2 */
	sub_659( x + 8, y + 8, 93 );
	sub_659( x + 8, y + 24, 92 );

	sub_658();

	if ( !get_joystick_calibration_point( which, 0, x + 8, y + 8, &min_x, &min_y ) )
		return 0;

	print_joystick_prompt( "Move Joystick to lower right", "and press button #2" );
	/* Erase the previous arrow */
	sub_659( x + 24, y + 8, 67 );
	/* Draw arrow */
	sub_659( x + 40, y + 24, 99 );
	/* Draw buttons 1 & 2 */
	sub_659( x + 8, y + 8, 92 );
	sub_659( x + 8, y + 24, 93 );
	sub_658();

	if ( !get_joystick_calibration_point( which, 1, x + 8, y + 24, &max_x, &max_y ) )
		return 0;

	/* Wait for the buttons to be released */
	while ( joystick_buttons_debounced( which ) )
		;

	/* And set up the joystick */
	if ( min_x != max_x && min_y != max_y )
	{
		setup_joystick( which, min_x, local3, miin_y, local4 );
		return 1;
	}
	else
	{

		return 0;
	}
}

int joystick1_menu_proc( MENU_PROC_MSG msg, US_CardItem far *param )
{
	if ( msg == MP_ENTER_MENU )
	{
		/* Set up joystick 1 */
		if ( calibrate_joystick( 0 ) )
		{
			game_controller[0] = CTRL_JOYSTICK1;
			green_message_box( "USING JOYSTICK #1", "PRESS ANY KEY", NULL );
			update_options_menus();
		}
		return 1;
	}
	else
	{

		return 0;
	}
}

/*   U.10B1 ,55                                   push    bp                      ; PARAMETER_2 */
int joystick2_menu_proc( MENU_PROC_MSG msg, US_CardItem far *param )
{
	if ( msg == MP_ENTER_MENU )
	{
		/* Set up joystick 2 */
		if ( calibrate_joystick( 1 ) )
		{
			game_controller[0] = CTRL_JOYSTICK2;
			green_message_box( "USING JOYSTICK #2", "PRESS ANY KEY", NULL );
			update_options_menus();
		}
		return 1;
	}
	else
	{

		return 0;
	}
}


char *gamepad_strings[] ={
	"Red", "Blue", "Yellow", "Green",
	"Jump", "Pogo", "Fire", "Status"
};

void print_gamepad_prompt( char *s1, char *s2 )
{
	char *s;
	int w, h;

	/* Erase the screen and print the prompt */
	VH_Bar( 75, 64, 158, 68, 8 );
	print_x = 82;
	print_y = 64;
	VH_DrawPropString( "Make sure that the button" );
	print_x = 82;
	print_y = 72;
	VH_DrawPropString( "switch is set for 4 buttons" );

	/* Print the current button assignments */
	for ( i = 0; i < 4; i++ )
	{
		/* Print the button color */
		print_x = 82;
		print_y = 88 + i * 8;
		VH_DrawPropString( gamepad_strings[i] );
		VH_DrawPropString( ":" );

		/* Print the button assignment */
		s = "?";
		for ( j = 0; j < param4; j++ )
		{

			if ( gamepad_buttons[j] == i )
				s = gamepad_strings[j + 4];
		}
		print_x = 130;
		VH_DrawPropString( s );
	}

	/* Print the given strings */
	VH_MeasurePropString( s1, &w, &h );
	print_x = w;
	VH_MeasurePropString( s2, &w, &h );
	print_x = 74 + (160 - print_x - w) / 2;
	print_y = 124;
	VH_DrawPropString( s1 );
	VH_DrawPropString( s2 );
	sub_658();
}

int choose_gamepad_button( int button, char *button_string )
{
	int b, i, t;

	print_gamepad_prompt( "PRESS BUTTON FOR ", button_string );

	while ( 1 )
	{
		/* Make sure the user presses only one button */
		do
		{
			/* Wait for a button to be pressed */
			do
			{
				/* Let the user cancel if they want */
				if ( last_scan == KEY_ESC )
				{
					keyboard[KEY_ESC] = 0;
					if ( last_scan == KEY_ESC )
						last_scan = 0;
					return 0;
				}
				b = joystick_buttons_debounced( 2 );	/* both joysticks */
			} while ( b == 0 );

			/* Work out which buttons were pressed */
			t = 0;
			for ( i = 0; i < 4; i++ )
			{
				if ( b & (1 << i) )
				{
					t += i + 1;
				}
			}
		} while ( t == 0 || t >= 5 );
		t--;

		/* Don't let them choose a button that has already been used */
		for ( i = 0; i < button; i++ )
		{
			if ( gamepad_buttons[i] == t )
				break;
		}

		/* If they chose a button which was not already used */
		if ( i == button )
		{

			gamepad_buttons[button] = t;
			return 1;
		}
	}
}

int gamepad_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{
	if ( msg == MP_ENTER_MENU )
	{
		if ( gamepad )
		{
			gamepad = 0;
		}
		else
		{
			footer_str[1] = footer_str[2] = "";
			footer_str[0] = "ESC to back out";
			US_DrawCards();

			fontcolour = 2;
			print_x = 82;
			print_y = 82;
			fontcolour = 2;
			clear_keybuf();

			if ( choose_gamepad_button( 0, "JUMP" ) &&
					choose_gamepad_button( 1, "POGO" ) &&
					choose_gamepad_button( 2, "FIRE" ) &&
					choose_gamepad_button( 3, "STATUS" ) )
			{

				gamepad = 1;
				print_gamepad_prompt( "PRESS ANY KEY", "" );
				wait_for_button();
			}
			else
			{
				gamepad = 0;
			}
		}
		update_options_menus();
		return 1;
	}
	else
	{

		return 0;
	}


	;
	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	;
	SUBROUTINE
		;
	;
	Called from :   U.15FC, 160F
		;

	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	/*   U.1390                       sub_609         proc    near */
	void draw_savegame_item_border( US_CardItem far * item )
	{

		int c;

		/* Set the item's position */
		item->y = us_currentCard->y + 60;
		item->y += ((item - savegame_menu_items) / sizeof ( US_CardItem )) * 11;

		/* Choose an appropriate color */
		fontcolour = (item->state & US_Is_Selected) ? 2 : 10;
		c = fontcolour ^ 8;

		/* Draw the rectangle */
		VH_HLine( item->x, item->x + 148, item->y, c );
		VH_HLine( item->x, item->x + 148, item->y + 9, c );
		VH_VLine( item->y, item->y + 9, item->x, _SI );
		VH_VLine( item->y, item->y + 9, item->x + 148, c );
	}



	;
	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	;
	SUBROUTINE
		;
	;
	Called from :   U.16C0
		;

	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	/*   U.146D                       sub_610         proc    near */
	void load_savegame_item( US_CardItem far * item )
	{
		int i;
		SAVEFILE_ENTRY *e;
		char *file;
		int handle;
		int error;

		if ( USL_ConfirmComm( 4 ) )
		{
			i = (item - savegame_menu_items) / sizeof (US_CardItem);
			e = savefiles[i];
			load_save_message( "Loading", savefiles[i].name );

			_SI = 0;
			file = get_savefile_name( i );
			if ( (handle = open( file, O_RDONLY | O_BINARY )) != -1 )
			{
				if ( read( handle, e, sizeof ( SAVEFILE_ENTRY ) ) != sizeof ( SAVEFILE_ENTRY ) )
				{
					if ( !p_load_game || !(*p_load_game)( handle) )
						error_message( error = errno );
				}
				else
				{
					error_message( error = errno );
				}
				close( handle );
			}
			else
			{
				error_message( error = errno );
			}

			if ( error )
			{	/* is this condition right? */
AZ:
				load_game_error = 1;
AZ:
				us_currentCommand = 0;	/* ? last command ? */
AZ:
				command_confirmed = 0;	/* ? command success state ? */
			}
			else
			{
AZ:
				A53B = 0;
			}
			e->used = 1;

			if ( AZ : A53B )
				paused = 1;
			US_DrawCards();
		}
	}



	;
	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	;
	SUBROUTINE
		;
	;
	Called from :   U.190F
		;

	== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
	/*   U.1581                       sub_611         proc    near */
	int load_game_menu_proc( MENU_PROC_MSG *msg, US_CardItem far * item )
	{
		int result, i;

		result = 0;
		switch ( msg )
		{
		case MP_ENTER_MENU:
			if ( getenv( "UID" ) )
				get_savefiles();

			for ( i = 0; i < 6; i++ )
			{
				if ( savefiles[i].name )
					savegame_menu_items[i].state &= ~US_Is_Disabled;
				else
					savegame_menu_items[i].state |= US_Is_Disabled;
			}
		}
		break;

	case MP_DRAW_ITEM_ICON:
		draw_savegame_item_border( item );
		result = 1;
		break;

	case MP_DRAW_ITEM:
		draw_savegame_item_border( item );

		/* Draw the caption */
		VH_Bar( item->x + 1, item->y + 2, 146, 7, 8 );
		i = (item - savegame_menu_items) / sizof( US_CardItem );
		if ( savefiles[i].used )
			print_x = item->x + 2;

		else
			print_x = item->x + 60;
		print_y = item->y + 2;
		VH_DrawPropString( savefiles[i].used ? savefiles[i].name : "Empty" );

		result = 1;
		break;

	case US_MSG_ItemEntered:
		load_savegame_item( item );
		result = 1;
		break;
	}
	return result;
}

void save_savegame_item( US_CardItem far *item )
{
	int i, n;
	int handle;
	SAVEFILE_ENTRY *e;
	char *fname;
	int error

	footer_str[2] = "Type name";
	footer_str[1] = "Enter accepts";
	US_DrawCards();

	i = (item - savegame_menu_items) / sizeof ( US_CardItem );
	e = savefiles[i];

	/* Prompt the user to enter a name */
	fontcolour = 2;
	VH_Bar( item->x + 1, item->y + 2, 146, 7, 8 );
	e->unknown1 = 0xA537;
	n = window_input( item->x + 2, item->y + 2, e->name, (e->used ? e->name : NULL), 1, 32, 138 );

	/* If they entered no name, give a default */
	if ( strlen( e->name ) == 0 )
		strcpy( e->name, "Untitled" );

	/* If the input was not canceled */
	if ( n != 0 )
	{
		load_save_message( "Saving", e->name );

		/* Save the file */
		fname = get_savefile_name( i );
		error = 0;
		handle = open( fname, O_BINARY | O_CREAT | O_WRONLY, S_IFREG | S_IREAD | S_IWRITE );
		if ( handle != -1 )
		{
			/* Save the game */
			if ( write( handle, e, sizeof ( SAVEFILE_ENTRY ) ) == sizeof ( SAVEFILE_ENTRY ) )
			{
				if ( !p_save_game || !(n = (*p_save_game)( handle )) )
					error_message( error = errno );
			}
			else
			{
				error = (errno == 2) ? 8 : errno;
				error_message( error );
			}
			close( handle );
		}
		else
		{
			error = (errno == 2) ? 8 : errno;
			errormessage( error );
		}

		/* Delete the file if an error occurred */
		if ( error )
		{
			unlink( fname );
			n = 0;
		}
	}

	if ( e->used == 0 )
		e->used = n;

	if ( n )
		game_unsaved = 0;

	sub_621();
}

int save_game_menu_proc( MENU_PROC_MSG msg, US_CardItem *item )
{
	int i;

	switch ( msg )
	{
	case MP_ENTER_MENU:
		if ( getenv( "UID" ) )
			get_savefiles();

		/* Enable all the entries */
		for ( i = 0; i < 6; i++ )
		{

			savegame_menu_items[i].state &= ~US_Is_Disabled;
		}
		return 0;
	case US_MSG_ItemEntered:
		save_savegame_item( item );
		return 1;
	default:
		return load_game_menu_proc( msg, item );
	}
}

void show_paddlewar_score( int keen_score, int comp_score )
{

	fontcolour = 2;
	print_y = 52;
	window_print_y = 52;

	print_x = 80;
	VH_Bar( print_x, print_y, 42, 6, 8 );
	VH_DrawPropString( "KEEN:" );

	window_print_x = print_x;
	window_print_unsigned( keen_score );

	print_x = 182;
	VH_Bar( print_x, print_y, 50, 6, 8 );
	VH_DrawPropString( "COMP:" );

	window_print_x = print_x;
	window_print_unsigned( comp_score );
}

void paddlewar( void )
{
	int ball_visible, new_round, y_bounce, done, keen_won_last, comp_move_counter;
	int ball_y, keen_x, comp_x, bounce_point, ball_real_x, ball_real_y;
	int old_keen_x, old_comp_x, old_ball_x, old_ball_y, keen_score, comp_score;
	int speedup_delay, ball_x_speed;
	long start_delay, lasttime, timediff;
	CONTROLLER_STATUS state;
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
	lasttime = time_count;

	do
	{
		while ( (timediff = time_count - lasttime) == 0 )
			;
		lasttime = time_count;
		if ( timediff > 4 )
		{
			timediff = 4;
		}

		while ( timediff-- && !done && last_scan != KEY_ESC )
		{
			get_controller_status( &state );
			if ( state.dx < 0 || keyboard[KEY_LEFT] )
			{
				if ( keen_x > 78 )
				{
					keen_x -= 2;
				}
				else if ( state.dx > 0 || keyboard[KEY_RIGHT] )
				{
					if ( keen_x < 219 )
						keen_x += 2;
				}
				if ( new_round )
				{
					ball_visible = 0;
					start_delay = 70;
					speedup_delay = 10;
					new_round = 0;

					/* Erase the ball */
					VH_Bar( old_ball_x, old_ball_y, 5, 5, 8 );
				}
				if ( ball_visible && (comp_move_counter++ % 3) != 0 )
				{
					ball_x = ball_real_x / 4;
					if ( ball_x & 1 == 0 )
						ball_x += next_random_number() & 1;

					if ( comp_x + 6 < ball_x && comp_x < 219 )
					{
						comp_x++;
					}
					else if ( comp_x + 6 > ball_x && comp_x > 78 )
					{
						comp_x--;
					}
				}
				if ( ball_visible == 0  )
				{
					if ( --start_delay == 0 )
					{
						ball_visible = 1;
						ball_x_speed = 1 - (next_random_number() % 3);
						ball_y_speed = 3;
						if ( keen_won_last )
							ball_y_speed = -ball_y_speed;
						ball_real_x = 612;
						ball_real_y = 396;
					}
				}
				if ( ball_visible != 0 )
				{
					if ( (ball_real_x + ball_x_speed) / 4 > 228 || (ball_real_x + ball_x_speed) / 4 < 78 )
						sub_544( 48 );
					ball_x_speed = -ball_x_speed;
				}

				ball_real_x += ball_x_speed;

				if ( (ball_real_y + ball_y_speed) / 4 > 137 )
				{
					new_round = 1;
					keen_won_last = 0;
					comp_score++;
					sub_544( 50 );
					show_paddlewar_score( keen_score, comp_score );
					if ( comp_score == 21 )
					{
						green_message_box( "You lost!", "Press any key", NULL );
						done = 1;
						continue;
					}
				}
				else if ( (ball_real_y + ball_y_speed) / 4 < 62 )
				{
					new_round = 1;
					keen_won_last = 1;
					keen_score++;
					sub_544( 51 );	/* play_sound */
					show_paddlewar_score( keen_score, comp_score );
					if ( keen_score == 21 )
					{
						green_message_box( "You won!", "Press any key" );
						done = 1;
						continue;
					}
				}

				ball_real_y += ball_y_speed;
				ball_x = ball_real_x / 4;
				ball_y = ball_real_y / 4;

				if ( !new_round )
				{
					if ( ball_y_speed < 0 && ball_y >= 66 && ball_y < 69 && (comp_x - 5) <= ball_x && (comp_x + 11) > ball_x )
					{
						bounce_point = comp_x;
						y_bounce = 1;
						sub_544( 49 );
					}
					else if ( ball_y_speed > 0 && ball_y >= 132 && ball_y < 135 && (keen_x - 5) <= ball_x && (keen_x + 11) < ball_x )
					{
						if ( ball_y_speed / 4 < 3 )
						{
							speedup_delay--;
							if ( speedup_delay == 0 )
							{
								ball_y_speed++;
								speedup_delay = 10;
							}
						}
						bounce_point = keen_x;
						y_bounce = ball_visible;
						sub_544( 47 );
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
		}

		if ( ball_visible )
		{
			VH_Bar( old_ball_x, old_ball_y, 5, 5, 8 );
			old_ball_x = ball_x;
			old_ball_y = ball_y;

			sub_665( ball_x, ball_y, (ball_x & 1) ? 126 : 125 );	/* draw_sprite? */
		}

		VH_Bar( old_comp_x - 3, 66, 16, 3, 8 );
		old_comp_x = comp_x;
		sub_665( comp_x, 66, 124 );

		VH_Bar( old_keen_x - 3, 135, 16, 3, 8 );
		old_keen_x = keen_x;
		sub_665( keen_x, 135, 124 );

		sub_658();

	} while ( last_scan != KEY_ESC && !done );

	clear_keybuf();
}

int paddlewar_menu_proc( MENU_PROC_MSG msg, US_CardItem far *item )
{

	if ( msg != MP_ENTER_MENU )
		return 0;

	/* Draw the watch */
	VH_DrawBitmap( 0, 0, 103 );

	/* Draw the PaddleWar title */
	VH_DrawBitmap( 130, 48, 100 );

	/* Draw a line above the playing area */
	VH_HLine( 77, 231, 60, 10 );

	/* Draw a line below the playing area */
	VH_HLine( 77, 231, 143, 10 );

	/* Play the game */
	paddlewar();
	return 1;
}
#endif

void USL_EnableMenuItems( US_Card *card )
{
	US_CardItem *item;

	if ( card->items )
	{
		item = card->items;
		while ( item->type != US_ITEM_None )
		{
			item->state &= ~US_IS_Disabled;
			item->state &= ~US_IS_Selected;
			item->state &= ~US_IS_Checked;

			if ( item->subMenu  )
				USL_EnableMenuItems( item->subMenu );
			item++;
		}
	}
}

int USL_GetCheckedItem( US_Card *card )
{
	US_CardItem *item;
	int index;

	item = card->items;
	index = 0;
	while ( item->type != US_ITEM_None )
	{
		if ( item->state & US_IS_Checked )
		{

			return index;
		}
		item++;
		index++;
	}
	return -1;
}

void USL_SelectCardItem( US_Card *card, int index, int redraw )
{
	US_CardItem *item;

	if ( card->selectedItem != index )
	{
		item = &(card->items[ card->selectedItem ]);
		item->state &= ~US_IS_Selected;
		if ( redraw )
			USL_DrawCardItem( item );
	}
	card->selectedItem = index;
	item = &(card->items[ card->selectedItem ]);
	item->state |= US_IS_Selected;

	if ( redraw )
		USL_DrawCardItem( item );
}

void US_SelectItem(US_Card *card, int itemIndex, bool redraw)
{
	US_CardItem *item;

	// If the item is not already selected
	if (card->selectedItem != itemIndex)
	{
		// Unselect the old item
		US_CardItem *oldItem = &(card->items[card->selectedItem]);
		oldItem->state &= ~US_IS_Selected;
		if (redraw)
			USL_DrawCardItem(oldItem);
	}

	// Select the new item.
	card->selectedItem = itemIndex;
	item = &(card->items[card->selectedItem]);
	item->state |= US_IS_Selected;

	if (redraw)
		USL_DrawCardItem(item);
}

void USL_CheckCardItem(US_Card *card, int itemIndex, bool redraw)
{
	int i;
	US_CardItem *cardItem;

	US_SelectItem(card, itemIndex, redraw);
	cardItem = card->items;

	i = 0;

	while (cardItem->type != US_ITEM_None)
	{
		if (cardItem->type == US_ITEM_Radio)
		{
			if (i == itemIndex)
			{
				// Check the item that needs to be checked
				cardItem->state |= US_IS_Checked;
				if (redraw)
				{
					USL_DrawCardItem(cardItem);
				}
			}
			else if (cardItem->state & US_IS_Checked)
			{
				// Uncheck the previously checked item
				cardItem->state &= ~US_IS_Checked;
				if (redraw)
				{
					USL_DrawCardItem(cardItem);
				}
			}
		}
		cardItem++;
		i++;
	}
}


void US_SelectNextItem()
{
	if (us_currentCard->items[ us_currentCard->selectedItem + 1 ].type != US_ITEM_None )
	{

		US_SelectItem(us_currentCard, us_currentCard->selectedItem + 1, true);
	}
}

void US_SelectPrevItem()
{
	if (us_currentCard->selectedItem > 0)
	{

		US_SelectItem(us_currentCard, us_currentCard->selectedItem - 1, true);
	}
}

void USL_SetMenuFooter( void )
{
	footer_str[2] = "Arrows move";
	footer_str[1] = "Enter selects";
	footer_str[0] = (us_cardStackIndex != 0) ? "ESC to back out" : "ESC to quit";
	USL_SelectCardItem( us_currentCard, us_currentCard->selectedItem, 0 );
	US_DrawCards();
}

void USL_DownLevel(US_Card *card)
{
	if (!card)
		Quit("USL_DownLevel() - nil card");

	USL_PushCard(card);

	if (card->msgCallback)
	{
		if (card->msgCallback(US_MSG_CardEntered, 0))
		{

			USL_PopCard();
		}
	}

	//TODO: Handle Joystick/Kbd footer stuff
	USL_SetMenuFooter();
}

void USL_UpLevel()
{
	if (!us_cardStackIndex)
	{
		//TODO: Quit?
		return;
	}

	if (us_currentCard->items)
	{

		us_currentCard->items[us_currentCard->selectedItem].state &= ~US_IS_Selected;
	}

	USL_PopCard();

	//TODO: Footer stuff
	USL_SetMenuFooter();
}

void US_SelectCurrentItem()
{
	US_CardItem *item;

	item = &(us_currentCard->items[ us_currentCard->selectedItem ]);

	// Play disallowed sound if menu item is disabled
	if ( item->state & US_IS_Disabled )
	{
		SD_PlaySound(SOUND_NEEDKEYCARD);
		return;
	}

	// Handle the various Card item types
	switch ( item->type )
	{
	case US_ITEM_Normal:
		if ( us_currentCard->msgCallback )
		{
			if ( us_currentCard->msgCallback( US_MSG_ItemEntered, item ) )
				break;
		}
		USL_ConfirmComm( item->command );
		return;
	case US_ITEM_Radio:
		USL_CheckCardItem( us_currentCard, us_currentCard->selectedItem, 1 );
		return;
	case US_ITEM_Submenu:
		USL_DownLevel( item->subMenu );
		break;
	}
}

void USL_UpdateCards( void )
{
	int i;

	/* SFX and Music menus */
	i = SoundMode;

	if ( (i == sdm_AdLib) && quiet_sfx )
		i++;

	USL_CheckCardItem( &ck_us_soundMenu, i, 0 );
	USL_CheckCardItem( &ck_us_musicMenu, MusicMode, 0 );

	if ( !AdLibPresent )
	{
		/* Disable adlib & sb menu items */
		ck_us_soundMenuItems[2].state |= US_IS_Disabled;
		ck_us_soundMenuItems[3].state |= US_IS_Disabled;
		ck_us_musicMenuItems[1].state |= US_IS_Disabled;
	}
#if 0
	/* Joystick and gamepad menu items*/
	if ( !joystick_present[0] )
		configure_menu_items[4].state |= US_IS_Disabled;
	if ( !joystick_present[1] )
		configure_menu_items[5].state |= US_IS_Disabled;
	if ( !joystick_present[0] && !joystick_present[1] )
		configure_menu_items[6].state |= US_IS_Disabled;

#endif
	ck_us_mainMenuItems[4].caption = (game_in_progress) ? "RETURN TO GAME" : "RETURN TO DEMO";
	/* Save game and end game items */
	if ( !game_in_progress )
	{
		ck_us_mainMenuItems[2].state |= US_IS_Disabled;
		ck_us_mainMenuItems[5].state |= US_IS_Disabled;
	}

	/* Hilite 'return to game' or 'new game' option */
	ck_us_mainMenu.selectedItem = (game_in_progress) ? 4 : 0;

	/* Update the options menu */
	//update_options_menus();
}

void USL_BeginCards()
{

	// NOTE: I'm caching more stuff here than is necessary

	// Cache all bitmaps 
	for (int i = ca_gfxInfoE.offBitmaps; i < (ca_gfxInfoE.offBitmaps + ca_gfxInfoE.numBitmaps); ++i)
	{
		CA_CacheGrChunk(i);
	}

	// Cache the font
	CA_CacheGrChunk(4);

	// Cache the wristwatch screen masked bitmap
	CA_CacheGrChunk(ca_gfxInfoE.offMasked);

	// Cache the tile8's
	CA_CacheGrChunk(ca_gfxInfoE.offTiles8);
	CA_CacheGrChunk(ca_gfxInfoE.offTiles8m);

	// Cache the first few sprites for paddlewar
	for (int i = ca_gfxInfoE.offSprites; i < (ca_gfxInfoE.offSprites + 10); ++i)
	{
		CA_CacheGrChunk(i);
	}

	CA_LoadAllSounds();

	// fontnumber = 1;
	// US_SetPrintFunctions(VW_MeasurePropString, VWB_DrawPropString);
	//fontcolour = 15;

	// VW_ClearVideo(3);
	// RF_Reset();

	us_currentCommand = US_Comm_None;

	USL_EnableMenuItems(&ck_us_mainMenu);

	USL_UpdateCards();

	US_SetupCards(&ck_us_mainMenu);

	USL_SetMenuFooter();

	if (game_in_progress)
	{
		game_unsaved = 1;
	}

	IN_ClearKeysDown();
}

void USL_HandleComm( int command )
{
	switch ( command )
	{
	case US_Comm_EndGame:	/* end game */
		load_game_error = 1;
		return;

	case US_Comm_Quit:	/* quit */
		quit_to_dos = 1;
		return;

	case US_Comm_NewEasyGame:	/* easy game */
		ck_startingDifficulty = D_Easy;
		return;

	case US_Comm_NewNormalGame:	/* normal game */
		ck_startingDifficulty = D_Normal;
		return;

	case US_Comm_NewHardGame:	/* hard game */
		ck_startingDifficulty = D_Hard;
		return;

	case US_Comm_ReturnToGame:
	case 4:	/* return to game, ? */
		return;

	default:
		Quit( "USL_HandleComm() - unknown" );	/* quit */
	}
}

void USL_SetSoundAndMusic()
{
	int i;

	i = USL_GetCheckedItem( &ck_us_soundMenu );
	// Quiet AdLib sound effects
	if ( i == 3 )
	{
		quiet_sfx = true;
		i--;
	}
	else
	{
		quiet_sfx = false;
	}
	if ( i != (int)SoundMode )
		SD_SetSoundMode( (SDMode)i );
	i = USL_GetCheckedItem( &ck_us_musicMenu );
	if ( i != (int)MusicMode )
		SD_SetMusicMode( (SMMode)i );
}

void USL_EndCards()
{

	USL_SetSoundAndMusic();

	if (us_currentCommand != US_Comm_None)
	{
		USL_HandleComm(us_currentCommand);
	}

	fontnumber = 0;
	fontcolour = 15;

	if (ck_startingDifficulty && p_exit_menu)
		p_exit_menu();

	if (quit_to_dos)
	{
		US_CenterWindow(0x14, 3);
		fontcolour = 3;
		US_PrintCentered("Quitting...");
		fontcolour = 15;
		//VW_UpdateScreen();
		Quit(NULL);
	}

	IN_ClearKeysDown();
	// SD_WaitSoundDone();
	// VW_ClearVideo(3); // Draw Cyan)
	// CA_DownLevel();
	CA_LoadAllSounds();
}

// What is this function for?
void USL_EnterCurrentItem()
{
	US_CardItem *item = &us_currentCard->items[us_currentCard->selectedItem];

	if ( item->state & US_IS_Disabled )
	{
		SD_PlaySound(SOUND_NEEDKEYCARD);
		return;
	}

	switch (item->type)
	{
	case US_ITEM_Normal:
		if (us_currentCard->msgCallback )
		{
			if (us_currentCard->msgCallback(US_MSG_ItemEntered, item))
			{
				break;
			}
		}

		USL_ConfirmComm( item->command );
		return;
	case US_ITEM_Radio:
		USL_CheckCardItem(us_currentCard, us_currentCard->selectedItem, 1);
		return;
	case US_ITEM_Submenu:
		USL_PushCard( item->subMenu );
		break;
	}
}

void US_RunCards()
{
	int controller_dy;
	unsigned long cursor_time;
	US_CardItem *item;
	//ControlInfo status;
	int action_taken;
	int index;

	bool cursor = false;

	/* clear all keys except for function keys */
	if (IN_GetLastScan() < IN_SC_F1 || IN_GetLastScan() > IN_SC_F10)
		IN_ClearKeysDown();

	// load all menu stuff and draw main menu
	USL_BeginCards();
	US_DrawCards();

	controller_dy = 0;
	command_confirmed = 0;
	cursor = 1;
	action_taken = 1;

	while (!command_confirmed)
	{
		US_CardItem *item = &(us_currentCard->items[us_currentCard->selectedItem]);

		IN_ScanCode lastScan = IN_GetLastScan();

		/* Draw the icon for the current item highlighted or not */
		if ( action_taken )
		{
			cursor_time = CK_GetNumTotalTics() + 35; // 1/2 second flashes
			action_taken = false;
		}

		// draw icon with cursor blink
		if ( CK_GetNumTotalTics() >= cursor_time )
		{
			cursor = !cursor;
			action_taken = true;

			if ( !cursor)
				item->state &= ~US_IS_Selected;

			USL_DrawCardItemIcon( item );
			item->state |= US_IS_Selected;
		}

		//VW_UpdateScreen();

		if (lastScan != IN_SC_None)
		{
			switch (lastScan)
			{
			case IN_SC_UpArrow:
				US_SelectPrevItem();
				action_taken = 1;
				break;
			case IN_SC_DownArrow:
				US_SelectNextItem();
				action_taken = 1;
				break;
			case IN_SC_Enter:
				US_SelectCurrentItem();
				action_taken = 1;
				break;
			case IN_SC_Escape:
				USL_UpLevel();
				action_taken = 1;
				break;
			case IN_SC_F1:
				HelpScreens();
				US_DrawCards();
				action_taken = 1;
				break;
			default:
				for (int i = 0; us_currentCard->items[i].type != US_ITEM_None; ++i)
				{
					if (lastScan == us_currentCard->items[i].shortcutKey)
					{
						US_SelectItem(us_currentCard, i, true);
						action_taken = 1;
						break;
					}
				}
			}
		}
		else
		{
			//TODO: Add mouse/joystick support
		}
		IN_PumpEvents();
		CK_SetTicsPerFrame();
		VL_Present();
	}

	USL_EndCards();
}
