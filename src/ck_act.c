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

#include "id_str.h"
#include "id_us.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "ck_act.h"
#include "ck_cross.h"

/* The Action Manager
 * This subsystem loads the 'ACTION.CKx' file, which is a text file
 * with 'action' definitions. These are compiled into structures,
 * which have the functions loaded from the function-table.
 *
 */

#define CK_FUNCTABL_SIZE 256

STR_Table *ck_functionTable;

void CK_ACT_SetupFunctions()
{
	STR_AllocTable(&ck_functionTable, CK_FUNCTABL_SIZE);
}

void CK_ACT_AddFunction(const char *fnName, CK_ACT_Function fn)
{
	if (!STR_AddEntry(ck_functionTable, fnName, (void *)fn))
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Function array is full: %s is %d\n", fnName, CK_FUNCTABL_SIZE);
		Quit("AddFunction: Function table is full!");
	}
}

void CK_ACT_AddColFunction(const char *fnName, CK_ACT_ColFunction fn)
{
	if (!STR_AddEntry(ck_functionTable, fnName, (void *)fn))
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Function array is full: %s is %d\n", fnName, CK_FUNCTABL_SIZE);
		Quit("AddColFunction: Function table is full!");
	}
}

CK_ACT_Function CK_ACT_GetFunction(const char *fnName)
{
	if (!strcmp(fnName, "NULL"))
		return 0;
	CK_ACT_Function fnPtr = (CK_ACT_Function)(STR_LookupEntry(ck_functionTable, fnName));
	if (fnPtr == 0)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "GetFunction: Could not find function \"%s\"\n", fnName);
		Quit("GetFunction: Function not found. Check your 'ACTION.CKx' file!");
	}
	return fnPtr;
}

CK_ACT_ColFunction CK_ACT_GetColFunction(const char *fnName)
{
	if (!strcmp(fnName, "NULL"))
		return 0;
	CK_ACT_ColFunction fnPtr = (CK_ACT_ColFunction)(STR_LookupEntry(ck_functionTable, fnName));
	if (fnPtr == 0)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "GetColFunction: Could not find function \"%s\"\n", fnName);
		Quit("GetColFunction: Collision function not found. Check your 'ACTION.CKx' file!");
	}
	return fnPtr;
}

#define CK_VAR_MAXVARS 1024
#define CK_VAR_MAXACTIONS 512

STR_Table *ck_varTable;
ID_MM_Arena *ck_varArena;
CK_action *ck_actionData;
int ck_actionsUsed;

typedef enum CK_VAR_VarType
{
	VAR_Invalid,
	VAR_EOF,
	VAR_Bool,
	VAR_Int,
	VAR_String,
	VAR_Action,
	VAR_TOK_Include
} CK_VAR_VarType;

#ifdef CK_VAR_TYPECHECK
typedef struct CK_VAR_Variable
{
	CK_VAR_VarType type;
	void *value;
} CK_VAR_Variable;
#endif

void CK_VAR_Startup()
{
	STR_AllocTable(&ck_varTable, CK_VAR_MAXVARS);
	ck_varArena = MM_ArenaCreate(65536);
	MM_GetPtr((mm_ptr_t *)&ck_actionData, sizeof(CK_action) * CK_VAR_MAXACTIONS);
	ck_actionsUsed = 0;
}

void CK_VAR_SetEntry(const char *name, void *val)
{
	STR_AddEntry(ck_varTable, name, val);
}

void *CK_VAR_GetByName(const char *name, void *def)
{
	return STR_LookupEntryWithDefault(ck_varTable, name, def);
}

const char *CK_VAR_GetString(const char *name, const char *def)
{
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)CK_VAR_GetByName(name, NULL);
	if (!var)
	{
#ifdef CK_VAR_WARNONNOTSET
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "String variable \"%s\" not set, returning default \"%s\"\n", name, def);
#endif
		return def;
	}
	if (var->type != VAR_String)
		Quit("CK_VAR_GetString: Tried to access a non-string variable!");
	return (const char *)var->value;
#else
	return (const char *)CK_VAR_GetByName(name, (void *)def);
#endif
}

const char *CK_VAR_GetStringByNameAndIndex(const char *name, int index)
{
	char fullName[256];
	sprintf(fullName, "%s%d", name, index);
	return CK_VAR_GetString(fullName, name);
}

intptr_t CK_VAR_GetInt(const char *name, intptr_t def)
{
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)CK_VAR_GetByName(name, NULL);
	if (!var)
	{
#ifdef CK_VAR_WARNONNOTSET
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Integer variable \"%s\" not set, returning default %ld (0x%lX)\n", name, def, def);
#endif
		return def;
	}
	if (var->type != VAR_Int)
		Quit("CK_VAR_GetInt: Tried to access a non-integer variable!");
	return (intptr_t)var->value;
#else
	return (intptr_t)CK_VAR_GetByName(name, (void *)def);
#endif
}

CK_action *CK_GetActionByName(const char *name)
{
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)CK_VAR_GetByName(name, NULL);
	if (!var)
	{
#ifdef CK_VAR_WARNONNOTSET
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Action \"%s\" not set, returning NULL\n", name);
#endif
		return NULL;
	}
	if (var->type != VAR_Action)
		Quit("CK_GetActionByName: Tried to access a non-action variable!");
	return (CK_action *)var->value;
