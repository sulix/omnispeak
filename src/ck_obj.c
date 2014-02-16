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
#include "id_heads.h"
#include "id_rf.h"
#include "id_us.h"

// This file contains some object functions (think, etc) which are common to
// several episodes.

void CK_DoorOpen(CK_object *obj)
{
	uint16_t tilesToReplace[0x30];

	if (obj->user1 + 2 > 0x30)
	{
		Quit("Door too tall!");
	}

	for (int i = 0; i < obj->user1 + 2; ++i)
	{
		tilesToReplace[i] = CA_TileAtPos(obj->posX, obj->posY+i, 1) + 1;
	}

	RF_ReplaceTiles(tilesToReplace, 1, obj->posX, obj->posY, 1, obj->user1 + 2);
}

void CK_OBJ_SetupFunctions()
{
	CK_ACT_AddFunction("CK_DoorOpen", &CK_DoorOpen);
}
