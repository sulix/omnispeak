#ifndef CK_ACT_H
#define CK_ACT_H

#include "ck_def.h"

typedef void (*CK_ACT_Function)(CK_object *obj);
typedef void (*CK_ACT_ColFunction)(CK_object *obj1, CK_object *obj2);


void CK_ACT_SetupFunctions();
void CK_ACT_AddFunction(const char *fnName, CK_ACT_Function fn);
void CK_ACT_AddColFunction(const char *fnName, CK_ACT_ColFunction fn);
CK_ACT_Function CK_ACT_GetFunction(const char *fnName);
CK_ACT_ColFunction CK_ACT_GetColFunction(const char *fnName);
CK_action *CK_GetActionByName(const char *name);
CK_action *CK_GetOrCreateActionByName(const char *name);
void CK_ACT_LoadActions(char *filename);
#endif
