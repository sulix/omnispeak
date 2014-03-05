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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ck_def.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_vl.h"

int help_delay, help_pic, help_y, help_x;
char *help_ptr;
int help_line;
int help_line_startx[18], help_line_endx[18];
int help_cur_page, help_num_pages, help_topic;
int help_full_page;
/* The chunks for each of the topics */
int help_chunks[] ={
	4914, 4915, 4916, 4920, 4917
};

void RipToEOL( void )
{
	while ( *help_ptr++ != '\n' )
		;
}

int ParseNumber( void )
{
	char s[81];
	char *p;
	char c;

	/* Find first digit */
	c = *(char *) help_ptr;
	while ( c < '0' || c > '9' )
	{
		help_ptr++;
		c = *help_ptr;
	}

	/* Copy digits into array */
	p = s;
	do
	{
		*p++ = c; /* will be digit */

		help_ptr++;
		c = *help_ptr;
	} while ( c >= '0' && c <= '9' );

	*p = 0; /* Null terminator */

	return atoi( s );
}

void ParsePicCommand( void )
{
	help_y = ParseNumber();
	help_x = ParseNumber();
	help_pic = ParseNumber();
	RipToEOL();
}

/*   F.00A1                       sub_276         proc    near */
void ParseTimedCommand( void )
{
	help_y = ParseNumber();
	help_x = ParseNumber();
	help_pic = ParseNumber();
	help_delay = ParseNumber();
	RipToEOL();
}

void TimedPicCommand( void )
{
	int timeCount = 0;

	ParseTimedCommand();
	// VW_WaitVBL( 1 );
	VL_GetTics(1);
	// VWL_ScreenToScreen( AZ : A7B4, AZ : A7B2, 40, 200 );
	// (long) TimeCount = 0;

	while ( timeCount+=VL_GetTics(1) < help_delay )
		;
	VH_DrawBitmap( help_x & 0xFFF8, help_y, help_pic );
}

void HandleCommand( void )
{
	int16_t i, w, h, midx, wrapx, miny, maxy;
	VH_BitmapTableEntry bmpinfo;

	help_ptr++;
	switch ( toupper( *help_ptr ) )
	{
	case 'B':
		/* Solid red box y, x, w, h */
		help_y = ParseNumber();
		help_x = ParseNumber();
		w = ParseNumber();
		h = ParseNumber();
		VH_Bar( help_x, help_y, w, h, 4 );
		RipToEOL();
		break;

	case 'P': case 'E':
		/* Page or end of text */
		help_full_page = 1;
		help_ptr--;
		break;

	case 'C':
		/* Set the text color */
		help_ptr++;

		/* A single hex digit is used for the text color */
		i = toupper( *help_ptr );
		if ( i >= '0' && i <= '9' )
			US_SetPrintColour( i - '0');
		else if ( i >= 'A' && i <= 'F' )
			US_SetPrintColour( i - 'A' + 10);

		/* We're writing on a red background, so take that into account */
		US_SetPrintColour(US_GetPrintColour()^4);
		help_ptr++;
		break;

	case 'L':
		/* Set the next location for text */
		US_SetPrintY(ParseNumber());

		/* Set the y position to the nearest line */
		help_line = (US_GetPrintY() - 10) / 10;
		US_SetPrintY(help_line * 10 + 10);

		/* Set the x position */
		US_SetPrintX(ParseNumber());

		/* Skip to the next line */
		while ( *help_ptr++ != '\n' )
			;
		break;

	case 'T':
		TimedPicCommand();
		break;

	case 'G':
		/* Draw the picture */
		ParsePicCommand();
		VH_DrawBitmap( help_x & ~7, help_y, help_pic );
		bmpinfo = VH_GetBitmapTableEntry(help_pic - 6);
		w = bmpinfo.width * 8;
		h = bmpinfo.height;

		/* Wrap text either on the left or the right-hand-side of the picture */
		midx = help_x + w / 2;
		if ( midx > 160 )
			wrapx = help_x - 8;
		else
			wrapx = help_x + w + 8;

		/* Get the first line that the picture covers */
		miny = (help_y - 10) / 10;
		if ( miny < 0 )
			miny = 0;

		/* Get the last line that the picture covers */
		maxy = (help_y + h - 10) / 10;
		if ( maxy >= 18 )
			maxy = 17;

		/* Set the lines' starting or finishing text positions */
		for ( i = miny; i <= maxy; i++ )
		{
			if ( midx > 160 )	/* Wrapping to the left */
				help_line_endx[ i ] = wrapx;
			else			/* Wrapping to the right */
				help_line_startx[ i ] = wrapx;
		}

		/* Make sure the next print doesn't start in the middle of the pic */
		if ( help_line_startx[ help_line ] > US_GetPrintX() )
			US_SetPrintX(help_line_startx[ help_line ]);
		break;
	}
}

