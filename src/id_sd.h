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

#ifndef	ID_SD_H
#define	ID_SD_H

#include "audiock5.h"

typedef	enum
{
	sdm_Off,
	sdm_PC,sdm_AdLib,
} SDMode;

typedef	enum
{
	smm_Off,smm_AdLib
} SMMode;

/* We do NOT use PCSound here as done in the original code
 * since it may fail to work as expected after compilation
 * with a different development environment.
 */

#if 0
typedef	struct
{
	uint32_t length;
	uint16_t priority;
} SoundCommon;

typedef	struct
{
	SoundCommon common;
	byte *data;
} PCSound;
#endif

extern SDMode SoundMode;

#endif
