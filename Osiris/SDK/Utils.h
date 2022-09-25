#pragma once

#include <numbers>
#include <tuple>

#include "../SDK/GlobalVars.h"

#include "../Memory.h"

class matrix3x4;

static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }
static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;

void resetMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector absAngle, Vector mins, Vector maxs) noexcept;

int getMaxUserCmdProcessTicks() noexcept;

enum class GameMode
{
	None,
	Casual,
	Competitive,
	Wingman,
	WeaponsExpert,
	ArmsRace,
	Demolition,
	Deathmatch,
	Training,
	Custom,
	Guardian,
	CoopStrike,
	WarGames,
	DangerZone
};


GameMode getGameMode() noexcept;

#define maxUserCmdProcessTicks getMaxUserCmdProcessTicks()