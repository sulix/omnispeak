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

#ifndef CK5_EP_H
#define CK5_EP_H

#include <stdbool.h>
#include "ck_ep.h"

/*
 * Contains definitions relevent only to Keen 5
 */
extern CK_Episode ck5_episode;

/* Action functions setup */
void CK5_Obj1_SetupFunctions();
void CK5_Obj3_SetupFunctions();
void CK5_SetupFunctions();

void CK5_ScanInfoLayer();


/* Spawning functions */

/* ck5_misc.c */
void CK5_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void CK5_SpawnItem(int tileX, int tileY, int itemNumber);

/* ck5_obj1.c */
void CK5_TurretSpawn(int tileX, int tileY, int direction);

/* ck5_obj3.c */
void CK5_SpawnSpirogrip(int tileX, int tileY);
void CK5_SpawnKorath(int tileX, int tileY);
#endif
