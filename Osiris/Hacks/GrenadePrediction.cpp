#include <string>
#include "GrenadePrediction.h"

#include "../SDK/Cvar.h"
#include "../SDK/ConVar.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Surface.h"

#include "../Config.h"
#include "../GameData.h"
#include "../Memory.h"
#include "../Interfaces.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include <mutex>


std::vector<std::pair<ImVec2, ImVec2>> screenPoints;
std::vector<std::pair<ImVec2, ImVec2>> endPoints;
std::vector<std::pair<ImVec2, std::string>> dmgPoints;
std::vector<std::pair<ImVec2, ImVec2>> savedPoints;

std::mutex renderMutex;

int grenade_act{ 1 };

static bool worldToScreen(const Vector& in, ImVec2& out, bool floor = true) noexcept
{
	const auto& matrix = GameData::toScreenMatrix();

	const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;
	if (w < 0.001f)
		return false;

	out = ImGui::GetIO().DisplaySize / 2.0f;
	out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
	out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;
	if (floor)
		out = ImFloor(out);
	return true;
}

void TraceHull(Vector& src, Vector& end, Trace& tr)
{
	if (!config->misc.nadePredict)
		return;

	if (!localPlayer)
		return;

	interfaces->engineTrace->traceRay({ src, end, Vector{-2.0f, -2.0f, -2.0f}, Vector{2.0f, 2.0f, 2.0f} }, 0x200400B, { localPlayer.get() }, tr);
}

void Setup(Vector& vecSrc, Vector& vecThrow, Vector viewangles)
{
	auto AngleVectors = [](const Vector & angles, Vector * forward, Vector * right, Vector * up)
	{
		float sr, sp, sy, cr, cp, cy;

		sp = static_cast<float>(sin(double(angles.x) * 0.01745329251f));
		cp = static_cast<float>(cos(double(angles.x) * 0.01745329251f));
		sy = static_cast<float>(sin(double(angles.y) * 0.01745329251f));
		cy = static_cast<float>(cos(double(angles.y) * 0.01745329251f));
		sr = static_cast<float>(sin(double(angles.z) * 0.01745329251f));
		cr = static_cast<float>(cos(double(angles.z) * 0.01745329251f));

		if (forward)
		{
			forward->x = cp * cy;
			forward->y = cp * sy;
			forward->z = -sp;
		}

		if (right)
		{
			right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
			right->y = (-1 * sr * sp * sy + -1 * cr * cy);
			right->z = -1 * sr * cp;
		}

		if (up)
		{
			up->x = (cr * sp * cy + -sr * -sy);
			up->y = (cr * sp * sy + -sr * cy);
			up->z = cr * cp;
		}
	};
	Vector angThrow = viewangles;
	float pitch = angThrow.x;

	if (pitch <= 90.0f)
	{
		if (pitch < -90.0f)
		{
			pitch += 360.0f;
		}
	}
	else
	{
		pitch -= 360.0f;
	}

	float a = pitch - (90.0f - fabs(pitch)) * 10.0f / 90.0f;
	angThrow.x = a;

	float flVel = 750.0f * 0.9f;

	static const float power[] = { 1.0f, 1.0f, 0.5f, 0.0f };
	float b = power[grenade_act];
	b = b * 0.7f;
	b = b + 0.3f;
	flVel *= b;

	Vector vForward, vRight, vUp;
	AngleVectors(angThrow, &vForward, &vRight, &vUp);

	vecSrc = localPlayer->getEyePosition();
	float off = (power[grenade_act] * 12.0f) - 12.0f;
	vecSrc.z += off;

	Trace tr;
	Vector vecDest = vecSrc;
	vecDest += vForward * 22.0f;

	TraceHull(vecSrc, vecDest, tr);

	Vector vecBack = vForward; vecBack *= 6.0f;
	vecSrc = tr.endpos;
	vecSrc -= vecBack;

	vecThrow = localPlayer->velocity(); vecThrow *= 1.25f;
	vecThrow += vForward * flVel;
}

