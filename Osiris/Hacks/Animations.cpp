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

Animations::Datas Animations::data;

void Animations::init() noexcept
{
    static auto jiggleBones = interfaces->cvar->findVar("r_jiggle_bones");
    if (jiggleBones->getInt() >= 1)
        jiggleBones->setValue(0);

    static auto extrapolate = interfaces->cvar->findVar("cl_extrapolate");
    if (extrapolate->getInt() >= 1)
        extrapolate->setValue(0);
}

void Animations::update(UserCmd* cmd, bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    data.viewangles = cmd->viewangles;
    data.sendPacket = sendPacket;
    localPlayer->getAnimstate()->buttons = cmd->buttons;
    localPlayer->getAnimstate()->doAnimationEvent(PLAYERANIMEVENT_COUNT); // Build activity modifiers
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

    if (data.sendPacket)
    {
        std::array<AnimationLayer, 13> layers;

        std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());

        auto backupAbs = localPlayer->getAbsAngle();
        auto backupPoses = localPlayer->poseParameters();

        localPlayer->updateState(fakeAnimState, data.viewangles);
        memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeAnimState->footYaw, 0 });
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        localPlayer->getAnimationLayer(ANIMATION_LAYER_LEAN)->weight = std::numeric_limits<float>::epsilon();
        data.gotMatrix = localPlayer->setupBones(data.fakematrix, MAXSTUDIOBONES, 0x7FF00, memory->globalVars->currenttime);
        const auto origin = localPlayer->getRenderOrigin();
        if (data.gotMatrix)
        {
            for (auto& i : data.fakematrix)
            {
                i[0][3] -= origin.x;
                i[1][3] -= origin.y;
                i[2][3] -= origin.z;
            }
        }
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        localPlayer->poseParameters() = backupPoses;
        memory->setAbsAngle(localPlayer.get(), Vector{ 0,backupAbs.y,0 });
    }
}

void Animations::real(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START)
        return;

    static std::array<AnimationLayer, 13> layers{};

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if(stage == FrameStage::RENDER_START)
    {
        static auto backupPoses = localPlayer->poseParameters();
        static auto backupAbs = localPlayer->getAnimstate()->footYaw;

        static int oldTick = 0;

        localPlayer->getEFlags() &= ~0x1000;

        if (oldTick != memory->globalVars->tickCount)
        {
            oldTick = memory->globalVars->tickCount;
            Animations::data.updating = true;

            // allow animations to be animated in the same frame
            if (localPlayer->getAnimstate()->lastUpdateFrame == memory->globalVars->framecount)
                localPlayer->getAnimstate()->lastUpdateFrame -= 1;

            localPlayer->updateState(localPlayer->getAnimstate(), Animations::data.viewangles);
            // updateClientSideAnimation calls animState update, but uses eyeAngles, 
            // note: after updating the animstate lastUpdateFrame will be equal to memory->globalVars->framecount, so it wont be executed
            // so no need to worry about it
            localPlayer->updateClientSideAnimation(); 

            if (data.sendPacket)
            {
                std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
                backupPoses = localPlayer->poseParameters();
                backupAbs = localPlayer->getAnimstate()->footYaw;
            }
            Animations::data.updating = false;
        }
        memory->setAbsAngle(localPlayer.get(), Vector{ 0,backupAbs,0 });
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        localPlayer->poseParameters() = backupPoses;
    }
}

void Animations::players(FrameStage stage) noexcept
{
    if (stage != FrameStage::NET_UPDATE_END)
        return;

    if (!localPlayer)
    {
        for (auto& record : data.player)
            record.clear();
        return;
    }
    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        auto& player = data.player[i];
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        {
            player.clear();
            continue;
        }

        std::memcpy(&player.layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayerCount());

        const auto frameTime = memory->globalVars->frametime;
        const auto currentTime = memory->globalVars->currenttime;

        memory->globalVars->frametime = memory->globalVars->intervalPerTick;
        memory->globalVars->currenttime = entity->simulationTime();

        if (player.oldSimtime != entity->simulationTime())
        {
            if (player.lastOrigin.notNull())
                player.velocity = (entity->origin() - player.lastOrigin) * (1.0f / fabsf(entity->simulationTime() - player.oldSimtime));
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

        data.update = true;
        entity->updateClientSideAnimation();
        data.update = false;

        std::memcpy(entity->animOverlays(), &player.layers, sizeof(AnimationLayer) * entity->getAnimationLayerCount());

        memory->globalVars->frametime = frameTime;
        memory->globalVars->currenttime = currentTime;

        if (player.oldSimtime != entity->simulationTime())
        {
            player.chokedPackets = static_cast<int>(fabsf(entity->simulationTime() - player.oldSimtime) / memory->globalVars->intervalPerTick) - 1;
            player.oldSimtime = entity->simulationTime();
            player.currentSimtime = entity->simulationTime();
            player.gotMatrix = entity->setupBones(player.matrix.data(), 256, 0x7FF00, memory->globalVars->currenttime);
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
        }
    }
}