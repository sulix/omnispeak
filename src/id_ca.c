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

// ID_CA: The Cache Manager
// The cache manager handles the loading and decoding of game files,
// be they graphics, sound or maps. It provides decompression functions
// for Huffman, Carmack and RLEW.
//
// NOTE: At the moment this is not endian-independent.


#include "id_ca.h"
#include "id_us.h"
#include "ck_ep.h"
#include "ck_cross.h"

#include <string.h>
#include <stdio.h>
#include "SDL.h"

// For chdir
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#define CA_THREEBYTEHEADERS

#ifndef _WIN32
#include <sys/stat.h>
#include <dirent.h>

bool CAL_AdjustFilenameCase(char *filename)
{
	// Quickly check to see if the file exists with the current case.
	struct stat fileStat;
	if (!stat(filename, &fileStat))
	{
		return true;
	}

	DIR *currentDirPtr = opendir(".");

	// Search the current directory for matching names.
	for (struct dirent *dirEntry = readdir(currentDirPtr); dirEntry; dirEntry = readdir(currentDirPtr))
	{
		if (!CK_Cross_strcasecmp(dirEntry->d_name, filename))
		{
			// We've found our file!
			// TODO: This is more than a little ugly.
			strcpy(filename, dirEntry->d_name);
			closedir(currentDirPtr);
			return true;
		}
	}

	// We didn't find a matching file.
	closedir(currentDirPtr);
	return false;
}
#else
#define WIN32_MEAN_AND_LEAN
#undef UNICODE
#include <windows.h>
bool CAL_AdjustFilenameCase(char *filename)
{
	DWORD fileAttribs = GetFileAttributes(filename);
	return (fileAttribs != INVALID_FILE_ATTRIBUTES);
}
#endif

//Begin globals

/* These functions read a little-endian value. */

uint8_t CAL_ReadByte(void *offset)
{
	return *((uint8_t*)(offset));
}

int16_t CAL_ReadWord(void *offset)
{
	return (int16_t)SDL_SwapLE16(*((uint16_t*)(offset)));
}

int32_t CAL_ReadLong(void *offset)
{
	return (int32_t)SDL_SwapLE32(*((uint32_t*)(offset)));
}

int8_t CAL_ReadSByte(void *offset)
{
	return *((int8_t*)(offset));
}

uint16_t CAL_ReadUWord(void *offset)
{
	return SDL_SwapLE16(*((uint16_t*)(offset)));
}

uint32_t CAL_ReadULong(void *offset)
{
	return SDL_SwapLE32(*((uint32_t*)(offset)));
}

//Begin locals
SDMode oldsoundmode;

ca_audinfo ca_audInfoE;

uint8_t ca_levelnum = 0, ca_levelbit = 0;
int16_t ca_mapOn = 0;
uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS] = {0};

ca_gfxinfo ca_gfxInfoE;
mm_ptr_t ca_graphChunks[CA_MAX_GRAPH_CHUNKS];

// Keen: custom cachebox hooks
void	(*ca_beginCacheBox)	(const char *title, int numcache);
void	(*ca_updateCacheBox)	(void);
void	(*ca_finishCacheBox)	(void);

// Does a file exist (with filename case correction)
bool CA_IsFilePresent(const char *filename)
{
	static char newname[16];
	strcpy(newname,filename);
	if (!CAL_AdjustFilenameCase(newname))
	{
		return false;	
	}
	return true;
}

// Adjusts the extension on a filename to match the current episode.
// This function is NOT thread safe, and the string returned is only
// valid until the NEXT invocation of this function.
char* CAL_AdjustExtension(const char *filename)
{
	static char newname[16];
	strcpy(newname,filename);
	size_t fnamelen = strlen(filename);
	newname[fnamelen-3] = ck_currentEpisode->ext[0];
	newname[fnamelen-2] = ck_currentEpisode->ext[1];
	newname[fnamelen-1] = ck_currentEpisode->ext[2];
	if (!CAL_AdjustFilenameCase(newname))
	{
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't find file \"%s\" in the current directory.\n", newname);
	}
	return newname;
}

