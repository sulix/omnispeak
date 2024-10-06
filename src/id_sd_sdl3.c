
//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "id_cfg.h"
#include "id_sd.h"
#include "id_us.h"
#include "ck_cross.h"

#include "opl/dbopl.h"
#include "opl/nuked_opl3.h"

#define SD_SDL_WITH_QUEUEAUDIO
// Are we using the SDL_PutAudioStreamData function?
static bool sd_sdl_queueAudio = true;
static int sd_sdl_queueAudioMinBufSize = 0;

static int sd_oplDelaySamples;
static bool sd_nukedBufferWrites;

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
static bool SD_PC_Speaker_On;
static int16_t SD_SDL_CurrentBeepSample;
static uint32_t SD_SDL_BeepHalfCycleCounter, SD_SDL_BeepHalfCycleCounterUpperBound;

// WARNING: These vars refer to the libSDL library!!!
SDL_AudioSpec SD_SDL_AudioSpec;
static bool SD_SDL_AudioSubsystem_Up;
static uint64_t SD_SDL_ScaledSamplesPerPartsTimesPITRate;
static uint32_t SD_SDL_ScaledSamplesPartNum = 0;
static uint32_t SD_SDL_SampleOffsetInSound, SD_SDL_SamplesInCurrentPart;

// Used for filling with samples from alOut (alOut_lLw), in addition
// to SD_SDL_CallBack (because waits between/after AdLib writes are expected)
static int16_t SD_ALOut_Samples[512 * 2];
static uint32_t SD_ALOut_SamplesStart = 0, SD_ALOut_SamplesEnd = 0;

// Used for the timer fallback.
static volatile int SD_SDL_timerDivisor = 1;
static volatile bool SD_SDL_useTimerFallback = false;
static uint64_t SD_SDL_nextTickAt = 0;
static SDL_Thread *SD_SDL_t0Thread = 0;
static SDL_Mutex *sd_sdl3_audioMutex;

static SDL_Condition *SD_SDL_TimerConditionVar;
static bool SD_SDL_WaitTicksSpin = false;

SDL_AudioStream *sd_sdl3_audioDevice;

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SD_SDL_SetTimer0(int16_t int_8_divisor)
{
	SD_SDL_ScaledSamplesPerPartsTimesPITRate = int_8_divisor * SD_SDL_AudioSpec.freq;
	// Since the following division may lead to truncation, SD_SDL_SamplesInCurrentPart
	// can change during playback by +-1 (otherwise music may be a bit faster than intended).
	SD_SDL_SamplesInCurrentPart = SD_SDL_ScaledSamplesPerPartsTimesPITRate / PC_PIT_RATE;

	// For the timer fallback.
	SD_SDL_timerDivisor = int_8_divisor;
}

/*******************************************************************************
OPL emulation, powered by dbopl from DOSBox and using bits of code from Wolf4SDL
*******************************************************************************/

typedef enum SD_OPLEmulator
{
	SD_OPL_EMULATOR_NONE,
	SD_OPL_EMULATOR_DBOPL,
	SD_OPL_EMULATOR_NUKED
} SD_OPLEmulator;

static SD_OPLEmulator sd_oplEmulator;

Chip oplChip;
opl3_chip nuked_oplChip;
int numChannels;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	if (sd_oplEmulator == SD_OPL_EMULATOR_DBOPL)
	{
		DBOPL_InitTables();
		Chip__Chip(&oplChip);
		Chip__Setup(&oplChip, rate);
	}
	else if (sd_oplEmulator == SD_OPL_EMULATOR_NUKED)
	{
		OPL3_Reset(&nuked_oplChip, rate);
	}
	return false;
}

static inline void YM3812Write(Chip *which, Bit32u reg, Bit8u val)
{
	if (sd_oplEmulator == SD_OPL_EMULATOR_DBOPL)
	{
		Chip__WriteReg(which, reg, val);
	}
	else if (sd_oplEmulator == SD_OPL_EMULATOR_NUKED)
	{
		if (sd_nukedBufferWrites)
			OPL3_WriteRegBuffered(&nuked_oplChip, reg, val);
		else
			OPL3_WriteReg(&nuked_oplChip, reg, val);
	}
}