void NewLine( void )
{
	char c;

	help_line++;
	if ( help_line == 18 )
	{
		/* Move back to line 1 and read input till the next ^P or ^E command */
		help_full_page = 1;
		while ( 1 )
		{
			if ( *help_ptr == '^' )
			{
				c = toupper( *(help_ptr + 1) );
				if ( c == 'E' || c == 'P' )
				{
					help_full_page = 1;
					return;
				}
			}
			help_ptr++;
		}
	}

	/* Move to the next line */
	US_SetPrintX(help_line_startx[ help_line ]);
	US_SetPrintY(US_GetPrintY() + 10);
}

void HandleChar( void )
{
	char c;
	c = *help_ptr++;

	/* If the last character was a newline, do something about it! */
	if ( c == '\n' )
		NewLine();
}

void HandleWord( void )
{
	char buf[80];
	uint16_t w, h, maxx;
	int16_t i;

	/* Read a word into the buffer */
	buf[0] = *help_ptr++;
	i = 1;
	while ( *help_ptr > ' ' )
	{
		buf[i++] = *help_ptr++;
		if ( i == 80 )
			Quit( "PageLayout: Word limit exceeded" );
	}

	/* Terminate the buffer with a null */
	buf[i] = 0;

	/* Measure the word */
	VH_MeasurePropString( buf,  &w, &h, US_GetPrintFont()  );

	/* See if we can find a line to fit the word on */
	while ( help_line_endx[ help_line ] < US_GetPrintX() + w )
	{
		NewLine();
		if ( help_full_page )
			return;
	}

	/* Display the word */
	maxx = US_GetPrintX() + w;
	VH_DrawPropString( buf, US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour() );
	US_SetPrintX(maxx);

	/* Handle spaces between this word and the next */
	while ( *help_ptr == ' ' )
	{
		US_SetPrintX(US_GetPrintX() + 7);
		help_ptr++;
	}
}

