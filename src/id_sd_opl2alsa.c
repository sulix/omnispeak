/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2020 Omnispeak Authors

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
#include <sys/io.h>

#include <alsa/asoundlib.h>
#include <sound/asound_fm.h>
#include <sys/ioctl.h>
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

#define SD_ADLIB_REG_PORT 0xc050  //0x388
#define SD_ADLIB_DATA_PORT 0xc051 //0x389

static volatile int timerDivisor = 1;

static SDL_mutex *soundSystemMutex = 0;
static SDL_Thread *t0Thread = 0;

static SDL_cond *SD_ALSAOPL2_TimerConditionVar;
static bool SD_ALSAOPL2_WaitTicksSpin = false;

void SD_ALSAOPL2_SetTimer0(int16_t int_8_divisor)
{
	//Configure the PIT frequency.
	timerDivisor = int_8_divisor;
}

void SDL_t0Service(void);

static uint64_t SD_LastPITTickTime;

static volatile bool soundSystemUp;

int SD_ALSAOPL2_t0InterruptThread(void *param)
{
	while (soundSystemUp)
	{
		uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
		uint32_t ticks = (currPitTicks - SD_LastPITTickTime) / timerDivisor;

		if (ticks)
		{
			//if (SDL_LockMutex(soundSystemMutex))
			//	continue;
			SDL_t0Service();
			if (!SD_ALSAOPL2_WaitTicksSpin)
				SDL_CondBroadcast(SD_ALSAOPL2_TimerConditionVar);
			//SDL_UnlockMutex(soundSystemMutex);
			SD_LastPITTickTime = currPitTicks;
		}
		else
		{
			uint64_t ticksRemaining = SD_LastPITTickTime + timerDivisor - currPitTicks;
			uint64_t platformTicks = ticksRemaining * 1000 / PC_PIT_RATE;

			SDL_Delay(platformTicks);
		}
	}
	return 0;
}

snd_hwdep_t *sd_alsa_oplHwDep;
struct snd_dm_fm_voice sd_alsa_oplOperators[32];
struct snd_dm_fm_note sd_alsa_oplChannels[16];
struct snd_dm_fm_params sd_alsa_oplParams;

const int regToOper[0x20] =
	{0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1,
		12, 13, 14, 15, 16, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1};

