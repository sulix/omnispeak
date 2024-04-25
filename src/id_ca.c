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
#include "id_fs.h"
#include "id_us.h"
#include "id_vh.h"
#include "ck_cross.h"
#include "ck_def.h"
#include "ck_ep.h"

#include <stdio.h>
#include <string.h>

#define CA_THREEBYTEHEADERS

// For chdir
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>

size_t CA_GetFileSize(char *filename)
{
	struct stat fileStat;
	if (stat(filename, &fileStat))
		return 0;

	return fileStat.st_size;
}

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
size_t CA_GetFileSize(char *filename)
{
	WIN32_FILE_ATTRIBUTE_DATA fileStat;
	if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &fileStat))
		return 0;

	// NOTE: size_t is 32-bit on win32 (and none of Keen's files should be big),
	// so we just use the low 32-bits of the filesize here. This stops the
	// annoying compiler warning we'd otherwise get.
	return fileStat.nFileSizeLow;
}

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
	return *((uint8_t *)(offset));
}

int16_t CAL_ReadWord(void *offset)
{
	uint16_t val;
	memcpy(&val, offset, sizeof(val));
	return (int16_t)CK_Cross_SwapLE16(val);
}

int32_t CAL_ReadLong(void *offset)
{
	uint32_t val;
	memcpy(&val, offset, sizeof(val));
	return (int32_t)CK_Cross_SwapLE32(val);
}

int8_t CAL_ReadSByte(void *offset)
{
	return *((int8_t *)(offset));
}

uint16_t CAL_ReadUWord(void *offset)
{
	uint16_t val;
	memcpy(&val, offset, sizeof(val));
	return CK_Cross_SwapLE16(val);
}

uint32_t CAL_ReadULong(void *offset)
{
	uint32_t val;
	memcpy(&val, offset, sizeof(val));
	return CK_Cross_SwapLE32(val);
}

//Begin locals
SD_SoundMode oldsoundmode;

ca_audinfo ca_audInfoE;

uint8_t ca_levelnum = 0, ca_levelbit = 0;
int16_t ca_mapOn = 0;
uint8_t ca_graphChunkNeeded[CA_MAX_GRAPH_CHUNKS] = {0};

ca_gfxinfo ca_gfxInfoE;
mm_ptr_t ca_graphChunks[CA_MAX_GRAPH_CHUNKS];

// Keen: custom cachebox hooks
void (*ca_beginCacheBox)(const char *title, int numcache);
void (*ca_updateCacheBox)(void);
void (*ca_finishCacheBox)(void);

bool CA_LoadFile(const char *filename, mm_ptr_t *ptr, int *memsize)
{
	FS_File f = FS_OpenOmniFile(FS_AdjustExtension(filename));

	if (!FS_IsFileValid(f))
		return false;

	//Get length of file
	int length = FS_GetFileSize(f);

	MM_GetPtr(ptr, length);

	if (memsize)
		*memsize = length;

	int amountRead = FS_Read(*ptr, 1, length, f);

	FS_CloseFile(f);

	if (amountRead != length)
		return false;
	return true;
}

//
// Huffman Decompression Code
//

typedef struct
{
	uint16_t bit_0;
	uint16_t bit_1;
} ca_huffnode;

void CAL_OptimizeNodes(ca_huffnode *table)
{
	//STUB: This optimization is not very helpful on modern machines.
}

void CAL_HuffExpand(void *src, void *dest, int expLength, ca_huffnode *table, int srcLength)
{
	int headptr = 254;
	uint8_t *srcptr = (uint8_t *)src;
	uint8_t *dstptr = (uint8_t *)dest;
	int src_bit = 1; //ch in asm src
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

		if (headptr > 255)
			headptr -= 256;
		else
		{
			*(dstptr++) = (uint8_t)(headptr & 0xff);
			headptr = 254;
			len++;
			if (len == expLength)
				break;
		}

		src_bit <<= 1;
		if (src_bit == 256)
		{
			src_char = *(srcptr++);
			src_bit = 1;
			complen++;
			if (complen > srcLength)
				break;
		}
	}
}

#define CA_CARMACK_NEARTAG 0xA700
#define CA_CARMACK_FARTAG 0xA800

