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

#include "id_ca.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_vl.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_act.h"
#include "ck5_ep.h"
#include <stdio.h>

void CK_SetDraw(CK_object *obj)
{
  obj->visible = true;
}

// Think function for adding gravity
void CK_Fall(CK_object *obj)
{
	CK_PhysGravityHigh(obj);
	ck_nextX = obj->velX * SD_GetSpriteSync();
}

// Think function for adding a slightly lower amount of gravity

void CK_Fall2(CK_object *obj)
{
	CK_PhysGravityMid(obj);
	ck_nextX = obj->velX * SD_GetSpriteSync();
}

void CK_Glide(CK_object *obj)
{
	ck_nextX = obj->velX * SD_GetSpriteSync();
	ck_nextY = obj->velY * SD_GetSpriteSync();
}

void CK_BasicDrawFunc1(CK_object *obj)
{
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * For walking and turning around at edges
 */
void CK_BasicDrawFunc2(CK_object *obj)
{
	// Hit wall walking right; turn around and go left
	if (obj->xDirection == IN_motion_Right && obj->leftTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Hit wall walking left; turn around and go right
	else if (obj->xDirection == IN_motion_Left && obj->rightTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Walked off of ledge; turn around
	else if (obj->topTI == 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * For slugs  (and maybe others???)
 */
void CK_BasicDrawFunc3(CK_object *obj)
{
	// Hit wall walking right; turn around and go left
	if (obj->xDirection == IN_motion_Right && obj->leftTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Hit wall walking left; turn around and go right
	else if (obj->xDirection == IN_motion_Left && obj->rightTI != 0)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
		// Walked off of ledge; turn around
	else if (obj->topTI == 0 || (obj->topTI & ~SLOPEMASK))
	{
		obj->posX -= obj->deltaPosX * 2;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Some weird placeholder
 */
void CK_ObjBadstate(CK_object *obj)
{
	Quit("Object with bad state!");
}

/*
 * Think function for stunned creatures
 */
void CK_BasicDrawFunc4(CK_object *obj)
{
	int starsX, starsY;

	// Handle physics
	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = 0;
	}

	if (obj->bottomTI)
	{
		obj->velY = 0;
	}

	if (obj->topTI)
	{
		obj->velX = obj->velY = 0;
		if (obj->currentAction->next)
		{
			CK_SetAction2(obj, obj->currentAction->next);
		}
	}

	// Draw the primary chunk
	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);


	// Draw the stunner stars, offset based on the initial type of the stunned
	// critter

	starsX = starsY = 0;

  if (ck_currentEpisode->ep == EP_CK4)
  {

    switch (obj->user4)
    {

      case CT4_Slug:
        starsY = -0x80;
        break;

      case CT4_Egg:
        starsX = 0x80;
        starsY = -0x80;
        break;

      case CT4_Wormmouth:
        starsX = 0x40;
        starsY = -350;
        break;

      case CT4_Bounder:
        starsX = 0x40;
        starsY = -0x80;
        break;

      case CT4_Inchworm:
        starsX = -0x40;
        starsY = -0x80;
        break;

      case CT4_Lick:
        starsY = -0x80;
        break;

      case CT4_Smirky:
        starsX = 0x80;
        break;
    }
  }
  else if (ck_currentEpisode->ep == EP_CK5)
  {
    switch (obj->user4)
    {
    case CT5_Sparky:
      starsX += 0x40;
      break;
    case CT5_Ampton:
      starsY -= 0x80;
      break;
    case CT5_Korath:
      starsY -= 0x80;
      break;
    }
  }

	// Tick the star 3-frame animation forward
	if ((obj->user1 += SD_GetSpriteSync()) > 10)
	{
		obj->user1 -= 10;
		if (++obj->user2 >= 3)
			obj->user2 = 0;
	}

	// FIXME: Will cause problems on 64-bit systems
	RF_AddSpriteDraw((RF_SpriteDrawEntry**) (&obj->user3), obj->posX + starsX, obj->posY + starsY, obj->user2 + SPR_STARS1, false, 3);
}

void CK_StunCreature(CK_object *creature, CK_object *stunner, CK_action *new_creature_act)
{
	// Kill the stunner shot
	CK_ShotHit(stunner);

	// Set stunned creature action
	creature->user1 = creature->user2 = creature->user3 = 0;
	creature->user4 = creature->type;
	CK_SetAction2(creature, new_creature_act);
  creature->type = CT_CLASS(StunnedCreature);

	// Make the creature jump up a bit
	if ((creature->velY -= 0x18) < -0x30)
		creature->velY = -0x30;
}

void CK_DeadlyCol(CK_object *o1, CK_object *o2)
{
	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
	}
	else if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK_LethalCol(CK_object *o1, CK_object *o2)
{
	if (o2->type == CT_Player)
	{
		CK_KillKeen();
	}
}

void CK_Misc_SetupFunctions(void)
{
  CK_ACT_AddFunction("CK_SetDraw", &CK_SetDraw);
	CK_ACT_AddFunction("CK_Fall", &CK_Fall);
	CK_ACT_AddFunction("CK_Fall2", &CK_Fall2);
	CK_ACT_AddFunction("CK_Glide", &CK_Glide);
	CK_ACT_AddFunction("CK_BasicDrawFunc1", &CK_BasicDrawFunc1);
	CK_ACT_AddFunction("CK_BasicDrawFunc2", &CK_BasicDrawFunc2);
	CK_ACT_AddFunction("CK_BasicDrawFunc3", &CK_BasicDrawFunc3);
	CK_ACT_AddFunction("CK_ObjBadstate", &CK_ObjBadstate);
	CK_ACT_AddFunction("CK_BasicDrawFunc4", &CK_BasicDrawFunc4);
  CK_ACT_AddColFunction("CK_DeadlyCol", &CK_DeadlyCol);
  CK_ACT_AddColFunction("CK_LethalCol", &CK_LethalCol);
}
