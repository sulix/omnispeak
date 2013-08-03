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

//#inclucd "id_heads.h"
#include "ck_play.h"
#include "ck_game.h"
#include "ck_def.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_us.h"
#include "id_rf.h"
#include "id_vl.h"

#include "ck_act.h"
#include "ck5_ep.h"

#include <stdlib.h> /* For abs() */
#include <string.h> /* For memset() */

#define max(a,b) ((a<b)?b:a)
#define min(a,b) ((a<b)?a:b)

CK_object ck_objArray[CK_MAX_OBJECTS];

CK_object *ck_freeObject;
CK_object *ck_lastObject;
int ck_numObjects;

CK_object *ck_keenObj;

static CK_object tempObj;

static long ck_numTotalTics;
static int ck_ticsThisFrame;

// The rectangle within which objects are active.
static int ck_activeX0Tile;
static int ck_activeY0Tile;
static int ck_activeX1Tile;
static int ck_activeY1Tile;


void CK_KeenCheckSpecialTileInfo(CK_object *obj);

bool ck_demoEnabled;

CK_GameState ck_gameState;


void CK_DebugMemory()
{
	US_CenterWindow(16,10);

	US_CPrint("Memory Usage:");
	US_CPrint("-------------");
	US_PrintF("In Use      : %dk", MM_UsedMemory()/1024);
	US_PrintF("Blocks      : %d", MM_UsedBlocks());
	US_PrintF("Purgable    : %d", MM_PurgableBlocks());
	US_PrintF("GFX Mem Used: %dk", VL_MemUsed()/1024);
	US_PrintF("GFX Surfaces: %d", VL_NumSurfaces());
	VL_Present();
	IN_WaitKey();
	//MM_ShowMemory();
}




void CK_SetTicsPerFrame()
{
	if (!ck_demoEnabled)
		ck_ticsThisFrame = VL_GetTics(true);
	else
	{
		VL_GetTics(true);
		ck_ticsThisFrame = 2;
	}
	ck_numTotalTics += ck_ticsThisFrame;
}

int CK_GetTicksPerFrame()
{
	return ck_ticsThisFrame;
}

long CK_GetNumTotalTics()
{
	return ck_numTotalTics;
}


void CK_SetupObjArray()
{
	for (int i = 0; i < CK_MAX_OBJECTS; ++i)
	{
		ck_objArray[i].prev = &(ck_objArray[i+1]);
		ck_objArray[i].next = 0;
	}

	ck_objArray[CK_MAX_OBJECTS-1].prev = 0;

	ck_freeObject = &ck_objArray[0];
	ck_lastObject = 0;
	ck_numObjects = 0;

	ck_keenObj = CK_GetNewObj(false);

	// TODO: Add Andy's `special object'?
}

CK_object *CK_GetNewObj(bool nonCritical)
{
	if (!ck_freeObject)
	{
		if (nonCritical)
		{
			//printf("Warning: No free spots in objarray! Using temp object\n");
			return &tempObj;
		}
		else
			Quit("GetNewObj: No free spots in objarray!");
	}

	CK_object *newObj = ck_freeObject;
	ck_freeObject = ck_freeObject->prev;

	//Clear any old crap out of the struct.
	memset(newObj, 0, sizeof(CK_object));


	if (ck_lastObject)
	{
		ck_lastObject->next = newObj;
	}
	newObj->prev = ck_lastObject;
	

	newObj->active = true;
	newObj->clipped = true;

	ck_lastObject = newObj;
	ck_numObjects++;


	return newObj;
}


void CK_RemoveObj(CK_object *obj)
{
	if (obj == ck_keenObj)
	{
		Quit("RemoveObj: Tried to remove the player!");
	}

	if (obj == ck_lastObject)
		ck_lastObject = obj->prev;
	else
		obj->next->prev = obj->prev;
	
	obj->prev->next = obj->next;
	obj->next = 0;

	obj->prev = ck_freeObject;
	ck_freeObject = obj;
	ck_numObjects--;
}


