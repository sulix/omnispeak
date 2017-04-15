#include "ck_def.h"
#include "ck_act.h"
#include "ck_net.h"
#include "ck_play.h"

#include "id_rf.h"
#include "id_vl.h"

#include <SDL.h>

#include <string.h>
#include <enet/enet.h>

static ENetHost *client;
static ENetPeer *server;

static int baseSendTime;
static int sendTic;
static int rebaseTic;
static int serverTic;

static int clientPlayerId;

#define INPUT_BUFFER_SIZE 256
static CK_TicInput inputBuffer[INPUT_BUFFER_SIZE][MAX_NET_PLAYERS];

static CK_NetGameState authState;

/* 
 * Camera
 */

extern int rf_scrollXUnit;
extern int rf_scrollYUnit;

// The intended y-coordinate of the bottom of the keen sprite
// in pixels from the top of the screen.
static uint16_t screenYpx;

// Centre the camera on the given object.

static void CentreCamera(CK_object *obj)
{
    uint16_t screenX, screenY;

    screenYpx = 140;

    if (obj->posX < RF_PixelToUnit(152))
        screenX = 0;
    else
        screenX = obj->posX - RF_PixelToUnit(152);

    if (obj->clipRects.unitY2 < RF_PixelToUnit(140))
        screenY = 0;
    else
        screenY = obj->clipRects.unitY2 - RF_PixelToUnit(140);

    RF_Reposition(screenX, screenY);
}

void Net_CentreCameraSelf(int playerId)
{
    if (net_mode == Net_Client && playerId == clientPlayerId)
        CentreCamera(net_state->playerStates[clientPlayerId].obj);
}

