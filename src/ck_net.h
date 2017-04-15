#ifndef CK_NET_H
#define CK_NET_H

#define CLIENT_INPUT_PER_PACKET 32
#define CLIENT_ACK_PER_PACKET 32
#define MAX_NET_PLAYERS 16
#define TIME_PER_TIC 2

extern int net_numPlayers;
extern int net_level;

typedef enum
{
    Net_None,
    Net_Server,
    Net_Client,
} CK_NetMode;

typedef struct
{
    uint16_t playerId;
    uint16_t numPlayers;
    uint16_t level;

} Net_SetupPacket;

extern CK_NetMode net_mode;

typedef struct CK_NetPlayerState
{
	int jumpTimer;
	int poleGrabTime;
	bool jumpIsPressed;
	bool jumpWasPressed;
	bool pogoIsPressed;
	bool pogoWasPressed;
	bool shootIsPressed;
	bool shootWasPressed;
	bool keenSliding;
	CK_object *platform;
    int invincibilityTimer;
    CK_object *obj;
    int pogoTimer;
    int numShots;
    int numBombs;
    int keyGems[4];
    int health;
    int respawnTimer;
    bool respawn; // respawning this frame?
} CK_NetPlayerState;

#define MAX_SPAWN_POINTS 32
typedef struct CK_Net_SpawnPoint
{
    uint16_t tileX, tileY;
    bool direction;
    int team;
} CK_Net_SpawnPoint;

typedef enum {
    Net_RT_Nil,
    Net_RT_Tile,
    Net_RT_Item,
} Net_RespawnType;

#define NET_MAX_RESPAWNS 256

typedef struct Net_Respawn {
    uint16_t tileX, tileY;
    union {
        int item;
        struct {
            uint16_t index;
            uint16_t anim;
        } tile;
    } type;
    int timer;
    struct Net_Respawn *prev, *next;
} Net_Respawn;

typedef struct {

    int numPlayers;
    int level;
    int tic;
    CK_NetPlayerState playerStates[MAX_NET_PLAYERS];
    IN_ControlFrame inputFrames[MAX_NET_PLAYERS];

    // Object list
    CK_object objArray[CK_MAX_OBJECTS];
    CK_object *freeObject;
    CK_object *firstObject;
    CK_object *lastObject;
    int numObjects;

    // Tile and item Respawn list
    Net_Respawn respawnArray[NET_MAX_RESPAWNS];
    Net_Respawn *firstRespawn;
    Net_Respawn *freeRespawn;
    int numRespawns;

    // Player spawnpoints
    CK_Net_SpawnPoint spawnPoints[MAX_SPAWN_POINTS];
    int numSpawn;

    // Mutable tiles
} CK_NetGameState;

extern CK_NetGameState *net_state;

typedef uint32_t CK_ClientAck;

typedef struct
{ bool jump : 1;
    bool pogo : 1;
    bool button2 : 1;
    bool button3 : 1;
    int xDirection : 2;
    int yDirection : 2;
} CK_TicInput;

typedef struct
{
    uint32_t tic;
    CK_TicInput input;
    // CK_TicInput inputs[CLIENT_INPUT_PER_PACKET];
    // CK_ClientAck acks;
} CK_ClientInputPacket;

typedef struct
{
    uint32_t tic;
    int16_t lead;
    CK_TicInput inputs[MAX_NET_PLAYERS];
} CK_ServerInputPacket;

void Net_PlaySound(soundnames sound);

CK_TicInput Net_InputFrameToTicInput(IN_ControlFrame cf);
IN_ControlFrame Net_TicInputToControlFrame(CK_TicInput ti);

void CK_Net_SetupObjArray();
CK_object *CK_Net_GetNewObj(bool nonCritical);
void CK_Net_RemoveObj(CK_object *obj);
void CK_Net_PlayInit(int level, int players);
void CK_Net_PlayTic(CK_TicInput (*input)[MAX_NET_PLAYERS]); 

// ck_server.c
extern int net_numPlayers;
void CK_Net_RunServer();

// ck_client.c
void Net_CentreCameraSelf(int playerId);
void CK_Net_RunClient();

#endif