static inline void YM3812UpdateOne(Chip *which, int16_t *stream, int length)
{
	if (sd_oplEmulator == SD_OPL_EMULATOR_DBOPL)
	{
		Bit32s buffer[512 * 2];
		int i;

		// length is at maximum samplesPerMusicTick = param_samplerate / 700
		// so 512 is sufficient for a sample rate of 358.4 kHz (default 44.1 kHz)
		if (length > 512)
			length = 512;

		if(which->opl3Active)
		{
			Chip__GenerateBlock3(which, length, buffer);

			// GenerateBlock3 generates a number of "length" 32-bit stereo samples
			// so we need to convert them to 16-bit mono samples
			if (numChannels == 1)
			{
				for(i = 0; i < length; i++)
				{
					int32_t sample = buffer[i*2] + buffer[i*2+1];
					if(sample > 16383) sample = 16383;
					else if(sample < -16384) sample = -16384;
					stream[i] = sample;
				}
			}
			else if (numChannels == 2)
			{
				for(i = 0; i < length * 2; i++)
				{
					int32_t sample = buffer[i];
					if(sample > 16383) sample = 16383;
					else if(sample < -16384) sample = -16384;
					stream[i] = sample;
				}
			}

		}
		else
		{
			Chip__GenerateBlock2(which, length, buffer);

			// GenerateBlock2 generates a number of "length" 32-bit mono samples
			// so we only need to convert them to 16-bit mono samples
			if (numChannels == 1)
			{
				for (i = 0; i < length; i++)
				{
					// Scale volume
					Bit32s sample = 2 * buffer[i];
					if (sample > 16383)
						sample = 16383;
					else if (sample < -16384)
						sample = -16384;
					stream[i] = (int16_t)sample;
				}
			}
			else if (numChannels == 2)
			{
				for (i = 0; i < length; i++)
				{
					// Scale volume
					Bit32s sample = 2 * buffer[i];
					if (sample > 16383)
						sample = 16383;
					else if (sample < -16384)
						sample = -16384;
					stream[i*2] = (int16_t)sample;
					stream[i*2+1] = (int16_t)sample;
				}
			}
		}
	}
	else if (sd_oplEmulator == SD_OPL_EMULATOR_NUKED)
	{
		// Nuked OPL3 always generates stereo, but Omnispeak often uses
		// mono streams.
		int16_t buffer[512 * 2];
		int offset = 0;

		while (length)
		{
			// The length should really never be 512 samples,
			// (see above for the DBOPL driver), but nevertheless
			// we handle the case. (Maybe someone's running at an
			// absurd sample rate.)
			int chunkLen = CK_Cross_min(length, 512);



			if (numChannels == 1)
			{
				OPL3_GenerateStream(&nuked_oplChip, buffer, chunkLen);
				for (int i = 0; i < chunkLen; i++)
				{
					// Add L + R to get a mono sample
					int32_t sample = buffer[i*2] + buffer[i*2+1];
					// We store it temporarily in a 32-bit value,
					// then clamp the result to 16-bit. This will
					// sound bad, but better than integer overflow.
					if (sample > 32767)
						sample = 32767;
					else if (sample < -32768)
						sample = -32768;
					stream[offset + i] = (int16_t)sample;
				}
			}
			else if (numChannels == 2)
			{
				OPL3_GenerateStream(&nuked_oplChip, stream + offset * 2, chunkLen);
			}
			length -= chunkLen;
			offset += chunkLen;
		}
	}
}

static int sd_sdl_bonusSamplesQueued = 0;

