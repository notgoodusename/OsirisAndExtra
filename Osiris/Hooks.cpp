#include <functional>
#include <string>

#include "imgui/imgui.h"

#include <intrin.h>
#include <Windows.h>
#include <Psapi.h>

#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "MinHook/MinHook.h"

#include "Config.h"
#include "EventListener.h"
#include "GameData.h"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"

#include "Hacks/Aimbot.h"
#include "Hacks/Animations.h"
#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/Fakelag.h"
#include "Hacks/StreamProofESP.h"
#include "Hacks/Glow.h"
#include "Hacks/GrenadePrediction.h"
#include "Hacks/Legitbot.h"
#include "Hacks/Misc.h"
#include "Hacks/Ragebot.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Sound.h"
#include "Hacks/Triggerbot.h"
#include "Hacks/Visuals.h"

#include "SDK/ClientState.h"
#include "SDK/ConVar.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/FrameStage.h"
#include "SDK/GameEvent.h"
#include "SDK/GameUI.h"
#include "SDK/GlobalVars.h"
#include "SDK/Input.h"
#include "SDK/InputSystem.h"
#include "SDK/MaterialSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/NetworkChannel.h"
#include "SDK/NetworkMessage.h"
#include "SDK/Panel.h"
#include "SDK/Platform.h"
#include "SDK/RenderContext.h"
#include "SDK/SoundInfo.h"
#include "SDK/SoundEmitter.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    [[maybe_unused]] static const auto once = [](HWND window) noexcept {
        netvars = std::make_unique<Netvars>();
        eventListener = std::make_unique<EventListener>();

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(window);
        config = std::make_unique<Config>();
        gui = std::make_unique<GUI>();

        hooks->install();

        return true;
    }(window);

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);

    interfaces->inputSystem->enableInput(!gui->isOpen());

    return CallWindowProcW(hooks->originalWndProc, window, msg, wParam, lParam);
}

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    [[maybe_unused]] static bool imguiInit{ ImGui_ImplDX9_Init(device) };

    if (config->loadScheduledFonts())
        ImGui_ImplDX9_DestroyFontsTexture();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    StreamProofESP::render();
    NadePrediction::draw();
    Misc::purchaseList();
    Misc::noscopeCrosshair(ImGui::GetBackgroundDrawList());
    Misc::recoilCrosshair(ImGui::GetBackgroundDrawList());
    Misc::drawOffscreenEnemies(ImGui::GetBackgroundDrawList());
    Misc::drawBombTimer();
    Misc::spectatorList();
    Visuals::hitMarker(nullptr, ImGui::GetBackgroundDrawList());
    Visuals::drawMolotovHull(ImGui::GetBackgroundDrawList());
    Misc::watermark();

    Legitbot::updateInput();
    Visuals::updateInput();
    StreamProofESP::updateInput();
    Misc::updateInput();
    Triggerbot::updateInput();
    Chams::updateInput();
    Glow::updateInput();
    Ragebot::updateInput();
    AntiAim::updateInput();

    gui->handleToggle();

    if (gui->isOpen())
        gui->render();

    ImGui::EndFrame();
    ImGui::Render();

    if (device->BeginScene() == D3D_OK) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }

    GameData::clearUnusedAvatars();

    return hooks->originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    SkinChanger::clearItemIconTextures();
    GameData::clearTextures();
    return hooks->originalReset(device, params);
}

int __stdcall getUnverifiedFileHashes(void* thisPointer, int maxFiles)
{
    if (config->misc.svPureBypass)
        return 0;
    return hooks->fileSystem.callOriginal<int, 101>(thisPointer, maxFiles);
}

int __fastcall canLoadThirdPartyFiles(void* thisPointer, void* edx) noexcept
{
    if (config->misc.svPureBypass)
        return 1;
    return hooks->fileSystem.callOriginal<int, 127>(thisPointer);
}

static int __fastcall sendDatagram(NetworkChannel* network, void* edx, void* datagram)
{
    static auto original = hooks->networkChannel.getOriginal<int, 46, void*>(datagram);
    if (!config->backtrack.fakeLatency || datagram || !interfaces->engine->isInGame())
        return original(network, datagram);

    int instate = network->inReliableState;
    int insequencenr = network->inSequenceNr;

    float delta = max(0.f, std::clamp(config->backtrack.fakeLatencyAmount / 1000.f, 0.f, Backtrack::getMaxUnlag()) - network->getLatency(0));

    Backtrack::addLatencyToNetwork(network, delta);

    int result = original(network, datagram);

    network->inReliableState = instate;
    network->inSequenceNr = insequencenr;

    return result;
}

