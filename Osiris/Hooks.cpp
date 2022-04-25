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
#include "Logger.h"
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
#include "Hacks/Knifebot.h"
#include "Hacks/Legitbot.h"
#include "Hacks/Misc.h"
#include "Hacks/Ragebot.h"
#include "Hacks/Resolver.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Sound.h"
#include "Hacks/Tickbase.h"
#include "Hacks/Triggerbot.h"
#include "Hacks/Visuals.h"

#include "SDK/ClassId.h"
#include "SDK/Client.h"
#include "SDK/ClientState.h"
#include "SDK/ConVar.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/FrameStage.h"
#include "SDK/GameEvent.h"
#include "SDK/GameMovement.h"
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
#include "SDK/Prediction.h"
#include "SDK/PredictionCopy.h"
#include "SDK/RenderContext.h"
#include "SDK/SoundInfo.h"
#include "SDK/SoundEmitter.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"
#include "SDK/ViewSetup.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    [[maybe_unused]] static const auto once = [](HWND window) noexcept {
        Netvars::init();
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

    if (const auto& displaySize = ImGui::GetIO().DisplaySize; displaySize.x > 0.0f && displaySize.y > 0.0f) {
        StreamProofESP::render();
        NadePrediction::draw();
        Misc::purchaseList();
        Visuals::visualizeSpread(ImGui::GetBackgroundDrawList());
        Misc::noscopeCrosshair(ImGui::GetBackgroundDrawList());
        Misc::recoilCrosshair(ImGui::GetBackgroundDrawList());
        Misc::drawOffscreenEnemies(ImGui::GetBackgroundDrawList());
        Misc::drawBombTimer();
        Misc::spectatorList();
        Misc::showKeybinds();
        Visuals::hitMarker(nullptr, ImGui::GetBackgroundDrawList());
        Visuals::drawMolotovHull(ImGui::GetBackgroundDrawList());
        Visuals::drawSmokeHull(ImGui::GetBackgroundDrawList());
        Misc::watermark();
        Misc::drawAutoPeek(ImGui::GetBackgroundDrawList());
        Logger::process(ImGui::GetBackgroundDrawList());

        Legitbot::updateInput();
        Visuals::updateInput();
        StreamProofESP::updateInput();
        Misc::updateInput();
        Triggerbot::updateInput();
        Chams::updateInput();
        Glow::updateInput();
        Ragebot::updateInput();
        AntiAim::updateInput();
        Misc::drawPlayerList();
        gui->handleToggle();

        if (gui->isOpen())
            gui->render();
    }

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

static int __stdcall getUnverifiedFileHashes(void* thisPointer, int maxFiles)
{
    if (config->misc.svPureBypass)
        return 0;
    return hooks->fileSystem.callOriginal<int, 101>(thisPointer, maxFiles);
}

static int __fastcall canLoadThirdPartyFiles(void* thisPointer, void* edx) noexcept
{
    if (config->misc.svPureBypass)
        return 1;
    return hooks->fileSystem.callOriginal<int, 128>(thisPointer);
}

static bool __stdcall isConnected() noexcept
{
    if (config->misc.inventoryUnlocker && RETURN_ADDRESS() == memory->unlockInventory)
        return false;

    return hooks->engine.callOriginal<bool, 27>();
}

static int __fastcall sendDatagramHook(NetworkChannel* network, void* edx, void* datagram)
{
    static auto original = hooks->sendDatagram.getOriginal<int>(datagram);
    if (!config->backtrack.fakeLatency || datagram || !interfaces->engine->isInGame())
        return original(network, datagram);

    int instate = network->inReliableState;
    int insequencenr = network->inSequenceNr;

    float delta = max(0.f, config->backtrack.fakeLatencyAmount / 1000.f);

    Backtrack::addLatencyToNetwork(network, delta);

    int result = original(network, datagram);

    network->inReliableState = instate;
    network->inSequenceNr = insequencenr;

    return result;
}

static void __fastcall packetStart(void* thisPointer, void* edx, int incomingSequence, int outgoingAcknowledged) noexcept
{
    Animations::packetStart();

    return hooks->clientState.callOriginal<void, 5>(incomingSequence, outgoingAcknowledged);
}

static void __fastcall postDataUpdateHook(void* thisPointer, void* edx, int updateType) noexcept
{
    static auto original = hooks->postDataUpdate.getOriginal<void>(updateType);

    original(thisPointer, updateType);
    
    Animations::postDataUpdate();
    return;
}