void CAL_CarmackExpand(void *src, void *dest, int expLength)
{
	uint16_t *srcptr = (uint16_t *)src;
	uint16_t *dstptr = (uint16_t *)dest;
	uint16_t *runptr;
	uint16_t ch, count, offset;
	expLength /= 2; //We're dealing with two-byte words

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
				srcptr = (uint16_t *)(((uint8_t *)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = CAL_ReadByte(srcptr);
				srcptr = (uint16_t *)(((uint8_t *)srcptr) + 1);
				runptr = dstptr - offset; //(uint16_t*)offset;
				expLength -= count;
				while (count--)
					*(dstptr++) = *(runptr++);
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
				srcptr = (uint16_t *)(((uint8_t *)srcptr) + 1);
				*(dstptr++) = ch;
				expLength--;
			}
			else
			{
				offset = CAL_ReadWord(srcptr++);
				runptr = (uint16_t *)dest + offset; //(uint16_t*)offset;
				expLength -= count;
				while (count--)
					*(dstptr++) = *(runptr++);
			}
		}
		else
		{
			*(dstptr++) = ch; //*(srcptr++);
			--expLength;
		}
	}
}

int CAL_RLEWCompress(void *src, int expLength, void *dest, uint16_t rletag)
{
	int compLength = 0;
	uint16_t *srcptr = (uint16_t *)src;
	uint16_t *dstptr = (uint16_t *)dest;
	uint16_t count;

	if (expLength & 1)
		Quit("CAL_RLEWCompress: Expanded length must be divisible by 2.");

	while (expLength)
	{
		count = 1;
		uint16_t val = *srcptr++;
		expLength -= 2;
		while (expLength && *srcptr == val)
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
			compLength += count * 2;
			while (count--)
				*dstptr++ = val;
		}
	}
	return compLength;
}

void CAL_RLEWExpand(void *src, void *dest, int expLength, uint16_t rletag)
{
	uint16_t *srcptr = (uint16_t *)src;
	uint16_t *dstptr = (uint16_t *)dest;
	uint16_t count, value;

	while (expLength > 0)
	{
		value = *srcptr++;
		if (value != rletag)
		{
			*(dstptr++) = value;
			expLength -= 2;
		}
		else
		{
			count = *(srcptr++);
			value = *(srcptr++);
			expLength -= count * 2;
			if (expLength < 0)
				return;
			for (int i = 0; i < count; ++i)
			{
				*(dstptr++) = value;
			}
		}
	}
}

//
// Datafile loading routines.
//

static ca_huffnode *ca_gr_huffdict;

static FS_File ca_graphHandle; //File Pointer for ?GAGRAPH file.
void *ca_graphStarts;

// Size of the ?GAHEAD file (i.e. number of chunks * 3)
int ca_graphHeadSize;
// Size of the ?GAGRAPH file (in bytes)
int ca_graphFileSize;

//Get the offset of a (compressed) chunk in the ?GAGRAPH file.
long CAL_GetGrChunkStart(int chunk)
{
	int offset = chunk * 3;
	if (offset >= ca_graphHeadSize)
		return -1;
	long value = ((uint8_t *)ca_graphStarts)[offset] | ((uint8_t *)ca_graphStarts)[offset + 1] << 8 | ((uint8_t *)ca_graphStarts)[offset + 2] << 16;
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
			chunkExpandedLength = 40 * 4;
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
	int nextChunk = chunk + 1;
	int sizeOffset = 0;
	if (chunk < ca_gfxInfoE.offTiles8 && chunk >= ca_gfxInfoE.offBinaries)
		sizeOffset = 4;
	if (nextChunk * 3 >= ca_graphHeadSize)
		return ca_graphFileSize - CAL_GetGrChunkStart(chunk) - sizeOffset;
	while (CAL_GetGrChunkStart(nextChunk) == -1)
		nextChunk++;
	return CAL_GetGrChunkStart(nextChunk) - CAL_GetGrChunkStart(chunk) - sizeOffset;
}

void CA_LockGrChunk(int chunk)
{
	MM_SetLock(&ca_graphChunks[chunk], true);
}

void CAL_ShiftSprite(uint8_t *srcImage, uint8_t *dstImage, int width, int height, int pxShift)
{
	// For the mask plane, we want to fill with 0xFF, so that unused bits are masked out.
	uint8_t *dstPtr = dstImage;
	uint8_t *srcPtr = srcImage;
	for (int y = 0; y < height; ++y)
	{
		*dstPtr = 0xFF;
		for (int x = 0; x < width; ++x)
		{
			//uint16_t val = ~(((uint16_t)(~(*srcPtr++))) << pxShift);
			uint16_t raw_val = ~(*srcPtr++) << 8;
			uint16_t val = ~((raw_val) >> pxShift);
			*dstPtr &= val >> 8;
			*(++dstPtr) = val & 0xFF;
		}
		dstPtr++;
	}

	// For the data planes, we want to fill with 0.
	// Note that we can shift all four planes as though it were one very tall plane.
	for (int y = 0; y < height * 4; ++y)
	{
		*dstPtr = 0;
		for (int x = 0; x < width; ++x)
		{
			uint16_t raw_val = (*srcPtr++) << 8;
			uint16_t val = (raw_val) >> pxShift;
			*dstPtr |= val >> 8;
			*(++dstPtr) = val & 0xFF;
		}
		dstPtr++;
	}
}