static bool __stdcall createMove(float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 24>(inputSampleTime, cmd);

    if (!cmd->commandNumber)
        return result;

    uintptr_t* framePointer;
    __asm mov framePointer, ebp;
    bool& sendPacket = *reinterpret_cast<bool*>(*framePointer - 0x1C);

    static auto pitchDown = interfaces->cvar->findVar("cl_pitchdown");
    static auto pitchUp = interfaces->cvar->findVar("cl_pitchup");
    static auto forwardSpeed = interfaces->cvar->findVar("cl_forwardspeed");;
    static auto sideSpeed = interfaces->cvar->findVar("cl_sidespeed");;

    static void* oldPointer = nullptr;
    if (auto network = memory->clientState->netChannel; network && oldPointer != network)
    {
        oldPointer = network;
        hooks->networkChannel.init(network);
        hooks->networkChannel.hookAt(46, sendDatagram);
    }

    static auto previousViewAngles{ cmd->viewangles };
    auto currentViewAngles{ cmd->viewangles };

    memory->globalVars->serverTime(cmd);
    Misc::antiAfkKick(cmd);
    Misc::fastStop(cmd);
    Misc::prepareRevolver(cmd);
    Visuals::removeShadows();
    Misc::runReportbot();
    Misc::bunnyHop(cmd);
    Misc::removeCrouchCooldown(cmd);
    Misc::autoPistol(cmd);
    Misc::autoReload(cmd);
    Misc::updateClanTag();
    Misc::fakeBan();
    Misc::stealNames();
    Misc::revealRanks(cmd);
    Misc::quickReload(cmd);
    Misc::fixTabletSignal();
    Misc::slowwalk(cmd);
    Backtrack::updateIncomingSequences();

    EnginePrediction::run(cmd);
    NadePrediction::run(cmd);

    Legitbot::run(cmd);
    Triggerbot::run(cmd);
    Backtrack::run(cmd);
    Ragebot::run(cmd);

    Misc::edgejump(cmd);
    Misc::fastPlant(cmd);

    if (AntiAim::canRun(cmd)) {
        Fakelag::run(sendPacket);
        AntiAim::run(cmd, previousViewAngles, currentViewAngles, sendPacket);
    }

    Misc::autoStrafe(cmd, currentViewAngles);
    Misc::moonwalk(cmd);

    auto viewAnglesDelta{ cmd->viewangles - previousViewAngles };
    viewAnglesDelta.normalize();
    viewAnglesDelta.x = std::clamp(viewAnglesDelta.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
    viewAnglesDelta.y = std::clamp(viewAnglesDelta.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

    cmd->viewangles = previousViewAngles + viewAnglesDelta;

    cmd->viewangles.normalize();
    Misc::fixMovement(cmd, currentViewAngles.y);

    cmd->viewangles.x = std::clamp(cmd->viewangles.x, -pitchUp->getFloat(), pitchDown->getFloat());
    cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
    cmd->viewangles.z = 0.0f;
    cmd->forwardmove = std::clamp(cmd->forwardmove, -forwardSpeed->getFloat(), forwardSpeed->getFloat());
    cmd->sidemove = std::clamp(cmd->sidemove, -sideSpeed->getFloat(), sideSpeed->getFloat());

    previousViewAngles = cmd->viewangles;
    Animations::update(cmd, sendPacket);
    Animations::fake();
    return false;
}

static void __stdcall doPostScreenEffects(void* param) noexcept
{
    if (interfaces->engine->isInGame()) {
        Visuals::thirdperson();
        Misc::inverseRagdollGravity();
        Visuals::reduceFlashEffect();
        Visuals::remove3dSky();
        Glow::render();
    }
    hooks->clientMode.callOriginal<void, 44>(param);
}

static float __stdcall getViewModelFov() noexcept
{
    float additionalFov = static_cast<float>(config->visuals.viewmodelFov);
    if (localPlayer) {
        if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            additionalFov = 0.0f;
    }

    return hooks->clientMode.callOriginal<float, 35>() + additionalFov;
}

static void __stdcall drawModelExecute(void* ctx, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    if (interfaces->studioRender->isForcedMaterialOverride())
        return hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);

    if (Visuals::removeHands(info.model->name) || Visuals::removeSleeves(info.model->name) || Visuals::removeWeapons(info.model->name))
        return;

    static Chams chams;
    if (!chams.render(ctx, state, info, customBoneToWorld))
        hooks->modelRender.callOriginal<void, 21>(ctx, state, std::cref(info), customBoneToWorld);

    interfaces->studioRender->forcedMaterialOverride(nullptr);
}

static bool __fastcall svCheatsGetBool(void* _this) noexcept
{
    if (std::uintptr_t(_ReturnAddress()) == memory->cameraThink && config->visuals.thirdperson)
        return true;

    return hooks->svCheats.getOriginal<bool, 13>()(_this);
}

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
    [[maybe_unused]] static auto backtrackInit = (Backtrack::init(), false);
    Animations::init();

    if (interfaces->engine->isConnected() && !interfaces->engine->isInGame())
        Misc::changeName(true, nullptr, 0.0f);
    
    if (stage == FrameStage::START)
        GameData::update();

    if (stage == FrameStage::RENDER_START) {
        Misc::preserveKillfeed();
        Misc::disablePanoramablur();
        Misc::fakePrime();
        Misc::updateEventListeners();
        Visuals::updateEventListeners();
    }
    if (interfaces->engine->isInGame()) {
        Visuals::skybox(stage);
        Visuals::removeBlur(stage);
        Misc::oppositeHandKnife(stage);
        Visuals::removeGrass(stage);
        Visuals::modifySmoke(stage);
        Visuals::playerModel(stage);
        Visuals::disablePostProcessing(stage);
        Visuals::removeVisualRecoil(stage);
        Visuals::applyZoom(stage);
        SkinChanger::run(stage);
        Animations::real(stage);
        Animations::players(stage);
        Backtrack::update(stage);
    }
    hooks->client.callOriginal<void, 37>(stage);
}