int CK_ActionThink(CK_object *obj, int time)
{
	CK_action *action = obj->currentAction;


	// ThinkMethod: 2
	if (obj->currentAction->type == AT_Frame)
	{
		if (obj->currentAction->think)
		{
			if (obj->timeUntillThink)
			{
				obj->timeUntillThink--;
			}
			else
			{
				obj->currentAction->think(obj);
			}
		}
		return 0;
	}

	int newTime = time + obj->actionTimer;

	// If we won't run out of time.
	if (action->timer > newTime || !(action->timer))
	{
		obj->actionTimer = newTime;

		if (action->type == AT_ScaledOnce || action->type == AT_ScaledFrame)
		{
			if (obj->xDirection)
			{
				//TODO: Work out what to do with the nextVelocity stuff.
				obj->nextX += action->velX * time * obj->xDirection;
			}
			if (obj->yDirection)
			{
				obj->nextY += action->velY * time * obj->yDirection;
			}
		}

		if (action->type == AT_UnscaledFrame || action->type == AT_ScaledFrame)
		{
			if (action->think)
			{
				if (obj->timeUntillThink)
					obj->timeUntillThink--;
				else
				{
					obj->currentAction->think(obj);
				}
			}
		}
		return 0;
	}
	else
	{
		int remainingTime = action->timer - obj->actionTimer;
		newTime -= action->timer;
		obj->actionTimer = 0;

		if (action->type == AT_ScaledOnce || action->type == AT_ScaledFrame)
		{
			if (obj->xDirection)
			{
				obj->nextX += action->velX * remainingTime * obj->xDirection;
			}
			if (obj->yDirection)
			{
				obj->nextY += action->velY * remainingTime * obj->yDirection;
			}
		}
		else if (action->type == AT_UnscaledOnce || action->type == AT_UnscaledFrame)
		{
			if (obj->xDirection)
			{
				obj->nextX += action->velX * obj->xDirection;
			}
			if (obj->yDirection)
			{
				obj->nextY += action->velY * obj->yDirection;
			}
		}

		if (action->think)
		{
				if (obj->timeUntillThink)
					obj->timeUntillThink--;
				else
				{
					obj->currentAction->think(obj);
				}
		}

		if (action != obj->currentAction)
		{
			if (!obj->currentAction)
			{
				return 0;
			}
		}
		else
		{
			obj->currentAction = action->next;
			//obj->actionTimer = obj->currentAction->timer;
		}
	}
	return newTime;
}

void CK_RunAction(CK_object *obj)
{
	int oldChunk = obj->gfxChunk;
	int tics = CK_GetTicksPerFrame();

	int oldPosX = obj->posX;
	int oldPosY = obj->posY;

	obj->deltaPosX = obj->deltaPosY = 0;

	obj->nextX = obj->nextY = 0;

	CK_action *prevAction = obj->currentAction;

	int ticsLeft = CK_ActionThink(obj, tics);

	if (obj->currentAction != prevAction)
	{
		obj->actionTimer = 0;
		prevAction = obj->currentAction;
	}

	while (obj->currentAction && ticsLeft)
	{
		if (obj->currentAction->protectAnimation || obj->currentAction->timer > ticsLeft)
		{
			ticsLeft = CK_ActionThink(obj, ticsLeft);
		}
		else
		{
			ticsLeft = CK_ActionThink(obj, obj->currentAction->timer - 1);
		}

		if (obj->currentAction != prevAction)
		{
			obj->actionTimer = 0;
			prevAction = obj->currentAction;
		}
	
}
	
	if (!obj->currentAction)
	{
		RF_RemoveSpriteDraw(&obj->sde);
		CK_RemoveObj(obj);
		return;
	}

	if (obj->currentAction->chunkLeft && obj->xDirection <= 0) obj->gfxChunk = obj->currentAction->chunkLeft;
	if (obj->currentAction->chunkRight && obj->xDirection > 0) obj->gfxChunk = obj->currentAction->chunkRight;

	if (obj->gfxChunk != oldChunk || obj->nextX || obj->nextY || obj->topTI == 0x19)
	{
		CK_PhysUpdateNormalObj(obj);
	}
}

void hackdraw(CK_object *me)
{
	RF_AddSpriteDraw(&(me->sde),(150 << 4),(100 << 4),121,false,0);
}

void CK_DebugKeys()
{
	if (IN_GetKeyState(IN_SC_J))
	{
		ck_gameState.jumpCheat = !ck_gameState.jumpCheat;
		//TODO: Something here?
		US_CenterWindow(18,3);

		if (ck_gameState.jumpCheat)
			US_CPrint("\nJump cheat ON");
		else
			US_CPrint("\nJump cheat OFF");

		VL_Present();
		IN_WaitKey();
	}

	if (IN_GetKeyState(IN_SC_M))
	{
		CK_DebugMemory();
	}
}

// Check non-game keys
void CK_CheckKeys()
{
	if(IN_GetKeyState(IN_SC_F10))
	{
		CK_DebugKeys();
	}
}

IN_ControlFrame ck_inputFrame;