void CAL_CacheSprite(int chunkNumber, uint8_t *compressed, int compLength)
{
	int spriteNumber = chunkNumber - ca_gfxInfoE.offSprites;

	VH_SpriteTableEntry *sprite = VH_GetSpriteTableEntry(spriteNumber);

	// The size of one plane of the unshifted sprite.
	size_t smallPlane = sprite->width * sprite->height;
	// The size of one plane of a shifted sprite (+ 1 byte/row)
	size_t bigPlane = (sprite->width + 1) * sprite->height;

	size_t fullSize = (smallPlane + (sprite->shifts - 1) * bigPlane) * 5;

	MM_GetPtr(&ca_graphChunks[chunkNumber], sizeof(VH_ShiftedSprite) + fullSize);
	VH_ShiftedSprite *shifted = (VH_ShiftedSprite *)ca_graphChunks[chunkNumber];

	size_t shiftOffsets[5];

	shiftOffsets[0] = 0;
	shiftOffsets[1] = smallPlane * 5;
	shiftOffsets[2] = shiftOffsets[1] + bigPlane * 5;
	shiftOffsets[3] = shiftOffsets[2] + bigPlane * 5;
	shiftOffsets[4] = shiftOffsets[3] + bigPlane * 5;

	CAL_HuffExpand(compressed, shifted->data, smallPlane * 5, ca_gr_huffdict, compLength);

	switch (sprite->shifts)
	{
	case 1:
		for (int i = 0; i < 4; ++i)
		{
			shifted->sprShiftByteWidths[i] = sprite->width;
			shifted->sprShiftOffset[i] = shiftOffsets[0];
		}
		break;
	case 2:
		for (int i = 0; i < 2; ++i)
		{
			shifted->sprShiftByteWidths[i] = sprite->width;
			shifted->sprShiftOffset[i] = shiftOffsets[0];
		}
		for (int i = 2; i < 4; ++i)
		{
			shifted->sprShiftByteWidths[i] = sprite->width + 1;
			shifted->sprShiftOffset[i] = shiftOffsets[1];
		}
		CAL_ShiftSprite(shifted->data, &shifted->data[shiftOffsets[1]], sprite->width, sprite->height, 4);
		break;
	case 4:
		shifted->sprShiftByteWidths[0] = sprite->width;
		shifted->sprShiftByteWidths[1] = sprite->width + 1;
		shifted->sprShiftByteWidths[2] = sprite->width + 1;
		shifted->sprShiftByteWidths[3] = sprite->width + 1;

		shifted->sprShiftOffset[0] = shiftOffsets[0];
		shifted->sprShiftOffset[1] = shiftOffsets[1];
		shifted->sprShiftOffset[2] = shiftOffsets[2];
		shifted->sprShiftOffset[3] = shiftOffsets[3];

		CAL_ShiftSprite(shifted->data, &shifted->data[shiftOffsets[1]], sprite->width, sprite->height, 2);
		CAL_ShiftSprite(shifted->data, &shifted->data[shiftOffsets[2]], sprite->width, sprite->height, 4);
		CAL_ShiftSprite(shifted->data, &shifted->data[shiftOffsets[3]], sprite->width, sprite->height, 6);
		break;
	default:
		Quit("CAL_CacheSprite: Bad shifts number!");
	}
}

void CAL_SetupGrFile()
{
	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "CAL_SetupGrFile: Loading graphics headers...\n");

	//Load the ?GADICT
	CA_LoadFile("EGADICT.EXT", (void **)(&ca_gr_huffdict), 0);

#ifdef CK_CROSS_IS_BIGENDIAN
	for (int i = 0; i < 256; ++i)
	{
		ca_gr_huffdict[i].bit_0 = CK_Cross_SwapLE16(ca_gr_huffdict[i].bit_0);
		ca_gr_huffdict[i].bit_1 = CK_Cross_SwapLE16(ca_gr_huffdict[i].bit_1);
	}
#endif

	// We don't need to 'OptimizeNodes'.
	//CAL_OptimizeNodes(ca_gr_huffdict);

	//Load the ?GAHEAD
	CA_LoadFile("EGAHEAD.EXT", &ca_graphStarts, &ca_graphHeadSize);

	// Read chunk type info from GFEINFO?
	FS_File gfxinfoe = FS_OpenOmniFile(FS_AdjustExtension("GFXINFOE.EXT"));
	if (!FS_IsFileValid(gfxinfoe))
		Quit("Couldn't load GFXINFOE!");
	size_t gfxinfoeLen = FS_Read(&ca_gfxInfoE, sizeof(ca_gfxinfo), 1, gfxinfoe);
	FS_CloseFile(gfxinfoe);
	if (gfxinfoeLen != 1)
		Quit("Couldn't read GFXINFOE!");
