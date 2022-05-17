#pragma once

struct UserCmd;

namespace Tickbase
{
    void run(UserCmd*) noexcept;

    bool CanDT();

    bool DTWeapon();


    bool CanFireWeapon(float curtime);

    float Firerate();

    bool CanFireWithExploit(int m_iShiftedTick);

    void updateInput() noexcept;

    int doubletapCharge = 0,
        ticksToShift = 0,
        commandNumber = 0,
        lastShift = 0,
        shiftAmount = 0,
        maxTicks = 16;
    bool isShifting = false;
    bool reset = false;
    bool charged = false;
}