void CK_HandleInput()
{
	IN_ReadControls(0, &ck_inputFrame);

	static int pogoTimer = 0;

	if (ck_demoEnabled) // Two-button firing mode.
	{
		ck_keenState.shootIsPressed = ck_inputFrame.jump && ck_inputFrame.pogo;
		if (ck_keenState.shootIsPressed)
		{
			ck_keenState.jumpWasPressed = ck_keenState.jumpIsPressed = false;
			ck_keenState.pogoWasPressed = ck_keenState.pogoIsPressed = false;
		}
		else
		{
			ck_keenState.shootWasPressed = false;
			ck_keenState.jumpWasPressed = false;
			ck_keenState.jumpIsPressed = ck_inputFrame.jump;
			
			ck_keenState.pogoWasPressed = false;
			if (ck_inputFrame.pogo)
			{
				// Here be dragons!
				// In order to better emulate the original trilogy's controls, a delay
				// is introduced when pogoing in two-button firing.
				if (pogoTimer < 9)
				{
					pogoTimer += CK_GetTicksPerFrame();
				}
				else
				{
					ck_keenState.pogoIsPressed = true;
				}
			}
			else
			{
				// If the player lets go of pogo, pogo immediately.
				if (pogoTimer)
				{
					ck_keenState.pogoIsPressed = true;
				}
				else
				{
					ck_keenState.pogoIsPressed = false;
				}
				pogoTimer = 0;
			}
		}
	}
	else
	{
		ck_keenState.jumpIsPressed = ck_inputFrame.jump;
		ck_keenState.pogoIsPressed = ck_inputFrame.pogo;
		if (IN_GetKeyState(IN_SC_Space)) ck_keenState.shootIsPressed = true;
		else ck_keenState.shootIsPressed = false;
		if (!ck_keenState.jumpIsPressed) ck_keenState.jumpWasPressed = false;
		if (!ck_keenState.pogoIsPressed) ck_keenState.pogoWasPressed = false;
		if (!ck_keenState.shootIsPressed) ck_keenState.shootWasPressed = false;
	}


	CK_CheckKeys();

}

extern int rf_scrollXUnit;
extern int rf_scrollYUnit;

// Centre the camera on the given object.
void CK_CentreCamera(CK_object *obj)
{
	int screenX, screenY;

	if (obj->posX < (152 << 4))
		screenX = 0;
	else
		screenX = obj->posX - (152 << 4);

	if (obj->clipRects.unitY2 < (140 << 4))
		screenY = 0;
	else
		screenY = obj->posY - (140 << 4);

	RF_Reposition(screenX, screenY);

	//TODO: This is 4 in Andy's disasm.
	ck_activeX0Tile = max((rf_scrollXUnit >> 8) - 6, 0);
	ck_activeX1Tile = max((rf_scrollXUnit >> 8) + (320 >> 4) + 6, 0);
	ck_activeY0Tile = max((rf_scrollYUnit >> 8) - 6, 0);
	ck_activeY1Tile = max((rf_scrollYUnit >> 8) + (200 >> 4) + 6, 0);
}

