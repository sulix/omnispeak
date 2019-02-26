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

#define CK_ACT_MAXACTIONS 512

STR_Table *ck_actionTable;
CK_action *ck_actionData;
ID_MM_Arena *ck_actionStringArena;
int ck_actionsUsed;

void CK_ACT_SetupActionDB()
{
	MM_GetPtr((mm_ptr_t *)&ck_actionData, CK_ACT_MAXACTIONS * sizeof(CK_action));
	STR_AllocTable(&ck_actionTable, CK_ACT_MAXACTIONS);
	ck_actionStringArena = MM_ArenaCreate(16384);
}

CK_action *CK_GetActionByName(const char *name)
{
	return (CK_action *)STR_LookupEntry(ck_actionTable, name);
}

CK_action *CK_GetOrCreateActionByName(const char *name)
{
	CK_action *ptr = (CK_action *)STR_LookupEntry(ck_actionTable, name);
	if (!ptr)
	{
		ptr = &ck_actionData[ck_actionsUsed++];
		char *dupName = MM_ArenaStrDup(ck_actionStringArena, name);
		STR_AddEntry(ck_actionTable, dupName, (void *)(ptr));
	}
	return ptr;
}


CK_ActionType CK_ACT_GetActionType(STR_ParserState *ps)
{
	const char *tok = STR_GetToken(ps);
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

bool CK_ACT_ParseAction(STR_ParserState *ps)
{
	if (!STR_ExpectToken(ps, "%action"))
		return false;

	const char *actName = STR_GetToken(ps);

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
	cThink = STR_GetToken(ps);
	cCollide = STR_GetToken(ps);
	cDraw = STR_GetToken(ps);

	act->think = CK_ACT_GetFunction(cThink);
	act->collide = CK_ACT_GetColFunction(cCollide);
	act->draw = CK_ACT_GetFunction(cDraw);

	const char *nextActionName = STR_GetToken(ps);

	act->next = strcmp(nextActionName, "NULL") ? CK_GetOrCreateActionByName(nextActionName) : 0;

	MM_ArenaReset(ps->tempArena);

	return true;
}

// POTENTIALLY SLOW function - Use in game loading only!
CK_action *CK_LookupActionFrom16BitOffset(uint16_t offset)
{
	for (int i = 0; i < ck_actionsUsed; ++i)
		if (ck_actionData[i].compatDosPointer == offset)
			return &ck_actionData[i];

	return NULL;
}

void CK_ACT_LoadActions(const char *filename)
{
	ck_actionsUsed = 0;
	CK_ACT_SetupActionDB();
	int numActionsParsed = 0;
	STR_ParserState parserstate;

	CA_LoadFile(filename, (mm_ptr_t *)(&parserstate.data), &(parserstate.datasize));
	parserstate.dataindex = 0;
	parserstate.linecount = 0;

	parserstate.tempArena = MM_ArenaCreate(1024);

	while (CK_ACT_ParseAction(&parserstate))
		numActionsParsed++;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Parsed %d actions over %d lines.\n", numActionsParsed, parserstate.linecount);

	MM_ArenaDestroy(parserstate.tempArena);
}
