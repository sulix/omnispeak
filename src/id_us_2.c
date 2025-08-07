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
 * on the fly.  Here, we are just calling VHB_DrawPropString and VH_MeasurePropString
 * directly.
 */

#include <stdbool.h>
// Need this for NULL
#include <stddef.h>
// For strcpy, strcat
#include <string.h>

#include "id_heads.h"
#include "id_in.h"
#include "id_sd.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"
#include "ck_def.h"
#include "ck_text.h"

void USL_DrawCardItemIcon(US_CardItem *item);
void CK_US_SetKeyBinding(US_CardItem *item, int which_control);
void CK_US_SetJoyBinding(US_CardItem *item, IN_JoyConfItem which_control);
#include "ck_us_2.c"

// Card stack can have at most 7 cards
#define US_MAX_CARDSTACK 7

// Menu motion repeat rate when using a controller (lower value = faster)
#define US_MENU_MOTION_SPEED 40

extern CK_Difficulty ck_startingDifficulty;
int game_unsaved, game_in_progress, quit_to_dos, load_game_error;

// The last selected command.
US_CardCommand us_currentCommand = US_Comm_None;
bool command_confirmed;

static int us_cardStackIndex;
US_Card *us_currentCard;
US_Card *us_cardStack[US_MAX_CARDSTACK];

US_CardItem new_game_menu_items[] = {
	{US_ITEM_Normal, 0, IN_SC_E, "BEGIN EASY GAME", US_Comm_NewEasyGame, NULL, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_N, "BEGIN NORMAL GAME", US_Comm_NewNormalGame, NULL, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_H, "BEGIN HARD GAME", US_Comm_NewHardGame, NULL, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, NULL, US_Comm_None, NULL, 0, 0},
};

US_Card new_game_menu = {8, 0, CK_CHUNKID(PIC_NEWGAMECARD), 0, new_game_menu_items, NULL, 0, 0, 0};

US_CardItem main_menu_items[] = {
	{US_ITEM_Submenu, 0, IN_SC_N, "NEW GAME", US_Comm_None, &new_game_menu, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_L, "LOAD GAME", US_Comm_None, /*load_game_menu*/ NULL, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_S, "SAVE GAME", US_Comm_None, /*save_game_menu*/ NULL, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_C, "CONFIGURE", US_Comm_None, /*&configure_menu*/ NULL, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_R, NULL, US_Comm_ReturnToGame, NULL, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_E, "END GAME", US_Comm_EndGame, NULL, 0, 0},
	{US_ITEM_Submenu, 0, IN_SC_P, "PADDLE WAR", US_Comm_None, /*paddlewar_menu*/ NULL, 0, 0},
	{US_ITEM_Normal, 0, IN_SC_Q, "QUIT", US_Comm_Quit, NULL, 0, 0},
	{US_ITEM_None, 0, IN_SC_None, NULL, US_Comm_None, NULL, 0, 0}};

//AS:00A2
US_Card main_menu = {32, 4, CK_CHUNKID(PIC_MENUCARD), 0, main_menu_items, NULL, 0, 0, 0};

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

	VHB_DrawTile8(item->x, item->y, gfxChunk);
}

void USL_DrawCardItem(US_CardItem *item)
{
	// Send a US_MSG_DrawItemIcon message to the card.
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_DrawItem, item))
		return;

	// Gray out the area underneath the text
	VHB_Bar(75, item->y, 159, 8, 8);

	USL_DrawCardItemIcon(item);

	if (!(item->state & US_IS_Selected) || (item->state & US_IS_Disabled))
		US_SetPrintColour(10);
	else
		US_SetPrintColour(2);

	VHB_DrawPropString(item->caption, item->x + 8, item->y + 1, 1, US_GetPrintColour());

	US_SetPrintColour(15);
}

const char *footer_str[3];

