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
        bool betweenShots{ false };
        int priority{ 0 };
        float fov{ 0.0f };
        int hitboxes{ 0 };
        int hitChance{ 50 };
        int multiPoint{ 0 };
        int minDamage{ 1 };
    };
    std::array<Ragebot, 40> ragebot;
    bool ragebotOnKey{ false };
    KeyBind ragebotKey = KeyBind::NONE;
    int ragebotKeyMode{ 0 };

    struct Fakelag {
        bool enabled = false;
        int mode = 0;
        int limit = 1;
    } fakelag;

    struct RageAntiAimConfig {
        bool enabled = false;
        int pitch = 0; //Off, Down, Zero, Up
        int yawBase = 0; //Off, Forward, Backward, Right, Left
        int yawAdd = 0; //-180/180
        bool atTargets = false;
    } rageAntiAim;

    struct FakeAngle {
        bool enabled = false;
        KeyBindToggle invert = KeyBind::NONE;
        int leftLimit = 60;
        int rightLimit = 60;
        int peekMode = 0; //Off, Peek real, Peek fake
        int lbyMode = 0; // Normal, Opposite, sway, 
    } fakeAngle;

    struct LegitAntiAimConfig {
        bool enabled = false;
        bool extend = false;
        KeyBindToggle invert = KeyBind::NONE;
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
    bool legitbotOnKey{ false };
    KeyBind legitbotKey = KeyBind::NONE;
    int legitbotKeyMode{ 0 };

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
    KeyBind triggerbotHoldKey = KeyBind::NONE;

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
    KeyBindToggle chamsToggleKey = KeyBind::NONE;
    KeyBind chamsHoldKey = KeyBind::NONE;

    struct StreamProofESP {
        KeyBindToggle toggleKey = KeyBind::NONE;
        KeyBind holdKey = KeyBind::NONE;

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
        bool fullBright{ false };
        bool wireframeSmoke{ false };
        bool zoom{ false };
        KeyBindToggle zoomKey = KeyBind::NONE;
        bool thirdperson{ false };
        KeyBindToggle thirdpersonKey = KeyBind::NONE;
        int thirdpersonDistance{ 0 };
        int viewmodelFov{ 0 };
        int fov{ 0 };
        int farZ{ 0 };
        int flashReduction{ 0 };
        int skybox{ 0 };
        bool deagleSpinner{ false };
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
        BulletTracers bulletTracers;
        ColorToggle molotovHull{ 1.0f, 0.27f, 0.0f, 0.3f };
    } visuals;

    std::array<item_setting, 36> skinChanger;

    struct Misc {
        Misc() { clanTag[0] = '\0'; }

        KeyBind menuKey = KeyBind::INSERT;
        bool antiAfkKick{ false };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        bool customClanTag{ false };
        bool clocktag{ false };
        bool animatedClanTag{ false };
        bool fastDuck{ false };
        bool knifeBot{ false };
        int knifeBotMode{ 0 };
        bool moonwalk{ false };
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
        bool svPureBypass{ false };
        PreserveKillfeed preserveKillfeed;
        char clanTag[16];
        KeyBind edgejumpkey = KeyBind::NONE;
        KeyBindToggle jumpBugKey = KeyBind::NONE;
        KeyBind slowwalkKey = KeyBind::NONE;
        KeyBindToggle fakeduckKey = KeyBind::NONE;
        KeyBindToggle autoPeekKey = KeyBind::NONE;
        ColorToggleThickness noscopeCrosshair;
        ColorToggleThickness recoilCrosshair;

        struct SpectatorList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        SpectatorList spectatorList;
        struct Watermark {
            bool enabled = false;
        };
        Watermark watermark;
        float aspectratio{ 0 };
        std::string killMessageString{ "Gotcha!" };
        ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
        KeyBind prepareRevolverKey = KeyBind::NONE;
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