static int __stdcall emitSound(void* filter, int entityIndex, int channel, const char* soundEntry, unsigned int soundEntryHash, const char* sample, float volume, int seed, int soundLevel, int flags, int pitch, const Vector& origin, const Vector& direction, void* utlVecOrigins, bool updatePositions, float soundtime, int speakerentity, void* soundParams) noexcept
{
    Sound::modulateSound(soundEntry, entityIndex, volume);
    Misc::autoAccept(soundEntry);

    volume = std::clamp(volume, 0.0f, 1.0f);
    return hooks->sound.callOriginal<int, 5>(filter, entityIndex, channel, soundEntry, soundEntryHash, sample, volume, seed, soundLevel, flags, pitch, std::cref(origin), std::cref(direction), utlVecOrigins, updatePositions, soundtime, speakerentity, soundParams);
}

static bool __stdcall shouldDrawFog() noexcept
{
    if constexpr (std::is_same_v<HookType, MinHook>) {
#ifdef _DEBUG
    // Check if we always get the same return address
    if (*static_cast<std::uint32_t*>(_ReturnAddress()) == 0x6274C084) {
        static const auto returnAddress = std::uintptr_t(_ReturnAddress());
        assert(returnAddress == std::uintptr_t(_ReturnAddress()));
    }
#endif

    if (*static_cast<std::uint32_t*>(_ReturnAddress()) != 0x6274C084)
        return hooks->clientMode.callOriginal<bool, 17>();
    }
    
    return !config->visuals.noFog;
}

static bool __stdcall shouldDrawViewModel() noexcept
{
    if (config->visuals.zoom && localPlayer && localPlayer->fov() < 45 && localPlayer->fovStart() < 45)
        return false;
    return hooks->clientMode.callOriginal<bool, 27>();
}

static void __stdcall lockCursor() noexcept
{
    if (gui->isOpen())
        return interfaces->surface->unlockCursor();
    return hooks->surface.callOriginal<void, 67>();
}

static void __stdcall setDrawColor(int r, int g, int b, int a) noexcept
{
    if (config->visuals.noScopeOverlay && (std::uintptr_t(_ReturnAddress()) == memory->scopeDust || std::uintptr_t(_ReturnAddress()) == memory->scopeArc))
        a = 0;
    hooks->surface.callOriginal<void, 15>(r, g, b, a);
}

struct ViewSetup {
    PAD(172);
    void* csm;
    float fov;
    PAD(32);
    float farZ;
};

