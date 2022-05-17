#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/Localplayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#define TICKS_TO_TIME( t )		(memory->globalVars->intervalPerTick *( t ) )
//TODO: fix random prediction errors, make ragebot backtrack on tickbase manip

void Tickbase::run(UserCmd* cmd) noexcept
{
	static float spawnTime = 0.f;
	if (CanDT())
	{
		if (spawnTime != localPlayer->spawnTime())
		{
			spawnTime = localPlayer->spawnTime();
			doubletapCharge = 0;
		}

		static bool enabled = false;
		if (config->doubletapkey.isActive() != enabled && config->doubletapkey.isActive())
			doubletapCharge = 0;
		enabled = config->doubletapkey.isActive();

		auto activeWeapon = localPlayer->getActiveWeapon();
		if (!activeWeapon)
			return;

		if (activeWeapon->isGrenade())
			return;

		auto weaponData = activeWeapon->getWeaponData();
		if (!weaponData)
			return;

		auto netChannel = interfaces->engine->getNetworkChannel();
		if (!netChannel)
			return;

		maxTicks = ((*memory->gameRules)->isValveDS()) ? 8 : 16;

		int ticksPerShot = max(0, static_cast<int>(weaponData->cycletime / memory->globalVars->intervalPerTick));

		//If it has been clamped dont remove ticks
		if (ticksPerShot > maxTicks)
			ticksPerShot = maxTicks;
		else //If it has not been clamped you need to do -2 ticks to get prediction to work well
			ticksPerShot -= 2;

		shiftAmount = std::clamp(ticksPerShot, 0, maxTicks - netChannel->chokedPackets);

		if (shiftAmount > doubletapCharge)
			return;

		if (CanFireWithExploit(lastShift))
		{
			if ((cmd->buttons & UserCmd::IN_ATTACK))
			{
				ticksToShift = shiftAmount;
				lastShift = shiftAmount;
				commandNumber = cmd->commandNumber;
			}
		}
	}
}

bool Tickbase::CanDT() {

	if (!localPlayer || !localPlayer->isAlive())
		return false;

	if (!config->doubletapkey.isActive() && !config->tickbase.enabled)
		return false;

	if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
		return false;

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return false;

	return true;
}
bool Tickbase::DTWeapon() {
	switch (localPlayer->getActiveWeapon()->itemDefinitionIndex2()) {
	case WeaponId::Taser: return false; break;
	case WeaponId::Flashbang: return false; break;
	case WeaponId::HeGrenade: return false; break;
	case WeaponId::C4: return false; break;
	case WeaponId::Revolver: return false; break;
	case WeaponId::Awp: return false; break;
	case WeaponId::Mag7: return false; break;
	case WeaponId::Sawedoff: return false; break;
	case WeaponId::Nova: return false; break;
	}


	return true;
}
bool Tickbase::CanFireWeapon(float curtime) {
	if (!localPlayer || !localPlayer->isAlive())
		return false;

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return false;

	if (curtime >= activeWeapon->nextPrimaryAttack()) {
		if (DTWeapon())
			return true;
	}

	return false;
}
bool Tickbase::CanFireWithExploit(int m_iShiftedTick) {
	float curtime = TICKS_TO_TIME(localPlayer->tickBase() - m_iShiftedTick);
	return CanFireWeapon(curtime);
}
void Tickbase::updateInput() noexcept
{
	config->doubletapkey.handleToggle();
}