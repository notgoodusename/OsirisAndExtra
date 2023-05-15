#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string_view>
#include <utility>
#include <Windows.h>
#include <Psapi.h>

#include "SDK/LocalPlayer.h"

#include "Interfaces.h"
#include "Memory.h"


template <typename T>
static constexpr auto relativeToAbsolute(uintptr_t address) noexcept
{
    return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
}

static std::pair<void*, std::size_t> getModuleInformation(const char* name) noexcept
{
    if (HMODULE handle = GetModuleHandleA(name)) {
        if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
            return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
    }
    return {};
}

[[nodiscard]] static auto generateBadCharTable(std::string_view pattern) noexcept
{
    assert(!pattern.empty());

    std::array<std::size_t, (std::numeric_limits<std::uint8_t>::max)() + 1> table;

    auto lastWildcard = pattern.rfind('?');
    if (lastWildcard == std::string_view::npos)
        lastWildcard = 0;

    const auto defaultShift = (std::max)(std::size_t(1), pattern.length() - 1 - lastWildcard);
    table.fill(defaultShift);

    for (auto i = lastWildcard; i < pattern.length() - 1; ++i)
        table[static_cast<std::uint8_t>(pattern[i])] = pattern.length() - 1 - i;

    return table;
}

static std::uintptr_t findPattern(const char* moduleName, std::string_view pattern, bool reportNotFound = true) noexcept
{
    static auto id = 0;
    ++id;

    const auto [moduleBase, moduleSize] = getModuleInformation(moduleName);

    if (moduleBase && moduleSize) {
        const auto lastIdx = pattern.length() - 1;
        const auto badCharTable = generateBadCharTable(pattern);

        auto start = static_cast<const char*>(moduleBase);
        const auto end = start + moduleSize - pattern.length();

        while (start <= end) {
            int i = lastIdx;
            while (i >= 0 && (pattern[i] == '?' || start[i] == pattern[i]))
                --i;

            if (i < 0)
                return reinterpret_cast<std::uintptr_t>(start);

            start += badCharTable[static_cast<std::uint8_t>(start[lastIdx])];
        }
    }

    if(reportNotFound)
        MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "Osiris", MB_OK | MB_ICONWARNING);
    return 0;
}