bool CA_FarWrite(int handle, uint8_t *source, int length)
{
	// TODO: Implement
	return false;

}

// CA_ReadFile reads a whole file into the preallocated memory buffer at 'offset'
// NOTE that this function is deprecated: use CA_SafeReadFile instead.
//
bool CA_ReadFile(const char *filename, void *offset)
{
	FILE *f = fopen(CAL_AdjustExtension(filename), "rb");

	//Find the length of the file.
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	int totalRead = fread(offset, length, 1, f);

	fclose(f);

	return (length == totalRead);
}

// Reads a file into a buffer of length bufLength
bool CA_SafeReadFile(const char *filename, void *offset, int bufLength)
{
	FILE *f = fopen(CAL_AdjustExtension(filename), "rb");

	//Find length of the file.
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	int amountToRead = (length > bufLength)?bufLength:length;

	int totalRead = fread(offset, amountToRead, 1, f);

	fclose(f);

	return (totalRead == amountToRead);
}

bool CA_WriteFile(const char *filename, void *offset, int bufLength)
{
	FILE *f = fopen(CAL_AdjustExtension(filename), "wb");

	if (!f) return false;

	int amountWritten = fwrite(offset,bufLength,1,f);

	fclose(f);

	return (amountWritten == bufLength);
}

bool CA_LoadFile(const char *filename, mm_ptr_t *ptr, int *memsize)
{
	FILE *f = fopen(CAL_AdjustExtension(filename), "rb");

	//Get length of file
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	MM_GetPtr(ptr,length);

	if (memsize)
		*memsize = length;

	int amountRead = fread(*ptr,1, length,f);

	fclose(f);

	if (amountRead != length)
		return false;
	return true;
}

//
// Huffman Decompression Code
//

typedef struct {
	uint16_t bit_0;
	uint16_t bit_1;
} ca_huffnode;

void CAL_OptimizeNodes(ca_huffnode *table)
{
	//STUB: This optimization is not very helpful on modern machines.
}


void CAL_HuffExpand(void *src, void *dest, int expLength, ca_huffnode *table)
{
	int headptr = 254;
	uint8_t *srcptr = (uint8_t*)src;
	uint8_t *dstptr = (uint8_t*)dest;
	int src_bit = 1;	//ch in asm src
	uint8_t src_char = *(srcptr++);
	int len = 0;
	int complen = 1;
	while (len < expLength)
	{
		if (src_char & src_bit)
		{
			// We've got a '1' bit.
			headptr = table[headptr].bit_1;
		}
		else
		{
			// We've got a '0' bit.
			headptr = table[headptr].bit_0;
		}


		if (headptr > 255) headptr -= 256;
		else {
			*(dstptr++) = (uint8_t)(headptr & 0xff);
			headptr = 254;
			len++;
			if (len == expLength) break;
		}

		src_bit <<= 1;
		if (src_bit == 256) {
			src_char = *(srcptr++);
			src_bit = 1;
			complen++;
		}
	}
}

#define CA_CARMACK_NEARTAG 0xA700
#define CA_CARMACK_FARTAG 0xA800