#else
	return (CK_action *)CK_VAR_GetByName(name, NULL);
#endif
}

CK_action *CK_GetOrCreateActionByName(const char *name)
{
	CK_action *ptr = NULL;
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)CK_VAR_GetByName(name, NULL);
	if (var)
	{
		if (var->type != VAR_Action)
			Quit("CK_GetOrCreateActionByName: Variable already exists with non-action type!");
		ptr = (CK_action *)var->value;
	}
#else
	ptr = (CK_action *)STR_LookupEntry(ck_varTable, name);
#endif
	if (!ptr)
	{
		if (ck_actionsUsed >= CK_VAR_MAXACTIONS)
			Quit("Too many actions!");
		ptr = &(ck_actionData[ck_actionsUsed++]);
		char *dupName = MM_ArenaStrDup(ck_varArena, name);
#ifdef CK_VAR_TYPECHECK
		CK_VAR_Variable *var = (CK_VAR_Variable *)MM_ArenaAlloc(ck_varArena, sizeof(*var));
		var->type = VAR_Action;
		var->value = (void *)ptr;
		CK_VAR_SetEntry(dupName, (void *)var);
#else
		CK_VAR_SetEntry(dupName, (void *)ptr);
#endif
	}
	return ptr;
}

// POTENTIALLY SLOW function - Use in game loading only!
CK_action *CK_LookupActionFrom16BitOffset(uint16_t offset)
{
	for (int i = 0; i < ck_actionsUsed; ++i)
		if (ck_actionData[i].compatDosPointer == offset)
			return &ck_actionData[i];

	return NULL;
}

void CK_VAR_SetInt(const char *name, intptr_t val)
{
	const char *realName = MM_ArenaStrDup(ck_varArena, name);
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)MM_ArenaAlloc(ck_varArena, sizeof(*var));
	var->type = VAR_Int;
	var->value = (void *)val;
	CK_VAR_SetEntry(realName, (void *)var);
#else
	CK_VAR_SetEntry(realName, (void *)val);
#endif
}

void CK_VAR_SetString(const char *name, const char *val)
{
	const char *realName = MM_ArenaStrDup(ck_varArena, name);
	const char *realVal = MM_ArenaStrDup(ck_varArena, val);
#ifdef CK_VAR_TYPECHECK
	CK_VAR_Variable *var = (CK_VAR_Variable *)MM_ArenaAlloc(ck_varArena, sizeof(*var));
	var->type = VAR_String;
	var->value = (void *)realVal;
	CK_VAR_SetEntry(realName, (void *)var);
#else
	CK_VAR_SetEntry(realName, (void *)realVal);
#endif
}

// == Parser ===

CK_VAR_VarType CK_VAR_ParseVarType(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);
	CK_VAR_VarType varType = VAR_Invalid;
	if (tok.tokenType == STR_TOK_EOF)
	{
		return VAR_EOF;
	}
	if (tok.tokenType != STR_TOK_Ident)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Got a non-ident token reading var type (line %d), value = %s\n", ps->linecount, tok.valuePtr);
		return VAR_Invalid;
	}
	if (STR_IsTokenIdent(tok, "%bool"))
		varType = VAR_Bool;
	else if (STR_IsTokenIdent(tok, "%int"))
		varType = VAR_Int;
	else if (STR_IsTokenIdent(tok, "%string"))
		varType = VAR_String;
	else if (STR_IsTokenIdent(tok, "%action"))
		varType = VAR_Action;
	else if (STR_IsTokenIdent(tok, "%include"))
		varType = VAR_TOK_Include;
	else
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Unknown var type \"%s\"\n", tok.valuePtr);
	}

	return varType;
}

CK_ActionType CK_ACT_GetActionType(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);
	CK_ActionType at = AT_UnscaledOnce;

	if (STR_IsTokenIdent(tok, "UnscaledOnce"))
		at = AT_UnscaledOnce;
	else if (STR_IsTokenIdent(tok, "ScaledOnce"))
		at = AT_ScaledOnce;
	else if (STR_IsTokenIdent(tok, "Frame"))
		at = AT_Frame;
	else if (STR_IsTokenIdent(tok, "UnscaledFrame"))
		at = AT_UnscaledFrame;
	else if (STR_IsTokenIdent(tok, "ScaledFrame"))
		at = AT_ScaledFrame;
	else
	{
		char varTypeString[ID_STR_MAX_TOKEN_LENGTH];
		STR_GetStringValue(tok, varTypeString, ID_STR_MAX_TOKEN_LENGTH);
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Got a bad action type %s on line %d.\n", varTypeString, ps->linecount);
		at = (CK_ActionType)(STR_GetIntegerValue(tok));
	}
	return at;
}