#ifdef CK_CROSS_IS_BIGENDIAN
	uint16_t *uptr = (uint16_t *)&ca_gfxInfoE;
	for (size_t loopVar = 0; loopVar < sizeof(ca_gfxInfoE) / 2; loopVar++, uptr++)
		*uptr = CK_Cross_Swap16(*uptr);
#endif

	//Load the graphics --- we will keep the file open for the duration of the game.
	ca_graphHandle = FS_OpenKeenFile(FS_AdjustExtension("EGAGRAPH.EXT"));

	// Find the size of the file. Some mods do not have the last entry in the ?GAHEAD,
	// so we compute it like so.
	ca_graphFileSize = FS_GetFileSize(ca_graphHandle);

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

void CAL_ExpandGrChunk(int chunk, void *source, int compressedLength)
{
	//TODO: Support non-basic chunks.
	int32_t length = CAL_GetGrChunkExpLength(chunk);

	if (!length)
	{
		length = CAL_ReadLong(source);
		source = (uint8_t *)source + 4;
		compressedLength -= 4;
	}

	if (length < 0)
	{
		Quit("Tried to expand an invalid chunk! Make sure you're using a compatible version of Keen!");
	}

	if (chunk >= ca_gfxInfoE.offSprites && chunk < ca_gfxInfoE.offSprites + ca_gfxInfoE.numSprites)
	{
		CAL_CacheSprite(chunk, (uint8_t *)source, compressedLength);
	}
	else
	{
		MM_GetPtr(&ca_graphChunks[chunk], length);
		CAL_HuffExpand(source, ca_graphChunks[chunk], length, ca_gr_huffdict, compressedLength);
	}
}

mm_ptr_t CA_GetGrChunk(int base, int index, const char *chunkType, bool required)
{
	mm_ptr_t result;
	int chunk = base + index;
	if ((chunk < 0) || (chunk >= CA_MAX_GRAPH_CHUNKS))
		Quit("tried to render a graphics chunk with invalid ID");
	result = ca_graphChunks[chunk];
	if (!result)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "tried to render a %s which was not cached (index = %d, chunk = %d)\n", chunkType, index, chunk);
		CA_CacheGrChunk(chunk);
		result = ca_graphChunks[chunk];
		if (!result && required)
			Quit("tried to render an uncached graphics chunk");
	}
	return result;
}

