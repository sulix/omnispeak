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

#include "id_ca.h"
#include "id_sd.h"
#include "id_us.h"
#include "ck_cross.h"

#ifdef SD_OPL2_WITH_ALSA
SD_Backend *SD_Impl_GetBackend_ALSAOPL2();
#endif

// Global variables accessed from other modules
bool sd_haveAdlib;
SD_SoundMode sd_soundMode;
ID_MusicMode sd_musicMode;
bool sd_quietAdlibSfx;

// Internal globals
static bool sd_started = false;
SD_Backend *sd_backend;
uint8_t **sd_sfxChunkArray;

// SFX number and Priority
volatile soundnames sd_currentSfxId;
volatile uint16_t sd_currentSfxPriority;

// Timing functions
#define PC_PIT_RATE 1193182
#define SD_SFX_PART_RATE 140
/* In the original exe, upon setting a rate of 140Hz or 560Hz for some
 * interrupt handler, the value 1192030 divided by the desired rate is
 * calculated, to be programmed for timer 0 as a consequence.
 * For THIS value, it is rather 1193182 that should be divided by it, in order
 * to obtain a better approximation of the actual rate.
 */
#define SD_SOUND_PART_RATE_BASE 1192030

// Kind of same as Wolf3D's TimeCount from ID_SD.C
static volatile uint32_t sd_timeCount = 0;
// Same as Wolf3D's lasttimecount from WL_DRAW.C?
static volatile int32_t sd_lastTimeCount = 0;
// Number of sprite "think" ticks.
static volatile uint16_t sd_SpriteSync = 0;
// PIT timer divisor, scaled (bt 8 if music is on, 2 otherwise).
// NOT initialized to 0 since this can lead to division by zero on startup.
static int16_t sd_scaledPITTimerDivisor = 1;
// A few variables used for timing measurements (PC_PIT_RATE units per second)
static uint64_t sd_lastPITTickTime;

uint32_t SD_GetTimeCount()
{
	return sd_timeCount;
}

void SD_SetTimeCount(uint32_t newval)
{
	SD_GetTimeCount(); // Refresh SD_LastPITTickTime to be in sync with SDL_GetTicks()
	sd_timeCount = newval;
}

int32_t SD_GetLastTimeCount()
{
	return sd_lastTimeCount;
}

void SD_SetLastTimeCount(int32_t newval)
{
	sd_lastTimeCount = newval;
}

uint16_t SD_GetSpriteSync()
{
	return sd_SpriteSync;
}

void SD_SetSpriteSync(uint16_t newval)
{
	sd_SpriteSync = newval;
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
static void SD_SetIntsPerSecond(int16_t tickrate)
{
	sd_scaledPITTimerDivisor = ((int32_t)SD_SOUND_PART_RATE_BASE / (int32_t)tickrate) & 0xFFFF;
	sd_backend->lock();
	sd_backend->setTimer0(sd_scaledPITTimerDivisor);
	sd_backend->unlock();
}

static void SD_SetTimerSpeed(void)
{
	int16_t scaleFactor = (sd_musicMode == smm_AdLib) ? 4 : 1;
	SD_SetIntsPerSecond(SD_SFX_PART_RATE * scaleFactor);
	sd_scaledPITTimerDivisor *= (2 * scaleFactor);
}

// SFX Common

// Register that the current sound effect is finished.
static void SD_SoundFinished()
{
	sd_currentSfxId = (soundnames)0;
	sd_currentSfxPriority = 0;
}

typedef struct
{
	uint32_t length;
	uint16_t priority;
} __attribute__((__packed__)) SD_SoundEffectCommon;

// ======= PC Speaker ========
typedef struct
{
	SD_SoundEffectCommon common;
	uint8_t data[1];
} __attribute__((__packed__)) SD_PCSound;

// Pointer to the current PC Speaker SFX data.
volatile uint8_t *sd_pc_currentSfxData;
// The last frequency the PIT speaker channel was set to.
volatile uint8_t sd_pc_lastFrequency;
// Number of bytes currently into the PC Speaker effect data.
volatile int sd_pc_currentSfxIndex;
// Length of PC Speaker effect data, in bytes.
volatile int sd_pc_currentSfxLength;

// Play a PC speaker sound (expects sd_backend locked)
static void SD_PC_PlaySound_Low(SD_PCSound *sound)
{
	sd_pc_lastFrequency = 255;
	sd_pc_currentSfxLength = CK_Cross_SwapLE32(sound->common.length);
	sd_pc_currentSfxData = (uint8_t *)sound->data;
	sd_pc_currentSfxIndex = 0;
}

// Play a PC speaker sound (expects sd_backend unlocked)
void SD_PC_PlaySound(SD_PCSound *sound)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SD_PC_PlaySound_Low(sound);
	sd_backend->unlock();
}

