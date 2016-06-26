/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2013 David Gow <david@ingeniumdigital.com>

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

#ifndef CK_EP_H
#define CK_EP_H

typedef enum CK_Episode
{
  EP_Nil,
  EP_CK4,
  EP_CK5,
  EP_CK6,
} CK_Episode;

//This structure defines an episode of Commander Keen, providing
//function pointers to episode specific code and data.
typedef struct CK_EpisodeDef
{
  // Identifier
  CK_Episode ep;

	// Extension for data files
	char ext[4];

	// Setup function names for ACTIONS.EXT
	void (*setupFunctions)();

	// Scan the 'info layer' of a map.
	void (*scanInfoLayer)();

  // Faux-#define episode-dependent constants
	void (*defineConstants)();
} CK_EpisodeDef;

extern CK_EpisodeDef *ck_currentEpisode;

#endif
