#include "Aimbot.h"
#include "Animations.h"
#include "Resolver.h"
#include "AntiAim.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"

std::deque<Resolver::SnapShot> snapshots;
float desyncAng{ 0 };

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
		desyncAng = 0;
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
		if (!localPlayer || !localPlayer->isAlive())
		{
			snapshots.clear();
			return;
		}

		if (event->getInt("attacker") != localPlayer->getUserId())
			break;

		const auto hitgroup = event->getInt("hitgroup");
		if (hitgroup < HitGroup::Head || hitgroup > HitGroup::RightLeg)
			break;

		auto& snapshot = snapshots.front();

		if (snapshot.player.workingangle != 0)
			snapshot.player.workingangle = desyncAng;
		const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);

		Logger::addLog("Hit " + entity->getPlayerName() + ", using desync: " + std::to_string(desyncAng));
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
	if (!config->ragebot[0].resolver)
		snapshots.clear();
}

void Resolver::processMissedShots() noexcept
{
	if (!config->ragebot[0].resolver)
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

	const auto angle = Aimbot::calculateRelativeAngle(snapshot.eyePosition, snapshot.bulletImpact, Vector{ });
	const auto end = snapshot.bulletImpact + Vector::fromAngle(angle) * 2000.f;

	const auto matrix = snapshot.backtrackRecord == -1 ? snapshot.player.matrix.data() : snapshot.player.backtrackRecords.at(snapshot.backtrackRecord).matrix;

	bool resolverMissed = false;

	for (int hitbox = 0; hitbox < Hitboxes::Max; hitbox++)
	{
		if (Aimbot::hitboxIntersection(matrix, hitbox, set, snapshot.eyePosition, end))
		{
			resolverMissed = true;
			std::string missed = "Missed " + entity->getPlayerName() + " due to resolver";
			if (snapshot.backtrackRecord > 0)
				missed += ", BT[" + std::to_string(snapshot.backtrackRecord) + "]";
			if (!(std::count(snapshot.player.blacklisted.begin(), snapshot.player.blacklisted.end(), desyncAng))) {
				snapshot.player.blacklisted.push_back(desyncAng);
				missed += ", Desync: " + std::to_string(desyncAng);
			}
			Logger::addLog(missed);
			Animations::setPlayer(snapshot.playerIndex)->misses++;
			break;
		}
	}
	if (!resolverMissed)
		Logger::addLog("Missed " + entity->getPlayerName() + " due to spread");
}
float clampedangle(float initialangle)
{
	float result = initialangle;
	if (initialangle > 60.f)
	{
		result = initialangle / 2.f;
		desyncAng = result;
	}
	else if (initialangle < -60.f)
	{
		result = initialangle / 2.f;
		desyncAng = result;
	}
	return result;
}
void Resolver::runPreUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!config->ragebot[0].resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

	if (player.chokedPackets <= 0)
		return;
	if (!localPlayer || !localPlayer->isAlive())
	{
		snapshots.clear();
		return;
	}

	if (snapshots.empty())
		return;
	if (misses > 0)
	{
		auto snapshot = snapshots.front();
		float eye_feet = entity->eyeAngles().y - entity->getAnimstate()->footYaw;
		float desyncSide = 2 * eye_feet <= 0.0f ? 1 : -1;
		if (eye_feet == 1.f)
		{
			if (std::count(snapshot.player.blacklisted.begin(), snapshot.player.blacklisted.end(), desyncAng)) {
				desyncAng += 25.f;
			}
		}
		else if (desyncSide == -1.f)
		{
			if (std::count(snapshot.player.blacklisted.begin(), snapshot.player.blacklisted.end(), desyncAng)) {
				desyncAng += -25.f;
			}
		}
		else if (eye_feet == 0.f)
		{
			if (entity->velocity().length2D() > 3.0f) {
				desyncAng = entity->getAnimstate()->footYaw;
			}
			else
			desyncAng = entity->getMaxDesyncAngle() - 2.f;
		}
	}
	return;
}

void Resolver::runPostUpdate(Animations::Players player, Entity* entity) noexcept
{
	if (!config->ragebot[0].resolver)
		return;

	const auto misses = player.misses;
	if (!entity || !entity->isAlive())
		return;

	if (player.chokedPackets <= 0)
		return;
	if (!localPlayer || !localPlayer->isAlive())
	{
		snapshots.clear();
		return;
	}

	if (snapshots.empty())
		return;
	if (misses > 0)
	{
		auto animstate = entity->getAnimstate();
		float eye_feet = entity->eyeAngles().y - entity->getAnimstate()->footYaw;
		if (eye_feet == 0.f)
		{
			if (entity->velocity().length2D() > 3.0f) {
				desyncAng = entity->getAnimstate()->footYaw;
			}
		}
		Vector eyeangle{ entity->eyeAngles() };
		auto snapshot = snapshots.front();
		if (snapshot.player.workingangle != 0.f)
		{
			desyncAng = snapshot.player.workingangle;
		}
		eyeangle.y += clampedangle(desyncAng);
		entity->updateState(animstate, eyeangle);
	}
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

	static ImpactEventListener listener[3];
	static bool listenerRegistered = false;

	if (config->ragebot[0].resolver && !listenerRegistered) {
		interfaces->gameEventManager->addListener(&listener[0], "bullet_impact");
		interfaces->gameEventManager->addListener(&listener[1], "player_hurt");
		interfaces->gameEventManager->addListener(&listener[2], "round_start");
		//interfaces->gameEventManager->addListener(&listener[3], "player_death"); //causes crash
		listenerRegistered = true;
	}

	else if ((!config->ragebot[0].resolver || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener[0]);
		interfaces->gameEventManager->removeListener(&listener[1]);
		interfaces->gameEventManager->removeListener(&listener[2]);
		//interfaces->gameEventManager->removeListener(&listener[3]);
		listenerRegistered = false;
	}
}