void SD_ALSAOPL2_alOut(uint8_t reg, uint8_t val)
{
	bool paramsDirty = false;
	if (reg == 0x08)
	{
		sd_alsa_oplParams.kbd_split = (val >> 6) & 1;
		paramsDirty = true;
	}
	else if (reg == 0xBD)
	{
		// Perussion / Params
		sd_alsa_oplParams.hihat = (val)&1;
		sd_alsa_oplParams.cymbal = (val >> 1) & 1;
		sd_alsa_oplParams.tomtom = (val >> 2) & 1;
		sd_alsa_oplParams.snare = (val >> 3) & 1;
		sd_alsa_oplParams.bass = (val >> 4) & 1;
		sd_alsa_oplParams.rhythm = (val >> 5) & 1;
		sd_alsa_oplParams.vib_depth = (val >> 6) & 1;
		sd_alsa_oplParams.am_depth = (val >> 7) & 1;
		paramsDirty = true;
	}
	else if ((reg & 0xf0) == 0xa0)
	{
		// Channel Freq (low 8 bits)
		sd_alsa_oplChannels[reg & 0xf].fnum = (sd_alsa_oplChannels[reg & 0xf].fnum & 0x300) | (val & 0xff);
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_PLAY_NOTE, (void *)&sd_alsa_oplChannels[reg & 0xf]);
	}
	else if ((reg & 0xf0) == 0xb0)
	{
		// Channel freq (high 3 bits)
		sd_alsa_oplChannels[reg & 0xf].fnum = (sd_alsa_oplChannels[reg & 0xf].fnum & 0xff) | ((val << 8) & 0x300);
		sd_alsa_oplChannels[reg & 0xf].octave = (val >> 2) & 7;
		sd_alsa_oplChannels[reg & 0xf].key_on = (val >> 5) & 1;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_PLAY_NOTE, (void *)&sd_alsa_oplChannels[reg & 0xf]);
	}
	else if ((reg & 0xf0) == 0xc0)
	{
		int operTbl[] = {0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20, 24, 25, 26, 30, 31, 32};
		int oper = operTbl[reg & 0xf];
		if (oper >= 18)
			return;
		sd_alsa_oplOperators[oper].connection = (val)&1;
		sd_alsa_oplOperators[oper + 3].connection = (val)&1;
		sd_alsa_oplOperators[oper].feedback = (val >> 1) & 7;
		sd_alsa_oplOperators[oper + 3].feedback = (val >> 1) & 7;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x20)
	{
		int oper = regToOper[reg & 0x1f];
		if (oper == -1)
			return;
		sd_alsa_oplOperators[oper].harmonic = val & 0xf;
		sd_alsa_oplOperators[oper].kbd_scale = (val >> 4) & 1;
		sd_alsa_oplOperators[oper].do_sustain = (val >> 5) & 1;
		sd_alsa_oplOperators[oper].vibrato = (val >> 6) & 1;
		sd_alsa_oplOperators[oper].am = (val >> 7) & 1;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x40)
	{
		int oper = regToOper[reg & 0x1f];
		if (oper == -1)
			return;
		sd_alsa_oplOperators[oper].volume = ~val & 0x3f;
		sd_alsa_oplOperators[oper].scale_level = (val >> 6) & 3;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x60)
	{
		int oper = regToOper[reg & 0x1f];
		if (oper == -1)
			return;
		sd_alsa_oplOperators[oper].decay = val & 0xf;
		sd_alsa_oplOperators[oper].attack = (val >> 4) & 0xf;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0x80)
	{
		int oper = regToOper[reg & 0x1f];
		if (oper == -1)
			return;
		sd_alsa_oplOperators[oper].release = val & 0xf;
		sd_alsa_oplOperators[oper].sustain = (val >> 4) & 0xf;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else if ((reg & 0xe0) == 0xe0)
	{
		int oper = regToOper[reg & 0x1f];
		if (oper == -1)
			return;
		sd_alsa_oplOperators[oper].waveform = val & 0x3;
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_VOICE, (void *)&sd_alsa_oplOperators[oper]);
	}
	else
	{
		//CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Unknown OPL operator %x = %x\n", reg, val);
	}

	if (paramsDirty)
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_PARAMS, (void *)&sd_alsa_oplParams);
}

void SD_ALSAOPL2_PCSpkOn(bool on, int freq)
{
	// We fake a PC Speaker using OPL2. It's pretty dodgy.

	// Setup operator
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_CHAR, 0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_VOLUME, 0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_ATTACK, 0xFF);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_SUSPEND, 0x0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_WAVE, 0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_CHAR + 3, 0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_VOLUME + 3, 0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_ATTACK + 3, 0xFF);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_SUSPEND + 3, 0x0);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_WAVE + 3, 0);

	// Setup channel
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_CONNECTION, 0xF0);

	uint64_t fNum = freq ? (((PC_PIT_RATE / (freq)) << 16) / 49716) : 0;

	uint8_t hibyte = ((fNum >> 8) & 3) | 12;
	if (on)
		hibyte |= 0x20;
	else
		hibyte = 0;
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_NOTE_LO, fNum & 0xFF);
	SD_ALSAOPL2_alOut(SD_ADLIB_REG_NOTE_HI, hibyte);
}

static void setupStructs()
{
	memset(&sd_alsa_oplParams, 0, sizeof(sd_alsa_oplParams));
	memset(sd_alsa_oplOperators, 0, sizeof(sd_alsa_oplOperators));
	memset(sd_alsa_oplChannels, 0, sizeof(sd_alsa_oplChannels));

	for (int i = 0; i < 18; ++i)
	{
		sd_alsa_oplOperators[i].op = (i / 3) % 2;
		sd_alsa_oplOperators[i].voice = (i / 6) * 3 + i % 3;
		sd_alsa_oplOperators[i].left = 1;
		sd_alsa_oplOperators[i].right = 1;
	}

	for (int i = 0; i < 9; ++i)
	{
		sd_alsa_oplChannels[i].voice = i;
	}
}