void PageLayout( int show_status )
{
	int16_t old_print_color, i;
	char c;

	/* Save the current print color */
	old_print_color = US_GetPrintColour();
	US_SetPrintColour(10);

	/* We want to scanout from the right offset. */
	VL_SetScrollCoords(0,0);

	/* Fill the background and draw the border */
	VH_Bar( 0, 0, 320, 200, 4 );
	VH_DrawBitmap( 0, 0, 28 );	/* Top border */
	VH_DrawBitmap( 0, 8, 29 );	/* Left border */
	VH_DrawBitmap( 312, 8, 30 ); /* Right border */
	if ( show_status )
		VH_DrawBitmap( 8, 176, 31 ); /* Bottom status bar */
	else
		VH_DrawBitmap( 8, 192, 32 ); /* Bottom border */

	/* Set the lines' start and end positions so the text stays within the border */
	for ( i = 0; i < 18; i++ )
	{
		help_line_startx[i] = 10;
		help_line_endx[i] = 310;
	}

	/* The text should start inside the border */
	US_SetPrintX(10);
	US_SetPrintY(10);
	help_line = 0;
	help_full_page = 0;

	/* Skip any nonprinting characters at the start */
	while ( *help_ptr <= ' ' )
		help_ptr++;

	/* Make sure we start with a "new page" command */
	if ( !(*help_ptr++ == '^' && toupper( *help_ptr ) == 'P') )
		Quit( "PageLayout: Text not headed with ^P" );

	/* Move to the next line after the ^P */
	while ( *help_ptr++ != '\n' )
		;

	do
	{
		/* Parse a command or print a word */
		c = *help_ptr;
		if ( c == '^' )
			HandleCommand();
		else if ( c <= ' ' )
			HandleChar();
		else
			HandleWord();
	} while ( help_full_page == 0 );

	/* Set the page number that we're on */
	help_cur_page++;

	/* Show the current page number */
	if ( show_status )
	{
		char buf[64], buf2[64];
		strcpy( buf, "pg " );
		// itoa( help_cur_page, buf2, 10 );
		sprintf(buf2, "%d", help_cur_page); 
		strcat( buf, buf2);
		strcat( buf, " of " );
		// itoa( help_num_pages, buf2, 10 );
		sprintf(buf2, "%d", help_num_pages); 
		strcat( buf, buf2);

		US_SetPrintColour(8);
		US_SetPrintY(186);
		US_SetPrintX(218);
		VH_DrawPropString( buf, US_GetPrintX(), US_GetPrintY(), US_GetPrintFont(), US_GetPrintColour() );
	}

	/* Restore the original print color */
	US_SetPrintColour(old_print_color);
}

void BackPage( void )
{
	help_cur_page--;

	/* Look backwards for a '^P' */
	do
	{
		help_ptr--;
	} while ( !(*help_ptr == '^' && toupper( *(help_ptr + 1) ) == 'P') );
}

extern uint8_t ca_levelnum;
extern uint8_t ca_levelbit;
extern uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS];

void CacheLayoutGraphics( void )
{
	char *pstart;
	char *pmax;
	char c;

	/* Initialise the pointers */
	pstart = help_ptr;
	pmax = pstart + 30000;

	help_num_pages = help_cur_page = 0;

	/* Cache the border graphics */
	// TODO: GrNeeded
#if 0
SEG_AX:
	0050 |= AZ : 7AEC;
SEG_AX:
	0051 |= AZ : 7AEC;
SEG_AX:
	0052 |= AZ : 7AEC;
SEG_AX:
	0053 |= AZ : 7AEC;
SEG_AX:
	0054 |= AZ : 7AEC;
#endif

	do
	{
		if ( *help_ptr == '^' )
		{
			help_ptr++;
			c = toupper( *help_ptr );

			/* Count pages */
			if ( c == 'P' )
			{
				help_num_pages++;
			}
			/* If we've reached the end, rewind the help pointer and return */
			if ( c == 'E' )
			{
				CA_CacheMarks( 0 );
				help_ptr = pstart;
				return;
			}
			/* Cache ordinary graphics */
			if ( c == 'G' )
			{
				ParsePicCommand();
				ca_graphChunkNeeded[help_pic] |= ca_levelbit;
			}
			/* Cache timed graphics */
			if ( c == 'T' )
			{
				ParseTimedCommand();
				ca_graphChunkNeeded[help_pic] |= ca_levelbit;
			}
		}
		else
		{
			help_ptr++;
		}
	} while ( help_ptr < pmax );

	/* If we got here, we didn't find the end of the text */
	Quit( "CacheLayoutGraphics: No ^E to terminate file!" );
}

