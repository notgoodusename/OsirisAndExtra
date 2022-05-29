#pragma once
#include "../SDK/GameEvent.h"
struct UserCmd;

namespace Tickbase
{
    void run(UserCmd* cmd) noexcept;

    void getEvent(GameEvent* event) noexcept;

    bool CanDT();

    bool DTWeapon();
    void Telepeek();
    bool warmup = true;
    bool dotelepeek = false;

    bool CanFireWeapon(float curtime);

    bool CanHideWeapon(float curtime);

    float Firerate();

    bool CanFireWithExploit(int m_iShiftedTick);

    bool CanHideWithExploit(int m_iShiftedTick);

    void updateEventListeners(bool forceRemove) noexcept;

    void updateInput() noexcept;

    int doubletapCharge = 0,
        ticksToShift = 0,
        commandNumber = 0,
        lastShift = 0,
        shiftAmount = 0,
        maxTicks = 19;
    bool isShifting = false;
    bool reset = false;
    bool charged = false;
}