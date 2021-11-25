#include "Animations.h"
#include "EnginePrediction.h"

#include "../Memory.h"
#include "../Interfaces.h"

#include "../SDK/LocalPlayer.h"
#include "../SDK/Cvar.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/MemAlloc.h"
#include "../SDK/Input.h"

static std::array<Animations::Players, 65> players{};
static std::array<matrix3x4, MAXSTUDIOBONES> fakematrix{};
static bool updatingLocal{ true };
static bool updatingEntity{ false };
static bool updatingFake{ false };
static bool sendPacket{ true };
static bool gotMatrix{ false };
static Vector viewangles{};
static std::array<AnimationLayer, 13> staticLayers{};

void Animations::init() noexcept
{
    static auto jiggleBones = interfaces->cvar->findVar("r_jiggle_bones");
    if (jiggleBones->getInt() >= 1)
        jiggleBones->setValue(0);

    static auto extrapolate = interfaces->cvar->findVar("cl_extrapolate");
    if (extrapolate->getInt() >= 1)
        extrapolate->setValue(0);
}

void Animations::update(UserCmd* cmd, bool& _sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    viewangles = cmd->viewangles;
    sendPacket = _sendPacket;
    localPlayer->getAnimstate()->buttons = cmd->buttons;
}

void Animations::fake() noexcept
{
    static AnimState* fakeAnimState = nullptr;
    static bool updateFakeAnim = true;
    static bool initFakeAnim = true;
    static float spawnTime = 0.f;

    if (!localPlayer || !localPlayer->isAlive() || !localPlayer->getAnimstate())
        return;

    if (spawnTime != localPlayer->spawnTime() || updateFakeAnim)
    {
        spawnTime = localPlayer->spawnTime();
        initFakeAnim = false;
        updateFakeAnim = false;
    }

    if (!initFakeAnim)
    {
        fakeAnimState = static_cast<AnimState*>(memory->memalloc->Alloc(sizeof(AnimState)));

        if (fakeAnimState != nullptr)
            localPlayer->createState(fakeAnimState);

        initFakeAnim = true;
    }

    if (!fakeAnimState)
        return;

    if (sendPacket)
    {
        updatingFake = true;

        std::array<AnimationLayer, 13> layers;

        std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

        auto backupAbs = localPlayer->getAbsAngle();
        auto backupPoses = localPlayer->poseParameters();

        localPlayer->updateState(fakeAnimState, viewangles);
        memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeAnimState->footYaw, 0 });
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        localPlayer->getAnimationLayer(ANIMATION_LAYER_LEAN)->weight = std::numeric_limits<float>::epsilon();
        gotMatrix = localPlayer->setupBones(fakematrix.data(), MAXSTUDIOBONES, 0x7FF00, memory->globalVars->currenttime);
        const auto origin = localPlayer->getRenderOrigin();
        if (gotMatrix)
        {
            for (auto& i : fakematrix)
            {
                i[0][3] -= origin.x;
                i[1][3] -= origin.y;
                i[2][3] -= origin.z;
            }
        }
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        localPlayer->poseParameters() = backupPoses;
        memory->setAbsAngle(localPlayer.get(), Vector{ 0,backupAbs.y,0 });
        updatingFake = false;
    }
}

void Animations::real(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START)
        return;

    if (!localPlayer)
        return;

    static std::array<AnimationLayer, 13> layers{};

    if(layers.empty())
        std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

    static auto backupPoses = localPlayer->poseParameters();
    static auto backupAbs = localPlayer->getAnimstate()->footYaw;

    static int oldTick = 0;

    if (oldTick != memory->globalVars->tickCount)
    {
        oldTick = memory->globalVars->tickCount;
        updatingLocal = true;

        // allow animations to be animated in the same frame
        if (localPlayer->getAnimstate()->lastUpdateFrame == memory->globalVars->framecount)
            localPlayer->getAnimstate()->lastUpdateFrame -= 1;

        if (localPlayer->getAnimstate()->lastUpdateTime == memory->globalVars->currenttime)
            localPlayer->getAnimstate()->lastUpdateTime -= 1;

        localPlayer->updateState(localPlayer->getAnimstate(), viewangles);
        // updateClientSideAnimation calls animState update, but uses eyeAngles, 
        // note: after updating the animstate lastUpdateFrame will be equal to memory->globalVars->framecount, so it wont be executed
        // so no need to worry about it
        // a better aproach would be too hook eyeAngles an return cmd->viewangles
        localPlayer->updateClientSideAnimation();

        std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        std::memcpy(&staticLayers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        if (sendPacket)
        {
            backupPoses = localPlayer->poseParameters();
            backupAbs = localPlayer->getAnimstate()->footYaw;
        }
        updatingLocal = false;
        /*
        std::string a =
            "sequence: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).sequence)
            + "\n" + "order: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).order)
            + "\n" + "prevCycle: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).prevCycle)
            + "\n" + "weight" + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).weight)
            + "\n" + "weightDeltaRate: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).weightDeltaRate)
            + "\n" + "playbackRate: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).playbackRate)
            + "\n" + "cyle: " + std::to_string(layers.at(ANIMATION_LAYER_MOVEMENT_MOVE).cycle)
            + "\n" + "moveWeight: " + std::to_string(localPlayer->getAnimstate()->moveWeight)
            + "\n" + "primaryCycle: " + std::to_string(localPlayer->getAnimstate()->primaryCycle);
        memory->debugMsg(a.c_str());
        memory->debugMsg("\n");
        */
    }

    memory->setAbsAngle(localPlayer.get(), Vector{ 0, backupAbs, 0 });
    std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
    localPlayer->poseParameters() = backupPoses;
}