static void __stdcall overrideView(ViewSetup* setup) noexcept
{
    if (localPlayer && !localPlayer->isScoped())
        setup->fov += config->visuals.fov;
    setup->farZ += config->visuals.farZ * 10;
    hooks->clientMode.callOriginal<void, 18>(setup);
}

struct RenderableInfo {
    Entity* renderable;
    std::byte pad[18];
    uint16_t flags;
    uint16_t flags2;
};

static int __stdcall listLeavesInBox(const Vector& mins, const Vector& maxs, unsigned short* list, int listMax) noexcept
{
    if (std::uintptr_t(_ReturnAddress()) == memory->listLeaves) {
        if (const auto info = *reinterpret_cast<RenderableInfo**>(std::uintptr_t(_AddressOfReturnAddress()) + 0x14); info && info->renderable) {
            if (const auto ent = VirtualMethod::call<Entity*, 7>(info->renderable - 4); ent && ent->isPlayer()) {
                if (config->misc.disableModelOcclusion) {
                    // FIXME: sometimes players are rendered above smoke, maybe sort render list?
                    info->flags &= ~0x100;
                    info->flags2 |= 0x40;

                    constexpr float maxCoord = 16384.0f;
                    constexpr float minCoord = -maxCoord;
                    constexpr Vector min{ minCoord, minCoord, minCoord };
                    constexpr Vector max{ maxCoord, maxCoord, maxCoord };
                    return hooks->bspQuery.callOriginal<int, 6>(std::cref(min), std::cref(max), list, listMax);
                }
            }
        }
    }

    return hooks->bspQuery.callOriginal<int, 6>(std::cref(mins), std::cref(maxs), list, listMax);
}

static int __fastcall dispatchSound(SoundInfo& soundInfo) noexcept
{
    if (const char* soundName = interfaces->soundEmitter->getSoundName(soundInfo.soundIndex)) {
        Sound::modulateSound(soundName, soundInfo.entityIndex, soundInfo.volume);
        soundInfo.volume = std::clamp(soundInfo.volume, 0.0f, 1.0f);
    }
    return hooks->originalDispatchSound(soundInfo);
}

static void __stdcall render2dEffectsPreHud(void* viewSetup) noexcept
{
Visuals::applyScreenEffects();
Visuals::hitEffect();
hooks->viewRender.callOriginal<void, 39>(viewSetup);
}

static const DemoPlaybackParameters* __stdcall getDemoPlaybackParameters() noexcept
{
    const auto params = hooks->engine.callOriginal<const DemoPlaybackParameters*, 218>();

    if (params && config->misc.revealSuspect && std::uintptr_t(_ReturnAddress()) != memory->demoFileEndReached) {
        static DemoPlaybackParameters customParams;
        customParams = *params;
        customParams.anonymousPlayerIdentity = false;
        return &customParams;
    }

    return params;
}

static bool __stdcall isPlayingDemo() noexcept
{
    if (config->misc.revealMoney && std::uintptr_t(_ReturnAddress()) == memory->demoOrHLTV && *reinterpret_cast<std::uintptr_t*>((std::uintptr_t(_AddressOfReturnAddress()) - sizeof(std::uintptr_t)) + 8) == memory->money)
        return true;

    return hooks->engine.callOriginal<bool, 82>();
}

static bool __fastcall isHltv() noexcept
{
    if (_ReturnAddress() == memory->setupVelocityAddress || _ReturnAddress() == memory->accumulateLayersAddress)
        return true;
    return hooks->engine.callOriginal<bool, 93>();
}

static void __stdcall updateColorCorrectionWeights() noexcept
{
    hooks->clientMode.callOriginal<void, 58>();

    if (const auto& cfg = config->visuals.colorCorrection; cfg.enabled) {
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x498) = cfg.blue;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4A0) = cfg.red;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4A8) = cfg.mono;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4B0) = cfg.saturation;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4C0) = cfg.ghost;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4C8) = cfg.green;
        *reinterpret_cast<float*>(std::uintptr_t(memory->clientMode) + 0x4D0) = cfg.yellow;
    }

    if (config->visuals.noScopeOverlay)
        *memory->vignette = 0.0f;
}

static float __stdcall getScreenAspectRatio(int width, int height) noexcept
{
    if (config->misc.aspectratio)
        return config->misc.aspectratio;
    return hooks->engine.callOriginal<float, 101>(width, height);
}

