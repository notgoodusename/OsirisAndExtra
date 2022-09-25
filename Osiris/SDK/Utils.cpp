#include <cmath>

#include "../Memory.h"

#include "GlobalVars.h"
#include "Entity.h"
#include "matrix3x4.h"
#include "ModelInfo.h"
#include "Utils.h"
#include "Vector.h"

std::tuple<float, float, float> rainbowColor(float speed) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::make_tuple(std::sin(speed * memory->globalVars->realtime) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realtime + 2 * pi / 3) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realtime + 4 * pi / 3) * 0.5f + 0.5f);
}

void resetMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector absAngle, Vector mins, Vector maxs) noexcept
{
    memcpy(entity->getBoneCache().memory, boneCacheData, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
    memory->setAbsOrigin(entity, origin);
    memory->setAbsAngle(entity, Vector{ 0.f, absAngle.y, 0.f });
    entity->getCollideable()->setCollisionBounds(mins, maxs);
}

int getMaxUserCmdProcessTicks() noexcept
{
    if (const auto gameRules = (*memory->gameRules); gameRules)
        return (gameRules->isValveDS()) ? 8 : 16;
    return 16;
}

GameMode getGameMode() noexcept
{
    static auto gameType{ interfaces->cvar->findVar("game_type") };
    static auto gameMode{ interfaces->cvar->findVar("game_mode") };
    switch (gameType->getInt())
    {
    case 0:
        switch (gameMode->getInt())
        {
        case 0:
            return GameMode::Casual;
        case 1:
            return GameMode::Competitive;
        case 2:
            return GameMode::Wingman;
        case 3:
            return GameMode::WeaponsExpert;
        default:
            break;
        }
        break;
    case 1:
        switch (gameMode->getInt())
        {
        case 0:
            return GameMode::ArmsRace;
        case 1:
            return GameMode::Demolition;
        case 2:
            return GameMode::Deathmatch;
        default:
            break;
        }
        break;
    case 2:
        return GameMode::Training;
    case 3:
        return GameMode::Custom;
    case 4:
        switch (gameMode->getInt())
        {
        case 0:
            return GameMode::Guardian;
        case 1:
            return GameMode::CoopStrike;
        default:
            break;
        }
        break;
    case 5:
        return GameMode::WarGames;
    case 6:
        return GameMode::DangerZone;
    default:
        break;
    }
    return GameMode::None;
}