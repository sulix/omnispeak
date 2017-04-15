#include "id_rf.h"
#include "id_sd.h"
#include "ck_def.h"
#include "ck_act.h"
#include "ck_play.h"
#include "ck_game.h"
#include "ck_net.h"
#include "nk_keen.h"

#include <SDL.h>

#include <string.h>
#include <enet/enet.h>


int net_numPlayers;
int net_level;

CK_NetMode net_mode;
CK_NetGameState *net_state;

CK_object tempObj;

void Net_PlaySound(soundnames sound)
{
    if (net_mode == Net_Server)
        return;

    SD_PlaySound(sound);
}


CK_TicInput Net_InputFrameToTicInput(IN_ControlFrame cf)
{
    CK_TicInput ti;
    ti.jump = cf.jump;
    ti.pogo = cf.pogo;
    ti.button2 = cf.button2;
    ti.button3 = cf.button3;
    ti.xDirection = (int)cf.xDirection;
    ti.yDirection = (int)cf.yDirection;
    return ti;
}

IN_ControlFrame Net_TicInputToControlFrame(CK_TicInput ti)
{
    IN_ControlFrame cf;
    cf.jump = ti.jump;
    cf.pogo = ti.pogo;
    cf.button2 = ti.button2;
    cf.button3 = ti.button3;
    cf.xDirection = (IN_Motion)ti.xDirection;
    cf.yDirection = (IN_Motion)ti.yDirection;
    return cf;
}

void CK_Net_SetupObjArray()
{
    for (int i = 0; i < CK_MAX_OBJECTS; ++i)
    {
        net_state->objArray[i].prev = &(net_state->objArray[i + 1]);
        net_state->objArray[i].next = 0;
    }

    net_state->objArray[CK_MAX_OBJECTS - 1].prev = 0;
    net_state->freeObject = &net_state->objArray[0];
    net_state->lastObject = 0;
    net_state->numObjects = 0;

    // Dummy object for the list head
    net_state->firstObject = CK_Net_GetNewObj(false);
    CK_SetAction(net_state->firstObject, CK_GetActionByName("NK_ACT_NULL"));

    for (int i = 0; i < net_state->numPlayers; i++)
    {
        CK_object *obj = CK_Net_GetNewObj(false);
        net_state->playerStates[i].obj = obj;
        obj->user4 = i;
    }
}

CK_object *CK_Net_GetNewObj(bool nonCritical)
{
    if (!net_state->freeObject)
    {
        if (nonCritical)
        {
            //printf("Warning: No free spots in objarray! Using temp object\n");
            return &tempObj;
        }
        else
            Quit("GetNewObj: No free spots in objarray!");
    }

    CK_object *newObj = net_state->freeObject;
    net_state->freeObject = net_state->freeObject->prev;

    //Clear any old crap out of the struct.
    memset(newObj, 0, sizeof (CK_object));

    if (net_state->lastObject)
    {
        net_state->lastObject->next = newObj;
    }
    newObj->prev = net_state->lastObject;

    newObj->active = OBJ_ACTIVE;
    newObj->clipped = CLIP_normal;

    net_state->lastObject = newObj;
    net_state->numObjects++;

    return newObj;
}

void CK_Net_RemoveObj(CK_object *obj)
{
    if (obj == net_state->firstObject)
    {
        Quit("RemoveObj: Tried to remove the objlist head!");
    }

    RF_RemoveSpriteDraw(&obj->sde);

    if (obj == net_state->lastObject)
        net_state->lastObject = obj->prev;
    else
        obj->next->prev = obj->prev;

    obj->prev->next = obj->next;

    obj->prev = net_state->freeObject;
    net_state->freeObject = obj;
    net_state->numObjects--;
}

void Net_SetupRespawnArray()
{
    for (int i = 0; i < NET_MAX_RESPAWNS; ++i)
    {
        net_state->respawnArray[i].next = &(net_state->respawnArray[i + 1]);
        net_state->respawnArray[i].prev = &(net_state->respawnArray[i - 1]);
    }
    net_state->respawnArray[0].prev = &net_state->respawnArray[NET_MAX_RESPAWNS - 1];
    net_state->respawnArray[NET_MAX_RESPAWNS - 1].next = &net_state->respawnArray[0];

    net_state->firstRespawn = &net_state->respawnArray[0];
    net_state->freeRespawn = &net_state->respawnArray[0];
    net_state->numRespawns = 0;
}

