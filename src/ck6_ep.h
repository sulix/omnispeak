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
void CK6_SpawnFuseExplosion(int tileX, int tileY);
void CK6_SpawnLevelEnd(void);
void CK6_SpawnLightning(void);
CK_object *CK6_SpawnEnemyShot(int posX, int posY, CK_action *action);
void CK6_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void CK6_SpawnRedStandPlatform(int tileX, int tileY);
void CK6_SpawnItem(int tileX, int tileY, int itemNumber);

/* ck6_obj1.c */
void CK6_TurretSpawn(int tileX, int tileY, int direction);
void CK6_SneakPlatSpawn(int tileX, int tileY);
void CK6_GoPlatSpawn(int tileX, int tileY, int direction, bool purple);
void CK6_SpawnVolte(int tileX, int tileY);

/* ck6_obj2.c */
void CK6_SpawnSparky(int tileX, int tileY);
void CK6_SpawnAmpton(int tileX, int tileY);
void CK6_SpawnSlice(int tileX, int tileY, int dir);
void CK6_SpawnSliceDiag(int tileX, int tileY);
void CK6_SpawnShelly(int tileX, int tileY);

/* ck6_obj3.c */
void CK6_SpawnMine(int tileX, int tileY);
void CK6_SpawnRobo(int tileX, int tileY);
void CK6_SpawnSpirogrip(int tileX, int tileY);
void CK6_SpawnSpindred(int tileX, int tileY);
void CK6_SpawnMaster(int tileX, int tileY);
void CK6_SpawnShikadi(int tileX, int tileY);
void CK6_SpawnShocksund(int tileX, int tileY);
void CK6_SpawnSphereful(int tileX, int tileY);
void CK6_SpawnKorath(int tileX, int tileY);
void CK6_QEDSpawn(int tileX, int tileY);

/* ck6_map.c */
void CK6_MapMiscFlagsCheck(CK_object *keen);
void CK_SpawnMapKeen(int tileX, int tileY);
void CK6_MapKeenTeleSpawn(int tileX, int tileY);

/* Map functions */
void CK6_AnimateMapTeleporter(int tileX, int tileY);
void CK6_AnimateMapElevator(int tileX, int tileY, int dir);

/* Misc functions */

/* ck6_misc.c */
void CK6_ExplodeGalaxy();
void CK6_FuseMessage();

#endif