void USL_DrawMenuFooter(void)
{
	uint16_t w, h;

	US_SetPrintColour(10);

	/* "Arrows move" */
	VHB_DrawPropString(footer_str[2], 78, 135, 1, US_GetPrintColour());

	/* "Enter selects" */
	VH_MeasurePropString(footer_str[1], &w, &h, 1);
	VHB_DrawPropString(footer_str[1], 230 - w, 135, 1, US_GetPrintColour());

	/* "Esc to quit/back out" */
	VH_MeasurePropString(footer_str[0], &w, &h, 1);
	VHB_DrawPropString(footer_str[0], 74 + ((160 - w) / 2), 135 + h + 1, 1, US_GetPrintColour());

	US_SetPrintColour(0);

	VHB_HLine(77, 231, 133, 10);
}

void USL_DrawCard()
{
	// Run the custom draw procedure if it exists
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_Draw, 0))
		return;

	// Draw the header (if any))
	if (us_currentCard->gfxChunk)
	{
		VHB_HLine(77, 231, 55, 10);
		VHB_DrawBitmap(80, 48, CK_LookupChunk(us_currentCard->gfxChunk));
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
		VHB_DrawBitmap(0, 0, CK_CHUNKNUM(PIC_WRISTWATCH));
		USL_DrawCard();
	}

	VL_SetScrollCoords(0, 0);
	VH_UpdateScreen();
}

void USL_DialogSetup(uint16_t w, uint16_t h, uint16_t *x, uint16_t *y)
{
	VHB_DrawMaskedBitmap(74, 48, CK_CHUNKNUM(MPIC_WRISTWATCHSCREEN));

	/* Calculate the position */
	*x = 74 + (160 - w) / 2;
	*y = 48 + (102 - h) / 2;

	/* Fill the background */
	VHB_Bar(*x, *y, w + 1, h + 1, 8);

	/* Draw the border */
	VHB_HLine(*x - 1, *x + w + 1, *y - 1, 10);
	VHB_HLine(*x - 1, *x + w + 1, *y + h + 1, 10);
	VHB_VLine(*y - 1, *y + h + 1, *x - 1, 10);
	VHB_VLine(*y - 1, *y + h + 1, *x + w + 1, 10);
}

void USL_LoadSaveMessage(const char *s1, const char *s2)
{
	uint16_t x, y, w2, h, w1;
	uint16_t window_w;
	uint16_t print_x, print_y;
	char buf[36];

	snprintf(buf, sizeof(buf), "'%s'", s2);

	VH_MeasurePropString(s1, &w1, &h, 1);
	VH_MeasurePropString(buf, &w2, &h, 1);

	window_w = (w1 > w2) ? w1 : w2;
	window_w += 6;
	USL_DialogSetup(window_w, (h * 2) + 2, &x, &y);

	print_y = y + 2;
	print_x = x + (window_w - w1) / 2;
	VHB_DrawPropString(s1, print_x, print_y, 1, US_GetPrintColour());

	print_y += h;
	print_x = x + (window_w - w2) / 2;
	VHB_DrawPropString(buf, print_x, print_y, 1, US_GetPrintColour());

	VH_UpdateScreen();
}

