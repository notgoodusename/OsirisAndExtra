#include "SDK/ConVar.h"
#include "SDK/GameEvent.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/LocalPlayer.h"

#include "Interfaces.h"
#include "Logger.h"
#include "Memory.h"


std::string getStringFromHitgroup(int hitgroup) noexcept
{
    switch (hitgroup) {
    case HitGroup::Generic:
        return "generic";
    case HitGroup::Head:
        return "head";
    case HitGroup::Chest:
        return "chest";
    case HitGroup::Stomach:
        return "stomach";
    case HitGroup::LeftArm:
        return "left arm";
    case HitGroup::RightArm:
        return "right arm";
    case HitGroup::LeftLeg:
        return "left leg";
    case HitGroup::RightLeg:
        return "right leg";
    default:
        return "unknown";
    }
}

void Logger::getEvent(GameEvent* event) noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        return;
    }

    if (!event || !localPlayer || interfaces->engine->isHLTV())
        return;

    static auto c4Timer = interfaces->cvar->findVar("mp_c4timer");

    Log log;
    log.time = memory->globalVars->realtime + 1.0f;

    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("player_hurt"):  {

        const int hurt = interfaces->engine->getPlayerForUserID(event->getInt("userid"));
        const int attack = interfaces->engine->getPlayerForUserID(event->getInt("attacker"));
        const auto damage = std::to_string(event->getInt("dmg_health"));
        const auto hitgroup = getStringFromHitgroup(event->getInt("hitgroup"));

        if (hurt != localPlayer->index() && attack == localPlayer->index())
        {
            if ((config->misc.loggerOptions.events & 1 << DamageDealt) != 1 << DamageDealt)
                break;

            const auto player = interfaces->entityList->getEntity(hurt);
            if (!player)
                break;

            log.text = "Hurt " + player->getPlayerName() + " for " + damage + " in " + hitgroup;
        }
        else if (hurt == localPlayer->index() && attack != localPlayer->index())
        {
            if ((config->misc.loggerOptions.events & 1 << DamageReceived) != 1 << DamageReceived)
                break;

            const auto player = interfaces->entityList->getEntity(attack);
            if (!player)
                break;

            log.text = "Harmed by " + player->getPlayerName() + " for " + damage + " in " + hitgroup;
        }
        break;
    }
    case fnv::hash("bomb_planted"): {
        if ((config->misc.loggerOptions.events & 1 << BombPlants) != 1 << BombPlants)
            break;

        const int idx = interfaces->engine->getPlayerForUserID(event->getInt("userid"));
        if (idx == localPlayer->index())
            break;

        const auto player = interfaces->entityList->getEntity(idx);
        if (!player)
            break;

        const std::string site = event->getInt("site") ? "a" : "b";

        log.text = "Bomb planted at bombsite " + site + " by " + player->getPlayerName() + ", detonation in" + std::to_string(c4Timer->getFloat()) + " seconds";
        break;
    }
    case fnv::hash("hostage_follows"): {
        if ((config->misc.loggerOptions.events & 1 << HostageTaken) != 1 << HostageTaken)
            break;

        const int idx = interfaces->engine->getPlayerForUserID(event->getInt("userid"));
        if (idx == localPlayer->index())
            break;

        const auto player = interfaces->entityList->getEntity(idx);
        if (!player)
            break;

        log.text = "Hostage taken by " + player->getPlayerName();
        break;
    }
    default:
        return;
    }

    if (log.text.empty())
        return;

    logs.push_front(log);
}

void Logger::process() noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        return;
    }

    if (logs.empty())
        return;

    std::array<std::uint8_t, 4> color;
    color.at(0) = config->misc.logger.color.at(0) * 255.0f;
    color.at(1) = config->misc.logger.color.at(1) * 255.0f;
    color.at(2) = config->misc.logger.color.at(2) * 255.0f;
    color.at(3) = config->misc.logger.color.at(3) * 255.0f;

    if ((config->misc.loggerOptions.modes & 1 << Console) == 1 << Console)
    {
        for (auto log : logs)
            Helpers::logConsole(log.text + "\n", color);
    }

    if ((config->misc.loggerOptions.modes & 1 << EventLog) == 1 << EventLog)
    {
    }

    logs.clear();
}

void Logger::render() noexcept
{

}