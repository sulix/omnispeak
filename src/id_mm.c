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

// ID_MM: The Memory Manager
// The Memory Manager provides a system for allocating and deallocating
// memory. Memory blocks can be moved and removed when not in use to free
// up memory needed.
//
// Memory blocks are allocated returned double-indirect pointers, and these
// pointers are updated when a block's address changes.
//

#include "id_mm.h"
#include "id_us.h"
#include "ck_cross.h"

#include <stdlib.h>
#include <string.h>

mm_ptr_t buffer; // Misc buffer

#define MM_MAXBLOCKS 2048 //1200 in Keen5

typedef struct ID_MM_MemBlock
{
	void *ptr;
	int length;
	mm_ptr_t *userptr;
	bool locked;
	int purgelevel;
	struct ID_MM_MemBlock *next;
} ID_MM_MemBlock;

static ID_MM_MemBlock mm_blocks[MM_MAXBLOCKS];

static ID_MM_MemBlock *mm_free, *mm_purgeable;

static int mm_blocksused;
static int mm_numpurgeable;
static int mm_memused;

static void MML_ClearBlock()
{
	ID_MM_MemBlock *bestBlock = 0;
	for (int i = 0; i < MM_MAXBLOCKS; ++i)
	{
		//Is the block free?
		if (!mm_blocks[i].ptr)
			continue;
		//Can we purge it?
		if (!mm_blocks[i].purgelevel)
			continue;
		//Is it locked?
		if (mm_blocks[i].locked)
			continue;

		if (!bestBlock || mm_blocks[i].purgelevel > bestBlock->purgelevel)
		{
			bestBlock = &(mm_blocks[i]);
			continue;
		}
	}

	//Did we find a purgable block?
	if (!bestBlock)
		Quit("MML_ClearBlock(): No purgable blocks!");

	if (bestBlock->purgelevel < 3)
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "MM_ClearBlock(): Purging a block with purgelevel %d\n", bestBlock->purgelevel);

	//Free the sucker.
	MM_FreePtr(bestBlock->userptr);
}

static ID_MM_MemBlock *MML_GetNewBlock()
{
	ID_MM_MemBlock *newBlock;
	//If there aren't any free blocks, kick out some purgables.
	if (!mm_free)
		MML_ClearBlock();
	newBlock = mm_free;
	mm_free = mm_free->next;
	return newBlock;
}

static void MML_UpdateUserPointer(ID_MM_MemBlock *blk)
{
	(*blk->userptr) = blk->ptr;
}

static ID_MM_MemBlock *MML_BlockFromUserPointer(mm_ptr_t *ptr)
{
	for (int i = 0; i < MM_MAXBLOCKS; ++i)
		if (mm_blocks[i].userptr == ptr)
			return &(mm_blocks[i]);

	return (ID_MM_MemBlock *)(0);
}

void MM_Startup(void)
{
	//NOP
	for (int i = MM_MAXBLOCKS - 1; i >= 0; --i)
	{
		mm_blocks[i].ptr = 0;
		mm_blocks[i].next = (i == MM_MAXBLOCKS - 1) ? 0 : &(mm_blocks[i + 1]);
	}
	mm_free = &(mm_blocks[0]);
	mm_purgeable = 0;
	// Misc buffer
	MM_GetPtr(&buffer, BUFFERSIZE);

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "MM_Startup: %d blocks, %d KB Buffer\n", MM_MAXBLOCKS, BUFFERSIZE / 1024);
}

void MM_Shutdown(void)
{
	for (int i = 0; i < MM_MAXBLOCKS; ++i)
	{
		if (mm_blocks[i].ptr)
			free(mm_blocks[i].ptr);
	}
}

