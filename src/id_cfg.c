/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2021 Omnispeak Authors

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

#include "id_cfg.h"
#include "id_ca.h"
#include "id_fs.h"
#include "id_str.h"
#include "ck_cross.h"

ID_MM_Arena *cfg_configArena;
STR_Table *cfg_configEntries;
bool cfg_started = false;

bool CFG_ConfigExists(const char *name)
{
	return STR_LookupEntry(cfg_configEntries, name);
}

int CFG_GetConfigInt(const char *name, int defValue)
{
	CFG_Variable *var = (CFG_Variable *)STR_LookupEntry(cfg_configEntries, name);
	if (!var)
		return defValue;
	return var->int_value;
}

const char *CFG_GetConfigString(const char *name, const char *defValue)
{
	CFG_Variable *var = (CFG_Variable *)STR_LookupEntry(cfg_configEntries, name);
	if (!var)
		return defValue;
	return var->str_value;
}

bool CFG_GetConfigBool(const char *name, bool defValue)
{
	return CFG_GetConfigInt(name, defValue ? 1 : 0);
}

int CFG_GetConfigEnum(const char *name, const char **strings, int defValue)
{
	const char *valueStr = CFG_GetConfigString(name, NULL);
	if (!valueStr)
		return defValue;
	
	for (int i = 0; strings[i]; ++i)
	{
		if (!CK_Cross_strcasecmp(strings[i], valueStr))
			return i;
	}
	return defValue;
}

// Gets or creates a config variable. When created, the name is StrDup'ed into the
// config arena.
static CFG_Variable *CFG_GetOrCreateVariable(const char *name)
{
	CFG_Variable *var = (CFG_Variable *)STR_LookupEntry(cfg_configEntries, name);
	if (var)
		return var;

	var = (CFG_Variable *)MM_ArenaAlloc(cfg_configArena, sizeof(CFG_Variable));
	const char *allocedName = MM_ArenaStrDup(cfg_configArena, name);
	var->name = allocedName;

	// We need to initialise this.
	var->str_value = 0;

	// Insert it into the hashtable
	STR_AddEntry(cfg_configEntries, allocedName, var);
	return var;
}

void CFG_SetConfigInt(const char *name, int value)
{
	CFG_Variable *var = CFG_GetOrCreateVariable(name);
	var->str_value = 0;
	var->int_value = value;
	var->is_boolean = false;
}

void CFG_SetConfigString(const char *name, const char *value)
{
	CFG_Variable *var = CFG_GetOrCreateVariable(name);

	// We don't want to re-allocate the value if it's the same.
	if (!var->str_value || strcmp(value, var->str_value))
		var->str_value = MM_ArenaStrDup(cfg_configArena, value);
	var->int_value = 0;
}

void CFG_SetConfigBool(const char *name, bool value)
{
	CFG_Variable *var = CFG_GetOrCreateVariable(name);

	var->str_value = 0;
	var->int_value = value ? 1 : 0;
	var->is_boolean = true;
}

void CFG_SetConfigEnum(const char *name, const char **strings, int value)
{
	CFG_SetConfigString(name, strings[value]);
}

static bool CFG_ParseConfigLine(STR_ParserState *state)
{
	STR_Token nameTok = STR_GetToken(state);
	if (nameTok.tokenType == STR_TOK_EOF)
		return false;
	char name[ID_STR_MAX_TOKEN_LENGTH];
	STR_GetStringValue(nameTok, name, ID_STR_MAX_TOKEN_LENGTH);
	if (!STR_ExpectToken(state, "="))
		return false;
	STR_Token value = STR_GetToken(state);
	if (value.tokenType == STR_TOK_EOF)
		return false;
	else if (value.tokenType == STR_TOK_String)
	{
		char tokenBuf[ID_STR_MAX_TOKEN_LENGTH];
		STR_GetStringValue(value, tokenBuf, ID_STR_MAX_TOKEN_LENGTH);
		CFG_SetConfigString(name, tokenBuf);
	}
	else if (STR_IsTokenIdentCase(value, "false"))
		CFG_SetConfigBool(name, false);
	else if (STR_IsTokenIdentCase(value, "true"))
		CFG_SetConfigBool(name, true);
	else
		CFG_SetConfigInt(name, STR_GetIntegerValue(value));
	return true;
}

