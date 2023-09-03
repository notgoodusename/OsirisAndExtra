#include "../Memory.h"
#include "../Interfaces.h"

#include "Animations.h"
#include "Backtrack.h"
#include "EnginePrediction.h"
#include "Resolver.h"

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
static std::array<matrix3x4, MAXSTUDIOBONES> fakelagmatrix{};
static std::array<matrix3x4, MAXSTUDIOBONES> realmatrix{};
static Vector localAngle{};
static bool updatingLocal{ true };
static bool updatingEntity{ false };
static bool updatingFake{ false };
static bool sendPacket{ true };
static bool gotMatrix{ false };
static bool gotMatrixFakelag{ false };
static bool gotMatrixReal{ false };
static Vector viewangles{};
static Vector correctAngle{};
static int buildTransformsIndex = -1;
static std::array<AnimationLayer, 13> staticLayers{};
static std::array<AnimationLayer, 13> layers{};
static float primaryCycle{ 0.0f };
static float moveWeight{ 0.0f };
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

void Animations::reset() noexcept
{
    for (auto& record : players)
        record.reset();
    fakematrix = {};
    fakelagmatrix = {};
    localAngle = Vector{};
    updatingLocal = true;
    updatingEntity = false;
    sendPacket = true;
    gotMatrix = false;
    gotMatrixFakelag = false;
    gotMatrixReal = false;
    viewangles = Vector{};
    correctAngle = Vector{};
    buildTransformsIndex = -1;
    staticLayers = {};
    layers = {};
    primaryCycle = 0.0f;
    moveWeight = 0.0f;
    footYaw = {};
    poseParameters = {};
    sendPacketLayers = {};
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
        localAngle = cmd->viewangles;
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

            memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeAnimState->footYaw, 0 });
            std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
            localPlayer->getAnimationLayer(ANIMATION_LAYER_LEAN)->weight = std::numeric_limits<float>::epsilon();

            gotMatrixFakelag = localPlayer->setupBones(fakelagmatrix.data(), localPlayer->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);

            std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
            localPlayer->poseParameters() = backupPoses;
            memory->setAbsAngle(localPlayer.get(), Vector{ 0,backupAbs.y,0 });
            return;
        }
        memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeAnimState->footYaw, 0 });
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        localPlayer->getAnimationLayer(ANIMATION_LAYER_LEAN)->weight = std::numeric_limits<float>::epsilon();
        
        gotMatrix = localPlayer->setupBones(fakematrix.data(), localPlayer->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
        gotMatrixFakelag = gotMatrix;
        if (gotMatrix)
        {
            std::copy(fakematrix.begin(), fakematrix.end(), fakelagmatrix.data());
            const auto origin = localPlayer->getRenderOrigin();
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

float getExtraTicks() noexcept
{
    if (!config->backtrack.fakeLatency || config->backtrack.fakeLatencyAmount <= 0)
        return 0.f;
    return static_cast<float>(config->backtrack.fakeLatencyAmount) / 1000.f;
}

void Animations::handlePlayers(FrameStage stage) noexcept
{
    static auto gravity = interfaces->cvar->findVar("sv_gravity");
    const float timeLimit = static_cast<float>(config->backtrack.timeLimit) / 1000.f + getExtraTicks();
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
        const auto entity = interfaces->entityList->getEntity(i);
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

        const uintptr_t backupEffects = entity->getEffects();

        entity->getEffects() |= 8;

        bool runPostUpdate = false;

        if (player.simulationTime != entity->simulationTime() && player.simulationTime < entity->simulationTime())
        {
            runPostUpdate = true;
            if (player.simulationTime == -1.0f)
                player.simulationTime = entity->simulationTime();

            if (!player.layers.empty())
                std::memcpy(&player.oldlayers, &player.layers, sizeof(AnimationLayer) * entity->getAnimationLayersCount());
            std::memcpy(&player.layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayersCount());

            //Get chokedPackets

            const auto simDifference = fabsf(entity->simulationTime() - player.simulationTime);

            player.simulationTime != entity->simulationTime() ?
                player.chokedPackets = static_cast<int>(simDifference / memory->globalVars->intervalPerTick) - 1 : player.chokedPackets = 0;
            player.chokedPackets = std::clamp(player.chokedPackets, 0, maxUserCmdProcessTicks + 1);

            //Velocity values
            if (player.origin.notNull() && player.simulationTime != entity->simulationTime())
            {
                player.oldVelocity = player.velocity;
                player.velocity = (entity->origin() - player.origin) * (1.0f / simDifference);
            }

            //Misc variables
            player.moveWeight = entity->getAnimstate()->moveWeight;
            player.flags = entity->flags();

            if (player.simulationTime == entity->simulationTime())
            {
                player.duckAmount = entity->duckAmount();
                player.oldDuckAmount = player.duckAmount;
                player.origin = entity->origin();
                player.oldOrigin = player.origin;
            }
            else
            {
                player.oldDuckAmount = player.duckAmount;
                player.duckAmount = entity->duckAmount();
                player.oldOrigin = player.origin;
                player.origin = entity->origin();
            }

            //Velocity calculations
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

            Resolver::runPreUpdate(player, entity);

            //Run animations

            updatingEntity = true;
            if (player.chokedPackets <= 0) //We dont need to simulate commands
            {
                if (entity->getAnimstate()->lastUpdateFrame == memory->globalVars->framecount)
                    entity->getAnimstate()->lastUpdateFrame -= 1;

                if (entity->getAnimstate()->lastUpdateTime == memory->globalVars->currenttime)
                    entity->getAnimstate()->lastUpdateTime += ticksToTime(1);

                entity->getEFlags() &= ~0x1000;
                entity->getAbsVelocity() = player.velocity;
                entity->updateClientSideAnimation();
            }
            else
            {
                //Simulate missing ticks
                //TODO: Improve this drastically
                for (int i = 1; i <= player.chokedPackets + 1; i++)
                {
                    const float simulatedTime = player.simulationTime + (memory->globalVars->intervalPerTick * i);
                    const float lerpValue = 1.f - (entity->simulationTime() - simulatedTime) / (entity->simulationTime() - player.simulationTime);
                    const float currentTimeBackup = memory->globalVars->currenttime;

                    memory->globalVars->currenttime = simulatedTime;

                    entity->getEFlags() &= ~0x1000;
                    entity->getAbsVelocity() = Helpers::lerp(lerpValue, player.velocity, player.oldVelocity);
                    entity->duckAmount() = Helpers::lerp(lerpValue, player.duckAmount, player.oldDuckAmount);

                    if (entity->getAnimstate()->lastUpdateFrame == memory->globalVars->framecount)
                        entity->getAnimstate()->lastUpdateFrame -= 1;

                    if (entity->getAnimstate()->lastUpdateTime == memory->globalVars->currenttime)
                        entity->getAnimstate()->lastUpdateTime += ticksToTime(1);

                    entity->updateClientSideAnimation();

                    memory->globalVars->currenttime = currentTimeBackup;
                }
                entity->getEFlags() &= ~0x1000;
                entity->getAbsVelocity() = player.velocity;
                entity->duckAmount() = player.duckAmount;
            }
            updatingEntity = false;

            Resolver::runPostUpdate(player, entity);

            //Fix jump pose
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

        //Setupbones
        if (runPostUpdate)
        {
            player.simulationTime = entity->simulationTime();
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
            player.gotMatrix = entity->setupBones(player.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
        }

        memory->globalVars->frametime = frameTime;
        memory->globalVars->currenttime = currentTime;

        entity->getEffects() = backupEffects;

        //Backtrack records

        if (!config->backtrack.enabled || !entity->isOtherEnemy(localPlayer.get()))
        {
            player.backtrackRecords.clear();
            continue;
        }

        if (runPostUpdate)
        {
            if (!player.backtrackRecords.empty() && (player.backtrackRecords.front().simulationTime == entity->simulationTime()))
                continue;

            Players::Record record{ };
            record.origin = player.origin;
            record.absAngle = player.absAngle;
            record.simulationTime = player.simulationTime;
            record.mins = player.mins;
            record.maxs = player.maxs;
            std::copy(player.matrix.begin(), player.matrix.end(), record.matrix);
            record.positions.push_back(record.matrix[8].origin());

            player.backtrackRecords.push_front(record);

            while (player.backtrackRecords.size() > 3 && player.backtrackRecords.size() > static_cast<size_t>(timeToTicks(timeLimit)))
                player.backtrackRecords.pop_back();
        }
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
    moveWeight = localPlayer->getAnimstate()->moveWeight;
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
    localPlayer->getAnimstate()->moveWeight = moveWeight;
}

void Animations::saveCorrectAngle(int entityIndex, Vector correctAng) noexcept
{
    buildTransformsIndex = entityIndex;
    correctAngle = correctAng;
}

int& Animations::buildTransformationsIndex() noexcept
{
    return buildTransformsIndex;
}

Vector* Animations::getCorrectAngle() noexcept
{
    return &correctAngle;
}

Vector* Animations::getViewAngles() noexcept
{
    return &viewangles;
}

Vector* Animations::getLocalAngle() noexcept
{
    return &localAngle;
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

bool Animations::gotFakelagMatrix() noexcept
{
    return gotMatrixFakelag;
}

std::array<matrix3x4, MAXSTUDIOBONES> Animations::getFakelagMatrix() noexcept
{
    return fakelagmatrix;
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

Animations::Players* Animations::setPlayer(int index) noexcept
{
    return &players.at(index);
}

std::array<Animations::Players, 65> Animations::getPlayers() noexcept
{
    return players;
}

std::array<Animations::Players, 65>* Animations::setPlayers() noexcept
{
    return &players;
}

const std::deque<Animations::Players::Record>* Animations::getBacktrackRecords(int index) noexcept
{
    return &players.at(index).backtrackRecords;
}