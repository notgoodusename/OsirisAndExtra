#pragma once

struct UserCmd;

namespace Tickbase
{
    void run(UserCmd*) noexcept;

    int doubletapCharge = 0,
        ticksToShift = 0,
        commandNumber = 0,
        lastShift = 0,
        shiftAmount = 0,
        maxTicks = 16;
    bool isShifting = false;
}