static void NormalCamera(CK_object *obj)
{

    int16_t deltaX = 0, deltaY = 0;	// in Units

    //TODO: some unknown var must be 0
    //This var is a "ScrollDisabled flag." If keen dies, it's set so he
    // can fall out the bottom
    if (ck_scrollDisabled)
        return;

    // End level if keen makes it out either side
    if (obj->clipRects.unitX1 < rf_scrollXMinUnit || obj->clipRects.unitX2 > rf_scrollXMaxUnit + RF_PixelToUnit(320))
    {
        ck_gameState.levelState = 2;
        return;
    }

    // Kill keen if he falls out the bottom
    if (obj->clipRects.unitY2 > (rf_scrollYMaxUnit + RF_PixelToUnit(208)))
    {
#if 0
        obj->posY -= obj->clipRects.unitY2 - (rf_scrollYMaxUnit + RF_PixelToUnit(208));
        SD_PlaySound(SOUND_KEENFALL);
        ck_godMode = false;
        CK_KillKeen();
        return;
#endif
    }


    // Keep keen's x-coord between 144-192 pixels
    if (obj->posX < (rf_scrollXUnit + RF_PixelToUnit(144)))
        deltaX = obj->posX - (rf_scrollXUnit + RF_PixelToUnit(144));

    if (obj->posX > (rf_scrollXUnit + RF_PixelToUnit(192)))
        deltaX = obj->posX - (rf_scrollXUnit + RF_PixelToUnit(192));


    // Keen should be able to look up and down.
    if (obj->currentAction == CK_GetActionByName("NK_ACT_keenLookUp2"))
    {
        int16_t pxToMove;
        if (screenYpx + SD_GetSpriteSync() > 167)
        {
            // Keen should never be so low on the screen that his
            // feet are more than 167 px from the top.
            pxToMove = 167 - screenYpx;
        }
        else
        {
            // Move 1px per tick.
            pxToMove = SD_GetSpriteSync();
        }
        screenYpx += pxToMove;
        deltaY = RF_PixelToUnit(-pxToMove);

    }
    else if (obj->currentAction == CK_GetActionByName("NK_ACT_keenLookDown3"))
    {
        int16_t pxToMove;
        if (screenYpx - SD_GetSpriteSync() < 33)
        {
            // Keen should never be so high on the screen that his
            // feet are fewer than 33 px from the top.
            pxToMove = screenYpx - 33;
        }
        else
        {
            // Move 1px/tick.
            pxToMove = SD_GetSpriteSync();
        }
        screenYpx -= pxToMove;
        deltaY = RF_PixelToUnit(pxToMove);
    }

    if (obj->topTI || !obj->clipped || obj->currentAction == CK_GetActionByName("NK_ACT_keenHang1"))
    {
        if (obj->currentAction != CK_GetActionByName("NK_ACT_keenPull1") &&
                obj->currentAction != CK_GetActionByName("NK_ACT_keenPull2") &&
                obj->currentAction != CK_GetActionByName("NK_ACT_keenPull3") &&
                obj->currentAction != CK_GetActionByName("NK_ACT_keenPull4"))
        {
            deltaY += obj->deltaPosY;

            //TODO: Something hideous
            // TODO: Convert to 16-bit once the rest is converted
            // (due to unsigned vs signed mess)
            int cmpAmt = RF_PixelToUnit(screenYpx) + rf_scrollYUnit + deltaY;
            if (cmpAmt != obj->clipRects.unitY2)
            {
                int oneDiff, otherDiff;
                if (obj->clipRects.unitY2 < cmpAmt)
                    oneDiff = cmpAmt - obj->clipRects.unitY2;
                else
                    oneDiff = obj->clipRects.unitY2 - cmpAmt;

                // TODO: Unsigned shift left,
                // followed by a signed shift right...
                otherDiff = (signed)((unsigned)oneDiff << 4) >> 7;
                if (otherDiff > 48)
                    otherDiff = 48;
                otherDiff *= SD_GetSpriteSync();
                if (otherDiff < 16)
                {
                    if (oneDiff < 16)
                        otherDiff = oneDiff;
                    else
                        otherDiff = 16;
                }
                if (obj->clipRects.unitY2 < cmpAmt)
                    deltaY -= otherDiff;
                else
                    deltaY += otherDiff;
                int16_t adjAmt = (((screenYpx << 4) + rf_scrollYUnit + deltaY - obj->clipRects.unitY2));
                int16_t adjAmt2 = abs(adjAmt / 8);

                adjAmt2 = (adjAmt2 <= 48) ? adjAmt2 : 48;

                if (adjAmt > 0)
                    deltaY -= adjAmt2;
                else
                    deltaY += adjAmt2;
            }
        }
    }
    else
    {
        // Reset to 140px.
        screenYpx = 140;
    }


    // Scroll the screen to keep keen between 33 and 167 px.
    if (obj->clipRects.unitY2 < (rf_scrollYUnit + deltaY + RF_PixelToUnit(32)))
        deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + RF_PixelToUnit(32));


    if (obj->clipRects.unitY2 > (rf_scrollYUnit + deltaY + RF_PixelToUnit(168)))
        deltaY += obj->clipRects.unitY2 - (rf_scrollYUnit + deltaY + RF_PixelToUnit(168));

    //Don't scroll more than one tile's worth per frame.
    if (deltaX || deltaY)
    {
        if (deltaX > 255) deltaX = 255;
        else if (deltaX < -255) deltaX = -255;

        if (deltaY > 255) deltaY = 255;
        else

            if (deltaY < -255) deltaY = -255;

        // Do the scroll!
        RF_SmoothScroll(deltaX, deltaY);
    }
}

/*
 * Network
 */

