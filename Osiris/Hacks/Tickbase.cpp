#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "Tickbase.h"

#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"

/*
For those confused as to why i hooked clMove and writeUserCmdDelta, its simple
For teleport you run all the ticks you send, so you need to create commands for each cmd sent, basically run clMove multiple times
And without teleport you dont need to run commands, since the commands sent wont be ran (because it will only mess with tickbase)
*/

int targetTickShift{ 0 };
int tickShift{ 0 };
int shiftCommand{ 0 };
int shiftedTickbase{ 0 };
int ticksAllowedForProcessing{ 0 };
int chokedPackets{ 0 };
int pauseTicks{ 0 };
float realTime{ 0.0f };
bool shifting{ false };
bool finalTick{ false };
bool hasHadTickbaseActive{ false };

void Tickbase::start(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
    {
        hasHadTickbaseActive = false;
        return;
    }
    if (cmd->buttons & UserCmd::IN_ATTACK)
    {
        config->tickbase.lastFireShiftTick = memory->globalVars->tickCount + config->tickbase.onshotFlAmount;
        config->tickbase.readyFire = true;

    }/*
    if (cmd->viewangles.x < 85.f)
        config->tickbase.readyFire = true;
    else
        config->tickbase.readyFire = false;*/
    if (!config->tickbase.doubletap.isActive() && !config->tickbase.hideshots.isActive())
        config->tickbase.DisabledTickbase = true;
    else
        config->tickbase.DisabledTickbase = false;
    if (const auto netChannel = interfaces->engine->getNetworkChannel(); netChannel)

        if (netChannel->chokedPackets > chokedPackets)
            chokedPackets = netChannel->chokedPackets;

    if (!config->tickbase.doubletap.isActive() && !config->tickbase.hideshots.isActive())
    {
        if (hasHadTickbaseActive)
            shift(cmd, ticksAllowedForProcessing, true);
        hasHadTickbaseActive = false;
        return;
    }

    if (config->tickbase.doubletap.isActive())
        targetTickShift = 13;
    else if (config->tickbase.hideshots.isActive())
        targetTickShift = 9;

    //We do -1 to leave 1 tick to fakelag
    targetTickShift = std::clamp(targetTickShift, 0, maxUserCmdProcessTicks - 1);
    hasHadTickbaseActive = true;
}

void Tickbase::end(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config->tickbase.doubletap.isActive() && !config->tickbase.hideshots.isActive())
    {
        targetTickShift = 0;
        return;
    }

    if (cmd->buttons & UserCmd::IN_ATTACK)
       shift(cmd, targetTickShift);
}

bool Tickbase::shift(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShift(shiftAmount, forceShift))
        return false;

    realTime = memory->globalVars->realtime;
    shiftedTickbase = shiftAmount;
    shiftCommand = cmd->commandNumber;
    tickShift = shiftAmount;
    return true;
}

bool Tickbase::canRun() noexcept
{
    static float spawnTime = 0.f;
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
    {
        ticksAllowedForProcessing = 0;
        chokedPackets = 0;
        pauseTicks = 0;
        return true;
    }

    if (!localPlayer || !localPlayer->isAlive() || !targetTickShift)
    {
        ticksAllowedForProcessing = 0;
        return true;
    }

    if ((*memory->gameRules)->freezePeriod())
    {
        realTime = memory->globalVars->realtime;
        return true;
    }

    if (spawnTime != localPlayer->spawnTime())
    {
        spawnTime = localPlayer->spawnTime();
        ticksAllowedForProcessing = 0;
        pauseTicks = 0;
    }

    if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
    {
        realTime = memory->globalVars->realtime;
        return true;
    }

    if ((ticksAllowedForProcessing < targetTickShift || chokedPackets > maxUserCmdProcessTicks - targetTickShift) && memory->globalVars->realtime - realTime > 1.0f)
    {
        ticksAllowedForProcessing = min(ticksAllowedForProcessing++, maxUserCmdProcessTicks);
        chokedPackets = max(chokedPackets--, 0);
        pauseTicks++;
        return false;
    }
    return true;
}

bool Tickbase::canShift(int shiftAmount, bool forceShift) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;

    if (!shiftAmount || shiftAmount > ticksAllowedForProcessing || memory->globalVars->realtime - realTime <= 0.5f)
        return false;

    if (forceShift)
        return true;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->isKnife() || activeWeapon->isGrenade() || activeWeapon->isBomb()
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Taser
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Healthshot)
        return false;

    const float shiftTime = (localPlayer->tickBase() - shiftAmount) * memory->globalVars->intervalPerTick;
    if (localPlayer->nextAttack() > shiftTime)
        return false;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return false;

	return activeWeapon->nextPrimaryAttack() <= shiftTime;
}

int Tickbase::getCorrectTickbase(int commandNumber) noexcept
{
    const int tickBase = localPlayer->tickBase();

    if (commandNumber == shiftCommand)
        return tickBase - shiftedTickbase;
    else if (commandNumber == shiftCommand + 1)
    {
        if (!config->tickbase.teleport)
            return tickBase + shiftedTickbase;
        return tickBase;
    }
    if (pauseTicks)
        return tickBase + pauseTicks;
	return tickBase;
}

int& Tickbase::pausedTicks() noexcept
{
    return pauseTicks;
}

//If you have dt enabled, you need to shift 13 ticks, so it will return 13 ticks
//If you have hs enabled, you need to shift 9 ticks, so it will return 7 ticks
int Tickbase::getTargetTickShift() noexcept
{
	return targetTickShift;
}

int Tickbase::getTickshift() noexcept
{
	return tickShift;
}

void Tickbase::resetTickshift() noexcept
{
	shiftedTickbase = tickShift;
    //Without teleport we only need to recharge after fakelagging
    if (config->tickbase.teleport)
        ticksAllowedForProcessing = max(ticksAllowedForProcessing - tickShift, 0);
	tickShift = 0;
}

bool& Tickbase::isFinalTick() noexcept
{
    return finalTick;
}

bool& Tickbase::isShifting() noexcept
{
    return shifting;
}

void Tickbase::updateInput() noexcept
{
    config->tickbase.doubletap.handleToggle();
    config->tickbase.hideshots.handleToggle();
}

void Tickbase::reset() noexcept
{
    hasHadTickbaseActive = false;
    pauseTicks = 0;
    chokedPackets = 0;
    tickShift = 0;
    shiftCommand = 0;
    shiftedTickbase = 0;
    ticksAllowedForProcessing = 0;
    realTime = 0.0f;
}
