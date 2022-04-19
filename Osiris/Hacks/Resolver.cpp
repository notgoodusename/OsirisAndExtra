#include "Aimbot.h"
#include "Animations.h"
#include "Resolver.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"

std::deque<Resolver::SnapShot> snapshots;

bool resolver = false;

void Resolver::saveRecord(int playerIndex, float playerSimulationTime) noexcept
{
	const auto player = Animations::getPlayer(playerIndex);
	if (!player.gotMatrix)
		return;

	SnapShot snapshot;
	snapshot.player = player;
	snapshot.playerIndex = playerIndex;
	snapshot.eyePosition = localPlayer->getEyePosition();

	if (player.simulationTime == playerSimulationTime)
	{
		snapshots.push_back(snapshot);
		return;
	}

	for (int i = 0; i < static_cast<int>(player.backtrackRecords.size()); i++)
	{
		if (player.backtrackRecords.at(i).simulationTime == playerSimulationTime)
		{
			snapshots.push_back(snapshot);
			return;
		}
	}
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
		if (players->empty())
			break;

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
		if (playerId == localPlayer->getUserId())
			break;

		const auto index = interfaces->engine->getPlayerForUserID(playerId);
		Animations::setPlayer(index)->misses = 0;
		break;
	}
	case fnv::hash("player_hurt"):
	{
		if (snapshots.empty())
			break;

		if (event->getInt("attacker") != localPlayer->getUserId())
			break;

		auto& snapshot = snapshots.front();

		snapshot.getImpact = false; //We hurt the player so dont calculate it
		break;
	}
	case fnv::hash("bullet_impact"):
	{
		if (snapshots.empty())
			break;

		if (event->getInt("userid") != localPlayer->getUserId())
			break;

		auto& snapshot = snapshots.front();

		snapshot.time = memory->globalVars->serverTime();

		if (snapshot.getImpact)
			snapshot.bulletImpact = Vector{ event->getFloat("x"), event->getFloat("y"), event->getFloat("z") };
		else
			snapshots.pop_front();
		break;
	}
	default:
		break;
	}
	snapshots.clear(); //Remove this later
}

void Resolver::processMissedShots() noexcept
{
	if (snapshots.empty())
		return;

	for (int i = 0; i < static_cast<int>(snapshots.size()); i++)
	{
		auto& snapshot = snapshots.at(i);
		if (!snapshot.player.gotMatrix)
			continue;
		
		const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);
		if (!entity || !entity->isAlive() || entity->isDormant())
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

		const auto angle = Aimbot::calculateRelativeAngle(snapshot.eyePosition, snapshot.bulletImpact, Vector{ });
		const Vector forward = Vector::fromAngle(angle);
		const auto end = snapshot.bulletImpact + forward * 2000.f;

		for (int hitbox = 0; hitbox < Hitboxes::Max; hitbox++)
		{
			if (Aimbot::hitboxIntersection(snapshot.player.matrix.data(), hitbox, set, snapshot.eyePosition, end))
			{
				//Resolver miss
			}
		}
		//Spread miss
	}
	snapshots.clear();
}

void Resolver::runPlayer(int index) noexcept
{
	const auto player = Animations::getPlayer(index);
	const auto entity = interfaces->entityList->getEntity(index);

	//Check if bot and chokedpackets

	//Calculate using animlayers and other
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