void CFG_LoadConfig(const char *filename)
{
	int numVarsParsed = 0;
	STR_ParserState parserstate;

	FS_LoadUserFile(filename, (mm_ptr_t *)(&parserstate.data), &(parserstate.datasize));
	if (!parserstate.data)
		return;

	parserstate.dataindex = 0;
	parserstate.linecount = 0;
	parserstate.haveBufferedToken = false;

	while (CFG_ParseConfigLine(&parserstate))
		numVarsParsed++;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Parsed %d config options from \"%s\" over %d lines\n", numVarsParsed, filename, parserstate.linecount);

	MM_FreePtr((mm_ptr_t *)&parserstate.data);
}

static int cfg_unmodifiedRangeStart = 0;
static int cfg_unmodifiedRangeEnd = 0;

static void CFG_FlushUnmodifiedRange(FS_File outputHandle, STR_ParserState *state)
{
	FS_Write(&state->data[cfg_unmodifiedRangeStart], cfg_unmodifiedRangeEnd - cfg_unmodifiedRangeStart, 1, outputHandle);
	cfg_unmodifiedRangeStart = cfg_unmodifiedRangeEnd;
}

static bool CFG_SaveConfigLine(FS_File outputHandle, STR_ParserState *state)
{
	// We do some funky stuff here to handle corrupt files as well as
	// we can:
	// We update the "unmodified range" so that its end is the beginning
	// of the name (which is an identifier).
	//
	char name[ID_STR_MAX_TOKEN_LENGTH];
	STR_Token nameTok = STR_GetToken(state);
	cfg_unmodifiedRangeEnd = nameTok.firstIndex;
	if (nameTok.tokenType == STR_TOK_EOF)
		return false;
	STR_GetStringValue(nameTok, name, ID_STR_MAX_TOKEN_LENGTH);
	if (!STR_ExpectToken(state, "="))
	{
		// If we're at the end of the file, the unmodified range should
		// end at the file's end.
		cfg_unmodifiedRangeEnd = state->datasize;
		return false;
	}
	STR_Token value = STR_PeekToken(state);
	if (value.tokenType == STR_TOK_EOF)
	{
		// If we're at the end of the file, the unmodified range should
		// end at the file's end.
		cfg_unmodifiedRangeEnd = state->datasize;
		return false;
	}

	cfg_unmodifiedRangeEnd = value.firstIndex;
	CFG_FlushUnmodifiedRange(outputHandle, state);

	CFG_Variable *var = (CFG_Variable *)STR_LookupEntry(cfg_configEntries, name);
	if (!var)
	{
		// Just flush the old value out.
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Unexpected entry in config file: \"%s\". Has the config changed since the game was started?", name);
		cfg_unmodifiedRangeStart = value.firstIndex;
		cfg_unmodifiedRangeEnd = value.lastIndex;
		CFG_FlushUnmodifiedRange(outputHandle, state);
		// Eat the token.
		STR_GetToken(state);
	}
	else if (value.tokenType == STR_TOK_String)
	{
		// We don't mind re-writing out strings, so just eat the
		// old token.
		STR_GetToken(state);
		FS_PrintF(outputHandle, "\"%s\"", var->str_value);
		var->saved = true;
	}
	else
	{
		int oldValue = 0;
		if (!CK_Cross_strcasecmp(value.valuePtr, "false"))
		{
			oldValue = 0;
			// Eat the token.
			STR_GetToken(state);
		}
		else if (!CK_Cross_strcasecmp(value.valuePtr, "true"))
		{
			oldValue = 1;
			// Eat the token.
			STR_GetToken(state);
		}
		else
			oldValue = STR_GetInteger(state);


		if (oldValue == var->int_value)
		{
			// If the old value matches the new, mark it unmodified.
			cfg_unmodifiedRangeStart = value.firstIndex;
			cfg_unmodifiedRangeEnd = value.lastIndex;
			CFG_FlushUnmodifiedRange(outputHandle, state);
		}
		else
		{
			// Otherwise, if it's a bool, write it out as a bool, else use decimal
			if (var->is_boolean)
				FS_PrintF(outputHandle, "%s", var->int_value ? "true" : "false");
			else
				FS_PrintF(outputHandle, "%d", var->int_value);
		}
		var->saved = true;
	}

	cfg_unmodifiedRangeStart = cfg_unmodifiedRangeEnd = value.lastIndex;

	return true;
}

