#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/Localplayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"

void Tickbase::run(UserCmd* cmd) noexcept
{
	static float spawnTime = 0.f;

	if (!localPlayer || !localPlayer->isAlive())
		return;

    if (spawnTime != localPlayer->spawnTime())
    {
        spawnTime = localPlayer->spawnTime();
		doubletapCharge = 0;
    }

	static bool enabled = false;
	if (config->tickbase.enabled != enabled && config->tickbase.enabled)
		doubletapCharge = 0;
	enabled = config->tickbase.enabled;
	if (!config->tickbase.enabled)
		return;

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (activeWeapon->isGrenade() || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
		return;

	auto netChannel = interfaces->engine->getNetworkChannel();
	if (!netChannel)
		return;

	maxTicks = ((*memory->gameRules)->isValveDS()) ? 8 : 16;

	//TODO: Dynamicaly change shiftAmount
	shiftAmount = 14;
	if (activeWeapon->isPistol())
		shiftAmount = 7;
	

	shiftAmount = std::clamp(shiftAmount, 0, maxTicks - netChannel->chokedPackets);

	if (shiftAmount > doubletapCharge)
		return;

	if ((cmd->buttons & UserCmd::IN_ATTACK))
	{
		ticksToShift = shiftAmount;
		lastShift = shiftAmount;
		commandNumber = cmd->commandNumber;
	}
}