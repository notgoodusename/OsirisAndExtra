#include "Aimbot.h"
#include "AntiAim.h"

#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"

#include "../Interfaces.h"

bool updateLby(bool update = false) noexcept
{
    static float timer = 0.f;
    static bool lastValue = false;
    if (!update)
        return lastValue;

    if (!(localPlayer->flags() & 1) || !localPlayer->getAnimstate())
    {
        lastValue = false;
        return false;
    }

    if (localPlayer->velocity().length2D() > 0.1f || fabsf(localPlayer->velocity().z) > 100.f)
        timer = memory->globalVars->serverTime() + 0.22f;

    if (timer < memory->globalVars->serverTime())
    {
        timer = memory->globalVars->serverTime() + 1.1f;
        lastValue = true;
        return true;
    }
    lastValue = false;
    return false;
}
bool invert = false;
bool autoDirection(Vector eyeAngle) noexcept
{
    constexpr float maxRange{ 8192.0f };

    Vector eye = eyeAngle;
    eye.x = 0.f;
    Vector eyeAnglesLeft45 = eye;
    Vector eyeAnglesRight45 = eye;
    eyeAnglesLeft45.y += 45.f;
    eyeAnglesRight45.y -= 45.f;


    eyeAnglesLeft45.toAngle();

    Vector viewAnglesLeft45 = {};
    viewAnglesLeft45 = viewAnglesLeft45.fromAngle(eyeAnglesLeft45) * maxRange;

    Vector viewAnglesRight45 = {};
    viewAnglesRight45 = viewAnglesRight45.fromAngle(eyeAnglesRight45) * maxRange;

    static Trace traceLeft45;
    static Trace traceRight45;

    Vector startPosition{ localPlayer->getEyePosition() };

    interfaces->engineTrace->traceRay({ startPosition, startPosition + viewAnglesLeft45 }, 0x4600400B, { localPlayer.get() }, traceLeft45);
    interfaces->engineTrace->traceRay({ startPosition, startPosition + viewAnglesRight45 }, 0x4600400B, { localPlayer.get() }, traceRight45);

    float distanceLeft45 = sqrtf(powf(startPosition.x - traceRight45.endpos.x, 2) + powf(startPosition.y - traceRight45.endpos.y, 2) + powf(startPosition.z - traceRight45.endpos.z, 2));
    float distanceRight45 = sqrtf(powf(startPosition.x - traceLeft45.endpos.x, 2) + powf(startPosition.y - traceLeft45.endpos.y, 2) + powf(startPosition.z - traceLeft45.endpos.z, 2));

    float mindistance = min(distanceLeft45, distanceRight45);

    if (distanceLeft45 == mindistance)
        return false;
    return true;
}
float RandomFloat(float a, float b ,float multiplier) {
    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    float result = a + r;
    return result * multiplier;
}
void AntiAim::rage(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (cmd->viewangles.x == currentViewAngles.x && config->rageAntiAim.enabled)
    {
        if (config->fakeAngle.enabled)
            if (!config->fakelag.enabled)
                sendPacket = cmd->tickCount % 2;
        if (!sendPacket && (cmd->buttons & UserCmd::IN_ATTACK))
            return;

        switch (config->rageAntiAim.pitch)
        {
        case 0: //None
            break;
        case 1: //Down
            cmd->viewangles.x = 89.f;
            break;
        case 2: //Zero
            cmd->viewangles.x = 0.f;
            break;
        case 3: //Up
            cmd->viewangles.x = -89.f;
            break;
        default:
            break;
        }
    }
    if (cmd->viewangles.y == currentViewAngles.y)
    {
        if (config->rageAntiAim.yawBase != 0 
            && config->rageAntiAim.enabled)   //AntiAim
        {
            float yaw = 0.f;
            static float staticYaw = 0.f;
            if (config->rageAntiAim.atTargets)
            {
                Vector localPlayerEyePosition = localPlayer->getEyePosition();
                const auto aimPunch = localPlayer->getAimPunch();
                float bestFov = 255.f;
                float yawAngle = 0.f;
                for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
                    const auto entity{ interfaces->entityList->getEntity(i) };
                    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                        || !entity->isOtherEnemy(localPlayer.get()) || entity->gunGameImmunity())
                        continue;

                    const auto angle{ Aimbot::calculateRelativeAngle(localPlayerEyePosition, entity->getAbsOrigin(), cmd->viewangles + aimPunch) };
                    const auto fov{ angle.length2D() };
                    if (fov < bestFov)
                    {
                        yawAngle = angle.y;
                        bestFov = fov;
                    }
                }
                yaw = yawAngle;
            }

            if (config->rageAntiAim.yawBase != 5)
                staticYaw = 0.f;
            bool isInvertToggled = config->fakeAngle.invert.isActive();
            switch (config->rageAntiAim.yawBase)
            {
            case 1: //Paranoia
            {
                yaw += (isInvertToggled ? -15 : +15) + 180.f;
                if (!autoDirection(cmd->viewangles))
                {
                    config->rageAntiAim.yawAdd = RandomFloat(0.f, 59.f, 1.f);
                    break;
                }
                else
                {
                    config->rageAntiAim.yawAdd = RandomFloat(-59.f, 0.f, 1.f);
                    break;
                }
            }
            case 2: //Back
                yaw += 180.f;
                break;
            case 3: //Right
                yaw += -90.f;
                break;
            case 4: //Left
                yaw += 90.f;
                break;
            case 5:
                staticYaw += static_cast<float>(config->rageAntiAim.spinBase);
                yaw += staticYaw;
                break;
            default:
                break;
            }
            yaw += static_cast<float>(config->rageAntiAim.yawAdd);
            cmd->viewangles.y += yaw;
        }
        if (config->fakeAngle.enabled) //Fakeangle
        {
            bool isInvertToggled = config->fakeAngle.invert.isActive();
            static bool invert = true;
            if (config->fakeAngle.peekMode != 3)
                invert = isInvertToggled;
            float leftDesyncAngle = config->fakeAngle.leftLimit * 2.f;
            float rightDesyncAngle = config->fakeAngle.rightLimit * -2.f;

            switch (config->fakeAngle.peekMode)
            {
            case 0:
                break;
            case 1: // Peek real
                if(!isInvertToggled)
                    invert = !autoDirection(cmd->viewangles);
                else
                    invert = autoDirection(cmd->viewangles);
                break;
            case 2: // Peek fake
                if (isInvertToggled)
                    invert = !autoDirection(cmd->viewangles);
                else
                    invert = autoDirection(cmd->viewangles);
                break;
            case 3: // Jitter
                if (sendPacket)
                    invert = !invert;
                break;
            default:
                break;
            }

            switch (config->fakeAngle.lbyMode)
            {
            case 0: // Normal(sidemove)
                if (fabsf(cmd->sidemove) < 5.0f)
                {
                    if (cmd->buttons & UserCmd::IN_DUCK)
                        cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
                    else
                        cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
                }
                break;
            case 1: // Opposite (Lby break)
                if (updateLby())
                {
                    float desyncangle = RandomFloat(45, leftDesyncAngle, 1.f);
                    float desyncangler = RandomFloat(45, rightDesyncAngle, 1.f);
                    cmd->viewangles.y += !invert ? desyncangle : desyncangler;
                    sendPacket = false;
                    return;
                }
                break;
            case 2: //Sway (flip every lby update)
                static bool flip = false;
                if (updateLby())
                {
                    cmd->viewangles.y += !flip ? leftDesyncAngle : rightDesyncAngle;
                    sendPacket = false;
                    flip = !flip;
                    return;
                }
                if (!sendPacket)
                    cmd->viewangles.y += flip ? leftDesyncAngle : rightDesyncAngle;
                break;
            }
            if (config->fakeAngle.experimental)
            {
                if (sendPacket)
                cmd->viewangles.z = invert ? 38 : -38;
                else
                cmd->viewangles.z = invert ? 45 : -45;
            }
            if (sendPacket)
                return;
            cmd->viewangles.y += invert ? leftDesyncAngle : rightDesyncAngle;
        }
    }
}

