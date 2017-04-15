/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2012 David Gow <david@ingeniumdigital.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck_act.h"
#include "ck_net.h"

#include "nk_keen.h"
#include "nk_obj.h"

#include "nk5_ep.h"

#include "id_heads.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_ti.h"
#include "id_ca.h"
#include "id_sd.h"

// For all the shitty debug stuff  I have.
#include <stdio.h>

#define max(a,b) (((a)>(b))?(a):(b))

#if 0
extern soundnames *ck_itemSounds;
extern uint16_t ck_itemPoints[];//  = {  0,   0,   0,   0, 100, 200, 500, 1000, 2000, 5000,   0,   0,   0};
extern uint16_t *ck_itemShadows;
extern void CK6_ToggleBigSwitch(CK_object *obj, bool p);
#endif

void NK_KeenColFunc(CK_object *a, CK_object *b)
{
    CK_NetPlayerState *ps_a = &net_state->playerStates[a->user4];

    if (b->type == CTN_Item)
    {
        Net_PlaySound(ck_itemSounds[b->user1]);

        b->type = 1;
        b->zLayer = 3;
        b->gfxChunk = ck_itemShadows[b->user1];

        // CK_IncreaseScore(ck_itemPoints[b->user1]);

        //b->yDirection = -1;

        if (b->user1 < 4)
        {
            ps_a->keyGems[b->user1]++;
        }
        else if (b->user1 == 10)
        {
            ps_a->health += 100;
        }
        else if (b->user1 == 11)
        {
            ps_a->numShots += 5;
        }
        CK_SetAction2(b, CK_GetActionByName("NK_ACT_itemNotify"));
    }
#if 0
    else if (b->type == CT_CLASS(Platform)) //Platform
    {
        if (!ps->platform)
            CK_PhysPushY(a,b);
    }
#endif
}

static int ck_KeenRunXVels[8] = {0, 0, 4, 4, 8, -4, -4, -8};

static int ck_KeenPoleOffs[3] = {-8, 0, 8};


void NK_KeenIncreaseScore(int score)
{
#if 0
    ck_gameState.keenScore += score;
    if (IN_DemoGetMode() != IN_Demo_Off) return;
    if (ck_gameState.keenScore > ck_gameState.nextKeenAt)
    {
        Net_PlaySound(SOUND_GOTEXTRALIFE);
        ck_gameState.numLives++;
        ck_gameState.nextKeenAt *= 2;
    }
#endif
}

void NK_AddSpawnPoint(int tileX, int tileY, int direction, int team)
{
    int i = net_state->numSpawn++;
    net_state->spawnPoints[i].tileX = tileX;
    net_state->spawnPoints[i].tileY = tileY;
    net_state->spawnPoints[i].direction = direction;
    net_state->spawnPoints[i].team = team;
}

void NK_SpawnKeen(int playerId, int spawnId)
{
    // TODO: team games
    CK_object *obj = net_state->playerStates[playerId].obj;
    obj->type = CT_Player;
    obj->active = OBJ_ALWAYS_ACTIVE;
    obj->visible = true;
    obj->zLayer = 1;
    obj->clipped = CLIP_normal;
    obj->posX = RF_TileToUnit(net_state->spawnPoints[spawnId].tileX);
    obj->posY = RF_TileToUnit(net_state->spawnPoints[spawnId].tileY) - 241;
    obj->xDirection = net_state->spawnPoints[spawnId].direction;
    obj->yDirection = IN_motion_Down;
    CK_SetAction(obj, CK_GetActionByName("NK_ACT_keenStanding"));
    net_state->playerStates[playerId].respawn = true;
}

static uint16_t emptyTile = 0;

void NK_KeenGetTileItem(int tileX, int tileY, int itemNumber)
{
    RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);
    Net_PlaySound(ck_itemSounds[itemNumber]);

    // CK_IncreaseScore(ck_itemPoints[itemNumber]);

    // TODO: Handle more kinds of pick-ups

#if 0
    if (itemNumber == 11)
    {
        ck_gameState.numShots += 5;
    }
#endif

    CK_object *notify = CK_Net_GetNewObj(true);
    notify->type = 1;
    notify->zLayer = 3;
    notify->posX = RF_TileToUnit(tileX);
    notify->posY = RF_TileToUnit(tileY);
    notify->yDirection = -1;
    notify->user2 = ck_itemShadows[itemNumber];
    notify->gfxChunk = notify->user2;
    CK_SetAction(notify, CK_GetActionByName("NK_ACT_itemNotify"));
    notify->clipped = CLIP_not;
}

void NK_KeenGetTileCentilife(CK_object *obj, int tileX, int tileY)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    RF_ReplaceTiles(&emptyTile, 1, tileX, tileY, 1, 1);
    Net_PlaySound(SOUND_GOTCENTILIFE);
    printf("here");
    NK_SpawnCentilifeNotify(tileX, tileY);
    ps->health += 5;
}

void NK_KeenCheckSpecialTileInfo(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    for (int y = obj->clipRects.tileY1; y <= obj->clipRects.tileY2; ++y)
    {
        for (int x = obj->clipRects.tileX1; x <= obj->clipRects.tileX2; ++x)
        {
            int specialTileInfo =  (TI_ForeMisc(CA_TileAtPos(x,y,1)) & 0x7F);
            switch (specialTileInfo)
            {
                case 0: 
                    break;
                case MISCFLAG_DEADLY:
                        NK_KillKeen(obj);
                        break;
                case 4:
                        NK_KeenGetTileCentilife(obj, x,y);
                        break;
                case MISCFLAG_GEMHOLDER0:
                case MISCFLAG_GEMHOLDER1:
                case MISCFLAG_GEMHOLDER2:
                case MISCFLAG_GEMHOLDER3:
                        if (y == obj->clipRects.tileY2 && obj->topTI && obj->currentAction != CK_GetActionByName("NK_ACT_keenPlaceGem")
                                && ps->keyGems[specialTileInfo - MISCFLAG_GEMHOLDER0])
                        {
                            int targetXUnit = RF_TileToUnit(x) - 64;
                            if (obj->posX == targetXUnit)
                            {
                                ps->keyGems[specialTileInfo - MISCFLAG_GEMHOLDER0]--;
                                CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenPlaceGem"));
                            }
                            else
                            {
                                obj->user1 = targetXUnit;
                                obj->currentAction = CK_GetActionByName("NK_ACT_keenSlide");
                            }
                        }
                        return;
                case 21:
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                case 27:
                case 28:
                case 29:
                        NK_KeenGetTileItem(x,y,specialTileInfo - 17);
                        break;
            }
        }
    }
}