void CFG_SaveConfig(const char *filename)
{
	// Saving the config is actually quite (over) complicated. Here's how
	// it works:
	// 1. We mark all variables as unsaved.
	// 2. We re-parse the existing config file, keeping track of the last
	// "unmodified range". Basically, every byte that's not part of a
	// "value" is copied exactly as is. Values are re-inserted from the
	// loaded config. (The exception is integers, which are only
	// re-inserted if their value differs, in order to preserve the base.
	// We mark these variables as "saved" when doing so.
	// 3. We loop over all variables again, and append any unsaved ones
	// to the file.

	// Mark all variables unsaved.
	size_t currentVarIndex = 0;
	CFG_Variable *currentVar;
	while ((currentVar = (CFG_Variable *)STR_GetNextEntry(cfg_configEntries, &currentVarIndex)))
	{
		currentVar->saved = false;
	}

	int numVarsSaved = 0;
	// Load the old save file.
	STR_ParserState parserstate;

	FS_LoadUserFile(filename, (mm_ptr_t *)(&parserstate.data), &(parserstate.datasize));
	FS_File outputHandle = FS_CreateUserFile(filename);
	if (!outputHandle)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Unable to save config to \"%s\"\n", filename);
		if (parserstate.data)
			MM_FreePtr((mm_ptr_t *)&parserstate.data);
		return;
	}
	if (parserstate.data)
	{
		parserstate.dataindex = 0;
		parserstate.linecount = 0;
		parserstate.haveBufferedToken = false;

		// Go through the file, re-writing the values of all the config vars.
		while (CFG_SaveConfigLine(outputHandle, &parserstate))
			numVarsSaved++;

		CFG_FlushUnmodifiedRange(outputHandle, &parserstate);

		MM_FreePtr((mm_ptr_t *)&parserstate.data);
	}

	// Now, loop over the remaining variables and write them out.
	currentVarIndex = 0;
	while ((currentVar = (CFG_Variable *)STR_GetNextEntry(cfg_configEntries, &currentVarIndex)))
	{
		if (currentVar->saved)
			continue;

		if (currentVar->str_value)
			FS_PrintF(outputHandle, "%s = \"%s\"\n", currentVar->name, currentVar->str_value);
		else if (currentVar->is_boolean)
			FS_PrintF(outputHandle, "%s = %s\n", currentVar->name, currentVar->int_value ? "true" : "false");
		else
			FS_PrintF(outputHandle, "%s = %d\n", currentVar->name, currentVar->int_value);
	}

	FS_CloseFile(outputHandle);
}

void CFG_Startup()
{
	cfg_configArena = MM_ArenaCreate(4096);
	STR_AllocTable(&cfg_configEntries, 256);

	CFG_LoadConfig("OMNISPK.CFG");
	cfg_started = true;
}

void CFG_Shutdown()
{
	if (cfg_started)
	{
		CFG_SaveConfig("OMNISPK.CFG");

		MM_ArenaDestroy(cfg_configArena);
	}
	cfg_started = false;
}
