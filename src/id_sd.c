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

SD_Backend *sd_backend;

SDMode SoundMode;
SMMode MusicMode;
uint16_t SoundPriority, DigiPriority;
// Internal variables (TODO more to add)
static bool SD_Started;
volatile soundnames SoundNumber,DigiNumber;
uint8_t **SoundTable;
int16_t NeedsDigitized; // Unused

// PC Sound variables
uint8_t pcLastSample, *pcSound;
uint32_t pcLengthLeft;
uint16_t pcSoundLookup[255];

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

/******************************************************************
Timing subsection.
Originally SDL_t0Service is responsible for incrementing TimeCount.
/*****************************************************************/

// PRIVATE variables (in this source port)
static uint32_t TimeCount = 0; // Kind of same as Wolf3D's TimeCount from ID_SD.C
static int32_t lasttimecount = 0; // Same as Wolf3D's lasttimecount from WL_DRAW.C?
static uint16_t SpriteSync = 0;
// PIT timer divisor, scaled (bt 8 if music is on, 2 otherwise).
// NOT initialized to 0 since this can lead to division by zero on startup.
static int16_t ScaledTimerDivisor = 1;
// A few variables used for timing measurements (PC_PIT_RATE units per second)
static uint64_t SD_LastPITTickTime;

uint32_t SD_GetTimeCount()
{
	return TimeCount;
}

void SD_SetTimeCount(uint32_t newval)
{
	SD_GetTimeCount(); // Refresh SD_LastPITTickTime to be in sync with SDL_GetTicks()
	TimeCount = newval;
}

int32_t SD_GetLastTimeCount(void) { return lasttimecount; }
void SD_SetLastTimeCount(int32_t newval) { lasttimecount = newval; }
uint16_t SD_GetSpriteSync(void) { return SpriteSync; }
void SD_SetSpriteSync(uint16_t newval) { SpriteSync = newval; }

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void alOut(uint8_t reg, uint8_t val)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	sd_backend->alOut(reg, val);
	sd_backend->unlock();
}

/* FIXME: The SDL prefix may conflict with SDL functions in the future(???)
 * Best (but hackish) solution, if it happens: Add our own custom prefix.
 */

void SDL_t0Service(void);

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_SetIntsPerSecond(int16_t tickrate)
{
	ScaledTimerDivisor = ((int32_t)SD_SOUND_PART_RATE_BASE / (int32_t)tickrate) & 0xFFFF;
	sd_backend->lock();
	sd_backend->setTimer0(ScaledTimerDivisor);
	sd_backend->unlock();
}

void SDL_PCPlaySound_Low(PCSound *sound)
{
	pcLastSample = 255;
	pcLengthLeft = CK_Cross_SwapLE32(sound->common.length);
	pcSound = (uint8_t *)sound->data;
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_PCPlaySound(PCSound *sound)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SDL_PCPlaySound_Low(sound);
	sd_backend->unlock();
}

void SDL_PCStopSound_Low(void)
{
	pcSound = 0;
	// Turn the speaker off
	if (sd_backend)
		sd_backend->pcSpkOn(false, 0);
}

/* NEVER call this from the SDL callback!!! (Or you want a deadlock?) */
void SDL_PCStopSound(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SDL_PCStopSound_Low();
	sd_backend->unlock();
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
				sd_backend->pcSpkOn(true, t);
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
				// Turn the speaker & gate off
				sd_backend->pcSpkOn(false, 0);
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
	if (sd_backend)
		sd_backend->pcSpkOn(false, 0);
}

/* NEVER call this from the callback!!! */
void SDL_ShutPC(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SDL_ShutPC_Low();
	sd_backend->unlock();
}

void SDL_ALStopSound_Low(void)
{
	alSound = 0;
	sd_backend->alOut(alFreqH + 0,0);
}

/* NEVER call this from the callback!!! */
void SDL_ALStopSound(void)
{
	if (sd_backend)
		sd_backend->lock();
	SDL_ALStopSound_Low();
	if (sd_backend)
		sd_backend->unlock();
}