void NK_KeenPressSwitchThink(CK_object *obj)
{
    uint16_t switchTarget = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 2);
    int switchTargetX = (switchTarget >> 8);
    int switchTargetY = (switchTarget & 0xFF);
    uint16_t switchTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);
    uint8_t switchMisc = TI_ForeMisc(switchTile);

    // Toggle the switch.
    uint16_t switchNextTile = switchTile + TI_ForeAnimTile(switchTile);
    RF_ReplaceTiles(&switchNextTile, 1, obj->clipRects.tileXmid, obj->clipRects.tileY1, 1, 1);
    Net_PlaySound(SOUND_KEENOUTOFAMMO);

    if (switchMisc == MISCFLAG_SWITCHBRIDGE)
    {
        for (int tileY = switchTargetY; tileY < switchTargetY + 2; ++tileY)
        {
            for (int tileX = switchTargetX - ((tileY == switchTargetY)? 0 : 1); tileX < CA_GetMapWidth(); ++tileX)
            {
                uint16_t currentTile = CA_TileAtPos(tileX, tileY, 1);
                if (!TI_ForeAnimTile(currentTile)) 
                    break;
                uint16_t newTile = currentTile + TI_ForeAnimTile(currentTile);
                RF_ReplaceTiles(&newTile, 1, tileX, tileY, 1, 1);
            }
        }
    }
    else
    {
        int infoPlaneInverses[8] = {2,3,0,1,6,7,4,5};
        uint16_t infoPlaneValue = CA_TileAtPos(switchTargetX, switchTargetY, 2);
        if (infoPlaneValue >= 91 && infoPlaneValue < 99)
        {
            // Invert the direction of the goplat arrow.
            infoPlaneValue = infoPlaneInverses[infoPlaneValue - 91] + 91;
        }
        else
        {
            // Insert or remove a [B] block.
            infoPlaneValue ^= 0x1F;
        }

        CA_mapPlanes[2][switchTargetY*CA_GetMapWidth() + switchTargetX] = infoPlaneValue;
    }
}

bool NK_KeenPressUp(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    uint8_t tileMiscFlag = TI_ForeMisc(CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1));

    // Are we pressing a switch?
    if (tileMiscFlag == MISCFLAG_SWITCHPLATON || tileMiscFlag == MISCFLAG_SWITCHPLATOFF || tileMiscFlag == MISCFLAG_SWITCHBRIDGE)
    {
        int16_t destXunit = RF_TileToUnit(obj->clipRects.tileXmid) - 64;
        if (obj->posX == destXunit)
        {
            // Flip that switch!
            // obj->currentAction = CK_GetActionByName("NK_ACT_keenPressSwitch1");

            // Simply changing the action pointer causes keen to glitch-fall
            // The same thing happens in DOS-netkeen too, so we need to use SetAction2
            // Maybe the sprite hitboxes are improperly set 
            CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenPressSwitch1"));
        }
        else
        {
            obj->user1 = destXunit;
            obj->currentAction = CK_GetActionByName("NK_ACT_keenSlide");
        }
        ps->keenSliding = true;
        return true;
    }

    // Are we enterting a door?
    if (tileMiscFlag == MISCFLAG_DOOR || tileMiscFlag == MISCFLAG_SECURITYDOOR )
    {
        uint16_t destUnitX = RF_TileToUnit(obj->clipRects.tileXmid) + 96;

        // If the door is two tiles wide, we want to be in the centre.
        uint8_t miscFlagLeft = TI_ForeMisc(CA_TileAtPos(obj->clipRects.tileXmid - 1, obj->clipRects.tileY1, 1));
        if (miscFlagLeft == MISCFLAG_DOOR || miscFlagLeft == MISCFLAG_SECURITYDOOR)
        {
            destUnitX -= 256;
        }

        if (obj->posX == destUnitX)
        {
            //We're at the door.
            {
                ps->invincibilityTimer = 110;
                obj->currentAction = CK_GetActionByName("NK_ACT_keenEnterDoor0");
                obj->zLayer = 0;
            }
        }
        else
        {
            obj->user1 = destUnitX;
            obj->currentAction = CK_GetActionByName("NK_ACT_keenSlide");
        }

        ps->keenSliding = true;
        return true;
    }

    // No? Return to our caller, who will handle poles/looking up.
    return false;
}

// Think function for keen "sliding" towards a switch, keygem or door.
void NK_KeenSlide(CK_object *obj)
{
    int16_t deltaX = obj->user1 - obj->posX;
    if (deltaX < 0)
    {
        // Move left one px per tick.
        ck_nextX -= SD_GetSpriteSync() * 16;
        // If we're not at our target yet, return.
        if (ck_nextX > deltaX) 
            return;
    }
    else if (deltaX > 0)
    {
        // Move right one px per tick.
        ck_nextX += SD_GetSpriteSync() * 16;
        // If we're not at our target yet, return.
        if (ck_nextX < deltaX) 
            return;
    }

    // We're at our target.
    ck_nextX = deltaX;
    obj->user1 = 0;
    if (!NK_KeenPressUp(obj))
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenStanding");
    }
}

void NK_KeenEnterDoor0(CK_object *obj)
{
    Net_PlaySound(SOUND_KEENWALK0);
}

void NK_KeenEnterDoor1(CK_object *obj)
{
    Net_PlaySound(SOUND_KEENWALK1);
}

// Think function for entering a door.
void NK_KeenEnterDoor(CK_object *obj)
{
    uint16_t destination = CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY2, 2);

    obj->posY = RF_TileToUnit((destination&0xFF)) - 256 + 15;
    obj->posX = RF_TileToUnit((destination >> 8));
    obj->zLayer = 1;
    obj->clipped = CLIP_not;
    CK_SetAction2(obj, obj->currentAction->next);
    obj->clipped = CLIP_normal;
    Net_CentreCameraSelf(obj->user4);
}

void NK_KeenPlaceGem(CK_object *obj)
{
    uint16_t oldFGTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
    uint16_t newFGTile = oldFGTile + 18;
    uint16_t target = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 2);

    int targetX = target >> 8;
    int targetY = target & 0xFF;

    RF_ReplaceTiles(&newFGTile, 1, obj->clipRects.tileXmid, obj->clipRects.tileY2, 1, 1);

    Net_PlaySound(SOUND_OPENGEMDOOR);

    CK_object *newObj = CK_Net_GetNewObj(false);

    newObj->posX = targetX;
    newObj->posY = targetY;

    if (targetX > CA_GetMapWidth() || targetX < 2 || targetY > CA_GetMapHeight() || targetY < 2)
    {
        Quit("Keyholder points to a bad spot!");
    }

    // Calculate the height of the door.
    int doorHeight = 0;
    int doorY = targetY;
    uint16_t doorTile = CA_TileAtPos(targetX, doorY, 1);
    do
    {
        doorHeight++;
        doorY++;
    } while (CA_TileAtPos(targetX, doorY, 1) == doorTile);

    newObj->user1 = doorHeight;
    newObj->clipped = CLIP_not;
    newObj->type = CT_Friendly;
    CK_SetAction(newObj, CK_GetActionByName("NK_ACT_DoorOpen1"));
}

