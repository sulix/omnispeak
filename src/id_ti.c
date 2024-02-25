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

#include "ck_config.h"
#include "ck_cross.h"
#include "id_ca.h"

#include <stdint.h>

#ifdef CK_WARN_ON_TILEINFO_OOB
#define TI_WARN_ON_OOB() CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Out of bounds tileinfo access in %s (tile = %d, $%x)\n", __FUNCTION__, tile, tile)
#else
#define TI_WARN_ON_OOB() do { } while (0)
#endif

// Tileinfo

//TODO: Split this up into several arrays? That would probably be _very_ smart.
uint8_t *ti_tileInfo;

int8_t TI_BackAnimTile(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16;
	if (tile > ca_gfxInfoE.numTiles16)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_BackAnimTime(uint16_t tile)
{
	if (tile > ca_gfxInfoE.numTiles16)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[tile];
}

uint8_t TI_ForeTop(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_ForeBottom(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 2;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_ForeLeft(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 3;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_ForeRight(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_ForeMisc(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 5;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

int8_t TI_ForeAnimTile(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 4;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}

uint8_t TI_ForeAnimTime(uint16_t tile)
{
	const int index = tile + ca_gfxInfoE.numTiles16 * 2 + ca_gfxInfoE.numTiles16m * 6;
	if (tile > ca_gfxInfoE.numTiles16m)
	{
		TI_WARN_ON_OOB();
		return 0;
	}
	return ti_tileInfo[index];
}