void CA_CacheGrChunk(int chunk)
{
	CA_MarkGrChunk(chunk);

	//Is the chunk already loaded?
	if (ca_graphChunks[chunk])
	{
		//If so, keep it in memory.
		MM_SetPurge(&ca_graphChunks[chunk], 0);
		return;
	}

	int compressedLength = CAL_GetGrChunkCompLength(chunk);

	if (CAL_GetGrChunkStart(chunk) == -1)
		return;

	FS_SeekTo(ca_graphHandle, CAL_GetGrChunkStart(chunk));

	mm_ptr_t compdata;
	MM_GetPtr(&compdata, compressedLength);
	int read = 0; // fread(compdata,1,compressedLength, ca_graphHandle);
	do
	{
		int curRead = FS_Read(((uint8_t *)compdata) + read, 1, compressedLength - read, ca_graphHandle);
		if (curRead < 0)
		{
			Quit("Error reading compressed graphics chunk.");
		}
		read += curRead;
	} while (read < compressedLength);
	CAL_ExpandGrChunk(chunk, compdata, compressedLength);
	MM_FreePtr(&compdata);

#ifdef CK_CROSS_IS_BIGENDIAN
	if (chunk == ca_gfxInfoE.hdrBitmaps)
	{
		VH_BitmapTableEntry *bitmapTable = (VH_BitmapTableEntry *)(ca_graphChunks[chunk]);
		for (int i = 0; i < ca_gfxInfoE.numBitmaps; ++i)
		{
			bitmapTable[i].width = CK_Cross_SwapLE16(bitmapTable[i].width);
			bitmapTable[i].height = CK_Cross_SwapLE16(bitmapTable[i].height);
		}
	}
	else if (chunk == ca_gfxInfoE.hdrMasked)
	{
		VH_BitmapTableEntry *maskedTable = (VH_BitmapTableEntry *)(ca_graphChunks[chunk]);
		for (int i = 0; i < ca_gfxInfoE.numMasked; ++i)
		{
			maskedTable[i].width = CK_Cross_SwapLE16(maskedTable[i].width);
			maskedTable[i].height = CK_Cross_SwapLE16(maskedTable[i].height);
		}
	}
	else if (chunk == ca_gfxInfoE.hdrSprites)
	{
		VH_SpriteTableEntry *spriteTable = (VH_SpriteTableEntry *)(ca_graphChunks[chunk]);
		for (int i = 0; i < ca_gfxInfoE.numSprites; ++i)
		{
			spriteTable[i].width = CK_Cross_SwapLE16(spriteTable[i].width);
			spriteTable[i].height = CK_Cross_SwapLE16(spriteTable[i].height);
			spriteTable[i].originX = CK_Cross_SwapLE16(spriteTable[i].originX);
			spriteTable[i].originY = CK_Cross_SwapLE16(spriteTable[i].originY);
			spriteTable[i].xl = CK_Cross_SwapLE16(spriteTable[i].xl);
			spriteTable[i].yl = CK_Cross_SwapLE16(spriteTable[i].yl);
			spriteTable[i].xh = CK_Cross_SwapLE16(spriteTable[i].xh);
			spriteTable[i].yh = CK_Cross_SwapLE16(spriteTable[i].yh);
			spriteTable[i].shifts = CK_Cross_SwapLE16(spriteTable[i].shifts);
		}
	}
	else if (chunk >= CK_CHUNKNUM(FON_MAINFONT) && chunk <= /*FON_WATCHFONT*/ CK_CHUNKNUM(FON_MAINFONT) + 2)
	{
		VH_Font *font = (VH_Font *)ca_graphChunks[chunk];
		font->height = CK_Cross_SwapLE16(font->height);
		for (int i = 0; i < (int)(sizeof(font->location) / sizeof(*(font->location))); ++i)
		{
			font->location[i] = CK_Cross_SwapLE16(font->location[i]);
		}
	}
	else if (chunk >= CK_CHUNKNUM(EXTERN_COMMANDER) && chunk <= CK_CHUNKNUM(EXTERN_KEEN))
	{
		introbmptype *intro = (introbmptype *)ca_graphChunks[chunk];
		intro->height = CK_Cross_SwapLE16(intro->height);
		intro->width = CK_Cross_SwapLE16(intro->width);
		for (int i = 0; i < (int)(sizeof(intro->linestarts) / sizeof(*(intro->linestarts))); ++i)
		{
			intro->linestarts[i] = CK_Cross_SwapLE16(intro->linestarts[i]);
		}
	}
#endif
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
			MM_SetPurge((mm_ptr_t *)(&ca_graphChunks[i]), 3);
		}
	}
}

void CA_MarkGrChunk(int chunk)
{
	ca_graphChunkNeeded[chunk] |= ca_levelbit;
}

