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
#include "id_mm.h"
#include "id_us.h"
#include "ck_cross.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* String manager, allows objects to be indexed by strings */

// Hash a string using an xor variant of the djb2 hash
static unsigned int STR_HashString(const char *str)
{
	size_t slen = strlen(str);
	unsigned int hash = 0;
	for (int i = 0; i < slen; ++i)
	{
		hash = ((hash << 5) + hash) ^ (unsigned int)(str[i]);
	}
	return hash;
}

// Allocate a table 'tabl' of size 'size'
void STR_AllocTable(STR_Table **tabl, size_t size)
{
	MM_GetPtr((mm_ptr_t *)(tabl), sizeof(STR_Table) + size * (sizeof(STR_Entry)));
	// Lock it in memory so that it doesn't get purged.
	MM_SetLock((mm_ptr_t *)(tabl), true);
	(*tabl)->size = size;
	for (size_t i = 0; i < size; ++i)
	{
		(*tabl)->arr[i].str = 0;
		(*tabl)->arr[i].ptr = 0;
	}
}

// Returns the pointer associated with 'str' in 'tabl', defaulting to 'def'
void *STR_LookupEntryWithDefault(STR_Table *tabl, const char *str, void *def)
{
	int hash = STR_HashString(str) % tabl->size;
	for (size_t i = hash;; i = (i + 1) % tabl->size)
	{
		if (tabl->arr[i].str == 0)
			break;
		if (strcmp(str, tabl->arr[i].str) == 0)
		{
			return (tabl->arr[i].ptr);
		}
	}
	return def;
}

// Returns the pointer associated with 'str' in 'tabl'
void *STR_LookupEntry(STR_Table *tabl, const char *str)
{
	return STR_LookupEntryWithDefault(tabl, str, (void *)(0));
}

// Add an entry 'str' with pointer 'value' to 'tabl'. Returns 'true' on success
bool STR_AddEntry(STR_Table *tabl, const char *str, void *value)
{
	int hash = STR_HashString(str) % tabl->size;
	for (size_t i = hash;; i = (i + 1) % tabl->size)
	{
		if (tabl->arr[i].str == 0)
		{
			tabl->arr[i].str = str;
			tabl->arr[i].ptr = value;
			return true;
		}
	}
	return false;
}

// Iterate through the entires in the hashtable. "index" will be updated afterwards.
void *STR_GetNextEntry(STR_Table *tabl, size_t *index)
{
	for (size_t i = *index;; i++)
	{
		if (i >= tabl->size)
		{
			// We're done.
			*index = 0;
			return (void *)0;
		}
		if (tabl->arr[i].str != 0)
		{
			*index = i + 1;
			return (tabl->arr[i].ptr);
		}
	}
}

static char STR_PeekCharacter(STR_ParserState *ps)
{
	if (ps->dataindex >= ps->datasize)
		return '\0';
	return ps->data[ps->dataindex];
}

static char STR_GetCharacter(STR_ParserState *ps)
{
	if (ps->dataindex >= ps->datasize)
		return '\0';
	char c = ps->data[ps->dataindex++];
	if (c == '\n')
		ps->linecount++;
	return c;
}

static void STR_SkipWhitespace(STR_ParserState *ps)
{
	char c;
	do
	{
		c = STR_PeekCharacter(ps);
		// Comments starting with '#' and ending with '\n'
		if (c == '#')
		{
			while (STR_PeekCharacter(ps) != '\n')
			{
				c = STR_GetCharacter(ps);
			}
			c = '\n';
		}
		else if (isspace(c))
			STR_GetCharacter(ps);
	} while (c && isspace(c));
}

STR_Token STR_GetToken(STR_ParserState *ps)
{
	// Return a buffered token if we have one.
	if (ps->haveBufferedToken)
	{
		ps->haveBufferedToken = false;
		return ps->bufferedToken;
	}
	char tokenbuf[ID_STR_MAX_TOKEN_LENGTH];
	int i = 0;
	STR_SkipWhitespace(ps);
	STR_Token tok;
	tok.tokenType = STR_TOK_EOF;
	tok.firstIndex = ps->dataindex;
	if (STR_PeekCharacter(ps) && STR_PeekCharacter(ps) == '"')
	{
		// This is a string.
		tok.tokenType = STR_TOK_String;
		STR_GetCharacter(ps);
		while (STR_PeekCharacter(ps) != '"')
		{
			char c = STR_GetCharacter(ps);
			if (c == '\\')
			{
				c = STR_GetCharacter(ps);
				switch (c)
				{
				case 'n':
					c = '\n';
					break;
				default:
					// c is now whatever was escaped (e.g. '\')
					break;
				}
			}
			tokenbuf[i++] = c;
			if (i == ID_STR_MAX_TOKEN_LENGTH)
				Quit("Token exceeded max length!");
		}
		STR_GetCharacter(ps);
	}
	else if (STR_PeekCharacter(ps))
		tok.tokenType = STR_TOK_Ident;
	{
		while (STR_PeekCharacter(ps) && !isspace(STR_PeekCharacter(ps)))
		{
			tokenbuf[i++] = STR_GetCharacter(ps);
			if (i == ID_STR_MAX_TOKEN_LENGTH)
				Quit("Token exceeded max length!");
		}
	}
	tok.lastIndex = ps->dataindex;
	tokenbuf[i] = '\0';

	// Copy the token into the temp Arena
	// TODO: Use a range into the input instead
	const char *value = MM_ArenaStrDup(ps->tempArena, tokenbuf);
	tok.valuePtr = value;
	tok.valueLength = i;
	return tok;
}

STR_Token STR_PeekToken(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);

	// Unget the token.
	ps->haveBufferedToken = true;
	ps->bufferedToken = tok;

	return tok;
}

const char *STR_GetString(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);
	//TODO: If we actually use valueLength, this will need reallocating to
	// add the NULL terminator.
	return tok.valuePtr;
}

const char *STR_GetIdent(STR_ParserState *ps)
{
	STR_Token tok = STR_GetToken(ps);
	//TODO: If we actually use valueLength, this will need reallocating to
	// add the NULL terminator.
	return tok.valuePtr;
}

int STR_GetInteger(STR_ParserState *ps)
{
	STR_Token token = STR_GetToken(ps);
	int result = 0;

	// NOTE: For the time being,
	if (token.tokenType != STR_TOK_Ident && token.tokenType != STR_TOK_Number)
		return 0;

	/* strtol does not support the '$' prefix for hex */
	if (token.valuePtr[0] == '$')
		result = strtol(token.valuePtr + 1, 0, 16);
	else
		result = strtol(token.valuePtr, 0, 0);
	//printf("GetInteger %s -> %d\n", token, result);
	return result;
}

bool STR_ExpectToken(STR_ParserState *ps, const char *str)
{
	STR_Token tok = STR_GetToken(ps);

	// An EOF is never what we expect.
	if (tok.tokenType == STR_TOK_EOF)
		return false;

	bool result = !strncmp(tok.valuePtr, str, tok.valueLength);
	//TODO: ValuePtr may not be NULL-terminated in the future.
	if (!result)
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "ExpectToken, got \"%s\" expected \"%s\" on line %d\n", tok.valuePtr, str, ps->linecount);
	return result;
}