void CAL_CarmackExpand(void *src, void *dest, int expLength)
{
	uint16_t *srcptr = (uint16_t*)src;
	uint16_t *dstptr = (uint16_t*)dest;
	uint16_t *runptr;
	uint16_t ch, count, offset;
	expLength /= 2;	//We're dealing with two-byte words

	while (expLength > 0)
	{
		ch = CAL_ReadWord(srcptr++);
		if ((ch & 0xff00) == CA_CARMACK_NEARTAG)
		{
			count = ch & 0xff;
			if (!count)
			{
				//Read a byte and output a7xx
				ch &= 0xff00;
				ch |= CAL_ReadByte(srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = CAL_ReadByte(srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				runptr = dstptr - offset;//(uint16_t*)offset;
				expLength -= count;
				while (count--) *(dstptr++) = *(runptr++);
			}
		}
		else if ((ch & 0xff00) == CA_CARMACK_FARTAG)
		{
			count = ch & 0xff;
			if (!count)
			{
				//Read a byte and output a8xx
				ch &= 0xff00;
				ch |= CAL_ReadByte(srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = CAL_ReadWord(srcptr++);
				runptr = (uint16_t*)dest + offset;//(uint16_t*)offset;
				expLength -= count;
				while (count--) *(dstptr++) = *(runptr++);
			}
		}
		else
		{
			*(dstptr++) = ch;//*(srcptr++);
			--expLength;
		}
	}
}


int CAL_RLEWCompress (void *src, int expLength, void *dest, uint16_t rletag)
{
	int compLength = 0;
	uint16_t *srcptr = (uint16_t*)src;
	uint16_t *dstptr = (uint16_t*)dest;
	uint16_t count;

	while (expLength)
	{
		count = 1;
		uint16_t val = *srcptr++;
		expLength -= 2;
		while (*srcptr == val && expLength)
		{
			count++;
			expLength -= 2;
			srcptr++;
		}
		if (count > 3 || val == rletag)
		{
			*dstptr++ = rletag;
			*dstptr++ = count;
			*dstptr++ = val;
			compLength += 6;
		}
		else
		{
			compLength += count*2;
			while(count--) *dstptr++ = val;
		}
	}
	return compLength;
}

void CAL_RLEWExpand (void *src, void *dest, int expLength, uint16_t rletag)
{
	uint16_t *srcptr = (uint16_t*)src;
	uint16_t *dstptr = (uint16_t*)dest;
	uint16_t count, value;

	while (expLength > 0)
	{
		value = CAL_ReadWord(srcptr++);
		if (value != rletag)
		{
			*(dstptr++) = value;
			expLength -= 2;
		}
		else
		{
			count = *(srcptr++);
			value = *(srcptr++);
			expLength -= count*2;
			if(expLength < 0) return;
			for(int i = 0; i < count; ++i) { *(dstptr++) = value; }
		}
	}
}

//
// Datafile loading routines.
//

static ca_huffnode *ca_gr_huffdict;

static FILE *ca_graphHandle;	//File Pointer for ?GAGRAPH file.
void *ca_graphStarts;

// Size of the ?GAHEAD file (i.e. number of chunks * 3)
int ca_graphHeadSize;
// Size of the ?GAGRAPH file (in bytes)
int ca_graphFileSize;

//Get the offset of a (compressed) chunk in the ?GAGRAPH file.
long CAL_GetGrChunkStart(int chunk)
{
	int offset = chunk*3;
	if (offset >= ca_graphHeadSize)
		return -1;
	long value = ((uint8_t*)ca_graphStarts)[offset]
		| ((uint8_t*)ca_graphStarts)[offset+1] << 8
		| ((uint8_t*)ca_graphStarts)[offset+2] << 16;
	if (value == 0xffffff)
		return -1;
	return value;
}

int CAL_GetGrChunkExpLength(int chunk)
{
	uint32_t chunkExpandedLength;
	if (chunk >= ca_gfxInfoE.offTiles8 && chunk < ca_gfxInfoE.offBinaries)
	{
		if (chunk < ca_gfxInfoE.offTiles8m)
		{
			chunkExpandedLength = 32 * ca_gfxInfoE.numTiles8;
		}
		else if (chunk < ca_gfxInfoE.offTiles16)
		{
			chunkExpandedLength = 40 * ca_gfxInfoE.numTiles8m;
		}
		else if (chunk < ca_gfxInfoE.offTiles16m)
		{
			chunkExpandedLength = 128;
		}
		else if (chunk < ca_gfxInfoE.offTiles32)
		{
			chunkExpandedLength = 40*4;
		}
		else if (chunk < ca_gfxInfoE.offTiles32m)
		{
			chunkExpandedLength = 32 * 16;
		}
		else
		{
			chunkExpandedLength = 40 * 16;
		}
	}
	else
	{
		//TODO: Work out how to return this. Struct?
		//fseek(ca_graphHandle, CAL_GetGrChunkStart(chunk), SEEK_SET);
		//fread(&chunkExpandedLength,4,1,ca_graphHandle);
		return 0;
	}
	return chunkExpandedLength;
}

int CAL_GetGrChunkCompLength(int chunk)
{
	int nextChunk = chunk +1;
	int sizeOffset = 0;
	if (chunk < ca_gfxInfoE.offTiles8 && chunk >= ca_gfxInfoE.offBinaries)
		sizeOffset = 4;
	if (nextChunk * 3 >= ca_graphHeadSize)
		return ca_graphFileSize - CAL_GetGrChunkStart(chunk) - sizeOffset;
	while (CAL_GetGrChunkStart(nextChunk) == -1) nextChunk++;
	return CAL_GetGrChunkStart(nextChunk) - CAL_GetGrChunkStart(chunk) - sizeOffset;
}

void CA_LockGrChunk(int chunk)
{
	MM_SetLock(&ca_graphChunks[chunk], true);
}

void CAL_SetupGrFile()
{
	//TODO: Setup cfg mechanism for filenames, chunk data.

	//Load the ?GADICT
	CA_LoadFile("EGADICT.EXT", (void**)(&ca_gr_huffdict), 0);

	// We don't need to 'OptimizeNodes'.
	//CAL_OptimizeNodes(ca_gr_huffdict);

	//Load the ?GAHEAD
	CA_LoadFile("EGAHEAD.EXT", &ca_graphStarts, &ca_graphHeadSize);

	// Read chunk type info from GFEINFO?
	FILE *gfxinfoe = fopen(CAL_AdjustExtension("GFXINFOE.EXT"),"rb");
	fread(&ca_gfxInfoE, 1, sizeof(ca_gfxinfo), gfxinfoe);
	fclose(gfxinfoe);

	//Load the graphics --- we will keep the file open for the duration of the game.
	ca_graphHandle = fopen(CAL_AdjustExtension("EGAGRAPH.EXT"),"rb");

	// Find the size of the file. Some mods do not have the last entry in the ?GAHEAD,
	// so we compute it like so.
	fseek(ca_graphHandle, 0, SEEK_END);
	ca_graphFileSize = ftell(ca_graphHandle);

	// Read in the graphics headers (from TED's GFXINFOE)
	// For some reason, keen decompresses these differently, not
	// purring the resultant pointers in the ca_graphChunks array.
	// Presumably this is to stop them from being evicted,
	// but we have MM_SetLock for that, don't we?
	CA_CacheGrChunk(ca_gfxInfoE.hdrBitmaps);
	CA_CacheGrChunk(ca_gfxInfoE.hdrMasked);
	CA_CacheGrChunk(ca_gfxInfoE.hdrSprites);

	//Lock them in memory.
	CA_LockGrChunk(ca_gfxInfoE.hdrBitmaps);
	CA_LockGrChunk(ca_gfxInfoE.hdrMasked);
	CA_LockGrChunk(ca_gfxInfoE.hdrSprites);
}

void CAL_ExpandGrChunk(int chunk, void *source)
{
	//TODO: Support non-basic chunks.
	int32_t length = CAL_GetGrChunkExpLength(chunk);

	if (!length)
	{
		length = CAL_ReadLong(source);
		source = (uint8_t*)source+4;
	}

	MM_GetPtr(&ca_graphChunks[chunk],length);
	CAL_HuffExpand(source,ca_graphChunks[chunk],length,ca_gr_huffdict);
}

void CA_CacheGrChunk(int chunk)
{
	CA_MarkGrChunk(chunk);

	//Is the chunk already loaded?
	if (ca_graphChunks[chunk])
	{
		//If so, keep it in memory.
		MM_SetPurge(&ca_graphChunks[chunk],0);
		return;
	}

	int compressedLength = CAL_GetGrChunkCompLength(chunk);

	if (CAL_GetGrChunkStart(chunk) == -1) return;

	fseek(ca_graphHandle, CAL_GetGrChunkStart(chunk), SEEK_SET);

	mm_ptr_t compdata;
	MM_GetPtr(&compdata,compressedLength);
	int read = 0;// fread(compdata,1,compressedLength, ca_graphHandle);
	do
	{
		int curRead = fread(((uint8_t*)compdata)+read,1,compressedLength - read, ca_graphHandle);
		if (curRead < 0)
		{
			Quit("Error reading compressed graphics chunk.");
		}
		read += curRead;
	} while (read < compressedLength);
	CAL_ExpandGrChunk(chunk, compdata);
	MM_FreePtr(&compdata);
}

// CA_ClearMarks:
// Marks all graphics as unused in the current level.
void CA_ClearMarks(void)
{
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		ca_graphChunkNeeded[i] &= ~ca_levelbit;
	}
}

void CA_SetGrPurge(void)
{
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		if (ca_graphChunks[i])
		{
			MM_SetPurge((mm_ptr_t*)(&ca_graphChunks[i]),3);
		}
	}
}

void CA_MarkGrChunk(int chunk)
{
	ca_graphChunkNeeded[chunk] |= ca_levelbit;
}

void CA_CacheMarks(const char *msg)
{
	bool isMessage = msg? true : false;
	int numChunksToCache = 0;

	// Mark all unused chunks as purgeable, needed chunks as unpurgeable,
	// and count number of chunks to cache.
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		if (ca_graphChunkNeeded[i] & ca_levelbit)
		{
			if (ca_graphChunks[i])
			{
				MM_SetPurge(&ca_graphChunks[i], 0);
			}
			else
			{
				numChunksToCache++;
			}
		}
		else
		{
			if (ca_graphChunks[i])
			{
				MM_SetPurge(&ca_graphChunks[i],3);
			}
		}
	}

	if (!numChunksToCache) return;

	//Loading screen.
	if (isMessage && ca_beginCacheBox)
		ca_beginCacheBox(msg, numChunksToCache);

	// Cache all of the chunks we'll need.
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		if ( (ca_graphChunkNeeded[i] & ca_levelbit) && (!ca_graphChunks[i]) )
		{
			//Update loading screen.
			if (isMessage && ca_updateCacheBox)
				ca_updateCacheBox();

			// In the original keen code, a lot of work here went into coalescing reads.
			// The C standard library does this sort of thing for us, particularly
			// given that we'll be loading things in file order anyway, so we just
			// use CA_CacheGrChunk instead.


			CA_CacheGrChunk(i);
		}
	}

	//Finish Loading Screen
	if (isMessage && ca_finishCacheBox)
		ca_finishCacheBox();

}

