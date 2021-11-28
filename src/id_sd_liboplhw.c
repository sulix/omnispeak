/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2021 Omnispeak Authors

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

#include <SDL.h>
#include <unistd.h>

#include <oplhw.h>

#include "id_cfg.h"
#include "id_sd.h"
#include "id_us.h"
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

static oplhw_device *sd_oplhw_device;

static volatile int timerDivisor = 1;

static SDL_mutex *soundSystemMutex = 0;
static SDL_Thread *t0Thread = 0;

static SDL_cond *SD_OPLHW_TimerConditionVar;
static bool SD_OPLHW_WaitTicksSpin = false;

void SD_OPLHW_SetTimer0(int16_t int_8_divisor)
{
	//Configure the PIT frequency.
	timerDivisor = int_8_divisor;
}

void SDL_t0Service(void);

static uint64_t SD_LastPITTickTime;

static volatile bool soundSystemUp;

int SD_OPLHW_t0InterruptThread(void *param)
{
	while (soundSystemUp)
	{
		uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
		uint32_t ticks = (currPitTicks - SD_LastPITTickTime) / timerDivisor;

		if (ticks)
		{
			if (SDL_LockMutex(soundSystemMutex))
				continue;
			SDL_t0Service();
			SDL_UnlockMutex(soundSystemMutex);
			if (!SD_OPLHW_WaitTicksSpin)
				SDL_CondBroadcast(SD_OPLHW_TimerConditionVar);
			SD_LastPITTickTime = currPitTicks;
		}
	}
	return 0;
}

void SD_OPLHW_alOut(uint8_t reg, uint8_t val)
{
	if (sd_oplhw_device)
	{
		oplhw_Write(sd_oplhw_device, reg, val);
	}
}

void SD_OPLHW_PCSpkOn(bool on, int freq)
{
	// Setup operator
	SD_OPLHW_alOut(SD_ADLIB_REG_CHAR, 0);
	SD_OPLHW_alOut(SD_ADLIB_REG_VOLUME, 0);
	SD_OPLHW_alOut(SD_ADLIB_REG_ATTACK, 0xFF);
	SD_OPLHW_alOut(SD_ADLIB_REG_SUSPEND, 0x0);
	SD_OPLHW_alOut(SD_ADLIB_REG_WAVE, 0);
	SD_OPLHW_alOut(SD_ADLIB_REG_CHAR + 3, 0);
	SD_OPLHW_alOut(SD_ADLIB_REG_VOLUME + 3, 0);
	SD_OPLHW_alOut(SD_ADLIB_REG_ATTACK + 3, 0xFF);
	SD_OPLHW_alOut(SD_ADLIB_REG_SUSPEND + 3, 0x0);
	SD_OPLHW_alOut(SD_ADLIB_REG_WAVE + 3, 0);

	// Setup channel
	SD_OPLHW_alOut(SD_ADLIB_REG_CONNECTION, 0xF0);

	uint64_t fNum = freq ? (((PC_PIT_RATE / (freq)) << 16) / 49716) : 0;

	uint8_t hibyte = ((fNum >> 8) & 3) | 12;
	if (on)
		hibyte |= 0x20;
	else
		hibyte = 0;
	SD_OPLHW_alOut(SD_ADLIB_REG_NOTE_LO, fNum & 0xFF);
	SD_OPLHW_alOut(SD_ADLIB_REG_NOTE_HI, hibyte);
}

void SD_OPLHW_Startup(void)
{
	const char *deviceName = CFG_GetConfigString("sd_liboplhw_device", "");
	for (int i = 0; i < us_argc; ++i)
	{
		if (!CK_Cross_strcasecmp(us_argv[i], "/OPLHW"))
			deviceName = us_argv[i + 1];
	}

	// Setup a condition variable to signal threads waiting for timer updates.
	SD_OPLHW_WaitTicksSpin = CFG_GetConfigBool("sd_alsa_waitTicksSpin", false);
	if (!SD_OPLHW_WaitTicksSpin)
		SD_OPLHW_TimerConditionVar = SDL_CreateCond();

	sd_oplhw_device = oplhw_OpenDevice(deviceName);
	if (!sd_oplhw_device)
		Quit("Couldn't open liboplhw device!");

	soundSystemMutex = SDL_CreateMutex();
	soundSystemUp = true;

	uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
	SD_LastPITTickTime = currPitTicks;
	t0Thread = SDL_CreateThread(SD_OPLHW_t0InterruptThread, "ID_SD: t0 interrupt thread.", NULL);
}

void SD_OPLHW_Shutdown(void)
{
	oplhw_CloseDevice(sd_oplhw_device);
}

bool SD_OPLHW_mutexLocked = false;

void SD_OPLHW_Lock()
{
	if (SD_OPLHW_mutexLocked)
		Quit("Attempted sound system re-entry (locking an already locked mutex)");
	if (SDL_LockMutex(soundSystemMutex))
		Quit("Couldn't lock sound system mutex.");
	SD_OPLHW_mutexLocked = true;
}

void SD_OPLHW_Unlock()
{
	if (!SD_OPLHW_mutexLocked)
		Quit("Tried to unlock the already unlocked sound system mutex.");
	SDL_UnlockMutex(soundSystemMutex);
	SD_OPLHW_mutexLocked = false;
}

void SD_OPLHW_WaitTick()
{
	SDL_mutex *mtx = SDL_CreateMutex();
	SDL_LockMutex(mtx);
	// Timeout of 2ms, as the PIT rate is ~1.1ms..
	SDL_CondWaitTimeout(SD_OPLHW_TimerConditionVar, mtx, 2);
	SDL_UnlockMutex(mtx);
}


SD_Backend sd_liboplhw_backend = {
	.startup = SD_OPLHW_Startup,
	.shutdown = SD_OPLHW_Shutdown,
	.lock = SD_OPLHW_Lock,
	.unlock = SD_OPLHW_Unlock,
	.alOut = SD_OPLHW_alOut,
	.pcSpkOn = SD_OPLHW_PCSpkOn,
	.setTimer0 = SD_OPLHW_SetTimer0,
	.waitTick = SD_OPLHW_WaitTick};

SD_Backend *SD_Impl_GetBackend_OPLHW()
{
	return &sd_liboplhw_backend;
}