void Animations::handlePlayers(FrameStage stage) noexcept
{
    if (stage != FrameStage::NET_UPDATE_END)
        return;

    if (!localPlayer)
    {
        for (auto& record : players)
            record.clear();
        return;
    }
    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        auto& player = players.at(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        {
            player.clear();
            continue;
        }

        std::memcpy(&player.layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayersCount());

        const auto frameTime = memory->globalVars->frametime;
        const auto currentTime = memory->globalVars->currenttime;

        memory->globalVars->frametime = memory->globalVars->intervalPerTick;
        memory->globalVars->currenttime = entity->simulationTime();

        if (player.simulationTime != entity->simulationTime())
        {
            if (player.lastOrigin.notNull())
                player.velocity = (entity->origin() - player.lastOrigin) * (1.0f / fabsf(entity->simulationTime() - player.simulationTime));
            player.lastOrigin = entity->origin();

            if (entity->flags() & 1
                && player.layers[ANIMATION_LAYER_ALIVELOOP].weight > 0.f
                && player.layers[ANIMATION_LAYER_ALIVELOOP].weight < 1.f)
            {
                float velocityLengthXY = 0.f;
                auto weapon = entity->getActiveWeapon();
                float flMaxSpeedRun = weapon ? std::fmaxf(weapon->getMaxSpeed(), 0.001f) : CS_PLAYER_SPEED_RUN;

                auto modifier = 0.35f * (1.0f - player.layers[ANIMATION_LAYER_ALIVELOOP].weight);

                if (modifier > 0.f && modifier < 1.0f)
                    velocityLengthXY = flMaxSpeedRun * (modifier + 0.55f);

                if (velocityLengthXY != 0.f)
                {
                    velocityLengthXY = entity->velocity().length2D() / velocityLengthXY;

                    player.velocity.x *= velocityLengthXY;
                    player.velocity.y *= velocityLengthXY;
                }
            }

            if (player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate <= 0.f 
                || player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight <= 0.f 
                && entity->flags() & 1)
            {
                player.velocity.x = 0.f;
                player.velocity.y = 0.f;
            }

            if (entity->getAnimstate()->getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
                entity->getAnimstate()->getLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
            {
                player.velocity.x = std::clamp(entity->velocity().x, -1.f, 1.f);
                player.velocity.y = std::clamp(entity->velocity().y, -1.f, 1.f);
            }
        }

        entity->getEFlags() &= ~0x1000;
        entity->getAbsVelocity() = player.velocity;

        updatingEntity = true;
        entity->updateClientSideAnimation();
        updatingEntity = false;

        if (player.simulationTime != entity->simulationTime())
        {
            player.chokedPackets = static_cast<int>(fabsf(entity->simulationTime() - player.simulationTime) / memory->globalVars->intervalPerTick) - 1;
            player.simulationTime = entity->simulationTime();
            player.gotMatrix = entity->setupBones(player.matrix.data(), 256, 0x7FF00, memory->globalVars->currenttime);
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
        }

        std::memcpy(entity->animOverlays(), &player.layers, sizeof(AnimationLayer) * entity->getAnimationLayersCount());

        memory->globalVars->frametime = frameTime;
        memory->globalVars->currenttime = currentTime;
    }
}

void Animations::packetStart() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    //std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
}

void verifyLayer(int32_t activity) noexcept
{
    AnimationLayer currentlayer = *localPlayer->getAnimationLayer(activity);
    AnimationLayer previousLayer = staticLayers.at(activity);

    auto& layer = *localPlayer->getAnimationLayer(activity);
    if (!&layer)
        return;

    if (currentlayer.order != previousLayer.order)
    {
        layer.order = previousLayer.order;
    }

    if (currentlayer.sequence != previousLayer.sequence)
    {
        layer.sequence = previousLayer.sequence;
    }

    if (currentlayer.prevCycle != previousLayer.prevCycle)
    {
        layer.prevCycle = previousLayer.prevCycle;
    }

    if (currentlayer.weight != previousLayer.weight)
    {
        layer.weight = previousLayer.weight;
    }

    if (currentlayer.weightDeltaRate != previousLayer.weightDeltaRate)
    {
        layer.weightDeltaRate = previousLayer.weightDeltaRate;
    }

    if (currentlayer.playbackRate != previousLayer.playbackRate)
    {
        layer.playbackRate = previousLayer.playbackRate;
    }

    if (currentlayer.cycle != previousLayer.cycle)
    {
        layer.cycle = previousLayer.cycle;
    }
}

void Animations::packetEnd() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    for (int i = 0; i < 13; i++)
    {
        verifyLayer(i);
    }
}

bool Animations::isLocalUpdating() noexcept
{
    return updatingLocal;
}

bool Animations::isEntityUpdating() noexcept
{
    return updatingEntity;
}

bool Animations::isFakeUpdating() noexcept
{
    return updatingFake;
}

bool Animations::gotFakeMatrix() noexcept
{
    return gotMatrix;
}

std::array<matrix3x4, MAXSTUDIOBONES> Animations::getFakeMatrix() noexcept
{
    return fakematrix;
}

Animations::Players Animations::getPlayer(int index) noexcept
{
    return players.at(index);
}