// CA_UpLevel:
// Pushes a new level onto the resource stack.
void CA_UpLevel(void)
{
	if (ca_levelnum == 7)
	{
		Quit("CA_UpLevel: Up past level 7!");
	}

	ca_levelbit <<= 1;
	ca_levelnum++;
}

// CA_DownLevel:
// Uncaches everything in the current level and pops it from the rsrc stack.
void CA_DownLevel(void)
{
	if (ca_levelnum == 0)
	{
		Quit("CA_DownLevel: Down past level 0!");
	}

	ca_levelbit >>= 1;
	ca_levelnum--;

	CA_CacheMarks(0);
}

// Map loading fns
typedef struct CA_MapHead
{
	uint16_t rleTag;
	uint32_t headerOffsets[CA_NUMMAPS];
} __attribute__((__packed__)) CA_MapHead;

CA_MapHead *ca_MapHead;

FILE *ca_GameMaps;

CA_MapHeader *CA_MapHeaders[CA_NUMMAPS];

uint16_t *CA_mapPlanes[CA_NUMMAPPLANES];

extern uint8_t *ti_tileInfo;
void CAL_SetupMapFile(void)
{
	CA_LoadFile("MAPHEAD.EXT", (void**)(&ca_MapHead), 0);
	ca_GameMaps = fopen(CAL_AdjustExtension("GAMEMAPS.EXT"), "rb");
	CA_LoadFile("TILEINFO.EXT",(void**)(&ti_tileInfo), 0);
}

