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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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
void* STR_LookupEntry(STR_Table *tabl, const char* str);
bool STR_AddEntry(STR_Table *tabl, const char *str, void *value);

#endif //ID_STR_H

