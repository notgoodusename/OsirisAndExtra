#pragma once

#include <numbers>
#include <tuple>

#include "../SDK/GlobalVars.h"

#include "../Memory.h"

static auto maxUserCmdProcessTicks = 16;
static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }
static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;