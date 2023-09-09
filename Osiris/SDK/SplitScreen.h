#pragma once

#include "Pad.h"
#include "ClientState.h"

struct SplitScreen {
    void* vmt;

    struct SplitPlayer {
        PAD(8)
        ClientState client;
    };

    SplitPlayer* splitScreenPlayers[1];
};