Net_Respawn *Net_GetNewRespawn()
{
    if (net_state->numRespawns == NET_MAX_RESPAWNS)
    {
        Quit("GetNewRespawn: No free spots in respawnarray!");
    }

    Net_Respawn *newSpawn = net_state->freeRespawn;
    net_state->freeRespawn = net_state->freeRespawn->next;
    memset(newSpawn, 0, sizeof (Net_Respawn));
    net_state->numRespawns++;

    return newSpawn;
}

void Net_RemoveRespawn(Net_Respawn *respawn)
{
    Net_Respawn *last = net_state->freeRespawn->prev;
    Net_Respawn *free = net_state->freeRespawn;
    Net_Respawn *prev = respawn->prev;
    Net_Respawn *next = respawn->next;
    Net_Respawn *first;

    if (net_state->numRespawns == 1)
    {
        net_state->firstRespawn = net_state->freeRespawn;
    }
    else
    {
        if (net_state->firstRespawn == respawn)
        {
            net_state->firstRespawn = net_state->firstRespawn->next;
        }

        prev->next = next;
        next->prev = prev;

        last->next = respawn;
        free->prev = respawn;

        respawn->next = free;
        respawn->prev = last;

        net_state->freeRespawn = respawn;
    }

    net_state->numRespawns--;
}

extern int16_t CK_ActionThink(CK_object *obj, int16_t time);
void CK_Net_RunAction(CK_object *obj)
{
    int16_t oldChunk = obj->gfxChunk;

    //TODO: Check these
    //int16_t oldPosX = obj->posX;
    //int16_t oldPosY = obj->posY;

    obj->deltaPosX = obj->deltaPosY = 0;

    ck_nextX = ck_nextY = 0;

    CK_action *prevAction = obj->currentAction;

    int16_t ticsLeft = CK_ActionThink(obj, SD_GetSpriteSync());

    if (obj->currentAction != prevAction)
    {
        obj->actionTimer = 0;
        prevAction = obj->currentAction;
    }
    while (ticsLeft)
    {
        if (prevAction->protectAnimation || prevAction->timer > ticsLeft)
        {
            ticsLeft = CK_ActionThink(obj, ticsLeft);
        }
        else
        {
            ticsLeft = CK_ActionThink(obj, prevAction->timer - 1);
        }
        // Do NOT use prevAction here
        if (obj->currentAction != prevAction)
        {
            obj->actionTimer = 0;
            prevAction = obj->currentAction;
        }
    }

    if (!prevAction)
    {
        CK_Net_RemoveObj(obj);
        return;
    }
    if (prevAction->chunkRight)
    {
        obj->gfxChunk = (obj->xDirection > 0) ? prevAction->chunkRight : prevAction->chunkLeft;
    }
    if (obj->gfxChunk == (uint16_t)-1)
    {
        obj->gfxChunk = 0;
    }
#if 0
    if (obj->currentAction->chunkLeft && obj->xDirection <= 0) obj->gfxChunk = obj->currentAction->chunkLeft;
    if (obj->currentAction->chunkRight && obj->xDirection > 0) obj->gfxChunk = obj->currentAction->chunkRight;
#endif
    if (obj->gfxChunk != oldChunk || ck_nextX || ck_nextY || obj->topTI == 0x19)
    {
        if (obj->clipped == CLIP_simple)
            CK_PhysFullClipToWalls(obj);
        else
            CK_PhysUpdateNormalObj(obj);
    }
}

// Initializes the authoritative game state
void CK_Net_PlayInit(int level, int numPlayers) 
{
    memset(net_state, 0, sizeof(CK_NetGameState));

    net_state->level = level;
    net_state->numPlayers = numPlayers;

    US_InitRndT(false);
    CA_CacheMap(net_state->level);
    RF_NewMap();
    CA_ClearMarks();

    CK_Net_SetupObjArray();
    ck_currentEpisode->scanInfoLayer();

    RF_MarkTileGraphics();
    CA_LoadAllSounds();
    CA_CacheMarks(NULL);

    SD_SetSpriteSync(2);
    SD_SetLastTimeCount(3);
    SD_SetTimeCount(3);
}

