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

#ifndef ID_TI_H
#define ID_TI_H

#include <stdint.h>

int8_t TI_BackAnimTile(uint16_t tile);

uint8_t TI_BackAnimTime(uint16_t tile);

uint8_t TI_ForeTop(uint16_t tile);

uint8_t TI_ForeLeft(uint16_t tile);

uint8_t TI_ForeBottom(uint16_t tile);

uint8_t TI_ForeRight(uint16_t tile);

uint8_t TI_ForeMisc(uint16_t tile);

int8_t TI_ForeAnimTile(uint16_t tile);

uint8_t TI_ForeAnimTime(uint16_t tile);

#endif
