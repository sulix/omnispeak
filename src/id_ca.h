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

#ifndef ID_CA_H
#define ID_CA_H

#include <stdbool.h>
#include <stdint.h>

#include "ck_cross.h"

#include "id_mm.h"
#include "id_sd.h"
// -- Common --
void CA_Startup(void);
void CA_Shutdown(void);

// -- Audio --

#define CA_MAX_AUDIO_CHUNKS 256
typedef struct ca_audinfo
{
	uint16_t numSongs, numSounds, numSndChunks;
	uint16_t startPCSounds, startAdlibSounds, startDigiSounds, startMusic;
} ca_audinfo;

extern ca_audinfo ca_audInfoE;

void CA_CacheAudioChunk(int16_t chunk);
void CA_LoadAllSounds(void);

// -- File IO --

size_t CA_GetFileSize(char *filename);
char *CAL_AdjustExtension(const char *filename);
bool CA_FarWrite(int handle, uint8_t *source, int length);
bool CA_ReadFile(const char *filename, void *offset);
bool CA_SafeReadFile(const char *filename, void *offset, int bufLength);
bool CA_WriteFile(const char *filename, void *offset, int bufLength);
bool CA_LoadFile(const char *filename, mm_ptr_t *ptr, int *memsize);
int CAL_RLEWCompress(void *src, int expLength, void *dest, uint16_t rletag);
void CAL_RLEWExpand(void *src, void *dest, int expLength, uint16_t rletag);

// -- Graphics --

#define CA_MAX_GRAPH_CHUNKS 8192
// The GFXINFOE structure containing info about the EGAGRAPH (see TED5)
typedef struct ca_gfxinfo
{
	uint16_t numTiles8, numTiles8m, numTiles16, numTiles16m, numTiles32, numTiles32m;
	uint16_t offTiles8, offTiles8m, offTiles16, offTiles16m, offTiles32, offTiles32m;
	uint16_t numBitmaps, numMasked, numSprites;
	uint16_t offBitmaps, offMasked, offSprites;
	uint16_t hdrBitmaps, hdrMasked, hdrSprites;
	uint16_t numBinaries, offBinaries;
} ca_gfxinfo;

extern uint8_t ca_levelnum, ca_levelbit;
extern int16_t ca_mapOn;
extern uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS];
extern ca_gfxinfo ca_gfxInfoE;

extern mm_ptr_t ca_graphChunks[CA_MAX_GRAPH_CHUNKS];

mm_ptr_t CA_GetGrChunk(int base, int index, const char *chunkType, bool required);
void CA_CacheGrChunk(int chunk);
void CA_ClearMarks(void);
void CA_SetGrPurge(void);
void CA_MarkGrChunk(int chunk);
void CA_LockGrChunk(int chunk);

// For the score box.
void CAL_ShiftSprite(uint8_t *srcImage, uint8_t *dstImage, int width, int height, int pxShift);

void CA_CacheMarks(const char *msg);
void CA_UpLevel(void);
void CA_DownLevel(void);

// -- Maps --

#define CA_NUMMAPPLANES 3
#define CA_NUMMAPS 100

typedef CK_PACKED_STRUCT(CA_MapHeader
{
	uint32_t planeOffsets[CA_NUMMAPPLANES];
	uint16_t planeLengths[CA_NUMMAPPLANES];
	uint16_t width, height;
	char name[16];
	char signature[4];
}) CA_MapHeader;

extern CA_MapHeader *CA_MapHeaders[CA_NUMMAPS];

extern uint16_t *CA_mapPlanes[CA_NUMMAPPLANES];

extern uint8_t *CA_audio[CA_MAX_AUDIO_CHUNKS];

// Keen: custom cachebox hooks
extern void (*ca_beginCacheBox)(const char *title, int numcache);
extern void (*ca_updateCacheBox)(void);
extern void (*ca_finishCacheBox)(void);

void CA_CacheMap(int mapIndex);
uint16_t *CA_TilePtrAtPos(int16_t x, int16_t y, int16_t plane);
uint16_t CA_TileAtPos(int16_t x, int16_t y, int16_t plane);
void CA_SetTileAtPos(int16_t x, int16_t y, int16_t plane, uint16_t value);
uint16_t CA_GetMapHeight();
uint16_t CA_GetMapWidth();

// Omnispeak: Common lump marking code
void CA_InitLumps(void);
void CA_ClearLumps(void);
void CA_ClearLumps(void);
void CA_MarkLumpNeeded(int lump);
void CA_MarkAllLumps(void);
#define CA_MARKLUMP(name) CA_MarkLumpNeeded(CK_INT(name, -1))

#endif