void CA_CacheMarks(const char *msg)
{
	bool isMessage = msg ? true : false;
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
				MM_SetPurge(&ca_graphChunks[i], 3);
			}
		}
	}

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "CA_CacheMarks: Caching %d chunks...\n%s\n", numChunksToCache, msg ? msg : "");

	if (!numChunksToCache)
		return;

	//Loading screen.
	if (isMessage && ca_beginCacheBox)
		ca_beginCacheBox(msg, numChunksToCache);

	// Cache all of the chunks we'll need.
	for (int i = 0; i < CA_MAX_GRAPH_CHUNKS; ++i)
	{
		if ((ca_graphChunkNeeded[i] & ca_levelbit) && (!ca_graphChunks[i]))
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

// Lump helper functions
bool *ca_lumpsNeeded;

void CA_InitLumps(void)
{
	int numLumps = CK_INT(NUMLUMPS, 0);
	MM_GetPtr((mm_ptr_t*)&ca_lumpsNeeded, sizeof(bool) * numLumps);

}

void CA_ClearLumps(void)
{
	int numLumps = CK_INT(NUMLUMPS, 0);
	memset(ca_lumpsNeeded, 0, numLumps * sizeof(bool));
}

void CA_MarkLumpNeeded(int lump)
{
#ifdef CK_DEBUG
	if (!ca_lumpsNeeded)
		Quit("CA_MarkLumpNeeded: Lump array not initialised with CA_InitLumps()!");

	if (lump < 0 || lump >= CK_INT(NUMLUMPS, 0))
		Quit("CA_MarkLumpNeeded: Tried to mark a nonexistant lump!");
#endif

	ca_lumpsNeeded[lump] = true;
}

void CA_MarkAllLumps(void)
{
	int numLumps = CK_INT(NUMLUMPS, 0);
#ifdef CK_DEBUG
	if (!ca_lumpsNeeded)
		Quit("CA_MarkAllLumps: Lump array not initialised with CA_InitLumps()!");
#endif
	for (int i = 0; i < numLumps; ++i)
	{
		if (ca_lumpsNeeded[i])
		{
			intptr_t lumpStart = CK_INTELEMENT(lumpStarts, i);
			intptr_t lumpEnd = CK_INTELEMENT(lumpEnds, i);
			for (int chunk = lumpStart; chunk <= lumpEnd; ++chunk)
				CA_MarkGrChunk(chunk);
		}
	}
}

// Map loading fns
typedef CK_PACKED_STRUCT(CA_MapHead
{
	uint16_t rleTag;
	uint32_t headerOffsets[CA_NUMMAPS];
}) CA_MapHead;

CA_MapHead *ca_MapHead;

FS_File ca_GameMaps;

CA_MapHeader *CA_MapHeaders[CA_NUMMAPS];

uint16_t *CA_mapPlanes[CA_NUMMAPPLANES];

extern uint8_t *ti_tileInfo;
void CAL_SetupMapFile(void)
{
	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "CAL_SetupMapFile: Loading map headers...\n");

	int mapHeadFileSize = 0;
	CA_LoadFile("MAPHEAD.EXT", (void **)(&ca_MapHead), &mapHeadFileSize);
#ifdef CK_CROSS_IS_BIGENDIAN
	ca_MapHead->rleTag = CK_Cross_SwapLE16(ca_MapHead->rleTag);
	for (int i = 0; i < CA_NUMMAPS; ++i)
	{
		ca_MapHead->headerOffsets[i] = CK_Cross_SwapLE32(ca_MapHead->headerOffsets[i]);
	}
#endif
	ca_GameMaps = FS_OpenKeenFile(FS_AdjustExtension("GAMEMAPS.EXT"));
	// Try reading TILEINFO.EXT first, otherwise use data from MAPHEAD.EXT
	ti_tileInfo = NULL;
	if (!CA_LoadFile("TILEINFO.EXT", (void **)(&ti_tileInfo), 0))
	{
		if (ti_tileInfo) // CA_LoadFile may leave a memory leak
			MM_FreePtr((void **)&ti_tileInfo);

		if (mapHeadFileSize <= sizeof(*ca_MapHead))
			Quit("Can't open TILEINFO file, and MAPHEAD file lacks tileinfo data!");

		ti_tileInfo = (uint8_t *)(ca_MapHead + 1);
	}
}

static ca_huffnode *ca_audiohuffman;

static FS_File ca_audiohandle; //File Pointer for AUDIO file.
int32_t *ca_audiostarts;

void CAL_SetupAudioFile(void)
{
	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "CAL_SetupAudioFile: Loading audio headers...\n");

	// Read audio chunk type info from AUDINFOE
	FS_File audinfoe = FS_OpenOmniFile(FS_AdjustExtension("AUDINFOE.EXT"));
	size_t audinfoeLen = FS_Read(&ca_audInfoE, sizeof(ca_audinfo), 1, audinfoe);
	FS_CloseFile(audinfoe);
	if (audinfoeLen != 1)
		Quit("Couldn't read AUDINFOE!");
#ifdef CK_CROSS_IS_BIGENDIAN
	uint16_t *uptr = (uint16_t *)&ca_audInfoE;
	for (size_t loopVar = 0; loopVar < sizeof(ca_audInfoE) / 2; loopVar++, uptr++)
		*uptr = CK_Cross_Swap16(*uptr);
#endif

#ifndef CA_AUDIOUNCOMPRESSED
	//Load the AUDIODCT
	CA_LoadFile("AUDIODCT.EXT", (void **)(&ca_audiohuffman), 0);

#ifdef CK_CROSS_IS_BIGENDIAN
	for (int i = 0; i < 256; ++i)
	{
		ca_audiohuffman[i].bit_0 = CK_Cross_SwapLE16(ca_audiohuffman[i].bit_0);
		ca_audiohuffman[i].bit_1 = CK_Cross_SwapLE16(ca_audiohuffman[i].bit_1);
	}
#endif

	// We don't need to 'OptimizeNodes'.
	//CAL_OptimizeNodes(ca_audiohuffman);

	//Load the AUDIOHHD
	CA_LoadFile("AUDIOHHD.EXT", (void **)(&ca_audiostarts), 0);

	//Load the sound data --- we will keep the file open for the duration of the game.
	ca_audiohandle = FS_OpenKeenFile(FS_AdjustExtension("AUDIO.EXT"));
	if (!ca_audiohandle)
	{
		Quit("Can't open AUDIO.CKx!");
	}
