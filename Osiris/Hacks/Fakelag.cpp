#include "EnginePrediction.h"
#include "Fakelag.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Localplayer.h"
#include "../SDK/Vector.h"

void Fakelag::run(bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;
    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;
    if (EnginePrediction::getVelocity().length2D() < 1)
        return;

    auto chokedPackets = config->legitAntiAim.enabled || config->fakeAngle.enabled ? 2 : 0;
    if (config->fakelag.enabled && (!config->doubletapkey.isActive() && !config->hideshotskey.isActive()))
    {
        const float speed = EnginePrediction::getVelocity().length2D() >= 15.0f ? EnginePrediction::getVelocity().length2D() : 0.0f;
        switch (config->fakelag.mode) {
        case 0: //Static
            chokedPackets = config->fakelag.limit;
            break;
        case 1: //Adaptive
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (speed * memory->globalVars->intervalPerTick))), 1, config->fakelag.limit);
            break;
        case 2: // Random
            srand(static_cast<unsigned int>(time(NULL)));
            chokedPackets = rand() % config->fakelag.limit + 1;
            break;
        }
    }

    sendPacket = netChannel->chokedPackets >= chokedPackets;
}