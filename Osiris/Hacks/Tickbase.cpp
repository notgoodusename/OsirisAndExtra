#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/ClientState.h"

Tickbase::Tick Tickbase::tick;

bool canShift(int ticks) noexcept
{
    if (!localPlayer || !localPlayer->isAlive() || ticks <= 0)
        return false;

    if(localPlayer->nextAttack() > memory->globalVars->serverTime())
        return false;

    float nextAttack = (localPlayer->nextAttack() + (ticks * memory->globalVars->intervalPerTick));
    if (nextAttack >= memory->globalVars->serverTime())
        return false;

    auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->isGrenade() || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver) // we wanna shift everything we can, except revolver and grenades
        return false;

    float shiftTime = (localPlayer->tickBase() - ticks) * memory->globalVars->intervalPerTick;
    if (shiftTime < activeWeapon->nextPrimaryAttack())
        return false;

    return true;
}

void Tickbase::shiftTicks(int ticks, UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!canShift(ticks))
        return;

    tick.commandNumber = cmd->commandNumber;
    tick.tickBase = localPlayer->tickBase();
    tick.tickshift = ticks;
    tick.lastTickShift = ticks;
}

void Tickbase::run(UserCmd* cmd) noexcept
{
    tick.tickCount = cmd->tickCount;
    if (!localPlayer)
        return;

    if ((*memory->gameRules)->isValveDS())
        tick.maxUsercmdProcessticks = 8;
    else
        tick.maxUsercmdProcessticks = 16;

    if (!localPlayer->isAlive())
    {
        tick.recharge = tick.maxUsercmdProcessticks;
        return;
    }

    if (memory->clientState && tick.chokedCommands < memory->clientState->chokedCommands)
        tick.chokedCommands = memory->clientState->chokedCommands;

    if (Tickbase::tick.recharge > 0 || !config->tickBase.enabled) // Dont shift if we still have to recharge
        return;

    int ticks = 0;

    if (cmd->buttons & (UserCmd::IN_ATTACK))
        ticks = config->tickBase.tickShift;

    if (Tickbase::tick.chokedCommands + ticks > Tickbase::tick.maxUsercmdProcessticks)
    {
        if(ticks <= Tickbase::tick.maxUsercmdProcessticks)
            Tickbase::tick.recharge = Tickbase::tick.chokedCommands;
        return;
    }

    if (Tickbase::tick.chokedCommands >= 3)
    {
        Tickbase::tick.recharge = Tickbase::tick.chokedCommands;
        return;
    }

    if(cmd->buttons & (UserCmd::IN_ATTACK))
        shiftTicks(ticks, cmd);
}