void InitClient()
{
    ENetAddress address;
    ENetEvent event;

    enet_address_set_host (& address, "localhost");
    address.port = 1234;

    client = enet_host_create (NULL /* create a client host */,
            1 /* only allow 1 outgoing connection */,
            2 /* allow up 2 channels to be used, 0 and 1 */,
            57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
            14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);
    if (client == NULL)
    {
        Quit("An error occurred while trying to create an ENet client host.\n");
    }

    /* Initiate the connection, allocating the two channels 0 and 1. */
    server = enet_host_connect (client, & address, 2, 0);    
    if (server == NULL)
    {
        Quit("No available peers for initiating an ENet connection.\n");
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (client, & event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts ("Connection to localhost:1234 succeeded.");
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset (server);
        Quit ("Connection to localhost:1234 failed.");
    }

    // Wait for start signal from server
    bool start;
    int level;
    int numPlayers;
    do 
    {
        if (enet_host_service(client, & event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    if (event.packet->dataLength == sizeof(Net_SetupPacket))
                    {
                        Net_SetupPacket *packetData = (Net_SetupPacket *)event.packet->data;
                        numPlayers = packetData->numPlayers;
                        level = packetData->level;
                        clientPlayerId = packetData->playerId;

                        start = true;
                    }
                    else
                    {
                        Quit("CK_InitClient: Received unknown packet!");
                    }

                    enet_packet_destroy (event.packet);
                    break;
            }
        }
    } while (!start);

    net_state = &authState;
    CK_Net_PlayInit(level, numPlayers);
    baseSendTime = SD_GetTimeCount();
    sendTic = 0;
    rebaseTic = 0;
    ck_scrollDisabled = false;
    CentreCamera(net_state->playerStates[clientPlayerId].obj);
    /*
    RF_NetRefresh();
    VL_Present();
    */
}

CK_TicInput ClientInput()
{
    IN_ControlFrame frame;
    IN_ReadControls(0, &frame);
    return Net_InputFrameToTicInput(frame);
}


// Server is continuously feeding us state updates with lastTic run and
// lead or lag time by which our input for that tic arrived
void ClientLoop()
{
    int lastLead = 0;
    while (1)
    {
        // Delay until it's time to send input
        int nextSendTime = baseSendTime + TIME_PER_TIC * sendTic;
        do
        {
            // As soon as the next tic arrives, advance the authoritative state with it
            ENetEvent event;
            while (enet_host_service(client, &event, 0)) 
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        if (event.packet->dataLength == sizeof(CK_ServerInputPacket))
                        {
                            CK_ServerInputPacket *packetData = 
                                (CK_ServerInputPacket *)event.packet->data;

                            if (packetData->tic != serverTic)
                            {
                                Quit("CK_ClientLoop: Received input for unexpected tic!");
                            }

                            printf("get\t\ttic %d\t\t\ttime %d\t\tlead %d\n", packetData->tic, 
                                    SD_GetTimeCount(), packetData->lead);

                            lastLead = packetData->lead;
                            CK_Net_PlayTic(&packetData->inputs);
                            serverTic++;
                        }
                        else
                        {
                            Quit("CK_ClientLoop: Received unknown packet!");
                        }

                        enet_packet_destroy(event.packet);
                        break;
                }
            }
            SDL_Delay(1); // TODO: can we delay with better than 10 ms granularity?
        } while (SD_GetTimeCount() <= nextSendTime);

        // Get input now and send it
        // TODO: Client side prediction - store input, predict with this input
        IN_PumpEvents();
        CK_TicInput input = ClientInput();

        // If the lead time is outside a tolerable range, then rebase
        if (serverTic > rebaseTic + 5)
        {
            if (lastLead < -4)
            {
                // Inputs arrive at server too early; shift backwards
                baseSendTime -= lastLead + 2;
                rebaseTic = sendTic;
            }
            else if (lastLead > -1)
            {
                // Inputs arrive at server too late; shift forwards
                baseSendTime -= lastLead + 2;
                rebaseTic = sendTic;
            }
            else
            {
                // Inputs are arriving at the server in an appropriate range of time
                // before tic is run
            }
        }

        CK_ClientInputPacket data = { sendTic, input };
        ENetPacket *packet = enet_packet_create(&data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(server, 0, packet);
        printf("sent\t\tinput %d\t\ttime %d\n", sendTic, SD_GetTimeCount());
        sendTic++;

        // Update display
        NormalCamera(authState.playerStates[clientPlayerId].obj);

        if (net_state->playerStates[clientPlayerId].respawn)
            CentreCamera(net_state->playerStates[clientPlayerId].obj);

		// 0xef for the X-direction to match EGA keen's 2px horz scrolling.
		VL_SetScrollCoords(RF_UnitToPixel(rf_scrollXUnit & 0xef), RF_UnitToPixel(rf_scrollYUnit & 0xff));

        RF_NetRefresh();
        VL_Present();
    }
}

void CK_Net_RunClient()
{
    InitClient();
    ClientLoop();
}
