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
#include "ck_cross.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* String manager, allows objects to be indexed by strings */

// Hash a string using an xor variant of the djb2 hash
static unsigned int STR_HashString(const char *str)
{
	int slen = strlen(str);
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

// Returns the pointer associated with 'str' in 'tabl'
void *STR_LookupEntry(STR_Table *tabl, const char *str)
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
	return (void *)(0);
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

const char *STR_GetToken(STR_ParserState *ps)
{
	char tokenbuf[ID_STR_MAX_TOKEN_LENGTH];
	int i = 0;
	STR_SkipWhitespace(ps);
	while (STR_PeekCharacter(ps) && !isspace(STR_PeekCharacter(ps)))
		tokenbuf[i++] = STR_GetCharacter(ps);
	tokenbuf[i] = '\0';
	return MM_ArenaStrDup(ps->tempArena, tokenbuf);
}

int STR_GetInteger(STR_ParserState *ps)
{
	const char *token = STR_GetToken(ps);
	int result = 0;

	/* strtol does not support the '$' prefix for hex */
	if (token[0] == '$')
		result = strtol(token + 1, 0, 16);
	else
		result = strtol(token, 0, 0);
	//printf("GetInteger %s -> %d\n", token, result);
	return result;
}

bool STR_ExpectToken(STR_ParserState *ps, const char *str)
{
	const char *c = STR_GetToken(ps);
	bool result = !strcmp(c, str);
	if (!result)
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "ExpectToken, got \"%s\" expected \"%s\" on line %d\n", c, str, ps->linecount);
	return result;
}