void NK_KeenRidePlatform(CK_object *obj)
{
#if 0
    // Save the platform pointer, we might be wiping it.
    CK_object *plat = ps->platform;


    if (obj->clipRects.unitX2 < plat->clipRects.unitX1 || obj->clipRects.unitX1 > plat->clipRects.unitX2)
    {
        // We've fallen off the platform horizontally.
        ps->platform = 0;
        return;
    }

    if (obj->deltaPosY < 0)
    {
        // If we've jumped off the platform.
        ps->platform = 0;
        if (plat->deltaPosY < 0)
        {
            ck_nextX = 0;
            ck_nextY = plat->deltaPosY;
            CK_PhysUpdateSimpleObj(obj);
            return;
        }
    }
    else
    {
        //Ride the platform
        ck_nextX = plat->deltaPosX;
        ck_nextY = plat->clipRects.unitY1 - obj->clipRects.unitY2 - 16;
        CK_PhysUpdateSimpleObj(obj);

        // Keen has a "/NOPAN" parameter which disables use of the EGA panning
        // register. In order to make platforms less ugly when this is on
        // (though not much less ugly, IMHO), we would do some more processing here.
        // As is, we just do the line below, to keep keen at the same position while scrolling.

        // Keen's x position should move with 2px granularity when on a platform so that scrolling
        // looks nice. (Keen will stay at the same pixel position when scrolling, as we scroll 2px at a time).
        obj->posX |= plat->posX & 0x1F;

        // We've hit the ceiling?
        if (obj->bottomTI)
        {
            ps->platform = 0;
            return;
        }


        // We're standing on something, don't fall down!
        obj->topTI = 0x19;
    }
#endif
}


bool NK_KeenTryClimbPole(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];
    
    if (SD_GetLastTimeCount() < ps->poleGrabTime)
        ps->poleGrabTime = 0;
    else if (SD_GetLastTimeCount() - ps->poleGrabTime < 19)
        return false;

    uint16_t candidateTile = CA_TileAtPos(obj->clipRects.tileXmid,
            ((cf->yDirection==-1)?(RF_UnitToTile(obj->clipRects.unitY1+96)):(obj->clipRects.tileY2+1)), 1);

    if ((TI_ForeMisc(candidateTile) & 0x7F) == 1)
    {
        obj->posX = 128 + RF_TileToUnit(obj->clipRects.tileXmid - 1);
        ck_nextX = 0;
        ck_nextY = (cf->yDirection << 5);
        obj->clipped = CLIP_not;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleSit");
        return true;
    }
    return false;
}

void NK_HandleInputOnGround(CK_object *obj);

void NK_KeenRunningThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (!cf->xDirection)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenStanding");
        NK_HandleInputOnGround(obj);
        return;
    }

    obj->xDirection = cf->xDirection;

    if (cf->yDirection == -1)
    {
        if (NK_KeenTryClimbPole(obj)) return;
        if (ps->keenSliding || NK_KeenPressUp(obj))
            return;
    }
    else if (cf->yDirection == 1)
    {
        if (NK_KeenTryClimbPole(obj)) return;
    }

    if (ps->shootIsPressed && !ps->shootWasPressed)
    {
        ps->shootWasPressed = true;

        if (cf->yDirection == -1)
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenShootUp1");
        }
        else
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenShoot1");
        }
        return;
    }

    if (ps->jumpIsPressed && !ps->jumpWasPressed)
    {
        ps->jumpWasPressed = true;
        Net_PlaySound(SOUND_KEENJUMP);
        obj->velX = obj->xDirection * 16;
        obj->velY = -40;
        ck_nextX = ck_nextY = 0;
        ps->jumpTimer = 18;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenJump1");
    }

    if (ps->pogoIsPressed && !ps->pogoWasPressed)
    {
        ps->pogoWasPressed = true;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPogo1");
        Net_PlaySound(SOUND_KEENJUMP);
        obj->velX = obj->xDirection * 16;
        obj->velY = -48;
        // Should this be nextY? Andy seems to think so, but lemm thinks that X is right...
        ck_nextX = 0;
        ps->jumpTimer = 24;
        return;
    }

    // Andy seems to think this is Y as well. Need to do some more disasm.
    // If this is an X vel, then surely we'd want to multiply it by the direction?
    ck_nextX += ck_KeenRunXVels[obj->topTI&7] * SD_GetSpriteSync();

    if ((obj->currentAction->chunkLeft == CK_GetActionByName("NK_ACT_keenRun1")->chunkLeft) && !(obj->user3))
    {
        Net_PlaySound(SOUND_KEENWALK0);
        obj->user3 = 1;
        return;
    }

    if ((obj->currentAction->chunkLeft == CK_GetActionByName("NK_ACT_keenRun3")->chunkLeft) && !(obj->user3))
    {
        Net_PlaySound(SOUND_KEENWALK1);
        obj->user3 = 1;
        return;
    }

    if ((obj->currentAction->chunkLeft == CK_GetActionByName("NK_ACT_keenRun2")->chunkLeft) || (obj->currentAction->chunkLeft == CK_GetActionByName("NK_ACT_keenRun4")->chunkLeft))
    {
        obj->user3 = 0;
    }
}



void NK_HandleInputOnGround(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];
#if 0
    // If we're riding a platform, do it surfin' style!
    if (obj->topTI == 0x19)
    {
        // But only if such an action exists in this episode. :/
        if (CK_GetActionByName("NK_ACT_keenRidePlatform"))
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenRidePlatform");
        }
    }
