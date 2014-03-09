// Tools and definitions assisting with general cross-platform support

#ifndef CK_CROSS_H
#define CK_CROSS_H

#include "SDL.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define CK_CROSS_IS_BIGENDIAN
#endif

#define CK_Cross_Swap16 SDL_Swap16
#define CK_Cross_Swap32 SDL_Swap32
#define CK_Cross_SwapLE16 SDL_SwapLE16
#define CK_Cross_SwapLE32 SDL_SwapLE32

typedef enum CK_Log_Message_Class_T {
	CK_LOG_MSG_NORMAL, CK_LOG_MSG_WARNING, CK_LOG_MSG_ERROR
} CK_Log_Message_Class_T;

// Used for debugging
void CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...);

// Emulates the functionality of the "puts" function in text mode
void CK_Cross_puts(const char *str);

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

// Similar functions for enum <-> 8-bit conversions, given as a template
// (Declarations only; Implementations should be done in ck_cross.c)
#define CK_CROSS_DECLARE_FP_READWRITE_8LE_FUNCS(ourSampleEnum) \
size_t CK_Cross_fread_ ## ourSampleEnum ## _From8LE (void *ptr, size_t count, FILE *stream); \
size_t CK_Cross_fwrite_ ## ourSampleEnum ## _To8LE (const void *ptr, size_t count, FILE *stream);
// End of template

#endif
