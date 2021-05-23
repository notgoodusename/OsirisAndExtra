#pragma once

#include <numbers>
#include <tuple>

#include "../SDK/GlobalVars.h"

#include "../Memory.h"

static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;