#endif

    if (cf->xDirection)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenRun1");
        NK_KeenRunningThink(obj);
        ck_nextX = (obj->xDirection * obj->currentAction->velX * SD_GetSpriteSync())/4;
        return;
    }

    if (ps->shootIsPressed && !ps->shootWasPressed)
    {
        ps->shootWasPressed = true;
        if (cf->yDirection == -1)
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenShootUp1");
        }
        else
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenShoot1");
        }
        return;
    }

    if (ps->jumpIsPressed && !ps->jumpWasPressed)
    {
        ps->jumpWasPressed = true;
        Net_PlaySound(SOUND_KEENJUMP);
        obj->velX = 0;
        obj->velY = -40;
        ck_nextY = 0;
        ps->jumpTimer = 18;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenJump1");
        return;
    }

    if (ps->pogoIsPressed && !ps->pogoWasPressed)
    {
        ps->pogoWasPressed = true;
        Net_PlaySound(SOUND_KEENJUMP);
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPogo1");
        obj->velX = 0;
        obj->velY = -48;
        ck_nextY = 0;
        ps->jumpTimer = 24;
        return;
    }

    if (cf->yDirection == -1)
    {
        if (NK_KeenTryClimbPole(obj)) return;
        if (!ps->keenSliding && NK_KeenPressUp(obj)) return;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenLookUp1");
    }
    else if (cf->yDirection == 1)
    {
        // Try poles.
        if (NK_KeenTryClimbPole(obj)) 
            return;
        // Keen looks down.
        obj->currentAction = CK_GetActionByName("NK_ACT_keenLookDown1");
        return;
    }
}

void NK_KeenStandingThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

#if 0
    // If we're riding a platform, do it surfin' style!
    if (obj->topTI == 0x19)
    {
        // But only if such an action exists in this episode. :/
        if (CK_GetActionByName("NK_ACT_keenRidePlatform"))
        {
            obj->currentAction = CK_GetActionByName("NK_ACT_keenRidePlatform");
        }
    }
#endif

    if (cf->xDirection || cf->yDirection || ps->jumpIsPressed || ps->pogoIsPressed || ps->shootIsPressed)
    {
        obj->user1 = obj->user2 = 0;	//Idle Time + Idle State
        obj->currentAction = CK_GetActionByName("NK_ACT_keenStanding");
        NK_HandleInputOnGround(obj);
    }

	if ((obj->topTI & ~7) != 0x18)
		obj->user1 += SD_GetSpriteSync();

#if 0
	if (obj->user2 == 0 && obj->user1 > 200)
	{
		obj->currentAction = CK_GetActionByName("NK_ACT_keenIdle1");
        obj->user1 = 0;
        obj->user2++;
	}

	if (obj->user2 == 1 && obj->user1 > 200)
	{
		obj->user1 = obj->user2 = 0;
	}
#endif
}

void NK_KeenLookUpThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (cf->yDirection != -1 ||
            cf->xDirection != 0 ||
            (ps->jumpIsPressed && !ps->jumpWasPressed) ||
            (ps->pogoIsPressed && !ps->pogoWasPressed) ||
            (ps->shootIsPressed))
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenStanding");
        NK_HandleInputOnGround(obj);
    }
}

void NK_KeenLookDownThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    //Try to jump down
    if (ps->jumpIsPressed && !ps->jumpWasPressed && (obj->topTI&7) == 1)
    {
        ps->jumpWasPressed = true;

        //If the tiles below the player are blocking on any side but the top, they cannot be jumped through
        int tile1 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);
        int tile2 = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2+1, 1);

        if (TI_ForeLeft(tile1) || TI_ForeBottom(tile1) || TI_ForeRight(tile1))
            return;

        if (TI_ForeLeft(tile2) || TI_ForeBottom(tile2) || TI_ForeRight(tile2))
            return;

        uint16_t deltay = max(SD_GetSpriteSync(),4) << 4;

        //Moving platforms
        if (ps->platform)
            deltay += ps->platform->deltaPosY;

        ps->platform = 0;

        obj->clipRects.unitY2 += deltay;
        obj->posY += deltay;
        ck_nextX = 0;
        ck_nextY = 0;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenFall1");
        obj->velX = obj->velY = 0;
        Net_PlaySound(SOUND_KEENFALL);
        return;
    }

    if (cf->yDirection != 1 || cf->xDirection != 0 || (ps->jumpIsPressed && !ps->jumpWasPressed)
            || (ps->pogoIsPressed && !ps->pogoWasPressed))
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenLookDown4");
        return;
    }
}

