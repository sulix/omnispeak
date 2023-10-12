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

#ifndef ID_MM_H
#define ID_MM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BUFFERSIZE 0x1000 // Miscellaneous, always available buffer

typedef void *mm_ptr_t;

extern mm_ptr_t buffer; // Misc buffer

void MM_Startup(void);
void MM_Shutdown(void);
void MM_MapEMS(void);

void MM_GetPtr(mm_ptr_t *baseptr, unsigned long size);
void MM_FreePtr(mm_ptr_t *baseptr);

void MM_SetPurge(mm_ptr_t *baseptr, int purge);
void MM_SetLock(mm_ptr_t *baseptr, bool locked);
void MM_SortMem(void);

void MM_ShowMemory(void);

int MM_UsedMemory();
int MM_UsedBlocks();
int MM_PurgableBlocks();
long MM_UnusedMemory(void);
long MM_TotalFree(void);

void MM_BombOnError(bool bomb);

void MML_UseSpace(void *off, intptr_t len);

typedef struct ID_MM_Arena ID_MM_Arena;
ID_MM_Arena *MM_ArenaCreate(size_t size);
void *MM_ArenaAlloc(ID_MM_Arena *arena, size_t size);
void *MM_ArenaAllocAligned(ID_MM_Arena *arena, size_t size, size_t alignment);
char *MM_ArenaStrDup(ID_MM_Arena *arena, const char *str);
void MM_ArenaReset(ID_MM_Arena *arena);
void MM_ArenaDestroy(ID_MM_Arena *arena);

#endif
