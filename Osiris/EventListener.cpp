#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "GameData.h"

#include "Interfaces.h"
#include "Memory.h"
#include "Logger.h"

#include "Hacks/Misc.h"
#include "Hacks/Resolver.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Visuals.h"

EventListener::EventListener() noexcept
{
    assert(interfaces);

    // If you add here listeners which aren't used by client.dll (e.g., item_purchase, bullet_impact), the cheat will be detected by AntiDLL (community anticheat).
    // Instead, register listeners dynamically and only when certain functions are enabled - see Misc::updateEventListeners(), Visuals::updateEventListeners()

    interfaces->gameEventManager->addListener(this, "round_start");
    interfaces->gameEventManager->addListener(this, "round_freeze_end");
    interfaces->gameEventManager->addListener(this, "player_hurt");

    interfaces->gameEventManager->addListener(this, "bomb_planted");
    interfaces->gameEventManager->addListener(this, "hostage_follows");

    interfaces->gameEventManager->addListener(this, "weapon_fire");

    interfaces->gameEventManager->addListener(this, "smokegrenade_detonate");
    interfaces->gameEventManager->addListener(this, "molotov_detonate");
    interfaces->gameEventManager->addListener(this, "inferno_expire");

    interfaces->gameEventManager->addListener(this, "player_death");
    interfaces->gameEventManager->addListener(this, "vote_cast");

    if (const auto desc = memory->getEventDescriptor(interfaces->gameEventManager, "player_death", nullptr))
        std::swap(desc->listeners[0], desc->listeners[desc->listeners.size - 1]);
    else
        assert(false);
}

void EventListener::remove() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->removeListener(this);
}

void EventListener::fireGameEvent(GameEvent* event)
{
    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
        GameData::clearProjectileList();
        Misc::preserveKillfeed(true);
        Misc::autoBuy(event);
        Resolver::getEvent(event);
        Visuals::bulletTracer(*event);
        [[fallthrough]];
    case fnv::hash("round_freeze_end"):
        Misc::purchaseList(event);
        break;
    case fnv::hash("player_death"):
        SkinChanger::updateStatTrak(*event);
        SkinChanger::overrideHudIcon(*event);
        Misc::killfeedChanger(*event);
        Misc::killMessage(*event);
        Misc::killSound(*event);
        Resolver::getEvent(event);
        break;
    case fnv::hash("player_hurt"):
        Misc::playHitSound(*event);
        Visuals::hitEffect(event);
        Visuals::hitMarker(event);
        Visuals::drawHitboxMatrix(event);
        Logger::getEvent(event);
        Resolver::getEvent(event);
        break;
    case fnv::hash("weapon_fire"):
        Visuals::bulletTracer(*event);
        break;
    case fnv::hash("vote_cast"):
        Misc::voteRevealer(*event);
        break;
    case fnv::hash("bomb_planted"):
        Logger::getEvent(event);
        break;
    case fnv::hash("hostage_follows"):
        Logger::getEvent(event);
        break;
    case fnv::hash("smokegrenade_detonate"):
        Visuals::drawSmokeTimerEvent(event);
        break;
    case fnv::hash("molotov_detonate"):
        Visuals::drawMolotovTimerEvent(event);
        break;
    case fnv::hash("inferno_expire"):
        Visuals::molotovExtinguishEvent(event);
        break;
    }
}