static inline void PCSpeakerUpdateOne(int16_t *stream, int length);
void SD_SDL_alOut(uint8_t reg, uint8_t val)
{
	// FIXME: The original code for alOut adds 6 reads of the register port
	// after writing to it (3.3 microseconds), and then 35 more reads of
	// the register port after writing to the data port (23 microseconds).
	//
	// It is apparently important for a portion of the fuse breakage sound
	// at the least. For now a hack is implied.
	YM3812Write(&oplChip, reg, val);
	// Hack comes with a "magic number" that appears to make it work better
	int length = sd_oplDelaySamples;
	if (length)
	{
		if (sd_sdl_queueAudio)
		{
			YM3812UpdateOne(&oplChip, SD_ALOut_Samples, length);
			if (SD_PC_Speaker_On)
				PCSpeakerUpdateOne(SD_ALOut_Samples, length);
			SDL_PutAudioStreamData(sd_sdl3_audioDevice, SD_ALOut_Samples, length * numChannels * 2);
			sd_sdl_bonusSamplesQueued += length;
		}
		else
		{
			if (length > sizeof(SD_ALOut_Samples) / (sizeof(int16_t) * numChannels) - SD_ALOut_SamplesEnd)
				length = sizeof(SD_ALOut_Samples) / (sizeof(int16_t) * numChannels) - SD_ALOut_SamplesEnd;
			YM3812UpdateOne(&oplChip, &SD_ALOut_Samples[SD_ALOut_SamplesEnd * numChannels], length);
			SD_ALOut_SamplesEnd += length;
		}
	}
}

/************************************************************************
PC Speaker emulation; The function mixes audio
into an EXISTING stream (of OPL sound data)
ASSUMPTION: The speaker is outputting sound (PCSpeakerUpdateOne == true).
************************************************************************/
static inline void PCSpeakerUpdateOne(int16_t *stream, int length)
{
	for (int loopVar = 0; loopVar < length; loopVar++)
	{
		*stream = (*stream + SD_SDL_CurrentBeepSample) / 2; // Mix
		stream++;
		if (numChannels == 2)
		{
			*stream = (*stream + SD_SDL_CurrentBeepSample) / 2; // Mix
			stream++;
		}
		SD_SDL_BeepHalfCycleCounter += 2 * PC_PIT_RATE;
		if (SD_SDL_BeepHalfCycleCounter >= SD_SDL_BeepHalfCycleCounterUpperBound)
		{
			SD_SDL_BeepHalfCycleCounter %= SD_SDL_BeepHalfCycleCounterUpperBound;
			// 32767 - too loud
			SD_SDL_CurrentBeepSample = 24575 - SD_SDL_CurrentBeepSample;
		}
	}
}

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_t0Service(void);

