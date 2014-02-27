// Tools and definitions assisting with general cross-platform support

#ifndef CK_CROSS_H
#define CK_CROSS_H

typedef enum CK_Log_Message_Class_T {
	CK_LOG_MSG_NORMAL, CK_LOG_MSG_WARNING, CK_LOG_MSG_ERROR
} CK_Log_Message_Class_T;

// Used for debugging
void CK_Cross_LogMessage(CK_Log_Message_Class_T msgClass, const char *format, ...);

// Emulates the functionality of the "puts" function in text mode
void CK_Cross_puts(const char *str);

#endif