// Stop the current PC speaker sound effect (expects sd_backend locked)
static void SD_PC_StopSound_Low(void)
{
	sd_pc_currentSfxData = 0;
	// Turn the speaker off
	if (sd_backend)
		sd_backend->pcSpkOn(false, 0);
}

// Stop the current PC speaker sound effect (expects sd_backend unlocked)
void SD_PC_StopSound(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SD_PC_StopSound_Low();
	sd_backend->unlock();
}

// Callback for PC speaker sound effects
static void SD_PC_SoundService(void)
{
	uint8_t currentFrequency;
	uint16_t t;

	if (sd_pc_currentSfxData)
	{
		currentFrequency = sd_pc_currentSfxData[sd_pc_currentSfxIndex++];
		if (currentFrequency != sd_pc_lastFrequency)
		{
			sd_pc_lastFrequency = currentFrequency;
			if (currentFrequency)
			{
				// Turn the speaker & gate on
				sd_backend->pcSpkOn(true, currentFrequency * 60);
			}
			else // Time for some silence
			{
				// Turn the speaker & gate off
				sd_backend->pcSpkOn(false, 0);
			}
		}

		if (sd_pc_currentSfxIndex >= sd_pc_currentSfxLength)
		{
			// Turn off the speaker.
			sd_backend->pcSpkOn(false, 0);
			// Clear the sound data
			sd_pc_currentSfxData = NULL;
			sd_pc_currentSfxIndex = 0;
			// Set the currently playing sound to 0
			SD_SoundFinished();
		}
	}
}

// ======== Adlib ========

typedef struct
{
	uint8_t mChar, cChar;
	uint8_t mScale, cScale;
	uint8_t mAttack, cAttack;
	uint8_t mSus, cSus;
	uint8_t mWave, cWave;
	uint8_t nConn;

	// These are only for Muse - these bytes are really unused
	uint8_t voice;
	uint8_t mode;
	uint8_t unused[3];
} __attribute__((__packed__)) SD_AdlibInstrument;

typedef struct
{
	SD_SoundEffectCommon common;
	SD_AdlibInstrument inst;
	uint8_t block;
	uint8_t data[0];
} __attribute__((__packed__)) SD_AdlibSound;

// Pointer to the Adlib SFX data for currently playing effect
volatile uint8_t *sd_al_currentSfxData;
// Current sound effect's frequency block (raw value of NOTE_HI Adlib register)
volatile uint8_t sd_al_currentSfxHiFreqByte;
// Index into current Adlib sound effect in bytes.
volatile int sd_al_currentSfxIndex;
// Length of current Adlib sound effect in bytes.
volatile int sd_al_currentSfxLength;

// Set the current Adlib Instrument (channel/oper params) for channel 0 (sound effects)
static void SD_AL_SetAdlibSfxInstrument(SD_AdlibInstrument *inst)
{
	uint8_t c, m, cScale;

	m = 0;
	c = 3;
	sd_backend->alOut(m + SD_ADLIB_REG_CHAR, inst->mChar);
	sd_backend->alOut(m + SD_ADLIB_REG_VOLUME, inst->mScale);
	sd_backend->alOut(m + SD_ADLIB_REG_ATTACK, inst->mAttack);
	sd_backend->alOut(m + SD_ADLIB_REG_SUSPEND, inst->mSus);
	sd_backend->alOut(m + SD_ADLIB_REG_WAVE, inst->mWave);

	sd_backend->alOut(c + SD_ADLIB_REG_CHAR, inst->cChar);

	cScale = inst->cScale;
	if (sd_quietAdlibSfx)
	{
		cScale = 0x3F - cScale;
		/* TODO: All shifts should be arithmetic/signed
		 * and apply on 16-bit values. Are they??
		 */
		cScale = (uint8_t)((((int16_t)0) | cScale) >> 1) + (uint8_t)((((int16_t)0) | cScale) >> 2);
		cScale = 0x3F - cScale;
	}
	sd_backend->alOut(c + SD_ADLIB_REG_VOLUME, cScale);

	sd_backend->alOut(c + SD_ADLIB_REG_ATTACK, inst->cAttack);
	sd_backend->alOut(c + SD_ADLIB_REG_SUSPEND, inst->cSus);
	sd_backend->alOut(c + SD_ADLIB_REG_WAVE, inst->cWave);
}

