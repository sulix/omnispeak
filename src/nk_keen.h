#ifndef NK_KEEN_H
#define NK_KEEN_H

void NK_AddSpawnPoint(int tileX, int tileY, int direction, int team);
void NK_SpawnKeen(int playerId, int spawnId);
void NK_KillKeen(CK_object *obj);

void NK_KeenSetupFunctions();
void NK_KeenCheckSpecialTileInfo(CK_object *obj);

#endif // NK_KEEN_H
