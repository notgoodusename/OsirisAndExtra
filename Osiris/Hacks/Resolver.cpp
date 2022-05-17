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

		const auto playerId = event->getInt("userid");
		if (playerId == localPlayer->getUserId())
			break;

		const auto index = interfaces->engine->getPlayerForUserID(playerId);
		Animations::setPlayer(index)->shot = true;
			break;
	}
	case fnv::hash("player_death"):
	{
		//Reset player
		if (snapshots.empty())
			break;

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

		Logger::addLog("[Osiris] Hit " + entity->getPlayerName() + ", using Angle: " + std::to_string(desyncAng) + "°");
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
			std::string missed = "[Osiris] Missed " + entity->getPlayerName() + " due to resolver";
			if (snapshot.backtrackRecord > 0)
				missed += ", BT[" + std::to_string(snapshot.backtrackRecord) + "]";
			if (!(std::count(snapshot.player.blacklisted.begin(), snapshot.player.blacklisted.end(), desyncAng))) {
				snapshot.player.blacklisted.push_back(desyncAng);
				missed += ", Angle: " + std::to_string(desyncAng) + "°";
			}
			Logger::addLog(missed);
			Animations::setPlayer(snapshot.playerIndex)->misses++;
			break;
		}
	}
	if (!resolverMissed)
		Logger::addLog("[Osiris] Missed " + entity->getPlayerName() + " due to spread");
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
		return;

	if (snapshots.empty())
		return;

	Resolver::setup_detect(player, entity);
	Resolver::ResolveEntity(player, entity);
	desyncAng = entity->getAnimstate()->footYaw;

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
		return;

	if (snapshots.empty())
		return;

	auto& snapshot = snapshots.front();

	if (entity->velocity().length2D() < 0.1f)
	{
		auto animstate = entity->getAnimstate();
		float angle = snapshot.player.workingangle;
		static int side = player.side;
		if (angle != 0.f)
		{
			animstate->footYaw = angle;
			desyncAng = angle;
		}
		else
		{
			if (player.extended)
			{
				if (side == 1)
					desyncAng = entity->getAnimstate()->eyeYaw + (entity->getMaxDesyncAngle());
				else if (side == -1)
					desyncAng = entity->getAnimstate()->eyeYaw - (entity->getMaxDesyncAngle());
			}
			else
			{
				if (side == 1)
					desyncAng = entity->getAnimstate()->eyeYaw + (entity->getMaxDesyncAngle() / 1.42f);
				else if (side == -1)
					desyncAng = entity->getAnimstate()->eyeYaw - (entity->getMaxDesyncAngle() / 1.42f);
			}

			animstate->footYaw = desyncAng;
		}
	}
	

}
float build_server_abs_yaw(Animations::Players player, Entity* entity, float angle)
{
	Vector velocity = entity->velocity();
	auto anim_state = entity->getAnimstate();
	float m_flEyeYaw = angle;
	float m_flGoalFeetYaw = 0.f;

	float eye_feet_delta = Helpers::angleDiff(m_flEyeYaw, m_flGoalFeetYaw);

	static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) {
		Vector delta = a - b;
		float delta_length = delta.length();

		if (delta_length <= min_delta)
		{
			Vector result;

			if (-min_delta <= delta_length)
				return a;
			else
			{
				float iradius = 1.0f / (delta_length + FLT_EPSILON);
				return b - ((delta * iradius) * min_delta);
			}
		}
		else
		{
			float iradius = 1.0f / (delta_length + FLT_EPSILON);
			return b + ((delta * iradius) * min_delta);
		}
	};

	float spd = velocity.squareLength();;

	if (spd > std::powf(1.2f * 260.0f, 2.f))
	{
		Vector velocity_normalized = velocity.normalized();
		velocity = velocity_normalized * (1.2f * 260.0f);
	}

	float m_flChokedTime = anim_state->lastUpdateTime;
	float v25 = std::clamp(entity->duckAmount() + anim_state->duckAdditional, 0.0f, 1.0f);
	float v26 = anim_state->animDuckAmount;
	float v27 = m_flChokedTime * 6.0f;
	float v28;

	// clamp
	if ((v25 - v26) <= v27) {
		if (-v27 <= (v25 - v26))
			v28 = v25;
		else
			v28 = v26 - v27;
	}
	else {
		v28 = v26 + v27;
	}

	float flDuckAmount = std::clamp(v28, 0.0f, 1.0f);

	Vector animationVelocity = GetSmoothedVelocity(m_flChokedTime * 2000.0f, velocity, entity->velocity());
	float speed = std::fminf(animationVelocity.length(), 260.0f);

	float flMaxMovementSpeed = 260.0f;

	Entity* pWeapon = entity->getActiveWeapon();
	if (pWeapon && pWeapon->getWeaponData())
		flMaxMovementSpeed = std::fmaxf(pWeapon->getWeaponData()->maxSpeedAlt, 0.001f);

	float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float flDuckingSpeed = speed / (flMaxMovementSpeed * 0.340f);

	flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((anim_state->walkToRunTransition * -0.30000001) - 0.19999999) * flRunningSpeed) + 1.0f;

	if (flDuckAmount > 0.0f)
	{
		float flDuckingSpeed = std::clamp(flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
	}

	const float v60 = -58.f;
	const float v61 = 58.f;

	float flMinYawModifier = v60 * flYawModifier;
	float flMaxYawModifier = v61 * flYawModifier;

	if (eye_feet_delta <= flMaxYawModifier)
	{
		if (flMinYawModifier > eye_feet_delta)
			m_flGoalFeetYaw = fabs(flMinYawModifier) + m_flEyeYaw;
	}
	else
	{
		m_flGoalFeetYaw = m_flEyeYaw - fabs(flMaxYawModifier);
	}

	Helpers::normalizeYaw(m_flGoalFeetYaw);

	if (speed > 0.1f || fabs(velocity.z) > 100.0f)
	{
		m_flGoalFeetYaw = Helpers::approachAngle(
			m_flEyeYaw,
			m_flGoalFeetYaw,
			((anim_state->walkToRunTransition * 20.0f) + 30.0f)
			* m_flChokedTime);
	}
	else
	{
		m_flGoalFeetYaw = Helpers::approachAngle(
			entity->lby(),
			m_flGoalFeetYaw,
			m_flChokedTime * 100.0f);
	}

	return m_flGoalFeetYaw;
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
void Resolver::ResolveEntity(Animations::Players player, Entity* entity) {
	// get the players max rotation.
	float max_rotation = entity->getMaxDesyncAngle();
	int index = 0;
	float eye_yaw = entity->getAnimstate()->eyeYaw;

	// resolve shooting players separately.
	if (player.shot) {
		entity->getAnimstate()->footYaw = eye_yaw + Resolver::ResolveShot(player, entity);
		return;
	}
	else {
		if (entity->velocity().length2D() <= 0.1) {
			float angle_difference = Helpers::angleDiff(eye_yaw, entity->getAnimstate()->footYaw);
			index = 2 * angle_difference <= 0.0f ? 1 : -1;
		}
		else
		{
			if (!((int)player.layers[12].weight * 1000.f) && entity->velocity().length2D() > 0.1) {

				auto m_layer_delta1 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);
				auto m_layer_delta2 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);
				auto m_layer_delta3 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);

				if (m_layer_delta1 < m_layer_delta2
					|| m_layer_delta3 <= m_layer_delta2
					|| (signed int)(float)(m_layer_delta2 * 1000.0))
				{
					if (m_layer_delta1 >= m_layer_delta3
						&& m_layer_delta2 > m_layer_delta3
						&& !(signed int)(float)(m_layer_delta3 * 1000.0))
					{
						index = 1;
					}
				}
				else
				{
					index = -1;
				}
			}
		}
	}

	switch (player.misses % 3) {
	case 0: //default
		entity->getAnimstate()->footYaw = build_server_abs_yaw(player, entity, entity->eyeAngles().y + max_rotation * index);
		break;
	case 1: //reverse
		entity->getAnimstate()->footYaw = build_server_abs_yaw(player, entity, entity->eyeAngles().y + max_rotation * -index);
		break;
	case 2: //middle
		entity->getAnimstate()->footYaw = build_server_abs_yaw(player, entity, entity->eyeAngles().y);
		break;
	}

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
void Resolver::setup_detect(Animations::Players player, Entity* entity) {

	// detect if player is using maximum desync.
	if (player.layers[3].cycle == 0.f)
	{
		if (player.layers[3].weight = 0.f)
		{
			player.extended = true;
		}
	}
	/* calling detect side */
	Resolver::detect_side(entity, &player.side);
	int side = player.side;
	/* bruting vars */
	float resolve_value = 50.f;
	static float brute = 0.f;
	float fl_max_rotation = entity->getMaxDesyncAngle();
	float fl_eye_yaw = entity->getAnimstate()->eyeYaw;
	float perfect_resolve_yaw = resolve_value;
	bool fl_foword = fabsf(Helpers::angleNormalize(get_angle(entity) - get_foword_yaw(entity))) < 90.f;
	int fl_shots = player.misses;

	/* clamp angle */
	if (fl_max_rotation < resolve_value) {
		resolve_value = fl_max_rotation;
	}

	/* detect if entity is using max desync angle */
	if (player.extended) {
		resolve_value = fl_max_rotation;
	}
	/* setup brting */
	if (fl_shots == 0) {
		brute = perfect_resolve_yaw * (fl_foword ? -side : side);
	}
	else {
		switch (fl_shots % 3) {
		case 0: {
			brute = perfect_resolve_yaw * (fl_foword ? -side : side);
		} break;
		case 1: {
			brute = perfect_resolve_yaw * (fl_foword ? side : -side);
		} break;
		case 2: {
			brute = 0;
		} break;
		}
	}

	/* fix goalfeet yaw */
	entity->getAnimstate()->footYaw = fl_eye_yaw + brute;
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