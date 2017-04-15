#include "ck_def.h"
#include "ck_net.h"
#include "id_rf.h"

#include <SDL.h>

#include <string.h>
#include <enet/enet.h>


#define INPUT_BUFFER_SIZE 256
CK_TicInput inputBuffer[INPUT_BUFFER_SIZE][MAX_NET_PLAYERS];
int gameTic;
int lastPlayerTic[MAX_NET_PLAYERS];
int arrivalTimes[INPUT_BUFFER_SIZE][MAX_NET_PLAYERS];
int baseTicTime;

CK_NetGameState authState;

ENetHost *server;

typedef struct {
    // int lastAck;
    int playerIdx;
} CK_PeerData;

CK_PeerData ck_peerData[MAX_NET_PLAYERS];

void InitServer()
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    server = enet_host_create(&address, MAX_NET_PLAYERS, 2, 0, 0);

    if (server == NULL)
    {
        Quit("An error occurred while trying to create an ENet server host.\n");
    }

    // Wait for n players to join
    ENetEvent event;
    int conn = 0;
    int level = net_level;

    bool start = false;
    do 
    {
        if (enet_host_service(server, &event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("A new client connected from %x:%u.\n", 
                            event.peer -> address.host,
                            event.peer -> address.port);
                    /* Store any relevant client information here. */
                    event.peer -> data = &ck_peerData[conn];
                    ck_peerData[conn].playerIdx = conn;
                    conn++;
                    break;
            }

            // Got everyone, now go!
            if (conn == net_numPlayers) 
            {
                for (int i = 0; i < net_numPlayers; i++)
                {
                    Net_SetupPacket sp = { i, net_numPlayers, level};
                    ENetPacket *packet = enet_packet_create(&sp, sizeof(sp), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(&server->peers[i], 0, packet);
                }
                start = true;
            }
        }
    } while (!start);

    net_state = &authState;
    CK_Net_PlayInit(level, net_numPlayers);
    baseTicTime = SD_GetTimeCount();
}

void ServerLoop()
{
    while (1)
    {
        // Delay until it's time to run tic
        int nextTicTime = baseTicTime + TIME_PER_TIC * gameTic;
        do
        {
            // Get all the input
            ENetEvent event;
            while (enet_host_service(server, &event, 0)) 
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        if (event.packet->dataLength == sizeof(CK_ClientInputPacket))
                        {
                            CK_PeerData *peerData = (CK_PeerData *)event.peer->data;
                            CK_ClientInputPacket *packetData = 
                                (CK_ClientInputPacket *)event.packet->data;
                            int t = packetData->tic % INPUT_BUFFER_SIZE;
                            arrivalTimes[t][peerData->playerIdx] = SD_GetTimeCount();
                            inputBuffer[t][peerData->playerIdx] = packetData->input;
                            lastPlayerTic[peerData->playerIdx] = packetData->tic;

                            printf("got\t\tplayer %d\ttic %d\t\t\ttime %d\n", peerData->playerIdx,
                                    packetData->tic, SD_GetTimeCount());
                        }
                        else
                        {
                            Quit("serverLoop: Received unknown packet!");
                        }

                        enet_packet_destroy (event.packet);
                        break;
                }
            }
            SDL_Delay(1); // TODO: can we delay with better than 10 ms granularity?
        } while (SD_GetTimeCount() <= nextTicTime);

        // Run the tic
        for (int i = 0; i < MAX_NET_PLAYERS; i++)
        {
            // Dead Reckoning: If the client has missed the tic, then
            // assume that his input remains unchanged from last tic
            if (lastPlayerTic[i] < gameTic)
            {
                inputBuffer[gameTic % INPUT_BUFFER_SIZE][i] = 
                    inputBuffer[lastPlayerTic[i] % INPUT_BUFFER_SIZE][i];
            }
        }
        CK_Net_PlayTic(&inputBuffer[gameTic % INPUT_BUFFER_SIZE]);

        // Send input for this tic
        CK_ServerInputPacket data; 
        data.tic = gameTic;
        memcpy(&data.inputs, inputBuffer[gameTic % INPUT_BUFFER_SIZE], MAX_NET_PLAYERS * sizeof(CK_TicInput));
        int nowTime = SD_GetTimeCount();
        for (int i = 0; i < server->peerCount; i++)
        {
            if (server->peers[i].state != ENET_PEER_STATE_CONNECTED)
                continue;

            int last = lastPlayerTic[i];
            int getTime = arrivalTimes[last % INPUT_BUFFER_SIZE][i];
            int runTime = baseTicTime + last * TIME_PER_TIC;
            data.lead = getTime - runTime;
            ENetPacket *packet = enet_packet_create(&data, sizeof(CK_ServerInputPacket), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(&server->peers[i], 0, packet);
            printf("sent\t\tplayer %d\ttic %d\t\t\ttime %d\t\tlead %d\n", i, gameTic, SD_GetTimeCount(), data.lead);
        }

        // Next tic
        gameTic++;
    }
}

void CK_Net_RunServer()
{
    InitServer();
    ServerLoop();
}

