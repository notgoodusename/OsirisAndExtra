#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "GameData.h"
#include "Hacks/Misc.h"
#include "Hacks/Resolver.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Visuals.h"
#include "Interfaces.h"
#include "Logger.h"
#include "Memory.h"

EventListener::EventListener() noexcept
{
    assert(interfaces);

    // If you add here listeners which aren't used by client.dll (e.g., item_purchase, bullet_impact), the cheat will be detected by AntiDLL (community anticheat).
    // Instead, register listeners dynamically and only when certain functions are enabled - see Misc::updateEventListeners(), Visuals::updateEventListeners()

    interfaces->gameEventManager->addListener(this, "round_start");
    interfaces->gameEventManager->addListener(this, "round_freeze_end");
    interfaces->gameEventManager->addListener(this, "player_hurt");
    interfaces->gameEventManager->addListener(this, "bullet_impact");

    interfaces->gameEventManager->addListener(this, "bomb_planted");
    interfaces->gameEventManager->addListener(this, "hostage_follows");

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
        [[fallthrough]];
    case fnv::hash("round_freeze_end"):
        Misc::purchaseList(event);
        break;
    case fnv::hash("player_death"):
        SkinChanger::updateStatTrak(*event);
        SkinChanger::overrideHudIcon(*event);
        Misc::killMessage(*event);
        Misc::killSound(*event);
        Resolver::getEvent(event);
        break;
    case fnv::hash("player_hurt"):
        Misc::playHitSound(*event);
        Visuals::hitEffect(event);
        Visuals::hitMarker(event);
        break;  
    case fnv::hash("bullet_impact"):
        Logger::getEvent(event);
        Resolver::getEvent(event);
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
    }
}
