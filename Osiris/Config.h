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
        int minDamageOverride{ 1 };
    };
    std::array<Ragebot, 40> ragebot;
    KeyBind ragebotKey{ std::string("ragebot") };
    KeyBind minDamageOverrideKey{ std::string("min damage override"), KeyMode::Off };

    struct Fakelag {
        bool enabled = false;
        int mode = 0;
        int limit = 1;
    } fakelag;

    struct RageAntiAimConfig {
        bool enabled = false;
        int pitch = 0; //Off, Down, Zero, Up
        Yaw yawBase = Yaw::off;
        KeyBind manualForward{ std::string("manual forward"), KeyMode::Off },
            manualBackward{ std::string("manual backward"), KeyMode::Off },
            manualRight{ std::string("manual right"), KeyMode::Off },
            manualLeft{ std::string("manual left"), KeyMode::Off };
        int yawModifier = 0; //Off, Jitter
        int yawAdd = 0; //-180/180
        int spinBase = 0; //-180/180
        int jitterRange = 0;
        bool atTargets = false;
    } rageAntiAim;

    struct FakeAngle {
        bool enabled = false;
        KeyBind invert{ std::string("fake angle invert") };
        int leftLimit = 60;
        int rightLimit = 60;
        int peekMode = 0; //Off, Peek real, Peek fake
        int lbyMode = 0; // Normal, Opposite, sway, 
    } fakeAngle;

    struct Tickbase {
        KeyBind doubletap{ std::string("doubletap"), KeyMode::Off };
        KeyBind hideshots{ std::string("hideshots"), KeyMode::Off };
        bool teleport{ false };
    } tickbase;

    struct LegitAntiAimConfig {
        bool enabled = false;
        bool extend = false;
        KeyBind invert{ std::string("legit aa invert") };
    } legitAntiAim;

    bool disableInFreezetime{ true };

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
    ColorToggleOutline legitbotFov{ 1.0f, 1.0f, 1.0f, 0.25f };

    struct RecoilControlSystem {
        bool enabled{ false };
        bool silent{ false };
        int shotsFired{ 0 };
        float horizontal{ 1.0f };
        float vertical{ 1.0f };
    } recoilControlSystem;

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

    struct GlowItem : Color4 {
        bool enabled = false;
        bool healthBased = false;
        int style = 0;
    };

    struct PlayerGlow {
        GlowItem all, visible, occluded;
    };

    std::unordered_map<std::string, PlayerGlow> playerGlow;
    std::unordered_map<std::string, GlowItem> glow;
    KeyBind glowKey{ std::string("glow") };


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
        bool noViewBob{ false };
        bool noHands{ false };
        bool noSleeves{ false };
        bool noWeapons{ false };
        bool noSmoke{ false };
        bool wireframeSmoke{ false };
        bool noMolotov{ false };
        bool wireframeMolotov{ false };
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
        bool zoom{ false };
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
        ColorToggle3 world;
        ColorToggle3 props;
        ColorToggle3 sky;
        std::string customSkybox;
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
        
        struct FootstepESP {
            ColorToggle footstepBeams{ 0.2f, 0.5f, 1.f, 1.0f };
            int footstepBeamRadius = 0;
            int footstepBeamThickness = 0;
        } footsteps;
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
        struct MolotovPolygon
        {
            bool enabled{ false };
            Color4 enemy{ 1.f, 0.27f, 0.f, 0.3f };
            Color4 team{ 0.37f, 1.f, 0.37f, 0.3f };
            Color4 self{ 1.f, 0.09f, 0.96f, 0.3f };
        } molotovPolygon;
        struct Viewmodel
        {
            bool enabled { false };
            int fov{ 0 };
            float x { 0.0f };
            float y { 0.0f };
            float z { 0.0f };
            float roll { 0.0f };
        } viewModel;
        struct OnHitHitbox
        {
            ColorToggle color{ 1.f, 1.f, 1.f, 1.f };
            float duration = 2.f;
        } onHitHitbox;
        ColorToggleOutline spreadCircle { 1.0f, 1.0f, 1.0f, 0.25f };
        int asusWalls = 100;
        int asusProps = 100;
        bool smokeTimer{ false };
        Color4 smokeTimerBG{ 1.0f, 1.0f, 1.0f, 0.5f };
        Color4 smokeTimerTimer{ 0.0f, 0.0f, 1.0f, 1.0f };
        Color4 smokeTimerText{ 0.0f, 0.0f, 0.0f, 1.0f };
        bool molotovTimer{ false };
        Color4 molotovTimerBG{ 1.0f, 1.0f, 1.0f, 0.5f };
        Color4 molotovTimerTimer{ 0.0f, 0.0f, 1.0f, 1.0f };
        Color4 molotovTimerText{ 0.0f, 0.0f, 0.0f, 1.0f };
    } visuals;

    std::array<item_setting, 36> skinChanger;

    struct Misc {
        Misc() { clanTag[0] = '\0'; name[0] = '\0'; menuKey.keyMode = KeyMode::Toggle; }

        KeyBind menuKey = KeyBind::INSERT;
        bool antiAfkKick{ false };
        bool adBlock{ false };
        int forceRelayCluster{ 0 };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        bool customClanTag{ false };
        bool customName{ false };
        bool clocktag{ false };
        bool animatedClanTag{ false };
        bool fastDuck{ false };
        bool knifeBot{ false };
        int knifeBotMode{ 0 };
        bool moonwalk{ false };
        bool blockBot{ false };
        KeyBind blockBotKey{ std::string("block bot") };
        bool edgeJump{ false };
        KeyBind edgeJumpKey{ std::string("edgejump") };
        bool miniJump{ false };
        int miniJumpCrouchLock{ 0 };
        KeyBind miniJumpKey{ std::string("mini jump") };
        bool jumpBug{ false };
        KeyBind jumpBugKey{ std::string("jump bug") };
        bool edgeBug{ false };
        int edgeBugPredAmnt{ 20 };
        KeyBind edgeBugKey{ std::string("edge bug") };
        bool autoPixelSurf{ false };
        int autoPixelSurfPredAmnt{ 2 };
        KeyBind autoPixelSurfKey{ std::string("auto pixel surf") };
        bool slowwalk{ false };
        int slowwalkAmnt{ 0 };
        KeyBind slowwalkKey{ std::string("slowwalk") };
        bool fakeduck{ false };
        KeyBind fakeduckKey{ std::string("fakeduck") };
        ColorToggle autoPeek{ 1.0f, 1.0f, 1.0f, 1.0f };
        KeyBind autoPeekKey{ std::string("autopeek") };
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
        bool unhideConvars{ false };
        KillfeedChanger killfeedChanger;
        PreserveKillfeed preserveKillfeed;
        char clanTag[16];
        char name[16];
        ColorToggleThickness noscopeCrosshair;
        ColorToggleThickness recoilCrosshair;
        ColorToggleThickness nadeDamagePredict;
        Color4 nadeTrailPredict;
        Color4 nadeCirclePredict{ 0.f, 0.5f, 1.f, 1.f };

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
            ImVec2 pos;
        };
        Watermark watermark;
        float aspectratio{ 0 };
        std::string killMessageString{ "Gotcha!" };
        ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
        ColorToggle3 hurtIndicator{ 0.0f, 0.8f, 0.7f };
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

        struct JumpStats {
            bool enabled = false;
            bool showFails = true;
            bool showColorOnFail = false;
            bool simplifyNaming = false;
        } jumpStats;

        struct Velocity {
            bool enabled = false;
            float position{ 0.9f };
            float alpha{ 1.0f };
            ColorToggle color{ 1.0f, 1.0f, 1.0f, 1.0f };
        } velocity;

        struct KeyBoardDisplay {
            bool enabled = false;
            float position{ 0.8f };
            bool showKeyTiles = false;
            Color4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
        } keyBoardDisplay;
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
