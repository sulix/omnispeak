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

#include <stdbool.h>
#include <stdint.h>
#include "SDL.h"

#include "id_sd.h"
#include "id_ca.h"

#define PC_PIT_RATE 1193182
#define SD_SOUND_PART_RATE 140
/* In the original exe, upon setting a rate of 140Hz or 560Hz for some
 * interrupt handler, the value 1192030 divided by the desired rate is
 * calculated, to be programmed for timer 0 as a consequence.
 * For THIS value, it is rather 1193182 that should be divided by it, in order
 * to obtain a better approximation of the actual rate.
 */
#define SD_SOUND_PART_RATE_BASE 1192030

// Global variables (TODO more to add)
SDMode SoundMode;
uint16_t SoundPriority, DigiPriority;
// Internal variables (TODO more to add)
static bool SD_Started;
soundnames SoundNumber,DigiNumber;
uint8_t **SoundTable;
int16_t NeedsDigitized; // Unused

// PC Sound variables
uint8_t pcLastSample, *pcSound;
uint32_t pcLengthLeft;
uint16_t pcSoundLookup[255];
static bool SD_PC_Speaker_On; // Sort of replacement for assembly code

// WARNING: These vars refer to the libSDL library!!!
SDL_AudioSpec SD_SDL_AudioSpec;
static bool SD_SDL_AudioSubsystem_Up;
static uint32_t SD_SDL_SampleOffsetInSound, SD_SDL_SamplesPerPart, SD_SDL_BeepHalfCycleCounter;
static int16_t SD_SDL_CurrentSampleVal;


/* TODO: List of functions from vanilla Keen 5 to filter
 * (not all to be implemented):
 * SDL_SetTimer0
 * SDL_SetIntsPerSecond
 * int_handler_1 (Never used?)
 * SDL_PCPlaySound
 * SDL_PCStopSound
 * SDL_PCService (called from SDL_t0Service)
 * SDL_ShutPC
 * alOut
 * SDL_ALStopSound
 * SDL_AlSetFXInst
 * SDL_ALPlaySound
 * SDL_ALSoundService (called from SDL_t0Service)
 * SDL_ALService (called from SDL_t0Service)
 * SDL_ShutAL
 * SDL_CleanAL
 * SDL_StartAL
 * SDL_DetectAdlib
 * SDL_t0Service (called at a rate of about 140Hz (no music) or 560Hz (music on)
 * SDL_ShutDevice
 * SDL_CleanDevice
 * SDL_StartDevice
 * SDL_SetTimerSpeed (140Hz with no music, 560Hz with music)
 *
 * SD_SetSoundMode
 * SD_SetMusicMode
 * SD_Startup
 * SD_Default (Called from load_config, picks defaults based on available hardware and more)
 * SD_Shutdown
 * SD_SetUserHook (apparently called only by sub_25CF8 - which is unused?)
 * SD_PlaySound
 * SD_SoundPlaying
 * SD_StopSound
 * SD_WaitSoundDone
 * SD_MusicOn
 * SD_MusicOff
 * SD_StartMusic
 * SD_FadeOutMusic
 * SD_MusicPlaying
 *
 * For now we'd implement:
 * SDL_PCPlaySound, SDL_PCStopSound, SDL_ShutPC,
 * SDL_ShutDevice, SDL_CleanDevice, SDL_StartDevice,
 *
 * SD_SetSoundMode,
 * SD_Startup (partial), SD_Default (partial), SD_Shutdown (partial),
 * SD_PlaySound, SD_SoundPlaying, SD_StopSound, SD_WaitSoundDone
 */

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_PCStopSound_Low(void);

