#include <stdarg.h>
#include <stdio.h>
#include "ck_cross.h"

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
