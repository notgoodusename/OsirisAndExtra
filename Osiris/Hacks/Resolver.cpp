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
	case fnv::hash("weapon_fire"):
	{
		if (snapshots.empty())
			break;

		auto& snapshot = snapshots.front();

		if (int playerint = snapshot.playerIndex; event->getInt("userid") != playerint)
			break;

			snapshot.player.shot = true;
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

	if (!localPlayer || !localPlayer->isAlive())
	{
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
			Logger::addLog(std::to_string(snapshot.player.misses));
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
float get_backward_side(Entity* entity) {
	if (!entity->isAlive())
		return -1.f;
	float result = Helpers::angleDiff(localPlayer->origin().y, entity->origin().y);
	return result;
}
float get_angle(Entity* entity) {
	return Helpers::angleNormalize(entity->eyeAngles().y);
}
float get_foword_yaw(Entity* entity) {
	return Helpers::angleNormalize(get_backward_side(entity) - 180.f);
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
		return;
	}

	if (snapshots.empty())
		return;

	if (player.layers[3].cycle == 0.f)
	{
		if (player.layers[3].weight = 0.f)
		{
			player.extended = true;
		}
	}

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
		return;
	}

	if (snapshots.empty())
		return;
	static int side{};
	Resolver::detect_side(entity, &side);
	auto& snapshot = snapshots.front();
	if (player.shot)
	{
		player.shot = false;
		desyncAng = Resolver::ResolveShot(player, entity);
	}
	if (misses > 0)
	{
		static int side{};
		Resolver::detect_side(entity, &side);
		float resolve_value = 50.f;
		auto& snapshot = snapshots.front();
		float fl_max_rotation = entity->getMaxDesyncAngle();
		float perfect_resolve_yaw = resolve_value;
		bool fl_foword = fabsf(Helpers::angleNormalize(get_angle(entity) - get_foword_yaw(entity))) < 90.f;
		int fl_shots = misses;
		/* clamp angle */
		if (fl_max_rotation < resolve_value) {
			resolve_value = fl_max_rotation;
		}


		/* detect if player is using max desync angle */
		if (player.extended) {
			resolve_value = fl_max_rotation;
		}

		/* setup brting */
		if (fl_shots == 0) {
			desyncAng = perfect_resolve_yaw * (fl_foword ? -side : side);
		}
		else
		{
			switch (misses % 3) {
			case 0: {
				desyncAng = perfect_resolve_yaw * (fl_foword ? -side : side);
			} break;
			case 1: {
				desyncAng = perfect_resolve_yaw * (fl_foword ? side : -side);
			} break;
			case 2: {
				desyncAng = 0;
			} break;

			}
		}
		/* fix goalfeet yaw */
		if (snapshot.player.workingangle != 0.f)
		{
			desyncAng = snapshot.player.workingangle;
		}
		if (std::count(snapshot.player.blacklisted.begin(), snapshot.player.blacklisted.end(), desyncAng))
		{
			if (player.extended)
			{
				if (desyncAng < 0.f)
					desyncAng += -15.f;
				else
					desyncAng += 15.f;
			}
			else
			{
				if (desyncAng < 0.f)
					desyncAng += -8.f;
				else
					desyncAng += 8.f;
			}
		}
	}

	if (snapshot.player.workingangle != 0.f)
	{
		desyncAng = snapshot.player.workingangle;
	}
	entity->getAnimstate()->footYaw = entity->getAnimstate()->eyeYaw + desyncAng;


}
float Resolver::ResolveShot(Animations::Players player, Entity* entity) {
	/* fix unrestricted shot */
	float flPseudoFireYaw = Helpers::angleNormalize(Helpers::angleDiff(localPlayer->origin().y, player.matrix[8].origin().y));
	if (player.extended) {
		float flLeftFireYawDelta = fabsf(Helpers::angleNormalize(flPseudoFireYaw - (entity->eyeAngles().y + 58.f)));
		float flRightFireYawDelta = fabsf(Helpers::angleNormalize(flPseudoFireYaw - (entity->eyeAngles().y - 58.f)));

		return flLeftFireYawDelta > flRightFireYawDelta ? -58.f : 58.f;
	}
	else {
		float flLeftFireYawDelta = fabsf(Helpers::angleNormalize(flPseudoFireYaw - (entity->eyeAngles().y + 29.f)));
		float flRightFireYawDelta = fabsf(Helpers::angleNormalize(flPseudoFireYaw - (entity->eyeAngles().y - 29.f)));

		return flLeftFireYawDelta > flRightFireYawDelta ? -29.f : 29.f;
	}
}
void Resolver::detect_side(Entity* entity, int* side) {
	/* externs */
	Vector src3D, dst3D, forward, right, up;
	float back_two, right_two, left_two;
	Trace tr;
	Helpers::AngleVectors(Vector(0, get_backward_side(entity), 0), &forward, &right, &up);
	/* filtering */

	src3D = entity->getEyePosition();
	dst3D = src3D + (forward * 384);

	/* back engine tracers */
	interfaces->engineTrace->traceRay({ src3D, dst3D }, 0x200400B, { entity }, tr);
	back_two = (tr.endpos - tr.startpos).length();

	/* right engine tracers */
	interfaces->engineTrace->traceRay(Ray(src3D + right * 35, dst3D + right * 35), 0x200400B, { entity }, tr);
	right_two = (tr.endpos - tr.startpos).length();

	/* left engine tracers */
	interfaces->engineTrace->traceRay(Ray(src3D - right * 35, dst3D - right * 35), 0x200400B, { entity }, tr);
	left_two = (tr.endpos - tr.startpos).length();

	/* fix side */
	if (left_two > right_two) {
		*side = -1;
	}
	else if (right_two > left_two) {
		*side = 1;
	}
	else
		*side = 0;
}


void Resolver::updateEventListeners(bool forceRemove) noexcept
{
	class ImpactEventListener : public GameEventListener {
	public:
		void fireGameEvent(GameEvent* event) {
			getEvent(event);
		}
	};

	static ImpactEventListener listener[4];
	static bool listenerRegistered = false;

	if (config->ragebot[0].resolver && !listenerRegistered) {
		interfaces->gameEventManager->addListener(&listener[0], "bullet_impact");
		interfaces->gameEventManager->addListener(&listener[1], "player_hurt");
		interfaces->gameEventManager->addListener(&listener[2], "round_start");
		interfaces->gameEventManager->addListener(&listener[3], "weapon_fire");
		//interfaces->gameEventManager->addListener(&listener[4], "player_death"); //causes crash
		listenerRegistered = true;
	}

	else if ((!config->ragebot[0].resolver || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener[0]);
		interfaces->gameEventManager->removeListener(&listener[1]);
		interfaces->gameEventManager->removeListener(&listener[2]);
		interfaces->gameEventManager->removeListener(&listener[3]);
		//interfaces->gameEventManager->removeListener(&listener[4]);
		listenerRegistered = false;
	}
}