intptr_t CK_VAR_ParseIntOrVar(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);
	if (*tok.valuePtr == '@')
	{
		// This is an indirectly loaded integer.
		char varName[ID_STR_MAX_TOKEN_LENGTH];
		STR_GetStringValue(tok, varName, ID_STR_MAX_TOKEN_LENGTH);
		
		// Now, check if it exists. We have to do this separately for integers,
		// as we can't just use NULL as a default value.
		if (!STR_DoesEntryExist(ck_varTable, varName+1))
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Couldn't resolve integer variable reference %s on line %d.\n", varName, ps->linecount);
		
		return CK_VAR_GetInt(varName+1, 0);
	}
	else
		return STR_GetIntegerValue(tok);
}

bool CK_VAR_ParseAction(STR_ParserState *ps)
{
	char actName[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetIdent(ps, actName, ID_STR_MAX_TOKEN_LENGTH);

	CK_action *act = CK_GetOrCreateActionByName(actName);

	act->compatDosPointer = CK_VAR_ParseIntOrVar(ps);

	act->chunkLeft = CK_VAR_ParseIntOrVar(ps);

	act->chunkRight = CK_VAR_ParseIntOrVar(ps);

	act->type = CK_ACT_GetActionType(ps);

	act->protectAnimation = CK_VAR_ParseIntOrVar(ps);

	act->stickToGround = CK_VAR_ParseIntOrVar(ps);

	act->timer = CK_VAR_ParseIntOrVar(ps);

	act->velX = CK_VAR_ParseIntOrVar(ps);
	act->velY = CK_VAR_ParseIntOrVar(ps);

	char cThink[ID_STR_MAX_TOKEN_LENGTH];
	char cCollide[ID_STR_MAX_TOKEN_LENGTH];
	char cDraw[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetIdent(ps, cThink, sizeof(cThink));
	STR_GetIdent(ps, cCollide, sizeof(cCollide));
	STR_GetIdent(ps, cDraw, sizeof(cDraw));

#ifdef CK_VAR_FUNCTIONS_AS_STRINGS
	act->think = MM_ArenaStrDup(ck_varArena, cThink);
	act->collide = MM_ArenaStrDup(ck_varArena, cCollide);
	act->draw = MM_ArenaStrDup(ck_varArena, cDraw);
#else
	act->think = CK_ACT_GetFunction(cThink);
	act->collide = CK_ACT_GetColFunction(cCollide);
	act->draw = CK_ACT_GetFunction(cDraw);
#endif

	char nextActionName[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetIdent(ps, nextActionName, ID_STR_MAX_TOKEN_LENGTH);

#ifdef CK_VAR_FUNCTIONS_AS_STRINGS
	act->next = MM_ArenaStrDup(ck_varArena, nextActionName);
#else
	act->next = strcmp(nextActionName, "NULL") ? CK_GetOrCreateActionByName(nextActionName) : 0;
#endif

	return true;
}

void CK_VAR_ParseInt(STR_ParserState *ps)
{
	char varName[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetIdent(ps, varName, ID_STR_MAX_TOKEN_LENGTH);
	intptr_t val = CK_VAR_ParseIntOrVar(ps);
	CK_VAR_SetInt(varName, val);
}

void CK_VAR_ParseString(STR_ParserState *ps)
{
	char varName[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetIdent(ps, varName, ID_STR_MAX_TOKEN_LENGTH);
	char stringBuf[4096] = {0};
	char *valBuf = stringBuf;
	size_t lengthRemaining = 4096;
	do
	{
		STR_Token tok = STR_GetToken(ps);
		size_t chunkLen = STR_GetStringValue(tok, valBuf, lengthRemaining);
		lengthRemaining -= chunkLen;
		valBuf += chunkLen;
		if (!lengthRemaining)
			Quit("String too long!");
	} while (STR_PeekToken(ps).tokenType == STR_TOK_String);
	CK_VAR_SetString(varName, stringBuf);
}

bool CK_VAR_ParseVar(STR_ParserState *ps)
{
	CK_VAR_VarType varType = CK_VAR_ParseVarType(ps);

	if (varType == VAR_EOF)
	{
		return false;
	}

	switch (varType)
	{
	case VAR_Int:
		CK_VAR_ParseInt(ps);
		break;
	case VAR_String:
		CK_VAR_ParseString(ps);
		break;
	case VAR_Action:
		CK_VAR_ParseAction(ps);
		break;
	case VAR_TOK_Include:
	{
		char filename[ID_STR_MAX_TOKEN_LENGTH];
		STR_GetString(ps, filename, ID_STR_MAX_TOKEN_LENGTH);
		CK_VAR_LoadVars(filename);
		break;
	}
	default:
		Quit("Unsupported var type.");
	}

	return true;
}

void CK_VAR_LoadVars(const char *filename)
{
	int numVarsParsed = 0;
	STR_ParserState parserstate;

	CA_LoadFile(filename, (mm_ptr_t *)(&parserstate.data), &(parserstate.datasize));
	parserstate.dataindex = 0;
	parserstate.linecount = 0;
	parserstate.haveBufferedToken = false;

	while (CK_VAR_ParseVar(&parserstate))
		numVarsParsed++;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Parsed %d vars from \"%s\" over %d lines (%d actions created).\n", numVarsParsed, filename, parserstate.linecount, ck_actionsUsed);
}