#else
	//Load the AUDIOHHD
	CA_LoadFile("AUDIOHED.EXT", (void **)(&ca_audiostarts), 0);

	//Load the sound data --- we will keep the file open for the duration of the game.
	ca_audiohandle = FS_OpenKeenFile(FS_AdjustExtension("AUDIOT.EXT"));
	if (!ca_audiohandle)
	{
		Quit("Can't open AUDIOT.CKx!");
	}
#endif
}

void CA_CacheMap(int mapIndex)
{
	//TODO: Support having multiple maps cached at once.
	//Unload the previous map.
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		if (CA_mapPlanes[plane])
		{
			MM_FreePtr((void **)(&CA_mapPlanes[plane]));
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

		MM_GetPtr((void **)(&CA_MapHeaders[mapIndex]), sizeof(CA_MapHeader));

		FS_SeekTo(ca_GameMaps, headerOffset);

		size_t mapHeaderSize = FS_Read(CA_MapHeaders[mapIndex], sizeof(CA_MapHeader), 1, ca_GameMaps);
		if (mapHeaderSize != 1)
			Quit("Couldn't read map header from GAMEMAPS!");
#ifdef CK_CROSS_IS_BIGENDIAN
		for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
		{
			CA_MapHeaders[mapIndex]->planeOffsets[plane] = CK_Cross_SwapLE32(CA_MapHeaders[mapIndex]->planeOffsets[plane]);
			CA_MapHeaders[mapIndex]->planeLengths[plane] = CK_Cross_SwapLE16(CA_MapHeaders[mapIndex]->planeLengths[plane]);
		}
		CA_MapHeaders[mapIndex]->width = CK_Cross_SwapLE16(CA_MapHeaders[mapIndex]->width);
		CA_MapHeaders[mapIndex]->height = CK_Cross_SwapLE16(CA_MapHeaders[mapIndex]->height);
#endif
	}
	else
	{
		//Make sure we don't purge it accidentally.
		MM_SetPurge((void **)(&CA_MapHeaders[mapIndex]), 0);
	}

	int planeSize = CA_MapHeaders[mapIndex]->width * CA_MapHeaders[mapIndex]->height * 2;

	//Load the map data
	for (int plane = 0; plane < CA_NUMMAPPLANES; ++plane)
	{
		int32_t planeOffset = CA_MapHeaders[mapIndex]->planeOffsets[plane];
		int32_t planeCompLength = CA_MapHeaders[mapIndex]->planeLengths[plane];

		MM_GetPtr((void **)&CA_mapPlanes[plane], planeSize);

		FS_SeekTo(ca_GameMaps, planeOffset);

		uint16_t *compBuffer;
		MM_GetPtr((void **)(&compBuffer), planeCompLength);
		//MM_SetLock(&compBuffer,true);

		int read = 0;
		do
		{
			int curRead = FS_Read(((uint8_t *)compBuffer) + read, 1, planeCompLength - read, ca_GameMaps);
			if (curRead < 0)
			{
				Quit("Error reading compressed map plane.");
			}
			read += curRead;
		} while (read < planeCompLength);

		uint16_t carmackExpanded = CK_Cross_SwapLE16(*compBuffer);

		uint16_t *rlewBuffer;
		MM_GetPtr((void **)(&rlewBuffer), carmackExpanded);

		//Decompress the map.
		CAL_CarmackExpand(compBuffer + 1, rlewBuffer, carmackExpanded);
		CAL_RLEWExpand(rlewBuffer + 1, CA_mapPlanes[plane], planeSize, ca_MapHead->rleTag);

		//Release the temp buffers.
		//MM_UnLockPtr(&compBuffer);
		MM_FreePtr((void **)(&compBuffer));
		MM_FreePtr((void **)(&rlewBuffer));
	}
}

// CA_Startup opens the core CA datafiles
void CA_Startup(void)
{
	// Load the ?GAGRAPH.EXT file!
	CAL_SetupGrFile();

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
		FS_CloseFile(ca_GameMaps);
	if (ca_graphHandle)
		FS_CloseFile(ca_graphHandle);
	if (ca_audiohandle)
		FS_CloseFile(ca_audiohandle);
}

uint8_t *CA_audio[CA_MAX_AUDIO_CHUNKS];