static bool __stdcall createMove(float inputSampleTime, UserCmd* cmd, bool& sendPacket) noexcept
{
    static auto previousViewAngles{ cmd->viewangles };
    const auto viewAngles{ cmd->viewangles };
    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };

    if (const auto gameRules = (*memory->gameRules); gameRules)
        maxUserCmdProcessTicks = (gameRules->isValveDS()) ? 8 : 16;

    Resolver::processMissedShots();
    
    memory->globalVars->serverTime(cmd);
    if (Tickbase::isShifting && config->tickbase.enabled)
    {
        sendPacket = Tickbase::ticksToShift == 1;
        Misc::autoPeek(cmd, currentViewAngles);
        if (!config->tickbase.teleport)
        {
            cmd->tickCount += 200;
            cmd->hasbeenpredicted = true;
        }

        cmd->viewangles.normalize();

        cmd->viewangles.x = std::clamp(cmd->viewangles.x, -89.0f, 89.0f);
        cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
        cmd->viewangles.z = 0.0f;
        cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
        cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);

        return false;
    }


    Misc::antiAfkKick(cmd);
    Misc::fastStop(cmd);
    Misc::prepareRevolver(cmd);
    Visuals::removeShadows();
    Visuals::fullBright();
    Visuals::shadowChanger();
    Misc::runReportbot();
    Misc::bunnyHop(cmd);
    Misc::removeCrouchCooldown(cmd);
    Misc::autoPistol(cmd);
    Misc::autoReload(cmd);
    Misc::updateClanTag();
    Misc::stealNames();
    Misc::revealRanks(cmd);
    Misc::fixTabletSignal();
    Misc::slowwalk(cmd);
    Backtrack::updateIncomingSequences();

    EnginePrediction::update();
    EnginePrediction::run(cmd);
    NadePrediction::run(cmd);

    Legitbot::run(cmd);
    Backtrack::run(cmd);
    Triggerbot::run(cmd);
    Ragebot::run(cmd);

    Misc::autoPeek(cmd, currentViewAngles);

    Misc::edgejump(cmd);
    Misc::blockBot(cmd);
    Misc::fastPlant(cmd);

    Knifebot::run(cmd);

    if (AntiAim::canRun(cmd))
    {
        Fakelag::run(sendPacket);
        AntiAim::run(cmd, previousViewAngles, currentViewAngles, sendPacket);
    }

    Misc::fakeDuck(cmd, sendPacket);
    Misc::autoStrafe(cmd, currentViewAngles);
    Misc::jumpBug(cmd);
    Misc::runFreeCam(cmd, viewAngles);
    Misc::moonwalk(cmd);

    auto viewAnglesDelta{ cmd->viewangles - previousViewAngles };
    viewAnglesDelta.normalize();
    cmd->viewangles = previousViewAngles + viewAnglesDelta;

    cmd->viewangles.normalize();

    if ((currentViewAngles != cmd->viewangles
        || cmd->forwardmove != currentCmd.forwardmove
        || cmd->sidemove != currentCmd.sidemove) && (cmd->sidemove != 0 || cmd->forwardmove != 0))
    {
        Misc::fixMovement(cmd, currentViewAngles.y);
    }

    cmd->viewangles.x = std::clamp(cmd->viewangles.x, -89.0f, 89.0f);
    cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
    cmd->viewangles.z = 0.0f;
    cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
    cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);
    cmd->upmove = std::clamp(cmd->upmove, -320.0f, 320.0f);

    previousViewAngles = cmd->viewangles;
    Tickbase::run(cmd);
    Visuals::updateShots(cmd);

    if (localPlayer && localPlayer->isAlive())
    {
        memory->restoreEntityToPredictedFrame(0, interfaces->prediction->split->commandsPredicted - 1);
    }
    Animations::update(cmd, sendPacket);
    Animations::fake();
    return false;
}

static void __stdcall CHLCreateMove(int sequenceNumber, float inputSampleTime, bool active, bool& sendPacket) noexcept
{
    static auto original = hooks->client.getOriginal<void, 22>(sequenceNumber, inputSampleTime, active);

    original(interfaces->client, sequenceNumber, inputSampleTime, active);

    UserCmd* cmd = memory->input->getUserCmd(0, sequenceNumber);
    if (!cmd || !cmd->commandNumber)
        return;

    VerifiedUserCmd* verified = memory->input->getVerifiedUserCmd(sequenceNumber);
    if (!verified)
        return;

    bool cmoveactive = createMove(inputSampleTime, cmd, sendPacket);

    verified->cmd = *cmd;
    verified->crc = cmd->getChecksum();
}

