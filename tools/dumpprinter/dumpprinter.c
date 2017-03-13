#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*** Some stuff was copied-and-pasted-and-modified from Omnispeak. ***/
/*** ALL fields are printed as unsigned, making it possible to     ***/
/*** read and print each object as an array of 16-bit values.      ***/

/*** WARNING - Only little-endian is supported! ***/

static size_t CK_Cross_freadInt16LE(void *ptr, size_t count, FILE *stream)
{
	count = fread(ptr, 2, count, stream);
#if 0
//#ifdef CK_CROSS_IS_BIGENDIAN
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint16_t *) ptr)++)
		*(uint16_t *) ptr = CK_Cross_Swap16(*(uint16_t *) ptr);
#endif
	return count;
}

static size_t CK_Cross_freadInt32LE(void *ptr, size_t count, FILE *stream)
{
	count = fread(ptr, 4, count, stream);
#if 0
//#ifdef CK_CROSS_IS_BIGENDIAN
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint32_t *) ptr)++)
		*(uint32_t *) ptr = CK_Cross_Swap32(*(uint32_t *) ptr);
#endif
	return count;
}


typedef struct CK_GameState
{
	uint16_t mapPosX;
	uint16_t mapPosY;
	uint16_t levelsDone[25];	// Number of levels complete
	uint32_t/*int32_t*/ keenScore;		// Keen's score. (This _is_ signed, by the looks of all the 'jl' instructions)
	uint32_t/*int32_t*/ nextKeenAt;		// Score keen will get a new life at.
	uint16_t/*int16_t*/ numShots;
	uint16_t/*int16_t*/ numCentilife;

	// The episode-specific variables come here
	// They were probably conditionally compiled in the DOS version
	// so that the Gamestate struct is variably sized between eps.
	union
	{
		struct
		{
			uint16_t/*int16_t*/ wetsuit;
			uint16_t/*int16_t*/ membersRescued;
		} ck4;

		struct
		{
			uint16_t/*int16_t*/ securityCard;
			uint16_t/*int16_t*/ word_4729C;
			uint16_t/*int16_t*/ fusesRemaining;
		} ck5;

		struct
		{
			uint16_t/*int16_t*/ sandwich;
			uint16_t/*int16_t*/ rope;	// 1 == collected, 2 == deployed on cliff
			uint16_t/*int16_t*/ passcard;
			uint16_t/*int16_t*/ inRocket; // true if flying
		} ck6;
	} ep;

	uint16_t/*int16_t*/ keyGems[4];
	uint16_t/*int16_t*/ currentLevel;
	uint16_t/*int16_t*/ numLives;			// Number of lives keen has.
	uint16_t/*int16_t*/ difficulty;		// Difficulty level of current game
	uint16_t platformOffset;	// This was a 16-bit pointer in DOS Keen5.exe
} CK_GameState;

/*** NOTE: There's no need for CK_object, we simply read object as an array ***/

static bool processTimeCount(FILE *fp)
{
	uint32_t timecount;
	if (CK_Cross_freadInt32LE(&timecount, 1, fp) != 1)
		return false;
	printf("Timecount: %08X\n", timecount);
	return true;
}

static int g_episodeNum;

static bool processGameState(FILE *fp)
{
	CK_GameState state;
	int i;

	if ((CK_Cross_freadInt16LE(&state.mapPosX, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.mapPosY, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(state.levelsDone, sizeof(state.levelsDone)/2, fp) != sizeof(state.levelsDone)/2)
	    || (CK_Cross_freadInt32LE(&state.keenScore, 1, fp) != 1)
	    || (CK_Cross_freadInt32LE(&state.nextKeenAt, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.numShots, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.numCentilife, 1, fp) != 1)
	    || ((g_episodeNum == 4) &&
	       (
	        (CK_Cross_freadInt16LE(&state.ep.ck4.wetsuit, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck4.membersRescued, 1, fp) != 1)
	     )
	    )
	    || ((g_episodeNum == 5) &&
	       (
	        (CK_Cross_freadInt16LE(&state.ep.ck5.securityCard, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck5.word_4729C, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck5.fusesRemaining, 1, fp) != 1)
	     )
	    )
	    || ((g_episodeNum == 6) &&
	       (
	        (CK_Cross_freadInt16LE(&state.ep.ck6.sandwich, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck6.rope, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck6.passcard, 1, fp) != 1)
	        || (CK_Cross_freadInt16LE(&state.ep.ck6.inRocket, 1, fp) != 1)
	     )
	    )
	    || (CK_Cross_freadInt16LE(state.keyGems, sizeof(state.keyGems)/2, fp) != sizeof(state.keyGems)/2)
	    || (CK_Cross_freadInt16LE(&state.currentLevel, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.numLives, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.difficulty, 1, fp) != 1)
	    || (CK_Cross_freadInt16LE(&state.platformOffset, 1, fp) != 1) // BACKWARDS COMPATIBILITY
	)
		return false;

	printf("Gamestate: %04X %04X", state.mapPosX, state.mapPosY);

	for (i = 0; i < 25; ++i)
		printf(" %04X", state.levelsDone[i]);

	printf(" %08X %08X %04X %04X", state.keenScore, state.nextKeenAt, state.numShots, state.numCentilife);

	if (g_episodeNum == 4)
		printf(" %04X %04X", state.ep.ck4.wetsuit, state.ep.ck4.membersRescued);
	else if (g_episodeNum == 5)
		printf(" %04X %04X %04X", state.ep.ck5.securityCard, state.ep.ck5.word_4729C, state.ep.ck5.fusesRemaining);
	else if (g_episodeNum == 6)
		printf(" %04X %04X %04X %04X", state.ep.ck6.sandwich, state.ep.ck6.rope, state.ep.ck6.passcard, state.ep.ck6.inRocket);

	for (i = 0; i < 4; ++i)
		printf(" %04X", state.keyGems[i]);

	printf(" %04X %04X %04X %04X\n", state.currentLevel, state.numLives, state.difficulty, state.platformOffset);

	return true;
}

static bool processObjects(FILE *fp)
{
	uint16_t objFields[38];
	int i, j;
	for (j = 0; j < 100; ++j)
	{
		if (CK_Cross_freadInt16LE(&objFields, 38, fp) != 38)
			return false;

		printf("Obj %02X:", j);
		for (i = 0; i < 38; ++i)
			printf(" %04X", objFields[i]);
		printf("\n");
	}
	return true;
}

int main(int argc, char **argv)
{
	// Minor little-endian test
	uint16_t someUInt16 = 0x1234;
	uint8_t *someUint8Ptr = (uint8_t *)&someUInt16;
	if ((someUint8Ptr[0] != 0x34) || (someUint8Ptr[1] != 0x12))
	{
		fprintf(stderr, "FATAL ERROR - Only Little-Endian architectures are supported!\n");
		return 2;
	}

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <inFile> <episode>\n", argv[0]);
		return 0;
	}
	g_episodeNum = atoi(argv[2]);
	if ((g_episodeNum < 4) || (g_episodeNum > 6))
	{
		fprintf(stderr, "<episode> argument must be a number in the range 4-6.\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "ERROR - Cannot open file %s for reading\n", argv[1]);
		return 1;
	}

	while (processTimeCount(fp) && processGameState(fp) && processObjects(fp))
		;

	fclose(fp);
	return 0;
}
