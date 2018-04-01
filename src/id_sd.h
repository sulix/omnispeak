//
//	ID Engine
//	ID_SD.h - Sound Manager Header
//	Version for Wolfenstein
//	By Jason Blochowiak
//

#ifndef	ID_SD_H
#define	ID_SD_H

typedef int16_t soundnames;
typedef int16_t musicnames;

// #include "audiock5.h"
#include "opl/dbopl.h"

typedef	enum
{
	sdm_Off,
	sdm_PC,sdm_AdLib,
} SDMode;

typedef	enum
{
	smm_Off,smm_AdLib
} SMMode;

typedef	struct
{
	uint32_t length;
	uint16_t priority;
} __attribute__((__packed__)) SoundCommon;

typedef	struct
{
	SoundCommon common;
	uint8_t data[1];
} __attribute__((__packed__)) PCSound;

// 	Registers for the AdLib card
#define	alFMStatus	0x388	// R
#define	alFMAddr	0x388	// W
#define	alFMData	0x389	// W

//	Register addresses
// Operator stuff
#define	alChar		0x20
#define	alScale		0x40
#define	alAttack	0x60
#define	alSus		0x80
#define	alWave		0xe0
// Channel stuff
#define	alFreqL		0xa0
#define	alFreqH		0xb0
#define	alFeedCon	0xc0
// Global stuff
#define	alEffects	0xbd

typedef	struct
		{
			uint8_t	mChar,cChar,
					mScale,cScale,
					mAttack,cAttack,
					mSus,cSus,
					mWave,cWave,
					nConn,

					// These are only for Muse - these bytes are really unused
					voice,
					mode,
					unused[3];
		} __attribute__((__packed__)) Instrument;

typedef	struct
		{
			SoundCommon	common;
			Instrument	inst;
			uint8_t		block,
						data[1];
		} __attribute__((__packed__)) AdLibSound;

//
//	Sequencing stuff
//
#define	sqMaxTracks	10
#define	sqMaxMoods	1	// DEBUG

#define	sev_Null		0	// Does nothing
#define	sev_NoteOff		1	// Turns a note off
#define	sev_NoteOn		2	// Turns a note on
#define	sev_NotePitch	3	// Sets the pitch of a currently playing note
#define	sev_NewInst		4	// Installs a new instrument
#define	sev_NewPerc		5	// Installs a new percussive instrument
#define	sev_PercOn		6	// Turns a percussive note on
#define	sev_PercOff		7	// Turns a percussive note off
#define	sev_SeqEnd		-1	// Terminates a sequence

// 	Flags for MusicGroup.flags
#define	sf_Melodic		0
#define	sf_Percussive	1

#if 1
typedef	struct
		{
			uint16_t length,
			         values[1];
		} __attribute__((__packed__)) MusicGroup;
#else
typedef	struct
		{
			uint16_t flags,
			         count,
			         offsets[1];
		} __attribute__((__packed__)) MusicGroup;
#endif

typedef	struct
		{
			/* This part needs to be set up by the user */
			uint16_t mood, *moods[sqMaxMoods];

			/* The rest is set up by the code */
			Instrument inst;
			bool percussive;
			uint16_t *seq;
			uint32_t nextevent;
		} __attribute__((__packed__)) ActiveTrack;

#define	sqmode_Normal		0
#define	sqmode_FadeIn		1
#define	sqmode_FadeOut		2

#define	sqMaxFade		64	// DEBUG

extern bool AdLibPresent;
extern SDMode SoundMode;
extern SMMode MusicMode;
extern bool quiet_sfx;

bool SD_SetSoundMode(SDMode mode);
bool SD_SetMusicMode(SMMode mode);
void SD_Startup(void);
void SD_Default(bool gotit,SDMode sd,SMMode sm);
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

/* Timing related functions */

uint32_t SD_GetTimeCount(void);
void SD_SetTimeCount(uint32_t newval);
int32_t SD_GetLastTimeCount(void);
void SD_SetLastTimeCount(int32_t newval);
uint16_t SD_GetSpriteSync(void);
void SD_SetSpriteSync(uint16_t newval);

#endif