// Stop the current Adlib sound effect (expects sd_backend locked)
static void SD_AL_StopSound_Low()
{
	// Turn off channel 0.
	sd_backend->alOut(SD_ADLIB_REG_NOTE_HI, 0);
	// Clear the sound data
	sd_al_currentSfxData = NULL;
	sd_al_currentSfxIndex = 0;
}

// Stop the current Adlib sound effect (expects sd_backend unlocked)
static void SD_AL_StopSound()
{
	if (sd_backend)
		sd_backend->lock();
	SD_AL_StopSound_Low();
	if (sd_backend)
		sd_backend->unlock();
}

// Play an Adlib sound effect (expects sd_backend locked)
static void SD_AL_PlaySound_Low(SD_AdlibSound *sound)
{
	// Stop any previous sound.
	SD_AL_StopSound_Low();

	sd_al_currentSfxLength = CK_Cross_SwapLE32(sound->common.length);
	sd_al_currentSfxData = (uint8_t *)sound->data;
	sd_al_currentSfxIndex = 0;

	// Set the frequency block and "note on" bit
	sd_al_currentSfxHiFreqByte = ((sound->block & 7) << 2) | 0x20;

	if ((sound->inst.mSus | sound->inst.cSus) == 0)
		Quit("SDL_ALPlaySound() - Bad instrument");

	SD_AL_SetAdlibSfxInstrument(&sound->inst);
}

// Play an Adlib sound effect (expects sd_backend unlocked)
static void SD_AL_PlaySound(SD_AdlibSound *sound)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SD_AL_PlaySound_Low(sound);
	sd_backend->unlock();
}

// Callback for Adlib sound effects
static void SD_AL_SoundService()
{
	if (sd_al_currentSfxData)
	{
		uint8_t currentFrequency = sd_al_currentSfxData[sd_al_currentSfxIndex++];
		if (currentFrequency == 0)
		{
			sd_backend->alOut(SD_ADLIB_REG_NOTE_HI, 0);
		}
		else
		{
			sd_backend->alOut(SD_ADLIB_REG_NOTE_LO, currentFrequency);
			sd_backend->alOut(SD_ADLIB_REG_NOTE_HI, sd_al_currentSfxHiFreqByte);
		}

		if (sd_al_currentSfxIndex >= sd_al_currentSfxLength)
		{
			// Turn off channel 0.
			sd_backend->alOut(SD_ADLIB_REG_NOTE_HI, 0);
			// Clear the sound data
			sd_al_currentSfxData = NULL;
			sd_al_currentSfxIndex = 0;
			// Set the currently playing sound to 0
			SD_SoundFinished();
		}
	}
}

// ======= Adlib Music ========

// Pointer to current Adlib/IMF music data for current track
uint8_t *sd_music_data;
// Index into current tracks in bytes.
int sd_music_currentIndex;
// Current time in the track (in IMF ticks)
int sd_music_currentTime;
// The time (in ticks) the next event occurs at
int sd_music_nextEventTime;
// Total size of the current track (in bytes)
int sd_music_trackSize;
// Is music currently playing?
bool sd_musicStarted;

typedef struct SD_MusicTrack
{
	uint16_t length;
	uint8_t data[0];
} __attribute__((__packed__)) SD_MusicTrack;

// Reset the Adlib card
static void SD_AL_Reset()
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	for (int i = 0; i < 255; ++i)
		sd_backend->alOut(i, 0);
	// Enable wave control
	sd_backend->alOut(1, 0x20);
	sd_backend->unlock();
}

// Callback for Adlib/IMF music playback.
static void SD_AL_MusicService()
{
	if (!sd_musicStarted)
		return;

	while (sd_music_currentIndex < sd_music_trackSize)
	{
		if (sd_music_nextEventTime > sd_music_currentTime)
			break;

		uint8_t reg = sd_music_data[sd_music_currentIndex++];
		uint8_t val = sd_music_data[sd_music_currentIndex++];
		uint16_t timeDelta = sd_music_data[sd_music_currentIndex++];
		timeDelta |= (sd_music_data[sd_music_currentIndex++]) << 8;

		sd_backend->alOut(reg, val);
		sd_music_nextEventTime += timeDelta;
	}
	sd_music_currentTime++;
	if (sd_music_currentIndex >= sd_music_trackSize)
	{
		sd_music_currentIndex = 0;
		sd_music_currentTime = 0;
		sd_music_nextEventTime = 0;
	}
}