static ca_huffnode *ca_audiohuffman;

static FILE *ca_audiohandle;	//File Pointer for AUDIO file.
int32_t *ca_audiostarts;

void CAL_SetupAudioFile(void)
{
	// Read audio chunk type info from AUDINFOE
  FILE *audinfoe = fopen(CAL_AdjustExtension("AUDINFOE.EXT"),"rb");
	fread(&ca_audInfoE, 1, sizeof(ca_audinfo), audinfoe);
	fclose(audinfoe);

	//Load the AUDIODCT
	CA_LoadFile("AUDIODCT.EXT", (void**)(&ca_audiohuffman), 0);

	// We don't need to 'OptimizeNodes'.
	//CAL_OptimizeNodes(ca_audiohuffman);

	//Load the AUDIOHED
	CA_LoadFile("AUDIOHED.EXT", (void **)(&ca_audiostarts), 0);

	//Load the sound data --- we will keep the file open for the duration of the game.
	ca_audiohandle = fopen(CAL_AdjustExtension("AUDIO.EXT"),"rb");
	if (!ca_audiohandle)
	{
		Quit("Can't open AUDIO.CK5!");
	}
}

void CA_CacheMap(int mapIndex)
{
	//TODO: Support having multiple maps cached at once.
	//Unload the previous map.
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		if (CA_mapPlanes[plane])
		{
			MM_FreePtr((void**)(&CA_mapPlanes[plane]));
		}
	}
	ca_mapOn = mapIndex;

	//Have we loaded this map's header?
	if (!CA_MapHeaders[mapIndex])
	{
		uint32_t headerOffset = ca_MapHead->headerOffsets[mapIndex];

		if (headerOffset <= 0)
		{
			Quit("CA_CacheMap: Tried to load a non-existant map!");
		}

		MM_GetPtr((void**)(&CA_MapHeaders[mapIndex]),sizeof(CA_MapHeader));

		fseek(ca_GameMaps, headerOffset, SEEK_SET);

		fread(CA_MapHeaders[mapIndex], 1, sizeof(CA_MapHeader), ca_GameMaps);

	}
	else
	{
		//Make sure we don't purge it accidentally.
		MM_SetPurge((void**)(&CA_MapHeaders[mapIndex]), 0);
	}

	int planeSize = CA_MapHeaders[mapIndex]->width * CA_MapHeaders[mapIndex]->height * 2;

	//Load the map data
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		int32_t planeOffset = CA_MapHeaders[mapIndex]->planeOffsets[plane];
		int32_t planeCompLength = CA_MapHeaders[mapIndex]->planeLengths[plane];

		MM_GetPtr((void**)&CA_mapPlanes[plane],planeSize);

		fseek(ca_GameMaps, planeOffset, SEEK_SET);

		uint16_t *compBuffer;
		MM_GetPtr((void**)(&compBuffer),planeCompLength);
		//MM_SetLock(&compBuffer,true);


		int read = 0;
		do
		{
			int curRead = fread(((uint8_t*)compBuffer)+read, 1, planeCompLength - read, ca_GameMaps);
			if (curRead < 0)
			{
				Quit("Error reading compressed map plane.");
			}
			read += curRead;
		} while (read < planeCompLength);

		uint16_t carmackExpanded = *compBuffer;

		uint16_t *rlewBuffer;
		MM_GetPtr((void**)(&rlewBuffer), carmackExpanded);

		//Decompress the map.
		CAL_CarmackExpand(compBuffer+1, rlewBuffer, carmackExpanded);
		CAL_RLEWExpand(rlewBuffer+1, CA_mapPlanes[plane], planeSize, ca_MapHead->rleTag);

		//Release the temp buffers.
		//MM_UnLockPtr(&compBuffer);
		MM_FreePtr((void**)(&compBuffer));
		MM_FreePtr((void**)(&rlewBuffer));
	}
}

