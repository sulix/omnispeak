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


typedef struct CK_object CK_object;
typedef struct CK_action CK_action;

void CK_PhysUpdateNormalObj(CK_object *obj);
void CK_PhysUpdateSimpleObj(CK_object *obj);
void CK_PhysPushX(CK_object *pushee, CK_object *pusher);
void CK_PhysPushY(CK_object *pushee, CK_object *pusher);
void CK_SetAction(CK_object *obj, CK_action *act);
void CK_SetAction2(CK_object *obj, CK_action *act);
void CK_PhysGravityHigh(CK_object *obj);
void CK_PhysGravityMid(CK_object *obj);
void CK_PhysGravityLow(CK_object *obj);
void CK_PhysDampHorz(CK_object *obj);
void CK_PhysAccelHorz(CK_object *obj, int accX, int velLimit);

void CK_ResetClipRects(CK_object *obj);
#endif