// Run the normal camera which follows keen
void CK_NormalCamera(CK_object *obj)
{
	//TODO: some unknown var must be 0
	
	//TODO: Check if keen is outside map bounds.

	int deltaX = 0, deltaY = 0;	// in Units

	// The indended y-coordinate of the bottom of the keen sprite
	// in pixels from the top of the screen.
	static int screenYpx = 140 << 4;

	// Keep keen's x-coord between 144-192 pixels
	if (obj->posX < (rf_scrollXUnit + (144 << 4)))
		deltaX = obj->posX - (rf_scrollXUnit+(144 << 4));
	
	if (obj->posX > (rf_scrollXUnit + (192 << 4)))
		deltaX = obj->posX - (rf_scrollXUnit+(192 << 4));


	// Keen should be able to look up and down.
	if (obj->currentAction == CK_GetActionByName("CK_ACT_keenLookUp2"))
	{
		int pxToMove;
		if (screenYpx + CK_GetTicksPerFrame() > 167)
		{
			// Keen should never be so low on the screen that his
			// feet are more than 167 px from the top.
			pxToMove = 167 - screenYpx;
		}
		else
		{
			// Move 1px per tick.
			pxToMove = CK_GetTicksPerFrame();
		}
		screenYpx += pxToMove;
		deltaY = (-pxToMove) << 4;

	}
	else if (obj->currentAction == CK_GetActionByName("CK_ACT_keenLookDown2"))
	{
		int pxToMove;
		if (screenYpx - CK_GetTicksPerFrame() < 33)
		{
			// Keen should never be so high on the screen that his
			// feet are fewer than 33 px from the top.
			pxToMove = screenYpx - 33;
		}
		else
		{
			// Move 1px/tick.
			pxToMove = CK_GetTicksPerFrame();
		}
		screenYpx -= pxToMove;
		deltaY = pxToMove << 4;
	}

	// If we're attached to the ground, or otherwise awesome
	// do somethink inscrutible.
	if (obj->topTI || !obj->clipped || obj->currentAction == CK_GetActionByName("CK_ACT_KeenHang1"))
	{
		deltaY += obj->deltaPosY;

		//TODO: Something hideous
		if ((screenYpx << 4) + rf_scrollYUnit + deltaY !=  (obj->clipRects.unitY2))
		{
			int adjAmt = (((screenYpx << 4) + rf_scrollYUnit + deltaY - obj->clipRects.unitY2));
			int adjAmt2 = abs(adjAmt / 8);

			adjAmt2 = (adjAmt2 <= 48)?adjAmt2:48;
			
			if (adjAmt > 0)
				deltaY -= adjAmt2;
			else
				deltaY += adjAmt2;
			
		}
				
	}
	else
	{
		// Reset to 140px.
		screenYpx = 140;
	}
	

	// Scroll the screen to keep keen between 33 and 167 px.
	if (obj->clipRects.unitY2 < (rf_scrollYUnit + deltaY + (32 << 4)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + (32 << 4));

	
	if (obj->clipRects.unitY2 > (rf_scrollYUnit + deltaY + (168 << 4)))
		deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + (168 << 4));

	//Don't scroll more than one tile's worth per frame.
	if (deltaX || deltaY)
	{
		if (deltaX > 255) deltaX = 255;
		else if (deltaX < -255) deltaX = -255;

		if (deltaY > 255) deltaY = 255;
		else if (deltaY < -255) deltaY = -255;

		// Do the scroll!
		RF_SmoothScroll(deltaX, deltaY);
	
		// Update the rectangle of active objects
		ck_activeX0Tile = max((rf_scrollXUnit >> 8) - 6, 0);
		ck_activeX1Tile = max((rf_scrollXUnit >> 8) + (320 >> 4) + 6, 0);
		ck_activeY0Tile = max((rf_scrollYUnit >> 8) - 6, 0);
		ck_activeY1Tile = max((rf_scrollYUnit >> 8) + (200 >> 4) + 6, 0);
	}
}

//TODO: Add some demo number stuff
void CK_PlayDemo(int demoNumber)
{
	uint8_t *demoBuf;

	int demoChunk = 4926 + demoNumber;

//	CK_NewGame();

	CA_CacheGrChunk(demoChunk);
	demoBuf = ca_graphChunks[demoChunk];
	//MM_SetLock(&CA_CacheGrChunk
	
	uint16_t demoMap = *demoBuf;
	demoBuf += 2;
	uint16_t demoLen = *((uint16_t *)demoBuf);
	demoBuf += 2;

	ck_currentMapNumber =demoMap;

	CK_LoadLevel(true);

	ck_demoEnabled = true;

	ck_gameState.difficulty = D_Normal;

	IN_DemoStartPlaying(demoBuf,demoLen);


	CK_PlayLoop();

	// What should we do after playing the demo?
	CK_HandleDemoKeys();

	// We have to get rid of the demo buffer, as if we want to play it
	// again, we need a fresh copy. ID_IN modifies the buffer.
	MM_FreePtr(&ca_graphChunks[demoChunk]);
}


