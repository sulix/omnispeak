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

#ifndef CK4_EP_H
#define CK4_EP_H

#include <stdbool.h>
#include "ck_ep.h"

#define SOUND_WORMMOUTHBITE 5
#define SOUND_MUSHROOMLEAP 16
#define SOUND_SKYPESTSQUISH 22
#define SOUND_MUSHROOMHOP 28
#define SOUND_COUNCILSAVE 31
#define SOUND_BERKELOIDTHROW 33
#define SOUND_LIGHTNINGBOLT 39
#define SOUND_FIREBALLLAND 40
#define SOUND_FOOTAPPEAR 45
#define SOUND_SLUGSLIME 46

/*
 * Contains definitions relevant only to Keen 5
 */
extern CK_EpisodeDef ck4_episode;

/* Action functions setup */
void CK4_Obj1_SetupFunctions(void);
void CK4_Obj2_SetupFunctions(void);
void CK4_Obj3_SetupFunctions(void);
void CK4_Map_SetupFunctions(void);
void CK4_SetupFunctions(void);
void CK4_DefineConstants(void);

void CK4_ScanInfoLayer();

/* Spawning functions */

/* ck4_misc.c */
void CK4_SpawnLevelEnd(void);
CK_object *CK4_SpawnEnemyShot(int posX, int posY, CK_action *action);
void CK4_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void CK4_SpawnRedStandPlatform(int tileX, int tileY);

void CK4_KeenSwimDraw(CK_object *obj);
void CK4_KeenSwimCol(CK_object *a, CK_object *b);
void CK4_KeenSwimFast(CK_object *obj);
void CK4_KeenSwim(CK_object *obj);
void CK4_ShowCouncilMessage(void);
void CK4_ShowWetsuitMessage(void);
void CK4_ShowCantSwimMessage(void);
void CK4_ShowJanitorMessage(void);
void CK4_ShowPrincessMessage(void);

/* ck4_obj1.c */
void CK4_SpawnMiragia(int tileX, int tileY);
void CK4_SpawnCouncilMember(int tileX, int tileY);
void CK4_SpawnSlug(int tileX, int tileY);
void CK4_SpawnMushroom(int tileX, int tileY);
void CK4_SpawnEgg(int tileX, int tileY);
void CK4_SpawnBird(int tileX, int tileY);
void CK4_SpawnArachnut(int tileX, int tileY);
void CK4_SpawnSkypest(int tileX, int tileY);

/* ck4_obj2.c */
void CK4_SpawnWormmouth(int tileX, int tileY);
void CK4_SpawnCloud(int tileX, int tileY);
void CK4_SpawnBerkeloid(int tileX, int tileY);
void CK4_SpawnInchworm(int tileX, int tileY);
void CK4_SpawnFoot(int tileX, int tileY);


/* ck4_obj3.c */

/* ck4_map.c */
void CK_DemoSignSpawn();
void CK_UpdateScoreBox(CK_object *scorebox);
void CK_SpawnMapKeen(int tileX, int tileY);

/* Map functions */
void CK_MapMiscFlagsCheck(CK_object *keen);
void CK_MapFlagSpawn(int tileX, int tileY);

/* Misc functions */

/* ck4_misc.c */
void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act);

#endif
