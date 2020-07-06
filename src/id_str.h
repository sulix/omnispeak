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

#ifndef ID_STR_H
#define ID_STR_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "id_mm.h"
/* String manager, allows objects to be indexed by strings */

typedef struct STR_Entry
{
	const char *str;
	void *ptr;
} STR_Entry;

typedef struct STR_Table
{
	size_t size;
	STR_Entry arr[];
} STR_Table;

void STR_AllocTable(STR_Table **tabl, size_t size);
void *STR_LookupEntryWithDefault(STR_Table *tabl, const char *str, void *def);
void *STR_LookupEntry(STR_Table *tabl, const char *str);
bool STR_AddEntry(STR_Table *tabl, const char *str, void *value);

void *STR_GetNextEntry(STR_Table *tabl, size_t *index);

#define ID_STR_MAX_TOKEN_LENGTH 1024

typedef enum STR_TokenType
{
	STR_TOK_EOF,
	STR_TOK_Ident,
	STR_TOK_Number,
	STR_TOK_String
} STR_TokenType;

typedef struct STR_Token
{
	STR_TokenType tokenType;
	const char *valuePtr;
	int valueLength;

	// These are the first byte index in the input of the token, and the
	// last byte index, respectively. These are used for saving modified
	// files out.
	int firstIndex;
	int lastIndex;
} STR_Token;

typedef struct STR_ParserState
{
	char *data;
	int dataindex;
	int datasize;
	int linecount;
	ID_MM_Arena *tempArena;
	// This is a token whose value has been PeekToken()ed.
	bool haveBufferedToken;
	STR_Token bufferedToken;
} STR_ParserState;

STR_Token STR_GetToken(STR_ParserState *ps);
STR_Token STR_PeekToken(STR_ParserState *ps);
const char *STR_GetString(STR_ParserState *ps);
const char *STR_GetIdent(STR_ParserState *ps);
int STR_GetInteger(STR_ParserState *ps);
bool STR_ExpectToken(STR_ParserState *ps, const char *str);

#endif //ID_STR_H
