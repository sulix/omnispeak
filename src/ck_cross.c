#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "id_in.h"
#include "id_sd.h"
#include "ck_cross.h"

#ifdef CK_RAND_DEBUG
#include <execinfo.h>
#endif

void CK_PRINTF_FORMAT(2, 3) CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...)
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
	for (; (*s1) && (CK_Cross_toupper(*s1) == CK_Cross_toupper(*s2)); s1++, s2++)
		;
	/* But now, first we cast from int to char, and only *then* to */
	/* unsigned char, so the correct difference can be calculated. */
	uc1 = (unsigned char)((char)(CK_Cross_toupper(*s1)));
	uc2 = (unsigned char)((char)(CK_Cross_toupper(*s2)));
	/* We should still cast back to int, for a signed difference. */
	/* Assumption: An int can store any unsigned char value.      */
	return ((int)uc1 - (int)uc2);
}
