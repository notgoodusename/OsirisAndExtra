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
#include "../SDK/Vector.h"

static std::array<Animations::Players, 65> players{};
static std::array<matrix3x4, MAXSTUDIOBONES> fakematrix{};
static std::array<matrix3x4, MAXSTUDIOBONES> realmatrix{};
static bool updatingLocal{ true };
static bool updatingEntity{ false };
static bool updatingFake{ false };
static bool sendPacket{ true };
static bool gotMatrix{ false };
static bool gotMatrixReal{ false };
static Vector viewangles{};
static std::array<AnimationLayer, 13> staticLayers{};
static std::array<AnimationLayer, 13> layers{};
static float primaryCycle{0.f};

static float footYaw{};
static std::array<float, 24> poseParameters{};
static std::array<AnimationLayer, 13> sendPacketLayers{};

void Animations::init() noexcept
{
    static auto threadedBoneSetup = interfaces->cvar->findVar("cl_threaded_bone_setup");
    threadedBoneSetup->setValue(1);

    static auto extrapolate = interfaces->cvar->findVar("cl_extrapolate");
    extrapolate->setValue(0);
}

void Animations::update(UserCmd* cmd, bool& _sendPacket) noexcept
{
    static float spawnTime = 0.f;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (interfaces->engine->isHLTV())
        return;

    if (spawnTime != localPlayer->spawnTime())
    {
        spawnTime = localPlayer->spawnTime();
        for (int i = 0; i < 13; i++)
        {
            if (i == ANIMATION_LAYER_FLINCH ||
                i == ANIMATION_LAYER_FLASHED ||
                i == ANIMATION_LAYER_WHOLE_BODY ||
                i == ANIMATION_LAYER_WEAPON_ACTION ||
                i == ANIMATION_LAYER_WEAPON_ACTION_RECROUCH)
            {
                continue;
            }
            auto& l = *localPlayer->getAnimationLayer(i);
            if (!&l)
                continue;
            l.reset();
        }
    }

    if (!localPlayer->getAnimstate())
        return;

    viewangles = cmd->viewangles;
    sendPacket = _sendPacket;
    localPlayer->getAnimstate()->buttons = cmd->buttons;

    updatingLocal = true;

    // allow animations to be animated in the same frame
    if (localPlayer->getAnimstate()->lastUpdateFrame == memory->globalVars->framecount)
        localPlayer->getAnimstate()->lastUpdateFrame -= 1;

    if (localPlayer->getAnimstate()->lastUpdateTime == memory->globalVars->currenttime)
        localPlayer->getAnimstate()->lastUpdateTime += ticksToTime(1);

    localPlayer->getEFlags() &= ~0x1000;
    localPlayer->getAbsVelocity() = EnginePrediction::getVelocity();

    localPlayer->updateState(localPlayer->getAnimstate(), viewangles);
    localPlayer->updateClientSideAnimation();

    std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

    if (sendPacket)
    {
        std::memcpy(&sendPacketLayers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        footYaw = localPlayer->getAnimstate()->footYaw;
        poseParameters = localPlayer->poseParameters();
        gotMatrixReal = localPlayer->setupBones(realmatrix.data(), localPlayer->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
        const auto origin = localPlayer->getRenderOrigin();
        if (gotMatrixReal)
        {
            for (auto& i : realmatrix)
            {
                i[0][3] -= origin.x;
                i[1][3] -= origin.y;
                i[2][3] -= origin.z;
            }
        }
    }
    updatingLocal = false;
}

void Animations::fake() noexcept
{
    static AnimState* fakeAnimState = nullptr;
    static bool updateFakeAnim = true;
    static bool initFakeAnim = true;
    static float spawnTime = 0.f;

    if (!localPlayer || !localPlayer->isAlive() || !localPlayer->getAnimstate())
        return;

    if (interfaces->engine->isHLTV())
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
        const auto backupAbs = localPlayer->getAbsAngle();
        const auto backupPoses = localPlayer->poseParameters();

        localPlayer->updateState(fakeAnimState, viewangles);
        if (fabsf(fakeAnimState->footYaw - footYaw) <= 5.f)
        {
            gotMatrix = false;
            updatingFake = false;

            std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
            localPlayer->poseParameters() = backupPoses;
            memory->setAbsAngle(localPlayer.get(), Vector{ 0,backupAbs.y,0 });
            return;
        }
        memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeAnimState->footYaw, 0 });
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        localPlayer->getAnimationLayer(ANIMATION_LAYER_LEAN)->weight = std::numeric_limits<float>::epsilon();
        
        gotMatrix = localPlayer->setupBones(fakematrix.data(), localPlayer->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
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

void Animations::renderStart(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START)
        return;

    if (!localPlayer)
        return;

    if (interfaces->engine->isHLTV())
        return;

    for (int i = 0; i < 13; i++)
    {
        if (i == ANIMATION_LAYER_FLINCH ||
            i == ANIMATION_LAYER_FLASHED ||
            i == ANIMATION_LAYER_WHOLE_BODY ||
            i == ANIMATION_LAYER_WEAPON_ACTION ||
            i == ANIMATION_LAYER_WEAPON_ACTION_RECROUCH)
        {
            continue;
        }
        auto& l = *localPlayer->getAnimationLayer(i);
        if (!&l)
            continue;
        l = layers.at(i);
    }
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

        if (entity->spawnTime() != player.spawnTime)
        {
            player.spawnTime = entity->spawnTime();
            player.reset();
        }

        std::array<AnimationLayer, 13> layers;

        std::memcpy(&layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayersCount());

        const auto frameTime = memory->globalVars->frametime;
        const auto currentTime = memory->globalVars->currenttime;

        memory->globalVars->frametime = memory->globalVars->intervalPerTick;
        memory->globalVars->currenttime = entity->simulationTime();

        bool runPostUpdate = false;

        if (player.simulationTime != entity->simulationTime())
        {
            runPostUpdate = true;
            if (player.simulationTime == -1.0f)
                player.simulationTime = entity->simulationTime();

            if (!player.layers.empty())
                std::memcpy(&player.oldlayers, &player.layers, sizeof(AnimationLayer) * entity->getAnimationLayersCount());
            std::memcpy(&player.layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayersCount());

            const auto simDifference = fabsf(entity->simulationTime() - player.simulationTime);

            player.simulationTime != entity->simulationTime() ?
                player.chokedPackets = static_cast<int>(fabsf(entity->simulationTime() - player.simulationTime) / memory->globalVars->intervalPerTick) - 1 : player.chokedPackets = 0;
            player.chokedPackets = std::clamp(player.chokedPackets, 0, maxUserCmdProcessTicks);

            if (player.origin.notNull())
                player.velocity = (entity->origin() - player.origin) * static_cast<float>(player.chokedPackets + 1);

            player.origin = entity->origin();

            if (entity->flags() & 1)
                player.velocity.z = 0.f;
            else
            {
                const float weight = 1.0f - player.layers[ANIMATION_LAYER_ALIVELOOP].weight;
                if (weight > 0.0f)
                {
                    const float previousRate = player.oldlayers[ANIMATION_LAYER_ALIVELOOP].playbackRate;
                    const float currentRate = player.layers[ANIMATION_LAYER_ALIVELOOP].playbackRate;

                    if (previousRate == currentRate)
                    {
                        const int previousSequence = player.oldlayers[ANIMATION_LAYER_ALIVELOOP].sequence;
                        const int currentSequence = player.layers[ANIMATION_LAYER_ALIVELOOP].sequence;

                        if (previousSequence == currentSequence)
                        {
                            const float speedNormalized = (weight / 2.8571432f) + 0.55f;
                            if (speedNormalized > 0.0f)
                            {
                                const auto weapon = entity->getActiveWeapon();
                                const float maxSpeed = weapon ? std::fmaxf(weapon->getMaxSpeed(), 0.001f) : CS_PLAYER_SPEED_RUN;
                                const float speed = speedNormalized * maxSpeed;
                                if (speed > 0.0f && player.velocity.length2D() > 0.0f)
                                    player.velocity = (player.velocity / player.velocity.length()) * speed;
                            }
                        }
                    }
                }
                static auto gravity = interfaces->cvar->findVar("sv_gravity");
                player.velocity.z -= gravity->getFloat() * 0.5f * ticksToTime(player.chokedPackets);
            }

            if (entity->flags() & 1
                && player.layers[ANIMATION_LAYER_ALIVELOOP].weight > 0.f
                && player.layers[ANIMATION_LAYER_ALIVELOOP].weight < 1.f
                && player.layers[ANIMATION_LAYER_ALIVELOOP].cycle > player.oldlayers[ANIMATION_LAYER_ALIVELOOP].cycle)
            {
                float velocityLengthXY = 0.f;
                const auto weapon = entity->getActiveWeapon();
                const float maxSpeedRun = weapon ? std::fmaxf(weapon->getMaxSpeed(), 0.001f) : CS_PLAYER_SPEED_RUN;

                const auto modifier = 0.35f * (1.0f - player.layers[ANIMATION_LAYER_ALIVELOOP].weight);

                if (modifier > 0.f && modifier < 1.0f)
                    velocityLengthXY = maxSpeedRun * (modifier + 0.55f);

                if (velocityLengthXY > 0.f)
                {
                    velocityLengthXY = entity->velocity().length2D() / velocityLengthXY;

                    player.velocity.x *= velocityLengthXY;
                    player.velocity.y *= velocityLengthXY;
                }
            }

            if ((player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate <= 0.f
                || player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight <= 0.f)
                && entity->flags() & 1)
            {
                player.velocity.x = 0.f;
                player.velocity.y = 0.f;
            }
        }

        entity->getEFlags() &= ~0x1000;
        entity->getAbsVelocity() = player.velocity;

        updatingEntity = true;
        entity->updateClientSideAnimation();
        updatingEntity = false;

        if (runPostUpdate)
        {
            if (!(entity->flags() & 1) && !player.oldlayers.empty())// && entity->moveType() != MoveType::NOCLIP)
            {
                const auto currentActivity = entity->getAnimstate()->getLayerActivity(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
                if (currentActivity == ACT_CSGO_FALL || currentActivity == ACT_CSGO_JUMP)
                {
                    std::array<AnimationLayer, 13> backupLayers;

                    std::memcpy(&backupLayers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayersCount());
                    std::memcpy(entity->animOverlays(), &player.oldlayers, sizeof(AnimationLayer) * entity->getAnimationLayersCount());
                    const auto oldActivity = entity->getAnimstate()->getLayerActivity(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
                    std::memcpy(entity->animOverlays(), &backupLayers, sizeof(AnimationLayer) * entity->getAnimationLayersCount());

                    float cycle;
                    if (oldActivity != currentActivity)
                    {
                        entity->getAnimstate()->durationInAir = 0.0f;
                        cycle = 0.0f;
                    }
                    else if (player.layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].cycle < player.oldlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].cycle)
                    {
                        entity->getAnimstate()->durationInAir = 0.0f;
                        cycle = 0.0f;
                    }
                    else
                    {
                        cycle = player.oldlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].cycle;
                    }

                    int ticks = entity->getAnimstate()->getTicksFromCycle(player.layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].playbackRate,
                        player.layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].cycle, cycle);

                    if (ticks == 0 || player.layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].cycle >= 0.999f)
                    {
                        //if animation is finished already, fix up the total time in air
                        const int totalCommands = player.chokedPackets + 1;
                        const int ticksLeft = totalCommands - ticks;
                        ticks += ticksLeft;
                    }

                    entity->getAnimstate()->durationInAir = entity->getAnimstate()->lastUpdateIncrement * ticks;
                    entity->getAnimstate()->poseParamMappings[PLAYER_POSE_PARAM_JUMP_FALL].setValue(entity, std::clamp(Helpers::smoothStepBounds(0.72f, 1.52f, entity->getAnimstate()->durationInAir), 0.f, 1.f));
                }
            }
        }

        std::memcpy(entity->animOverlays(), &layers, sizeof(AnimationLayer)* entity->getAnimationLayersCount());

        if (runPostUpdate)
        {
            player.simulationTime = entity->simulationTime();
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
            player.gotMatrix = entity->setupBones(player.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
        }

        memory->globalVars->frametime = frameTime;
        memory->globalVars->currenttime = currentTime;
    }
}

