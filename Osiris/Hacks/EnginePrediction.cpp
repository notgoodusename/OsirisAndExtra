#include "../Interfaces.h"
#include "../Memory.h"

#include "../SDK/ClientState.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameMovement.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/MoveHelper.h"
#include "../SDK/Prediction.h"
#include "../SDK/PredictionCopy.h"

#include "EnginePrediction.h"

static int localPlayerFlags;
static Vector localPlayerVelocity;
static std::array<EnginePrediction::NetvarData, 150> netvarData;
static void* storedData = nullptr;

void EnginePrediction::reset() noexcept
{
    localPlayerFlags = {};
    localPlayerVelocity = Vector{};
    netvarData = {};
    storedData = nullptr;
}

void EnginePrediction::update() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto deltaTick = memory->clientState->deltaTick;
    const auto start = memory->clientState->lastCommandAck;
    const auto stop = memory->clientState->lastOutgoingCommand + memory->clientState->chokedCommands;
    interfaces->prediction->update(deltaTick, deltaTick > 0, start, stop);
}

void EnginePrediction::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    inPrediction = true;
    
    localPlayerFlags = localPlayer->flags();
    localPlayerVelocity = localPlayer->velocity();

    *memory->predictionRandomSeed = 0;

    const auto oldCurrenttime = memory->globalVars->currenttime;
    const auto oldFrametime = memory->globalVars->frametime;

    memory->globalVars->currenttime = memory->globalVars->serverTime();
    memory->globalVars->frametime = memory->globalVars->intervalPerTick;

    memory->moveHelper->setHost(localPlayer.get());
    interfaces->prediction->setupMove(localPlayer.get(), cmd, memory->moveHelper, memory->moveData);
    interfaces->gameMovement->processMovement(localPlayer.get(), memory->moveData);
    interfaces->prediction->finishMove(localPlayer.get(), cmd, memory->moveData);

    inPrediction = false;

    memory->moveHelper->setHost(nullptr);

    *memory->predictionRandomSeed = -1;

    memory->globalVars->currenttime = oldCurrenttime;
    memory->globalVars->frametime = oldFrametime;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || activeWeapon->isGrenade() || activeWeapon->isKnife())
        return;

    activeWeapon->updateAccuracyPenalty();
}

void EnginePrediction::save() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!storedData)
    {
        const auto allocSize = localPlayer->getIntermidateDataSize();
        storedData = new byte[allocSize];
    }

    if (!storedData)
        return;

    PredictionCopy helper(PC_EVERYTHING, (byte*)storedData, true, (byte*)localPlayer.get(), false, PredictionCopy::TRANSFERDATA_COPYONLY, NULL);
    helper.TransferData("EnginePrediction::save", localPlayer->index(), localPlayer->getPredDescMap());
}

void EnginePrediction::restore() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!storedData)
        return;

    PredictionCopy helper(PC_EVERYTHING, (byte*)localPlayer.get(), false, (byte*)storedData, true, PredictionCopy::TRANSFERDATA_COPYONLY, NULL);
    helper.TransferData("EnginePrediction::restore", localPlayer->index(), localPlayer->getPredDescMap());
}

void EnginePrediction::store() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    int tickbase = localPlayer->tickBase();

    NetvarData netvars{ };

    netvars.tickbase = tickbase;

    netvars.aimPunchAngle = localPlayer->aimPunchAngle();
    netvars.aimPunchAngleVelocity = localPlayer->aimPunchAngleVelocity();
    netvars.viewPunchAngle = localPlayer->viewPunchAngle();
    netvars.viewOffset = localPlayer->viewOffset();

    netvarData.at(tickbase % 150) = netvars;
}

void EnginePrediction::apply(FrameStage stage) noexcept
{
    if (stage != FrameStage::NET_UPDATE_END)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (netvarData.empty())
        return;

    int tickbase = localPlayer->tickBase();

    auto netvars = netvarData.at(tickbase % 150);

    if (!&netvars)
        return;

    if (netvars.tickbase != tickbase)
        return;

    localPlayer->aimPunchAngle() = NetvarData::checkDifference(localPlayer->aimPunchAngle(), netvars.aimPunchAngle);
    localPlayer->aimPunchAngleVelocity() = NetvarData::checkDifference(localPlayer->aimPunchAngleVelocity(), netvars.aimPunchAngleVelocity);
    localPlayer->viewPunchAngle() = NetvarData::checkDifference(localPlayer->viewPunchAngle(), netvars.viewPunchAngle);
    localPlayer->viewOffset() = NetvarData::checkDifference(localPlayer->viewOffset(), netvars.viewOffset);
}

int EnginePrediction::getFlags() noexcept
{
    return localPlayerFlags;
}

Vector EnginePrediction::getVelocity() noexcept
{
    return localPlayerVelocity;
}