// Updates the authoratative game state by one tic
void CK_Net_PlayTic(CK_TicInput (*input)[MAX_NET_PLAYERS]) 
{
    // Get input
    for (int i = 0; i < MAX_NET_PLAYERS; i++)
    {
        net_state->inputFrames[i] = Net_TicInputToControlFrame((*input)[i]);

        if (net_state->inputFrames[i].yDirection != -1)
            net_state->playerStates[i].keenSliding = false;

        net_state->playerStates[i].jumpIsPressed = net_state->inputFrames[i].jump;
        net_state->playerStates[i].pogoIsPressed = net_state->inputFrames[i].pogo;
        net_state->playerStates[i].shootIsPressed = net_state->inputFrames[i].button2;
        if (!net_state->playerStates[i].jumpIsPressed) 
            net_state->playerStates[i].jumpWasPressed = false;
        if (!net_state->playerStates[i].pogoIsPressed) 
            net_state->playerStates[i].pogoWasPressed = false;
        if (!net_state->playerStates[i].shootIsPressed) 
            net_state->playerStates[i].shootWasPressed = false;
    }

    // Respawn Things
    for (Net_Respawn *r = net_state->firstRespawn; r != net_state->freeRespawn; r = r->next)
    {
        if ((r->timer -= SD_GetSpriteSync()) <= 0)
        {
            // TODO: Respawn a gut
            Net_RemoveRespawn(r);
        }
    }

    // Do Actions
    for (CK_object *currentObj = net_state->firstObject; currentObj; currentObj = currentObj->next)
    {
        // currentObj->active = OBJ_ACTIVE;
        // currentObj->visible = true;
        CK_Net_RunAction(currentObj);
    }

    for (int i = 0; i < net_state->numPlayers; i++)
    {
        if (net_state->playerStates[i].platform)
            CK_KeenRidePlatform(net_state->playerStates[i].obj);
    }

    // Do collisions
    for (CK_object *currentObj = net_state->firstObject; currentObj; currentObj = currentObj->next)
    {
        for (CK_object *collideObj = currentObj->next; collideObj; collideObj = collideObj->next)
        {
            if (	(currentObj->clipRects.unitX2 > collideObj->clipRects.unitX1) &&
                    (currentObj->clipRects.unitX1 < collideObj->clipRects.unitX2) &&
                    (currentObj->clipRects.unitY1 < collideObj->clipRects.unitY2) &&
                    (currentObj->clipRects.unitY2 > collideObj->clipRects.unitY1) )
            {
                if (currentObj->currentAction->collide)
                    currentObj->currentAction->collide(currentObj, collideObj);
                if (collideObj->currentAction->collide)
                    collideObj->currentAction->collide(collideObj, currentObj);
            }
        }
    }

    for (int i = 0; i < net_state->numPlayers; i++)
    {
        NK_KeenCheckSpecialTileInfo(net_state->playerStates[i].obj);
    }


    // Do Drawfuncs
    for (CK_object *currentObj = net_state->firstObject; currentObj; currentObj = currentObj->next)
    {
        if (currentObj->clipRects.tileY2 >= (CA_GetMapHeight() - 1))
        {
            if (currentObj->type == CT_Player)
            {
                // Kill Keen if he exits the bottom of the map.
                // ck_gameState.levelState = 1;
                // TODO: kill keen
                continue;
            }
            else
            {
                CK_Net_RemoveObj(currentObj);
                continue;
            }
        }
        // if (currentObj->visible && currentObj->currentAction->draw)
        if (currentObj->currentAction->draw)
        {
            // currentObj->visible = false;	//We don't need to render it twice!
            currentObj->currentAction->draw(currentObj);
        }
    }

    // Timers and stuff
    for (int i = 0; i < net_state->numPlayers; i++)
    {
        net_state->playerStates[i].respawn = false;

        // Invincibility timer
        if (net_state->playerStates[i].invincibilityTimer)
        {
            net_state->playerStates[i].invincibilityTimer -= SD_GetSpriteSync();
            if (net_state->playerStates[i].invincibilityTimer < 0)
                net_state->playerStates[i].invincibilityTimer = 0;
        }

        // Respawn players
        if (net_state->playerStates[i].respawnTimer)
        {
            net_state->playerStates[i].respawnTimer -= SD_GetSpriteSync();
            if (net_state->playerStates[i].respawnTimer < 0)
                net_state->playerStates[i].respawnTimer = 0;

            if (net_state->playerStates[i].respawnTimer == 0)
                NK_SpawnKeen(i, US_RndT() % net_state->numSpawn);
        }

    }

    SD_SetLastTimeCount(SD_GetLastTimeCount() + SD_GetSpriteSync());
}

