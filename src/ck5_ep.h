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
 * Contains definitions relevant only to Keen 5
 */
extern CK_EpisodeDef ck5_episode;

/* Action functions setup */
void CK5_Obj1_SetupFunctions(void);
void CK5_Obj2_SetupFunctions(void);
void CK5_Obj3_SetupFunctions(void);
void CK5_Map_SetupFunctions(void);
void CK5_SetupFunctions(void);
void CK5_DefineConstants(void);

void CK5_ScanInfoLayer();

/* Spawning functions */

/* ck5_misc.c */
void CK5_SpawnFuseExplosion(int tileX, int tileY);
void CK5_SpawnLevelEnd(void);
void CK5_SpawnLightning(void);
CK_object *CK5_SpawnEnemyShot(int posX, int posY, CK_action *action);
void CK5_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void CK5_SpawnRedStandPlatform(int tileX, int tileY);
void CK5_SpawnItem(int tileX, int tileY, int itemNumber);

/* ck5_obj1.c */
void CK5_TurretSpawn(int tileX, int tileY, int direction);
void CK5_SneakPlatSpawn(int tileX, int tileY);
void CK5_GoPlatSpawn(int tileX, int tileY, int direction, bool purple);
void CK5_SpawnVolte(int tileX, int tileY);

/* ck5_obj2.c */
void CK5_SpawnSparky(int tileX, int tileY);
void CK5_SpawnAmpton(int tileX, int tileY);
void CK5_SpawnSlice(int tileX, int tileY, int dir);
void CK5_SpawnSliceDiag(int tileX, int tileY);
void CK5_SpawnShelly(int tileX, int tileY);

/* ck5_obj3.c */
void CK5_SpawnMine(int tileX, int tileY);
void CK5_SpawnRobo(int tileX, int tileY);
void CK5_SpawnSpirogrip(int tileX, int tileY);
void CK5_SpawnSpindred(int tileX, int tileY);
void CK5_SpawnMaster(int tileX, int tileY);
void CK5_SpawnShikadi(int tileX, int tileY);
void CK5_SpawnShocksund(int tileX, int tileY);
void CK5_SpawnSphereful(int tileX, int tileY);
void CK5_SpawnKorath(int tileX, int tileY);
void CK5_QEDSpawn(int tileX, int tileY);

/* ck5_map.c */
void CK_DemoSignSpawn();
void CK_UpdateScoreBox(CK_object *scorebox);
void CK_SpawnMapKeen(int tileX, int tileY);
void CK5_MapKeenTeleSpawn(int tileX, int tileY);

/* Map functions */
void CK_MapMiscFlagsCheck(CK_object *keen);
void CK_MapFlagSpawn(int tileX, int tileY);

/* Misc functions */

/* ck5_misc.c */
void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act);
void CK5_ExplodeGalaxy();
void CK5_FuseMessage();

#endif