int ShowHelp( void )
{
	int ymove, key;
	IN_ControlFrame cinfo;
	//CONTROLLER_STATUS cstatus;


	/* Erase the screen */
	VH_Bar( 0, 0, 320, 200, 4 );

	/* Cache graphics */
	CA_CacheGrChunk( 6 );	/* Help menu pic */
	CA_CacheGrChunk( 24 );	/* Help menu pointer */
	CA_CacheGrChunk( 28 );	/* Top border */
	CA_CacheGrChunk( 29 );	/* Left border */
	CA_CacheGrChunk( 30 );	/* Right border */
	CA_CacheGrChunk( 32 );	/* Bottom border */

	/* Draw the border and the main menu pic */
	VH_DrawBitmap( 0, 0, 28 );	/* Top border */
	VH_DrawBitmap( 0, 8, 29 );	/* Left border */
	VH_DrawBitmap( 312, 8, 30 );	/* Right border */
	VH_DrawBitmap( 8, 192, 32 );	/* Bottom border */
	VH_DrawBitmap( 96, 8, 6 );		/* Menu picture */

	ymove = 0;
	IN_ClearKeysDown();
	while ( 1 )
	{
		IN_PumpEvents();

		/* Make sure the pointer is in a valid place */
		if ( help_topic < 0 )
			help_topic = 0;
		else if ( help_topic > 4 )
			help_topic = 4;

		/* Draw the pointer */
		VH_DrawBitmap( 48, 48 + help_topic * 24, 24 );
		VL_SetScrollCoords(0,0);
		VL_Present(); //update_screen();


		/* Erase the pointer */
		VH_Bar( 48, 48 + help_topic * 24, 39, 24, 4 );

		/* Check for input */
		IN_ReadControls( 0, &cinfo );
		// get_controller_status( &cstatus );

		/* Handle keys */
		if ( IN_GetLastScan() != IN_SC_None )
		{
			key = IN_GetLastScan();
			IN_ClearKeysDown();
			switch ( key )
			{
			case IN_SC_UpArrow:
				help_topic--;
				break;
			case IN_SC_Escape:
				// VW_ClearVideo( 4 );
				VH_Bar(0, 0, 320, 200, 4);
				return -1;
			case IN_SC_Enter:
				// VW_ClearVideo( 4 );
				VH_Bar(0, 0, 320, 200, 4);
				return help_topic;
			case IN_SC_DownArrow:
				help_topic++;
				break;
			}
		}

		/* Handle other input */
		//ymove += cstatus.dy;
		if ( /*cstatus.button1 || cstatus.button2 || */cinfo.jump || cinfo.pogo )
		{
			// VW_ClearVideo( 4 );
			VH_Bar(0, 0, 320, 200, 4);
			return help_topic;
		}

		if ( ymove < -40 )
		{
			ymove += 40;
			help_topic--;
		}
		else if ( ymove > 40 )
		{
			ymove -= 40;
			help_topic++;
		}
	}
}

void HelpScreens(void)
{
	int16_t local1, local2, oldfont, page_changed, n;

	/* Save the variables */
	oldfont = US_GetPrintFont();
	// local1 = AZ : A7B2;
	// local2 = AZ : A7B4;

	/* Init stuff */
#if 0
AZ:
	A550 = 0;
	outportw( 0x3C4, 0x0F02 ); /* Set map mask register */
	sub_409();
	uncache_all_graphics();
	VW_ClearVideo( 4 );
	sub_472();
AZ:
	A7B4 = 0;
AZ:
	A7B2 = 0x8000;
	sub_679( AZ : A7B2, 0 );
#endif
	US_SetPrintFont(0);

	StartMusic(19);

	while ( 1 )
	{
		n = ShowHelp();
		// VW_ClearVideo( 4 );
		VH_Bar( 0, 0, 320, 200, 4 );

		/* If the user pressed Esc */
		if ( n == -1 )
		{
			/* Clean up and return */
			// CA_DownLevel();
			IN_ClearKeysDown();
			/*
			AZ: A7B4 = local2;
			AZ: A7B2 = local1;
			 */
			US_SetPrintFont(oldfont);
			// VW_ClearVideo( 4 );
			VH_Bar( 0, 0, 320, 200, 4 );
			// RF_Reset();
			StopMusic();
			return;
		}

		/* Cache the chunk with the topic */
		n = help_chunks[n];
		CA_CacheGrChunk( n );

		/* Set up the help ptr and initialise the parser */
		help_ptr = (char *) ca_graphChunks[n];
		CacheLayoutGraphics();

		page_changed = 1;
		do
		{
			if ( page_changed )
			{
				page_changed = 0;
				PageLayout( 1 );

				/*
				sub_679( AZ : A7B4, 0 );
				local4 = AZ : A7B2;
				AZ: A7B2 = AZ : A7B4;
				AZ: A7B4 = local4;
				 */
				VL_Present();
			}

			/* Wait for the user to press a key */
			// last_scan = 0;
			IN_ClearKeysDown();
			while ( IN_GetLastScan() == IN_SC_None )
				IN_PumpEvents();

			/* Take action */
			switch ( IN_GetLastScan() )
			{
			case IN_SC_UpArrow: case IN_SC_PgUp: case IN_SC_LeftArrow:
				if ( help_cur_page <= 1 )
					break;
				BackPage();	/* This one only takes us back to the *current* page */
				BackPage();
				page_changed = 1;
				break;
			case IN_SC_RightArrow: case IN_SC_DownArrow: case IN_SC_PgDown:
				if ( help_cur_page >= help_num_pages )
					break;
				page_changed = 1;
				break;
			}
		} while ( IN_GetLastScan() != IN_SC_Escape );

		/* Uncache the topic we just saw */
		MM_FreePtr( &ca_graphChunks[n] );
		IN_ClearKeysDown();

		/* Return to main help menu loop */
	}
}

