#pragma once

#include "ClientClass.h"
#include "Pad.h"
#include "NetworkChannel.h"
#include "Vector.h"

class EventInfo {
public:
    enum {
        EVENT_INDEX_BITS = 8,
        EVENT_DATA_LEN_BITS = 11,
	MAX_EVENT_DATA = 192,  // ( 1<<8 bits == 256, but only using 192 below )
    };
    short classID;
    float fireDelay;
    const void* sendTable;
    const ClientClass* clientClass;
    int bits;
    uint8_t* data;
    int flags;
    PAD(28)
    EventInfo* next;
};

class ClientState
{
public:
    void forceFullUpdate()
    {
        deltaTick = -1;
    }

    PAD(156)
    NetworkChannel* netChannel;
    int challengeNr;
    PAD(4)
    double connectTime;
    int retryNumber;
    PAD(84)
    int signonState;
    PAD(8)
    float nextCmdTime;
    int serverCount;
    int currentSequence;
    PAD(8)
    struct {
        float clockOffsets[16];
        int currentClockOffset;
        int serverTick;
        int clientTick;
    } clockDriftManager;
    int deltaTick;
    bool paused;
    PAD(7)
    int viewEntity;
    int playerSlot;
    char levelName[260];
    char levelNameShort[80];
    char groupName[80];
    char lastLevelNameShort[80];
    PAD(12)
    int maxClients;
    PAD(4083)
    uint32_t stringTableContainer;
    PAD(14737)
    float lastServerTickTime;
    bool inSimulation;
    PAD(3)
    int oldTickCount;
    float tickRemainder;
    float frameTime;
    int lastOutgoingCommand;
    int chokedCommands;
    int lastCommandAttack;
    int commandAttack;
    int soundSequence;
    int lastProgressPercent;
    bool ishltv;
    PAD(75)
    Vector angleViewPoint;
    PAD(208)
    EventInfo* events;
};