int PhysicsClipVelocity(const Vector& in, const Vector& normal, Vector& out, float overbounce)
{
	static const float STOP_EPSILON = 0.1f;

	float    backoff;
	float    change;
	float    angle;
	int        i, blocked;

	blocked = 0;

	angle = normal[2];

	if (angle > 0)
	{
		blocked |= 1;        // floor
	}
	if (!angle)
	{
		blocked |= 2;        // step
	}

	backoff = in.dotProduct(normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
		{
			out[i] = 0;
		}
	}

	return blocked;
}

void PushEntity(Vector& src, const Vector& move, Trace& tr)
{
	if (!config->misc.nadePredict)
		return;

	Vector vecAbsEnd = src;
	vecAbsEnd += move;
	TraceHull(src, vecAbsEnd, tr);
}

void ResolveFlyCollisionCustom(Trace& tr, Vector& vecVelocity, float interval)
{
	if (!config->misc.nadePredict)
		return;

	// Calculate elasticity
	float flSurfaceElasticity = 1.0;
	float flGrenadeElasticity = 0.45f;
	float flTotalElasticity = flGrenadeElasticity * flSurfaceElasticity;
	if (flTotalElasticity > 0.9f) flTotalElasticity = 0.9f;
	if (flTotalElasticity < 0.0f) flTotalElasticity = 0.0f;

	// Calculate bounce
	Vector vecAbsVelocity;
	PhysicsClipVelocity(vecVelocity, tr.plane.normal, vecAbsVelocity, 2.0f);
	vecAbsVelocity *= flTotalElasticity;

	float flSpeedSqr = vecAbsVelocity.squareLength();
	static const float flMinSpeedSqr = 20.0f * 20.0f;

	if (flSpeedSqr < flMinSpeedSqr)
	{
		vecAbsVelocity.x = 0.0f;
		vecAbsVelocity.y = 0.0f;
		vecAbsVelocity.z = 0.0f;
	}

	if (tr.plane.normal.z > 0.7f)
	{
		vecVelocity = vecAbsVelocity;
		vecAbsVelocity *= ((1.0f - tr.fraction) * interval);
		PushEntity(tr.endpos, vecAbsVelocity, tr);
	}
	else
	{
		vecVelocity = vecAbsVelocity;
	}
}

void AddGravityMove(Vector& move, Vector& vel, float frametime, bool onground)
{
	if (!config->misc.nadePredict)
		return;

	Vector basevel{ 0.0f, 0.0f, 0.0f };

	move.x = (vel.x + basevel.x) * frametime;
	move.y = (vel.y + basevel.y) * frametime;

	if (onground)
	{
		move.z = (vel.z + basevel.z) * frametime;
	}
	else
	{
		float gravity = 800.0f * 0.4f;
		float newZ = vel.z - (gravity * frametime);
		move.z = ((vel.z + newZ) / 2.0f + basevel.z) * frametime;
		vel.z = newZ;
	}
}

enum ACT
{
	ACT_NONE,
	ACT_THROW,
	ACT_LOB,
	ACT_DROP,
};

void Tick(int buttons)
{
	bool in_attack = buttons & UserCmd::IN_ATTACK;
	bool in_attack2 = buttons & UserCmd::IN_ATTACK2;

	grenade_act = (in_attack && in_attack2) ? ACT_LOB :
		(in_attack2) ? ACT_DROP :
		(in_attack) ? ACT_THROW :
		ACT_NONE;
}



bool checkDetonate(const Vector& vecThrow, const Trace& tr, int tick, float interval, Entity* activeWeapon)
{
	switch (activeWeapon->itemDefinitionIndex2())
	{
	case WeaponId::SmokeGrenade:
	case WeaponId::Decoy:
		if (vecThrow.length2D() < 0.1f)
		{
			int det_tick_mod = (int)(0.2f / interval);
			return !(tick % det_tick_mod);
		}
		return false;
	case WeaponId::Molotov:
	case WeaponId::IncGrenade:
		if (tr.fraction != 1.0f && tr.plane.normal.z > 0.7f)
			return true;
	case WeaponId::Flashbang:
	case WeaponId::HeGrenade:
		return (float)tick * interval > 1.5f && !(tick % (int)(0.2f / interval));
	default:
		return false;
	}
}

