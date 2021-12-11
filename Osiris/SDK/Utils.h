#pragma once

#include <numbers>
#include <tuple>

#include "../SDK/GlobalVars.h"

#include "../Memory.h"

class matrix3x4;

static auto maxUserCmdProcessTicks = 16;
static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }
static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;

void resetMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector absAngle, Vector mins, Vector maxs) noexcept;