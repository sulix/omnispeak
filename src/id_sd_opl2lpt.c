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
#ifdef SD_OPL2_WITH_IOPERM
#include <sys/io.h>
#endif

#include <SDL.h>
#ifdef SD_OPL2_WITH_IEEE1284
#include <ieee1284.h>
#endif
#include <unistd.h>

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

#define SD_ADLIB_REG_PORT 0xd012  //0x388
#define SD_ADLIB_DATA_PORT 0xd010 //0x389

#ifdef SD_OPL2_WITH_IEEE1284
static struct parport *sd_parport;
#endif

static volatile int timerDivisor = 1;

static SDL_mutex *soundSystemMutex = 0;
static SDL_Thread *t0Thread = 0;

void SD_OPL2LPT_SetTimer0(int16_t int_8_divisor)
{
	//Configure the PIT frequency.
	timerDivisor = int_8_divisor;
}

void SDL_t0Service(void);

static uint64_t SD_LastPITTickTime;

static volatile bool soundSystemUp;

int SD_OPL2LPT_t0InterruptThread(void *param)
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

void SD_OPL2LPT_alOut(uint8_t reg, uint8_t val)
{
#ifdef SD_OPL2_WITH_IEEE1284
	if (sd_parport)
	{
		ieee1284_write_data(sd_parport, reg);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN | C1284_NSTROBE) ^ C1284_INVERTED);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE) ^ C1284_INVERTED);
		usleep(4);

		ieee1284_write_data(sd_parport, val);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN | C1284_NINIT) ^ C1284_INVERTED);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN) ^ C1284_INVERTED);
		ieee1284_write_control(sd_parport, (C1284_NSELECTIN | C1284_NINIT) ^ C1284_INVERTED);
		usleep(33);
	}
#elif SD_OPL2_WITH_IOPERM
	outb(reg, SD_ADLIB_DATA_PORT);
	outb((C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE), SD_ADLIB_REG_PORT);
	outb((C1284_NSELECTIN | C1284_NSTROBE), SD_ADLIB_REG_PORT);
	outb((C1284_NSELECTIN | C1284_NINIT | C1284_NSTROBE), SD_ADLIB_REG_PORT);
	usleep(4);
	// TODO: Delay (TimerDelay10)
	outb(val, SD_ADLIB_DATA_PORT);
	outb((C1284_NSELECTIN | C1284_NINIT), SD_ADLIB_REG_PORT);
	outb((C1284_NSELECTIN), SD_ADLIB_REG_PORT);
	outb((C1284_NSELECTIN | C1284_NINIT), SD_ADLIB_REG_PORT);
	usleep(33);
#endif
}

void SD_OPL2LPT_PCSpkOn(bool on, int freq)
{
	// Setup operator
	SD_OPL2LPT_alOut(SD_ADLIB_REG_CHAR, 0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_VOLUME, 0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_ATTACK, 0xFF);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_SUSPEND, 0x0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_WAVE, 0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_CHAR + 3, 0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_VOLUME + 3, 0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_ATTACK + 3, 0xFF);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_SUSPEND + 3, 0x0);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_WAVE + 3, 0);

	// Setup channel
	SD_OPL2LPT_alOut(SD_ADLIB_REG_CONNECTION, 0xF0);

	uint64_t fNum = freq ? (((PC_PIT_RATE / (freq)) << 16) / 49716) : 0;

	uint8_t hibyte = ((fNum >> 8) & 3) | 12;
	if (on)
		hibyte |= 0x20;
	else
		hibyte = 0;
	SD_OPL2LPT_alOut(SD_ADLIB_REG_NOTE_LO, fNum & 0xFF);
	SD_OPL2LPT_alOut(SD_ADLIB_REG_NOTE_HI, hibyte);
}

void SD_OPL2LPT_Startup(void)
{
#ifdef SD_OPL2_WITH_IEEE1284
	const char *portName = CFG_GetConfigString("sd_opl2lpt_port", "");

	struct parport_list allPorts = {};

	if (ieee1284_find_ports(&allPorts, 0) != E1284_OK)
		Quit("Can't find parallel port!");

	for (int i = 0; i < allPorts.portc; ++i)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Found parallel port \"%s\"\n", allPorts.portv[i]->name);

		// If this isn't the port we're looking for, look at the next one.
		if (portName[0] && strcmp(portName, allPorts.portv[i]->name))
			continue;
		int caps = CAP1284_RAW;
		sd_parport = allPorts.portv[i];
		if (ieee1284_open(sd_parport, F1284_EXCL, &caps) != E1284_OK)
		{
			Quit("Couldn't open parallel port!");
		}

		if (ieee1284_claim(sd_parport) != E1284_OK)
		{
			CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Couldn't claim parallel port \"%s\"\n", sd_parport->name);
			perror("ieee1284_claim");
			Quit("Couldn't claim parallel port!");
		}
	}
	ieee1284_free_ports(&allPorts);
#elif SD_OPL2_WITH_IOPERM
	ioperm(SD_ADLIB_REG_PORT, 1, 1);
	ioperm(SD_ADLIB_DATA_PORT, 1, 1);
	ioperm(0x42, 2, 1);
	ioperm(0x61, 1, 1);
#endif
	soundSystemMutex = SDL_CreateMutex();
	soundSystemUp = true;

	uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
	SD_LastPITTickTime = currPitTicks;
	t0Thread = SDL_CreateThread(SD_OPL2LPT_t0InterruptThread, "ID_SD: t0 interrupt thread.", NULL);
}

void SD_OPL2LPT_Shutdown(void)
{
	ieee1284_close(sd_parport);
}

bool SD_OPL2LPT_mutexLocked = false;

void SD_OPL2LPT_Lock()
{
	if (SD_OPL2LPT_mutexLocked)
		Quit("Attempted sound system re-entry (locking an already locked mutex)");
	if (SDL_LockMutex(soundSystemMutex))
		Quit("Couldn't lock sound system mutex.");
	SD_OPL2LPT_mutexLocked = true;
}

void SD_OPL2LPT_Unlock()
{
	if (!SD_OPL2LPT_mutexLocked)
		Quit("Tried to unlock the already unlocked sound system mutex.");
	SDL_UnlockMutex(soundSystemMutex);
	SD_OPL2LPT_mutexLocked = false;
}

SD_Backend sd_opl2lpt_backend = {
	.startup = SD_OPL2LPT_Startup,
	.shutdown = SD_OPL2LPT_Shutdown,
	.lock = SD_OPL2LPT_Lock,
	.unlock = SD_OPL2LPT_Unlock,
	.alOut = SD_OPL2LPT_alOut,
	.pcSpkOn = SD_OPL2LPT_PCSpkOn,
	.setTimer0 = SD_OPL2LPT_SetTimer0};

SD_Backend *SD_Impl_GetBackend_OPL2LPT()
{
	return &sd_opl2lpt_backend;
}
