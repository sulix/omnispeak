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

#include <stdio.h>
#include <stdlib.h>
#include "id_heads.h"
#include "ck_cross.h"
#include "ck_def.h"

void Quit(const char *msg)
{
	// Shutdown VL early to return to text mode.
	VL_Shutdown();
	if (!msg || !(*msg))
	{
		// Avoid trying to re-print the order screen if caching it failed.
		static bool quitting = false;
		if (US_TerminalOk() && !quitting)
		{
			quitting = true;
			CA_CacheGrChunk(EXTERN_ORDERSCREEN);
			// There is a 7-byte BSAVE header at the start of the
			// chunk, and we don't want to print the last row, as
			// originally it would be overwritten by DOS anyway.
			US_PrintB8000Text((uint8_t *)(ca_graphChunks[EXTERN_ORDERSCREEN]) + 7, 2000 - 80);
		}
		else
			CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Thanks for playing Commander Keen!\n");
		CK_ShutdownID();
		exit(0);
	}
	else
	{
		CK_Cross_puts(msg);
		CK_ShutdownID();
#ifdef WITH_SDL
#if SDL_VERSION_ATLEAST(1, 3, 0)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Omnispeak", msg, NULL);
#endif
#endif
		exit(-1);
	}
}
