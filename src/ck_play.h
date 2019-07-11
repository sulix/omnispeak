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

#ifndef CK_PLAY_H
#define CK_PLAY_H

#include <stdbool.h>
#include <stdint.h>

#define STATUS_W 192
#define STATUS_H 152

struct CK_object;

extern int ck_activeX0Tile;
extern int ck_activeY0Tile;
extern int ck_activeX1Tile;
extern int ck_activeY1Tile;

extern bool ck_scrollDisabled;

extern bool ck_godMode;

extern bool ck_debugActive;

extern int16_t ck_invincibilityTimer;

extern bool ck_scoreBoxEnabled;
extern struct CK_object *ck_scoreBoxObj;

extern bool ck_twoButtonFiring;

extern bool ck_fixJerkyMotion;
extern bool ck_svgaCompatibility;

extern bool ck_gamePadEnabled;

extern int16_t *ck_levelMusic;

// Object Mgmt
struct CK_object *CK_GetNewObj(bool nonCritical);
void CK_SetupObjArray();
void CK_RemoveObj(struct CK_object *obj);

// Actions/Camera
void CK_RunAction(struct CK_object *obj);
void CK_CentreCamera(struct CK_object *obj);
void CK_MapCamera(struct CK_object *keen);
void CK_NormalCamera(struct CK_object *obj);

// Status Window
extern void *ck_statusSurface;
void CK_ShowStatusWindow(void);

// Playing
void CK_PlayDemo(int demoChunk);
void CK_PlayLoop();

#endif //!CK_PLAY_H