void drawCircle(Vector position, float points, float radius)
{
	float step = 3.141592654f * 2.0f / points;
	ImVec2 end2d{}, start2d{};
	Vector lastPos{};
	for (float a = -step; a < 3.141592654f * 2.0f; a += step) {
		Vector start{ radius * cosf(a) + position.x, radius * sinf(a) + position.y, position.z };

		Trace tr;
		TraceHull(position, start, tr);
		if (!tr.endpos.notNull())
			continue;

		if (worldToScreen(tr.endpos, start2d) && worldToScreen(lastPos, end2d) && lastPos != Vector{ })
		{
			if (start2d != ImVec2{ } && end2d != ImVec2{ })
				endPoints.emplace_back(std::pair<ImVec2, ImVec2>{ end2d, start2d });
		}
		lastPos = tr.endpos;
	}
}

inline auto CSGO_Armor(float flDamage, int ArmorValue)
{
	static float flArmorRatio = 0.5f;
	static float flArmorBonus = 0.5f;
	if (ArmorValue > 0) {
		float flNew = flDamage * flArmorRatio;
		float flArmor = (flDamage - flNew) * flArmorBonus;

		if (flArmor > static_cast<float>(ArmorValue)) {
			flArmor = static_cast<float>(ArmorValue) * (1.f / flArmorBonus);
			flNew = flDamage - flArmor;
		}

		flDamage = flNew;
	}
	return flDamage;
}

void drawDamage(Vector position) {
	static auto mp_friendlyfire = interfaces->cvar->findVar("mp_friendlyfire");
	static auto ff_damage_reduction_grenade = interfaces->cvar->findVar("ff_damage_reduction_grenade");

	ImVec2 pos{};

	GameData::Lock lock;
	for (auto& player : GameData::players()) {
		if (player.handle == localPlayer || player.dormant || !player.alive)
			continue;

		Vector center = player.origin + (player.obbMins + player.obbMaxs) * 0.5f;
		const float dist = (center - position).length();
		if (dist > 350.f)
			continue;

		Trace tr;
		TraceHull(position, center, tr);
		if (!tr.endpos.notNull() || !tr.entity || tr.entity->handle() != player.handle)
			continue;

		static const float a = 105.0f;
		static const float b = 25.0f;
		static const float c = 140.0f;

		const float d = ((dist - b) / c);
		const float flDamage = a * exp(-d * d);
		int dmg = max(static_cast<int>(ceilf(CSGO_Armor(flDamage, player.armor))), 0);
		dmg = min(dmg, (player.armor > 0) ? 57.f : 98.f);

		if (mp_friendlyfire->getInt() > 0 && !player.enemy)
			dmg *= ff_damage_reduction_grenade->getFloat();

		if (dmg < 1)
			continue;

		std::string dmg2text = player.health - dmg > 0 ? std::to_string(dmg) : "kILL";
		if (worldToScreen(player.origin, pos))
		{
			dmgPoints.emplace_back(std::pair<ImVec2, std::string>{ pos, dmg2text });
		}

	}
}