int ck_currentMapNumber;
// Play a level.
int CK_PlayLoop()
{

	// Note that this appears in CK_LoadLevel as well
	if (ck_demoEnabled)
		US_InitRndT(false);
	else
		US_InitRndT(true);

	// Should check this, from k5disasm
	ck_numTotalTics = 3;
	ck_ticsThisFrame = 3;

	CK_LoadLevel(true);

	// If this is nonzero, the level will quit immediately.
	ck_gameState.levelState = 0;

	CK_CentreCamera(ck_keenObj);
	while (ck_gameState.levelState == 0)
	{

		IN_PumpEvents();
		CK_SetTicsPerFrame();
		CK_HandleInput();

		// Set, unset active objects.
		for (CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{

			if (!currentObj->active &&
				(currentObj->clipRects.tileX2 >= (rf_scrollXUnit >> 8) - 1) &&
				(currentObj->clipRects.tileX1 <= (rf_scrollXUnit >> 8) + (320 >> 4) + 1) &&
				(currentObj->clipRects.tileY1 <= (rf_scrollYUnit >> 8) + (200 >> 4) + 1) &&
				(currentObj->clipRects.tileY2 >= (rf_scrollYUnit >> 8) - 1))
			{
				currentObj->active = OBJ_ACTIVE;
				currentObj->visible = true;
			}
			else if (currentObj->active && currentObj != ck_keenObj && (
				(currentObj->clipRects.tileX2 <= ck_activeX0Tile) ||
				(currentObj->clipRects.tileX1 >= ck_activeX1Tile) ||
				(currentObj->clipRects.tileY1 >= ck_activeY1Tile) ||
				(currentObj->clipRects.tileY2 <= ck_activeY0Tile)))
			{
				RF_RemoveSpriteDraw(&currentObj->sde);
				//TODO: Add an Episode callback. Ep 4 requires
				// type 33 to remove int33 (Andy's decomp)
				if (currentObj->active == OBJ_EXISTS_ONLY_ONSCREEN)
				{
					CK_RemoveObj(currentObj);
					continue;
				}
				else if (currentObj->active != OBJ_ALWAYS_ACTIVE)
				{
					if (US_RndT() < ck_numTotalTics * 2)
					{
						currentObj->active = OBJ_INACTIVE;
						continue;
					}
				}

			}

			if (currentObj->active)
				CK_RunAction(currentObj);
		}
				
				
		if (ck_keenState.platform)
			CK_KeenRidePlatform(ck_keenObj);

		for(CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{
			// Some strange Keen4 stuff here. Ignoring for now.


			if (!currentObj->active) continue;
			for (CK_object *collideObj = currentObj->next; collideObj; collideObj = collideObj->next)
			{
				if (!collideObj->active)
					continue;

				if (	(currentObj->clipRects.unitX2 > collideObj->clipRects.unitX1) &&
					(currentObj->clipRects.unitX1 < collideObj->clipRects.unitX2) &&
					(currentObj->clipRects.unitY1 < collideObj->clipRects.unitY2) &&
					(currentObj->clipRects.unitY2 > collideObj->clipRects.unitY1) )
				{
					if (currentObj->currentAction->collide)
						currentObj->currentAction->collide(currentObj,collideObj);
					if (collideObj->currentAction->collide)
						collideObj->currentAction->collide(collideObj,currentObj);
				}
			}
		}

		//TODO: If world map and keen4, check wetsuit.

		//TODO: If not world map, check keen -> item-tile collision.

		CK_KeenCheckSpecialTileInfo(ck_keenObj);



		for(CK_object *currentObj = ck_keenObj; currentObj; currentObj = currentObj->next)
		{
			if (currentObj->active)
			{
				//TODO: Check if object has fallen off the bottom of the map.
				//CK_ActionThink(currentObj,1);	
				//else...
				if (currentObj->visible && currentObj->currentAction->draw)
				{
					currentObj->visible = false;	//We don't need to render it twice!
					currentObj->currentAction->draw(currentObj);
				}
			}
		}

		//TODO: Follow player with camera.

		RF_Refresh();
#if 0
		for (CK_object *obj = ck_keenObj; obj; obj = obj->next){
		VL_ScreenRect((obj->clipRects.tileX1 << 4) - (rf_scrollXUnit >> 4), (obj->clipRects.tileY1 << 4) - (rf_scrollYUnit>>4),
			(obj->clipRects.tileX2 - obj->clipRects.tileX1 + 1) << 4,
			(obj->clipRects.tileY2 - obj->clipRects.tileY1 + 1) << 4, 10);

		VL_ScreenRect((obj->clipRects.tileXmid << 4) - (rf_scrollXUnit >> 4), (obj->clipRects.tileY2 << 4) - (rf_scrollYUnit>>4),16,16,9);
		VL_ScreenRect((obj->clipRects.unitX1 >> 4) - (rf_scrollXUnit >> 4), (obj->clipRects.unitY1 >> 4) - (rf_scrollYUnit>>4),(obj->clipRects.unitX2 - obj->clipRects.unitX1) >> 4,(obj->clipRects.unitY2 - obj->clipRects.unitY1) >> 4,8);
		}
#endif
		VL_SetScrollCoords((rf_scrollXUnit & 0xff) >> 4,(rf_scrollYUnit & 0xff) >> 4);
		CK_NormalCamera(ck_keenObj);
		//TODO: Slow-mo, extra VBLs.
		VL_Present();
		// If we've finished playing back our demo, or the player presses a key,
		// exit the playloop.
		if (IN_DemoGetMode() == IN_Demo_Playback && IN_GetLastScan() != IN_SC_None )
		{
			ck_gameState.levelState = 2;
			ck_demoEnabled = false;
			IN_DemoStopPlaying();
		}
		else if (IN_DemoGetMode() == IN_Demo_PlayDone)
		{
			ck_gameState.levelState = 2;
		}


	}
}
