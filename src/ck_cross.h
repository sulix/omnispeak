// Tools and definitions assisting with general cross-platform support

#ifndef CK_CROSS_H
#define CK_CROSS_H

#include <stdint.h>
#include <stdio.h>

#ifdef WITH_SDL
#if WITH_SDL == 3
#include <SDL3/SDL.h>
#else
#include "SDL.h"
#endif
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define CK_CROSS_IS_BIGENDIAN
#elif (SDL_BYTEORDER == SDL_LIL_ENDIAN)
#define CK_CROSS_IS_LITTLEENDIAN
#else
#error "ck_cross.h - Unknown platform endianness!"
#endif
#else // WITH_SDL
#if defined(__LITTLE_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define CK_CROSS_IS_LITTLEENDIAN
#elif defined(__BIG_ENDIAN__) || (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define CK_CROSS_IS_BIGENDIAN
#else
#error "cl_cross.h - Couldn't determine platform endianness!"
#endif
#endif

#define CK_Cross_Swap16(x) ((uint16_t)(((uint16_t)(x) << 8) | ((uint16_t)(x) >> 8)))

#define CK_Cross_Swap32(x) ((uint32_t)(((uint32_t)(x) << 24) | (((uint32_t)(x) << 8) & 0x00FF0000) | (((uint32_t)(x) >> 8) & 0x0000FF00) | ((uint32_t)(x) >> 24)))

#ifdef CK_CROSS_IS_LITTLEENDIAN
#define CK_Cross_SwapLE16(x) (x)
#define CK_Cross_SwapLE32(x) (x)
#else
#define CK_Cross_SwapLE16(x) CK_Cross_Swap16(x)
#define CK_Cross_SwapLE32(x) CK_Cross_Swap32(x)
#endif

// We rely on the exact memory layout of some structures, which is not
// standardised between compilers. This macro 'packs' a structure, and
// should be implemented on all compilers we support.
// Use it by wrapping the struct definition in the macro, e.g.:
// 	typedef struct a { uint16_t a; uint32_t b; } a;
// should become:
// 	typedef CK_PACKED_STRUCT(a { uint16_t; uint32_t b; }) a;
// This slightly weird syntax is required to make this work with some
// compilers' weird pragmas/attributes for structure packing.
#if defined(__GNUC__)
#define CK_PACKED_STRUCT(...) struct __VA_ARGS__ __attribute__((packed))
#elif defined(_MSC_VER)
#define CK_PACKED_STRUCT(...) __pragma(pack(push,1)) struct __VA_ARGS__ __pragma(pack(pop))
#elif defined(__WATCOMC__)
#define CK_PACKED_STRUCT(...) _Packed struct __VA_ARGS__
#else
#error Unknown compiler, no packed struct support
#endif


// Add printf format-string warnings on compilers which support them
#if defined(__GNUC__)
#define CK_PRINTF_FORMAT(fmtstridx, firstarg) __attribute__ ((format (printf, fmtstridx, firstarg)))
#else
#define CK_PRINTF_FORMAT(fmtstridx, firstarg)
#endif

typedef enum CK_Log_Message_Class_T
{
	CK_LOG_MSG_NORMAL,
	CK_LOG_MSG_WARNING,
	CK_LOG_MSG_ERROR
} CK_Log_Message_Class_T;

extern const char *ck_cross_logLevel_strings[];

// Log level.
extern CK_Log_Message_Class_T ck_cross_logLevel;

// Used for debugging
void CK_PRINTF_FORMAT(2, 3) CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...);

// Emulates the functionality of the "puts" function in text mode
void CK_Cross_puts(const char *str);

// More standard C functions emulated,
// taking English locale into account (and more, but NOT all)
int CK_Cross_toupper(int c);
// A bit less standard, but still done assuming English locale
int CK_Cross_strcasecmp(const char *s1, const char *s2);
int CK_Cross_strncasecmp(const char *s1, const char *s2, size_t n);

// The C standard library doesn't have an implementation of min/max, which is sad.
#define CK_Cross_max(x, y) ((x) < (y) ? (y) : (x))
#define CK_Cross_min(x, y) ((x) > (y) ? (y) : (x))

// Let's have our own definiton of ABS() as well
#define CK_Cross_abs(x) ((x) > 0 ? (x) : (-(x)))

// Safe strcpy variant which Quit()s if the buffer is too small.
size_t CK_Cross_strscpy(char* dst, const char* src, size_t bufsiz);

#endif