void CA_AssertFileExists(char *filename)
{
	if (!CAL_AdjustFilenameCase(filename))
	{
		char message[128];
#ifdef _MSC_VER
		// MSVC++ does not have snprintf, but does have _snprintf which does not null-terminate.
		// Here it shouldn't be a problem (Keen filenames are 8.3, so won't overflow the buffer), but it's
		// better safe than sorry.
		_snprintf(message, 127, "Could not find %s. Please copy it into the Omnispeak directory.", filename);
		message[127] = '\0';
#else
		snprintf(message, 128, "Could not find %s. Please copy it into the Omnispeak directory.", filename);
#endif
#if SDL_VERSION_ATLEAST(1,3,0)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Omnispeak", message, NULL);
#endif
		Quit(message);
	}
}

// CA_Startup opens the core CA datafiles
void CA_Startup(void)
{
	// Check for the existence of EGAGRAPH for the Keen Day preview.

	// If the file isn't in the current directory, it might be in the directory the game binary is in.
	char checkFile[] = "EGAGRAPH.EXT";

#if SDL_VERSION_ATLEAST(2,0,1)
	if (!CAL_AdjustExtension(checkFile))
	{
		chdir(SDL_GetBasePath());
	}
#endif
	// Check EGAGRAPH
	CA_AssertFileExists(CAL_AdjustExtension(checkFile));

	char audioCheckFile[] = "AUDIO.EXT";
	CA_AssertFileExists(CAL_AdjustExtension(audioCheckFile));

	char gameMapsCheckFile[] = "GAMEMAPS.EXT";
	CA_AssertFileExists(CAL_AdjustExtension(gameMapsCheckFile));


	// Load the ?GAGRAPH.EXT file!
	CAL_SetupGrFile();

	// Load some other chunks needed by the game
  // (Moved to CK_InitGame())
  // CA_CacheGrChunk(88);	//Title Screen
	// CA_CacheGrChunk(3);	// Main font

	// Setup the map file
	CAL_SetupMapFile();

	// Load the audio file
	CAL_SetupAudioFile();

	ca_mapOn = -1;
	ca_levelbit = 1;
	ca_levelnum = 0;
	// TODO: Cache box handlers and more?
}

