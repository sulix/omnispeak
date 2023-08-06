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

typedef struct SD_MusicTrack SD_MusicTrack;

typedef enum
{
	sdm_Off,
	sdm_PC,
	sdm_AdLib,
} SD_SoundMode;

typedef enum
{
	smm_Off,
	smm_AdLib
} ID_MusicMode;

bool SD_SetSoundMode(SD_SoundMode mode);
bool SD_SetMusicMode(ID_MusicMode mode);
SD_SoundMode SD_GetSoundMode();
ID_MusicMode SD_GetMusicMode();
void SD_SetQuietSfx(bool value);
bool SD_GetQuietSfx();
bool SD_IsAdlibPresent();
void SD_Startup(void);
void SD_Default(bool gotit, SD_SoundMode sd, ID_MusicMode sm);
void SD_Shutdown(void);
void SD_PlaySound(soundnames sound);
uint16_t SD_SoundPlaying(void);
void SD_StopSound(void);
void SD_WaitSoundDone(void);
void SD_MusicOn(void);
void SD_MusicOff(void);
void SD_StartMusic(SD_MusicTrack *music);
void SD_FadeOutMusic(void);
bool SD_MusicPlaying(void); // Actually return false for all time

/* Adlib Register Definitions */
//	Register addresses
// Operator stuff
#define SD_ADLIB_REG_CHAR 0x20
#define SD_ADLIB_REG_VOLUME 0x40
#define SD_ADLIB_REG_ATTACK 0x60
#define SD_ADLIB_REG_SUSPEND 0x80
#define SD_ADLIB_REG_WAVE 0xe0
// Channel stuff
#define SD_ADLIB_REG_NOTE_LO 0xa0
#define SD_ADLIB_REG_NOTE_HI 0xb0
#define SD_ADLIB_REG_CONNECTION 0xc0
// Global stuff
#define SD_ADLIB_REG_EFFECTS 0xbd

#define SD_ADLIB_NUM_CHANNELS 10

// Soundcard Detection (bitmasks)
#define SD_CARD_PC_SPEAKER 1 // Has PC-speaker support
#define SD_CARD_OPL2 2 // Has Adlib support
#define SD_CARD_OPL3 4 // Has OPL3 (SB16/Adlib Gold) support
#define SD_CARD_BLASTER 8 // Has Digitised Sound (Sound Blaster) support.


typedef struct SD_Backend
{
	void (*startup)();
	void (*shutdown)();
	void (*lock)();
	void (*unlock)();
	void (*alOut)(uint8_t reg, uint8_t val);
	void (*pcSpkOn)(bool on, int freq);
	void (*setTimer0)(int16_t int_8_divisor);
	void (*waitTick)();
	unsigned int (*detect)();
	void (*setOPL3)(bool on);
} SD_Backend;

SD_Backend *SD_Impl_GetBackend();
/* Timing related functions */

uint32_t SD_GetTimeCount(void);
void SD_SetTimeCount(uint32_t newval);
int32_t SD_GetLastTimeCount(void);
void SD_SetLastTimeCount(int32_t newval);
uint16_t SD_GetSpriteSync(void);
void SD_SetSpriteSync(uint16_t newval);
void SD_WaitTick(void);

#endif
