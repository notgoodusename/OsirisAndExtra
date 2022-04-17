#include "Animations.h"
#include "Resolver.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"

std::deque<Animations::Players> records;

bool resolver = false;

void Resolver::saveRecord(int playerIndex, float playerSimulationTime) noexcept
{
	const auto player = Animations::getPlayer(playerIndex);
	if (!player.gotMatrix)
		return;

	if (player.simulationTime == playerSimulationTime)
	{
		records.push_front(player);
		return;
	}

	for (int i = 0; i < static_cast<int>(player.backtrackRecords.size()); i++)
	{
		if (player.backtrackRecords.at(i).simulationTime == playerSimulationTime)
		{
			records.push_front(player);
			return;
		}
	}
}

void Resolver::runPlayer(int index) noexcept
{
	const auto player = Animations::getPlayer(index);
	const auto entity = interfaces->entityList->getEntity(index);

	//Check if bot and chokedpackets

	//Calculate using animlayers and other
}

void Resolver::getEvent(GameEvent* event) noexcept
{
	if (!event || !localPlayer || interfaces->engine->isHLTV())
		return;

	switch (fnv::hashRuntime(event->getName())) {
	case fnv::hash("round_start"):
	{
		//Reset all
		auto players = Animations::setPlayers();
		for (int i = 0; i < static_cast<int>(players->size()); i++)
		{
			players->at(i).misses = 0;
		}
		break;
	}
	case fnv::hash("player_death"):
	{
		//Reset player
		const auto playerId = event->getInt("userid");
		Animations::setPlayer(playerId)->misses = 0;
		break;
	}
	case fnv::hash("player_hurt"):
	{
		//get time
		break;
	}
	case fnv::hash("bullet_impact"):
	{
		//if hurt break
		// else
		//Match time and check if miss due to spread/resolver
		break;
	}
	default:
		break;
	}
}

void Resolver::updateEventListeners(bool forceRemove) noexcept
{
	class ImpactEventListener : public GameEventListener {
	public:
		void fireGameEvent(GameEvent* event) {
			getEvent(event);
		}
	};

	static ImpactEventListener listener;
	static bool listenerRegistered = false;

	if (resolver && !listenerRegistered) {
		interfaces->gameEventManager->addListener(&listener, "bullet_impact");
		listenerRegistered = true;
	}
	else if ((resolver || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener);
		listenerRegistered = false;
	}
}