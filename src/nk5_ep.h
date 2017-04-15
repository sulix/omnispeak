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

#ifndef NK5_EP_H
#define NK5_EP_H

#include <stdbool.h>
#include "ck_ep.h"

/*
 * Contains definitions relevant only to Netkeen 5
 */
extern CK_EpisodeDef nk5_episode;

/* Action functions setup */
void NK5_SetupFunctions(void);
bool NK5_IsPresent(void);
void NK5_DefineConstants(void);

void NK5_ScanInfoLayer();

/* Spawning functions */

/* nk5_misc.c */
CK_object *NK5_SpawnEnemyShot(int posX, int posY, CK_action *action);
void NK5_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void NK5_SpawnItem(int tileX, int tileY, int itemNumber);

#endif
