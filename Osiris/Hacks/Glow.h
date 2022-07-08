#pragma once

#include "../ConfigStructs.h"

namespace Glow
{
    void render() noexcept;
    void clearCustomObjects() noexcept;
    void updateInput() noexcept;

    // Config
    void resetConfig() noexcept;
}
