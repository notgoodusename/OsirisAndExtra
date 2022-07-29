#pragma once

#include <memory>
#include <type_traits>
#include <d3d9.h>
#include <Windows.h>

#include "Hooks/MinHook.h"
#include "Hooks/VmtHook.h"
#include "Hooks/VmtSwap.h"

#include "SDK/Platform.h"

struct SoundInfo;

using HookType = MinHook;

class Hooks {
public:
    Hooks(HMODULE moduleHandle) noexcept;

    WNDPROC originalWndProc;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;

    void install() noexcept;
    void uninstall() noexcept;

    std::add_pointer_t<int __fastcall(SoundInfo&)> originalDispatchSound;

    MinHook buildTransformations;
    MinHook doExtraBoneProcessing;
    MinHook shouldSkipAnimationFrame;
    MinHook standardBlendingRules;
    MinHook updateClientSideAnimation;
    MinHook checkForSequenceChange;

    MinHook modifyEyePosition;
    MinHook calculateView;
    MinHook updateState;

    MinHook resetState;
    MinHook setupVelocity;
    MinHook setupMovement;
    MinHook setupAliveloop;
    MinHook setupWeaponAction;

    MinHook sendDatagram;

    MinHook setupBones;
    MinHook eyeAngles;
    MinHook calcViewBob;

    MinHook postDataUpdate;

    MinHook clSendMove;
    MinHook clMove;

    MinHook getColorModulation;
    MinHook isUsingStaticPropDebugModes;
    MinHook traceFilterForHeadCollision;
    MinHook performScreenOverlay;
    MinHook postNetworkDataReceived;
    MinHook isDepthOfFieldEnabled;
    MinHook getClientModelRenderable;
    MinHook physicsSimulate;
    MinHook updateFlashBangEffect;

    MinHook newFunctionClientDLL;
    MinHook newFunctionEngineDLL;
    MinHook newFunctionStudioRenderDLL;
    MinHook newFunctionMaterialSystemDLL;

    HookType fileSystem;
    HookType bspQuery;
    HookType client;
    HookType clientMode;
    HookType clientState;
    HookType engine;
    HookType gameMovement;
    HookType modelRender;
    HookType sound;
    HookType surface;
    HookType viewRender;
    HookType svCheats;
private:
    HMODULE moduleHandle;
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
