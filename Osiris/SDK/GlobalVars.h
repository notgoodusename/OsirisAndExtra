#pragma once

#include <cstddef>

#include "Pad.h"

struct UserCmd;

struct GlobalVars {
    const float realtime;
    int framecount;
    const float absoluteFrameTime;
    PAD(4);
    float currenttime;
    float frametime;
    const int maxClients;
    const int tickCount;
    const float intervalPerTick;

    float serverTime(UserCmd* = nullptr) const noexcept;
};