void Animations::packetStart() noexcept
{
    if (!localPlayer || !localPlayer->animOverlays())
        return;

    if (interfaces->engine->isHLTV())
        return;

    std::memcpy(&staticLayers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

    if (!localPlayer->getAnimstate())
        return;

    primaryCycle = localPlayer->getAnimstate()->primaryCycle;
}

void verifyLayer(int32_t layer) noexcept
{
    AnimationLayer currentlayer = *localPlayer->getAnimationLayer(layer);
    AnimationLayer previousLayer = staticLayers.at(layer);

    auto& l = *localPlayer->getAnimationLayer(layer);
    if (!&l)
        return;

    if (currentlayer.order != previousLayer.order)
    {
        l.order = previousLayer.order;
    }

    if (currentlayer.sequence != previousLayer.sequence)
    {
        l.sequence = previousLayer.sequence;
    }

    if (currentlayer.prevCycle != previousLayer.prevCycle)
    {
        l.prevCycle = previousLayer.prevCycle;
    }

    if (currentlayer.weight != previousLayer.weight)
    {
        l.weight = previousLayer.weight;
    }

    if (currentlayer.weightDeltaRate != previousLayer.weightDeltaRate)
    {
        l.weightDeltaRate = previousLayer.weightDeltaRate;
    }

    if (currentlayer.playbackRate != previousLayer.playbackRate)
    {
        l.playbackRate = previousLayer.playbackRate;
    }

    if (currentlayer.cycle != previousLayer.cycle)
    {
        l.cycle = previousLayer.cycle;
    }
}

void Animations::postDataUpdate() noexcept
{
    if (!localPlayer || !localPlayer->animOverlays())
        return;

    if (interfaces->engine->isHLTV())
        return;

    for (int i = 0; i < 13; i++)
    {
        if (i == ANIMATION_LAYER_FLINCH ||
            i == ANIMATION_LAYER_FLASHED ||
            i == ANIMATION_LAYER_WHOLE_BODY ||
            i == ANIMATION_LAYER_WEAPON_ACTION ||
            i == ANIMATION_LAYER_WEAPON_ACTION_RECROUCH)
        {
            continue;
        }
        verifyLayer(i);
    }

    if (!localPlayer->getAnimstate())
        return;

    localPlayer->getAnimstate()->primaryCycle = primaryCycle;
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

bool Animations::gotRealMatrix() noexcept
{
    return gotMatrixReal;
}

std::array<matrix3x4, MAXSTUDIOBONES> Animations::getRealMatrix() noexcept
{
    return realmatrix;
}

float Animations::getFootYaw() noexcept
{
    return footYaw;
}

std::array<float, 24> Animations::getPoseParameters() noexcept
{
    return poseParameters;
}

std::array<AnimationLayer, 13> Animations::getAnimLayers() noexcept
{
    return sendPacketLayers;
}

Animations::Players Animations::getPlayer(int index) noexcept
{
    return players.at(index);
}