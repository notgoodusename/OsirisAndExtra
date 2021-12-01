#pragma once

#include "../ConfigStructs.h"

namespace Glow
{
    void render() noexcept;
    void clearCustomObjects() noexcept;
    void updateInput() noexcept;

    // GUI
    void drawGUI() noexcept;

    // Config
    json toJson() noexcept;
    void fromJson(const json& j) noexcept;
    void resetConfig() noexcept;
}
