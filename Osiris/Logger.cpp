#include "Interfaces.h"
#include "Memory.h"

#include "Logger.h"

#include "SDK/ConVar.h"
#include "SDK/GameEvent.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/LocalPlayer.h"

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

void Logger::reset() noexcept
{
    renderLogs.clear();
    logs.clear();
}

void Logger::getEvent(GameEvent* event) noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        renderLogs.clear();
        return;
    }

    if (!event || !localPlayer || interfaces->engine->isHLTV())
        return;

    static auto c4Timer = interfaces->cvar->findVar("mp_c4timer");

    Log log;
    log.time = memory->globalVars->realtime;

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

        log.text = "Bomb planted at bombsite " + site + " by " + player->getPlayerName() + ", detonation in " + std::to_string(c4Timer->getFloat()) + " seconds";
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
    renderLogs.push_front(log);
}

void Logger::process(ImDrawList* drawList) noexcept
{
    if (!config->misc.logger.enabled || config->misc.loggerOptions.modes == 0 || config->misc.loggerOptions.events == 0)
    {
        logs.clear();
        renderLogs.clear();
        return;
    }

    console();
    render(drawList);
}

void Logger::console() noexcept
{
    if (logs.empty())
        return;

    if ((config->misc.loggerOptions.modes & 1 << Console) != 1 << Console)
    {
        logs.clear();
        return;
    }

    std::array<std::uint8_t, 4> color;
    if (!config->misc.logger.rainbow)
    {
        color.at(0) = static_cast<uint8_t>(config->misc.logger.color.at(0) * 255.0f);
        color.at(1) = static_cast<uint8_t>(config->misc.logger.color.at(1) * 255.0f);
        color.at(2) = static_cast<uint8_t>(config->misc.logger.color.at(2) * 255.0f);
    }
    else
    {
        const auto [colorR, colorG, colorB] { rainbowColor(config->misc.logger.rainbowSpeed) };
        color.at(0) = static_cast<uint8_t>(colorR * 255.0f);
        color.at(1) = static_cast<uint8_t>(colorG * 255.0f);
        color.at(2) = static_cast<uint8_t>(colorB * 255.0f);
    }
    color.at(3) = static_cast<uint8_t>(255.0f);

    for (auto log : logs)
        Helpers::logConsole(log.text + "\n", color);

    logs.clear();
}

void Logger::render(ImDrawList* drawList) noexcept
{
    if ((config->misc.loggerOptions.modes & 1 << EventLog) != 1 << EventLog)
    {
        renderLogs.clear();
        return;
    }

    if (renderLogs.empty())
        return;

    while (renderLogs.size() > 6)
        renderLogs.pop_back();

    for (int i = renderLogs.size() - 1; i >= 0; i--)
    {
        if (renderLogs[i].time + 5.0f <= memory->globalVars->realtime)
            renderLogs[i].alpha -= 16.f;

        const auto alphaBackup = Helpers::getAlphaFactor();
        Helpers::setAlphaFactor(renderLogs[i].alpha / 255.0f);
        const auto color = Helpers::calculateColor(config->misc.logger);
        Helpers::setAlphaFactor(alphaBackup);
        drawList->AddText(ImVec2{ 14.0f, 5.0f + static_cast<float>(20 * i) + 1.0f }, color & IM_COL32_A_MASK, renderLogs[i].text.c_str());
        drawList->AddText(ImVec2{ 14.0f, 5.0f + static_cast<float>(20 * i) + 1.0f }, color, renderLogs[i].text.c_str());
    }

    for (int i = renderLogs.size() - 1; i >= 0; i--) {
        if (renderLogs[i].alpha <= 0.0f) {
            renderLogs.erase(renderLogs.begin() + i);
            break;
        }
    }
}

void Logger::addLog(std::string logText) noexcept
{
    Log log;
    log.time = memory->globalVars->realtime;
    log.text = logText;

    logs.push_front(log);
    renderLogs.push_front(log);
}