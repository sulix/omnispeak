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
#include "id_sd.h"
#include "id_vl.h"
#include "ck_act.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck6_ep.h"

#include <stdio.h>
#include <string.h>

// =========================================================================

void CK6_MapMiscFlagsCheck(CK_object *obj)
{
	if (obj->user3 == 0)
	{
		int tileX = obj->clipRects.tileXmid;
		int tileY = RF_UnitToTile(obj->clipRects.unitY1 +
			(obj->clipRects.unitY2 - obj->clipRects.unitY1) / 2);
		uint16_t tile = CA_TileAtPos(tileX, tileY, 1);
		uint8_t miscValue = TI_ForeMisc(tile);

		if (miscValue == MISCFLAG_TELEPORT)
			CK_AnimateMapTeleporter(tileX, tileY);
	}
}

void CK6_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK6_MapMiscFlagsCheck", &CK6_MapMiscFlagsCheck);
}