void NK_KeenDrawFunc(CK_object *obj)
{
    CK_NetPlayerState *keenState = &net_state->playerStates[obj->user4];

    if (!obj->topTI)
    {
        Net_PlaySound(SOUND_KEENFALL);
        obj->velX = obj->xDirection * 8;
        obj->velY = 0;
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenFall1"));
        keenState->jumpTimer = 0;
    }
    else if ((obj->topTI & 0xFFF8) == 8)
    {
        NK_KillKeen(obj);
    }
    else if (obj->topTI == 0x29)
    {
        // Keen6 conveyor belt right
        ck_nextX = SD_GetSpriteSync() * 8;
        ck_nextY = 0;
        obj->user1 = 0;
        CK_PhysUpdateNormalObj(obj);
    }
    else if (obj->topTI == 0x31) {
        // Keen6 conveyor belt left
        ck_nextX = SD_GetSpriteSync() * -8;
        ck_nextY = 0;
        obj->user1 = 0;
        CK_PhysUpdateNormalObj(obj);
    }

    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// TODO: More to modify here
void NK_KeenRunDrawFunc(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    if (!obj->topTI)
    {
        Net_PlaySound(SOUND_KEENFALL);
        obj->velX = obj->xDirection * 8;
        obj->velY = 0;
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenFall1"));
        ps->jumpTimer = 0;
    }
    else if ((obj->topTI & 0xFFF8) == 8)
    {
        NK_KillKeen(obj);
    }
    else if (obj->topTI == 0x29)
    {
        // Keen6 conveyor belt right
        ck_nextX = SD_GetSpriteSync() * 8;
        ck_nextY = 0;
        obj->user1 = 0;
        CK_PhysUpdateNormalObj(obj);
    }
    else if (obj->topTI == 0x31)
    {
        // Keen6 conveyor belt left
        ck_nextX = SD_GetSpriteSync() * -8;
        ck_nextY = 0;
        obj->user1 = 0;
        CK_PhysUpdateNormalObj(obj);
    }

    // If we're running into a wall, we should stand still.
    if ((obj->rightTI && obj->xDirection == -1) || (obj->leftTI && obj->xDirection == 1))
    {
        obj->actionTimer = 0;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenStanding");
        obj->gfxChunk = (obj->xDirection == -1) ? obj->currentAction->chunkLeft : obj->currentAction->chunkRight;
    }

    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void NK_KeenJumpThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (ps->jumpTimer)
    {
        if (ps->jumpTimer <= SD_GetSpriteSync())
        {
            ck_nextY = obj->velY * ps->jumpTimer;
            ps->jumpTimer = 0;
        }
        else
        {
            ck_nextY = obj->velY * SD_GetSpriteSync();
            ps->jumpTimer -= SD_GetSpriteSync();
        }

        // Stop moving up if we've let go of control.
        if (!ps->jumpIsPressed)
        {
            ps->jumpTimer = 0;
        }

        if (!ps->jumpTimer && obj->currentAction->next)
        {
            obj->currentAction = obj->currentAction->next;
        }
    }
    else
    {
        CK_PhysGravityHigh(obj);

        if (obj->velY > 0 && obj->currentAction != CK_GetActionByName("NK_ACT_keenFall1") && obj->currentAction != CK_GetActionByName("NK_ACT_keenFall2"))
        {
            obj->currentAction = obj->currentAction->next;
        }
    }

    //Move horizontally
    if (cf->xDirection)
    {
        obj->xDirection = cf->xDirection;
        CK_PhysAccelHorz(obj, cf->xDirection*2, 24);
    }
    else 
        CK_PhysDampHorz(obj);

    //Pole
    if (obj->bottomTI == 17)
    {
        ck_nextX = 0;
        obj->velX = 0;
    }

    //Shooting
    if (ps->shootIsPressed && !ps->shootWasPressed)
    {
        ps->shootWasPressed = true;
        switch (cf->yDirection)
        {
            case -1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShootUp1");
                break;
            case 0:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShoot1");
                break;
            case 1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShootDown1");
                break;
        }
        return;
    }

    if (ps->pogoIsPressed && !ps->pogoWasPressed)
    {
        ps->pogoWasPressed = true;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPogo2");
        ps->jumpTimer = 0;
        return;
    }

    if (cf->yDirection == -1)
    {
        NK_KeenTryClimbPole(obj);
    }
}

extern int rf_scrollXUnit, rf_scrollYUnit;

void NK_KeenJumpDrawFunc(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (obj->rightTI && obj->xDirection == -1)
    {
        obj->velX = 0;
    }
    else if (obj->leftTI && obj->xDirection == 1)
    {
        obj->velX = 0;
    }

    // Did we hit our head on the ceiling?
    if (obj->bottomTI)
    {
        //TODO: Something to do with poles (push keen into the centre)
        if (obj->bottomTI == 17)	//Pole
        {
            obj->posY -= 32;
            obj->clipRects.unitY1 -= 32;
            obj->velX = 0;
            obj->posX = RF_TileToUnit(obj->clipRects.tileXmid) - 32;
        }
        else
        {
#if 0
            if (obj->bottomTI == 0x21)  // Bloog switches
                CK6_ToggleBigSwitch(obj, false);
#endif
            Net_PlaySound(SOUND_KEENHITCEILING);
            if (obj->bottomTI > 1)
            {
                obj->velY += 16;
                if (obj->velY < 0)
                    obj->velY = 0;
            }
            else
            {
                obj->velY = 0;
            }
            ps->jumpTimer = 0;
        }
    }

    // Have we landed?
    if (obj->topTI)
    {
        obj->deltaPosY = 0;
        //Check if deadly.
        if ((obj->topTI & ~7) == 8)
        {
            NK_KillKeen(obj);
        }
        else
        {
            if (obj->topTI == 0x39) // Fuse
            {
                Net_PlaySound(SOUND_KEENLANDONFUSE);
            }
#if 0
            if (ck_currentEpisode->ep == EP_CK6 && obj->topTI == 0x21) // BigSwitch
            {
                CK6_ToggleBigSwitch(obj, true);
            }
#endif
            if (obj->topTI != 0x19 || !ps->jumpTimer) // Or standing on a platform.
            {
                obj->user1 = obj->user2 = 0;	// Being on the ground is boring.

                if (obj->currentAction == CK_GetActionByName("NK_ACT_keenJumpShoot1"))
                {
                    CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenShoot1"));
                }
                else if (obj->currentAction == CK_GetActionByName("NK_ACT_keenJumpShootUp1"))
                {
                    CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenShootUp1"));
                }
                else 
                    if (cf->xDirection)
                    {
                        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenRun1"));
                    }
                    else
                    {
                        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenStanding"));
                    }
                Net_PlaySound(SOUND_KEENLAND);
            }
        }
    }
    else if (obj->deltaPosY > 0)
    {
        // temp6 = Keen's previous upper y coord
        int temp6 = obj->clipRects.unitY1 - obj->deltaPosY;
        // temp8 = Keen's current upper y coord - 1.5 tiles, rounded to nearest tile, + 1.5 tiles
        int temp8 = ((obj->clipRects.unitY1 - 64) & 0xFF00) + 64;
        // temp10 = temp8 in tile coords, - 1
        int temp10 = RF_UnitToTile(temp8) - 1 ;

        // If we're moving past a tile boundary.
        if (temp6 < temp8 && obj->clipRects.unitY1 >= temp8)
        {
            // Moving left...
            if (cf->xDirection == -1)
            {
                int tileX = obj->clipRects.tileX1 - ((obj->rightTI)?1:0);
                int tileY = temp10;
                //VL_ScreenRect((tileX << 4) - (rf_scrollXUnit >> 4), (tileY << 4) - (rf_scrollYUnit >> 4), 16, 16, 1);
                int upperTile = CA_TileAtPos(tileX, tileY, 1);
                int lowerTile = CA_TileAtPos(tileX, tileY+1, 1);
                if ( (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile)) &&
                        TI_ForeRight(lowerTile) && TI_ForeTop(lowerTile))
                {
                    obj->xDirection = -1;
                    obj->clipped = CLIP_not;
                    obj->posX = (obj->posX & 0xFF00) + 128;
                    obj->posY = (temp8 - 64);
                    obj->velY = obj->deltaPosY = 0;
                    CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenHang1"));
                }
            }
            else if (cf->xDirection == 1)
            {
                int tileX = obj->clipRects.tileX2 + ((obj->leftTI)?1:0);
                int tileY = temp10;
                int upperTile = CA_TileAtPos(tileX, tileY, 1);
                int lowerTile = CA_TileAtPos(tileX, tileY+1, 1);

                if (!TI_ForeLeft(upperTile) && !TI_ForeRight(upperTile) && !TI_ForeTop(upperTile) && !TI_ForeBottom(upperTile) &&
                        TI_ForeLeft(lowerTile) && TI_ForeTop(lowerTile))
                {
                    obj->xDirection = 1;
                    obj->clipped = CLIP_not;
                    obj->posX = (obj->posX & 0xFF00) + 256;
                    obj->posY = (temp8 - 64);
                    obj->velY = obj->deltaPosY = 0;
                    CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenHang1"));
                }
            }
        }
    }

    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void NK_KeenPogoBounceThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    obj->velY = -48;
    ck_nextY = 6 * obj->velY;
    ps->jumpTimer = 24;
    Net_PlaySound(SOUND_KEENPOGO);
}

