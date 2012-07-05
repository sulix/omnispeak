#ifndef CK5_EP_H
#define CK5_EP_H

#include <stdbool.h>

/*
 * Contains definitions relevent only to Keen 5
 */

/* Action functions setup */
void CK5_Obj1_SetupFunctions();
void CK5_Obj3_SetupFunctions();
void CK5_SetupFunctions();

void CK5_ScanInfoLayer();


/* Spawning functions */

/* ck5_misc.c */
void CK5_SpawnRedBlockPlatform(int tileX, int tileY, int direction, bool purple);
void CK5_SpawnItem(int tileX, int tileY, int itemNumber);

/* ck5_obj1.c */
void CK5_TurretSpawn(int tileX, int tileY, int direction);

/* ck5_obj3.c */
void CK5_SpawnSpirogrip(int tileX, int tileY);
void CK5_SpawnKorath(int tileX, int tileY);
#endif
