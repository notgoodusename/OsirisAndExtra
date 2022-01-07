#include "Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Ragebot.h"
#include "EnginePrediction.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Utils.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/ModelInfo.h"

static bool keyPressed = false;

void Ragebot::updateInput() noexcept
{
    config->ragebotKey.handleToggle();
}

void Ragebot::run(UserCmd* cmd) noexcept
{
    const auto& cfg = config->ragebot;

    if (!config->ragebotKey.isActive())
        return;

    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!cfg[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!cfg[weaponIndex].enabled)
        weaponIndex = 0;

    if (!cfg[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!cfg[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

    if (!(cfg[weaponIndex].enabled && (cmd->buttons & UserCmd::IN_ATTACK || cfg[weaponIndex].autoShot || cfg[weaponIndex].aimlock)))
        return;

    float damageDiff = FLT_MAX;
    Vector bestTarget{ };
    Vector bestAngle{ };
    float bestSimulationTime = 0;
    const auto localPlayerEyePosition = localPlayer->getEyePosition();
    const auto aimPunch = localPlayer->getAimPunch();

    std::array<bool, Hitboxes::Max> hitbox{ false };

    // Head
    hitbox[Hitboxes::Head] = (cfg[weaponIndex].hitboxes & 1 << 0) == 1 << 0;
    // Chest
    hitbox[Hitboxes::UpperChest] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Thorax] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::LowerChest] = (cfg[weaponIndex].hitboxes & 1 << 1) == 1 << 1;
    //Stomach
    hitbox[Hitboxes::Belly] = (cfg[weaponIndex].hitboxes & 1 << 2) == 1 << 2;
    hitbox[Hitboxes::Pelvis] = (cfg[weaponIndex].hitboxes & 1 << 2) == 1 << 2;
    //Arms
    hitbox[Hitboxes::RightUpperArm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
    hitbox[Hitboxes::RightForearm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
    hitbox[Hitboxes::LeftUpperArm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
    hitbox[Hitboxes::LeftForearm] = (cfg[weaponIndex].hitboxes & 1 << 3) == 1 << 3;
    //Legs
    hitbox[Hitboxes::RightCalf] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
    hitbox[Hitboxes::RightThigh] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
    hitbox[Hitboxes::LeftCalf] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;
    hitbox[Hitboxes::LeftThigh] = (cfg[weaponIndex].hitboxes & 1 << 4) == 1 << 4;


    std::vector<Ragebot::Enemies> enemies;
    const auto localPlayerOrigin{ localPlayer->getAbsOrigin() };
    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto player = Animations::getPlayer(i);
        if (!player.gotMatrix)
            continue;

        const auto entity{ interfaces->entityList->getEntity(i) };
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
            || !entity->isOtherEnemy(localPlayer.get()) && !cfg[weaponIndex].friendlyFire || entity->gunGameImmunity())
            continue;

        const auto angle{ Aimbot::calculateRelativeAngle(localPlayerEyePosition, player.matrix[8].origin(), cmd->viewangles + aimPunch) };
        const auto origin{ entity->getAbsOrigin() };
        const auto fov{ angle.length2D() }; //fov
        const auto health{ entity->health() }; //health
        const auto distance{ localPlayerOrigin.distTo(origin) }; //distance
        enemies.emplace_back(i, health, distance, fov);
    }

    switch (cfg[weaponIndex].priority)
    {
    case 0:
        std::sort(enemies.begin(), enemies.end(), healthSort);
        break;
    case 1:
        std::sort(enemies.begin(), enemies.end(), distanceSort);
        break;
    case 2:
        std::sort(enemies.begin(), enemies.end(), fovSort);
        break;
    default:
        break;
    }

    for (const auto& target : enemies) 
    {
        const auto entity{ interfaces->entityList->getEntity(target.id) };
        const auto player = Animations::getPlayer(target.id);
        const int minDamage = std::clamp(std::clamp(cfg[weaponIndex].minDamage, 0, target.health), 0, activeWeapon->getWeaponData()->damage);
        damageDiff = FLT_MAX;

        auto backupBoneCache = entity->getBoneCache().memory;
        auto backupMins = entity->getCollideable()->obbMins();
        auto backupMaxs = entity->getCollideable()->obbMaxs();
        auto backupOrigin = entity->getAbsOrigin();
        auto backupAbsAngle = entity->getAbsAngle();

        memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
        memory->setAbsOrigin(entity, player.origin);
        memory->setAbsAngle(entity, Vector{ 0.f, player.absAngle.y, 0.f });
        entity->getCollideable()->setCollisionBounds(player.mins, player.maxs);

        const Model* model = entity->getModel();
        if (!model)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
        if (!hdr)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        StudioHitboxSet* set = hdr->getHitboxSet(0);
        if (!set)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        for (size_t i = 0; i < hitbox.size(); i++)
        {
            if (!hitbox[i])
                continue;

            StudioBbox* hitbox = set->getHitbox(i);
            if (!hitbox)
                continue;

            for (auto &bonePosition : Aimbot::multiPoint(entity, player.matrix.data(), hitbox, localPlayerEyePosition, i, cfg[weaponIndex].multiPoint))
            {
                const auto angle{ Aimbot::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch) };
                const auto fov{ angle.length2D() };
                if (fov > cfg[weaponIndex].fov)
                    continue;

                if (!cfg[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, bonePosition, 1))
                    continue;

                float damage = Aimbot::getScanDamage(entity, bonePosition, activeWeapon->getWeaponData(), minDamage, cfg[weaponIndex].friendlyFire);
                damage = std::clamp(damage, 0.0f, (float)entity->maxHealth());
                if (damage <= 0.f)
                    continue;

                if (!entity->isVisible(bonePosition) && (cfg[weaponIndex].visibleOnly || !damage))
                    continue;

                if (cfg[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !activeWeapon->zoomLevel() && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
                    cmd->buttons |= UserCmd::IN_ZOOM;

                if (cfg[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                {
                    resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);;
                    return;
                }

                if (cfg[weaponIndex].autoStop && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
                {
                    const auto velocity = EnginePrediction::getVelocity();
                    const auto speed = velocity.length2D();
                    if (speed >= 15.0f)
                    {
                        Vector direction = velocity.toAngle();
                        direction.y = cmd->viewangles.y - direction.y;

                        const auto negatedDirection = Vector::fromAngle(direction) * -speed;
                        cmd->forwardmove = negatedDirection.x;
                        cmd->sidemove = negatedDirection.y;
                    }
                }

                if (std::fabsf((float)target.health - damage) <= damageDiff)
                {
                    bestAngle = angle;
                    damageDiff = std::fabsf((float)target.health - damage);
                    bestTarget = bonePosition;
                    bestSimulationTime = entity->simulationTime();
                }
            }
        }

        if (bestTarget.notNull())
        {
            if (!Aimbot::hitChance(localPlayer.get(), entity, set, player.matrix.data(), activeWeapon, bestAngle, cmd, cfg[weaponIndex].hitChance))
            {
                bestTarget = Vector{ };
                bestAngle = Vector{ };
                bestSimulationTime = 0;
                damageDiff = FLT_MAX;
            }
        }

        resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);

        if (bestTarget.notNull())
            break;

        if (!config->backtrack.enabled)
            continue;

        const auto records = Backtrack::getRecords(entity->index());
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

        backupBoneCache = entity->getBoneCache().memory;
        backupMins = entity->getCollideable()->obbMins();
        backupMaxs = entity->getCollideable()->obbMaxs();
        backupOrigin = entity->getAbsOrigin();
        backupAbsAngle = entity->getAbsAngle();

        memcpy(entity->getBoneCache().memory, record.matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
        memory->setAbsOrigin(entity, record.origin);
        memory->setAbsAngle(entity, Vector{ 0.f, record.absAngle.y, 0.f });
        entity->getCollideable()->setCollisionBounds(record.mins, record.maxs);

        model = entity->getModel();
        if (!model)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        hdr = interfaces->modelInfo->getStudioModel(model);
        if (!hdr)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        set = hdr->getHitboxSet(0);
        if (!set)
        {
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            continue;
        }

        for (size_t i = 0; i < hitbox.size(); i++)
        {
            if (!hitbox[i])
                continue;

            StudioBbox* hitbox = set->getHitbox(i);
            if (!hitbox)
                continue;

            for (auto& bonePosition : Aimbot::multiPoint(entity, record.matrix, hitbox, localPlayerEyePosition, i, cfg[weaponIndex].multiPoint))
            {
                const auto angle{ Aimbot::calculateRelativeAngle(localPlayerEyePosition, bonePosition, (cmd->viewangles + aimPunch)) };
                const auto fov{ angle.length2D() };
                if (fov > cfg[weaponIndex].fov)
                    continue;

                if (!cfg[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, bonePosition, 1))
                    continue;

                float damage = Aimbot::getScanDamage(entity, bonePosition, activeWeapon->getWeaponData(), minDamage, cfg[weaponIndex].friendlyFire);
                damage = std::clamp(damage, 0.0f, (float)entity->maxHealth());
                if (damage <= 0.f)
                    continue;

                if (!entity->isVisible(bonePosition) && (cfg[weaponIndex].visibleOnly || !damage))
                    continue;

                if (cfg[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
                    cmd->buttons |= UserCmd::IN_ATTACK2;

                if (cfg[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                {
                    resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
                    return;
                }

                if (cfg[weaponIndex].autoStop && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
                {
                    const auto velocity = EnginePrediction::getVelocity();
                    const auto speed = velocity.length2D();
                    if (speed >= 15.0f)
                    {
                        Vector direction = velocity.toAngle();
                        direction.y = cmd->viewangles.y - direction.y;

                        const auto negatedDirection = Vector::fromAngle(direction) * -speed;
                        cmd->forwardmove = negatedDirection.x;
                        cmd->sidemove = negatedDirection.y;
                    }
                
                }

                if (std::fabsf((float)target.health - damage) <= damageDiff)
                {
                    bestAngle = angle;
                    damageDiff = std::fabsf((float)target.health - damage);
                    bestTarget = bonePosition;
                    bestSimulationTime = record.simulationTime;
                }
            }
        }

        if (bestTarget.notNull())
        {
            if (!Aimbot::hitChance(localPlayer.get(), entity, set, record.matrix, activeWeapon, bestAngle, cmd, cfg[weaponIndex].hitChance))
            {
                bestTarget = Vector{ };
                bestAngle = Vector{ };
                bestSimulationTime = 0;
                damageDiff = FLT_MAX;
            }
        }

        resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull()) 
    {
        static Vector lastAngles{ cmd->viewangles };
        static int lastCommand{ };

        if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && cfg[weaponIndex].silent)
            cmd->viewangles = lastAngles;

        auto angle = Aimbot::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles + aimPunch);
        bool clamped{ false };

        if (std::abs(angle.x) > config->misc.maxAngleDelta || std::abs(angle.y) > config->misc.maxAngleDelta) {
            angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
            angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
            clamped = true;
        }

        cmd->viewangles += angle;
        if (!cfg[weaponIndex].silent)
            interfaces->engine->setViewAngles(cmd->viewangles);

        if (cfg[weaponIndex].autoShot && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && !clamped)
            cmd->buttons |= UserCmd::IN_ATTACK;

        if (clamped)
            cmd->buttons &= ~UserCmd::IN_ATTACK;

        if(cmd->buttons & UserCmd::IN_ATTACK)
            cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());

        if (clamped) lastAngles = cmd->viewangles;
        else lastAngles = Vector{ };

        lastCommand = cmd->commandNumber;
    }
}