void SD_ALSAOPL2_Startup(void)
{
	const char *alsaDev = CFG_GetConfigString("sd_alsa_device", "hw:0,0");
	for (int i = 0; i < us_argc; ++i)
	{
		if (!CK_Cross_strcasecmp(us_argv[i], "/ALSADEV"))
			alsaDev = us_argv[i + 1];
	}

	// Setup a condition variable to signal threads waiting for timer updates.
	SD_ALSAOPL2_WaitTicksSpin = CFG_GetConfigBool("sd_alsa_waitTicksSpin", false);
	if (!SD_ALSAOPL2_WaitTicksSpin)
		SD_ALSAOPL2_TimerConditionVar = SDL_CreateCond();

	if (snd_hwdep_open(&sd_alsa_oplHwDep, alsaDev, SND_HWDEP_OPEN_WRITE) < 0)
		Quit("Couldn't open OPL3 HWDEP");

	snd_hwdep_info_t *info;
	snd_hwdep_info_alloca(&info);

	if (snd_hwdep_info(sd_alsa_oplHwDep, info))
		Quit("Couldn't get hwdep info.");

	int interface = snd_hwdep_info_get_iface(info);
	CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "Got hwdep device interface %d\n", interface);

	if (interface == SND_HWDEP_IFACE_OPL2)
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_MODE, (void *)SNDRV_DM_FM_MODE_OPL2);
	else if (interface == SND_HWDEP_IFACE_OPL3)
		snd_hwdep_ioctl(sd_alsa_oplHwDep, SNDRV_DM_FM_IOCTL_SET_MODE, (void *)SNDRV_DM_FM_MODE_OPL3);

	setupStructs();

	soundSystemMutex = SDL_CreateMutex();
	soundSystemUp = true;

	uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
	SD_LastPITTickTime = currPitTicks;
	t0Thread = SDL_CreateThread(SD_ALSAOPL2_t0InterruptThread, "ID_SD: t0 interrupt thread.", NULL);
}

void SD_ALSAOPL2_Shutdown(void)
{
	soundSystemUp = false;
	SDL_WaitThread(t0Thread, NULL);
	SDL_DestroyMutex(soundSystemMutex);
}

bool SD_ALSAOPL2_mutexLocked = false;

void SD_ALSAOPL2_Lock()
{
	if (SD_ALSAOPL2_mutexLocked)
		Quit("Attempted sound system re-entry (locking an already locked mutex)");
	if (SDL_LockMutex(soundSystemMutex))
		Quit("Couldn't lock sound system mutex.");
	SD_ALSAOPL2_mutexLocked = true;
}

void SD_ALSAOPL2_Unlock()
{
	if (!SD_ALSAOPL2_mutexLocked)
		Quit("Tried to unlock the already unlocked sound system mutex.");
	SDL_UnlockMutex(soundSystemMutex);
	SD_ALSAOPL2_mutexLocked = false;
}

void SD_ALSAOPL2_WaitTick()
{
	SDL_mutex *mtx = SDL_CreateMutex();
	SDL_LockMutex(mtx);
	SDL_CondWait(SD_ALSAOPL2_TimerConditionVar, mtx);
	SDL_UnlockMutex(mtx);
}

SD_Backend sd_opl2_backend = {
	.startup = SD_ALSAOPL2_Startup,
	.shutdown = SD_ALSAOPL2_Shutdown,
	.lock = SD_ALSAOPL2_Lock,
	.unlock = SD_ALSAOPL2_Unlock,
	.alOut = SD_ALSAOPL2_alOut,
	.pcSpkOn = SD_ALSAOPL2_PCSpkOn,
	.setTimer0 = SD_ALSAOPL2_SetTimer0,
	.waitTick = SD_ALSAOPL2_WaitTick};

SD_Backend *SD_Impl_GetBackend_ALSAOPL2()
{
	return &sd_opl2_backend;
}