#pragma warning(disable : 4409)
__declspec(naked) void __stdcall createMoveProxy(int sequenceNumber, float inputSampleTime, bool active)
{
    __asm
    {
        PUSH	EBP
        MOV		EBP, ESP
        PUSH	EBX
        LEA		ECX, [ESP]
        PUSH	ECX
        PUSH	active
        PUSH	inputSampleTime
        PUSH	sequenceNumber
        CALL	CHLCreateMove
        POP		EBX
        POP		EBP
        RETN	0xC
    }
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
    float additionalFov = static_cast<float>(config->visuals.viewModel.fov);
    if (localPlayer) {
        if (const auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            additionalFov = 0.0f;
    }

    if (!config->visuals.viewModel.enabled)
        additionalFov = 0.f;

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
    if (std::uintptr_t(_ReturnAddress()) == memory->cameraThink && (config->visuals.thirdperson || config->visuals.freeCam))
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
        Misc::forceRelayCluster();
        Misc::preserveKillfeed();
        Misc::disablePanoramablur();
        Misc::updateEventListeners();
        Visuals::updateEventListeners();
        Resolver::updateEventListeners();
    }
    if (interfaces->engine->isInGame()) {
        EnginePrediction::apply(stage);
        Visuals::drawBulletImpacts();
        Visuals::skybox(stage);
        Visuals::removeBlur(stage);
        Visuals::noZoom(stage);
        Misc::oppositeHandKnife(stage);
        Visuals::removeGrass(stage);
        Visuals::modifySmoke(stage);
        Visuals::playerModel(stage);
        Visuals::disablePostProcessing(stage);
        Visuals::removeVisualRecoil(stage);
        Visuals::applyZoom(stage);
        SkinChanger::run(stage);
        Misc::fixAnimationLOD(stage);
        Animations::renderStart(stage);
        Animations::handlePlayers(stage);
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

static void __stdcall overrideView(ViewSetup* setup) noexcept
{
    const auto zoomSensitivity = interfaces->cvar->findVar("zoom_sensitivity_ratio_mouse");
    static auto zoomSensitivityBackUp = zoomSensitivity->getFloat();
    if (localPlayer)
    {
        static bool once = true;
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->isSniperRifle() && localPlayer->isScoped())
        {
            if (config->visuals.keepFov)
            {
                if (once)
                {
                    zoomSensitivityBackUp = zoomSensitivity->getFloat();
                    once = false;
                }
                zoomSensitivity->setValue(0.f);
                setup->fov = 90.f;
            }
        }
        else if (!once)
        {
            zoomSensitivity->setValue(zoomSensitivityBackUp);
            once = true;
        }
    }

    if (localPlayer && ((localPlayer->isScoped() && config->visuals.keepFov) || !localPlayer->isScoped()))
        setup->fov += config->visuals.fov;

    setup->farZ += config->visuals.farZ * 10;

    if (localPlayer && localPlayer->isAlive() && config->misc.fakeduck && config->misc.fakeduckKey.isActive() && localPlayer->flags() & 1)
        setup->origin.z = localPlayer->getAbsOrigin().z + interfaces->gameMovement->getPlayerViewOffset(false).z;

    Misc::viewModelChanger(setup);

    hooks->clientMode.callOriginal<void, 18>(setup);

    Misc::freeCam(setup);
    Visuals::motionBlur(setup);
}

struct RenderableInfo {
    Entity* renderable;
    std::byte pad[18];
    uint16_t flags;
    uint16_t flags2;
};

static int __stdcall listLeavesInBox(const Vector& mins, const Vector& maxs, unsigned short* list, int listMax) noexcept
{
    if (config->misc.disableModelOcclusion && std::uintptr_t(_ReturnAddress()) == memory->insertIntoTree) {
        if (const auto info = *reinterpret_cast<RenderableInfo**>(std::uintptr_t(_AddressOfReturnAddress()) - sizeof(std::uintptr_t) + 0x18); info && info->renderable) {
            if (const auto ent = VirtualMethod::call<Entity*, 7>(info->renderable - sizeof(std::uintptr_t)); ent && ent->isPlayer()) {
                constexpr float maxCoord = 16384.0f;
                constexpr float minCoord = -maxCoord;
                constexpr Vector min{ minCoord, minCoord, minCoord };
                constexpr Vector max{ maxCoord, maxCoord, maxCoord };
                return hooks->bspQuery.callOriginal<int, 6>(std::cref(min), std::cref(max), list, listMax);
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

static bool __fastcall canUnduck(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->gameMovement.getOriginal<bool, 61>();

    const GameMovement* gameMovement = reinterpret_cast<GameMovement*>(thisPointer);

    const auto entity = gameMovement->player;

    if (!entity || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    if (entity->duckOverride())
        return false;

    if (entity->moveType() == MoveType::NOCLIP)
        return true;

    if (!entity->groundEntity())
        return original(thisPointer);

    static auto mp_solid_teammates = interfaces->cvar->findVar("mp_solid_teammates");
    if (mp_solid_teammates->getInt() == 1)
        return original(thisPointer);

    Vector newOrigin = localPlayer->getAbsOrigin();

    const auto HULL_MIN = Vector{ -16, -16, 0 };
    const auto HULL_MAX = Vector{ 16, 16, 72 };

    Trace trace;
    interfaces->engineTrace->traceRay({ newOrigin, newOrigin, HULL_MIN, HULL_MAX }, 0x201400B, { localPlayer.get() }, trace);

    if (trace.startSolid || trace.fraction != 1.f)
        return false;

    return true;
}

static void __fastcall buildTransformationsHook(void* thisPointer, void* edx, CStudioHdr* hdr, void* pos, void* q, matrix3x4* cameraTransform, int boneMask, void* boneComputed) noexcept
{
    static auto original = hooks->buildTransformations.getOriginal<void>(hdr, pos, q, cameraTransform, boneMask, boneComputed);

    const UtlVector<int> backupFlags = hdr->boneFlags;

    for (int i = 0; i < hdr->boneFlags.size; i++)
    {
        if(config->visuals.disableJiggleBones)
            hdr->boneFlags.elements[i] &= ~0x04;
        else
            hdr->boneFlags.elements[i] |= 0x04;
    }

    original(thisPointer, hdr, pos, q, cameraTransform, boneMask, boneComputed);

    hdr->boneFlags = backupFlags;
}

static void __fastcall doExtraBoneProcessingHook(void* thisPointer, void* edx, void* hdr, void* pos, void* q, const matrix3x4& matrix, uint8_t* bone_list, void* context) noexcept
{
    return;
}

static void __fastcall standardBlendingRulesHook(void* thisPointer, void* edx, void* hdr, void* pos, void* q, float currentTime, int boneMask) noexcept
{
    static auto original = hooks->standardBlendingRules.getOriginal<void>(hdr, pos, q, currentTime, boneMask);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    
    entity->getEffects() |= 8;

    original(thisPointer, hdr, pos, q, currentTime, boneMask);

    entity->getEffects() &= ~8;
}

static bool __fastcall shouldSkipAnimationFrameHook(void* thisPointer, void* edx) noexcept
{
    return false;
}

static void __fastcall updateClientSideAnimationHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->updateClientSideAnimation.getOriginal<void>();

    auto entity = reinterpret_cast<Entity*>(thisPointer);

    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || interfaces->engine->isHLTV())
        return original(thisPointer);

    if (entity != localPlayer.get())
    {
        if (Animations::isEntityUpdating())
            return original(thisPointer);
        return;
    }
    else if (entity == localPlayer.get())
    {
        if (Animations::isLocalUpdating())
            return original(thisPointer);
        return;
    }
}

static void __fastcall checkForSequenceChangeHook(void* thisPointer, void* edx, void* hdr, int currentSequence, bool forceNewSequence, bool interpolate) noexcept
{
    static auto original = hooks->checkForSequenceChange.getOriginal<void>(hdr, currentSequence, forceNewSequence, interpolate);

    return original(thisPointer, hdr, currentSequence, forceNewSequence, true);
}

static void __fastcall modifyEyePositionHook(void* thisPointer, void* edx, unsigned int* pos) noexcept
{
    static auto original = hooks->modifyEyePosition.getOriginal<void>(pos);

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, pos);

    const int bone = memory->lookUpBone(entity, "head_0");
    if (bone == -1)
        return;

    Vector eyePosition = reinterpret_cast<Vector&>(pos);

    if (animState->landing || animState->animDuckAmount != 0.f || !entity->groundEntity())
    {
        Vector bonePos;
        entity->getBonePos(bone, bonePos);
        bonePos.z += 1.7f;

        if (bonePos.z < eyePosition.z)
        {
            float lerpFraction = Helpers::simpleSplineRemapValClamped(fabsf(eyePosition.z - bonePos.z),
                FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MIN,
                FIRSTPERSON_TO_THIRDPERSON_VERTICAL_TOLERANCE_MAX,
                0.0f, 1.0f);

            eyePosition.z = Helpers::lerp(lerpFraction, eyePosition.z, bonePos.z);
        }
    }
    pos = reinterpret_cast<unsigned int*>(&eyePosition);
}

static void __fastcall calculateViewHook(void* thisPointer, void* edx, float* eyeOrigin, int eyeAngles, int zNear, int zFar, float* fov) noexcept
{
    static auto original = hooks->calculateView.getOriginal<void>(eyeOrigin, eyeAngles, zNear, zFar, fov);

    auto entity = reinterpret_cast<Entity*>(thisPointer);

    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, eyeOrigin, eyeAngles, zNear, zFar, fov);

    const auto oldUseNewAnimationState = entity->useNewAnimationState();

    entity->useNewAnimationState() = false;

    original(thisPointer, eyeOrigin, eyeAngles, zNear, zFar, fov);

    entity->useNewAnimationState() = oldUseNewAnimationState;
}

static void __vectorcall updateStateHook(void* thisPointer, void* unknown, float z, float y, float x, void* unknown1) noexcept
{
    using updateStateFn = void(__vectorcall*)(void*, void*, float, float, float, void*);
    static auto original = (updateStateFn)hooks->updateState.getDetour();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);
    if (!animState)
        return;

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->getModelPtr())
        return;
    
    if (!localPlayer || entity != localPlayer.get())
        return original(thisPointer, unknown, z, y, x, unknown1);

    return original(thisPointer, unknown, z, y, x, unknown1);
}