int USL_CtlDialog(const char *s1, const char *s2, const char *s3)
{
	uint16_t w1, w2, w3, h, sh;
	uint16_t window_w, x, y;
	IN_ControlFrame state;
	char k;

	uint16_t print_x, print_y;

	/* Find the lengths of the strings */
	VH_MeasurePropString(s1, &w1, &h, 1);
	VH_MeasurePropString(s2, &w2, &h, 1);
	if (s3)
		VH_MeasurePropString(s3, &w3, &h, 1);
	else
		w3 = 0;

	/* Work out the window size and position and draw it */
	window_w = (w1 > w2) ? ((w1 > w3) ? w1 : w3) : w2;
	window_w += 7;
	sh = h;
	h *= (s3 ? 5 : 4);
	USL_DialogSetup(window_w, h, &x, &y);

	/* Print the message */
	US_SetPrintColour(2);
	print_x = x + (window_w - w1) / 2;
	print_y = y + sh + 1;
	VHB_DrawPropString(s1, print_x, print_y, 1, US_GetPrintColour());

	print_y += (sh * 2) - 1;
	VHB_HLine(x + 3, x + window_w - 3, print_y, 10);

	/* Print the OK prompt */
	print_y += 2;
	US_SetPrintColour(10);
	print_x = x + (window_w - w2) / 2;
	VHB_DrawPropString(s2, print_x, print_y, 1, US_GetPrintColour());

	/* Print the third string ( if any ) */
	print_y += sh;
	if (s3)
	{
		print_x = x + (window_w - w3) / 2;
		VHB_DrawPropString(s3, print_x, print_y, 1, US_GetPrintColour());
	}

	VH_UpdateScreen();

	/* Wait for button1 or a key to be pressed */
	IN_ClearKeysDown();
	do
	{
		k = IN_SC_None;
		VL_Yield(); // Keep CPU usage low
		IN_PumpEvents();
		IN_ReadControls(0, &state);
		if (state.jump)
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
		VL_Present();
	} while (k == IN_SC_None);

	/* Wait for the button to be released */
	do
	{
		VL_Yield(); // Keep CPU usage low
		IN_PumpEvents();
		IN_ReadControls(0, &state);
		VL_Present();
	} while (state.jump || state.pogo);
	IN_ClearKeysDown();

	US_DrawCards();

	/* Return true or false based on the user's choice */
	if (k == IN_SC_Y)
		return 1;
	else
		return 0;
}

int USL_ConfirmComm(US_CardCommand command)
{
	int result;
	const char *s1, *s2, *s3;
	int ask_user;

	if (command == US_Comm_None)
		Quit("USL_ConfirmComm() - empty comm"); /* quit */

	result = 1;
	ask_user = 0;
	s3 = "ESC TO BACK OUT";
	switch (command)
	{
	case US_Comm_EndGame:
		s1 = "REALLY END CURRENT GAME?";
		s2 = "PRESS Y TO END IT";
		if (game_in_progress && game_unsaved)
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
		if (game_in_progress && game_unsaved)
			ask_user = 1;
		break;

	case US_Comm_NewEasyGame:
	case US_Comm_NewNormalGame:
	case US_Comm_NewHardGame:
		s1 = "YOU'RE IN A GAME";
		s2 = "PRESS Y FOR NEW GAME";
		if (game_in_progress && game_unsaved)
			ask_user = 1;
		break;

	default:
		break;
	}

	result = ask_user ? USL_CtlDialog(s1, s2, s3) : 1;
	if (result)
	{
		us_currentCommand = command;
		command_confirmed = 1;
	}
	return result;
}

void USL_HandleError(int error)
{
	// FIXME
}

void USL_EnableMenuItems(US_Card *card)
{
	US_CardItem *item;

	if (card->items)
	{
		item = card->items;
		while (item->type != US_ITEM_None)
		{
			item->state &= ~US_IS_Disabled;
			item->state &= ~US_IS_Selected;
			item->state &= ~US_IS_Checked;

			if (item->subMenu)
				USL_EnableMenuItems(item->subMenu);
			item++;
		}
	}
}

int USL_GetCheckedItem(US_Card *card)
{
	US_CardItem *item;
	int index;

	item = card->items;
	index = 0;
	while (item->type != US_ITEM_None)
	{
		if (item->state & US_IS_Checked)
		{

			return index;
		}
		item++;
		index++;
	}
	return -1;
}