// WARNING: This function refers to the libSDL library!!!
void SD_SDL_CallBack(void *unused, Uint8 *stream, int len)
{
	int16_t *currSamplePtr = (int16_t *)stream;
#if SDL_VERSION_ATLEAST(1,3,0)
	memset(stream, 0, len);
#endif
	// FIXME FIXME FIXME: Make stuff more thread safe
	// (and of course, add full AdLib support)
	if ((SoundMode == sdm_Off) || !SD_PC_Speaker_On || !pcSound)
	{
		return;
	}
	while (len)
	{
		// Start new part (say beep)
		if (!SD_SDL_SampleOffsetInSound)
		{
			if (pcLastSample != *pcSound)
			{
				// Start a new wave
				SD_SDL_BeepHalfCycleCounter = 0;
				SD_SDL_CurrentSampleVal = 0;
			}
			
		}
		if (!(*pcSound))
		{
			if (len >= 2*(SD_SDL_SamplesPerPart-SD_SDL_SampleOffsetInSound))
			{
				currSamplePtr += SD_SDL_SamplesPerPart-SD_SDL_SampleOffsetInSound;
				len -= 2*(SD_SDL_SamplesPerPart-SD_SDL_SampleOffsetInSound);
				SD_SDL_SampleOffsetInSound = SD_SDL_SamplesPerPart;
			}
			else
			{
				currSamplePtr += len/2;
				len = 0;
				SD_SDL_SampleOffsetInSound += len/2;
			}
		}
		else
		{
			for (; (SD_SDL_SampleOffsetInSound  < SD_SDL_SamplesPerPart) && len; SD_SDL_SampleOffsetInSound++, len -= 2, currSamplePtr++)
			{
				*currSamplePtr = SD_SDL_CurrentSampleVal;
				SD_SDL_BeepHalfCycleCounter += 2 * PC_PIT_RATE;
				if (SD_SDL_BeepHalfCycleCounter >= SD_SDL_AudioSpec.freq * pcSoundLookup[*pcSound])
				{
					SD_SDL_BeepHalfCycleCounter %= SD_SDL_AudioSpec.freq * pcSoundLookup[*pcSound];
					// 32767 - too loud
					SD_SDL_CurrentSampleVal = 8191-SD_SDL_CurrentSampleVal;
				}
			}
		}
		// End of part (beep)
		if (SD_SDL_SampleOffsetInSound == SD_SDL_SamplesPerPart)
		{
			SD_SDL_SampleOffsetInSound = 0;
			pcLastSample = *pcSound;
			pcSound++;
			pcLengthLeft--;

			/* That's tricky: The original code may prepare the
			 * PIT for the very last beep, but it then mutes
			 * the speaker immediately!
			 */
			if (pcLengthLeft <= 1)
			{
				SDL_PCStopSound_Low();
				SoundPriority = 0;
				SoundNumber = 0;

				break;
			}
		}
	}
}

