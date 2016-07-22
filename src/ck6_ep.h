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

#ifndef CK6_EP_H
#define CK6_EP_H

#include <stdbool.h>
#include "ck_ep.h"

/*
 * Contains definitions relevant only to Keen 5
 */
extern CK_EpisodeDef ck6_episode;

/* Action functions setup */
void CK6_Obj1_SetupFunctions(void);
void CK6_Obj2_SetupFunctions(void);
void CK6_Obj3_SetupFunctions(void);
void CK6_Map_SetupFunctions(void);
void CK6_SetupFunctions(void);
void CK6_DefineConstants(void);

void CK6_ScanInfoLayer();

/* Spawning functions */

/* ck6_misc.c */
CK_object *CK6_SpawnEnemyShot(int posX, int posY, CK_action *action);
void CK6_SpawnItem(int tileX, int tileY, int itemNumber);

/* ck6_obj1.c */
void CK6_SpawnBlooglet(int tileX, int tileY, int type);

/* ck6_obj2.c */
void CK6_SpawnNospike(int tileX, int tileY);
void CK6_SpawnGik(int tileX, int tileY);
void CK6_SpawnOrbatrix(int tileX, int tileY);
void CK6_SpawnBipship(int tileX, int tileY);
void CK6_SpawnFlect(int tileX, int tileY);

/* ck6_obj3.c */
void CK6_SpawnFleex(int tileX, int tileY);
void CK6_SpawnBobba(int tileX, int tileY);
void CK6_SpawnBabobba(int tileX, int tileY);
void CK6_SpawnBlorb(int tileX, int tileY);
void CK6_SpawnCeilick(int tileX, int tileY);

/* ck6_map.c */
void CK6_MapMiscFlagsCheck(CK_object *keen);
void CK_SpawnMapKeen(int tileX, int tileY);

/* Map functions */

/* Misc functions */

/* ck6_misc.c */

#endif
