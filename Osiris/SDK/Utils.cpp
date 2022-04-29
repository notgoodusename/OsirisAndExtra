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