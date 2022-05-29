#pragma once

#include "../ConfigStructs.h"

struct UserCmd;
struct Vector;

namespace AntiAim
{
    void rage(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
    void legit(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
    void run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept;
    void updateInput() noexcept;
    bool canRun(UserCmd* cmd) noexcept;
    float rollaavalue {0.f};
}
