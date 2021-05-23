#pragma once

struct UserCmd;

namespace Legitbot
{
    void run(UserCmd*) noexcept;
    void updateInput() noexcept;
}