#ifndef ID_MM_H
#define ID_MM_H

#include <stdbool.h>
#include <stdint.h>

typedef void *mm_ptr_t;

void MM_Startup (void);
void MM_Shutdown (void);
void MM_MapEMS (void);

void MM_GetPtr (mm_ptr_t *baseptr,unsigned long size);
void MM_FreePtr (mm_ptr_t *baseptr);

void MM_SetPurge (mm_ptr_t *baseptr, int purge);
void MM_SetLock (mm_ptr_t *baseptr, bool locked);
void MM_SortMem (void);

void MM_ShowMemory (void);

int MM_UsedMemory();
int MM_UsedBlocks();
int MM_PurgableBlocks();
long MM_UnusedMemory (void);
long MM_TotalFree (void);

void MM_BombOnError (bool bomb);

void MML_UseSpace (void *off, intptr_t len);


#endif
