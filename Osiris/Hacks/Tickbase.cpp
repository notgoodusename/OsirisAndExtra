#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/Localplayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/GameEvent.h"
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

		auto activeWeapon = localPlayer->getActiveWeapon();
		if (!activeWeapon)
			return;

		if (activeWeapon->isGrenade() || activeWeapon->itemDefinitionIndex2() == WeaponId::C4)
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
		if (cmd->buttons & UserCmd::IN_USE)
			return;
		

		maxTicks = ((*memory->gameRules)->isValveDS()) ? 8 : 19;
		if (!localPlayer || !localPlayer->isAlive())
			return;
		if (!warmup)
		{
			if ((config->doubletapkey.isActive() && !config->hideshotskey.isActive()) || (config->hideshotskey.isActive() && dotelepeek))
			{
				A:
				int ticksPerShot = max(0, static_cast<int>(weaponData->cycletime / memory->globalVars->intervalPerTick));
				//If it has been clamped dont remove ticks
				if (ticksPerShot > maxTicks)
					ticksPerShot = maxTicks;
				else //If it has not been clamped you need to do -2 ticks to get prediction to work well
					ticksPerShot -= 2;
				if (dotelepeek)
					ticksPerShot = 7;
				if (maxTicks > 12)
				{
					interfaces->cvar->findVar("sv_maxusrcmdprocessticks")->setValue(maxTicks);
					shiftAmount = std::clamp(ticksPerShot, 0, maxTicks);
					interfaces->cvar->findVar("cl_clock_correction")->setValue(0);
				}
				else 
				{
					shiftAmount = std::clamp(ticksPerShot, 0, maxTicks - netChannel->chokedPackets);
				}
				if (shiftAmount <= doubletapCharge)
				{ 
					if (CanFireWithExploit(lastShift))
					{
						if ((memory->globalVars->currenttime - (Firerate() - 0.05f)) >= activeWeapon->lastshottime() || dotelepeek)
						{
							if ((cmd->buttons & UserCmd::IN_ATTACK) || dotelepeek)
							{
								if (localPlayer && localPlayer->isAlive())
								{
									if (dotelepeek)
									{
										dotelepeek = false;
									}
									ticksToShift = shiftAmount;
									lastShift = shiftAmount;
									commandNumber = cmd->commandNumber;
								}
							}
						}
					}
				}
			}
			else if (!config->doubletapkey.isActive() && config->hideshotskey.isActive() && !dotelepeek)
			{
				int hideticks = ((*memory->gameRules)->isValveDS()) ? -6 : -9;
				int ticksPerShot = max(0, static_cast<int>(weaponData->cycletime / memory->globalVars->intervalPerTick));
				ticksPerShot = max(-ticksPerShot, hideticks);

				if (localPlayer && localPlayer->isAlive())
					shiftAmount = std::clamp(ticksPerShot, hideticks, maxTicks - netChannel->chokedPackets);
				if (CanHideWithExploit(lastShift))
				{
					if (doubletapCharge > 1)
						doubletapCharge = 0;

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
			else if (config->doubletapkey.isActive() && config->hideshotskey.isActive())
			{
				goto A;
			}
		}
	}
	
}

void Tickbase::getEvent(GameEvent* event) noexcept
{
	if (!event || !localPlayer || interfaces->engine->isHLTV())
		return;

	switch (fnv::hashRuntime(event->getName())) {
	case fnv::hash("round_prestart"):
	{
		warmup = true;
		break;
	}
	case fnv::hash("round_end"):
	{
		warmup = true;
		break;
	}
	case fnv::hash("round_freeze_end"):
	{
		warmup = false;
		break;
	}
	case fnv::hash("player_death"):
	{
		if (event->getInt("userid") != localPlayer->getUserId())
			break;
		warmup = true;
		break;
	}
	case fnv::hash("cs_win_panel_match"):
	{
		warmup = true;
		break;
	}
	case fnv::hash("cs_intermission"):
	{
		warmup = true;
		break;
	}
	default:
		break;
	}
}

bool Tickbase::CanDT() {

	if (warmup)
		return false;

	if (config->misc.fakeduckKey.isActive())
		return false;

	if (!localPlayer || !localPlayer->isAlive())
		return false;

	if (!config->tickbase.enabled)
		return false;

	if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
		return false;

	if (localPlayer->moveType() == MoveType::LADDER || localPlayer->moveType() == MoveType::NOCLIP)
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
void Tickbase::Telepeek()
{
	if (!dotelepeek){ dotelepeek = true; }
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
bool Tickbase::CanHideWeapon(float curtime) {
	if (!localPlayer || !localPlayer->isAlive())
		return false;

	auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return false;

	if (curtime >= activeWeapon->nextPrimaryAttack()) {
			return true;
	}

	return false;
}
float Tickbase::Firerate()
{
	if (!Tickbase::DTWeapon())
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
bool Tickbase::CanHideWithExploit(int m_iShiftedTick) {
	float curtime = TICKS_TO_TIME(localPlayer->tickBase() - m_iShiftedTick);
	return CanFireWeapon(curtime);
}

void Tickbase::updateEventListeners(bool forceRemove) noexcept
{
	class ImpactEventListener : public GameEventListener {
	public:
		void fireGameEvent(GameEvent* event) {
			getEvent(event);
		}
	};

	static ImpactEventListener listener[5]; 
	static bool listenerRegistered = false;

	if (config->tickbase.enabled && !listenerRegistered) {
		interfaces->gameEventManager->addListener(&listener[0], "round_prestart");
		interfaces->gameEventManager->addListener(&listener[1], "round_end");
		interfaces->gameEventManager->addListener(&listener[2], "round_freeze_end");
		interfaces->gameEventManager->addListener(&listener[3], "player_death");
		interfaces->gameEventManager->addListener(&listener[4], "cs_win_panel_match");
		interfaces->gameEventManager->addListener(&listener[5], "cs_intermission");
		listenerRegistered = true;
	}

	else if ((!config->tickbase.enabled || forceRemove) && listenerRegistered) {
		interfaces->gameEventManager->removeListener(&listener[0]);
		interfaces->gameEventManager->removeListener(&listener[1]);
		interfaces->gameEventManager->removeListener(&listener[2]);
		interfaces->gameEventManager->removeListener(&listener[3]);
		interfaces->gameEventManager->removeListener(&listener[4]);
		interfaces->gameEventManager->removeListener(&listener[5]);
		listenerRegistered = false;
	}
}
void Tickbase::updateInput() noexcept
{
	config->doubletapkey.handleToggle();
	config->hideshotskey.handleToggle();
}