// Set the current SoundMode (Off/PC/AdLib)
bool SD_SetSoundMode(SD_SoundMode mode)
{
	bool soundAvailable = false;
	int16_t sfxChunkOffset;
	SD_StopSound();
	switch (mode)
	{
	case sdm_Off:
		soundAvailable = true;
		break;
	case sdm_PC:
		sfxChunkOffset = 0;
		soundAvailable = true;
		break;
	case sdm_AdLib:
		if (!sd_haveAdlib)
		{
			break;
		}
		sfxChunkOffset = ca_audInfoE.numSounds;
		soundAvailable = true;
		break;
	default:
		soundAvailable = false;
	}
	if (soundAvailable && (mode != sd_soundMode))
	{
		//SD_StopDevice();
		sd_soundMode = mode;
		sd_sfxChunkArray = CA_audio + sfxChunkOffset;
		//SD_StartDevice();
		if (sd_backend)
		{
			SD_AL_Reset();
		}
	}
	SD_SetTimerSpeed();
	return soundAvailable;
}

// Set the current MusicMode (Off/AdLib)
bool SD_SetMusicMode(ID_MusicMode mode)
{
	bool musicAvailable = false;
	SD_MusicOff();
	// Wait for music to end.
	// TODO: This should probably pump events, etc.
	while (SD_MusicPlaying())
		;

	switch (mode)
	{
	case smm_Off:
		musicAvailable = true;
		break;
	case smm_AdLib:
		if (sd_haveAdlib)
			musicAvailable = true;
		break;
	}
	if (musicAvailable)
		sd_musicMode = mode;
	SD_SetTimerSpeed();
	return musicAvailable;
}

// Get the current SoundMode
SD_SoundMode SD_GetSoundMode()
{
	return sd_soundMode;
}

// Get the current MusicMode
ID_MusicMode SD_GetMusicMode()
{
	return sd_musicMode;
}

// Enable or disable quiet Adlib sound effects
void SD_SetQuietSfx(bool value)
{
	sd_quietAdlibSfx = value;
}

// Query if quiet adlib sound effects are enabled.
bool SD_GetQuietSfx()
{
	return sd_quietAdlibSfx;
}

// Check to see if an Adlib card (i.e., Adlib backend) is present.
bool SD_IsAdlibPresent()
{
	return true;
}

// Start the Sound Manager
void SD_Startup()
{
	if (sd_started)
		return;

	sd_backend = SD_Impl_GetBackend();
	for (int i = 0; i < us_argc; ++i)
	{
#ifdef SD_OPL2_WITH_ALSA
		if (!CK_Cross_strcasecmp(us_argv[i], "/ALSAOPL2"))
			sd_backend = SD_Impl_GetBackend_ALSAOPL2();
#endif
	}

	if (sd_backend)
		sd_backend->startup();

	// TODO: Support /NOAL switch
	sd_haveAdlib = true;

	SD_SetTimeCount(0);

	SD_SetSoundMode(sdm_Off);
	SD_SetMusicMode(smm_Off);

	sd_started = true;
}

// Set default Sound/Music modes
void SD_Default(bool gotit, SD_SoundMode sd, ID_MusicMode sm)
{
	// If not specified, Adlib is default.
	if (!gotit)
	{
		sd = sdm_AdLib;
		sm = smm_AdLib;
	}

	// If Adlib not available, no music, PC Speaker
	if (!sd_haveAdlib)
	{
		sd = (sd == sdm_AdLib) ? sdm_PC : sd;
		sm = smm_Off;
	}

	// Update if required
	if (sd != sd_soundMode)
		SD_SetSoundMode(sd);
	if (sm != sd_musicMode)
		SD_SetMusicMode(sm);
}

// Shut down the sound manager
void SD_Shutdown(void)
{
	if (!sd_started)
		return;

	SD_MusicOff();
	SD_AL_Reset();
	//SDL_ShutDevice();
	//SDL_CleanDevice();

	// Some timer stuff not done here
	if (sd_backend)
		sd_backend->shutdown();

	sd_started = false;
}

