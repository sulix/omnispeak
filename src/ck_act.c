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

void CK_VAR_Startup()
{
	STR_AllocTable(&ck_varTable, CK_VAR_MAXVARS);
	ck_varArena = MM_ArenaCreate(16384);
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
	return (const char *)CK_VAR_GetByName(name, (void *)def);
}

const char *CK_VAR_GetStringByNameAndIndex(const char *name, int index)
{
	char fullName[256];
	sprintf(fullName, "%s%d", name, index);
	return CK_VAR_GetString(fullName, name);
}

intptr_t CK_VAR_GetInt(const char *name, intptr_t def)
{
	return (intptr_t)CK_VAR_GetByName(name, (void *)def);
}

CK_action *CK_GetActionByName(const char *name)
{
	return (CK_action *)CK_VAR_GetByName(name, (void *)0);
}

CK_action *CK_GetOrCreateActionByName(const char *name)
{
	CK_action *ptr = (CK_action *)STR_LookupEntry(ck_varTable, name);
	if (!ptr)
	{
		if (ck_actionsUsed >= CK_VAR_MAXACTIONS)
			Quit("Too many actions!");
		ptr = &(ck_actionData[ck_actionsUsed++]);
		char *dupName = MM_ArenaStrDup(ck_varArena, name);
		STR_AddEntry(ck_varTable, dupName, (void *)(ptr));
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
	CK_VAR_SetEntry(realName, (void *)val);
}

void CK_VAR_SetString(const char *name, const char *val)
{
	const char *realName = MM_ArenaStrDup(ck_varArena, name);
	const char *realVal = MM_ArenaStrDup(ck_varArena, val);
	CK_VAR_SetEntry(realName, (void *)realVal);
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
	if (!strcmp(tok.valuePtr, "%bool"))
		varType = VAR_Bool;
	else if (!strcmp(tok.valuePtr, "%int"))
		varType = VAR_Int;
	else if (!strcmp(tok.valuePtr, "%string"))
		varType = VAR_String;
	else if (!strcmp(tok.valuePtr, "%action"))
		varType = VAR_Action;
	else if (!strcmp(tok.valuePtr, "%include"))
		varType = VAR_TOK_Include;
	else
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Unknown var type \"%s\"\n", tok.valuePtr);
	}

	return varType;
}

CK_ActionType CK_ACT_GetActionType(STR_ParserState *ps)
{
	const char *tok = STR_GetIdent(ps);
	CK_ActionType at = AT_UnscaledOnce;

	if (!strcmp(tok, "UnscaledOnce"))
		at = AT_UnscaledOnce;
	else if (!strcmp(tok, "ScaledOnce"))
		at = AT_ScaledOnce;
	else if (!strcmp(tok, "Frame"))
		at = AT_Frame;
	else if (!strcmp(tok, "UnscaledFrame"))
		at = AT_UnscaledFrame;
	else if (!strcmp(tok, "ScaledFrame"))
		at = AT_ScaledFrame;
	else
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Got a bad action type %s on line %d.\n", tok, ps->linecount);
		at = (CK_ActionType)(atoi(tok));
	}
	return at;
}

bool CK_VAR_ParseAction(STR_ParserState *ps)
{
	const char *actName = STR_GetIdent(ps);

	CK_action *act = CK_GetOrCreateActionByName(actName);

	act->compatDosPointer = STR_GetInteger(ps);

	act->chunkLeft = STR_GetInteger(ps);

	act->chunkRight = STR_GetInteger(ps);

	act->type = CK_ACT_GetActionType(ps);

	act->protectAnimation = STR_GetInteger(ps);

	act->stickToGround = STR_GetInteger(ps);

	act->timer = STR_GetInteger(ps);

	act->velX = STR_GetInteger(ps);
	act->velY = STR_GetInteger(ps);

	const char *cThink, *cCollide, *cDraw;
	cThink = STR_GetIdent(ps);
	cCollide = STR_GetIdent(ps);
	cDraw = STR_GetIdent(ps);

	act->think = CK_ACT_GetFunction(cThink);
	act->collide = CK_ACT_GetColFunction(cCollide);
	act->draw = CK_ACT_GetFunction(cDraw);

	const char *nextActionName = STR_GetIdent(ps);

	act->next = strcmp(nextActionName, "NULL") ? CK_GetOrCreateActionByName(nextActionName) : 0;

	return true;
}

void CK_VAR_ParseInt(STR_ParserState *ps)
{
	const char *varName = STR_GetIdent(ps);
	intptr_t val = STR_GetInteger(ps);
	CK_VAR_SetInt(varName, val);
}

void CK_VAR_ParseString(STR_ParserState *ps)
{
	const char *varName = STR_GetIdent(ps);
	char stringBuf[4096] = {0};
	do
	{
		const char *val = STR_GetString(ps);
		strcat(stringBuf, val);
	} while (STR_PeekToken(ps).tokenType == STR_TOK_String);
	CK_VAR_SetString(varName, stringBuf);
}

bool CK_VAR_ParseVar(STR_ParserState *ps)
{
	CK_VAR_VarType varType = CK_VAR_ParseVarType(ps);

	if (varType == VAR_EOF)
	{
		//MM_ArenaReset(ps->tempArena);
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
		const char *filename = STR_GetString(ps);
		CK_VAR_LoadVars(filename);
		break;
	}
	default:
		Quit("Unsupported var type.");
	}

	//MM_ArenaReset(ps->tempArena);
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

	parserstate.tempArena = MM_ArenaCreate(4096);

	while (CK_VAR_ParseVar(&parserstate))
		numVarsParsed++;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Parsed %d vars from \"%s\" over %d lines (%d actions created).\n", numVarsParsed, filename, parserstate.linecount, ck_actionsUsed);

	MM_ArenaDestroy(parserstate.tempArena);
}
