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

typedef struct CK_objPhysData
{
	// Unit cliping box
	int unitX1;
	int unitY1;
	int unitX2;
	int unitY2;
	int unitXmid;


	// Tile clipping box
	int tileX1;
	int tileY1;
	int tileX2;
	int tileY2;
	int tileXmid;
} CK_objPhysData;


struct CK_object;
struct CK_action;

bool CK_NotStuckInWall(struct CK_object *obj);

void CK_PhysUpdateNormalObj(struct CK_object *obj);
void CK_PhysUpdateSimpleObj(struct CK_object *obj);
void CK_PhysPushX(struct CK_object *pushee, struct CK_object *pusher);
void CK_PhysPushY(struct CK_object *pushee, struct CK_object *pusher);
void CK_SetAction(struct CK_object *obj, struct CK_action *act);
void CK_SetAction2(struct CK_object *obj, struct CK_action *act);
bool CK_ObjectVisible(struct CK_object *obj);
void CK_PhysGravityHigh(struct CK_object *obj);
void CK_PhysGravityMid(struct CK_object *obj);
void CK_PhysGravityLow(struct CK_object *obj);
void CK_PhysDampHorz(struct CK_object *obj);
void CK_PhysAccelHorz(struct CK_object *obj, int16_t accX, int16_t velLimit);

void CK_ResetClipRects(struct CK_object *obj);
#endif