void MM_GetPtr(mm_ptr_t *ptr, unsigned long size)
{
	ID_MM_MemBlock *blk = MML_GetNewBlock();
	//Try to allocate memory, freeing if we can't.
	do
	{
		blk->ptr = malloc(size);
		if (size && !blk->ptr)
		{
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "MM_GetPtr: Failed to alloc block (%lu bytes) with system malloc. Trying to free some space.\n", size);
			if (mm_numpurgeable)
				MML_ClearBlock();
			else
				Quit("MM_GetPtr: Out of Memory!");
		}
	} while (size && !blk->ptr);

	//Setup the block details (unlocked, non-purgable)
	blk->length = size;
	blk->userptr = ptr;
	blk->purgelevel = 0;
	blk->locked = false;

	//Update the stats
	mm_blocksused++;
	mm_memused += size;

	MML_UpdateUserPointer(blk);
}

void MM_FreePtr(mm_ptr_t *ptr)
{
	//Lookup the block
	ID_MM_MemBlock *blk = MML_BlockFromUserPointer(ptr);
	if (!blk)
		Quit("MM_FreePtr: Block not Found!");

	//Update the purgeable count.
	if (blk->purgelevel)
		mm_numpurgeable--;

	//Update the used memory counts.
	mm_blocksused--;
	mm_memused -= blk->length;

	//Add it to the free list
	blk->next = mm_free;
	mm_free = blk;

	//Free its memory.
	free(blk->ptr);
	blk->length = 0;
	blk->ptr = 0;
	blk->userptr = 0;

	// Set the pointer to NULL
	// This is important, as some CA functions check ptr != NULL to see if
	// things are loaded. Get this wrong and it will try to SetPurge on
	// nonexistant blocks. Not fun!
	*ptr = 0;
}

void MM_SetPurge(mm_ptr_t *ptr, int level)
{
	//Lookup the block
	ID_MM_MemBlock *blk = MML_BlockFromUserPointer(ptr);
	if (!blk)
		Quit("MM_SetPurge: Block not Found!");

	//Keep track of the purgeable block count.
	if (!blk->purgelevel && level)
		mm_numpurgeable++;
	else if (blk->purgelevel && !level)
		mm_numpurgeable--;

	//Set the purge level
	blk->purgelevel = level;
}

void MM_SetLock(mm_ptr_t *ptr, bool lock)
{
	//Lookup the block
	ID_MM_MemBlock *blk = MML_BlockFromUserPointer(ptr);
	if (!blk)
		Quit("MM_SetLock: Block not Found!");

	//Lock/Unlock the block
	blk->locked = lock;
}

int MM_UsedMemory()
{
	return mm_memused;
}

int MM_UsedBlocks()
{
	return mm_blocksused;
}

int MM_PurgableBlocks()
{
	return mm_numpurgeable;
}

void MM_SortMem()
{
	//We're not actually sorting memory, as we just handball
	//allocation over to the system at the moment.
	//All we'll do is purge _all_ purgable blocks.

	//NOTE: Keen locks the currently playing music at this point.
	//We will ignore this, as no sound is implemented.
	for (int i = 0; i < MM_MAXBLOCKS; ++i)
	{
		if (mm_blocks[i].ptr && mm_blocks[i].purgelevel && !mm_blocks[i].locked)
			MM_FreePtr(mm_blocks[i].userptr);
	}
}

void MM_ShowMemory()
{
	//TODO: This is a stub. I should at least add a stats dump here.
}

void MM_BombOnError(bool bomb)
{
	//TODO: Add support for this here.
}

//NOTE: Keen/Wolf3d have MML_UseSpace. This is incompatible with our use of the
//system allocator, so it is unused for now.

#ifndef ID_MM_DEBUGARENA

struct ID_MM_Arena
{
	size_t size;
	size_t currentOffset;
	// TODO: Add flags/mutex for multithreading?
};

ID_MM_Arena *MM_ArenaCreate(size_t size)
{
	if (size < sizeof(ID_MM_Arena))
		Quit("Tried to create an arena which was too small.");
	uint8_t *memblk = (uint8_t *)malloc(size);
	if (!memblk)
		return 0;

	ID_MM_Arena *arena = (ID_MM_Arena *)memblk;
	arena->size = size;
	arena->currentOffset = sizeof(ID_MM_Arena);

	return arena;
}

