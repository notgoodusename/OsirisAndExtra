#pragma once

#include <string_view>

#include "../ConfigStructs.h"

#define THE_SOUND_OF_GOD() true

namespace Sound
{
    void modulateSound(std::string_view name, int entityIndex, float& volume) noexcept;

    // GUI
    void drawGUI() noexcept;

    // Config
    json toJson() noexcept;
    void fromJson(const json& j) noexcept;
    void resetConfig() noexcept;
}
