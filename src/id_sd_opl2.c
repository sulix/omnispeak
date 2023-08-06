/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2019 Omnispeak Authors

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

// See https://github.com/DhrBaksteen/ArduinoOPL2
// But that's C++, and we only use C, so reimplement it.
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define OPL2_PIN_LATCH 1 // Pin 12
#define OPL2_PIN_ADDR 4  // Pin 16
#define OPL2_PIN_RESET 5 // Pin 18

#define OPL2_SPI_SPEED 8000000
#define OPL2_SPI_CHANNEL 0

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

#define SD_ADLIB_REG_PORT 0x388
#define SD_ADLIB_DATA_PORT 0x389

static volatile int timerDivisor = 1;

static SDL_mutex *soundSystemMutex = 0;
static SDL_Thread *t0Thread = 0;

void SD_OPL2_SetTimer0(int16_t int_8_divisor)
{
	//Configure the PIT frequency.
	timerDivisor = int_8_divisor;
}

void SD_OPL2_alOut(uint8_t reg, uint8_t val)
{
	digitalWrite(OPL2_PIN_ADDR, LOW);

	wiringPiSPIDataRW(OPL2_SPI_CHANNEL, &reg, 1);

	digitalWrite(OPL2_PIN_LATCH, LOW);
	delayMicroseconds(1);
	digitalWrite(OPL2_PIN_LATCH, HIGH);
	delayMicroseconds(4);

	digitalWrite(OPL2_PIN_ADDR, HIGH);
	wiringPiSPIDataRW(OPL2_SPI_CHANNEL, &val, 1);
	digitalWrite(OPL2_PIN_LATCH, LOW);
	delayMicroseconds(1);
	digitalWrite(OPL2_PIN_LATCH, HIGH);
	delayMicroseconds(23);
}

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_t0Service(void);

static uint64_t SD_LastPITTickTime;

static volatile bool soundSystemUp;

int SD_OPL2_t0InterruptThread(void *param)
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
			SD_LastPITTickTime = currPitTicks;
		}
	}
	return 0;
}

void SD_OPL2_PCSpkOn(bool on, int freq)
{
	// No-op.
}

void SD_OPL2_Startup(void)
{
	wiringPiSetup();
	wiringPiSPISetup(OPL2_SPI_CHANNEL, OPL2_SPI_SPEED);

	pinMode(OPL2_PIN_LATCH, OUTPUT);
	pinMode(OPL2_PIN_ADDR, OUTPUT);
	pinMode(OPL2_PIN_RESET, OUTPUT);

	digitalWrite(OPL2_PIN_LATCH, HIGH);
	digitalWrite(OPL2_PIN_ADDR, LOW);
	digitalWrite(OPL2_PIN_RESET, HIGH);

	soundSystemMutex = SDL_CreateMutex();
	soundSystemUp = true;

	uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
	SD_LastPITTickTime = currPitTicks;
	t0Thread = SDL_CreateThread(SD_OPL2_t0InterruptThread, "ID_SD: t0 interrupt thread.", NULL);
}

void SD_OPL2_Shutdown(void)
{
	soundSystemUp = false;
	SDL_WaitThread(t0Thread, NULL);
	SDL_DestroyMutex(soundSystemMutex);
}

bool SD_OPL2_mutexLocked = false;

void SD_OPL2_Lock()
{
	if (SD_OPL2_mutexLocked)
		Quit("Attempted sound system re-entry (locking an already locked mutex)");
	if (SDL_LockMutex(soundSystemMutex))
		Quit("Couldn't lock sound system mutex.");
	SD_OPL2_mutexLocked = true;
}

void SD_OPL2_Unlock()
{
	if (!SD_OPL2_mutexLocked)
		Quit("Tried to unlock the already unlocked sound system mutex.");
	SDL_UnlockMutex(soundSystemMutex);
	SD_OPL2_mutexLocked = false;
}

unsigned int SD_OPL2_Detect()
{
	return SD_CARD_OPL2;
}

void SD_OPL2_SetOPL3(bool on)
{
	// This card is OPL2 only
}

SD_Backend sd_opl2_backend = {
	.startup = SD_OPL2_Startup,
	.shutdown = SD_OPL2_Shutdown,
	.lock = SD_OPL2_Lock,
	.unlock = SD_OPL2_Unlock,
	.alOut = SD_OPL2_alOut,
	.pcSpkOn = SD_OPL2_PCSpkOn,
	.setTimer0 = SD_OPL2_SetTimer0,
	.detect = SD_OPL2_Detect,
	.setOPL3 = SD_OPL2_SetOPL3
};

SD_Backend *SD_Impl_GetBackend()
{
	return &sd_opl2_backend;
}