void NadePrediction::run(UserCmd* cmd) noexcept
{
	renderMutex.lock();

	screenPoints.clear();
	endPoints.clear();
	dmgPoints.clear();

	if (!config->misc.nadePredict)
	{
		renderMutex.unlock();
		return;
	}

	if (!localPlayer || !localPlayer->isAlive())
	{
		renderMutex.unlock();
		return;
	}

	if (interfaces->engine->isHLTV())
	{
		renderMutex.unlock();
		return;
	}

	Tick(cmd->buttons);
	if (localPlayer->moveType() == MoveType::NOCLIP)
	{
		renderMutex.unlock();
		return;
	}

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon || !activeWeapon->isGrenade())
	{
		renderMutex.unlock();
		return;
	}
	const auto itemDefinition = activeWeapon->itemDefinitionIndex2();
	if (itemDefinition != WeaponId::SmokeGrenade
		&& itemDefinition != WeaponId::Decoy
		&& itemDefinition != WeaponId::Molotov
		&& itemDefinition != WeaponId::IncGrenade
		&& itemDefinition != WeaponId::Flashbang
		&& itemDefinition != WeaponId::HeGrenade)
	{
		renderMutex.unlock();
		return;
	}

	Vector vecSrc, vecThrow;
	Setup(vecSrc, vecThrow, cmd->viewangles);

	float interval = memory->globalVars->intervalPerTick;
	int logstep = static_cast<int>(0.05f / interval);
	int logtimer = 0;

	std::vector<Vector> path;

	for (unsigned int i = 0; i < path.max_size() - 1; ++i)
	{
		if (!logtimer)
			path.emplace_back(vecSrc);

		Vector move;
		AddGravityMove(move, vecThrow, interval, false);

		// Push entity
		Trace tr;
		PushEntity(vecSrc, move, tr);

		int result = 0;
		if (checkDetonate(vecThrow, tr, i, interval, activeWeapon))
			result |= 1;

		if (tr.fraction != 1.0f)
		{
			result |= 2; // Collision!
			ResolveFlyCollisionCustom(tr, vecThrow, interval);
		}

		vecSrc = tr.endpos;

		if (result & 1)
			break;

		if ((result & 2) || logtimer >= logstep)
			logtimer = 0;
		else
			++logtimer;
	}

	path.emplace_back(vecSrc);

	Vector prev = path[0];
	ImVec2 nadeStart, nadeEnd;
	Vector lastPos{ };
	for (auto& nade : path)
	{
		if (worldToScreen(prev, nadeStart) && worldToScreen(nade, nadeEnd))
		{
			screenPoints.emplace_back(std::pair<ImVec2, ImVec2>{ nadeStart, nadeEnd });
			prev = nade;
			lastPos = nade;
		}
	}

	if (lastPos.notNull()) {
		drawCircle(lastPos, 120, 150);
		if (config->misc.nadeDamagePredict.enabled && itemDefinition == WeaponId::HeGrenade)
			drawDamage(lastPos);
	}
		

	renderMutex.unlock();
}

void NadePrediction::draw() noexcept
{
	if (!config->misc.nadePredict)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	if (interfaces->engine->isHLTV())
		return;

	if (renderMutex.try_lock())
	{
		savedPoints = screenPoints;

		renderMutex.unlock();
	}

	if (savedPoints.empty())
		return;

	static const auto redColor = ImGui::GetColorU32(ImVec4(1.f, 0.f, 0.f, 1.f));
	static const auto whiteColor = ImGui::GetColorU32(ImVec4(0.3f, 0.1f, 0.3f, 1.f));
	auto blueColor = ImGui::GetColorU32(ImVec4(0.3f, 0.1f, 0.3f, 1.f));

	auto drawList = ImGui::GetBackgroundDrawList();
	// draw end nade path
	for (auto& point : endPoints)
		drawList->AddLine(ImVec2(point.first.x, point.first.y), ImVec2(point.second.x, point.second.y), blueColor, 2.f);

	//	draw nade path
	for (auto& point : savedPoints)
		drawList->AddLine(ImVec2(point.first.x, point.first.y), ImVec2(point.second.x, point.second.y), whiteColor, 1.5f);

	// draw nade damage
	if (config->misc.nadeDamagePredict.enabled)
	{
		for (auto& point : dmgPoints)
		{
			const auto textSize = ImGui::CalcTextSize(point.second.c_str());
			const auto horizontalOffset = textSize.x / 2;
			const auto verticalOffset = textSize.y;

			drawList->AddText({ point.first.x - horizontalOffset + 1.0f, point.first.y - verticalOffset + 1.0f }, Helpers::calculateColor(config->misc.nadeDamagePredict) & IM_COL32_A_MASK, point.second.c_str());
			drawList->AddText({ point.first.x - horizontalOffset, point.first.y - verticalOffset }, Helpers::calculateColor(config->misc.nadeDamagePredict), point.second.c_str());
		}
	}
}