void CA_Shutdown(void)
{
	if (ca_GameMaps)
		fclose(ca_GameMaps);
	if (ca_graphHandle)
		fclose(ca_graphHandle);
	if (ca_audiohandle)
		fclose(ca_audiohandle);
}

uint8_t *CA_audio[CA_MAX_AUDIO_CHUNKS];

void CA_CacheAudioChunk(int16_t chunk)
{
	int32_t pos, compressed, expanded;
	mm_ptr_t bigbuffer, source;
	if (CA_audio[chunk])
	{
		MM_SetPurge((void**)(&CA_audio[chunk]), 0);
		return;
	}

	//
	// load the chunk into a buffer, either the miscbuffer if it fits, or allocate
	// a larger buffer
	//
	pos = SDL_SwapLE32(ca_audiostarts[chunk]);
	compressed = SDL_SwapLE32(ca_audiostarts[chunk+1])-pos; //+1 is not in keen...

	fseek(ca_audiohandle,pos,SEEK_SET);

	if (compressed<=BUFFERSIZE)
	{
		fread(buffer,compressed,1,ca_audiohandle);
		source = buffer;
	}
	else
	{
		MM_GetPtr(&bigbuffer,compressed);
		// TODO: Check for mmerror
#if 0
		if (mmerror)
			return;
#endif
		MM_SetLock(&bigbuffer,true);
		fread(bigbuffer,compressed,1,ca_audiohandle);
		source = bigbuffer;
	}

	expanded = CAL_ReadLong(source);
	source = (mm_ptr_t)((uint8_t *)source + 4); // skip over length
	MM_GetPtr((void**)(&CA_audio[chunk]),expanded);
	// TODO: Check for mmerror
#if 0
	if (mmerror)
		goto done;
#endif
	CAL_HuffExpand (source,CA_audio[chunk],expanded,ca_audiohuffman);

//done:
	if (compressed>BUFFERSIZE)
		MM_FreePtr(&bigbuffer);
}