Memory::Memory() noexcept
{
    present = findPattern("gameoverlayrenderer", "\xFF\x15????\x8B\xF0\x85\xFF") + 2;
    reset = findPattern("gameoverlayrenderer", "\xC7\x45?????\xFF\x15????\x8B\xD8") + 9;

    clientMode = **reinterpret_cast<ClientMode***>((*reinterpret_cast<uintptr_t**>(interfaces->client))[10] + 5);
    input = *reinterpret_cast<Input**>((*reinterpret_cast<uintptr_t**>(interfaces->client))[16] + 1);
    globalVars = **reinterpret_cast<GlobalVars***>((*reinterpret_cast<uintptr_t**>(interfaces->client))[11] + 10);
    glowObjectManager = *reinterpret_cast<GlowObjectManager**>(findPattern(CLIENT_DLL, "\x0F\x11\x05????\x83\xC8\x01") + 3);
    disablePostProcessing = *reinterpret_cast<bool**>(findPattern(CLIENT_DLL, "\x83\xEC\x4C\x80\x3D") + 5);
    loadSky = relativeToAbsolute<decltype(loadSky)>(findPattern(ENGINE_DLL, "\xE8????\x84\xC0\x74\x2D\xA1") + 1);
    setClanTag = reinterpret_cast<decltype(setClanTag)>(findPattern(ENGINE_DLL, "\x53\x56\x57\x8B\xDA\x8B\xF9\xFF\x15"));
    lineGoesThroughSmoke = relativeToAbsolute<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x4C\x24\x30\x33\xD2") + 1);
    cameraThink = findPattern(CLIENT_DLL, "\x85\xC0\x75\x30\x38\x87");
    getSequenceActivity = reinterpret_cast<decltype(getSequenceActivity)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x8B\xF1\x83"));
    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\x8B\xCE\xE8????\x02\xC0") + 3);
    auto temp = reinterpret_cast<std::uintptr_t*>(findPattern(CLIENT_DLL, "\xB9????\xE8????\x8B\x5D\x08") + 1);
    hud = *temp;
    findHudElement = relativeToAbsolute<decltype(findHudElement)>(reinterpret_cast<uintptr_t>(temp) + 5);
    clearHudWeapon = relativeToAbsolute<decltype(clearHudWeapon)>(findPattern(CLIENT_DLL, "\xE8????\x8B\xF0\xC6\x44\x24??\xC6\x44\x24") + 1);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB7\x0F") + 1);
    setAbsOrigin = relativeToAbsolute<decltype(setAbsOrigin)>(findPattern(CLIENT_DLL, "\xE8????\xEB\x19\x8B\x07") + 1);
    insertIntoTree = findPattern(CLIENT_DLL, "\x56\x52\xFF\x50\x18") + 5;
    dispatchSound = reinterpret_cast<int*>(findPattern(ENGINE_DLL, "\x74\x0B\xE8????\x8B\x3D") + 3);
    traceToExit = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x4C\xF3\x0F\x10\x75");
    viewRender = **reinterpret_cast<ViewRender***>(findPattern(CLIENT_DLL, "\x8B\x0D????\xFF\x75\x0C\x8B\x45\x08") + 2);
    viewRenderBeams = *reinterpret_cast<ViewRenderBeams**>(findPattern(CLIENT_DLL, "\xB9????\x0F\x11\x44\x24?\xC7\x44\x24?????\xF3\x0F\x10\x84\x24") + 1);
    drawScreenEffectMaterial = relativeToAbsolute<uintptr_t>(findPattern(CLIENT_DLL, "\xE8????\x83\xC4\x0C\x8D\x4D\xF8") + 1);
    submitReportFunction = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x28\x8B\x4D\x08");
    const auto tier0 = GetModuleHandleW(L"tier0");
    debugMsg = reinterpret_cast<decltype(debugMsg)>(GetProcAddress(tier0, "Msg"));
    conColorMsg = reinterpret_cast<decltype(conColorMsg)>(GetProcAddress(tier0, "?ConColorMsg@@YAXABVColor@@PBDZZ"));
    vignette = *reinterpret_cast<float**>(findPattern(CLIENT_DLL, "\x0F\x11\x05????\xF3\x0F\x7E\x87") + 3) + 1;
    equipWearable = reinterpret_cast<decltype(equipWearable)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x10\x53\x8B\x5D\x08\x57\x8B\xF9"));
    predictionRandomSeed = *reinterpret_cast<int**>(findPattern(CLIENT_DLL, "\x8B\x0D????\xBA????\xE8????\x83\xC4\x04") + 2);
    moveData = **reinterpret_cast<MoveData***>(findPattern(CLIENT_DLL, "\xA1????\xF3\x0F\x59\xCD") + 1);
    moveHelper = **reinterpret_cast<MoveHelper***>(findPattern(CLIENT_DLL, "\x8B\x0D????\x8B\x45?\x51\x8B\xD4\x89\x02\x8B\x01") + 2);
    keyValuesFromString = relativeToAbsolute<decltype(keyValuesFromString)>(findPattern(CLIENT_DLL, "\xE8????\x83\xC4\x04\x89\x45\xD8") + 1);
    keyValuesFindKey = relativeToAbsolute<decltype(keyValuesFindKey)>(findPattern(CLIENT_DLL, "\xE8????\xF7\x45") + 1);
    keyValuesSetString = relativeToAbsolute<decltype(keyValuesSetString)>(findPattern(CLIENT_DLL, "\xE8????\x89\x77\x38") + 1);
    weaponSystem = *reinterpret_cast<WeaponSystem**>(findPattern(CLIENT_DLL, "\x8B\x35????\xFF\x10\x0F\xB7\xC0") + 2);
    getPlayerViewmodelArmConfigForPlayerModel = relativeToAbsolute<decltype(getPlayerViewmodelArmConfigForPlayerModel)>(findPattern(CLIENT_DLL, "\xE8????\x89\x87????\x6A") + 1);
    getEventDescriptor = relativeToAbsolute<decltype(getEventDescriptor)>(findPattern(ENGINE_DLL, "\xE8????\x8B\xD8\x85\xDB\x75\x27") + 1);
    activeChannels = *reinterpret_cast<ActiveChannels**>(findPattern(ENGINE_DLL, "\x8B\x1D????\x89\x5C\x24\x48") + 2);
    channels = *reinterpret_cast<Channel**>(findPattern(ENGINE_DLL, "\x81\xC2????\x8B\x72\x54") + 2);
    playerResource = *reinterpret_cast<PlayerResource***>(findPattern(CLIENT_DLL, "\x74\x30\x8B\x35????\x85\xF6") + 4);
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x66\x83\x3E") + 1);
    scopeDust = findPattern(CLIENT_DLL, "\xFF\x50\x3C\x8B\x4C\x24\x20") + 3;
    scopeArc = findPattern(CLIENT_DLL, "\x8B\x0D????\xFF\xB7????\x8B\x01\xFF\x90????\x8B\x7C\x24\x1C");
    demoOrHLTV = findPattern(CLIENT_DLL, "\x84\xC0\x75\x09\x38\x05");
    money = findPattern(CLIENT_DLL, "\x84\xC0\x75\x0C\x5B");
    demoFileEndReached = findPattern(CLIENT_DLL, "\x8B\xC8\x85\xC9\x74\x1F\x80\x79\x10");
    plantedC4s = *reinterpret_cast<decltype(plantedC4s)*>(findPattern(CLIENT_DLL, "\x7E\x2C\x8B\x15") + 4);
    gameRules = *reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\x8B\xEC\x8B\x0D????\x85\xC9\x74\x07") + 4);
    setOrAddAttributeValueByNameFunction = relativeToAbsolute<decltype(setOrAddAttributeValueByNameFunction)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x8D????\x85\xC9\x74\x10") + 1);
    registeredPanoramaEvents = reinterpret_cast<decltype(registeredPanoramaEvents)>(*reinterpret_cast<std::uintptr_t*>(findPattern(CLIENT_DLL, "\xE8????\xA1????\xA8\x01\x75\x21") + 6) - 36);
    makePanoramaSymbolFn = relativeToAbsolute<decltype(makePanoramaSymbolFn)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB7\x45\x0E\x8D\x4D\x0E") + 1);

    localPlayer.init(*reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\xA1????\x89\x45\xBC\x85\xC0") + 1));

    shouldDrawFogReturnAddress = relativeToAbsolute<std::uintptr_t>(findPattern(CLIENT_DLL, "\xE8????\x8B\x0D????\x0F\xB6\xD0") + 1) + 82;

    // Custom
    clientState = **reinterpret_cast<ClientState***>(findPattern(ENGINE_DLL, "\xA1????\x8B\x80????\xC3") + 1); //52
    memalloc = *reinterpret_cast<MemAlloc**>(GetProcAddress(GetModuleHandleA("tier0.dll"), "g_pMemAlloc"));
    setAbsAngle = reinterpret_cast<decltype(setAbsAngle)>(reinterpret_cast<DWORD*>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x64\x53\x56\x57\x8B\xF1")));
    createState = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\xF1\xB9????\xC7");
    updateState = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x18\x56\x57\x8B\xF9\xF3");
    resetState = findPattern(CLIENT_DLL, "\x56\x6A\x01\x68????\x8B\xF1");
    invalidateBoneCache = findPattern(CLIENT_DLL, "\x80\x3D?????\x74\x16\xA1????\x48\xC7\x81");

    setupVelocityAddress = *(reinterpret_cast<void**>(findPattern(CLIENT_DLL, "\x84\xC0\x75\x38\x8B\x0D????\x8B\x01\x8B\x80")));
    accumulateLayersAddress = *(reinterpret_cast<void**>(findPattern(CLIENT_DLL, "\x84\xC0\x75\x0D\xF6\x87")));
    standardBlendingRules = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF0\xB8\xF8\x10");

    smokeCount = *reinterpret_cast<int**>(findPattern(CLIENT_DLL, "\xA3????\x57\x8B\xCB") + 1);

    buildTransformations = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF0\x81\xEC????\x56\x57\x8B\xF9\x8B\x0D????\x89\x7C\x24\x28\x8B");
    doExtraBoneProcessing = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC????\x53\x56\x8B\xF1\x57\x89\x74\x24\x1C\x80");
    shouldSkipAnimationFrame = findPattern(CLIENT_DLL, "\x57\x8B\xF9\x8B\x07\x8B\x80????\xFF\xD0\x84\xC0\x75\x02");
    updateClientSideAnimation = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x56\x8B\xF1\x80\xBE?????\x74\x36");

    checkForSequenceChange = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x53\x8B\x5D\x08\x56\x8B\xF1\x57\x85");

    sendDatagram = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x83\xE4\xF0\xB8????\xE8????\x56\x57\x8B\xF9\x89\x7C\x24\x14");

    modifyEyePosition = relativeToAbsolute<decltype(modifyEyePosition)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x06\x8B\xCE\xFF\x90????\x85\xC0\x74\x50") + 1);

    lookUpBone = reinterpret_cast<decltype(lookUpBone)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x53\x56\x8B\xF1\x57\x83\xBE?????\x75\x14"));
    getBonePos = relativeToAbsolute<decltype(getBonePos)>(findPattern(CLIENT_DLL, "\xE8????\x8D\x14\x24") + 1);

    setCollisionBounds = relativeToAbsolute<decltype(setCollisionBounds)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xBF\x87????") + 1);
    calculateView = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x14\x53\x56\x57\xFF\x75\x18");

    setupVelocity = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x30\x56\x57\x8B\x3D");
    setupMovement = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC????\x56\x57\x8B\x3D????\x8B\xF1");
    setupAliveloop = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x53\x56\x57\x8B\xF9\x8B\x77\x60");

    randomSeed = reinterpret_cast<decltype(randomSeed)>(GetProcAddress(GetModuleHandleA(VSTDLIB_DLL), "RandomSeed"));
    randomFloat = reinterpret_cast<decltype(randomFloat)>(GetProcAddress(GetModuleHandleA(VSTDLIB_DLL), "RandomFloat"));

    getWeaponPrefix = reinterpret_cast<decltype(getWeaponPrefix)>(findPattern(CLIENT_DLL, "\x53\x56\x57\x8B\xF9\x33\xF6\x8B\x4F\x60\x8B\x01\xFF"));
    getLayerActivity = reinterpret_cast<decltype(getLayerActivity)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x8B\x35????\x57\x8B\xF9\x8B\xCE\x8B\x06\xFF\x90????\x8B\x7F\x60\x83"));

    lookUpSequence = relativeToAbsolute<decltype(lookUpSequence)>(findPattern(CLIENT_DLL, "\xE8????\x5E\x83\xF8\xFF") + 1);
    seqdesc = relativeToAbsolute<decltype(seqdesc)>(findPattern(CLIENT_DLL, "\xE8????\x03\x40\x04") + 1);
    getFirstSequenceAnimTag = relativeToAbsolute<decltype(getFirstSequenceAnimTag)>(findPattern(CLIENT_DLL, "\xE8????\xF3\x0F\x11\x86????\x0F\x57\xDB") + 1);
    getSequenceLinearMotion = relativeToAbsolute<decltype(getSequenceLinearMotion)>(findPattern(CLIENT_DLL, "\xE8????\xF3\x0F\x10\x4D?\x83\xC4\x08\xF3\x0F\x10\x45?\xF3\x0F\x59\xC0") + 1);
    sequenceDuration = relativeToAbsolute<decltype(sequenceDuration)>(findPattern(CLIENT_DLL, "\xE8????\xF3\x0F\x11\x86????\x51") + 1);
    lookUpPoseParameter = relativeToAbsolute<decltype(lookUpPoseParameter)>(findPattern(CLIENT_DLL, "\xE8????\x85\xC0\x79\x08") + 1);
    studioSetPoseParameter = relativeToAbsolute<decltype(studioSetPoseParameter)>(findPattern(CLIENT_DLL, "\xE8????\x0F\x28\xD8\x83\xC4\x04") + 1);
    calcAbsoluteVelocity = relativeToAbsolute<decltype(calcAbsoluteVelocity)>(findPattern(CLIENT_DLL, "\xE8????\x83\x7B\x30\x00") + 1);
    utilPlayerByIndex = reinterpret_cast<decltype(utilPlayerByIndex)>(findPattern(SERVER_DLL, "\x85\xC9\x7E\x32\xA1????"));
    drawServerHitboxes = findPattern(SERVER_DLL, "\x55\x8B\xEC\x81\xEC????\x53\x56\x8B\x35????\x8B\xD9\x57\x8B\xCE");
    postDataUpdate = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x53\x56\x8B\xF1\x57\x80??????\x74\x0A");

    setupBones = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF0\xB8\xD8");

    restoreEntityToPredictedFrame = reinterpret_cast<decltype(restoreEntityToPredictedFrame)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x4D?\x56\xE8????\x8B\x75"));

    markSurroundingBoundsDirty = relativeToAbsolute<decltype(markSurroundingBoundsDirty)>(findPattern(CLIENT_DLL, "\xE8????\x83\xFB\x01\x75\x41") + 1);
    isBreakableEntity = reinterpret_cast<decltype(isBreakableEntity)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x56\x8B\xF1\x85\xF6\x74\x68"));

    clSendMove = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x8B\x4D\x04\x81\xEC????");
    clMsgMoveSetData = relativeToAbsolute<decltype(clMsgMoveSetData)>(findPattern(ENGINE_DLL, "\xE8????\x8D\x7E\x18") + 1);
    clMsgMoveDescontructor = relativeToAbsolute<decltype(clMsgMoveDescontructor)>(findPattern(ENGINE_DLL, "\xE8????\x5F\x5E\x5B\x8B\xE5\x5D\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x55\x8B\xEC\x81\xEC????") + 1);
    clMove = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x81\xEC????\x53\x56\x8A\xF9");
    chokeLimit = findPattern(ENGINE_DLL, "\xB8????\x3B\xF0\x0F\x4F\xF0\x89\x5D\xFC") + 1;
    relayCluster = *reinterpret_cast<std::string**>(findPattern(STEAMNETWORKINGSOCKETS_DLL, "\xB8????\xB9????\x0F\x43") + 1);
    unlockInventory = findPattern(CLIENT_DLL, "\x84\xC0\x75\x05\xB0\x01\x5F");
    getColorModulation = findPattern(MATERIALSYSTEM_DLL, "\x55\x8B\xEC\x83\xEC?\x56\x8B\xF1\x8A\x46");
    isUsingStaticPropDebugModes = relativeToAbsolute<decltype(isUsingStaticPropDebugModes)>(findPattern(ENGINE_DLL, "\xE8????\x84\xC0\x8B\x45\x08") + 1);
    traceFilterForHeadCollision = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\x75\x0C\x57\x8B\xF9\xF7\xC6????");
    performScreenOverlay = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\xA1????\x53\x56\x8B\xD9");
    postNetworkDataReceived = relativeToAbsolute<decltype(postNetworkDataReceived)>(findPattern(CLIENT_DLL, "\xE8????\x33\xF6\x6A\x02") + 1);
    saveData = relativeToAbsolute<decltype(saveData)>(findPattern(CLIENT_DLL, "\xE8????\x6B\x45\x08\x34") + 1);
    isDepthOfFieldEnabled = findPattern(CLIENT_DLL, "\x8B\x0D????\x56\x8B\x01\xFF\x50\x34\x8B\xF0\x85\xF6\x75\x04");
    eyeAngles = findPattern(CLIENT_DLL, "\x56\x8B\xF1\x85\xF6\x74\x32");
    eyePositionAndVectors = findPattern(CLIENT_DLL, "\x8B\x55\x0C\x8B\xC8\xE8????\x83\xC4\x08\x5E\x8B\xE5");
    calcViewBob = findPattern(CLIENT_DLL, "\x55\x8B\xEC\xA1????\x83\xEC\x10\x56\x8B\xF1\xB9????");
    getClientModelRenderable = findPattern(CLIENT_DLL, "\x56\x8B\xF1\x80\xBE?????\x0F\x84????\x80\xBE");
    physicsSimulate = findPattern(CLIENT_DLL, "\x56\x8B\xF1\x8B?????\x83\xF9\xFF\x74\x23");
    updateFlashBangEffect = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x55\x04\x56\x8B\xF1\x57\x8D\x8E????");
    writeUsercmd = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x51\x53\x56\x8B\xD9\x8B\x0D");
    reevauluateAnimLODAddress = relativeToAbsolute<decltype(reevauluateAnimLODAddress)>(findPattern(CLIENT_DLL, "\xE8????\x8B\xCE\xE8????\x8B\x8F????") + 0x2B);
    physicsRunThink = reinterpret_cast<decltype(physicsRunThink)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x10\x53\x56\x57\x8B\xF9\x8B\x87"));
    checkHasThinkFunction = reinterpret_cast<decltype(checkHasThinkFunction)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x57\x8B\xF9\x8B\xB7????\x8B\xC6\xC1\xE8"));
    postThinkVPhysics = reinterpret_cast<decltype(postThinkVPhysics)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC????\x53\x8B\xD9\x56\x57\x83\xBB?????\x0F\x84"));
    simulatePlayerSimulatedEntities = reinterpret_cast<decltype(simulatePlayerSimulatedEntities)>(findPattern(CLIENT_DLL, "\x56\x8B\xF1\x57\x8B\xBE????\x83\xEF\x01\x78\x74"));

    predictionPlayer = *reinterpret_cast<int**>(findPattern(CLIENT_DLL, "\x89\x35????\xF3\x0F\x10\x48") + 0x2);

    newFunctionClientDLL = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\xF1\x33\xC0\x57\x8B\x7D\x08");
    newFunctionEngineDLL = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x56\x8B\xF1\x33\xC0\x57\x8B\x7D\x08");
    newFunctionStudioRenderDLL = findPattern(STUDIORENDER_DLL, "\x55\x8B\xEC\x56\x8B\xF1\x33\xC0\x57\x8B\x7D\x08");
    newFunctionMaterialSystemDLL = findPattern(MATERIALSYSTEM_DLL, "\x55\x8B\xEC\x56\x8B\xF1\x33\xC0\x57\x8B\x7D\x08");
    transferData = reinterpret_cast<decltype(transferData)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x45\x10\x53\x56\x8B\xF1\x57"));
    findLoggingChannel = reinterpret_cast<decltype(findLoggingChannel)>(GetProcAddress(GetModuleHandleA("tier0.dll"), "LoggingSystem_FindChannel"));
    logDirect = reinterpret_cast<decltype(logDirect)>(GetProcAddress(GetModuleHandleA("tier0.dll"), "LoggingSystem_LogDirect"));
    getGameModeNameFn = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x0D????\x53\x57\x8B\x01");

    createSimpleThread = reinterpret_cast<decltype(createSimpleThread)>(GetProcAddress(GetModuleHandleA("tier0.dll"), "CreateSimpleThread"));
    releaseThreadHandle = reinterpret_cast<decltype(releaseThreadHandle)>(GetProcAddress(GetModuleHandleA("tier0.dll"), "ReleaseThreadHandle"));
}