static void __stdcall renderSmokeOverlay(bool update) noexcept
{
    if (config->visuals.noSmoke || config->visuals.wireframeSmoke)
        *reinterpret_cast<float*>(std::uintptr_t(memory->viewRender) + 0x588) = 0.0f;
    else
        hooks->viewRender.callOriginal<void, 41>(update);
}

static void __stdcall onJump(float stamina) noexcept
{
    hooks->gameMovement.callOriginal<void, 32>(stamina);

    if (localPlayer && localPlayer->isAlive() && localPlayer->getAnimstate())
        localPlayer->getAnimstate()->doAnimationEvent(PLAYERANIMEVENT_JUMP);
}

void __fastcall doExtraBoneProcessingHook(void* thisPointer, void* edx, void* hdr, void* pos, void* q, const matrix3x4& matrix, uint8_t* bone_list, void* context) noexcept
{
    return;
}

void __fastcall standardBlendingRulesHook(void* thisPointer, void* edx, void* hdr, void* pos, void* q, float currentTime, int boneMask) noexcept
{
    static auto original = hooks->standardBlendingRules.getOriginal<void>(hdr, pos, q, currentTime, boneMask);

    auto entity = reinterpret_cast<Entity*>(thisPointer);

    entity->getEffects() |= 8;

    original(thisPointer, hdr, pos, q, currentTime, boneMask);

    entity->getEffects() &= ~8;
}

bool __fastcall shouldSkipAnimationFrameHook(void* thisPointer, void* edx) noexcept
{
    return false;
}

void __fastcall updateClientSideAnimationHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->updateClientSideAnimation.getOriginal<void>();

    auto entity = reinterpret_cast<Entity*>(thisPointer);

    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer)
        return original(thisPointer);

    if (entity != localPlayer.get())
    {
        if (Animations::data.update)
            return original(thisPointer);
    }
    else if (entity == localPlayer.get())
    {
        if (Animations::data.updating)
            return original(thisPointer);
    }
}

void __fastcall checkForSequenceChangeHook(void* thisPointer, void* edx, void* hdr, int curSequence, bool forceNewSequence, bool interpolate) noexcept
{
    static auto original = hooks->checkForSequenceChange.getOriginal<void>(hdr, curSequence, forceNewSequence, interpolate);

    return original(thisPointer, hdr, curSequence, forceNewSequence, false);
}

void __fastcall modifyEyePositionHook(void* thisPointer, void* edx, Vector& pos) noexcept
{
    using modifyEyePositionFn = void(__thiscall*)(void*, Vector&);
    static auto original = (modifyEyePositionFn)hooks->modifyEyePosition.getDetour();
    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    auto entity = reinterpret_cast<Entity*>(animState->player);

    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, pos);

    const int bone = memory->lookUpBone(entity, "head_0");
    if (bone == -1)
        return;

    if (animState->landing || animState->animDuckAmount != 0.f || !entity->groundEntity())
    {
        Vector bonePos;
        entity->getBonePos(bone, bonePos);
        bonePos.z += 1.7f;

        if (bonePos.z < pos.z)
        {
            float tmp = std::clamp((fabsf(pos.z - bonePos.z) - 4.0f) * (1.0f / 6.0f), 0.0f, 1.0f);
            pos.z += ((bonePos.z - pos.z) * (((powf(tmp, 2) * -2.0f) * tmp) + (3.0f * powf(tmp, 2))));
        }
    }
}

void __fastcall calculateViewHook(void* thisPointer, void* edx, Vector& eyeOrigin, Vector& eyeAngles, float& zNear, float& zFar, float& fov) noexcept
{
    using calculateViewFn = void(__thiscall*)(void*, Vector&, Vector&, float&, float&, float&);
    static auto original = (calculateViewFn)hooks->calculateView.getDetour();

    auto entity = reinterpret_cast<Entity*>(thisPointer);

    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, eyeOrigin, eyeAngles, zNear, zFar, fov);

    const auto oldUseNewAnimationState = entity->useNewAnimationState();

    entity->useNewAnimationState() = false;

    original(thisPointer, eyeOrigin, eyeAngles, zNear, zFar, fov);

    entity->useNewAnimationState() = oldUseNewAnimationState;
}

void __vectorcall updateStateHook(void* thisPointer, void* unknown, float z, float y, float x, void* unknown1) noexcept
{
    using updateStateFn = void(__vectorcall*)(void*, void*, float, float, float, void*);
    static auto original = (updateStateFn)hooks->updateState.getDetour();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return original(thisPointer, unknown, z, y, x, unknown1);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, unknown, z, y, x, unknown1);

    if (!localPlayer->getModelPtr())
        return;

    return original(thisPointer, unknown, z, y, x, unknown1);
}

