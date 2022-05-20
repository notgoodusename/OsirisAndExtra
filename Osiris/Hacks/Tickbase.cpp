#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/Localplayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#define TICKS_TO_TIME( t )		(memory->globalVars->intervalPerTick *( t ) )
#define TIME_TO_TICKS( dt )		( (int)( 0.5 + (float)(dt) / memory->globalVars->intervalPerTick ) )
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
		if (localPlayer->flags() & (1 << 5))
			return;
		if (localPlayer->flags() & (1 << 6))
			return;

		maxTicks = ((*memory->gameRules)->isValveDS()) ? 8 : 19;
		if (!config->tickbase.hideshots)
		{
			if (!localPlayer || !localPlayer->isAlive())
				return;
			int ticksPerShot = max(0, static_cast<int>(weaponData->cycletime / memory->globalVars->intervalPerTick));

			//If it has been clamped dont remove ticks
			if (ticksPerShot > maxTicks)
				ticksPerShot = maxTicks;
			else //If it has not been clamped you need to do -2 ticks to get prediction to work well
				ticksPerShot -= 2;
			if (localPlayer && localPlayer->isAlive())
				shiftAmount = std::clamp(ticksPerShot, 0, maxTicks - netChannel->chokedPackets);

			if (shiftAmount > doubletapCharge)
				return;
			if (CanFireWithExploit(lastShift))
			{
				if ((memory->globalVars->currenttime - (Firerate() - 0.05f)) > activeWeapon->lastshottime())
				{
					if ((cmd->buttons & UserCmd::IN_ATTACK))
					{
						if (localPlayer && localPlayer->isAlive())
						{
							ticksToShift = shiftAmount;
							lastShift = shiftAmount;
							commandNumber = cmd->commandNumber;
						}
					}
				}
			}
		}
		else
		{
			if (config->tickbase.hideshots)
			{
				if (!localPlayer || !localPlayer->isAlive())
					return;
				int ticksPerShot = TIME_TO_TICKS(2.0f);
				ticksPerShot = max(-ticksPerShot, -6);
				if (localPlayer && localPlayer->isAlive())
					shiftAmount = std::clamp(ticksPerShot, -6, maxTicks - netChannel->chokedPackets);

				if (CanFireWithExploit(lastShift))
				{
					if ((memory->globalVars->currenttime - (Firerate() - 0.05f)) > activeWeapon->lastshottime())
					{
						if ((cmd->buttons & UserCmd::IN_ATTACK))
						{
							if (localPlayer && localPlayer->isAlive())
							{
								ticksToShift = shiftAmount;
								lastShift = shiftAmount;
								commandNumber = cmd->commandNumber;
							}
						}
					}
				}
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

	return true;
}
bool Tickbase::DTWeapon() {
		switch (localPlayer->getActiveWeapon()->itemDefinitionIndex2()) {
		case WeaponId::Taser: return false; break;
		case WeaponId::C4: return false; break;
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
float Tickbase::Firerate()
{
	if (!Tickbase::DTWeapon)
		return 0.f;
	auto pWeapon = localPlayer->getActiveWeapon();
	if (!pWeapon)
		return false;
	auto weaponData = pWeapon->getWeaponData();
	std::string WeaponName = weaponData->name;

	if (WeaponName == "weapon_glock")
		return 0.15f;
	else if (WeaponName == "weapon_hkp2000")
		return 0.169f;
	else if (WeaponName == "weapon_p250")//the cz and p250 have the same name idky same with other guns
		return 0.15f;
	else if (WeaponName == "weapon_tec9")
		return 0.12f;
	else if (WeaponName == "weapon_elite")
		return 0.12f;
	else if (WeaponName == "weapon_fiveseven")
		return 0.15f;
	else if (WeaponName == "weapon_deagle")
		return 0.224f;
	else if (WeaponName == "weapon_nova")
		return 0.882f;
	else if (WeaponName == "weapon_sawedoff")
		return 0.845f;
	else if (WeaponName == "weapon_mag7")
		return 0.845f;
	else if (WeaponName == "weapon_xm1014")
		return 0.35f;
	else if (WeaponName == "weapon_mac10")
		return 0.075f;
	else if (WeaponName == "weapon_ump45")
		return 0.089f;
	else if (WeaponName == "weapon_mp9")
		return 0.070f;
	else if (WeaponName == "weapon_bizon")
		return 0.08f;
	else if (WeaponName == "weapon_mp7")
		return 0.08f;
	else if (WeaponName == "weapon_p90")
		return 0.070f;
	else if (WeaponName == "weapon_galilar")
		return 0.089f;
	else if (WeaponName == "weapon_ak47")
		return 0.1f;
	else if (WeaponName == "weapon_sg556")
		return 0.089f;
	else if (WeaponName == "weapon_m4a1")
		return 0.089f;
	else if (WeaponName == "weapon_aug")
		return 0.089f;
	else if (WeaponName == "weapon_m249")
		return 0.08f;
	else if (WeaponName == "weapon_negev")
		return 0.0008f;
	else if (WeaponName == "weapon_ssg08")
		return 1.25f;
	else if (WeaponName == "weapon_awp")
		return 1.463f;
	else if (WeaponName == "weapon_g3sg1")
		return 0.25f;
	else if (WeaponName == "weapon_scar20")
		return 0.25f;
	else if (WeaponName == "weapon_mp5sd")
		return 0.08f;
	else
		return .0f;

}
bool Tickbase::CanFireWithExploit(int m_iShiftedTick) {
	float curtime = TICKS_TO_TIME(localPlayer->tickBase() - m_iShiftedTick);
	return CanFireWeapon(curtime);
}
void Tickbase::updateInput() noexcept
{
	config->doubletapkey.handleToggle();
}