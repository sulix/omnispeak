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
 */

#include <stdbool.h>

#include "id_heads.h"
#include "id_us.h"
#include "id_in.h"
#include "id_vh.h"
#include "id_vl.h"

// Card stack can have at most 7 cards
#define US_MAX_CARDSTACK 7

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

typedef enum US_CardItemState
{
	US_IS_Checked = 0x01,
	US_IS_Selected = 0x02,
	US_IS_Disabled = 0x04,
	US_IS_Gap = 0x08
} US_CardItemState;


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

// The last selected command.
US_CardCommand us_currentCommand = US_Comm_None;


struct US_Card;

typedef struct US_CardItem
{
	US_CardItemType type;
	US_CardItemState state;
	IN_ScanCode shortcutKey;
	int unknown;
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
	struct US_CardItem *items;	//Pointer to item array.
	US_CardCallbackFunc msgCallback;
	int selectedItem;		//Index of selected item.
} US_Card;


static int us_cardStackIndex;
US_Card *us_currentCard;
US_Card *us_cardStack[US_MAX_CARDSTACK];

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
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_DrawItemIcon, item))
		return;
	
	VL_ScreenRect(75, item->y, 159, 8, 8);
	
	USL_DrawCardItemIcon(item);

	int fontcolour = 2;
	if ( !(item->state & US_IS_Selected) ||  (item->state & US_IS_Disabled))
		fontcolour = 10;

	VH_DrawPropString(item->caption, item->x + 8, item->y + 1, 3, fontcolour);
}

void USL_DrawCard()
{
	if (us_currentCard->msgCallback && us_currentCard->msgCallback(US_MSG_Draw, 0))
		return;

	if (us_currentCard->gfxChunk)
	{
		VH_DrawBitmap(80,48,us_currentCard->gfxChunk);
	}

	if (us_currentCard->items)
	{
		int itemX = 74;
		int itemY = 60;

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
	if (us_currentCard->items || us_currentCard->gfxChunk)
	{
		VH_DrawBitmap(0, 0, 21);
		USL_DrawCard();
	}

	VL_Present(0,0);
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
		US_SelectItem(us_currentCard, us_currentCard->selectedItem + 1, true);
	}
}

void USL_DownLevel(US_Card *card)
{
	if (!card)
		Quit("USL_DownLevel() - nil card");

	USL_PushCard(card);

	if (card->msgCallback)
	{
		if (card->msgCallback(US_MSG_CardEntered,0))
		{
			USL_PopCard();
		}
	}

	//TODO: Handle Joystick/Kbd footer stuff
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
}

void USL_BeginCards()
{
	for (int i = ca_gfxInfoE.offBitmaps; i < (ca_gfxInfoE.offBitmaps + 1); ++i)
	{
		CA_CacheGrChunk(i);
	}
	
}

void USL_EndCards()
{
	if (us_currentCommand != US_Comm_None)
	{
		//USL_HandleComm(us_currentCommand);
	}
}

void USL_EnterCurrentItem()
{
	US_CardItem *item = &us_currentCard->items[us_currentCard->selectedItem];

	if ( item->state & US_IS_Disabled )
	{
		// Play sound 14
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

			//TODO: Implement USL_ConfirmComm
			//USL_ConfirmComm( item->command );
			return;
		case US_ITEM_Radio:
			// check_menu_item
			return;
		case US_ITEM_Submenu:
			USL_PushCard( item->subMenu );
			break;
	}
}


void US_RunCards()
{
	bool itemBlink = false;
	USL_BeginCards();

	US_DrawCards();

	while (us_currentCard != US_Comm_None)
	{
		US_CardItem *item = &(us_currentCard->items[us_currentCard->selectedItem]);

		IN_ScanCode lastScan = IN_GetLastScan();

		if (lastScan != IN_SC_None)
		{
			switch (lastScan)
			{
			case IN_SC_UpArrow:
				US_SelectPrevItem();
				break;
			case IN_SC_DownArrow:
				US_SelectNextItem();
				break;
			case IN_SC_Enter:
				break;
			case IN_SC_Escape:
				USL_UpLevel();
				break;
			default:
				for (int i = 0; us_currentCard->items[i].type != US_ITEM_None; ++i)
				{
					if (lastScan == us_currentCard->items[i].shortcutKey)
					{
						US_SelectItem(us_currentCard, i, true);
					}
				}
			}
		}

			
		//TODO: Add mouse/joystick support
	}

	USL_EndCards();
}