void NK_KeenPogoThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (!ps->jumpTimer)
    {
        CK_PhysGravityHigh(obj);
    }
    else
    {
        if (ps->jumpIsPressed || ps->jumpTimer <= 9) 
            CK_PhysGravityLow(obj);
        else 
            CK_PhysGravityHigh(obj);

        if (ps->jumpTimer <= SD_GetSpriteSync()) 
            ps->jumpTimer = 0;
        else 
            ps->jumpTimer -= SD_GetSpriteSync();

        if (!ps->jumpTimer && obj->currentAction->next)
            obj->currentAction = obj->currentAction->next;
    }
    if (cf->xDirection)
    {
        if (!obj->velX)
            obj->xDirection = cf->xDirection;
        CK_PhysAccelHorz(obj, cf->xDirection, 24);
    }
    else
    {
        ck_nextX += obj->velX * SD_GetSpriteSync();
        if (obj->velX < 0) 
            obj->xDirection = -1;
        else if (obj->velX > 0) 
            obj->xDirection = 1;
    }

    // Stop for poles?
    if (obj->bottomTI == 17)
    {
        ck_nextX = 0;
        obj->velX = 0;
    }

    //Shooting
    if (ps->shootIsPressed && !ps->shootWasPressed)
    {
        ps->shootWasPressed = true;
        switch (cf->yDirection)
        {
            case -1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShootUp1");
                return;
            case 0:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShoot1");
                return;
            case 1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenJumpShootDown1");
                return;
        }
    }

    //Stop pogoing if Alt pressed
    if (ps->pogoIsPressed && !ps->pogoWasPressed)
    {
        ps->pogoWasPressed = true;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenFall1");
    }
}

void NK_KeenPogoDrawFunc(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    if (obj->rightTI && obj->xDirection == -1)
    {
        obj->velX = 0;
    }
    else if (obj->leftTI && obj->xDirection == 1)
    {
        obj->velX = 0;
    }

    if (obj->bottomTI)
    {
        if (obj->bottomTI == 17)	//Pole
        {
            obj->posY -= 32;
            obj->clipRects.unitY1 -= 32;
            obj->velX = 0;
            obj->posX = RF_TileToUnit(obj->clipRects.tileXmid) - 32;
        }
        else
        {
#if 0
            if (obj->bottomTI == 0x21)  // Bloog switches
                CK6_ToggleBigSwitch(obj, false);
#endif

            Net_PlaySound(SOUND_KEENHITCEILING);

            if (obj->bottomTI > 1)
            {
                obj->velY += 16;
                if (obj->velY < 0) obj->velY = 0;
            }
            else obj->velY = 0;

            ps->jumpTimer = 0;
        }
    }

    // Houston, we've landed!
    if (obj->topTI)
    {
        obj->deltaPosY = 0;

        //Check if deadly.  
        if ((obj->topTI & ~7) == 8)
        {
            NK_KillKeen(obj);
        }
        else
        {
#if 0
            if (ck_currentEpisode->ep == EP_CK6 && obj->topTI == 0x21) // BigSwitch
            {
                CK6_ToggleBigSwitch(obj, true);
            }
            else 
            if (obj->topTI == 0x39) // Fuse
            {
                if (obj->velY >= 0x30)
                {
                    NK_KeenBreakFuse(obj->clipRects.tileXmid, obj->clipRects.tileY2);
                    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
                    return;
                }
                Net_PlaySound(SOUND_KEENLANDONFUSE);
            }
#endif
            if (obj->topTI != 0x19 || ps->jumpTimer == 0)
            {
                obj->velY = -48;
                ps->jumpTimer = 24;
                Net_PlaySound(SOUND_KEENPOGO);
                CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenPogo2"));
            }
        }
    }

    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void NK_KeenSpecialColFunc(CK_object *obj, CK_object *other)
{
#if 0
    if (other->type == CT_CLASS(Platform))
    {
        obj->clipped = CLIP_normal;
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenFall1"));
        ps->jumpTimer = 0;
        obj->velX = 0;
        obj->velY = 0;
        CK_PhysPushY(obj,other);
    }
#endif
}

void NK_KeenSpecialDrawFunc(CK_object *obj)
{
    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// Used by CK6
void NK_KeenJumpDownThink(CK_object *obj)
{
    obj->clipped = CLIP_normal;
}


void NK_KeenHangThink(CK_object *obj)
{
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (cf->yDirection == -1 || cf->xDirection == obj->xDirection)
    {
        uint16_t tile;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPull1");

        if(obj->xDirection == 1)
        {
            tile = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1-1, 1);
            ck_nextY = -256;
        }
        else
        {
            tile = CA_TileAtPos(obj->clipRects.tileX1, obj->clipRects.tileY1-1, 1);
            ck_nextY = -128;
        }

        if (!(TI_ForeMisc(tile) & 0x80))
            obj->zLayer = 3;
    }
    else if (cf->yDirection == 1 || (cf->xDirection && cf->xDirection != obj->xDirection))
    {
        // Drop down.
        obj->currentAction = CK_GetActionByName("NK_ACT_keenFall1");
        obj->clipped = CLIP_normal;
    }
}

void NK_KeenPullThink1(CK_object *obj)
{
    if (obj->xDirection == 1)
        ck_nextX = 128;
    else
        ck_nextY = -128;
}

void NK_KeenPullThink2(CK_object *obj)
{
    ck_nextX = obj->xDirection * 128;
    ck_nextY = -128;
}

void NK_KeenPullThink3(CK_object *obj)
{
    ck_nextY = -128;
}

void NK_KeenPullThink4(CK_object *obj)
{
    obj->clipped = CLIP_normal;
    obj->zLayer = 1;
    ck_nextY = 128;
}


void NK_KeenDeathThink(CK_object *obj)
{
}

void NK_KillKeen(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];

    if (ps->invincibilityTimer)
    {
        return;
    }

    ps->respawnTimer = 240;
    ps->invincibilityTimer = 360;
    // ck_scrollDisabled = true;
    obj->clipped = CLIP_not;
    obj->zLayer = 3;

    if (US_RndT() < 0x80)
    {
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenDie0"));
    }
    else
    {
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenDie1"));
    }

    Net_PlaySound(SOUND_KEENDIE);
}

void NK_KeenPoleHandleInput(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (cf->xDirection)
        obj->xDirection = cf->xDirection;

    //Shooting things. *ZAP!*
    if (ps->shootIsPressed && !ps->shootWasPressed)
    {
        ps->shootWasPressed = true;

        switch (cf->yDirection)
        {
            case -1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleShootUp1");
                break;
            case 0:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleShoot1");
                break;
            case 1:
                obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleShootDown1");
                break;
        }
    }

    if (ps->jumpIsPressed && !ps->jumpWasPressed)
    {
        ps->jumpWasPressed = true;
        Net_PlaySound(SOUND_KEENJUMP);
        obj->velX = ck_KeenPoleOffs[cf->xDirection+1];
        obj->velY = -20;
        obj->clipped = CLIP_normal;
        ps->jumpTimer = 10;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenJump1");
        obj->yDirection = 1;
        ps->poleGrabTime = SD_GetLastTimeCount();
    }
}

