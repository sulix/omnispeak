#ifndef CK_PLAY_H
#define CK_PLAY_H

#include <stdbool.h>

typedef struct CK_object CK_object;

// Timing
void CK_SetTicsPerFrame();
int CK_GetTicksPerFrame();
long CK_GetNumTotalTics();

// Object Mgmt
CK_object *CK_GetNewObj(bool nonCritical);
void CK_SetupObjArray();
void CK_RemoveObj(CK_object *obj);

// Actions/Camera
void CK_RunAction(CK_object *obj);
void CK_CentreCamera(CK_object *obj);
void CK_NormalCamera(CK_object *obj);

// Playing
void CK_PlayDemo(int demoChunk);
int CK_PlayLoop();

#endif //!CK_PLAY_H
