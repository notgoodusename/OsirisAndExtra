#include "AimbotFunctions.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Knifebot.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Entity.h"

void knifeBotRage(UserCmd* cmd) noexcept
{
    if (!config->misc.knifeBot)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->isKnife())
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return;

    const auto localPlayerOrigin = localPlayer->origin();
    const auto localPlayerEyePosition = localPlayer->getEyePosition();
    const auto aimPunch = localPlayer->getAimPunch();

    auto bestDistance{ FLT_MAX };
    Entity* bestTarget{ };
    float bestSimulationTime = 0;
    Vector absAngle{ };
    Vector origin{ };
    Vector bestTargetPosition{ };

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
            || !entity->isOtherEnemy(localPlayer.get()) || entity->gunGameImmunity())
            continue;

        const auto player = Animations::getPlayer(i);
        if (!player.gotMatrix)
            continue;

        auto distance{ localPlayerOrigin.distTo(player.origin) };

        if (distance < bestDistance) {
            bestDistance = distance;
            bestTarget = entity;
            absAngle = player.absAngle;
            origin = player.origin;
            bestSimulationTime = player.simulationTime;
            bestTargetPosition = player.matrix[7].origin();
        }

        if (!config->backtrack.enabled)
            continue;

        const auto records = Animations::getBacktrackRecords(entity->index());
        if (!records || records->empty())
            continue;

        int lastTick = -1;

        for (int i = static_cast<int>(records->size() - 1); i >= 0; i--)
        {
            if (Backtrack::valid(records->at(i).simulationTime))
            {
                lastTick = i;
                break;
            }
        }

        if (lastTick <= -1)
            continue;

        const auto record = records->at(lastTick);

        distance = localPlayerOrigin.distTo(record.origin);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestTarget = entity;
            absAngle = record.absAngle;
            origin = record.origin;
            bestSimulationTime = record.simulationTime;
            bestTargetPosition = record.matrix[7].origin();
        }
    }

    if (!bestTarget || bestDistance > 75.0f)
        return;

    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bestTarget->getBonePosition(7), cmd->viewangles + aimPunch) };
    const bool firstSwing = (localPlayer->nextPrimaryAttack() + 0.4) < memory->globalVars->serverTime();

    bool backStab = false;

    Vector targetForward = Vector::fromAngle(absAngle);
    targetForward.z = 0;

    Vector vecLOS = (origin - localPlayer->origin());
    vecLOS.z = 0;
    vecLOS.normalize();

    float dot = vecLOS.dotProduct(targetForward);

    if (dot > 0.475f)
        backStab = true;

    auto hp = bestTarget->health();
    auto armor = bestTarget->armor() > 1;

    int minDmgSol = 40;
    int minDmgSag = 65;

    if (backStab)
    {
        minDmgSag = 180;
        minDmgSol = 90;
    }
    else if (!firstSwing)
        minDmgSol = 25;

    if (armor)
    {
        minDmgSag = static_cast<int>((float)minDmgSag * 0.85f);
        minDmgSol = static_cast<int>((float)minDmgSol * 0.85f);
    }

    if (hp <= minDmgSag && bestDistance <= 60)
        cmd->buttons |= UserCmd::IN_ATTACK2;
    else if (hp <= minDmgSol)
        cmd->buttons |= UserCmd::IN_ATTACK;
    else
        cmd->buttons |= UserCmd::IN_ATTACK;

    cmd->viewangles += angle;
    cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());
}

void knifeTrigger(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->isKnife())
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return;

    Vector startPos = localPlayer->getEyePosition();
    Vector endPos = startPos + Vector::fromAngle(cmd->viewangles) * 48.f;

    bool didHitShort = false;

    Trace trace;
    interfaces->engineTrace->traceRay({ startPos, endPos }, 0x200400B, { localPlayer.get() }, trace);

    if (trace.fraction >= 1.0f)
        interfaces->engineTrace->traceRay({ startPos, endPos, Vector{ -16, -16, -18 },  Vector { 16, 16, 18 } }, 0x200400B, { localPlayer.get() }, trace);


    endPos = startPos + Vector::fromAngle(cmd->viewangles) * 32.f;
    Trace tr;
    interfaces->engineTrace->traceRay({ startPos, endPos }, 0x200400B, { localPlayer.get() }, tr);

    if (tr.fraction >= 1.0f)
        interfaces->engineTrace->traceRay({ startPos, endPos, Vector{ -16, -16, -18 },  Vector { 16, 16, 18 } }, 0x200400B, { localPlayer.get() }, tr);

    didHitShort = tr.fraction < 1.0f;

    if (trace.fraction >= 1.0f)
        return;

    if (!trace.entity || !trace.entity->isPlayer())
        return;

    if (!trace.entity->isOtherEnemy(localPlayer.get()))
        return;

    if (trace.entity->gunGameImmunity())
        return;

    const auto player = Animations::getPlayer(trace.entity->index());
    if (!player.gotMatrix)
        return;

    const bool firstSwing = (localPlayer->nextPrimaryAttack() + 0.4) < memory->globalVars->serverTime();

    bool backStab = false;

    Vector targetForward = Vector::fromAngle(player.absAngle);
    targetForward.z = 0;

    Vector vecLOS = (player.origin - localPlayer->origin());
    vecLOS.z = 0;
    vecLOS.normalize();

    float dot = vecLOS.dotProduct(targetForward);

    if (dot > 0.475f)
        backStab = true;

    auto hp = trace.entity->health();
    auto armor = trace.entity->armor() > 1;

    int minDmgSol = 40;
    int minDmgSag = 65;

    if (backStab)
    {
        minDmgSag = 180;
        minDmgSol = 90;
    }
    else if (!firstSwing)
        minDmgSol = 25;

    if (armor)
    {
        minDmgSag = static_cast<int>((float)minDmgSag * 0.85f);
        minDmgSol = static_cast<int>((float)minDmgSol * 0.85f);
    }

    if (hp <= minDmgSag && didHitShort)
        cmd->buttons |= UserCmd::IN_ATTACK2;
    else if (hp <= minDmgSol)
        cmd->buttons |= UserCmd::IN_ATTACK;
    else
        cmd->buttons |= UserCmd::IN_ATTACK;

    cmd->tickCount = timeToTicks(player.simulationTime + Backtrack::getLerp());
}

void Knifebot::run(UserCmd* cmd) noexcept
{
    if (!config->misc.knifeBot)
        return;

    switch (config->misc.knifeBotMode)
    {
    case 0:
        knifeTrigger(cmd);
        break;
    case 1:
        knifeBotRage(cmd);
        break;
    default:
        break;
    }
}