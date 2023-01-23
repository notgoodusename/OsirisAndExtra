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
    bool extend_antiaim(UserCmd* cmd);
    //bool extend_antiaim(UserCmd* cmd, movement_backup_t const* const movement_backup, int const max_commands, int* const command, int* const cur, angle_t const* const target_extended_angle);
}