static std::array<AnimationLayer, 13> layers{};

//TODO: find a way to ignore networked data
void __fastcall preDataUpdateHook(void* thisPointer, void* edx, int updateType) noexcept
{
    static auto original = hooks->preDataUpdate.getOriginal<void>(updateType);

    auto entity = reinterpret_cast<Entity*>((uintptr_t)thisPointer - 8);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, updateType);

    std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());

    return original(thisPointer, updateType);
}

void __fastcall postDataUpdateHook(void* thisPointer, void* edx, int updateType) noexcept
{
    static auto original = hooks->postDataUpdate.getOriginal<void>(updateType);

    auto entity = reinterpret_cast<Entity*>((uintptr_t)thisPointer - 8);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, updateType);

    std::array<AnimationLayer, 13> currentLayers{};

    std::memcpy(&currentLayers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());

    for (int i = 0; i < 13; i++)
    {
        AnimationLayer _curLayer = currentLayers.at(i);
        AnimationLayer _prevLayer = layers.at(i);
        if (
            _curLayer.sequence != _prevLayer.sequence
            || _curLayer.weightDeltaRate!= _prevLayer.weightDeltaRate
            || _curLayer.cycle != _prevLayer.cycle
            || _curLayer.weight != _prevLayer.weight
            || _curLayer.playbackRate != _prevLayer.playbackRate
            )
        {

            {
                auto a = _curLayer;
                auto b = _prevLayer;
                auto c = 1;
            }
        }
    }

    return original(thisPointer, updateType);
}

void __fastcall resetStateHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->resetState.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity)
        return original(thisPointer);

    animState->lowerBodyRealignTimer = 0.f;
    animState->deployRateLimiting = false;
    animState->jumping = false;
    animState->buttons = 0;
    if(localPlayer && entity == localPlayer.get())
        entity->lby() = 0.f;

    return original(thisPointer);
}

void __fastcall setupVelocityHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupVelocity.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    animState->setupVelocity();
}

void __fastcall setupMovementHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupMovement.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    animState->setupMovement();
}

void __fastcall setupAliveloopHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupAliveloop.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    animState->setupAliveLoop();
}

void __fastcall notifyOnLayerChangeWeightHook(void* thisPointer, void* edx, void* layer, const float newWeight) noexcept
{
    static auto original = hooks->notifyOnLayerChangeWeight.getOriginal<void>(layer, newWeight);

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, layer, newWeight);
    return;
}

void __fastcall notifyOnLayerChangeCycleHook(void* thisPointer, void* edx, void* layer, const float newCycle) noexcept
{
    static auto original = hooks->notifyOnLayerChangeCycle.getOriginal<void>(layer, newCycle);

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, layer, newCycle);
    return;
}

Hooks::Hooks(HMODULE moduleHandle) noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    this->moduleHandle = moduleHandle;

    // interfaces and memory shouldn't be initialized in wndProc because they show MessageBox on error which would cause deadlock
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();

    window = FindWindowW(L"Valve001", nullptr);
    originalWndProc = WNDPROC(SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(wndProc)));
}

