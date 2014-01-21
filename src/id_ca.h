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

#include "id_mm.h"
// -- Common --
void CA_Startup();


// -- File IO --

char* CAL_AdjustExtension(char *filename);
bool CA_ReadFile(char *filename, void *offset);
bool CA_SafeReadFile(char *filename, void *offset, int bufLength);
bool CA_WriteFile(char *filename, void *offset, int bufLength);
bool CA_LoadFile(char *filename, mm_ptr_t *ptr, int *memsize);



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

ca_gfxinfo ca_gfxInfoE;

mm_ptr_t ca_graphChunks[CA_MAX_GRAPH_CHUNKS];

void CA_CacheGrChunk(int chunk);

// -- Maps --

#define CA_NUMMAPPLANES 3
#define CA_NUMMAPS 100


typedef struct CA_MapHeader
{
	uint32_t planeOffsets[CA_NUMMAPPLANES];
	uint16_t planeLengths[CA_NUMMAPPLANES];
	uint16_t width, height;
	char name[16];
	char signature[4];
} __attribute__((__packed__)) CA_MapHeader;

extern CA_MapHeader *CA_MapHeaders[100];

extern uint16_t *CA_mapPlanes[3];


void CA_CacheMap(int mapIndex);
uint16_t CA_TileAtPos(int x, int y, int plane);
uint16_t CA_GetMapHeight();
uint16_t CA_GetMapWidth(); 


#endif
