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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"

#include "id_sd.h"
#include "id_ca.h"
#include "id_us.h"
#include "ck_cross.h"

#define	SDL_SoundFinished() {SoundNumber = (soundnames)0; SoundPriority = 0;}

#define PC_PIT_RATE 1193182
#define SD_SFX_PART_RATE 140
/* In the original exe, upon setting a rate of 140Hz or 560Hz for some
 * interrupt handler, the value 1192030 divided by the desired rate is
 * calculated, to be programmed for timer 0 as a consequence.
 * For THIS value, it is rather 1193182 that should be divided by it, in order
 * to obtain a better approximation of the actual rate.
 */
#define SD_SOUND_PART_RATE_BASE 1192030

// Global variables (TODO more to add)
bool AdLibPresent, NeedsMusic;
SDMode SoundMode;
SMMode MusicMode;
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
// Sort of replacements for x86 behaviors and assembly code
static bool SD_PC_Speaker_On;
static int16_t SD_SDL_CurrentBeepSample;
static uint32_t SD_SDL_BeepHalfCycleCounter, SD_SDL_BeepHalfCycleCounterUpperBound;

// AdLib variables
bool alNoCheck;
uint8_t *alSound;
uint16_t alBlock;
uint32_t alLengthLeft;
uint32_t alTimeCount;
Instrument alZeroInst;

bool quiet_sfx;

// This table maps channel numbers to carrier and modulator op cells
static uint8_t carriers[9] =  { 3, 4, 5,11,12,13,19,20,21},
               modifiers[9] = { 0, 1, 2, 8, 9,10,16,17,18},
// This table maps percussive voice numbers to op cells
               pcarriers[5] = {19,0xff,0xff,0xff,0xff},
               pmodifiers[5] = {16,17,18,20,21};

// Sequencer variables
bool sqActive;
static uint16_t alFXReg; // Apparently always 0
static ActiveTrack *tracks[sqMaxTracks];
uint16_t *sqHack, *sqHackPtr, sqHackLen, sqHackSeqLen;
int32_t sqHackTime;

// WARNING: These vars refer to the libSDL library!!!
SDL_AudioSpec SD_SDL_AudioSpec;
static bool SD_SDL_AudioSubsystem_Up;
static uint32_t SD_SDL_SampleOffsetInSound, SD_SDL_SamplesPerPart/*, SD_SDL_MusSamplesPerPart*/;

// Used for filling with samples from alOut (alOut_lLw), in addition
// to SD_SDL_CallBack (because waits between/after AdLib writes are expected)
static int16_t SD_ALOut_Samples[512];
static uint32_t SD_ALOut_SamplesStart = 0, SD_ALOut_SamplesEnd = 0;

/******************************************************************
Timing subsection.
Originally SDL_t0Service is responsible for incrementing TimeCount.
/*****************************************************************/

// PRIVATE variables (in this source port)
static uint32_t TimeCount = 0; // Kind of same as Wolf3D's TimeCount from ID_SD.C
static int32_t lasttimecount = 0; // Same as Wolf3D's lasttimecount from WL_DRAW.C?
static uint16_t SpriteSync = 0;
// PIT timer divisor, scaled (bt 8 if music is on, 2 otherwise)
static int16_t ScaledTimerDivisor;
// A few variables used for timing measurements (PC_PIT_RATE units per second)
static uint64_t SD_LastPITTickTime;

uint32_t SD_GetTimeCount(void)
{
	// FIXME: What happens when SDL_GetTicks() reaches the upper bound?
	// May be challenging to fix... A proper solution should
	// only work with *differences between SDL_GetTicks values*.
	uint64_t currPitTicks = (uint64_t)(SDL_GetTicks()) * SD_SOUND_PART_RATE_BASE / 1000;
	uint32_t ticksToAdd = (currPitTicks - SD_LastPITTickTime) / ScaledTimerDivisor;
	SD_LastPITTickTime += ticksToAdd * ScaledTimerDivisor;
	TimeCount += ticksToAdd;
	return TimeCount;
}

