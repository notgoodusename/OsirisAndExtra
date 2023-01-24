#include "../Interfaces.h"

#include "AimbotFunctions.h"
#include "AntiAim.h"

#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"
int spinstopat = 0;
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

void AntiAim::rage(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (cmd->viewangles.x == currentViewAngles.x && config->rageAntiAim.enabled)
    {
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
        if (config->rageAntiAim.yawBase != Yaw::off
            && config->rageAntiAim.enabled)   //AntiAim
        {
            float yaw = 0.f;
            static float staticYaw = 0.f;
            static bool flipJitter = false;
            if (sendPacket)
                flipJitter ^= 1;
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

                    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, entity->getAbsOrigin(), cmd->viewangles + aimPunch) };
                    const auto fov{ angle.length2D() };
                    if (fov < bestFov)
                    {
                        yawAngle = angle.y;
                        bestFov = fov;
                    }
                }
                yaw = yawAngle;
            }

            if (config->rageAntiAim.yawBase != Yaw::spin)
                staticYaw = 1.f;

            switch (config->rageAntiAim.yawBase)
            {
            case Yaw::forward:
                yaw += 0.f;
                break;
            case Yaw::backward:
                yaw += 180.f;
                break;
            case Yaw::right:
                yaw += -90.f;
                break;
            case Yaw::left:
                yaw += 90.f;
                break;
            case Yaw::spin:
                if (memory->globalVars->tickCount < spinstopat){
                    staticYaw = staticYaw++ * 1.05f;
                    yaw += staticYaw;
                }
                else{
                    staticYaw = 1.f;
                    yaw = staticYaw;
                    spinstopat = memory->globalVars->tickCount + config->rageAntiAim.spinBase;
                }
                
                break;
            default:
                break;
            }

            const bool forward = config->rageAntiAim.manualForward.isActive();
            const bool back = config->rageAntiAim.manualBackward.isActive();
            const bool right = config->rageAntiAim.manualRight.isActive();
            const bool left = config->rageAntiAim.manualLeft.isActive();
            const bool isManualSet = forward || back || right || left;

            if (back) {
                yaw = -180.f;
                if (left)
                    yaw -= 45.f;
                else if (right)
                    yaw += 45.f;
            }
            else if (left) {
                yaw = 90.f;
                if (back)
                    yaw += 45.f;
                else if (forward)
                    yaw -= 45.f;
            }
            else if (right) {
                yaw = -90.f;
                if (back)
                    yaw -= 45.f;
                else if (forward)
                    yaw += 45.f;
            }
            else if(forward) {
                yaw = 0.f;
            }
            memory->randomSeed(memory->globalVars->tickCount);
            switch (config->rageAntiAim.yawModifier)
            {
            case 1: //Jitter
                yaw -= flipJitter ? (memory->randomFloat(config->rageAntiAim.jitterMin, config->rageAntiAim.jitterRange)) : -(memory->randomFloat(config->rageAntiAim.jitterMin, config->rageAntiAim.jitterRange));
                break;
            default:
                break;
            }

            if (!isManualSet)
                yaw += static_cast<float>(config->rageAntiAim.yawAdd);
            cmd->viewangles.y += yaw;
        }

        if (config->fakeAngle.enabled) //Fakeangle
        {
            bool isInvertToggled = config->fakeAngle.invert.isActive();
            static bool invert = true;
            if (config->fakeAngle.peekMode != 3)
                invert = isInvertToggled;
            float rollOffsetAngle = config->rageAntiAim.rollOffset * 2.f;
            if (config->rageAntiAim.roll && (std::abs(config->rageAntiAim.rollAdd)+ std::abs(config->rageAntiAim.rollOffset) < 5 || !config->rageAntiAim.rollAlt || !(cmd->buttons & UserCmd::IN_JUMP || localPlayer->velocity().length2D() > 50.f))){
                cmd->viewangles.z = invert ? config->rageAntiAim.rollAdd + config->rageAntiAim.rollPitch : (config->rageAntiAim.rollAdd + config->rageAntiAim.rollPitch)* -1.f;
                cmd->viewangles.x = config->rageAntiAim.rollPitch;
                //extend_antiaim(cmd);
                //shit hill
            }
            else
                cmd->viewangles.z = 0.f;
            if (const auto gameRules = (*memory->gameRules); gameRules)
                if (getGameMode() != GameMode::Competitive && gameRules->isValveDS())
                    return;
            if (config->tickbase.DisabledTickbase && config->tickbase.onshotFl && config->tickbase.lastFireShiftTick > memory->globalVars->tickCount)
                return;
            memory->randomSeed(memory->globalVars->tickCount);
            float leftDesyncAngle = memory->randomFloat(config->fakeAngle.leftMin, config->fakeAngle.leftLimit)* 2.f;
            float rightDesyncAngle = memory->randomFloat(config->fakeAngle.rightMin, config->fakeAngle.rightLimit)* -2.f;
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
                    cmd->viewangles.y += !invert ? leftDesyncAngle : rightDesyncAngle;
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
            if (sendPacket)
                return;
            cmd->viewangles.y += invert ? leftDesyncAngle : rightDesyncAngle;
            if (config->rageAntiAim.roll && (std::abs(config->rageAntiAim.rollAdd) + std::abs(config->rageAntiAim.rollOffset) < 5 || !config->rageAntiAim.rollAlt || !(cmd->buttons & UserCmd::IN_JUMP || localPlayer->velocity().length2D() > 50.f)))
                cmd->viewangles.z += invert ? rollOffsetAngle : rollOffsetAngle * -1.f;
        }
    }
}

void AntiAim::legit(UserCmd* cmd, const Vector& previousViewAngles, const Vector& currentViewAngles, bool& sendPacket) noexcept
{
    if (cmd->viewangles.y == currentViewAngles.y) 
    {
        bool invert = config->legitAntiAim.invert.isActive();
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

    config->rageAntiAim.manualForward.handleToggle();
    config->rageAntiAim.manualBackward.handleToggle();
    config->rageAntiAim.manualRight.handleToggle();
    config->rageAntiAim.manualLeft.handleToggle();
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

    if (config->disableInFreezetime && (!*memory->gameRules || (*memory->gameRules)->freezePeriod()))
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
int* cur;
bool AntiAim::extend_antiaim(UserCmd* cmd){
    int const insertion_counter = min(14 + 5, 61);
    static UserCmd* lastCmd;
    int* command = &cmd->commandNumber;
    static int lastCommand{ };

    if (lastCommand == cmd->commandNumber - 1)
    {
    begin:
        //cmd = (UserCmd*)(cmd->commandNumber, ++(*command));

        ++(*cur);

        //copy the latest command.
        //memcpy(cmd, lastCmd, sizeof(struct UserCmd));

        //cmd->commandNumber = *command;//idk if it made a shit hill

        //only set the viewangle in the last command.
        if (*cur >= insertion_counter)
        {
            memcpy(&cmd->viewangles.x, (void*)(config->rageAntiAim.rollPitch), sizeof(float) * 3);
            //cmd_set_move(cmd, movement_backup);
            return true;
        }
        else 
            goto begin;
    }
    lastCommand = cmd->commandNumber;
    return false;
}