// WARNING: This function refers to the libSDL library!!!
/* BIG BIG FIXME: This is the VERY wrong place to call the OPL emulator, etc! */
void SD_SDL_CallBack(void *unused, Uint8 *stream, int len)
{
	int16_t *currSamplePtr = (int16_t *)stream;
	uint32_t currNumOfSamples;
	bool isPartCompleted;
#if SDL_VERSION_ATLEAST(1, 3, 0)
	memset(stream, 0, len);
#endif
	while (len)
	{
		if (!SD_SDL_SampleOffsetInSound && !SD_SDL_useTimerFallback)
		{
			SDL_t0Service();
			if (!SD_SDL_WaitTicksSpin)
				SDL_BroadcastCondition(SD_SDL_TimerConditionVar);
		}
		// Now generate sound
		isPartCompleted = (len >= 2 * numChannels * (SD_SDL_SamplesInCurrentPart - SD_SDL_SampleOffsetInSound));
		currNumOfSamples = isPartCompleted ? (SD_SDL_SamplesInCurrentPart - SD_SDL_SampleOffsetInSound) : (len / (2 * numChannels));

		// AdLib (including hack for alOut delays)
		if (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart <= currNumOfSamples)
		{
			// Copy sound generated by alOut
			if (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart > 0)
				memcpy(currSamplePtr, &SD_ALOut_Samples[SD_ALOut_SamplesStart * numChannels], numChannels * 2 * (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart));
			// Generate what's left
			if (currNumOfSamples - (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart) > 0)
				YM3812UpdateOne(&oplChip, currSamplePtr + (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart) * numChannels, currNumOfSamples - (SD_ALOut_SamplesEnd - SD_ALOut_SamplesStart));
			// Finally update these
			SD_ALOut_SamplesStart = SD_ALOut_SamplesEnd = 0;
		}
		else
		{
			// Already generated enough by alOut, to be copied
			memcpy(currSamplePtr, &SD_ALOut_Samples[SD_ALOut_SamplesStart], 2 * numChannels * currNumOfSamples);
			SD_ALOut_SamplesStart += currNumOfSamples;
		}
		// PC Speaker
		if (SD_PC_Speaker_On)
			PCSpeakerUpdateOne(currSamplePtr, currNumOfSamples);
		// We're done for now
		currSamplePtr += currNumOfSamples * numChannels;
		SD_SDL_SampleOffsetInSound += currNumOfSamples;
		len -= 2 * numChannels * currNumOfSamples;
		// End of part?
		if (SD_SDL_SampleOffsetInSound >= SD_SDL_SamplesInCurrentPart)
		{
			SD_SDL_SampleOffsetInSound = 0;
			if (++SD_SDL_ScaledSamplesPartNum == PC_PIT_RATE)
				SD_SDL_ScaledSamplesPartNum = 0;

			SD_SDL_SamplesInCurrentPart = (SD_SDL_ScaledSamplesPartNum + 1) * SD_SDL_ScaledSamplesPerPartsTimesPITRate / PC_PIT_RATE - SD_SDL_ScaledSamplesPartNum * SD_SDL_ScaledSamplesPerPartsTimesPITRate / PC_PIT_RATE;
		}
	}
}

int SD_SDL_t0InterruptThread(void *param)
{
	while (SD_SDL_useTimerFallback)
	{
		uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
		// Top up to min buffer size if below.
		if (SD_SDL_AudioSubsystem_Up && sd_sdl_queueAudio && sd_sdl_queueAudioMinBufSize)
		{
			int curQueueLen = SDL_GetAudioStreamQueued(sd_sdl3_audioDevice) / (numChannels * sizeof(int16_t));
			if (curQueueLen < sd_sdl_queueAudioMinBufSize)
			{
				int length = sd_sdl_queueAudioMinBufSize - curQueueLen;
				YM3812UpdateOne(&oplChip, SD_ALOut_Samples, length);
				if (SD_PC_Speaker_On)
					PCSpeakerUpdateOne(SD_ALOut_Samples, length);
				SDL_PutAudioStreamData(sd_sdl3_audioDevice, SD_ALOut_Samples, length * numChannels * sizeof(int16_t));
				sd_sdl_bonusSamplesQueued -= length;
				if (sd_sdl_bonusSamplesQueued < 0)
					sd_sdl_bonusSamplesQueued = 0;
			}
		}

		if (currPitTicks >= SD_SDL_nextTickAt)
		{
			SDL_LockMutex(sd_sdl3_audioMutex);
			SDL_t0Service();
			SDL_UnlockMutex(sd_sdl3_audioMutex);
			if (!SD_SDL_WaitTicksSpin)
				SDL_BroadcastCondition(SD_SDL_TimerConditionVar);
			SD_SDL_nextTickAt += SD_SDL_timerDivisor;
			if (SD_SDL_AudioSubsystem_Up && sd_sdl_queueAudio)
			{
				int length = SD_SDL_SamplesInCurrentPart - sd_sdl_bonusSamplesQueued;
				sd_sdl_bonusSamplesQueued -= SD_SDL_SamplesInCurrentPart;
				if (sd_sdl_bonusSamplesQueued < 0) sd_sdl_bonusSamplesQueued = 0;
				if (length < 1) continue;
				YM3812UpdateOne(&oplChip, SD_ALOut_Samples, length);
				if (SD_PC_Speaker_On)
					PCSpeakerUpdateOne(SD_ALOut_Samples, length);
				SDL_PutAudioStreamData(sd_sdl3_audioDevice, SD_ALOut_Samples, length * numChannels * sizeof(int16_t));
				if (++SD_SDL_ScaledSamplesPartNum == PC_PIT_RATE)
					SD_SDL_ScaledSamplesPartNum = 0;

				SD_SDL_SamplesInCurrentPart = (SD_SDL_ScaledSamplesPartNum + 1) * SD_SDL_ScaledSamplesPerPartsTimesPITRate / PC_PIT_RATE - SD_SDL_ScaledSamplesPartNum * SD_SDL_ScaledSamplesPerPartsTimesPITRate / PC_PIT_RATE;
			}
		}
		else
		{
			uint64_t ticksRemaining = SD_SDL_nextTickAt - currPitTicks;
			uint64_t platformTicks = ticksRemaining * 1000 / PC_PIT_RATE;

			if (platformTicks > 5000)
			{
				/* If we're more than 5 seconds behind, just reset. */
				SD_SDL_nextTickAt = currPitTicks;
			}
			else
				SDL_Delay(platformTicks);
		}
	}
	return 0;
}

