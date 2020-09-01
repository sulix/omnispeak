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

#ifndef ID_SD_H
#define ID_SD_H

#include <stdbool.h>
#include <stdint.h>

typedef int16_t soundnames;
typedef int16_t musicnames;

typedef struct MusicGroup MusicGroup;

typedef enum
{
	sdm_Off,
	sdm_PC,
	sdm_AdLib,
} SDMode;

typedef enum
{
	smm_Off,
	smm_AdLib
} SMMode;

// Global variables accessed from other modules
// TODO: Rename these, update elsewhere, use accessor fns.
extern bool AdLibPresent;
extern SDMode SoundMode;
extern SMMode MusicMode;
extern bool quiet_sfx;

bool SD_SetSoundMode(SDMode mode);
bool SD_SetMusicMode(SMMode mode);
SDMode SD_GetSoundMode();
SMMode SD_GetMusicMode();
void SD_Startup(void);
void SD_Default(bool gotit, SDMode sd, SMMode sm);
void SD_Shutdown(void);
void SD_PlaySound(soundnames sound);
uint16_t SD_SoundPlaying(void);
void SD_StopSound(void);
void SD_WaitSoundDone(void);
void SD_MusicOn(void);
void SD_MusicOff(void);
void SD_StartMusic(MusicGroup *music);
void SD_FadeOutMusic(void);
bool SD_MusicPlaying(void); // Actually return false for all time

typedef struct SD_Backend
{
	void (*startup)();
	void (*shutdown)();
	void (*lock)();
	void (*unlock)();
	void (*alOut)(uint8_t reg, uint8_t val);
	void (*pcSpkOn)(bool on, int freq);
	void (*setTimer0)(int16_t int_8_divisor);
} SD_Backend;

SD_Backend *SD_Impl_GetBackend();
/* Timing related functions */

uint32_t SD_GetTimeCount(void);
void SD_SetTimeCount(uint32_t newval);
int32_t SD_GetLastTimeCount(void);
void SD_SetLastTimeCount(int32_t newval);
uint16_t SD_GetSpriteSync(void);
void SD_SetSpriteSync(uint16_t newval);

#endif