void *MM_ArenaAlloc(ID_MM_Arena *arena, size_t size)
{
	if (arena->currentOffset + size >= arena->size)
	{
		Quit("Arena is full!");
	}

	uint8_t *ptr = ((uint8_t *)arena) + arena->currentOffset;
	arena->currentOffset += size;
	return ptr;
}

void *MM_ArenaAllocAligned(ID_MM_Arena *arena, size_t size, size_t alignment)
{
	if (alignment & (alignment - 1))
	{
		Quit("MM_ArenaAllocAligned: Alignment is not a power of two!");
	}

	// Align the current offset.
	arena->currentOffset += alignment;
	arena->currentOffset &= ~(alignment-1);

	return MM_ArenaAlloc(arena, size);
}

char *MM_ArenaStrDup(ID_MM_Arena *arena, const char *str)
{
	size_t len = strlen(str) + 1;
	char *newStr = (char *)MM_ArenaAlloc(arena, len);
	memcpy(newStr, str, len);
	newStr[len - 1] = '\0';
	return newStr;
}

void MM_ArenaReset(ID_MM_Arena *arena)
{
	arena->currentOffset = sizeof(ID_MM_Arena);
}

void MM_ArenaDestroy(ID_MM_Arena *arena)
{
	arena->currentOffset = 0;
	arena->size = 0;
	free(arena);
}

#else

// For debug builds, the ID_MM_Arena is a linked list of allocation records.
// This allows tools like valgrind to check things.
struct ID_MM_Arena
{
	void *data;
	size_t size;
	struct ID_MM_Arena *next;
	// the following are valid only in the first element.
	size_t arena_size;
	size_t arena_used;
};

ID_MM_Arena *MM_ArenaCreate(size_t size)
{
	ID_MM_Arena *arena = (ID_MM_Arena *)malloc(sizeof(ID_MM_Arena));
	arena->arena_size = size;
	arena->arena_used = 0;
	arena->data = 0;
	arena->size = 0;
	arena->next = 0;
	return arena;
}

void *MM_ArenaAlloc(ID_MM_Arena *arena, size_t size)
{
	ID_MM_Arena *lastBlock = arena;
	if (arena->arena_used + size >= arena->arena_size)
		Quit("Arena is full!");

	while (lastBlock->next)
		lastBlock = lastBlock->next;

	// Allocate and record the block
	lastBlock->data = malloc(size);
	lastBlock->size = size;
	// Add a new tail element.
	lastBlock->next = (ID_MM_Arena *)malloc(sizeof(ID_MM_Arena));
	lastBlock->next->next = 0;

	return lastBlock->data;
}

void *MM_ArenaAllocAligned(ID_MM_Arena *arena, size_t size, size_t alignment)
{
	//TODO: Actually align, though malloc() is usually okay.
	return MM_ArenaAlloc(arena, size);
}

char *MM_ArenaStrDup(ID_MM_Arena *arena, const char *str)
{
	size_t len = strlen(str) + 1;
	char *newStr = (char *)MM_ArenaAlloc(arena, len);
	strcpy(newStr, str);
	newStr[len - 1] = '\0';
	return newStr;
}

void MM_ArenaReset(ID_MM_Arena *arena)
{
	ID_MM_Arena *block = arena->next;
	while (block)
	{
		ID_MM_Arena *nextBlock = block->next;
		if (nextBlock)
			free(block->data);
		free(block);
		block = nextBlock;
	}
	if (arena->next)
		free(arena->data);
	arena->arena_used = 0;
	arena->next = 0;
}

void MM_ArenaDestroy(ID_MM_Arena *arena)
{
	MM_ArenaReset(arena);
	free(arena);
}

#endif