void SD_SetTimeCount(uint32_t newval)
{
	SD_LastPITTickTime = (uint64_t)(SDL_GetTicks()) * SD_SOUND_PART_RATE_BASE / 1000;
	TimeCount = newval;
}

int32_t SD_GetLastTimeCount(void) { return lasttimecount; }
void SD_SetLastTimeCount(int32_t newval) { lasttimecount = newval; }
uint16_t SD_GetSpriteSync(void) { return SpriteSync; }
void SD_SetSpriteSync(uint16_t newval) { SpriteSync = newval; }

/*******************************************************************************
OPL emulation, powered by dbopl from DOSBox and using bits of code from Wolf4SDL
*******************************************************************************/

Chip oplChip;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	DBOPL_InitTables();
	Chip__Chip(&oplChip);
	Chip__Setup(&oplChip, rate);
	return false;
}

static inline void YM3812Write(Chip *which, Bit32u reg, Bit8u val)
{
	Chip__WriteReg(which, reg, val);
}

static inline void YM3812UpdateOne(Chip *which, int16_t *stream, int length)
{
	Bit32s buffer[512 * 2];
	int i;

	// length is at maximum samplesPerMusicTick = param_samplerate / 700
	// so 512 is sufficient for a sample rate of 358.4 kHz (default 44.1 kHz)
	if(length > 512)
		length = 512;
#if 0
	if(which->opl3Active)
	{
		Chip__GenerateBlock3(which, length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit stereo samples
		// so we need to convert them to 16-bit mono samples
		for(i = 0; i < length; i++)
		{
			// Scale volume and pick one channel
			Bit32s sample = 2*buffer[2*i];
			if(sample > 16383) sample = 16383;
			else if(sample < -16384) sample = -16384;
			stream[i] = sample;
		}
	}
	else
#endif
	{
		Chip__GenerateBlock2(which, length, buffer);

		// GenerateBlock2 generates a number of "length" 32-bit mono samples
		// so we only need to convert them to 16-bit mono samples
		for(i = 0; i < length; i++)
		{
			// Scale volume
			Bit32s sample = 2*buffer[i];
			if(sample > 16383) sample = 16383;
			else if(sample < -16384) sample = -16384;
			stream[i] = (int16_t) sample;
		}
	}
}

void alOut_Low(uint8_t reg, uint8_t val)
{
	// FIXME: The original code for alOut adds 6 reads of the register port
	// after writing to it (3.3 microseconds), and then 35 more reads of
	// the register port after writing to the data port (23 microseconds).
	//
	// It is apparently important for a portion of the fuse breakage sound
	// at the least. For now a hack is implied.
	YM3812Write(&oplChip, reg, val);
	// Hack comes with a "magic number" that appears to make it work better
	int length = SD_SDL_AudioSpec.freq / 10000;
	if (length > sizeof(SD_ALOut_Samples)/sizeof(int16_t) - SD_ALOut_SamplesEnd)
		length = sizeof(SD_ALOut_Samples)/sizeof(int16_t) - SD_ALOut_SamplesEnd;
	if (length)
	{
		YM3812UpdateOne(&oplChip, &SD_ALOut_Samples[SD_ALOut_SamplesEnd], length);
		SD_ALOut_SamplesEnd += length;
	}
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void alOut(uint8_t reg, uint8_t val)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	alOut_Low(reg, val);
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

/************************************************************************
PC Speaker emulation; The function mixes audio
into an EXISTING stream (of OPL sound data)
ASSUMPTION: The speaker is outputting sound (PCSpeakerUpdateOne == true).
************************************************************************/
static inline void PCSpeakerUpdateOne(int16_t *stream, int length)
{
	for (int loopVar = 0; loopVar < length; loopVar++, stream++)
	{
		*stream = (*stream + SD_SDL_CurrentBeepSample) / 2; // Mix
		SD_SDL_BeepHalfCycleCounter += 2 * PC_PIT_RATE;
		if (SD_SDL_BeepHalfCycleCounter >= SD_SDL_BeepHalfCycleCounterUpperBound)
		{
			SD_SDL_BeepHalfCycleCounter %= SD_SDL_BeepHalfCycleCounterUpperBound;
			// 32767 - too loud
			SD_SDL_CurrentBeepSample = 24575-SD_SDL_CurrentBeepSample;
		}
	}
}


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

void SDL_t0Service(void);

// WARNING: This function refers to the libSDL library!!!
/* BIG BIG FIXME: This is the VERY wrong place to call the OPL emulator, etc! */
void SD_SDL_CallBack(void *unused, Uint8 *stream, int len)
{
	int16_t *currSamplePtr = (int16_t *)stream;
	uint32_t currNumOfSamples;
	bool isPartCompleted;
#if SDL_VERSION_ATLEAST(1,3,0)
	memset(stream, 0, len);
#endif
	while (len)
	{
		if (!SD_SDL_SampleOffsetInSound)
		{
			SDL_t0Service();
		}
		// Now generate sound
		isPartCompleted = (len >= 2*(SD_SDL_SamplesPerPart-SD_SDL_SampleOffsetInSound));
		currNumOfSamples = isPartCompleted ? (SD_SDL_SamplesPerPart-SD_SDL_SampleOffsetInSound) : (len/2);

		// AdLib (including hack for alOut delays)
		if (SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart <= currNumOfSamples)
		{
			// Copy sound generated by alOut
			if (SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart > 0)
				memcpy(currSamplePtr, &SD_ALOut_Samples[SD_ALOut_SamplesStart], 2*(SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart));
			// Generate what's left
			if (currNumOfSamples-(SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart) > 0)
				YM3812UpdateOne(&oplChip, currSamplePtr+(SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart), currNumOfSamples-(SD_ALOut_SamplesEnd-SD_ALOut_SamplesStart));
			// Finally update these
			SD_ALOut_SamplesStart = SD_ALOut_SamplesEnd = 0;
		}
		else
		{
			// Already generated enough by alOut, to be copied
			memcpy(currSamplePtr, &SD_ALOut_Samples[SD_ALOut_SamplesStart], 2*currNumOfSamples);
			SD_ALOut_SamplesStart += currNumOfSamples;
		}
		// PC Speaker
		if (SD_PC_Speaker_On)
			PCSpeakerUpdateOne(currSamplePtr, currNumOfSamples);
		// We're done for now
		currSamplePtr += currNumOfSamples;
		SD_SDL_SampleOffsetInSound += currNumOfSamples;
		len -= 2*currNumOfSamples;
		// End of part?
		if (SD_SDL_SampleOffsetInSound >= SD_SDL_SamplesPerPart)
		{
			SD_SDL_SampleOffsetInSound = 0;
		}
	}
}

void SDL_SetTimer0(int16_t int_8_divisor)
{
	SD_SDL_SamplesPerPart = (int32_t)int_8_divisor * SD_SDL_AudioSpec.freq / PC_PIT_RATE;
	ScaledTimerDivisor = int_8_divisor;
}

void SDL_SetIntsPerSecond(int16_t tickrate)
{
	SDL_SetTimer0(((int32_t)SD_SOUND_PART_RATE_BASE / (int32_t)tickrate) & 0xFFFF);
}

void SDL_PCPlaySound_Low(PCSound *sound)
{
	pcLastSample = 255;
	pcLengthLeft = SDL_SwapLE32(sound->common.length);
	pcSound = (uint8_t *)sound->data;
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_PCPlaySound(PCSound *sound)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_PCPlaySound_Low(sound);
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

void SDL_PCStopSound_Low(void)
{
	pcSound = 0;
	// Turn the speaker off
	SD_PC_Speaker_On = false;
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

void SDL_PCService(void)
{
	// TODO: FINISH!
	uint8_t s;
	uint16_t t;

	if (pcSound)
	{
		s = *pcSound++;
		if (s != pcLastSample)
		{
#if 0
		asm	pushf
		asm	cli
#endif
			pcLastSample = s;
			if (s)					// We have a frequency!
			{
				t = pcSoundLookup[s];
				// Turn the speaker & gate on
				SD_PC_Speaker_On = true;
				SD_SDL_CurrentBeepSample = 0;
				SD_SDL_BeepHalfCycleCounter = 0;
				SD_SDL_BeepHalfCycleCounterUpperBound = SD_SDL_AudioSpec.freq * t;
#if 0
			asm	mov	bx,[t]

			asm	mov	al,0xb6			// Write to channel 2 (speaker) timer
			asm	out	43h,al
			asm	mov	al,bl
			asm	out	42h,al			// Low byte
			asm	mov	al,bh
			asm	out	42h,al			// High byte

			asm	in	al,0x61			// Turn the speaker & gate on
			asm	or	al,3
			asm	out	0x61,al
#endif
			}
			else					// Time for some silence
			{
				// Turn the speaker & gate on
				SD_PC_Speaker_On = false;
#if 0
			asm	in	al,0x61		  	// Turn the speaker & gate off
			asm	and	al,0xfc			// ~3
			asm	out	0x61,al
#endif
			}
#if 0
		asm	popf
#endif
		}

		if (!(--pcLengthLeft))
		{
			SDL_PCStopSound_Low();
			SDL_SoundFinished();
		}
	}
}

void SDL_ShutPC_Low(void)
{
	pcSound = 0;
	// Turn the speaker & gate off
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

void SDL_ALStopSound_Low(void)
{
	alSound = 0;
	alOut_Low(alFreqH + 0,0);
}

/* NEVER call this from the callback!!! */
void SDL_ALStopSound(void)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_ALStopSound_Low();
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

/* NEVER call this from the callback!!! */
static void SDL_AlSetFXInst(Instrument *inst)
{
	uint8_t	c,m,cScale;

	m = modifiers[0];
	c = carriers[0];
	alOut(m + alChar,inst->mChar);
	alOut(m + alScale,inst->mScale);
	alOut(m + alAttack,inst->mAttack);
	alOut(m + alSus,inst->mSus);
	alOut(m + alWave,inst->mWave);

	alOut(c + alChar,inst->cChar);

	cScale = inst->cScale;
	if (quiet_sfx)
	{
		cScale = 0x3F - cScale;
		/* TODO: All shifts should be arithmetic/signed
		 * and apply on 16-bit values. Are they??
		 */
		cScale = (uint8_t)((((int16_t)0) | cScale) >> 1) + (uint8_t)((((int16_t)0) | cScale) >> 2);
		cScale = 0x3F - cScale;
	}
	alOut(c + alScale,cScale);

	alOut(c + alAttack,inst->cAttack);
	alOut(c + alSus,inst->cSus);
	alOut(c + alWave,inst->cWave);
}

static void SDL_ALPlaySound_Low(AdLibSound *sound)
{
	Instrument *inst;

	// Do NOT call the non-low variant as we're already in such a variant!!!
	SDL_ALStopSound_Low();

	alLengthLeft = SDL_SwapLE32(sound->common.length);
	alSound = (uint8_t *)sound->data;

	alBlock = ((sound->block & 7) << 2) | 0x20;
	inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		Quit("SDL_ALPlaySound() - Bad instrument");
	}

	// SDL_AlSetFXInst(&alZeroInst);	// DEBUG
	SDL_AlSetFXInst(inst);
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
static void SDL_ALPlaySound(AdLibSound *sound)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_ALPlaySound_Low(sound);
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

void SDL_ALSoundService(void)
{
	uint8_t s;

	if (alSound)
	{
		s = *alSound++;
		if (!s)
			alOut_Low(alFreqH + 0,0);
		else
		{
			alOut_Low(alFreqL + 0,s);
			alOut_Low(alFreqH + 0,alBlock);
		}

		if (!(--alLengthLeft))
		{
			alSound = 0;
			alOut_Low(alFreqH + 0,0);
			SDL_SoundFinished();
		}
	}
}

void SDL_ALService(void)
{
	uint8_t a,v;
	uint16_t w;

	if (!sqActive)
		return;

	while (sqHackLen && (sqHackTime <= alTimeCount))
	{
		w = *sqHackPtr++;
		sqHackTime = alTimeCount + *sqHackPtr++;
		// TODO/FIXME: Endianness??
		a = *((uint8_t *)&w);
		v = *((uint8_t *)&w + 1);
		alOut_Low(a,v);
		sqHackLen -= 4;
	}
	alTimeCount++;
	if (!sqHackLen)
	{
		sqHackPtr = sqHack;
		sqHackLen = sqHackSeqLen;
		alTimeCount = sqHackTime = 0;
	}
}

static void SDL_ShutAL_Low(void)
{
	alOut_Low(alEffects, 0);
	alOut_Low(alFreqH + 0, 0);
	SDL_AlSetFXInst(&alZeroInst);
	alSound = 0;
}

/* NEVER call this from the callback!!! */
static void SDL_ShutAL(void)
{
	if (SD_SDL_AudioSubsystem_Up)
		SDL_LockAudio();
	SDL_ShutAL_Low();
	if (SD_SDL_AudioSubsystem_Up)
		SDL_UnlockAudio();
}

/* NEVER call this from the callback!!! */
static void SDL_CleanAL(void)
{
	int16_t i;
	alOut(alEffects, 0);
	for (i = 1; i < 0xf5; i++)
		alOut(i, 0);
}

/* NEVER call this from the callback!!! */
static void SDL_StartAL(void)
{
	alFXReg = 0;
	alOut(alEffects, alFXReg);
	SDL_AlSetFXInst(&alZeroInst);
}

/* NEVER call this from the callback!!! */
static bool SDL_DetectAdlib(bool assumepresence)
{
	uint8_t status1,status2;
	int16_t i;

	alOut(4,0x60);	// Reset T1 & T2
	alOut(4,0x80);	// Reset IRQ
//	status1 = readstat();
	alOut(2,0xff);	// Set timer 1
	alOut(4,0x21);	// Start timer 1

	/* Not relevant for us; We always assume the answer is "yes".
	 * Besides, the original code is speed-sensitive and tends
	 * to malfunction in too fast environments.
	 */

/*
	asm	mov	dx,0x388
	asm	mov	cx,100
	usecloop:
	asm	in	al,dx
	asm	loop usecloop
*/

//	status2 = readstat();
	alOut(4,0x60);
	alOut(4,0x80);

	// Assume the answer is always "Yes"...

//	if (assumepresence || (((status1 & 0xe0) == 0x00) && ((status2 & 0xe0) == 0xc0)))
	{
		for (i = 1;i <= 0xf5;i++) // Zero all the registers
			alOut(i,0);

		alOut(1,0x20); // Set WSE=1
		alOut(8,0);    // Set CSM=0 & SEL=0

		return true;
	}
//	else
//		return false;
}

void SDL_PCService(void);
void SDL_ALSoundService(void);
void SDL_ALService(void);

void SDL_t0Service(void)
{
	static uint16_t count = 1;
	if (MusicMode == smm_AdLib)
	{
		SDL_ALService();
		++count;
/*		if (!(count & 7))
		{
			LocalTime++;
			TimeCount++;
			if (SoundUserHook)
				SoundUserHook();
		}*/
		if (!(count & 3))
		{
			switch (SoundMode)
			{
			case sdm_PC:
				SDL_PCService();
				break;
			case sdm_AdLib:
				SDL_ALSoundService();
				break;
			}
		}
	}
	else
	{
		++count;
/*		if (!(count & 1))
		{
			LocalTime++;
			TimeCount++;
			if (SoundUserHook)
				SoundUserHook();
		}*/
		switch (SoundMode)
		{
		case sdm_PC:
			SDL_PCService();
			break;
		case sdm_AdLib:
			SDL_ALSoundService();
			break;
		}
	}
}

void SDL_ShutDevice(void)
{
	switch (SoundMode)
	{
		case sdm_PC: SDL_ShutPC(); break;
		case sdm_AdLib: SDL_ShutAL(); break;
	}
	SoundMode = sdm_Off;
}

void SDL_CleanDevice(void)
{
	if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
	{
		SDL_CleanAL();
	}
}

void SDL_StartDevice(void)
{
	if (SoundMode == sdm_AdLib)
	{
		SDL_StartAL();
	}
	SoundNumber = (soundnames)0; SoundPriority = 0;
}

void SDL_SetTimerSpeed(void)
{
	int16_t scaleFactor = (MusicMode == smm_AdLib) ? 4 : 1;
	SDL_SetIntsPerSecond(SD_SFX_PART_RATE * scaleFactor);
	ScaledTimerDivisor *= (2*scaleFactor);
}

void SD_StopSound(void);

bool SD_SetSoundMode(SDMode mode)
{
	bool any_sound; // FIXME: Should be set to false here?
	int16_t offset; // FIXME: Should be set to 0 here?
	SD_StopSound();
	switch (mode)
	{
		case sdm_Off:
			NeedsDigitized = 0;
			any_sound = true;
			break;
		case sdm_PC:
			offset = 0;
			NeedsDigitized = 0;
			any_sound = true;
			break;
		case sdm_AdLib:
			if (!AdLibPresent)
			{
				break;
			}
      offset = ca_audInfoE.numSounds;
			NeedsDigitized = 0;
			any_sound = true;
			break;
		default:
			any_sound = false;
	}
	if (any_sound && (mode != SoundMode))
	{
		SDL_ShutDevice();
		SoundMode = mode;
		/* TODO: Is that useful? */
		SoundTable = CA_audio + offset;
		SDL_StartDevice();
	}
	SDL_SetTimerSpeed();
	return any_sound;
}

bool SD_MusicPlaying(void);
void SD_FadeOutMusic(void);

bool SD_SetMusicMode(SMMode mode)
{
	bool result; // FIXME: Should be set to false here?
	SD_FadeOutMusic();
	while (SD_MusicPlaying())
	{
		// The original code simply waits in a busy loop.
		// Bad idea for new code.
		// TODO: What about checking for input/graphics/other status?
		SDL_Delay(1);
	}
	switch (mode)
	{
	case smm_Off:
		NeedsMusic = 0;
		result = true;
		break;
	case smm_AdLib:
		if (AdLibPresent)
		{
			NeedsMusic = 1;
			result = true;
		}
		break;
	default:
		result = false;
	}
	if (result)
		MusicMode = mode;
	SDL_SetTimerSpeed();
	return result;
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
		CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "SDL audio system initialization failed,\n%s\n", SDL_GetError());
		SD_SDL_AudioSubsystem_Up = false;
	}
	else
	{
		SD_SDL_AudioSpec.freq = 49716; // OPL rate
		SD_SDL_AudioSpec.format = AUDIO_S16;
		SD_SDL_AudioSpec.channels = 1;
		// Under wine, small buffer sizes cause a lot of crackling, so we double the
		// buffer size. This will result in a tiny amount (~10ms) of extra lag on windows,
		// but it's a price I'm prepared to pay to not have my ears explode.
#ifdef _WIN32
		SD_SDL_AudioSpec.samples = 1024;
#else
		SD_SDL_AudioSpec.samples = 512;
#endif
		SD_SDL_AudioSpec.callback = SD_SDL_CallBack;
		SD_SDL_AudioSpec.userdata = NULL;
		if (SDL_OpenAudio(&SD_SDL_AudioSpec, NULL))
		{
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Cannot open SDL audio device,\n%s\n", SDL_GetError());
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			SD_SDL_AudioSubsystem_Up = false;
		}
		else
		{
#if 0
			// TODO: This depends on music on/off? (560Hz vs 140Hz for general interrupt handler)
			SD_SDL_SamplesPerPart = ((uint64_t)SD_SOUND_PART_RATE_BASE / SD_SFX_PART_RATE) * SD_SDL_AudioSpec.freq / PC_PIT_RATE;
			SD_SDL_MusSamplesPerPart = ((uint64_t)SD_SOUND_PART_RATE_BASE / (4*SD_SFX_PART_RATE)) * SD_SDL_AudioSpec.freq / PC_PIT_RATE;
#endif
			SD_SDL_AudioSubsystem_Up = true;
		}

		if (YM3812Init(1, 3579545, SD_SDL_AudioSpec.freq))
		{
			CK_Cross_LogMessage(CK_LOG_MSG_WARNING, "Preparation of emulated OPL chip has failed\n");
		}
	}
	/*word_4E19A = 0; */ /* TODO: Unused variable? */
	alNoCheck = false;

	// TODO: Check command line arguments - Set alNoCheck to 1 if desired

	alTimeCount = 0;
	SD_SetTimeCount(0);

	SD_SetSoundMode(sdm_Off);
	SD_SetMusicMode(smm_Off);

	if (!alNoCheck)
		AdLibPresent = SDL_DetectAdlib(true);
	/* FIXME? Otherwise what is the value of AdLibPresent? */

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

///////////////////////////////////////////////////////////////////////////
//
//	SD_Default() - Sets up the default behaviour for the Sound Mgr whether
//		the config file was present or not.
//
///////////////////////////////////////////////////////////////////////////
void
SD_Default(bool gotit,SDMode sd,SMMode sm)
{
	bool	gotsd,gotsm;

	gotsd = gotsm = gotit;

	if (gotsd)	// Make sure requested sound hardware is available
	{
		switch (sd)
		{
		case sdm_AdLib:
			gotsd = AdLibPresent;
			break;
		}
	}
	if (!gotsd)
	{
		if (AdLibPresent)
			sd = sdm_AdLib;
		else
			sd = sdm_PC;
	}
	if (sd != SoundMode)
		SD_SetSoundMode(sd);


	if (gotsm)	// Make sure requested music hardware is available
	{
		switch (sm)
		{
		case sdm_AdLib:
			gotsm = AdLibPresent;
			break;
		}
	}
	if (!gotsm)
	{
		if (AdLibPresent)
			sm = smm_AdLib;
	}
	if (sm != MusicMode)
		SD_SetMusicMode(sm);
}

void SD_MusicOff(void);

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
	SD_MusicOff();
	//SD_StopSound();
	SDL_ShutDevice();
	SDL_CleanDevice();

	// Some timer stuff not done here

	SD_Started = false;
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
	switch (SoundMode)
	{
		case sdm_PC: SDL_PCPlaySound((PCSound *)(SoundTable[sound])); break;
		case sdm_AdLib: SDL_ALPlaySound((AdLibSound *)(SoundTable[sound])); break;
	}
	SoundNumber = sound;
	SoundPriority = priority;
}

uint16_t SD_SoundPlaying(void)
{
	switch (SoundMode)
	{
		case sdm_PC: return pcSound ? SoundNumber : 0;
		case sdm_AdLib: return alSound ? SoundNumber : 0;
	}
	return 0;
}

void SD_StopSound(void)
{
	switch (SoundMode)
	{
		case sdm_PC: SDL_PCStopSound(); break;
		case sdm_AdLib: SDL_ALStopSound(); break;
	}
	SDL_SoundFinished();
}

void SD_WaitSoundDone(void)
{
	while (SD_SoundPlaying())
	{
		// The original code simply waits in a busy loop.
		// Bad idea for new code.
		// TODO: What about checking for input/graphics/other status?
		SDL_Delay(1);
	}
}

void SD_MusicOn(void)
{
	sqActive = 1;
}

/* NEVER call this from the callback!!! */
void SD_MusicOff(void)
{
	uint16_t i;
	if (MusicMode == smm_AdLib)
	{
		alFXReg = 0;
		alOut(alEffects, 0);
		for (i = 0; i < sqMaxTracks; i++)
			alOut(alFreqH + i + 1, 0);
	}
	sqActive = 0;
}

void SD_StartMusic(MusicGroup *music)
{
	SD_MusicOff();
	if (MusicMode == smm_AdLib)
	{
		sqHackPtr = sqHack = (uint16_t *)music->values;
		sqHackSeqLen = sqHackLen = music->length;
		sqHackTime = 0;
		alTimeCount = 0;
		SD_MusicOn();
	}
}

void SD_FadeOutMusic(void)
{
	if (MusicMode == smm_AdLib)
		SD_MusicOff();
}

bool SD_MusicPlaying(void)
{
	return false; // All it really does...
#if 0
	bool	result;

	switch (MusicMode)
	{
	case smm_AdLib:
		result = false;
		// DEBUG - not written
		break;
	default:
		result = false;
	}

	return(result);
#endif
}