void AntiAim::legit(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (cmd->viewangles.y == currentViewAngles.y) 
    {
        invert = config->legitAntiAim.invert.isActive();
        float desyncAngle = localPlayer->getMaxDesyncAngle() * 2.f;
        if (updateLby() && config->legitAntiAim.extend)
        {
            cmd->viewangles.y += !invert ? desyncAngle : -desyncAngle;
            sendPacket = false;
            return;
        }

        if (fabsf(cmd->sidemove) < 5.0f && !config->legitAntiAim.extend)
        {
            if (cmd->buttons & UserCmd::IN_DUCK)
                cmd->sidemove = cmd->tickCount & 1 ? 3.25f : -3.25f;
            else
                cmd->sidemove = cmd->tickCount & 1 ? 1.1f : -1.1f;
        }

        if (sendPacket)
            return;

        cmd->viewangles.y += invert ? desyncAngle : -desyncAngle;
    }
}

void AntiAim::updateInput() noexcept
{
    config->legitAntiAim.invert.handleToggle();
    config->fakeAngle.invert.handleToggle();
}

void AntiAim::run(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (cmd->buttons & (UserCmd::IN_USE))
        return;

    if (localPlayer->moveType() == MoveType::LADDER || localPlayer->moveType() == MoveType::NOCLIP)
        return;

    if (config->legitAntiAim.enabled)
        AntiAim::legit(cmd, previousViewAngles, currentViewAngles, sendPacket);
    else if (config->rageAntiAim.enabled || config->fakeAngle.enabled)
        AntiAim::rage(cmd, previousViewAngles, currentViewAngles, sendPacket);
}

bool AntiAim::canRun(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;
    
    updateLby(true); //Update lby timer

    if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
        return false;

    if (localPlayer->flags() & (1 << 6))
        return false;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return true;

    if (activeWeapon->isThrowing())
        return false;

    if (activeWeapon->isGrenade())
        return true;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto() || localPlayer->waitForNoAttack())
        return true;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime())
        return true;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return true;

    if (activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return true;

    if (localPlayer->nextAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return false;

    if (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && (cmd->buttons & (UserCmd::IN_ATTACK)))
        return false;
    
    if (activeWeapon->isKnife())
    {
        if (activeWeapon->nextSecondaryAttack() <= memory->globalVars->serverTime() && cmd->buttons & (UserCmd::IN_ATTACK2))
            return false;
    }

    if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver && activeWeapon->readyTime() <= memory->globalVars->serverTime() && cmd->buttons & (UserCmd::IN_ATTACK | UserCmd::IN_ATTACK2))
        return false;

    const auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return true;

    return true;
}