static void SDL_AlSetFXInst(Instrument *inst)
{
	uint8_t	c,m,cScale;

	m = modifiers[0];
	c = carriers[0];
	sd_backend->alOut(m + alChar,inst->mChar);
	sd_backend->alOut(m + alScale,inst->mScale);
	sd_backend->alOut(m + alAttack,inst->mAttack);
	sd_backend->alOut(m + alSus,inst->mSus);
	sd_backend->alOut(m + alWave,inst->mWave);

	sd_backend->alOut(c + alChar,inst->cChar);

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
	sd_backend->alOut(c + alScale,cScale);

	sd_backend->alOut(c + alAttack,inst->cAttack);
	sd_backend->alOut(c + alSus,inst->cSus);
	sd_backend->alOut(c + alWave,inst->cWave);
}

static void SDL_ALPlaySound_Low(AdLibSound *sound)
{
	Instrument *inst;

	// Do NOT call the non-low variant as we're already in such a variant!!!
	SDL_ALStopSound_Low();

	alLengthLeft = CK_Cross_SwapLE32(sound->common.length);
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
	if (!sd_backend)
		return;
	sd_backend->lock();
	SDL_ALPlaySound_Low(sound);
	sd_backend->unlock();
}

void SDL_ALSoundService(void)
{
	uint8_t s;

	if (alSound)
	{
		s = *alSound++;
		if (!s)
			sd_backend->alOut(alFreqH + 0,0);
		else
		{
			sd_backend->alOut(alFreqL + 0,s);
			sd_backend->alOut(alFreqH + 0,alBlock);
		}

		if (!(--alLengthLeft))
		{
			alSound = 0;
			sd_backend->alOut(alFreqH + 0,0);
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
		sd_backend->alOut(a,v);
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
	sd_backend->alOut(alEffects, 0);
	sd_backend->alOut(alFreqH + 0, 0);
	SDL_AlSetFXInst(&alZeroInst);
	alSound = 0;
}

/* NEVER call this from the callback!!! */
static void SDL_ShutAL(void)
{
	if (!sd_backend)
		return;
	sd_backend->lock();
	SDL_ShutAL_Low();
	sd_backend->unlock();
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
		if (!(count & 7))
		{
			TimeCount++;
		}
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
		if (!(count & 1))
		{
			TimeCount++;
		}
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
	
	sd_backend = SD_Impl_GetBackend();
	
	if (sd_backend)
		sd_backend->startup();
	
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
	SD_MusicOff();
	//SD_StopSound();
	SDL_ShutDevice();
	SDL_CleanDevice();

	// Some timer stuff not done here
	if (sd_backend)
		sd_backend->shutdown();

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
	length = CK_Cross_SwapLE32(*(uint32_t *)(&SoundTable[sound]));
	if (!length)
		Quit("SD_PlaySound() - Zero length sound");
	priority = CK_Cross_SwapLE16(*(uint16_t *)(SoundTable[sound] + 4));
	if (priority < SoundPriority)
	{
		return;
	}
	switch (SoundMode)
	{
		case sdm_PC: SDL_PCPlaySound((PCSound *)(SoundTable[sound])); break;
		case sdm_AdLib: SDL_ALPlaySound((AdLibSound *)(SoundTable[sound])); break;
		default: break;
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
		default: break;
	}
	return 0;
}

void SD_StopSound(void)
{
	switch (SoundMode)
	{
		case sdm_PC: SDL_PCStopSound(); break;
		case sdm_AdLib: SDL_ALStopSound(); break;
		default: break;
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
	}
}

/* NEVER call this from the callback!!! */
void SD_MusicOn(void)
{
	if (sd_backend)
		sd_backend->lock();
	sqActive = 1;
	if (sd_backend)
		sd_backend->unlock();
}

/* Also don't call this from the callback */
void SD_MusicOff(void)
{
	uint16_t i;
	if (sd_backend)
		sd_backend->lock();
	if (MusicMode == smm_AdLib)
	{
		alFXReg = 0;
		sd_backend->alOut(alEffects, 0);
		for (i = 0; i < sqMaxTracks; i++)
			sd_backend->alOut(alFreqH + i + 1, 0);
	}
	sqActive = 0;
	if (sd_backend)
		sd_backend->unlock();
}

void SD_StartMusic(MusicGroup *music)
{
	SD_MusicOff();
	if (MusicMode == smm_AdLib)
	{
		if (sd_backend)
			sd_backend->lock();
		sqHackPtr = sqHack = (uint16_t *)music->values;
		sqHackSeqLen = sqHackLen = music->length;
		sqHackTime = 0;
		alTimeCount = 0;
		if (sd_backend)
			sd_backend->unlock();
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
}
