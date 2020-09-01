/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2018 Omnispeak Authors

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "id_sd.h"
#include "ck_cross.h"

#define PC_PIT_RATE 1193182
#define SD_SFX_PART_RATE 140
/* In the original exe, upon setting a rate of 140Hz or 560Hz for some
 * interrupt handler, the value 1192030 divided by the desired rate is
 * calculated, to be programmed for timer 0 as a consequence.
 * For THIS value, it is rather 1193182 that should be divided by it, in order
 * to obtain a better approximation of the actual rate.
 */
#define SD_SOUND_PART_RATE_BASE 1192030

// Sort of replacements for x86 behaviors and assembly code

static bool SD_NULL_AudioSubsystem_Up;

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SD_NULL_SetTimer0(int16_t int_8_divisor)
{
}

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_t0Service(void);

void SD_NULL_alOut(uint8_t reg, uint8_t val)
{
}

void SD_NULL_PCSpkOn(bool on, int freq)
{
}

void SD_NULL_Startup(void)
{

	SD_NULL_AudioSubsystem_Up = true;
}

void SD_NULL_Shutdown(void)
{
	if (SD_NULL_AudioSubsystem_Up)
	{
		SD_NULL_AudioSubsystem_Up = false;
	}
}

bool SD_NULL_IsLocked = false;

void SD_NULL_Lock()
{
	if (SD_NULL_IsLocked)
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Tried to lock the audio system when it was already locked!\n");
	SD_NULL_IsLocked = true;
}

void SD_NULL_Unlock()
{
	if (!SD_NULL_IsLocked)
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Tried to unlock the audio system when it was already unlocked!\n");
	SD_NULL_IsLocked = false;
}

SD_Backend sd_null_backend = {
	.startup = SD_NULL_Startup,
	.shutdown = SD_NULL_Shutdown,
	.lock = SD_NULL_Lock,
	.unlock = SD_NULL_Unlock,
	.alOut = SD_NULL_alOut,
	.pcSpkOn = SD_NULL_PCSpkOn,
	.setTimer0 = SD_NULL_SetTimer0};

SD_Backend *SD_Impl_GetBackend()
{
	return &sd_null_backend;
}
