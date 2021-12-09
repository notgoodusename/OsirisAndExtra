#include "Fakelag.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Localplayer.h"

void Fakelag::run(bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;

    auto chokedPackets = config->legitAntiAim.enabled ? 1 : 0;
    if (config->fakelag.enabled)
    {
        switch (config->fakelag.mode) {
        case 0: //Static
            chokedPackets = config->fakelag.limit;
            break;
        case 1: //Adaptive
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (localPlayer->velocity().length() * memory->globalVars->intervalPerTick))), 1, config->fakelag.limit);
            break;
        }
    }

    sendPacket = netChannel->chokedPackets >= chokedPackets;
}