#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/Localplayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"

//TODO: fix random prediction errors, make ragebot backtrack on tickbase manip

void Tickbase::run(UserCmd* cmd) noexcept
{
	static float spawnTime = 0.f;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
		return;

	if (localPlayer->flags() & (1 << 6))
		return;

	if (spawnTime != localPlayer->spawnTime())
	{
		spawnTime = localPlayer->spawnTime();
		doubletapCharge = 0;
	}

	static bool enabled = false;
	if (config->doubletapkey.isActive() != enabled && config->doubletapkey.isActive())
		doubletapCharge = 0;
	enabled = config->doubletapkey.isActive();
	if (!config->tickbase.enabled)
		return;
	if (!config->doubletapkey.isActive())
		return;

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (activeWeapon->isGrenade() || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver || activeWeapon->itemDefinitionIndex2() == WeaponId::C4)
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

	if ((cmd->buttons & UserCmd::IN_ATTACK))
	{
		ticksToShift = shiftAmount;
		lastShift = shiftAmount;
		commandNumber = cmd->commandNumber;
	}
}
void Tickbase::updateInput() noexcept
{
	config->doubletapkey.handleToggle();
}