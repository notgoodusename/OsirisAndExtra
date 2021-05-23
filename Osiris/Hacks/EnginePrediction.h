#pragma once

struct UserCmd;

namespace EnginePrediction
{
    void run(UserCmd* cmd) noexcept;
    int getFlags() noexcept;
    Vector getVelocity() noexcept;
}