// Play a sound effect with id 'sound' using the current device
void SD_PlaySound(soundnames sound)
{
	uint32_t length;
	uint16_t priority;
	if (sd_soundMode == sdm_Off)
		return;
	if (!sd_sfxChunkArray[sound])
		Quit("SD_PlaySound() - Uncached sound");
	length = CK_Cross_SwapLE32(((SD_SoundEffectCommon *)sd_sfxChunkArray[sound])->length);
	if (!length)
		Quit("SD_PlaySound() - Zero length sound");
	priority = CK_Cross_SwapLE16(((SD_SoundEffectCommon *)sd_sfxChunkArray[sound])->priority);
	if (priority < sd_currentSfxPriority)
	{
		return;
	}
	switch (sd_soundMode)
	{
	case sdm_Off:
		break;
	case sdm_PC:
		SD_PC_PlaySound((SD_PCSound *)sd_sfxChunkArray[sound]);
		break;
	case sdm_AdLib:
		SD_AL_PlaySound((SD_AdlibSound *)sd_sfxChunkArray[sound]);
		break;
	default:
		Quit("SD_PlaySound() - Unsuppored sound mode");
	}
	sd_currentSfxId = sound;
	sd_currentSfxPriority = priority;
}

// Return the id of the currently playing sound effect.
uint16_t SD_SoundPlaying(void)
{
	return sd_currentSfxId;
}

// Stop the currently playing sound effect
void SD_StopSound(void)
{
	switch (sd_soundMode)
	{
	case sdm_Off:
		break;
	case sdm_PC:
		SD_PC_StopSound();
		break;
	case sdm_AdLib:
		SD_AL_StopSound();
		break;
	default:
		Quit("SD_StopSound() - Unsupported sound mode");
	}
	SD_SoundFinished();
}

// Wait until the currently playing sound is finished.
void SD_WaitSoundDone(void)
{
	while (SD_SoundPlaying())
		;
}

// Start playing the current music track.
void SD_MusicOn(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	sd_musicStarted = true;
	sd_backend->unlock();
}

// Stop playing the current music track.
void SD_MusicOff(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	sd_musicStarted = false;
	if (sd_musicMode == smm_AdLib)
	{
		// Reset the OPL2
		sd_backend->alOut(SD_ADLIB_REG_EFFECTS, 0);
		// TODO: Check this?
		for (int i = 1; i < SD_ADLIB_NUM_CHANNELS; ++i)
			sd_backend->alOut(SD_ADLIB_REG_NOTE_HI + i, 0);
	}
	sd_backend->unlock();
}

// Start a specific music track.
void SD_StartMusic(SD_MusicTrack *music)
{
	SD_MusicOff();
	if (sd_musicMode == smm_AdLib)
	{
		if (!sd_backend)
			return;
		sd_backend->lock();
		sd_music_data = music->data;
		sd_music_trackSize = music->length;
		sd_music_currentIndex = 0;
		sd_music_nextEventTime = 0;
		sd_music_currentTime = 0;
		sd_backend->unlock();
		if (!in_Paused)
			SD_MusicOn();
	}
}

// Stop the current music track (identical to SD_MusicOff() -- doesn't actually fade)
void SD_FadeOutMusic(void)
{
	SD_MusicOff();
}

// Check if music is currently playing.
bool SD_MusicPlaying()
{
	// For whatever reason, this always resturs false.
	return sd_musicStarted;
}

// The timer service.
void SDL_t0Service(void)
{
	static uint16_t count = 1;
	if (sd_musicMode == smm_AdLib)
	{
		SD_AL_MusicService();
		++count;
		if (!(count & 7))
		{
			sd_timeCount++;
		}
		if (!(count & 3))
		{
			switch (sd_soundMode)
			{
			case sdm_Off:
				break;
			case sdm_PC:
				SD_PC_SoundService();
				break;
			case sdm_AdLib:
				SD_AL_SoundService();
				break;
			}
		}
	}
	else
	{
		++count;
		if (!(count & 1))
		{
			sd_timeCount++;
		}
		switch (sd_soundMode)
		{
		case sdm_Off:
			break;
		case sdm_PC:
			SD_PC_SoundService();
			break;
		case sdm_AdLib:
			SD_AL_SoundService();
			break;
		}
	}
}
