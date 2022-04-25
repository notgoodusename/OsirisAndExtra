#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "imgui/imgui.h"
#include "Hacks/SkinChanger.h"
#include "ConfigStructs.h"
#include "InputUtil.h"

class Config {
public:
    Config() noexcept;
    void load(size_t, bool incremental) noexcept;
    void load(const char8_t* name, bool incremental) noexcept;
    void save(size_t) const noexcept;
    void add(const char*) noexcept;
    void remove(size_t) noexcept;
    void rename(size_t, const char*) noexcept;
    void reset() noexcept;
    void listConfigs() noexcept;
    void createConfigDir() const noexcept;
    void openConfigDir() const noexcept;

    constexpr auto& getConfigs() noexcept
    {
        return configs;
    }

    struct Ragebot {
        bool enabled{ false };
        bool aimlock{ false };
        bool silent{ false };
        bool friendlyFire{ false };
        bool visibleOnly{ true };
        bool scopedOnly{ true };
        bool ignoreFlash{ false };
        bool ignoreSmoke{ false };
        bool autoShot{ false };
        bool autoScope{ false };
        bool autoStop{ false };
        bool disableMultipointIfLowFPS{ false };
        bool disableBacktrackIfLowFPS{ false };
        bool betweenShots{ false };
        int priority{ 0 };
        float fov{ 0.0f };
        int hitboxes{ 0 };
        int hitChance{ 50 };
        int multiPoint{ 0 };
        int minDamage{ 1 };
    };
    std::array<Ragebot, 40> ragebot;
    KeyBind ragebotKey{ std::string("ragebot") };

    struct Fakelag {
        bool enabled = false;
        int mode = 0;
        int limit = 1;
    } fakelag;

    struct RageAntiAimConfig {
        bool enabled = false;
        int pitch = 0; //Off, Down, Zero, Up
        int yawBase = 0; //Off, Forward, Backward, Right, Left, Spin
        int yawAdd = 0; //-180/180
        int spinBase = 0; //-180/180
        bool atTargets = false;
    } rageAntiAim;
    struct Tickbase {
        bool enabled = false;
        bool teleport = false;
    } tickbase;


    struct FakeAngle {
        bool enabled = false;
        KeyBind invert{ std::string("fake angle invert") };
        int leftLimit = 60;
        int rightLimit = 60;
        int peekMode = 0; //Off, Peek real, Peek fake
        int lbyMode = 0; // Normal, Opposite, sway, 
    } fakeAngle;

    struct LegitAntiAimConfig {
        bool enabled = false;
        bool extend = false;
        KeyBind invert{ std::string("legit aa invert") };
    } legitAntiAim;

    struct Legitbot {
        bool enabled{ false };
        bool aimlock{ false };
        bool silent{ false };
        bool friendlyFire{ false };
        bool visibleOnly{ true };
        bool scopedOnly{ true };
        bool ignoreFlash{ false };
        bool ignoreSmoke{ false };
        bool autoScope{ false };
        float fov{ 0.0f };
        float smooth{ 1.0f };
        int reactionTime{ 100 };
        int hitboxes{ 0 };
        int minDamage{ 1 };
        bool killshot{ false };
        bool betweenShots{ true };
    };
    std::array<Legitbot, 40> legitbot;
    KeyBind legitbotKey{ std::string("legitbot") };

    struct Triggerbot {
        bool enabled = false;
        bool friendlyFire = false;
        bool scopedOnly = true;
        bool ignoreFlash = false;
        bool ignoreSmoke = false;
        bool killshot = false;
        int hitboxes = 0;
        int hitChance = 50;
        int shotDelay = 0;
        int minDamage = 1;
    };
    std::array<Triggerbot, 40> triggerbot;
    KeyBind triggerbotKey{ std::string("triggerbot") };

    struct Backtrack {
        bool enabled = false;
        bool ignoreSmoke = false;
        bool ignoreFlash = false;
        int timeLimit = 200;
        bool fakeLatency = false;
        int fakeLatencyAmount = 200;
    } backtrack;

    struct Chams {
        struct Material : Color4 {
            bool enabled = false;
            bool healthBased = false;
            bool blinking = false;
            bool wireframe = false;
            bool cover = false;
            bool ignorez = false;
            int material = 0;
        };
        std::array<Material, 7> materials;
    };

    std::unordered_map<std::string, Chams> chams;
    KeyBind chamsKey{ std::string("chams") };

    struct StreamProofESP {
        KeyBind key{ std::string("esp") };

