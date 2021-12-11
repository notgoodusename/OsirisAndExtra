#pragma once

struct UserCmd;
struct Vector;

namespace EnginePrediction
{
    void run(UserCmd* cmd) noexcept;
    int getFlags() noexcept;
    Vector getVelocity() noexcept;
}
