#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "AimbotFunctions.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Ragebot.h"
#include "EnginePrediction.h"
#include "Resolver.h"

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
    config->minDamageOverrideKey.handleToggle();
}

void runRagebot(UserCmd* cmd, Entity* entity, matrix3x4* matrix, Ragebot::Enemies target, std::array<bool, Hitboxes::Max> hitbox, Entity* activeWeapon, int weaponIndex, Vector localPlayerEyePosition, Vector aimPunch, int multiPoint, int minDamage, float& damageDiff, Vector& bestAngle, Vector& bestTarget) noexcept
{
    const auto& cfg = config->ragebot;

    damageDiff = FLT_MAX;

    const Model* model = entity->getModel();
    if (!model)
        return;

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
    if (!hdr)
        return;

    StudioHitboxSet* set = hdr->getHitboxSet(0);
    if (!set)
        return;

    for (size_t i = 0; i < hitbox.size(); i++)
    {
        if (!hitbox[i])
            continue;

        StudioBbox* hitbox = set->getHitbox(i);
        if (!hitbox)
            continue;

        for (auto& bonePosition : AimbotFunction::multiPoint(entity, matrix, hitbox, localPlayerEyePosition, i, multiPoint))
        {
            const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch) };
            const auto fov{ angle.length2D() };
            if (fov > cfg[weaponIndex].fov)
                continue;

            if (!cfg[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, bonePosition, 1))
                continue;

            float damage = AimbotFunction::getScanDamage(entity, bonePosition, activeWeapon->getWeaponData(), minDamage, cfg[weaponIndex].friendlyFire);
            damage = std::clamp(damage, 0.0f, (float)entity->maxHealth());
            if (damage <= 0.f)
                continue;

            if (!entity->isVisible(bonePosition) && (cfg[weaponIndex].visibleOnly || !damage))
                continue;

            if (cfg[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !activeWeapon->zoomLevel() && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
                cmd->buttons |= UserCmd::IN_ZOOM;

            if (cfg[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                return;

            if (cfg[weaponIndex].autoStop && localPlayer->flags() & 1 && !(cmd->buttons & UserCmd::IN_JUMP))
            {
                const auto velocity = EnginePrediction::getVelocity();
                const auto speed = velocity.length2D();
                const auto activeWeapon = localPlayer->getActiveWeapon();
                const auto weaponData = activeWeapon->getWeaponData();
                const float maxSpeed = (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;
                if (speed >= maxSpeed)
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
            }
        }
    }

    if (bestTarget.notNull())
    {
        if (!AimbotFunction::hitChance(localPlayer.get(), entity, set, matrix, activeWeapon, bestAngle, cmd, cfg[weaponIndex].hitChance))
        {
            bestTarget = Vector{ };
            bestAngle = Vector{ };
            damageDiff = FLT_MAX;
        }
    }
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
    int bestIndex{ -1 };
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

        const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, player.matrix[8].origin(), cmd->viewangles + aimPunch) };
        const auto origin{ entity->getAbsOrigin() };
        const auto fov{ angle.length2D() }; //fov
        const auto health{ entity->health() }; //health
        const auto distance{ localPlayerOrigin.distTo(origin) }; //distance
        enemies.emplace_back(i, health, distance, fov);
    }

    if (enemies.empty())
        return;

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

    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    auto multiPoint = cfg[weaponIndex].multiPoint;
    if (cfg[weaponIndex].disableMultipointIfLowFPS && static_cast<int>(1 / frameRate) <= 1 / memory->globalVars->intervalPerTick)
        multiPoint = 0;

    for (const auto& target : enemies) 
    {
        auto entity{ interfaces->entityList->getEntity(target.id) };
        auto player = Animations::getPlayer(target.id);
        int minDamage = std::clamp(std::clamp(config->minDamageOverrideKey.isActive() ? cfg[weaponIndex].minDamageOverride : cfg[weaponIndex].minDamage, 0, target.health), 0, activeWeapon->getWeaponData()->damage);

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupMins = entity->getCollideable()->obbMins();
        Vector backupMaxs = entity->getCollideable()->obbMaxs();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        for (int cycle = 0; cycle < 2; cycle++)
        {
            float currentSimulationTime = -1.0f;

            if (config->backtrack.enabled)
            {
                const auto records = Animations::getBacktrackRecords(entity->index());
                if (!records || records->empty())
                    continue;

                int bestTick = -1;
                if (cycle == 0)
                {
                    for (size_t i = 0; i < records->size(); i++)
                    {
                        if (Backtrack::valid(records->at(i).simulationTime))
                        {
                            bestTick = static_cast<int>(i);
                            break;
                        }
                    }
                }
                else
                {
                    for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
                    {
                        if (Backtrack::valid(records->at(i).simulationTime))
                        {
                            bestTick = i;
                            break;
                        }
                    }
                }

                if (bestTick <= -1)
                    continue;

                memcpy(entity->getBoneCache().memory, records->at(bestTick).matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, records->at(bestTick).origin);
                memory->setAbsAngle(entity, Vector{ 0.f, records->at(bestTick).absAngle.y, 0.f });
                memory->setCollisionBounds(entity->getCollideable(), records->at(bestTick).mins, records->at(bestTick).maxs);

                currentSimulationTime = records->at(bestTick).simulationTime;
            }
            else
            {
                //We skip backtrack
                if (cycle == 1)
                    continue;

                memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, player.origin);
                memory->setAbsAngle(entity, Vector{ 0.f, player.absAngle.y, 0.f });
                memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

                currentSimulationTime = player.simulationTime;
            }

            runRagebot(cmd, entity, entity->getBoneCache().memory, target, hitbox, activeWeapon, weaponIndex, localPlayerEyePosition, aimPunch, multiPoint, minDamage, damageDiff, bestAngle, bestTarget);
            resetMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            if (bestTarget.notNull())
            {
                bestSimulationTime = currentSimulationTime;
                bestIndex = target.id;
                break;
            }
        }
        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull()) 
    {
        static Vector lastAngles{ cmd->viewangles };
        static int lastCommand{ };

        if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && cfg[weaponIndex].silent)
            cmd->viewangles = lastAngles;

        auto angle = AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles + aimPunch);
        bool clamped{ false };

        if (std::abs(angle.x) > config->misc.maxAngleDelta || std::abs(angle.y) > config->misc.maxAngleDelta) {
            angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
            angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
            clamped = true;
        }

        if (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())
        {
            cmd->viewangles += angle;
            if (!cfg[weaponIndex].silent)
                interfaces->engine->setViewAngles(cmd->viewangles);

            if (cfg[weaponIndex].autoShot && !clamped)
                cmd->buttons |= UserCmd::IN_ATTACK;
        }

        if (clamped)
            cmd->buttons &= ~UserCmd::IN_ATTACK;

        if (cmd->buttons & UserCmd::IN_ATTACK)
        {
            cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());
            Resolver::saveRecord(bestIndex, bestSimulationTime);
        }

        if (clamped) lastAngles = cmd->viewangles;
        else lastAngles = Vector{ };

        lastCommand = cmd->commandNumber;
    }
}
