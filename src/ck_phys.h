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

#ifndef CK_PHYS_H
#define CK_PHYS_H

#include <stdbool.h>
#include <stdint.h>

#define SLOPEMASK 7

typedef struct CK_objPhysData
{
	// Unit cliping box
	uint16_t unitX1;
	uint16_t unitY1;
	uint16_t unitX2;
	uint16_t unitY2;
	uint16_t unitXmid;


	// Tile clipping box
	uint16_t tileX1;
	uint16_t tileY1;
	uint16_t tileX2;
	uint16_t tileY2;
	uint16_t tileXmid;
} CK_objPhysData;

typedef struct CK_objPhysDataDelta
{
	int16_t unitX1;
	int16_t unitY1;
	int16_t unitX2;
	int16_t unitY2;
	int16_t unitXmid;
} CK_objPhysDataDelta;


struct CK_object;
struct CK_action;

extern bool ck_keenIgnoreVertClip;

bool CK_NotStuckInWall(struct CK_object *obj);
bool CK_PreviewClipRects(struct CK_object *obj, struct CK_action *act);

void CK_PhysUpdateNormalObj(struct CK_object *obj);
void CK_PhysFullClipToWalls(struct CK_object *obj);
void CK_PhysUpdateSimpleObj(struct CK_object *obj);
void CK_PhysPushX(struct CK_object *pushee, struct CK_object *pusher);
void CK_PhysPushY(struct CK_object *pushee, struct CK_object *pusher);
void CK_PhysPushXY(struct CK_object *pushee, struct CK_object *pusher, bool squish);
void CK_SetAction(struct CK_object *obj, struct CK_action *act);
void CK_SetAction2(struct CK_object *obj, struct CK_action *act);
bool CK_ObjectVisible(struct CK_object *obj);
void CK_PhysGravityHigh(struct CK_object *obj);
void CK_PhysGravityMid(struct CK_object *obj);
void CK_PhysGravityLow(struct CK_object *obj);
void CK_PhysDampHorz(struct CK_object *obj);
void CK_PhysAccelHorz(struct CK_object *obj, int16_t accX, int16_t velLimit);
void CK_PhysAccelHorz2(struct CK_object *obj, int16_t accX, int16_t velLimit);
void CK_PhysAccelVert1(struct CK_object *obj, int16_t accY, int16_t velLimit);

void CK_ResetClipRects(struct CK_object *obj);
#endif