void USL_SelectCardItem(US_Card *card, int index, int redraw)
{
	US_CardItem *item;

	if (card->selectedItem != index)
	{
		item = &(card->items[card->selectedItem]);
		item->state &= ~US_IS_Selected;
		if (redraw)
			USL_DrawCardItem(item);
	}
	card->selectedItem = index;
	item = &(card->items[card->selectedItem]);
	item->state |= US_IS_Selected;

	if (redraw)
		USL_DrawCardItem(item);
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
	if (us_currentCard->items[us_currentCard->selectedItem + 1].type != US_ITEM_None)
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

void USL_SetMenuFooter(void)
{
	footer_str[2] = "Arrows move";
	footer_str[1] = "Enter selects";
	footer_str[0] = (us_cardStackIndex != 0) ? "ESC to back out" : "ESC to quit";
	USL_SelectCardItem(us_currentCard, us_currentCard->selectedItem, 0);
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

	//TODO:  Handle Joystick/Kbd footer stuff
	USL_SetMenuFooter();
}

void USL_UpLevel()
{
	if (!us_cardStackIndex)
	{
		USL_ConfirmComm(US_Comm_Quit);
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

	item = &(us_currentCard->items[us_currentCard->selectedItem]);

	// Play disallowed sound if menu item is disabled
	if (item->state & US_IS_Disabled)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_NEEDKEYCARD));
		return;
	}

	// Handle the various Card item types
	switch (item->type)
	{
	case US_ITEM_Normal:
		if (us_currentCard->msgCallback)
		{
			if (us_currentCard->msgCallback(US_MSG_ItemEntered, item))
				break;
		}
		USL_ConfirmComm(item->command);
		return;
	case US_ITEM_Radio:
		USL_CheckCardItem(us_currentCard, us_currentCard->selectedItem, 1);
		return;
	case US_ITEM_Submenu:
		USL_DownLevel(item->subMenu);
		break;
	default:
		break;
	}
}

extern void CK_US_UpdateOptionsMenus();

void USL_UpdateCards(void)
{
	int i;

	/* SFX and Music menus */
	i = SD_GetSoundMode();

	if ((i == sdm_AdLib) && SD_GetQuietSfx())
		i++;

	USL_CheckCardItem(&ck_us_soundMenu, i, 0);
	USL_CheckCardItem(&ck_us_musicMenu, SD_GetMusicMode(), 0);

	if (!SD_IsAdlibPresent())
	{
		/* Disable adlib & sb menu items */
		ck_us_soundMenuItems[2].state |= US_IS_Disabled;
		ck_us_soundMenuItems[3].state |= US_IS_Disabled;
		ck_us_musicMenuItems[1].state |= US_IS_Disabled;
	}

	/* Joystick and gamepad menu items*/
	if (true) //!joystick_present[0] )
		ck_us_configureMenuItems[4].state |= US_IS_Disabled;
	if (true) //!joystick_present[1] )
		ck_us_configureMenuItems[5].state |= US_IS_Disabled;
#if 0
	if ( !joystick_present[0] && !joystick_present[1] )
		configure_menu_items[6].state |= US_IS_Disabled;
#endif
	ck_us_mainMenuItems[4].caption = (game_in_progress) ? "RETURN TO GAME" : "RETURN TO DEMO";
	/* Save game and end game items */
	if (!game_in_progress)
	{
		ck_us_mainMenuItems[2].state |= US_IS_Disabled;
		ck_us_mainMenuItems[5].state |= US_IS_Disabled;
	}

	/* Hilite 'return to game' or 'new game' option */
	ck_us_mainMenu.selectedItem = (game_in_progress) ? 4 : 0;

	/* Update the options menu */
	CK_US_UpdateOptionsMenus();
}