void SD_SDL_PCSpkOn(bool on, int freq)
{
	SD_PC_Speaker_On = on;
	SD_SDL_CurrentBeepSample = 0;
	SD_SDL_BeepHalfCycleCounter = 0;
	SD_SDL_BeepHalfCycleCounterUpperBound = SD_SDL_AudioSpec.freq * freq;
}

void SD_SDL_Startup(void)
{
	const char *oplEmuString = CFG_GetConfigString("oplEmulator", "dbopl");
	if (!CK_Cross_strcasecmp(oplEmuString, "nukedopl3"))
		sd_oplEmulator = SD_OPL_EMULATOR_NUKED;
	else if (!CK_Cross_strcasecmp(oplEmuString, "dbopl"))
		sd_oplEmulator = SD_OPL_EMULATOR_DBOPL;
	else
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Unknown OPL emulator \"%s\". Valid values are \"dbopl\" and \"nukedopl3\".\n", oplEmuString);
		sd_oplEmulator = SD_OPL_EMULATOR_DBOPL;
	}

	SD_SDL_useTimerFallback = true;

	for (int i = 0; i < us_argc; ++i)
	{
		if (!CK_Cross_strcasecmp(us_argv[i], "/NUKEDOPL3"))
			sd_oplEmulator = SD_OPL_EMULATOR_NUKED;
	}

	// Check if we should use SDL_PutAudioStreamData
	sd_sdl_queueAudioMinBufSize = CFG_GetConfigInt("sd_sdl_queueAudioMinBufSize", 0);

	SD_SDL_AudioSpec.freq = CFG_GetConfigInt("sampleRate", 49716); // OPL rate
	SD_SDL_AudioSpec.format = SDL_AUDIO_S16;
	SD_SDL_AudioSpec.channels = CFG_GetConfigInt("audioChannels", 2);
	sd_oplDelaySamples = CFG_GetConfigInt("sd_oplDelaySamples", SD_SDL_AudioSpec.freq / 10000);
	sd_nukedBufferWrites = CFG_GetConfigBool("sd_nukedBufferWrites", sd_oplDelaySamples == 0);

	// Setup a condition variable to signal threads waiting for timer updates.
	SD_SDL_WaitTicksSpin = CFG_GetConfigBool("sd_sdl_waitTicksSpin", false);
	if (!SD_SDL_WaitTicksSpin)
		SD_SDL_TimerConditionVar = SDL_CreateCondition();

	// Allow us to override the audio driver.
	const char *defaultDriver = NULL;
	const char *sdlDriver = CFG_GetConfigString("sd_sdl_audioDriver", defaultDriver);
	SDL_setenv("SDL_AUDIODRIVER", sdlDriver, false);

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "SDL audio system initialization failed,\n%s\n", SDL_GetError());
		SD_SDL_AudioSubsystem_Up = false;
		SD_SDL_useTimerFallback = true;
	}
	else
	{
		sd_sdl3_audioDevice = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &SD_SDL_AudioSpec, NULL, NULL);
		if (!sd_sdl3_audioDevice)
		{
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Cannot open SDL audio device,\n%s\n", SDL_GetError());
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			SD_SDL_AudioSubsystem_Up = false;
		}
		else
		{
			SD_SDL_AudioSubsystem_Up = true;
			numChannels = SD_SDL_AudioSpec.channels;
			CK_Cross_LogMessage(CK_LOG_MSG_NORMAL, "SD_SDL3_Startup: Initialised device %s at %d Hz, %d Channels\n", SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(sd_sdl3_audioDevice)), SD_SDL_AudioSpec.freq, numChannels);
		}

		if (YM3812Init(1, 3579545, SD_SDL_AudioSpec.freq))
		{
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Preparation of emulated OPL chip has failed\n");
		}

		// Make sure we have all of these variables initialised to
		// sendible values before we start the audio callback, or we can
		// end up deadlocking, as the callback can run forever.
		SD_SDL_SetTimer0(8514);
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(sd_sdl3_audioDevice));
	}
	// Start the timer fallback if needed.
	if (SD_SDL_useTimerFallback)
	{
		uint64_t currPitTicks = (uint64_t)(SDL_GetPerformanceCounter()) * PC_PIT_RATE / SDL_GetPerformanceFrequency();
		SD_SDL_nextTickAt = currPitTicks;
		SD_SDL_t0Thread = SDL_CreateThread(SD_SDL_t0InterruptThread, "ID_SD: t0 interrupt thread.", NULL);
	}
}

