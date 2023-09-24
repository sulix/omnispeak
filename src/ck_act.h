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

#include <stdint.h>

typedef struct CK_object CK_object;
typedef struct CK_action CK_action;

typedef void (*CK_ACT_Function)(CK_object *obj);
typedef void (*CK_ACT_ColFunction)(CK_object *obj1, CK_object *obj2);

void CK_ACT_SetupFunctions();
void CK_ACT_AddFunction(const char *fnName, CK_ACT_Function fn);
void CK_ACT_AddColFunction(const char *fnName, CK_ACT_ColFunction fn);
CK_ACT_Function CK_ACT_GetFunction(const char *fnName);
CK_ACT_ColFunction CK_ACT_GetColFunction(const char *fnName);
void CK_VAR_Startup();
const char *CK_VAR_GetString(const char *name, const char *def);
const char *CK_VAR_GetStringByNameAndIndex(const char *name, int index);
intptr_t CK_VAR_GetInt(const char *name, intptr_t def);
intptr_t *CK_VAR_GetIntArray(const char *name);
intptr_t CK_VAR_GetIntArrayElement(const char *name, int index);
const char **CK_VAR_GetStringArray(const char *name);
const char *CK_VAR_GetStringArrayElement(const char *name, int index);
CK_action *CK_GetActionByName(const char *name);
CK_action *CK_GetOrCreateActionByName(const char *name);
CK_action *CK_LookupActionFrom16BitOffset(uint16_t offset); // POTENTIALLY SLOW function - Use in game loading only!
void CK_VAR_SetInt(const char *name, intptr_t val);
void CK_VAR_SetString(const char *name, const char *val);
void CK_VAR_LoadVars(const char *filename);

#ifdef CK_STRINGS_LINKED
// TODO: This is here in case we want to support building string values directly
// into the executable again. (I have an in-progress tool to do this, though
// it lacks support for a few things, so it's not workable at present.)
#define CK_STRING(s) STRING_ ## s
#else
#define CK_STRING(s) CK_VAR_GetString(#s, #s)
#endif

#ifdef CK_VARS_LINKED
#define CK_INT(name) INT_ ## name
#define CK_CHUNKNUM(name) CHUNK_ ## name
#define CK_SOUNDNUM(name) INT_ ## name
typedef int16_t chunk_id_t;
#define CK_CHUNKID(name) CK_CHUNKNUM(name)
#define CK_LookupChunk(id) (id)
#define CK_INTARRAY(name) INTARRAY_ ## name
#define CK_INTELEMENT(name, i) (CK_INTARRAY(name)[i])
#define CK_STRINGARRAY(name) STRARRAY_ ## name
#define CK_STRINGELEMENT(name) (CK_STRINGARRAY(name)[i])
#else
#define CK_INT(name, default) CK_VAR_GetInt(#name, default)
#define CK_CHUNKNUM(name) CK_VAR_GetInt(#name, 0)
#define CK_SOUNDNUM(name) CK_VAR_GetInt(#name, 0)
typedef const char *chunk_id_t;
#define CK_CHUNKID(name) #name
#define CK_LookupChunk(id) CK_VAR_GetInt(id, 0)
#define CK_INTARRAY(name) CK_VAR_GetIntArray(#name)
#define CK_INTELEMENT(name, i) CK_VAR_GetIntArrayElement(#name, i)
#define CK_STRINGARRAY(name) CK_VAR_GetStringArray(#name)
#define CK_STRINGELEMENT(name, i) CK_VAR_GetStringArrayElement(#name, i)
#endif


#endif