void Hooks::install() noexcept
{
    originalPresent = **reinterpret_cast<decltype(originalPresent)**>(memory->present);
    **reinterpret_cast<decltype(present)***>(memory->present) = present;
    originalReset = **reinterpret_cast<decltype(originalReset)**>(memory->reset);
    **reinterpret_cast<decltype(reset)***>(memory->reset) = reset;

    if constexpr (std::is_same_v<HookType, MinHook>)
        MH_Initialize();

    doExtraBoneProcessing.detour(memory->doExtraBoneProcessing, doExtraBoneProcessingHook);
    shouldSkipAnimationFrame.detour(memory->shouldSkipAnimationFrame, shouldSkipAnimationFrameHook);
    standardBlendingRules.detour(memory->standardBlendingRules, standardBlendingRulesHook);
    updateClientSideAnimation.detour(memory->updateClientSideAnimation, updateClientSideAnimationHook);
    checkForSequenceChange.detour(memory->checkForSequenceChange, checkForSequenceChangeHook);

    modifyEyePosition.detour(memory->modifyEyePosition, modifyEyePositionHook);
    calculateView.detour(memory->calculateView, calculateViewHook);
    updateState.detour(memory->updateState, updateStateHook);

    //Finish rebuilding

    resetState.detour(memory->resetState, resetStateHook);
    
    setupVelocity.detour(memory->setupVelocity, setupVelocityHook); //done
    setupMovement.detour(memory->setupMovement, setupMovementHook); //done
    setupAliveloop.detour(memory->setupAliveloop, setupAliveloopHook); // done

    notifyOnLayerChangeWeight.detour(memory->notifyOnLayerChangeWeight, notifyOnLayerChangeWeightHook);
    notifyOnLayerChangeCycle.detour(memory->notifyOnLayerChangeCycle, notifyOnLayerChangeCycleHook);

    //preDataUpdate.detour(memory->preDataUpdate, preDataUpdateHook);
    //postDataUpdate.detour(memory->postDataUpdate, postDataUpdateHook);
    
    bspQuery.init(interfaces->engine->getBSPTreeQuery());

    client.init(interfaces->client);
    client.hookAt(37, frameStageNotify);

    clientMode.init(memory->clientMode);
    clientMode.hookAt(17, shouldDrawFog);
    clientMode.hookAt(18, overrideView);
    clientMode.hookAt(24, createMove);
    clientMode.hookAt(27, shouldDrawViewModel);
    clientMode.hookAt(35, getViewModelFov);
    clientMode.hookAt(44, doPostScreenEffects);
    clientMode.hookAt(58, updateColorCorrectionWeights);

    engine.init(interfaces->engine);
    engine.hookAt(82, isPlayingDemo);
    engine.hookAt(93, isHltv);
    engine.hookAt(101, getScreenAspectRatio);
    engine.hookAt(218, getDemoPlaybackParameters);

    fileSystem.init(interfaces->fileSystem);
    fileSystem.hookAt(101, getUnverifiedFileHashes);
    fileSystem.hookAt(127, canLoadThirdPartyFiles);

    gameMovement.init(interfaces->gameMovement);
    gameMovement.hookAt(32, onJump);

    modelRender.init(interfaces->modelRender);
    modelRender.hookAt(21, drawModelExecute);

    sound.init(interfaces->sound);
    sound.hookAt(5, emitSound);

    surface.init(interfaces->surface);
    surface.hookAt(15, setDrawColor);

    svCheats.init(interfaces->cvar->findVar("sv_cheats"));
    svCheats.hookAt(13, svCheatsGetBool);

    viewRender.init(memory->viewRender);
    viewRender.hookAt(39, render2dEffectsPreHud);
    viewRender.hookAt(41, renderSmokeOverlay);

    if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        originalDispatchSound = decltype(originalDispatchSound)(uintptr_t(memory->dispatchSound + 1) + *memory->dispatchSound);
        *memory->dispatchSound = uintptr_t(dispatchSound) - uintptr_t(memory->dispatchSound + 1);
        VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
    }

    bspQuery.hookAt(6, listLeavesInBox);
    surface.hookAt(67, lockCursor);
    if constexpr (std::is_same_v<HookType, MinHook>)
        MH_EnableHook(MH_ALL_HOOKS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
    Sleep(100);

    interfaces->inputSystem->enableInput(true);
    eventListener->remove();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);

    FreeLibraryAndExitThread(moduleHandle, 0);
}

void Hooks::uninstall() noexcept
{
    Misc::updateEventListeners(true);
    Visuals::updateEventListeners(true);

    if constexpr (std::is_same_v<HookType, MinHook>) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    bspQuery.restore();
    client.restore();
    clientMode.restore();
    engine.restore();
    gameMovement.restore();
    modelRender.restore();
    sound.restore();
    surface.restore();
    svCheats.restore();
    viewRender.restore();
    networkChannel.restore();
    fileSystem.restore();

    netvars->restore();

    Glow::clearCustomObjects();

    SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(memory->present) = originalPresent;
    **reinterpret_cast<void***>(memory->reset) = originalReset;

    if (DWORD oldProtection; VirtualProtect(memory->dispatchSound, 4, PAGE_EXECUTE_READWRITE, &oldProtection)) {
        *memory->dispatchSound = uintptr_t(originalDispatchSound) - uintptr_t(memory->dispatchSound + 1);
        VirtualProtect(memory->dispatchSound, 4, oldProtection, nullptr);
    }

    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), moduleHandle, 0, nullptr))
        CloseHandle(thread);
}
