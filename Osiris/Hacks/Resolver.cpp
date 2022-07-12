#include "AimbotFunctions.h"
#include "Animations.h"
#include "Resolver.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"

std::deque<Resolver::SnapShot> snapshots;

bool resolver = false;

void Resolver::reset() noexcept
{
	snapshots.clear();
}

void Resolver::saveRecord(int playerIndex, float playerSimulationTime) noexcept
{
	const auto entity = interfaces->entityList->getEntity(playerIndex);
	const auto player = Animations::getPlayer(playerIndex);
	if (!player.gotMatrix || !entity)
		return;

	SnapShot snapshot;
	snapshot.player = player;
	snapshot.playerIndex = playerIndex;
	snapshot.eyePosition = localPlayer->getEyePosition();
	snapshot.model = entity->getModel();

	if (player.simulationTime == playerSimulationTime)
	{
		snapshots.push_back(snapshot);
		return;
	}

	for (int i = 0; i < static_cast<int>(player.backtrackRecords.size()); i++)
	{
		if (player.backtrackRecords.at(i).simulationTime == playerSimulationTime)
		{
			snapshot.backtrackRecord = i;
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
		snapshots.clear();
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

		const auto hitgroup = event->getInt("hitgroup");
		if (hitgroup < HitGroup::Head || hitgroup > HitGroup::RightLeg)
			break;

		snapshots.pop_front(); //Hit somebody so dont calculate
		break;
	}
	case fnv::hash("bullet_impact"):
	{
		if (snapshots.empty())
			break;

		if (event->getInt("userid") != localPlayer->getUserId())
			break;

		auto& snapshot = snapshots.front();

		if (!snapshot.gotImpact)
		{
			snapshot.time = memory->globalVars->serverTime();
			snapshot.bulletImpact = Vector{ event->getFloat("x"), event->getFloat("y"), event->getFloat("z") };
			snapshot.gotImpact = true;
		}
		break;
	}
	default:
		break;
	}
	if (!resolver)
		snapshots.clear();
}

void Resolver::processMissedShots() noexcept
{
	if (!resolver)
	{
		snapshots.clear();
		return;
	}

	if (!localPlayer ||!localPlayer->isAlive())
	{
		snapshots.clear();
		return;
	}

	if (snapshots.empty())
		return;

	if (snapshots.front().time == -1) //Didnt get data yet
		return;

	auto snapshot = snapshots.front();
	snapshots.pop_front(); //got the info no need for this
	if (!snapshot.player.gotMatrix)
		return;

	const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);
	if (!entity)
		return;

	const Model* model = snapshot.model;
	if (!model)
		return;

	StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
	if (!hdr)
		return;

	StudioHitboxSet* set = hdr->getHitboxSet(0);
	if (!set)
		return;

	const auto angle = AimbotFunction::calculateRelativeAngle(snapshot.eyePosition, snapshot.bulletImpact, Vector{ });
	const auto end = snapshot.bulletImpact + Vector::fromAngle(angle) * 2000.f;

	const auto matrix = snapshot.backtrackRecord == -1 ? snapshot.player.matrix.data() : snapshot.player.backtrackRecords.at(snapshot.backtrackRecord).matrix;

	bool resolverMissed = false;

	for (int hitbox = 0; hitbox < Hitboxes::Max; hitbox++)
	{
		if (AimbotFunction::hitboxIntersection(matrix, hitbox, set, snapshot.eyePosition, end))
		{
			resolverMissed = true;
			std::string missed = "Missed " + entity->getPlayerName() + " due to resolver";
			if (snapshot.backtrackRecord > 0)
				missed += "BT[" + std::to_string(snapshot.backtrackRecord) + "]";
			Logger::addLog(missed);
			Animations::setPlayer(snapshot.playerIndex)->misses++;
			break;
		}
	}
	if (!resolverMissed)
		Logger::addLog("Missed due to spread");
}

void Resolver::runPreUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

	if (player.chokedPackets <= 0)
		return;
}

void Resolver::runPostUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

	if (player.chokedPackets <= 0)
		return;
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
	else if ((!resolver || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener);
		listenerRegistered = false;
	}
}