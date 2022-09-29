#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <string>

#include "SDK/Platform.h"

class ClientMode;
class ClientState;
class Entity;
class GameEventDescriptor;
class GameEventManager;
class Input;
class ItemSystem;
class KeyValues;
class MemAlloc;
class MoveHelper;
class MoveData;
class PlantedC4;
class PlayerResource;
class ViewRender;
class ViewRenderBeams;
class WeaponSystem;
template <typename Key, typename Value>
struct UtlMap;
template <typename T>
class UtlVector;

struct ActiveChannels;
struct Channel;
struct CStudioHdr;
struct GlobalVars;
struct GlowObjectManager;
struct PanoramaEventRegistration;
struct Trace;
struct Vector;

class Memory {
public:
    Memory() noexcept;

    std::uintptr_t present;
    std::uintptr_t reset;

    ClientMode* clientMode;
    Input* input;
    GlobalVars* globalVars;
    GlowObjectManager* glowObjectManager;
    UtlVector<PlantedC4*>* plantedC4s;
    UtlMap<short, PanoramaEventRegistration>* registeredPanoramaEvents;

    bool* disablePostProcessing;

    std::add_pointer_t<void __fastcall(const char*)> loadSky;
    std::add_pointer_t<void __fastcall(const char*, const char*)> setClanTag;
    std::uintptr_t cameraThink;
    std::add_pointer_t<bool __cdecl(Vector, Vector, short)> lineGoesThroughSmoke;
    int(__thiscall* getSequenceActivity)(void*, int);
    bool(__thiscall* isOtherEnemy)(Entity*, Entity*);
    std::uintptr_t hud;
    int* (__thiscall* findHudElement)(std::uintptr_t, const char*);
    int(__thiscall* clearHudWeapon)(int*, int);
    std::add_pointer_t<ItemSystem* __cdecl()> itemSystem;
    void(__thiscall* setAbsOrigin)(Entity*, const Vector&);
    std::uintptr_t insertIntoTree;
    int* dispatchSound;
    std::uintptr_t traceToExit;
    ViewRender* viewRender;
    ViewRenderBeams* viewRenderBeams;
    std::uintptr_t drawScreenEffectMaterial;
    std::add_pointer_t<void __cdecl(const char* msg, ...)> debugMsg;
    std::add_pointer_t<void __cdecl(const std::array<std::uint8_t, 4>& color, const char* msg, ...)> conColorMsg;
    float* vignette;
    int(__thiscall* equipWearable)(void* wearable, void* player);
    int* predictionRandomSeed;
    MoveData* moveData;
    MoveHelper* moveHelper;
    std::uintptr_t keyValuesFromString;
    KeyValues* (__thiscall* keyValuesFindKey)(KeyValues* keyValues, const char* keyName, bool create);
    void(__thiscall* keyValuesSetString)(KeyValues* keyValues, const char* value);
    WeaponSystem* weaponSystem;
    std::add_pointer_t<const char** __fastcall(const char* playerModelName)> getPlayerViewmodelArmConfigForPlayerModel;
    GameEventDescriptor* (__thiscall* getEventDescriptor)(GameEventManager* _this, const char* name, int* cookie);
    ActiveChannels* activeChannels;
    Channel* channels;
    PlayerResource** playerResource;
    const wchar_t* (__thiscall* getDecoratedPlayerName)(PlayerResource* pr, int index, wchar_t* buffer, int buffsize, int flags);
    std::uintptr_t scopeDust;
    std::uintptr_t scopeArc;
    std::uintptr_t demoOrHLTV;
    std::uintptr_t money;
    std::uintptr_t demoFileEndReached;
    Entity** gameRules;

    short makePanoramaSymbol(const char* name) const noexcept
    {
        short symbol;
        makePanoramaSymbolFn(&symbol, name);
        return symbol;
    }

    bool submitReport(const char* xuid, const char* report) const noexcept
    {
        return reinterpret_cast<bool(__stdcall*)(const char*, const char*)>(submitReportFunction)(xuid, report);
    }

    void setOrAddAttributeValueByName(std::uintptr_t attributeList, const char* attribute, float value) const noexcept
    {
        __asm movd xmm2, value
        setOrAddAttributeValueByNameFunction(attributeList, attribute);
    }

    void setOrAddAttributeValueByName(std::uintptr_t attributeList, const char* attribute, int value) const noexcept
    {
        setOrAddAttributeValueByName(attributeList, attribute, *reinterpret_cast<float*>(&value) /* hack, but CSGO does that */);
    }

    std::uintptr_t shouldDrawFogReturnAddress;

    // Custom
    ClientState* clientState;
    MemAlloc* memalloc;

    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    std::uintptr_t updateState;
    std::uintptr_t createState;
    std::uintptr_t resetState;
    std::uintptr_t invalidateBoneCache;
    void* setupVelocityAddress;
    void* accumulateLayersAddress;

