#include "AimbotFunctions.h"
#include "Animations.h"
#include "Legitbot.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/ModelInfo.h"

void Legitbot::updateInput() noexcept
{
    config->legitbotKey.handleToggle();
}

void Legitbot::run(UserCmd* cmd) noexcept
{
    if (!config->legitbotKey.isActive())
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

    const auto& cfg = config->legitbot;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!cfg[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!cfg[weaponIndex].enabled)
        weaponIndex = 0;

    const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };

    if (config->recoilControlSystem.enabled && (config->recoilControlSystem.horizontal || config->recoilControlSystem.vertical) && aimPunch.notNull())
    {
        static Vector lastAimPunch{ };
        if (localPlayer->shotsFired() > config->recoilControlSystem.shotsFired)
        {
            if (cmd->buttons & UserCmd::IN_ATTACK)
            {
                Vector currentPunch = aimPunch;

                currentPunch.x *= config->recoilControlSystem.vertical;
                currentPunch.y *= config->recoilControlSystem.horizontal;

                if (!config->recoilControlSystem.silent)
                {
                    cmd->viewangles.y += lastAimPunch.y - currentPunch.y;
                    cmd->viewangles.x += lastAimPunch.x - currentPunch.x;
                    lastAimPunch.y = currentPunch.y;
                    lastAimPunch.x = currentPunch.x;
                }
                else
                {
                    cmd->viewangles.y -= currentPunch.y;
                    cmd->viewangles.x -= currentPunch.x;
                    lastAimPunch = Vector{ };
                }
            }
        }
        else
        {
            lastAimPunch = Vector{ };
        }

        if (!config->recoilControlSystem.silent)
            interfaces->engine->setViewAngles(cmd->viewangles);
    }

    if (!cfg[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!cfg[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

    if (cfg[weaponIndex].enabled && (cmd->buttons & UserCmd::IN_ATTACK || cfg[weaponIndex].aimlock)) {

        auto bestFov = cfg[weaponIndex].fov;
        Vector bestTarget{ };
        const auto localPlayerEyePosition = localPlayer->getEyePosition();

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

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            auto entity = interfaces->entityList->getEntity(i);
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                || !entity->isOtherEnemy(localPlayer.get()) && !cfg[weaponIndex].friendlyFire || entity->gunGameImmunity())
                continue;

            const Model* model = entity->getModel();
            if (!model)
                continue;

            StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
            if (!hdr)
                continue;

            StudioHitboxSet* set = hdr->getHitboxSet(0);
            if (!set)
                continue;

            const auto player = Animations::getPlayer(i);
            if (!player.gotMatrix)
                continue;

            for (size_t j = 0; j < hitbox.size(); j++)
            {
                if (!hitbox[j])
                    continue;

                StudioBbox* hitbox = set->getHitbox(j);
                if (!hitbox)
                    continue;

                for (auto& bonePosition : AimbotFunction::multiPoint(entity, player.matrix.data(), hitbox, localPlayerEyePosition, j, 0))
                {
                    const auto angle{ AimbotFunction::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch) };
                    const auto fov{ angle.length2D() };
                    if (fov > bestFov)
                        continue;

                    if (!cfg[weaponIndex].ignoreSmoke && memory->lineGoesThroughSmoke(localPlayerEyePosition, bonePosition, 1))
                        continue;

                    if (!entity->isVisible(bonePosition) && (cfg[weaponIndex].visibleOnly || !AimbotFunction::canScan(entity, bonePosition, activeWeapon->getWeaponData(), cfg[weaponIndex].killshot ? entity->health() : cfg[weaponIndex].minDamage, cfg[weaponIndex].friendlyFire)))
                        continue;

                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTarget = bonePosition;
                    }
                }
            }
        }

        static float lastTime = 0.f;
        if (bestTarget.notNull()) 
        {
            if (memory->globalVars->realtime - lastTime <= static_cast<float>(cfg[weaponIndex].reactionTime) / 1000.f)
                return;

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

            if (cfg[weaponIndex].autoScope && activeWeapon->isSniperRifle() && !localPlayer->isScoped() && !(cmd->buttons & (UserCmd::IN_JUMP)) && !clamped)
                cmd->buttons |= UserCmd::IN_ATTACK2;

            if (cfg[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                return;

            angle /= cfg[weaponIndex].smooth;
            cmd->viewangles += angle;
            if (!cfg[weaponIndex].silent)
                interfaces->engine->setViewAngles(cmd->viewangles);

            if (clamped || cfg[weaponIndex].smooth > 1.0f) lastAngles = cmd->viewangles;
            else lastAngles = Vector{ };

            lastCommand = cmd->commandNumber;
        }
        else
            lastTime = memory->globalVars->realtime;
    }
}