void NK_KeenPoleDownThink(CK_object *obj);

void NK_KeenPoleSitThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    if (cf->yDirection == 1)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleDown1");
        obj->yDirection = 1;
        NK_KeenPoleDownThink(obj);
        return;
    }
    else if (cf->yDirection == -1)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleUp1");
        obj->yDirection = -1;
        //NK_KeenPoleUpThink(obj);  // Keep this commented out
        return;
    }

    // When keen is at ground level, allow dismounting using left/right.
    if (cf->xDirection)
    {
        int groundTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2+1, 1);
        if (TI_ForeTop(groundTile))
        {
            obj->velX = 0;
            obj->velY = 0;
            obj->clipped = CLIP_normal;
            ps->jumpTimer = 0; obj->currentAction = CK_GetActionByName("NK_ACT_keenFall1");
            obj->yDirection = 1;
            Net_PlaySound(SOUND_KEENFALL);
            return;
        }
    }

    NK_KeenPoleHandleInput(obj);
}

void NK_KeenPoleUpThink(CK_object *obj)
{
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    int topTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);

    if ((TI_ForeMisc(topTile) & 127) != 1)
    {
        ck_nextY = 0;
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleSit");
        NK_KeenPoleHandleInput(obj);
        return;
    }

    if (cf->yDirection == 0)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleSit");
        obj->yDirection = 0;
        //NK_KeenPoleSitThink(obj);
    }
    else if (cf->yDirection == 1)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleDown1");
        obj->yDirection = 1;
        NK_KeenPoleDownThink(obj);
    }

    NK_KeenPoleHandleInput(obj);
}

void NK_KeenPoleDownThink(CK_object *obj)
{
    CK_NetPlayerState *ps = &net_state->playerStates[obj->user4];
    IN_ControlFrame *cf = &net_state->inputFrames[obj->user4];

    int tileUnderneath = CA_TileAtPos(obj->clipRects.tileXmid, RF_UnitToTile(obj->clipRects.unitY2-64), 1);

    if ((TI_ForeMisc(tileUnderneath) & 127) != 1)
    {
        // We're no longer on a pole.
        Net_PlaySound(SOUND_KEENFALL);
        obj->currentAction = CK_GetActionByName("NK_ACT_keenFall1");
        ps->jumpTimer = 0;
        obj->velX = ck_KeenPoleOffs[cf->xDirection + 1];
        obj->velY = 0;
        obj->clipped = CLIP_normal;
        obj->clipRects.tileY2 -= 1;	//WTF?
        return;
    }

    if (cf->yDirection == 0)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleSit");
        obj->yDirection = 0;
    }
    else if (cf->yDirection == -1)
    {
        obj->currentAction = CK_GetActionByName("NK_ACT_keenPoleUp1");
        obj->yDirection = -1;
    }

    NK_KeenPoleHandleInput(obj);
}

void NK_KeenPoleDownDrawFunc(CK_object *obj)
{
    // Check if keen is trying to climb through the floor.
    // It's quite a strange clipping error if he succeeds.

    int groundTile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY2, 1);

    if (TI_ForeTop(groundTile) == 1)
    {
        int yReset = -(obj->clipRects.unitY2 & 255) - 1;
        obj->posY += yReset;
        obj->clipRects.unitY2 += yReset;
        obj->clipRects.tileY2 += -1;
        obj->clipped = CLIP_normal;
        CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenLookDown1"));
    }

    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

// Defined for a seemingly unused state
void NK_KeenSetClipped(CK_object *obj)
{
    obj->clipped = CLIP_normal;
}

// Shooting
void NK_SpawnShot(int playerId, int x, int y, int direction)
{
    CK_NetPlayerState *ps = &net_state->playerStates[playerId];

    if (ps->numShots == 0)
    {
        Net_PlaySound(SOUND_KEENOUTOFAMMO);
        return;
    }

    ps->numShots--;

    CK_object *shot = CK_Net_GetNewObj(true);
    shot->posX = x;
    shot->posY = y;
    shot->zLayer = 0;
    shot->type = CT_Stunner; // TODO: obj_stunner

    Net_PlaySound(SOUND_KEENSHOOT);

    switch(direction)
    {
        case 0:
            shot->xDirection = 0;
            shot->yDirection = -1;
            break;
        case 2:
            shot->xDirection = 1;
            shot->yDirection = 0;
            break;
        case 4:
            shot->xDirection = 0;
            shot->yDirection = 1;
            break;
        case 6:
            shot->xDirection = -1;
            shot->yDirection = 0;
            break;
        default:
            Quit("SpawnShot: Bad dir!");
    }

    CK_SetAction(shot, CK_GetActionByName("NK_ACT_keenShot1"));
}

void NK_ShotHit(CK_object *obj)
{
    obj->type = CT_Friendly;
    CK_SetAction2(obj, CK_GetActionByName("NK_ACT_keenShotHit1"));
    Net_PlaySound(SOUND_KEENSHOTHIT);
}

void NK_ShotThink(CK_object *shot)
{
    // Everything is always active, so we don't need this think
    // It was just used for hitting things offscreen
}