void help_endgame( void )
{
	char *ptext;
	int i;

	/* Set up */
	// VW_ClearVideo( 4 );
	VH_Bar(0, 0, 320, 200, 4);
	// RF_Reset();
	// CA_UpLevel();
	// CA_SetGrPurge2();

	/* Cache the chunkss we need */
	CA_CacheGrChunk( 0x1B );	/* Dim arrow */
	CA_CacheGrChunk( 0x1A );	/* Bright arrow */

	// Check for Korath Fuse
	if (ck_gameState.levelsDone[13] == 0x0E)
	{
		CA_CacheGrChunk( 4919 );
		ptext = (char *)ca_graphChunks[4919];
	}
	else
	{
		CA_CacheGrChunk( 4918 );
		ptext = (char *)ca_graphChunks[4918];
	}

	/* Initialise the parser */
	help_ptr = (char *) ptext;
	CacheLayoutGraphics();

	/* Play some music */
	StartMusic( 14 );

	while ( help_cur_page < help_num_pages )
	{
		/* Draw the current page (which also advances to the next page) */
		PageLayout( 0 );
		IN_ClearKeysDown();
		//sub_679( AZ : A7B4, 0 );
		bool advancePage = false;
		while ( !advancePage )
		{
			/* Draw the bright arrow and wait a short time */
			VH_DrawBitmap( 0x12A, 0xB8, 0x1A );
			for ( i = 0; i < 70; i++ )
			{
				if ( IN_GetLastScan() != IN_SC_None )
				{
					advancePage = true;
					break;
				}
				VL_Present();
				IN_PumpEvents();
				VL_GetTics( 1 );
			}

			/* Draw the dim arrow and wait a short time */
			VH_DrawBitmap( 0x12A, 0xB8, 0x1B );
			for ( i = 0; i < 70; i++ )
			{
				if ( IN_GetLastScan() != IN_SC_None )
				{
					advancePage = true;
					break;
				}
				VL_Present();
				IN_PumpEvents();
				VL_GetTics( 1 );
			}
		}
	}

	/* Uncache our graphics and clean up */
	StopMusic();
	if (ck_gameState.levelsDone[13] == 0x0E)
		MM_FreePtr( &ca_graphChunks[4919] );
	else
		MM_FreePtr( &ca_graphChunks[4918] );
	MM_FreePtr( &ca_graphChunks[0x1B] );
	MM_FreePtr( &ca_graphChunks[0x1A] );
	// CA_DownLevel();
	IN_ClearKeysDown();
	// VW_ClearVideo( 4 );
	VH_Bar(0, 0, 320, 200, 4);
	// RF_Reset();
	// CA_SetGrPurge();
}