static void __fastcall resetStateHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->resetState.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity)
        return original(thisPointer);

    original(thisPointer);

    animState->lowerBodyRealignTimer = 0.f;
    animState->deployRateLimiting = false;
    animState->jumping = false;
    animState->buttons = 0;
}

static void __fastcall setupVelocityHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupVelocity.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    if(Animations::isFakeUpdating())
        return original(thisPointer);

    animState->setupVelocity();
}

static void __fastcall setupMovementHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupMovement.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    if (Animations::isFakeUpdating())
        return original(thisPointer);

    animState->setupMovement();
}

static void __fastcall setupAliveloopHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->setupAliveloop.getOriginal<void>();

    auto animState = reinterpret_cast<AnimState*>(thisPointer);

    auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !entity->isAlive() || !entity->isPlayer() || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    if (Animations::isFakeUpdating())
        return original(thisPointer);

    animState->setupAliveLoop();
}

static bool __fastcall setupBonesHook(void* thisPointer, void* edx, matrix3x4* boneToWorldOut , int maxBones, int boneMask, float currentTime) noexcept
{
    static auto original = hooks->setupBones.getOriginal<bool>(boneToWorldOut, boneMask, maxBones, currentTime);

    auto entity = reinterpret_cast<Entity*>(reinterpret_cast<uintptr_t>(thisPointer) - 4);

    if (!entity || !localPlayer || localPlayer.get() != entity || interfaces->engine->isHLTV())
        return original(thisPointer, boneToWorldOut, maxBones, boneMask, currentTime);

    if (!memory->input->isCameraInThirdPerson)
    {
        memory->setAbsAngle(localPlayer.get(), Vector{ 0.f, Animations::getFootYaw(), 0.f });
        return original(thisPointer, boneToWorldOut, maxBones, boneMask, currentTime);
    }

    if (Animations::isFakeUpdating())
        return original(thisPointer, boneToWorldOut, maxBones, boneMask, currentTime);

    if (!Animations::isLocalUpdating())
    {
        const auto poseParameters = localPlayer->poseParameters();
        localPlayer->poseParameters() = Animations::getPoseParameters();

        std::array<AnimationLayer, 13> layers;
        std::memcpy(&layers, localPlayer->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

        std::array<AnimationLayer, 13> layer = Animations::getAnimLayers();
        std::memcpy(localPlayer->animOverlays(), &layer, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());

        memory->setAbsAngle(localPlayer.get(), Vector{ 0.f, Animations::getFootYaw(), 0.f });
        original(thisPointer, nullptr, maxBones, boneMask, currentTime);

        localPlayer->poseParameters() = poseParameters;
        std::memcpy(localPlayer->animOverlays(), &layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayersCount());
        
        if (boneToWorldOut)
        {
            auto renderOrigin = entity->getRenderOrigin();
            auto realMatrix = Animations::getRealMatrix();
            for (auto& i : realMatrix)
            {
                i[0][3] += renderOrigin.x;
                i[1][3] += renderOrigin.y;
                i[2][3] += renderOrigin.z;
            }
            memcpy(boneToWorldOut, realMatrix.data(), sizeof(matrix3x4) * maxBones);
            renderOrigin = entity->getRenderOrigin();
            for (auto& i : realMatrix)
            {
                i[0][3] -= renderOrigin.x;
                i[1][3] -= renderOrigin.y;
                i[2][3] -= renderOrigin.z;
            }
        }
        return true;
    }
    else
        return original(thisPointer, boneToWorldOut, maxBones, boneMask, currentTime);
}

static void __cdecl clSendMoveHook() noexcept
{
    int nextCommandNr = memory->clientState->lastOutgoingCommand + memory->clientState->chokedCommands + 1;
    int chokedCommands = memory->clientState->chokedCommands;

    byte data[4000 /* MAX_CMD_BUFFER */];
    clMsgMove moveMsg;

    moveMsg.dataOut.startWriting(data, sizeof(data));

    const int backupCommands = 2;
    const int newCommands = max(chokedCommands + 1, 0);

    moveMsg.setNumBackupCommands(backupCommands);
    moveMsg.setNumNewCommands(newCommands);

    const int numCmds = newCommands + backupCommands;
    int from = -1;
    bool ok = true;

    for (int to = nextCommandNr - numCmds + 1; to <= nextCommandNr; ++to) {

        bool isnewcmd = to >= (nextCommandNr - newCommands + 1);

        ok = ok && interfaces->client->writeUsercmdDeltaToBuffer(0, &moveMsg.dataOut, from, to, isnewcmd);
        from = to;
    }

    if (ok) {

        moveMsg.setData(moveMsg.dataOut.getData(), moveMsg.dataOut.getNumBytesWritten());

        memory->clientState->netChannel->sendNetMsg(reinterpret_cast<void*>(&moveMsg));
    }
}
static void __cdecl clMoveHook(float accumulatedExtraSamples, bool finalTick) noexcept
{
    using clMoveFn = void(__cdecl*)(float, bool);
    static auto original = (clMoveFn)hooks->clMove.getDetour();

    static float realTime = 0.0f;

    if (!config->tickbase.enabled)
        return original(accumulatedExtraSamples, finalTick);

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return original(accumulatedExtraSamples, finalTick);

    if (!localPlayer || !localPlayer->isAlive())
        return original(accumulatedExtraSamples, finalTick);

    if (Tickbase::doubletapCharge < Tickbase::shiftAmount && memory->globalVars->realtime - realTime > 1.0f)
    {
        Tickbase::doubletapCharge++;
        return;
    }

    original(accumulatedExtraSamples, finalTick);

    if (Tickbase::lastShift == 0)
        return;

    Tickbase::isShifting = true;
    {
        for (Tickbase::ticksToShift = min(Tickbase::doubletapCharge, Tickbase::lastShift); Tickbase::ticksToShift > 0; Tickbase::ticksToShift--, Tickbase::doubletapCharge--)
            original(accumulatedExtraSamples, finalTick);
    }
    Tickbase::isShifting = false;
    Tickbase::lastShift = 0;
    realTime = memory->globalVars->realtime;
}





static void __fastcall getColorModulationHook(void* thisPointer, void* edx, float* r, float* g, float* b) noexcept
{
    static auto original = hooks->getColorModulation.getOriginal<void>(r, g, b);

    original(thisPointer, r, g, b);

    if (!config->visuals.mapColor.enabled)
        return;

    const auto material = reinterpret_cast<Material*>(thisPointer);
    if (!material)
        return;

    const std::string_view textureGroup = material->getTextureGroupName();
    if (!textureGroup.starts_with("World") && !textureGroup.starts_with("StaticProp"))
        return;

    const auto isProp = textureGroup.starts_with("StaticProp");
    if (config->visuals.mapColor.rainbow)
    {
        const auto [colorR, colorG, colorB] { rainbowColor(config->visuals.mapColor.rainbowSpeed) };
        *r *= colorR;
        *g *= colorG;
        *b *= colorB;
    }
    else
    {
        *r *= config->visuals.mapColor.color.at(0);
        *g *= config->visuals.mapColor.color.at(1);
        *b *= config->visuals.mapColor.color.at(2);
    }

    isProp ? *r *= 0.5f : *r *= 0.23f;
    isProp ? *g *= 0.5f : *g *= 0.23f;
    isProp ? *b *= 0.5f : *b *= 0.23f;
}

static bool __fastcall traceFilterForHeadCollisionHook(void* thisPointer, void* edx, Entity* player, unsigned int traceParams) noexcept
{
    static auto original = hooks->traceFilterForHeadCollision.getOriginal<bool>(player, traceParams);

    if (!localPlayer || !localPlayer->isAlive())
        return original(thisPointer, player, traceParams);

    if (!player || !player->isPlayer() || player == localPlayer.get())
        return original(thisPointer, player, traceParams);

    if (fabsf(player->getAbsOrigin().z - localPlayer->getAbsOrigin().z) < 10.0f)
        return false;

    return original(thisPointer, player, traceParams);
}

static bool __fastcall dispatchUserMessage(void* thisPointer, void* edx, int messageType, int argument, int secondArgument, void* data) noexcept
{
    static auto original = hooks->client.getOriginal<bool, 38>(messageType, argument, secondArgument, data);

    if (messageType == CS_UM_TextMsg || messageType == CS_UM_HudMsg || messageType == CS_UM_SayText)
    {
        if (config->misc.adBlock && !(*(memory->gameRules))->isValveDS())
            return true;
    }

    return original(thisPointer, messageType, argument, secondArgument, data);
}

static void __fastcall performScreenOverlayHook(void* thisPointer, void* edx, int x, int y, int width, int height) noexcept
{
    static auto original = hooks->performScreenOverlay.getOriginal<void>(x, y, width, height);

    if (!config->misc.adBlock || (*(memory->gameRules))->isValveDS())
        return original(thisPointer, x, y, width, height);
}

static bool __stdcall isDepthOfFieldEnabledHook() noexcept
{
    Visuals::motionBlur(nullptr);
    return false;
}

static bool __fastcall isUsingStaticPropDebugModesHook(void* thisPointer, void* edx) noexcept
{
    return config->visuals.mapColor.enabled;
}

static char __fastcall newFunctionBypass(void* thisPointer, void* edx, const char* moduleName) noexcept
{
    return 1;
}

static void __fastcall runCommand(void* thisPointer, void* edx, Entity* entity, UserCmd* cmd, MoveHelper* moveHelper)
{
    static auto original = hooks->prediction.getOriginal<void, 19, Entity*, UserCmd*, MoveHelper*>(entity, cmd, moveHelper);

    if (!entity || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, entity, cmd, moveHelper);

    original(thisPointer, entity, cmd, moveHelper);

    EnginePrediction::store();
}

static bool __fastcall postNetworkDataReceivedHook(void* thisPointer, void* edx, int commandsAcknowledged) noexcept
{
    const auto entity = reinterpret_cast<Entity*>(reinterpret_cast<uintptr_t>(thisPointer));
    static auto original = hooks->postNetworkDataReceived.getOriginal<bool>(commandsAcknowledged);
    if (!localPlayer || entity != localPlayer.get())
        return original(thisPointer, commandsAcknowledged);
    if (commandsAcknowledged <= 0)
        return false;

    static auto cl_showerror = interfaces->cvar->findVar("cl_showerror");

    bool haderrors = false;

    // Store network data into post networking pristine state slot (slot 64)
    memory->saveData(thisPointer, "PostNetworkDataReceived", SLOT_ORIGINALDATA, PC_EVERYTHING);

    // Show any networked fields that are different
    bool showthis = cl_showerror->getInt() >= 2;

    if (cl_showerror->getInt() < 0)
    {
        if (entity->index() == -cl_showerror->getInt())
        {
            showthis = true;
        }
        else
        {
            showthis = false;
        }
    }

    byte* predictedStateData = (byte*)entity->getPredictedFrame(commandsAcknowledged - 1);
    const byte* originalStateData = (const byte*)entity->getOriginalNetworkDataObject();

    PredictionCopy errorCheckHelper(PC_NETWORKED_ONLY,
        predictedStateData, TD_OFFSET_PACKED,
        originalStateData, TD_OFFSET_PACKED,
        showthis ?
        PredictionCopy::TRANSFERDATA_ERRORCHECK_SPEW :
        PredictionCopy::TRANSFERDATA_ERRORCHECK_NOSPEW);

    haderrors = errorCheckHelper.TransferData("", entity->index(), entity->getPredDescMap()) > 0 ? true : false;

    return haderrors;
}

static Vector* __fastcall eyeAnglesHook(void* thisPointer, void* edx) noexcept
{
    static auto original = hooks->eyeAngles.getOriginal<Vector*>();
    
    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (std::uintptr_t(_ReturnAddress()) != memory->eyePositionAndVectors || !localPlayer || entity != localPlayer.get())
        return original(thisPointer);

    return Animations::getLocalAngle();
}

void resetAll() noexcept
{
    Animations::reset();
    Logger::reset();
    EnginePrediction::reset();
    Glow::clearCustomObjects();
    Visuals::reset();
}

static void __fastcall levelShutDown(void* thisPointer) noexcept
{
    static auto original = hooks->client.getOriginal<void, 7>();

    original(thisPointer);
    resetAll();
}


Hooks::Hooks(HMODULE moduleHandle) noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    this->moduleHandle = moduleHandle;

    // interfaces and memory shouldn't be initialized in wndProc because they show MessageBox on error which would cause deadlock
    interfaces = std::make_unique<const Interfaces>();
    memory = std::make_unique<const Memory>();
    dynamicClassId = std::make_unique<const ClassIdManager>();

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

    newFunctionClientDLL.detour(memory->newFunctionClientDLL, newFunctionBypass);
    newFunctionEngineDLL.detour(memory->newFunctionEngineDLL, newFunctionBypass);
    newFunctionStudioRenderDLL.detour(memory->newFunctionStudioRenderDLL, newFunctionBypass);
    newFunctionMaterialSystemDLL.detour(memory->newFunctionMaterialSystemDLL, newFunctionBypass);

    sendDatagram.detour(memory->sendDatagram, sendDatagramHook);
    
    buildTransformations.detour(memory->buildTransformations, buildTransformationsHook);
    doExtraBoneProcessing.detour(memory->doExtraBoneProcessing, doExtraBoneProcessingHook);
    shouldSkipAnimationFrame.detour(memory->shouldSkipAnimationFrame, shouldSkipAnimationFrameHook);
    standardBlendingRules.detour(memory->standardBlendingRules, standardBlendingRulesHook);
    updateClientSideAnimation.detour(memory->updateClientSideAnimation, updateClientSideAnimationHook);

    setupVelocity.detour(memory->setupVelocity, setupVelocityHook);
    setupMovement.detour(memory->setupMovement, setupMovementHook);
    setupAliveloop.detour(memory->setupAliveloop, setupAliveloopHook);

    updateState.detour(memory->updateState, updateStateHook);
    resetState.detour(memory->resetState, resetStateHook);

    postDataUpdate.detour(memory->postDataUpdate, postDataUpdateHook);

    setupBones.detour(memory->setupBones, setupBonesHook);

    modifyEyePosition.detour(memory->modifyEyePosition, modifyEyePositionHook);
    calculateView.detour(memory->calculateView, calculateViewHook);
    clMove.detour(memory->clMove, clMoveHook);
    checkForSequenceChange.detour(memory->checkForSequenceChange, checkForSequenceChangeHook);

    getColorModulation.detour(memory->getColorModulation, getColorModulationHook);
    isUsingStaticPropDebugModes.detour(memory->isUsingStaticPropDebugModes, isUsingStaticPropDebugModesHook);

    traceFilterForHeadCollision.detour(memory->traceFilterForHeadCollision, traceFilterForHeadCollisionHook);
    performScreenOverlay.detour(memory->performScreenOverlay, performScreenOverlayHook);
    isDepthOfFieldEnabled.detour(memory->isDepthOfFieldEnabled, isDepthOfFieldEnabledHook);
    eyeAngles.detour(memory->eyeAngles, eyeAnglesHook);
    clSendMove.detour(memory->clSendMove, clSendMoveHook);
    //postNetworkDataReceived.detour(memory->postNetworkDataReceived, postNetworkDataReceivedHook);

    bspQuery.init(interfaces->engine->getBSPTreeQuery());

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(22, createMoveProxy);
    client.hookAt(37, frameStageNotify);
    client.hookAt(38, dispatchUserMessage);
    
    clientMode.init(memory->clientMode);
    clientMode.hookAt(17, shouldDrawFog);
    clientMode.hookAt(18, overrideView);
    clientMode.hookAt(27, shouldDrawViewModel);
    clientMode.hookAt(35, getViewModelFov);
    clientMode.hookAt(44, doPostScreenEffects);
    clientMode.hookAt(58, updateColorCorrectionWeights);

    clientState.init((ClientState*)(uint32_t(memory->clientState) + 0x8));
    clientState.hookAt(5, packetStart);

    engine.init(interfaces->engine);
    engine.hookAt(27, isConnected);
    engine.hookAt(82, isPlayingDemo);
    engine.hookAt(93, isHltv);
    engine.hookAt(101, getScreenAspectRatio);
    engine.hookAt(218, getDemoPlaybackParameters);

    fileSystem.init(interfaces->fileSystem);
    fileSystem.hookAt(101, getUnverifiedFileHashes);
    fileSystem.hookAt(128, canLoadThirdPartyFiles);

    gameMovement.init(interfaces->gameMovement);
    gameMovement.hookAt(32, onJump);
    //gameMovement.hookAt(61, canUnduck);

    modelRender.init(interfaces->modelRender);
    modelRender.hookAt(21, drawModelExecute);


    prediction.init(interfaces->prediction);
    prediction.hookAt(19, runCommand);

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
        *memory->dispatchSound = uintptr_t(&dispatchSound) - uintptr_t(memory->dispatchSound + 1);
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
    Resolver::updateEventListeners(true);

    if constexpr (std::is_same_v<HookType, MinHook>) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    bspQuery.restore();
    client.restore();
    clientMode.restore();
    clientState.restore();
    engine.restore();
    gameMovement.restore();
    modelRender.restore();
    prediction.restore();
    sound.restore();
    surface.restore();
    svCheats.restore();
    viewRender.restore();
    fileSystem.restore();
    Netvars::restore();

    Glow::clearCustomObjects();
    resetAll();

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
