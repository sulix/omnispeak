// Tools and definitions assisting with general cross-platform support

#ifndef CK_CROSS_H
#define CK_CROSS_H

#include "SDL.h"
#include <stdint.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define CK_CROSS_IS_BIGENDIAN
#elif (SDL_BYTEORDER == SDL_LIL_ENDIAN)
#define CK_CROSS_IS_LITTLEENDIAN
#else
#error "ck_cross.h - Unknown platform endianness!"
#endif

#define CK_Cross_Swap16(x) ((uint16_t)(((uint16_t)(x)<<8)|((uint16_t)(x)>>8)))

#define CK_Cross_Swap32(x) ((uint32_t)(((uint32_t)(x)<<24)|(((uint32_t)(x)<<8)&0x00FF0000)|(((uint32_t)(x)>>8)&0x0000FF00)|((uint32_t)(x)>>24)))

#ifdef CK_CROSS_IS_LITTLEENDIAN
#define CK_Cross_SwapLE16(x) (x)
#define CK_Cross_SwapLE32(x) (x)
#else
#define CK_Cross_SwapLE16(x) CK_Cross_Swap16(x)
#define CK_Cross_SwapLE32(x) CK_Cross_Swap32(x)
#endif

typedef enum CK_Log_Message_Class_T {
	CK_LOG_MSG_NORMAL, CK_LOG_MSG_WARNING, CK_LOG_MSG_ERROR
} CK_Log_Message_Class_T;

// Used for debugging
void CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...);

// Emulates the functionality of the "puts" function in text mode
void CK_Cross_puts(const char *str);

// More standard C functions emulated,
// taking English locale into account (and more, but NOT all)
int CK_Cross_toupper(int c);
// A bit less standard, but still done assuming English locale
int CK_Cross_strcasecmp(const char *s1, const char *s2);

// Used for reading buffers of a specific type, assuming Little-Endian
// byte order in the file's data itself. It gets converted to native order.
size_t CK_Cross_freadInt8LE(void *ptr, size_t count, FILE *stream);
size_t CK_Cross_freadInt16LE(void *ptr, size_t count, FILE *stream);
size_t CK_Cross_freadInt32LE(void *ptr, size_t count, FILE *stream);
// Used for writing buffers of a specific type, converting
// native byte order to Little-Endian order within the file.
size_t CK_Cross_fwriteInt8LE(const void *ptr, size_t count, FILE *stream);
size_t CK_Cross_fwriteInt16LE(const void *ptr, size_t count, FILE *stream);
size_t CK_Cross_fwriteInt32LE(const void *ptr, size_t count, FILE *stream);
// Similar methods for reading/writing bools from/to int16_t
// (0 as false, 1 as true and any nonzero as true for reading.)
// TODO: Maybe int16_t's should be used internally? (Same as vanilla Keen.)
size_t CK_Cross_freadBoolFrom16LE(void *ptr, size_t count, FILE *stream);
size_t CK_Cross_fwriteBoolTo16LE(const void *ptr, size_t count, FILE *stream);

#if 0
// Similar functions for enum <-> 8-bit conversions, given as a template
// (Declarations only; Implementations should be done in ck_cross.c)
#define CK_CROSS_DECLARE_FP_READWRITE_8LE_FUNCS(ourSampleEnum) \
size_t CK_Cross_fread_ ## ourSampleEnum ## _From8LE (void *ptr, size_t count, FILE *stream); \
size_t CK_Cross_fwrite_ ## ourSampleEnum ## _To8LE (const void *ptr, size_t count, FILE *stream);
// End of template
#endif

#endif