void SD_SDL_Shutdown(void)
{
	if (SD_SDL_useTimerFallback)
	{
		SD_SDL_useTimerFallback = false;
		SDL_WaitThread(SD_SDL_t0Thread, NULL);
	}
	if (SD_SDL_AudioSubsystem_Up)
	{
		SDL_DestroyAudioStream(sd_sdl3_audioDevice);
		SDL_DestroyMutex(sd_sdl3_audioMutex);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		SD_SDL_AudioSubsystem_Up = false;
	}
}

bool SD_SDL_IsLocked = false;

void SD_SDL_Lock()
{
	if (SD_SDL_IsLocked)
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Tried to lock the audio system when it was already locked!\n");
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockMutex(sd_sdl3_audioMutex);
	SD_SDL_IsLocked = true;
}

void SD_SDL_Unlock()
{
	if (!SD_SDL_IsLocked)
		CK_Cross_LogMessage(CK_LOG_MSG_ERROR, "Tried to unlock the audio system when it was already unlocked!\n");
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockMutex(sd_sdl3_audioMutex);
	SD_SDL_IsLocked = false;
}

void SD_SDL_WaitTick()
{
	SDL_Mutex *mtx = SDL_CreateMutex();
	SDL_LockMutex(mtx);
	// Timeout of 2ms, as the PIT rate is ~1.1ms..
	SDL_WaitConditionTimeout(SD_SDL_TimerConditionVar, mtx, 2);
	SDL_UnlockMutex(mtx);
}

SD_Backend sd_sdl_backend = {
	.startup = SD_SDL_Startup,
	.shutdown = SD_SDL_Shutdown,
	.lock = SD_SDL_Lock,
	.unlock = SD_SDL_Unlock,
	.alOut = SD_SDL_alOut,
	.pcSpkOn = SD_SDL_PCSpkOn,
	.setTimer0 = SD_SDL_SetTimer0,
	.waitTick = SD_SDL_WaitTick};

SD_Backend *SD_Impl_GetBackend()
{
	return &sd_sdl_backend;
}
