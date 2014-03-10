#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "ck_cross.h"
#include "id_in.h"
#include "id_sd.h"

#ifdef CK_RAND_DEBUG
#include <execinfo.h>
#endif

void CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...)
{
	// TODO: For now we simply do this.
	va_list args;
	va_start(args, format);
	switch (msgClass)
	{
	case CK_LOG_MSG_NORMAL:
		vprintf(format, args);
		break;
	case CK_LOG_MSG_WARNING:
		fprintf(stderr, "Warning: ");
		vfprintf(stderr, format, args);
		break;
	case CK_LOG_MSG_ERROR:
		fprintf(stderr, "Error: ");
		vfprintf(stderr, format, args);
		break;
	}
	va_end(args);
}

#ifdef CK_RAND_DEBUG

void CK_Cross_StackTrace()
{
	int numFunctions;
	char **strings;
	void *buffer[100];

	numFunctions = backtrace(buffer, 100);
	strings = backtrace_symbols(buffer, numFunctions);
	for (int i = 0; i < numFunctions; ++i)
	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "[BT] %s\n", strings[i]);
}

#endif

void CK_Cross_puts(const char *str)
{
	// Reason for this wrapper: Maybe a different
	// mechanism will be used in the future.
	puts(str);
}

int CK_Cross_toupper(int c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}

int CK_Cross_strcasecmp(const char *s1, const char *s2)
{
	unsigned char uc1, uc2;
	/* This one is easy. We don't care if a value is signed or not. */
	/* All that matters here is consistency (everything is signed). */
	for (; (*s1) && (CK_Cross_toupper(*s1) == CK_Cross_toupper(*s2)); s1++, s2++);
	/* But now, first we cast from int to char, and only *then* to */
	/* unsigned char, so the correct difference can be calculated. */
	uc1 = (unsigned char)((char)(CK_Cross_toupper(*s1)));
	uc2 = (unsigned char)((char)(CK_Cross_toupper(*s2)));
	/* We should still cast back to int, for a signed difference. */
	/* Assumption: An int can store any unsigned char value.      */
	return ((int)uc1 - (int)uc2);
}

size_t CK_Cross_freadInt8LE(void *ptr, size_t count, FILE *stream)
{
	return fread(ptr, 1, count, stream);
}

size_t CK_Cross_freadInt16LE(void *ptr, size_t count, FILE *stream)
{
	count = fread(ptr, 2, count, stream);
#ifdef CK_CROSS_IS_BIGENDIAN
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint16_t *) ptr)++)
		*(uint16_t *) ptr = CK_Cross_Swap16(*(uint16_t *) ptr);
#endif
	return count;
}

size_t CK_Cross_freadInt32LE(void *ptr, size_t count, FILE *stream)
{
	count = fread(ptr, 4, count, stream);
#ifdef CK_CROSS_IS_BIGENDIAN
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint32_t *) ptr)++)
		*(uint32_t *) ptr = CK_Cross_Swap32(*(uint32_t *) ptr);
#endif
	return count;
}

size_t CK_Cross_fwriteInt8LE(const void *ptr, size_t count, FILE *stream)
{
	return fwrite(ptr, 1, count, stream);
}

size_t CK_Cross_fwriteInt16LE(const void *ptr, size_t count, FILE *stream)
{
#ifndef CK_CROSS_IS_BIGENDIAN
	return fwrite(ptr, 2, count, stream);
#else
	uint16_t val;
	size_t actualCount = 0;
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint16_t *) ptr)++)
	{
		val = CK_Cross_Swap16(*(uint16_t *) ptr);
		actualCount += fwrite(&val, 2, 1, stream);
	}
	return actualCount;
#endif
}

size_t CK_Cross_fwriteInt32LE(const void *ptr, size_t count, FILE *stream)
{
#ifndef CK_CROSS_IS_BIGENDIAN
	return fwrite(ptr, 4, count, stream);
#else
	uint32_t val;
	size_t actualCount = 0;
	for (size_t loopVar = 0; loopVar < count; loopVar++, ((uint32_t *) ptr)++)
	{
		val = CK_Cross_Swap32(*(uint32_t *) ptr);
		actualCount += fwrite(&val, 4, 1, stream);
	}
	return actualCount;
#endif
}

size_t CK_Cross_freadBoolFrom16LE(void *ptr, size_t count, FILE *stream)
{
	uint16_t val;
	size_t actualCount = 0;
	bool *currBoolPtr = (bool *)ptr; // No lvalue compilation error
	for (size_t loopVar = 0; loopVar < count; loopVar++, currBoolPtr++)
	{
		if (fread(&val, 2, 1, stream)) // Should be either 0 or 1
		{
			*currBoolPtr = (val); // NOTE: No need to byte-swap
			actualCount++;
		}
	}
	return actualCount;
}

size_t CK_Cross_fwriteBoolTo16LE(const void *ptr, size_t count, FILE *stream)
{
	uint16_t val;
	size_t actualCount = 0;
	bool *currBoolPtr = (bool *)ptr; // No lvalue compilation error
	for (size_t loopVar = 0; loopVar < count; loopVar++, currBoolPtr++)
	{
		val = CK_Cross_SwapLE16((*currBoolPtr) ? 1 : 0);
		actualCount += fwrite(&val, 2, 1, stream);
	}
	return actualCount;
}

#if 0
/*** Beginning of template implementation of enum I/O ***/

#define CK_CROSS_IMPLEMENT_FP_READWRITE_8LE_FUNCS(ourSampleEnum) \
size_t CK_Cross_fread_ ## ourSampleEnum ## _From8LE (void *ptr, size_t count, FILE *stream) \
{ \
	uint8_t val; \
	size_t actualCount = 0; \
	ourSampleEnum *currEnumPtr = (ourSampleEnum *)ptr; /* No lvalue compilation error */ \
	for (size_t loopVar = 0; loopVar < count; loopVar++, currEnumPtr++) \
	{ \
		if (fread(&val, 1, 1, stream)) /* Should be either 0 or 1 */ \
		{ \
			*currEnumPtr = (ourSampleEnum)val; \
			actualCount++; \
		} \
	} \
	return actualCount; \
} \
\
size_t CK_Cross_fwrite_ ## ourSampleEnum ## _To8LE (const void *ptr, size_t count, FILE *stream) \
{ \
	uint8_t val; \
	size_t actualCount = 0; \
	ourSampleEnum *currEnumPtr = (ourSampleEnum *)ptr; /* No lvalue compilation error */ \
	for (size_t loopVar = 0; loopVar < count; loopVar++, currEnumPtr++) \
	{ \
		val = (uint8_t)(*currEnumPtr); \
		actualCount += fwrite(&val, 1, 1, stream); \
	} \
	return actualCount; \
} \

/*** End of template implementation of enum I/O ***/
#endif