    std::uintptr_t buildTransformations;
    std::uintptr_t doExtraBoneProcessing;
    std::uintptr_t standardBlendingRules;
    std::uintptr_t shouldSkipAnimationFrame;
    std::uintptr_t updateClientSideAnimation;
    std::uintptr_t checkForSequenceChange;
    std::uintptr_t sendDatagram;

    std::uintptr_t modifyEyePosition;

    int(__thiscall* lookUpBone)(void*, const char*);
    void(__thiscall* getBonePos)(void*, int, Vector*);

    void(__thiscall* setCollisionBounds)(void*, const Vector&, const Vector&);

    std::uintptr_t calculateView;

    std::uintptr_t setupVelocity;
    std::uintptr_t setupMovement;
    std::uintptr_t setupAliveloop;

    std::add_pointer_t<int __cdecl(const int, ...)> randomSeed;
    std::add_pointer_t<float __cdecl(const float, const float, ...)> randomFloat;

    const char* (__thiscall* getWeaponPrefix)(void*);
    int(__thiscall* getLayerActivity)(void*, int);

    int(__thiscall* lookUpSequence)(void*, const char*);
    int(__thiscall* seqdesc)(void*, int);
    float(__thiscall* getFirstSequenceAnimTag)(void*, int, int, int);
    void(__fastcall* getSequenceLinearMotion)(void*, int, float*, Vector*); // void __fastcall GetSequenceLinearMotion(_DWORD *studioHdr@<ecx>, int sequence@<edx>, int poseParameter, _DWORD *vectorReturn)
    float(__thiscall* sequenceDuration)(void*, int);
    int(__stdcall* lookUpPoseParameter)(CStudioHdr*, const char*);
    std::uintptr_t studioSetPoseParameter;
    void(__thiscall* calcAbsoluteVelocity)(void*);

    void*(__thiscall* utilPlayerByIndex)(int);
    std::uintptr_t drawServerHitboxes;
    std::uintptr_t postDataUpdate;

    std::uintptr_t setupBones;

    void(__stdcall* restoreEntityToPredictedFrame)(int, int);
    void(__thiscall* markSurroundingBoundsDirty)(void*);
    bool(__thiscall* isBreakableEntity)(void*);

    std::uintptr_t clSendMove;
    void(__thiscall* clMsgMoveSetData)(void*, unsigned char*, std::size_t);
    void(__thiscall* clMsgMoveDescontructor)(void*);
    std::uintptr_t clMove;
    std::uintptr_t chokeLimit;
    std::string* relayCluster;
    std::uintptr_t unlockInventory;
    std::uintptr_t getColorModulation;
    std::uintptr_t isUsingStaticPropDebugModes;
    std::uintptr_t traceFilterForHeadCollision;
    std::uintptr_t performScreenOverlay;
    std::uintptr_t postNetworkDataReceived;
    void(__thiscall* saveData)(void*, const char*, int, int);
    std::uintptr_t isDepthOfFieldEnabled;
    std::uintptr_t eyeAngles;
    std::uintptr_t eyePositionAndVectors;
    std::uintptr_t calcViewBob;
    std::uintptr_t getClientModelRenderable;
    std::uintptr_t physicsSimulate;
    std::uintptr_t updateFlashBangEffect;
    std::uintptr_t writeUsercmd;
    void* reevauluateAnimLODAddress;
    bool(__thiscall* physicsRunThink)(void*, int);
    void(__thiscall* checkHasThinkFunction)(void*, bool);
    bool(__thiscall* postThinkVPhysics)(void*);
    void(__thiscall* simulatePlayerSimulatedEntities)(void*);
    int* predictionPlayer;

    std::uintptr_t newFunctionClientDLL;
    std::uintptr_t newFunctionEngineDLL;
    std::uintptr_t newFunctionStudioRenderDLL;
    std::uintptr_t newFunctionMaterialSystemDLL;
    int(__thiscall* transferData)(void*, const char*, int, void*);
    int(__cdecl* findLoggingChannel)(const char* name);
    int(__cdecl* logDirect)(int id, int severity, const std::array<std::uint8_t, 4> color, const char* msg);

    void*(__cdecl* createSimpleThread)(void*, void*, unsigned long); // HANDLE(__cdecl* createSimpleThread)(LPVOID, LPVOID, SIZE_T);
    int(__cdecl* releaseThreadHandle)(void*); // int(__cdecl* releaseThreadHandle)(HANDLE);

    const char* getGameModeName(bool skirmish) const noexcept
    {
        return reinterpret_cast<const char* (__stdcall*)(bool)>(getGameModeNameFn)(skirmish);
    }
    //
private:
    void(__thiscall* setOrAddAttributeValueByNameFunction)(std::uintptr_t, const char* attribute);
    void(__thiscall* makePanoramaSymbolFn)(short* symbol, const char* name);

    std::uintptr_t submitReportFunction;
    std::uintptr_t getGameModeNameFn;
};

inline std::unique_ptr<const Memory> memory;
