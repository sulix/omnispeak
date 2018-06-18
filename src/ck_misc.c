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

#include <stdio.h>
#include "id_ca.h"
#include "id_in.h"
#include "id_rf.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck5_ep.h"

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

	else if (ck_currentEpisode->ep == EP_CK6)
	{
		switch (obj->user4)
		{
		case CT6_Bloogguard:
			starsX = 0x100;
			starsY = -0x40;
			break;

		case CT6_Flect:
			starsX = 0x40;
			starsY = -0x40;
			break;

		case CT6_Bloog:
		case CT6_Nospike:
			starsX = 0x80;
			starsY = -0x40;
			break;

		case CT6_Blooglet:
		case CT6_Babobba:
			starsY = -0x80;
			break;

		case CT6_Fleex:
			starsX = 0x100;
			starsY = 0x80;
			break;

		case CT6_Ceilick:
			starsY = 0xC0;
			break;

		default:
			Quit("No star spec for object!");
		}
	}

	// Tick the star 3-frame animation forward
	if ((obj->user1 += SD_GetSpriteSync()) > 10)
	{
		obj->user1 -= 10;
		if (++obj->user2 >= 3)
			obj->user2 = 0;
	}

	RF_AddSpriteDrawUsing16BitOffset(&obj->user3, obj->posX + starsX, obj->posY + starsY, obj->user2 + SPR_STARS1, false, 3);
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

/*
 * This comes before DieOnContactDraw in the CK6 Disassembly
 * Even though it is  only used for Giks, it is  not placed next to
 * the rest of the Gik code
 * Maybe leftover melon cart code?
 */

void CK_PushKeenCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
	{
		ck_keenIgnoreVertClip = true;
		CK_PhysPushX(b, a);
		ck_keenIgnoreVertClip = false;
	}
}

void CK_CarryKeenCol(CK_object *a, CK_object *b)
{
	if (b->type == CT_Player)
		CK_PhysPushY(b, a);
}

/*
 * This comes right before ShrapnelTileCol in the CK6 Disassembly
 * The use of the "currenAction->next" pattern suggests that it was another
 * episode-independent Draw function, even though it is apparently only used for the
 * bipship. Perhaps another remnant from Keen Dreams
 */

void CK_DieOnContactDraw(CK_object *obj)
{
	if (obj->rightTI || obj->leftTI)
		obj->velX = 0;

	if (obj->bottomTI)
		obj->velY = 0;

	if (obj->topTI)
	{
		obj->velY = 0;
		if (obj->currentAction)
		{
			CK_SetAction2(obj, obj->currentAction->next);
		}
		else
		{
			CK_RemoveObj(obj);
			return;
		}
	}

	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

/*
 * Tile collision for making the mine and shelley bits bounce, as well as the
 * babobba shot
 * It was also used in Keen Dreams for the flower power
 */

void CK_ShrapnelTileCol(CK_object *obj)
{
	int bouncePower; // is long in keen source
	int topflags_0, velY, absvelX;
	int d, motion;

	static int bouncearray[8][8] = {
		{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		{0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0},
		{0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
		{0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
		{0x3, 0x2, 0x1, 0x0, 0xF, 0xE, 0xD, 0xC},
		{0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
		{0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
		{0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4},
	};

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);

	// Bounce backwards if a side wall is hit
	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = -obj->velX / 2;
	}

	// Bounce down if a top wall is hit
	if (obj->bottomTI)
	{
		obj->velY = -obj->velY / 2;
		return;
	}

	// Bounce sideways if a slope floor is hit
	topflags_0 = obj->topTI;
	if (!topflags_0)
		return;

	if (obj->velY < 0)
		obj->velY = 0;

	absvelX = ABS(obj->velX);
	velY = obj->velY;

	if (absvelX > velY)
	{
		if (absvelX > velY * 2)
		{
			d = 0;
			bouncePower = absvelX * 286;
		}
		else
		{
			d = 1;
			bouncePower = absvelX * 362;
		}
	}
	else
	{
		if (velY > absvelX * 2)
		{
			d = 3;
			bouncePower = velY * 256;
		}
		else
		{
			d = 2;
			bouncePower = velY * 286;
		}
	}

	if (obj->velX > 0) //mirror around y-axis
		d = 7 - d;

	bouncePower /= 2; // absorb energy

	motion = bouncearray[obj->topTI][d];

	switch (motion)
	{
	case 0:
		obj->velX = bouncePower / 286;
		obj->velY = -obj->velX / 2;
		break;
	case 1:
		obj->velX = bouncePower / 362;
		obj->velY = -obj->velX;
		break;
	case 2:
		obj->velY = -(bouncePower / 286);
		obj->velX = -obj->velY / 2;
		break;
	case 3:
	case 4:
		obj->velX = 0;
		obj->velY = -(bouncePower / 256);
		break;
	case 5:
		obj->velY = -(bouncePower / 286);
		obj->velX = obj->velY / 2;
		break;
	case 6:
		obj->velY = -(bouncePower / 362);
		obj->velX = obj->velY;
		break;
	case 7:
		obj->velX = -(bouncePower / 286);
		obj->velY = obj->velX / 2;
		break;
	case 8:
		obj->velX = -(bouncePower / 286);
		obj->velY = -obj->velX / 2;
		break;
	case 9:
		obj->velX = -(bouncePower / 362);
		obj->velY = -obj->velX;
		break;
	case 0xA:
		obj->velY = bouncePower / 286;
		obj->velX = -obj->velY / 2;
		break;
	case 0xB:
	case 0xC:
		obj->velX = 0;
		obj->velY = -(bouncePower / 256);
		break;
	case 0xD:
		obj->velY = bouncePower / 286;
		obj->velX = obj->velY / 2;
		break;
	case 0xE:
		obj->velX = bouncePower / 362;
		obj->velY = bouncePower / 362;
		break;
	case 0xF:
		obj->velX = bouncePower / 256;
		obj->velY = obj->velX / 2;
		break;
	default:
		break;
	}

	// if speed is lower than threshhold, then disappear
	if (bouncePower < 0x1000)
	{
		// The babobba shot DOES have a next action, whereas the CK5 shrapnel doesn't
		// The original .exe has no check for a  NULL action here
		// we can't advance to a NULL action, because omnispeak will segfault (Keen5 wouldn't)
		// so we need to check if the next action is NULL

		if (obj->currentAction->next)
			CK_SetAction2(obj, obj->currentAction->next);
		else
		{
			// In order to get dumps to match properly, though, we
			// need obj->currentAction to be NULL.
			// In Keen5, oftern obj->visible is set, too, as
			// the object has moved (even if it is now invalid.
			obj->currentAction = 0;
			obj->visible = true;
			RF_RemoveSpriteDraw(&obj->sde);
			CK_RemoveObj(obj);
		}
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

	CK_ACT_AddColFunction("CK_PushKeenCol", &CK_PushKeenCol);
	CK_ACT_AddColFunction("CK_CarryKeenCol", &CK_CarryKeenCol);
	CK_ACT_AddFunction("CK_ShrapnelTileCol", &CK_ShrapnelTileCol);
	CK_ACT_AddFunction("CK_DieOnContactDraw", &CK_DieOnContactDraw);
}
