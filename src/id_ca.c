// ID_CA: The Cache Manager
// The cache manager handles the loading and decoding of game files,
// be they graphics, sound or maps. It provides decompression functions
// for Huffman, Carmack and RLEW.
//
// NOTE: At the moment this is not endian-independent.


#include "id_ca.h"
#include "id_us.h"

#include <stdio.h>

#define CA_THREEBYTEHEADERS


//Begin globals

// CA_ReadFile reads a whole file into the preallocated memory buffer at 'offset'
// NOTE that this function is deprecated: use CA_SafeReadFile instead.
//
bool CA_ReadFile(char *filename, void *offset)
{
	FILE *f = fopen(filename, "rb");
	
	//Find the length of the file.
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	int totalRead = fread(offset, length, 1, f);

	fclose(f);

	return (length == totalRead);
}

// Reads a file into a buffer of length bufLength
bool CA_SafeReadFile(char *filename, void *offset, int bufLength)
{
	FILE *f = fopen(filename, "rb");

	//Find length of the file.
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	int amountToRead = (length > bufLength)?bufLength:length;

	int totalRead = fread(offset, amountToRead, 1, f);

	fclose(f);

	return (totalRead == amountToRead);
}

bool CA_WriteFile(char *filename, void *offset, int bufLength)
{
	FILE *f = fopen(filename, "wb");

	if (!f) return false;

	int amountWritten = fwrite(offset,bufLength,1,f);

	fclose(f);

	return (amountWritten == bufLength);
}

bool CA_LoadFile(char *filename, mm_ptr_t *ptr, int *memsize)
{
	FILE *f = fopen(filename, "rb");

	//Get length of file
	fseek(f,0,SEEK_END);
	int length = ftell(f);
	fseek(f,0,SEEK_SET);

	MM_GetPtr(ptr,length);

	if (memsize)
		*memsize = length;

	int amountRead = fread(*ptr,1, length,f);

	printf("LoadFile: read %d of %d bytes from %s.\n",amountRead,length, filename);
	
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
	printf("CAL_HuffExpand() Decompressed %d bytes (bitmask %d) to %d bytes.\n", complen, src_bit, expLength);
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
		ch = *(srcptr++);
		if ((ch & 0xff00) == CA_CARMACK_NEARTAG)
		{
			count = ch & 0xff;
			if (!count)
			{
				//Read a byte and output a7xx
				ch &= 0xff00;
				ch |= *((uint8_t*)srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = *((uint8_t*)srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				runptr = dstptr - offset;//(uint16_t*)offset;
				expLength -= count;
				printf("CAL_CarmackExpand: Got a near tag: count: %d, offset: %d\n",count,offset);
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
				ch |= *((uint8_t*)srcptr);
				srcptr = (uint16_t*)(((uint8_t*)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = *(srcptr++);
				runptr = (uint16_t*)dest + offset;//(uint16_t*)offset;
				expLength -= count;
				printf("CAL_CarmackExpand: Got a far tag: count %d, offset %d\n",count,offset);
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
	uint16_t *dstptr = (uint16_t*)dest-1;
	uint16_t count = 0;

	while (expLength)
	{
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
		value = *(srcptr++);
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
			printf("CA_RLEWExpand(): Got a run of %d (length %d)\n",value,count);
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

//Get the offset of a (compressed) chunk in the ?GAGRAPH file.
long CAL_GetGrChunkStart(int chunk)
{
	int offset = chunk*3;
	//Warning: This currently only works on LITTLE-ENDIAN systems.
	long value = (*(long *)(ca_graphStarts + offset)) & 0x00ffffff;
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
	while (CAL_GetGrChunkStart(nextChunk) == -1) nextChunk++;
	return CAL_GetGrChunkStart(nextChunk) - CAL_GetGrChunkStart(chunk) - sizeOffset;
}


void CAL_SetupGrFile()
{
	//TODO: Setup cfg mechanism for filenames, chunk data.
	// Read gfxinfoe for data?

	//Load the ?GADICT
	CA_LoadFile("EGADICT.CK5", (void**)(&ca_gr_huffdict), 0);

	//CAL_OptimizeNodes(ca_gr_huffdict);

	//Load the ?GAHEAD
	CA_LoadFile("EGAHEAD.CK5", &ca_graphStarts, 0);

	FILE *gfxinfoe = fopen("GFXINFOE.CK5","rb");
	fread(&ca_gfxInfoE, 1, sizeof(ca_gfxinfo), gfxinfoe);
	fclose(gfxinfoe);

	//Load the graphics --- we will keep the file open for the duration of the game.
	ca_graphHandle = fopen("EGAGRAPH.CK5","rb");

}

void CAL_ExpandGrChunk(int chunk, void *source)
{
	//TODO: Support non-basic chunks.
	int32_t length = CAL_GetGrChunkExpLength(chunk);
	
	if (!length)
	{
		length = *(uint32_t*)source;
		source = (uint8_t*)source+4;
	}

	printf("CAL_ExpandGrChunk(%d): Chunk length %d bytes.\n", chunk, length);
	MM_GetPtr(&ca_graphChunks[chunk],length);
	CAL_HuffExpand(source,ca_graphChunks[chunk],length,ca_gr_huffdict);
}

void CA_CacheGrChunk(int chunk)
{
	//TODO: Implement grneeded

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
	int read  = fread(compdata,1,compressedLength, ca_graphHandle);
	printf("CA_CacheGrChunk(%d): Read %d of %d bytes.\n",chunk, read, compressedLength);
	CAL_ExpandGrChunk(chunk, compdata);
	MM_FreePtr(&compdata);
}

// Map loading fns
typedef struct CA_MapHead
{
	uint16_t rleTag;
	uint32_t headerOffsets[CA_NUMMAPS];
} __attribute__((__packed__)) CA_MapHead;

CA_MapHead *ca_MapHead;

FILE *ca_GameMaps;

CA_MapHeader *CA_MapHeaders[100];

uint16_t *CA_mapPlanes[CA_NUMMAPPLANES];

extern uint8_t *ti_tileInfo;
void CAL_SetupMapFile()
{
	CA_LoadFile("MAPHEAD.CK5", (void**)(&ca_MapHead), 0);
	ca_GameMaps = fopen("GAMEMAPS.CK5", "rb");
	CA_LoadFile("TILEINFO.CK5",(void**)(&ti_tileInfo), 0);
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

		
		int read = fread(compBuffer, 1, planeCompLength, ca_GameMaps);
		printf("CA_CacheMap: read %d bytes of compressed map data.\n",read);


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

// CA_Startup opens the core CA datafiles
void CA_Startup()
{
	// Load the ?GAGRAPH.EXT file!
	CAL_SetupGrFile();

	// Read in the graphics headers (from TED's GFXINFOE)
	CA_CacheGrChunk(ca_gfxInfoE.hdrBitmaps);
	CA_CacheGrChunk(ca_gfxInfoE.hdrMasked);
	CA_CacheGrChunk(ca_gfxInfoE.hdrSprites);

	// Load some other chunks needed by the game
	CA_CacheGrChunk(88);	//TODO: What was this again
	CA_CacheGrChunk(3);	// Main font

	// Setup the map file
	CAL_SetupMapFile();
}

//TODO: Make this less of an ugly hack.
extern int ck_currentMapNumber;

uint16_t CA_TileAtPos(int x, int y, int plane)
{
	return CA_mapPlanes[plane][y*CA_MapHeaders[ck_currentMapNumber]->width+x];
}
