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

#ifndef CK_ACT_H
#define CK_ACT_H

#include "ck_def.h"

typedef void (*CK_ACT_Function)(CK_object *obj);
typedef void (*CK_ACT_ColFunction)(CK_object *obj1, CK_object *obj2);

void CK_ACT_SetupFunctions();
void CK_ACT_AddFunction(const char *fnName, CK_ACT_Function fn);
void CK_ACT_AddColFunction(const char *fnName, CK_ACT_ColFunction fn);
CK_ACT_Function CK_ACT_GetFunction(const char *fnName);
CK_ACT_ColFunction CK_ACT_GetColFunction(const char *fnName);
CK_action *CK_GetActionByName(const char *name);
CK_action *CK_GetOrCreateActionByName(const char *name);
CK_action *CK_LookupActionFrom16BitOffset(uint16_t offset); // POTENTIALLY SLOW function - Use in game loading only!
void CK_ACT_LoadActions(const char *filename);
#endif
