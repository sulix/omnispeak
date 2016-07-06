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
#define SOUND_MIMROCKJUMP 15
#define SOUND_MUSHROOMLEAP 16
#define SOUND_SKYPESTSQUISH 22
#define SOUND_KEENSWIMHI 26
#define SOUND_KEENSWIMLO 27
#define SOUND_MUSHROOMHOP 28
#define SOUND_SMIRKYSTEAL 29
#define SOUND_SMIRKYTELE 30
#define SOUND_COUNCILSAVE 31
#define SOUND_LICKFLAME 32
#define SOUND_BERKELOIDTHROW 33
#define SOUND_KEENBUBBLE 36
#define SOUND_CK4MINEEXPLODE 37
#define SOUND_SPRITESHOOT 38
#define SOUND_LIGHTNINGBOLT 39
#define SOUND_FIREBALLLAND 40
#define SOUND_DARTSHOOT 41
#define SOUND_DOPEFISHBURP 42
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
void CK4_Misc_SetupFunctions(void);
void CK4_SetupFunctions(void);
void CK4_DefineConstants(void);

void CK4_ScanInfoLayer();

/* Spawning functions */

/* ck4_misc.c */
void CK4_SpawnLevelEnd(void);
CK_object *CK4_SpawnEnemyShot(int posX, int posY, CK_action *action);
void CK4_SpawnScubaKeen (int tileX, int tileY);

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
void CK4_SpawnBounder(int tileX, int tileY);
void CK4_SpawnLick(int tileX, int tileY);
void CK4_SpawnAxisPlatform(int tileX, int tileY, int direction);

/* ck4_obj3.c */
void CK4_SpawnSmirky(int tileX, int tileY);
void CK4_SpawnMimrock(int tileX, int tileY);
void CK4_SpawnDopefish(int tileX, int tileY);
void CK4_SpawnSchoolfish(int tileX, int tileY);
void CK4_SpawnMine(int tileX, int tileY, int direction);
void CK4_SpawnSprite(int tileX, int tileY);
void CK4_SpawnLindsey(int tileX, int tileY);
void CK4_SpawnDartGun(int tileX, int tileY, int direction);
void CK4_SpawnWetsuit(int tileX, int tileY);

/* ck4_map.c */
void CK4_MapMiscFlagsCheck(CK_object *keen);

/* Map functions */
void CK_MapMiscFlagsCheck(CK_object *keen);
void CK_MapFlagSpawn(int tileX, int tileY);

/* Misc functions */

/* ck4_misc.c */
void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act);

#endif