void USL_BeginCards()
{
	CA_UpLevel();

	// NOTE: I'm caching more stuff here than is necessary

	// Cache all bitmaps
	for (int i = ca_gfxInfoE.offBitmaps; i < (ca_gfxInfoE.offBitmaps + ca_gfxInfoE.numBitmaps); ++i)
	{
		CA_CacheGrChunk(i);
	}

	// Cache the font
	CA_CacheGrChunk(CK_CHUNKNUM(FON_WATCHFONT));

	// Cache the wristwatch screen masked bitmap
	CA_CacheGrChunk(ca_gfxInfoE.offMasked);

	// Cache the tile8's
	CA_CacheGrChunk(ca_gfxInfoE.offTiles8);
	CA_CacheGrChunk(ca_gfxInfoE.offTiles8m);

	// Cache the first few sprites for paddlewar
	// for (int i = ca_gfxInfoE.offSprites; i < (ca_gfxInfoE.offSprites + 10); ++i)
	for (int i = CK_CHUNKNUM(SPR_PADDLE); i <= CK_CHUNKNUM(SPR_BALL3); ++i)
	{
		CA_CacheGrChunk(i);
	}

	CA_LoadAllSounds();

	US_SetPrintFont(1);
	// US_SetPrintFunctions(VW_MeasurePropString, VWB_DrawPropString);
	US_SetPrintColour(15);

	VL_ClearScreen(3);
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

void USL_HandleComm(int command)
{
	switch (command)
	{
	case US_Comm_EndGame: /* end game */
		load_game_error = 1;
		return;

	case US_Comm_Quit: /* quit */
		quit_to_dos = 1;
		return;

	case US_Comm_NewEasyGame: /* easy game */
		ck_startingDifficulty = D_Easy;
		return;

	case US_Comm_NewNormalGame: /* normal game */
		ck_startingDifficulty = D_Normal;
		return;

	case US_Comm_NewHardGame: /* hard game */
		ck_startingDifficulty = D_Hard;
		return;

	case US_Comm_ReturnToGame:
	case 4: /* return to game, ? */
		return;

	default:
		Quit("USL_HandleComm() - unknown"); /* quit */
	}
}

void USL_SetSoundAndMusic()
{
	int i;

	i = USL_GetCheckedItem(&ck_us_soundMenu);
	// Quiet AdLib sound effects
	if (i == 3)
	{
		SD_SetQuietSfx(true);
		i--;
	}
	else
	{
		SD_SetQuietSfx(false);
	}
	if (i != (int)SD_GetSoundMode())
		SD_SetSoundMode((SD_SoundMode)i);
	i = USL_GetCheckedItem(&ck_us_musicMenu);
	if (i != (int)SD_GetMusicMode())
		SD_SetMusicMode((ID_MusicMode)i);
}

void USL_EndCards()
{

	USL_SetSoundAndMusic();

	if (us_currentCommand != US_Comm_None)
	{
		USL_HandleComm(us_currentCommand);
	}

	US_SetPrintFont(0);
	US_SetPrintColour(15);

	if (ck_startingDifficulty && p_exit_menu)
		p_exit_menu();

	if (quit_to_dos)
	{
		// This should be TEDDeath(), but we don't actually have TED.
		if (us_tedLevel)
			Quit(NULL);
		US_CenterWindow(0x14, 3);
		US_SetPrintColour(3);
		US_PrintCentered("Quitting...");
		US_SetPrintColour(15);
		VH_UpdateScreen();
		Quit(NULL);
	}

	IN_ClearKeysDown();
	SD_WaitSoundDone();
	//FIXME: Cyan should be drawn, but this results in a cyan background
	//for longer than expected (possibly due to legal of EGA features)
	//VL_ClearScreen(3); // Draw Cyan
	VL_ClearScreen(0); // For now we draw black
	CA_DownLevel();
	CA_LoadAllSounds();
}

// What is this function for?
void USL_EnterCurrentItem()
{
	US_CardItem *item = &us_currentCard->items[us_currentCard->selectedItem];

	if (item->state & US_IS_Disabled)
	{
		SD_PlaySound(CK_SOUNDNUM(SOUND_NEEDKEYCARD));
		return;
	}

	switch (item->type)
	{
	case US_ITEM_Normal:
		if (us_currentCard->msgCallback)
		{
			if (us_currentCard->msgCallback(US_MSG_ItemEntered, item))
			{
				break;
			}
		}

		USL_ConfirmComm(item->command);
		return;
	case US_ITEM_Radio:
		USL_CheckCardItem(us_currentCard, us_currentCard->selectedItem, 1);
		return;
	case US_ITEM_Submenu:
		USL_PushCard(item->subMenu);
		break;
	default:
		break;
	}
}

void US_RunCards()
{
	int16_t controller_dy;
	int16_t prev_controller_motion;
	uint32_t cursor_time;
	//ControlInfo status;
	bool action_taken;

	bool cursor = false;

	/* clear all keys except for function keys */
	if (IN_GetLastScan() < IN_SC_F1 || IN_GetLastScan() > IN_SC_F10)
		IN_ClearKeysDown();

	// load all menu stuff and draw main menu
	USL_BeginCards();
	US_DrawCards();

	controller_dy = 0;
	prev_controller_motion = 0;
	command_confirmed = 0;
	cursor = 1;
	action_taken = true;

	while (!command_confirmed)
	{
		US_CardItem *item = &(us_currentCard->items[us_currentCard->selectedItem]);

		IN_ScanCode lastScan = IN_GetLastScan();

		/* Draw the icon for the current item highlighted or not */
		if (action_taken)
		{
			cursor_time = SD_GetTimeCount() + 35; // 1/2 second flashes
			action_taken = false;
		}

		// draw icon with cursor blink
		if (SD_GetTimeCount() >= cursor_time)
		{
			cursor = !cursor;
			action_taken = true;

			if (!cursor)
				item->state &= ~US_IS_Selected;

			USL_DrawCardItemIcon(item);
			item->state |= US_IS_Selected;
		}

		VH_UpdateScreen();

		if (lastScan != IN_SC_None)
		{
			switch (lastScan)
			{
			case IN_SC_UpArrow:
				US_SelectPrevItem();
				action_taken = true;
				break;
			case IN_SC_DownArrow:
				US_SelectNextItem();
				action_taken = true;
				break;
			case IN_SC_Enter:
				US_SelectCurrentItem();
				action_taken = true;
				break;
			case IN_SC_Escape:
				USL_UpLevel();
				action_taken = true;
				break;
			case IN_SC_F1:
#ifdef HAS_HELPSCREEN
				if (ck_currentEpisode->ep != EP_CK6)
				{
					HelpScreens();
					US_DrawCards();
					action_taken = true;
					break;
				}
#endif
				// Fall-through
			default:
				if (lastScan == in_kbdControls.jump || lastScan == in_kbdControls.pogo)
				{
					US_SelectCurrentItem();
					action_taken = true;
					break;
				}

				for (int i = 0; us_currentCard->items[i].type != US_ITEM_None; ++i)
				{
					if (lastScan == us_currentCard->items[i].shortcutKey)
					{
						US_SelectItem(us_currentCard, i, true);
						action_taken = true;
						break;
					}
				}
			}
			IN_ClearKeysDown();
		}
		else
		{
			IN_Cursor cursor;
			IN_ReadCursor(&cursor);

#ifndef CK_VANILLA
			if (cursor.yMotion != prev_controller_motion)
			{
				// when pushing the controller,
				// move one menu item immediately
				controller_dy = cursor.yMotion * 40;
			}
			else
#endif
			{
				controller_dy += cursor.yMotion;
			}
			prev_controller_motion = cursor.yMotion;

			if (cursor.button0)
			{
				while (cursor.button0)
				{
					IN_ReadCursor(&cursor);
					IN_PumpEvents();
					VL_Present();
				}
				US_SelectCurrentItem();
				action_taken = true;
			}
			else if (cursor.button1)
			{
				while (cursor.button1)
				{
					IN_ReadCursor(&cursor);
					IN_PumpEvents();
					VL_Present();
				}
				USL_UpLevel();
				action_taken = true;
			}
			else if (controller_dy < -US_MENU_MOTION_SPEED)
			{
				controller_dy += US_MENU_MOTION_SPEED;
				US_SelectPrevItem();
				action_taken = true;
			}
			else if (controller_dy > US_MENU_MOTION_SPEED)
			{
				controller_dy -= US_MENU_MOTION_SPEED;
				US_SelectNextItem();
				action_taken = true;
			}
		}
		IN_PumpEvents();
		//CK_SetTicsPerFrame();
		VL_Present();
	}

	USL_EndCards();
}