void CA_LoadAllSounds(void)
{
	int16_t offset; // FIXME: What about a mode differing from 1 or 2?
	uint16_t loopvar;
	if (oldsoundmode != sdm_Off)
	{
		switch (oldsoundmode)
		{
		case sdm_PC:
			offset = ca_audInfoE.startPCSounds; // STARTPCSOUNDS;
			break;
		case sdm_AdLib:
      offset = ca_audInfoE.startAdlibSounds; //STARTADLIBSOUNDS;
			break;
		default:
			// Shut up some gcc warnings.
			break;
		}
		for (loopvar = 0; loopvar < ca_audInfoE.numSounds; loopvar++, offset++)
		{
			if (CA_audio[offset])
			{
				MM_SetPurge((void**)(&CA_audio[offset]), 3);
			}
		}
	}
	if (SoundMode != sdm_Off)
	{
		switch (SoundMode)
		{
		case sdm_PC:
			offset = ca_audInfoE.startPCSounds; // STARTPCSOUNDS;
			break;
		case sdm_AdLib:
			offset = ca_audInfoE.startAdlibSounds; // STARTADLIBSOUNDS;
			break;
		default:
			// Shut up some gcc warnings.
			break;
		}
		for (loopvar = 0; loopvar < ca_audInfoE.numSounds; loopvar++, offset++)
		{
			CA_CacheAudioChunk(offset);
		}
	}
}

//TODO: Make this less of an ugly hack.
//TODO (Mar 6 2014): ok, no need anymore? (ca_mapOn is used here now)
//extern int ck_currentMapNumber;
//UPDATE (Mar 6 2014): The var above is now ck_gameState.currentLevel.

uint16_t *CA_TilePtrAtPos(int16_t x, int16_t y, int16_t plane)
{
	return &CA_mapPlanes[plane][y*CA_MapHeaders[ca_mapOn]->width+x];
}

uint16_t CA_TileAtPos(int16_t x, int16_t y, int16_t plane)
{
	// HACK - Somewhat reproducing a glitch occuring in Keen 4: Level 10,
	// when an object goes through the top of the map
	// (not reproduced in the exact same manner)
	CA_MapHeader *mapheader = CA_MapHeaders[ca_mapOn];
	if ((x >= 0) && (x < mapheader->width) && (y >= 0) && (y < mapheader->height))
		return CA_mapPlanes[plane][y*mapheader->width+x];
	return (y*x*y*x) % (ca_gfxInfoE.numTiles16*2 + ca_gfxInfoE.numTiles16m*6);
}

void CA_SetTileAtPos(int16_t x, int16_t y, int16_t plane, uint16_t value)
{
	CA_mapPlanes[plane][y*CA_MapHeaders[ca_mapOn]->width+x] = value;
}

uint16_t CA_GetMapWidth()
{
	return CA_MapHeaders[ca_mapOn]->width;
}

uint16_t CA_GetMapHeight()
{
	return CA_MapHeaders[ca_mapOn]->height;
}