void SDL_PCPlaySound_Low(uint8_t *chunkPtr)
{
	pcLastSample = 255;
	pcLengthLeft = SDL_SwapLE32(*(uint32_t *)chunkPtr);
	pcSound = chunkPtr + 6;
	SD_PC_Speaker_On = true;
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_PCPlaySound(uint8_t *chunkPtr)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_PCPlaySound_Low(chunkPtr);
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

void SDL_PCStopSound_Low(void)
{
	SD_PC_Speaker_On = false;
	pcSound = 0;
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_PCStopSound(void)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_PCStopSound_Low();
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

void SDL_ShutPC_Low(void)
{
	SD_PC_Speaker_On = false;
}

/* NEVER call this from the callback!!! */
void SDL_ShutPC(void)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_ShutPC_Low();
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

void SDL_ShutDevice(void)
{
	switch (SoundMode)
	{
		case 1: SDL_ShutPC(); break;
		/*case 2: SDL_ShutAL(); break;*/ /* TODO IMPLEMENT */
	}
	SoundMode = sdm_Off;
}

void SDL_CleanDevice(void)
{
	/* TODO IMPLEMENT */
	/*
	if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
	{
		SDL_CleanAL();
	}
	*/
}

void SDL_StartDevice(void)
{
	/* TODO FINISH IMPLEMENTATION */
	/*
	if (SoundMode == sdm_AdLib)
	{
		SDL_StartAL();
	}
	*/
	SoundNumber = SoundPriority = 0;
}

void SD_PlaySound(soundnames sound)
{
	uint32_t length;
	uint16_t priority;
	if (SoundMode == sdm_Off)
		return;
	if (!SoundTable[sound])
		Quit("SD_PlaySound() - Uncached sound");
	length = SDL_SwapLE32(*(uint32_t *)(&SoundTable[sound]));
	if (!length)
		Quit("SD_PlaySound() - Zero length sound");
	priority = SDL_SwapLE16(*(uint16_t *)(SoundTable[sound] + 4));
	if (priority < SoundPriority)
	{
		return;
	}
	// TODO: Check for AdLib; For now only PC Speaker emulation is implemented
	SDL_PCPlaySound(SoundTable[sound]);

	SoundNumber = sound;
	SoundPriority = priority;
}

uint16_t SD_SoundPlaying(void)
{
	/* TODO: Implement AdLib support */
	if ((SoundMode == sdm_Off) || !pcSound)
		return 0;
	return SoundNumber;
}

void SD_StopSound(void)
{
	/* TODO: Implement AdLib support */
	if (SoundMode == sdm_PC)
		SDL_PCStopSound();
	SoundPriority = 0;
	SoundNumber = 0;
}

void
SD_WaitSoundDone(void)
{
	while (SD_SoundPlaying())
	{
		// The original code simply waits in a busy loop.
		// Bad idea for new code.
		// TODO: What about checking for input/graphics/other status?
		SDL_Delay(1);
	}
}

int16_t SD_SetSoundMode(SDMode mode)
{
	int16_t any_sound, offset;
	SD_StopSound();
	switch (mode)
	{
		case sdm_Off:
			NeedsDigitized = 0;
			any_sound = 1;
			break;
		case sdm_PC:
			offset = 0;
			NeedsDigitized = 0;
			any_sound = 1;
			break;
		/* TODO IMPLEMENT */
		/*
		case sdm_AdLib:
			if (!AdlibPresent)
			{
				break;
			}
			offset = NUMSOUNDS;
			NeedsDigitized = 0;
			any_sound = 1;
		*/
		default:
			any_sound = 0;
	}
	if (any_sound && (mode != SoundMode))
	{
		SDL_ShutDevice();
		SoundMode = mode;
		/* TODO: Is that useful? */
		SoundTable = CA_audio + offset;
		SDL_StartDevice();
	}
	/* TODO: Call SDL_SetTimerSpeed? (Think not.) */
	return any_sound;
}

void SD_Startup(void)
{
	/****** TODO: FINISH! ******/

	if (SD_Started)
	{
		return;
	}
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		printf("WARNING: SDL audio system initialization failed,\n%s\n", SDL_GetError());
		SD_SDL_AudioSubsystem_Up = false;
	}
	else
	{
		SD_SDL_AudioSpec.freq = 44100;
		SD_SDL_AudioSpec.format = AUDIO_S16;
		SD_SDL_AudioSpec.channels = 1;
		/* FIXME: Having 1024 samples buffer (or less) is better in
		 * terms of latency. Unfortunately, in its current state the
		 * mixer callback would take too long to execute, and crackling
		 * is a possibility.
		 */
		SD_SDL_AudioSpec.samples = 4096;
		SD_SDL_AudioSpec.callback = SD_SDL_CallBack;
		SD_SDL_AudioSpec.userdata = NULL;
		if (SDL_OpenAudio(&SD_SDL_AudioSpec, NULL))
		{
			printf("WARNING: Cannot open SDL audio device,\n%s\n", SDL_GetError());
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			SD_SDL_AudioSubsystem_Up = false;
		}
		else
		{
			// TODO: This depends on music on/off? (560Hz vs 140Hz for general interrupt handler)
			SD_SDL_SamplesPerPart = ((uint64_t)SD_SOUND_PART_RATE_BASE / SD_SOUND_PART_RATE) * SD_SDL_AudioSpec.freq / PC_PIT_RATE;
			SD_SDL_AudioSubsystem_Up = true;
		}
	}
	/*word_4E19A = 0; */ /* TODO: Unused variable? */
	/*alNoCheck = 0;*/

	/* TODO: Set ticks to 0 (including AL ticks, but not limited to) */

	SD_SetSoundMode(sdm_Off);
	//SD_SetMusicMode(smm_Off);

	// More AdLib stuff should be here

	// For PC speaker
	for (int16_t loopvar = 0; loopvar < 255; loopvar++)
	{
		pcSoundLookup[loopvar] = loopvar*60;
	}

	if (SD_SDL_AudioSubsystem_Up)
	{
		SDL_PauseAudio(0);
	}
	SD_Started = true;
}

void SD_Default(bool gotit, SDMode sd, SMMode sm)
{
	// TODO: FINISH! (Especially the music part)
	if (sd != SoundMode)
	{
		SD_SetSoundMode(sd);
	}
}

void SD_Shutdown(void)
{
	if (!SD_Started)
	{
		return;
	}
	if (SD_SDL_AudioSubsystem_Up)
	{
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		SD_SDL_AudioSubsystem_Up = false;
	}
	// TODO: Complete! (Call SD_MusicOff and, maybe, add a bit more)
	SD_StopSound();
	SDL_ShutDevice();
	SDL_CleanDevice();

	SD_Started = false;
}