void NK_ShotDrawFunc(CK_object *obj)
{
    uint16_t t;

    // shoot down through a pole hole
    if (obj->topTI == 1 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
    {
        t = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY1-1, 1);
        if (TI_ForeTop(t) == 0x11)
        {
            obj->topTI = 0x11;
            obj->posX += 0x100 - (obj->posX & 0xFF);
        }
    }
    // move into pole hole before making contact
    else if (obj->topTI == 0x11 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
    {
        obj->posX &= 0xFF00;
    }

    // shoot through pole hole upwards
    if (obj->bottomTI == 1 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
    {
        t = CA_TileAtPos(obj->clipRects.tileX2, obj->clipRects.tileY2+1, 1);
        if (TI_ForeBottom(t) == 0x11)
        {
            obj->bottomTI = 0x11;
            obj->posX += 0x100 - (obj->posX & 0xFF);
        }
    }
    // move into pole hole whilst travelling upwards
    else if (obj->bottomTI == 0x11 && obj->clipRects.tileX1 != obj->clipRects.tileX2)
    {
        obj->posX &= 0xFF00;
    }

    // if hit any other type of object, die
    if (obj->topTI != 0x11 && obj->bottomTI != 0x11)
    {
        if (obj->topTI || obj->bottomTI || obj->rightTI || obj->leftTI)
        {
            NK_ShotHit(obj);
        }
    }
    else
        // correct for pole hole passage
    {
        ck_nextY = obj->currentAction->velY * SD_GetSpriteSync() * obj->yDirection;
        obj->posY += ck_nextY;
        obj->clipRects.unitY1 += ck_nextY;
        obj->clipRects.unitY2 += ck_nextY;
        obj->clipRects.tileY1 = RF_UnitToTile(obj->clipRects.unitY1);
        obj->clipRects.tileY2 = RF_UnitToTile(obj->clipRects.unitY2);
    }
    RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

void NK_KeenSpawnShot(CK_object *obj)
{
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenShoot1"))
    {
        if (obj->xDirection == 1)
        {
            NK_SpawnShot(obj->user4, obj->posX + 256, obj->posY + 64, 2);
        }
        else
        {
            NK_SpawnShot(obj->user4, obj->posX - 128, obj->posY + 64, 6);
        }
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenJumpShoot2"))
    {
        if (obj->xDirection == 1)
        {
            NK_SpawnShot(obj->user4, obj->posX + 256, obj->posY + 32, 2);
        }
        else
        {
            NK_SpawnShot(obj->user4, obj->posX, obj->posY + 32, 6);
        }
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenJumpShootDown2"))
    {
        NK_SpawnShot(obj->user4, obj->posX + 128, obj->posY + 288, 4);
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenJumpShootUp2"))
    {
        NK_SpawnShot(obj->user4, obj->posX + 80, obj->posY - 160, 0);
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenShootUp1"))
    {
        NK_SpawnShot(obj->user4, obj->posX + 80, obj->posY - 160, 0);
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenPoleShoot1"))
    {
        if (obj->xDirection == 1)
        {
            NK_SpawnShot(obj->user4, obj->posX + 256, obj->posY + 64, 2);
        }
        else
        {
            NK_SpawnShot(obj->user4, obj->posX - 128, obj->posY + 64, 6);
        }
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenPoleShootUp1"))
    {
        if (obj->xDirection == 1)
        {
            NK_SpawnShot(obj->user4, obj->posX + 96, obj->posY + 64, 0);
        }
        else
        {
            NK_SpawnShot(obj->user4, obj->posX + 192, obj->posY + 64, 0);
        }
    }
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenPoleShootDown1"))
    {
        if (obj->xDirection == 1)
        {
            NK_SpawnShot(obj->user4, obj->posX + 96, obj->posY + 384, 4);
        }
        else
        {
            NK_SpawnShot(obj->user4, obj->posX + 192, obj->posY + 384, 4);
        }
    }
}

void NK_KeenSetupFunctions()
{
    CK_ACT_AddFunction("NK_KeenSlide",&NK_KeenSlide);
    CK_ACT_AddFunction("NK_KeenEnterDoor0",&NK_KeenEnterDoor0);
    CK_ACT_AddFunction("NK_KeenEnterDoor1",&NK_KeenEnterDoor1);
    CK_ACT_AddFunction("NK_KeenEnterDoor",&NK_KeenEnterDoor);
    CK_ACT_AddFunction("NK_KeenPlaceGem",&NK_KeenPlaceGem);
    CK_ACT_AddFunction("NK_KeenRunningThink",&NK_KeenRunningThink);
    CK_ACT_AddFunction("NK_KeenStandingThink",&NK_KeenStandingThink);
#if 0
    CK_ACT_AddFunction("NK_HandleInputOnGround",&NK_HandleInputOnGround);
#endif
    CK_ACT_AddFunction("NK_KeenLookUpThink",&NK_KeenLookUpThink);
    CK_ACT_AddFunction("NK_KeenLookDownThink",&NK_KeenLookDownThink);
    CK_ACT_AddFunction("NK_KeenPressSwitchThink",&NK_KeenPressSwitchThink);
    CK_ACT_AddFunction("NK_KeenDrawFunc",&NK_KeenDrawFunc);
    CK_ACT_AddFunction("NK_KeenRunDrawFunc",&NK_KeenRunDrawFunc);
    CK_ACT_AddFunction("NK_KeenJumpThink",&NK_KeenJumpThink);
    CK_ACT_AddFunction("NK_KeenJumpDrawFunc",&NK_KeenJumpDrawFunc);
    CK_ACT_AddFunction("NK_KeenPogoThink",&NK_KeenPogoThink);
    CK_ACT_AddFunction("NK_KeenPogoBounceThink",&NK_KeenPogoBounceThink);
    CK_ACT_AddFunction("NK_KeenPogoDrawFunc",&NK_KeenPogoDrawFunc);
    CK_ACT_AddFunction("NK_KeenSpecialDrawFunc",&NK_KeenSpecialDrawFunc);
    CK_ACT_AddColFunction("NK_KeenSpecialColFunc",&NK_KeenSpecialColFunc);
    CK_ACT_AddFunction("NK_KeenHangThink",&NK_KeenHangThink);
    CK_ACT_AddFunction("NK_KeenPullThink1",&NK_KeenPullThink1);
    CK_ACT_AddFunction("NK_KeenPullThink2",&NK_KeenPullThink2);
    CK_ACT_AddFunction("NK_KeenPullThink3",&NK_KeenPullThink3);
    CK_ACT_AddFunction("NK_KeenPullThink4",&NK_KeenPullThink4);
    CK_ACT_AddFunction("NK_KeenPoleSitThink",&NK_KeenPoleSitThink);
    CK_ACT_AddFunction("NK_KeenPoleUpThink",&NK_KeenPoleUpThink);
    CK_ACT_AddFunction("NK_KeenPoleDownThink",&NK_KeenPoleDownThink);
    CK_ACT_AddFunction("NK_KeenJumpDownThink",&NK_KeenJumpDownThink);
    CK_ACT_AddFunction("NK_KeenPoleDownDrawFunc",&NK_KeenPoleDownDrawFunc);
#if 0
    CK_ACT_AddFunction("NK_KeenSetClipped",&NK_KeenSetClipped);
#endif
    CK_ACT_AddColFunction("NK_KeenColFunc",&NK_KeenColFunc);
    CK_ACT_AddFunction("NK_KeenDeathThink",&NK_KeenDeathThink);
    CK_ACT_AddFunction("NK_KeenSpawnShot", &NK_KeenSpawnShot);
    CK_ACT_AddFunction("NK_ShotThink", &NK_ShotThink);
    CK_ACT_AddFunction("NK_ShotDrawFunc", &NK_ShotDrawFunc);

}