        std::unordered_map<std::string, Player> allies;
        std::unordered_map<std::string, Player> enemies;
        std::unordered_map<std::string, Weapon> weapons;
        std::unordered_map<std::string, Projectile> projectiles;
        std::unordered_map<std::string, Shared> lootCrates;
        std::unordered_map<std::string, Shared> otherEntities;
    } streamProofESP;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    struct Visuals {
        bool disablePostProcessing{ false };
        bool inverseRagdollGravity{ false };
        bool noFog{ false };
        struct Fog
        {
            float start{ 0 };
            float end{ 0 };
            float density{ 0 };
        } fogOptions;
        ColorToggle3 fog;
        bool no3dSky{ false };
        bool noAimPunch{ false };
        bool noViewPunch{ false };
        bool noHands{ false };
        bool noSleeves{ false };
        bool noWeapons{ false };
        bool noSmoke{ false };
        bool noBlur{ false };
        bool noScopeOverlay{ false };
        bool noGrass{ false };
        bool noShadows{ false };
        struct ShadowsChanger
        {
            bool enabled{ false };
            int x{ 0 };
            int y{ 0 };
        } shadowsChanger;
        bool fullBright{ false };
        bool wireframeSmoke{ false };
        bool zoom{ false };
        bool noZoom{ false };
        KeyBind zoomKey{ std::string("zoom") };
        bool thirdperson{ false };
        KeyBind thirdpersonKey{ std::string("thirdperson") };
        int thirdpersonDistance{ 0 };
        bool freeCam{ false };
        KeyBind freeCamKey{ std::string("freecam") };
        int freeCamSpeed{ 2 };
        bool keepFov{ false };
        int fov{ 0 };
        int farZ{ 0 };
        int flashReduction{ 0 };
        int skybox{ 0 };
        bool deagleSpinner{ false };
        struct MotionBlur
        {
            bool enabled{ false };
            bool forwardEnabled{ false };
            float fallingMin{ 10.0f };
            float fallingMax{ 20.0f };
            float fallingIntensity{ 1.0f };
            float rotationIntensity{ 1.0f };
            float strength{ 1.0f };
        } motionBlur;
        int screenEffect{ 0 };
        int hitEffect{ 0 };
        float hitEffectTime{ 0.6f };
        int hitMarker{ 0 };
        float hitMarkerTime{ 0.6f };
        ColorToggle bulletImpacts{ 0.0f, 0.0f, 1.f, 0.5f };
        float bulletImpactsTime{ 4.f };
        int playerModelT{ 0 };
        int playerModelCT{ 0 };
        char playerModel[256] { };
        bool disableJiggleBones{ false };
        BulletTracers bulletTracers;
        ColorToggle molotovHull{ 1.0f, 0.27f, 0.0f, 0.3f };
        ColorToggle smokeHull{ 0.5f, 0.5f, 0.5f, 0.3f };
        struct Viewmodel
        {
            bool enabled { false };
            int fov{ 0 };
            float x { 0.0f };
            float y { 0.0f };
            float z { 0.0f };
            float roll { 0.0f };
        } viewModel;
        ColorToggleOutline spreadCircle { 1.0f, 1.0f, 1.0f, 0.25f };
        ColorToggle3 mapColor;
    } visuals;

    std::array<item_setting, 36> skinChanger;

    struct Misc {
        Misc() { clanTag[0] = '\0'; menuKey.keyMode = KeyMode::Toggle; }

        KeyBind menuKey = KeyBind::INSERT;
        bool antiAfkKick{ false };
        bool adBlock{ false };
        int forceRelayCluster{ 0 };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        bool customClanTag{ false };
        bool clocktag{ false };
        bool animatedClanTag{ false };
        bool fastDuck{ false };
        bool knifeBot{ false };
        int knifeBotMode{ 0 };
        bool moonwalk{ false };
        bool blockBot{ false };
        KeyBind blockBotKey{ std::string("block bot") };
        bool edgejump{ false };
        bool jumpBug{ false };
        bool slowwalk{ false };
        bool fakeduck{ false };
        ColorToggle autoPeek{ 1.0f, 1.0f, 1.0f, 1.0f };
        bool autoPistol{ false };
        bool autoReload{ false };
        bool autoAccept{ false };
        bool radarHack{ false };
        bool revealRanks{ false };
        bool revealMoney{ false };
        bool revealSuspect{ false };
        bool revealVotes{ false };
        bool disableModelOcclusion{ false };
        bool nameStealer{ false };
        bool disablePanoramablur{ false };
        bool killMessage{ false };
        bool nadePredict{ false };
        bool fixTabletSignal{ false };
        bool fastPlant{ false };
        bool fastStop{ false };
        bool prepareRevolver{ false };
        bool oppositeHandKnife = false;
        bool svPureBypass{ true };
        bool inventoryUnlocker{ false };
        PreserveKillfeed preserveKillfeed;
        char clanTag[16];
        KeyBind edgejumpkey{ std::string("edgejump") };
        KeyBind jumpBugKey{ std::string("jump bug") };
        KeyBind slowwalkKey{ std::string("slowwalk") };
        KeyBind fakeduckKey{ std::string("fakeduck") };
        KeyBind autoPeekKey{ std::string("autopeek") };
        ColorToggleThickness noscopeCrosshair;
        ColorToggleThickness recoilCrosshair;

        struct SpectatorList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        SpectatorList spectatorList;

        struct KeyBindList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        KeyBindList keybindList;

        struct Logger {
            int modes{ 0 };
            int events{ 0 };
        };

        Logger loggerOptions;

        ColorToggle3 logger;

        struct Watermark {
            bool enabled = false;
        };
        Watermark watermark;
        float aspectratio{ 0 };
        std::string killMessageString{ "Gotcha!" };
        ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
        KeyBind prepareRevolverKey{ std::string("prepare revolver") };
        int hitSound{ 0 };
        int quickHealthshotKey{ 0 };
        float maxAngleDelta{ 255.0f };
        int killSound{ 0 };
        std::string customKillSound;
        std::string customHitSound;
        PurchaseList purchaseList;

        struct Reportbot {
            bool enabled = false;
            bool textAbuse = false;
            bool griefing = false;
            bool wallhack = true;
            bool aimbot = true;
            bool other = true;
            int target = 0;
            int delay = 1;
            int rounds = 1;
        } reportbot;

        struct PlayerList {
            bool enabled = false;
            bool steamID = false;
            bool rank = false;
            bool wins = false;
            bool money = true;
            bool health = true;
            bool armor = false;

            ImVec2 pos;
        };

        PlayerList playerList;
        OffscreenEnemies offscreenEnemies;
        AutoBuy autoBuy;
    } misc;

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;
    const auto& getSystemFonts() noexcept { return systemFonts; }
    const auto& getFonts() noexcept { return fonts; }
private:
    std::vector<std::string> scheduledFonts{ "Default" };
    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;
    std::filesystem::path path;
    std::vector<std::string> configs;
};

inline std::unique_ptr<Config> config;
