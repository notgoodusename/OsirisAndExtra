#include "EnginePrediction.h"
#include "Fakelag.h"
#include "Tickbase.h"

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
    auto chokedPackets = config->legitAntiAim.enabled || config->fakeAngle.enabled ? 1 : 0;



    if (config->fakelag.enabled)
    {
        if (config->tickbase.DisabledTickbase && config->tickbase.onshotFl && config->tickbase.readyFire) {
            chokedPackets = 2;
            config->tickbase.readyFire = false;
            return;
        }
        if (config->tickbase.DisabledTickbase && config->tickbase.onshotFl && config->tickbase.lastFireShiftTick > memory->globalVars->tickCount) {
            chokedPackets = 0;
            sendPacket = true;
            return;
        }

        const float speed = EnginePrediction::getVelocity().length2D() >= 15.0f ? EnginePrediction::getVelocity().length2D() : 0.0f;
        switch (config->fakelag.mode) {
        case 0: //Static
            chokedPackets = config->fakelag.limit;
            break;
        case 1: //Adaptive
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (speed * memory->globalVars->intervalPerTick))), 1, config->fakelag.limit);
            break;
        case 2: // Random
            srand(static_cast<unsigned int>(time(nullptr)));
            chokedPackets = rand() % config->fakelag.limit + 1;
            break;
        case 3:
            int i;
            static auto frameRate = 1.0f;
            frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;
            srand(static_cast<unsigned int>(frameRate != 0.0f ? static_cast<int>(1 / frameRate) : 0));
            for (i = 1; i <= 30; ++i)
            {
                if (i == 29)
                    chokedPackets = 15;
                else
                {
                    if ((rand() % (360)) - 180 < 160)
                        chokedPackets = 2;
                    else {
                        chokedPackets = 1;
                        //sendPacket = true;
                    }
                }
            }
            i = 0;
            break;
        }
    }

    chokedPackets = std::clamp(chokedPackets, 0, maxUserCmdProcessTicks - Tickbase::getTargetTickShift());
    sendPacket = netChannel->chokedPackets >= chokedPackets;
}