void CA_CacheAudioChunk(int16_t chunk)
{
	int32_t pos, compressed, expanded;
	mm_ptr_t bigbuffer, source;
	if (CA_audio[chunk])
	{
		MM_SetPurge((void **)(&CA_audio[chunk]), 0);
		return;
	}

	//
	// load the chunk into a buffer, either the miscbuffer if it fits, or allocate
	// a larger buffer
	//
	pos = CK_Cross_SwapLE32(ca_audiostarts[chunk]);
	compressed = CK_Cross_SwapLE32(ca_audiostarts[chunk + 1]) - pos; //+1 is not in keen...

	FS_SeekTo(ca_audiohandle, pos);

#ifdef CA_AUDIOUNCOMPRESSED
	MM_GetPtr((void**)&CA_audio[chunk], compressed);
	FS_Read(CA_audio[chunk], 1, compressed, ca_audiohandle);
#else
	if (compressed <= BUFFERSIZE)
	{
		size_t readSize = FS_Read(buffer, 1, compressed, ca_audiohandle);
		if (readSize != compressed)
			Quit("Couldn't read compressed audio chunk!");
		source = buffer;
	}
	else
	{
		MM_GetPtr(&bigbuffer, compressed);
		// TODO: Check for mmerror
#if 0
		if (mmerror)
			return;
#endif
		MM_SetLock(&bigbuffer, true);
		size_t readSize = FS_Read(bigbuffer, 1, compressed, ca_audiohandle);
		source = bigbuffer;
		if (readSize != compressed)
			Quit("Couldn't read compressed audio chunk!");
	}

	expanded = CAL_ReadLong(source);
	source = (mm_ptr_t)((uint8_t *)source + 4); // skip over length
	MM_GetPtr((void **)(&CA_audio[chunk]), expanded);
	// TODO: Check for mmerror
#if 0
	if (mmerror)
		goto done;
#endif
	CAL_HuffExpand(source, CA_audio[chunk], expanded, ca_audiohuffman, compressed);

	//done:
	if (compressed > BUFFERSIZE)
		MM_FreePtr(&bigbuffer);
#endif
}

void CA_LoadAllSounds(void)
{
	int16_t offset;
	bool unload = false;
	uint16_t loopvar;

	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "CA_LoadAllSounds: Switching from sound mode %d → %d\n", oldsoundmode, SD_GetSoundMode());

	switch (oldsoundmode)
	{
	case sdm_Off:
		break;
	case sdm_PC:
		offset = ca_audInfoE.startPCSounds; // STARTPCSOUNDS;
		unload = true;
		break;
	case sdm_AdLib:
		offset = ca_audInfoE.startAdlibSounds; //STARTADLIBSOUNDS;
		unload = true;
		break;
	default:
		// Shut up some gcc warnings.
		break;
	}

	if (unload)
	{
		for (loopvar = 0; loopvar < ca_audInfoE.numSounds; loopvar++, offset++)
		{
			if (CA_audio[offset])
			{
				MM_SetPurge((void **)(&CA_audio[offset]), 3);
			}
		}
	}

	switch (SD_GetSoundMode())
	{
	case sdm_Off:
		return;
	case sdm_PC:
		offset = ca_audInfoE.startPCSounds; // STARTPCSOUNDS;
		break;
	case sdm_AdLib:
		offset = ca_audInfoE.startAdlibSounds; // STARTADLIBSOUNDS;
		break;
	default:
		// Shut up some gcc warnings.
		return;
	}
	for (loopvar = 0; loopvar < ca_audInfoE.numSounds; loopvar++, offset++)
	{
		CA_CacheAudioChunk(offset);
	}
}

//TODO: Make this less of an ugly hack.
//TODO (Mar 6 2014): ok, no need anymore? (ca_mapOn is used here now)
//extern int ck_currentMapNumber;
//UPDATE (Mar 6 2014): The var above is now ck_gameState.currentLevel.

uint16_t *CA_TilePtrAtPos(int16_t x, int16_t y, int16_t plane)
{
	return &CA_mapPlanes[plane][y * CA_MapHeaders[ca_mapOn]->width + x];
}

uint16_t CA_TileAtPos(int16_t x, int16_t y, int16_t plane)
{
	// HACK - Somewhat reproducing a glitch occuring in Keen 4: Level 10,
	// when an object goes through the top of the map
	// (not reproduced in the exact same manner)
	CA_MapHeader *mapheader = CA_MapHeaders[ca_mapOn];
	if ((x >= 0) && (x < mapheader->width) && (y >= 0) && (y < mapheader->height))
		return CA_mapPlanes[plane][y * mapheader->width + x];
	return (y * x * y * x) % (ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 6);
}

void CA_SetTileAtPos(int16_t x, int16_t y, int16_t plane, uint16_t value)
{
	CA_mapPlanes[plane][y * CA_MapHeaders[ca_mapOn]->width + x] = value;
}

uint16_t CA_GetMapWidth()
{
	return CA_MapHeaders[ca_mapOn]->width;
}

uint16_t CA_GetMapHeight()
{
	return CA_MapHeaders[ca_mapOn]->height;
}
