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

#ifndef ID_CFG_H
#define ID_CFG_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CFG_Variable
{
	const char *name;
	const char *str_value;
	int int_value;
	bool saved;
} CFG_Variable;

void CFG_LoadConfig(const char *filename);
void CFG_SaveConfig(const char *filename);

bool CFG_ConfigExists(const char *name);
int CFG_GetConfigInt(const char *name, int defValue);
const char *CFG_GetConfigString(const char *name, const char *defValue);

void CFG_SetConfigInt(const char *name, int value);
void CFG_SetConfigString(const char *name, const char *value);

void CFG_Startup();
void CFG_Shutdown();

#endif
