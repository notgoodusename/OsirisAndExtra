#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "SDK/Platform.h"

class ClientMode;
class ClientState;
class Entity;
class FileSystem;
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

    FileSystem* fileSystem;
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
    std::uintptr_t listLeaves;
    int* dispatchSound;
    std::uintptr_t traceToExit;
    ViewRender* viewRender;
    ViewRenderBeams* viewRenderBeams;
    std::uintptr_t drawScreenEffectMaterial;
    std::uint8_t* fakePrime;
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

    // Custom
    ClientState* clientState;
    MemAlloc* memalloc;

    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    std::uintptr_t updateState;
    std::uintptr_t createState;
    std::uintptr_t resetState;
    std::uintptr_t InvalidateBoneCache;
    void* isLoadOutAvailable;
    void* SetupVelocityAddress;
    void* AccumulateLayersAddress;

    std::uintptr_t doExtraBoneProcessing;
    std::uintptr_t standardBlendingRules;
    std::uintptr_t shouldSkipAnimationFrame;
    std::uintptr_t updateClientSideAnimation;
    std::uintptr_t checkForSequenceChange;

    std::uintptr_t modifyEyePosition;

    int(__thiscall* lookUpBone)(void*, const char*);
    void(__thiscall* getBonePos)(void*, int, Vector*);

    void(__thiscall* setCollisionBounds)(void*, const Vector&, const Vector&);

    std::uintptr_t calculateView;

    std::uintptr_t setupVelocity;
    std::uintptr_t setupMovement;
    std::uintptr_t setupAliveloop;

    std::uintptr_t preDataUpdate;
    std::uintptr_t postDataUpdate;

    std::add_pointer_t<int __cdecl(const int, ...)> randomSeed;
    std::add_pointer_t<float __cdecl(const float, const float, ...)> randomFloat;

    const char* (__thiscall* getWeaponPrefix)(void*);
    void(__thiscall* addActivityModifier)(void*, const char*);
    void* (__thiscall* findMapping)(void*);
    int(__thiscall* getLayerActivity)(void*, int);
    float(__thiscall* getLayerIdealWeightFromSeqCycle)(void*, int);
    int(__thiscall* selectWeightedSequenceFromModifiers)(void*, void*, int, const void*, int);

    int(__thiscall* lookUpSequence)(void*, const char*);
    int(__thiscall* seqdesc)(void*, int);
    float(__thiscall* getFirstSequenceAnimTag)(void*, int, int, int);
    void(__fastcall* getSequenceLinearMotion)(void*, int, float*, Vector*); // void __fastcall GetSequenceLinearMotion(_DWORD *studioHdr@<ecx>, int sequence@<edx>, int poseParameter, _DWORD *vectorReturn)
    
    bool(__thiscall* initPoseParameter)(void*, void*, const char*); //char __thiscall InitPoseParameter(_DWORD *poseParameter, int player, int name)
    std::uintptr_t studioSetPoseParameter;
    std::uintptr_t notifyOnLayerChangeWeight;
    std::uintptr_t notifyOnLayerChangeCycle;
    void(__thiscall* calcAbsoluteVelocity)(void*);

    void* writeUsercmdDeltaToBufferReturn;
    std::uintptr_t writeUsercmd;
    std::uintptr_t clMove;
    //
private:
    void(__thiscall* setOrAddAttributeValueByNameFunction)(std::uintptr_t, const char* attribute);
    void(__thiscall* makePanoramaSymbolFn)(short* symbol, const char* name);

    std::uintptr_t submitReportFunction;
};

inline std::unique_ptr<const Memory> memory;
