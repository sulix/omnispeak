/////////////////////////////////////////////////
//
// MUSE Header for .CK5
//
/////////////////////////////////////////////////

// TODO: Finish this

#define NUMSOUNDS 64
#define NUMSONGS 14
#define NUMSNDCHUNKS 206

//
// Sound names & indexes
//
typedef enum {
		SOUND_KEENWALK0 = 0,     // 0
		SOUND_KEENWALK1,         // 1
		SOUND_KEENJUMP,          // 2
		SOUND_KEENLAND,          // 3
		SOUND_KEENSHOOT,         // 4
		SOUND_MINEEXPLODE,       // 5
		SOUND_SLICEBUMP,         // 6
		SOUND_KEENPOGO,          // 7
		SOUND_GOTITEM,           // 8
		SOUND_GOTSTUNNER,        // 9
		SOUND_GOTVITALIN,        // 10
		SOUND_UNKNOWN11,         // 11
		SOUND_UNKNOWN12,         // 12
		SOUND_LEVELEXIT,         // 13
		SOUND_NEEDKEYCARD,       // 14
		SOUND_KEENHITCEILING,    // 15
		SOUND_SPINDREDFLYUP,     // 16
		SOUND_GOTEXTRALIFE,      // 17
		SOUND_OPENSECURITYDOOR,  // 18
		SOUND_GOTGEM,            // 19
		SOUND_KEENFALL,          // 20
		SOUND_KEENOUTOFAMMO,     // 21
		SOUND_UNKNOWN22,         // 22
		SOUND_KEENDIE,           // 23
		SOUND_UNKNOWN24,         // 24
		SOUND_KEENSHOTHIT,       // 25
		SOUND_UNKNOWN26,         // 26
		SOUND_SPIROSLAM,         // 27
		SOUND_SPINDREDSLAM,      // 28
		SOUND_ENEMYSHOOT,        // 29
		SOUND_ENEMYSHOTHIT,      // 30
		SOUND_AMPTONWALK0,       // 31
		SOUND_AMPTONWALK1,       // 32
		SOUND_AMPTONSTUN,        // 33
		SOUND_UNKNOWN34,         // 34
		SOUND_UNKNOWN35,         // 35
		SOUND_SHELLYEXPLODE,     // 36
		SOUND_SPINDREDFLYDOWN,   // 37
		SOUND_MASTERSHOT,        // 38
		SOUND_MASTERTELE,        // 39
		SOUND_POLEZAP,           // 40
		SOUND_UNKNOWN41,         // 41
		SOUND_SHOCKSUNDBARK,     // 42
		SOUND_UNKNOWN43,         // 43
		SOUND_UNKNOWN44,         // 44
		SOUND_BARKSHOTDIE,       // 45
		SOUND_UNKNOWN46,         // 46
		SOUND_UNKNOWN47,         // 47
		SOUND_UNKNOWN48,         // 48
		SOUND_UNKNOWN49,         // 49
		SOUND_UNKNOWN50,         // 50
		SOUND_UNKNOWN51,         // 51
		SOUND_UNKNOWN52,         // 52
		SOUND_UNKNOWN53,         // 53
		SOUND_UNKNOWN54,         // 54
		SOUND_GOTKEYCARD,        // 55
		SOUND_UNKNOWN56,         // 56
		SOUND_KEENLANDONFUSE,    // 57
		SOUND_SPARKYPREPCHARGE,  // 58
		SOUND_SPHEREFULCEILING,  // 59
		SOUND_UNKNOWN60,         // 60
		SOUND_SPIROFLY,          // 61
		LASTSOUND
} soundnames;

//
// Base offsets
//
#define STARTPCSOUNDS 0
#define STARTADLIBSOUNDS 64
#define STARTDIGISOUNDS 128 // Unused in vanilla Keen 4-6
#define STARTMUSIC 192

//
// Music names & indexes
//
typedef enum {
		CAMEIN_MUS = 0,          // 0
		LITTLEAMPTON_MUS,        // 1
		THEICE_MUS,              // 2
		SNOOPIN_MUS,             // 3
		BAGPIPES_MUS,            // 4
		WEDNESDAY_MUS,           // 5
		ROCKNOSTONE_MUS,         // 6
		OUTOFBREATH_MUS,         // 7
		SHIKADIAIRE_MUS,         // 8
		DIAMONDS_MUS,            // 9
		TIGHTER_MUS,             // 10
		ROBOREDROCK_MUS,         // 11
		FANFARE_MUS,             // 12
		BRINGEROFWAR_MUS,        // 13
		LASTMUSTRACK
} musicnames;

/////////////////////////////////////////////////
//
// Thanks for playing with MUSE!
//
/////////////////////////////////////////////////

#if 0
#define SOUND_KEENDIE 23
#define SOUND_STARTLEVEL 12
#define SOUND_ENDLEVEL 13
#define SOUND_PADDLESCORE 50
#endif
