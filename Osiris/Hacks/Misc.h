#pragma once

#include "../SDK/ClientMode.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Vector.h"

enum class FrameStage;
class GameEvent;
struct ImDrawList;
struct ViewSetup;

namespace Misc
{
    bool isInChat() noexcept;

    class JumpStatsCalculations
    {
    private:
        static const auto white = '\x01';
        static const auto violet = '\x03';
        static const auto green = '\x04';
        static const auto red = '\x07';
        static const auto golden = '\x09';
    public:
        void resetStats() noexcept;
        bool show() noexcept;
        void run(UserCmd* cmd) noexcept;
        
        //Last values
        short lastMousedx{ 0 };
        bool lastOnGround{ false };
        int lastButtons{ 0 };
        float oldVelocity{ 0.0f };
        bool jumpped{ false };
        Vector oldOrigin{ };
        Vector startPosition{ };

        //Current values
        float velocity{ 0.0f };
        bool onLadder{ false };
        bool onGround{ false };
        bool jumping{ false };
        bool jumpbugged{ false };
        bool isJumpbug{ false };
        bool hasJumped{ false };
        bool startedOnLadder{ false };
        bool isLadderJump{ false };
        bool shouldShow{ false };
        int jumps{ 0 };
        Vector origin{ };
        Vector landingPosition{ };
        int ticksInAir{ 0 };
        int ticksSynced{ 0 };

        //Final values
        float units{ 0.0f };
        int strafes{ 0 };
        float pre{ 0.0f };
        float maxVelocity{ 0.0f };
        float maxHeight{ 0.0f };
        int bhops{ 0 };
        float sync{ 0.0f };
    } jumpStatsCalculations;

    void jumpStats(UserCmd* cmd) noexcept;
    void edgeBug(UserCmd* cmd, Vector& angView) noexcept;
    void prePrediction(UserCmd* cmd) noexcept;
    void drawPlayerList() noexcept;
    void blockBot(UserCmd* cmd) noexcept; 
    void runFreeCam(UserCmd* cmd, Vector viewAngles) noexcept;
    void freeCam(ViewSetup* setup) noexcept;
    void viewModelChanger(ViewSetup* setup) noexcept;
    void drawAutoPeek(ImDrawList* drawList) noexcept;
    void autoPeek(UserCmd* cmd, Vector currentViewAngles) noexcept;
    void forceRelayCluster() noexcept;
    void jumpBug(UserCmd* cmd) noexcept;
    void unlockHiddenCvars() noexcept;
    void fakeDuck(UserCmd* cmd, bool& sendPacket) noexcept;
    void edgejump(UserCmd* cmd) noexcept;
    void slowwalk(UserCmd* cmd) noexcept;
    void inverseRagdollGravity() noexcept;
    void updateClanTag(bool = false) noexcept;
    void showKeybinds() noexcept;
    void spectatorList() noexcept;
    void noscopeCrosshair(ImDrawList* drawlist) noexcept;
    void recoilCrosshair(ImDrawList* drawList) noexcept;
    void watermark() noexcept;
    void prepareRevolver(UserCmd*) noexcept;
    void fastPlant(UserCmd*) noexcept;
    void fastStop(UserCmd*) noexcept;
    void drawBombTimer() noexcept;
    void hurtIndicator() noexcept;
    void stealNames() noexcept;
    void disablePanoramablur() noexcept;
    bool changeName(bool, const char*, float) noexcept;
    void bunnyHop(UserCmd*) noexcept;
    void fixTabletSignal() noexcept;
    void killfeedChanger(GameEvent& event) noexcept;
    void killMessage(GameEvent& event) noexcept;
    void fixMovement(UserCmd* cmd, float yaw) noexcept;
    void antiAfkKick(UserCmd* cmd) noexcept;
    void fixAnimationLOD(FrameStage stage) noexcept;
    void autoPistol(UserCmd* cmd) noexcept;
    void autoReload(UserCmd* cmd) noexcept;
    void revealRanks(UserCmd* cmd) noexcept;
    void autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept;
    void removeCrouchCooldown(UserCmd* cmd) noexcept;
    void moonwalk(UserCmd* cmd) noexcept;
    void playHitSound(GameEvent& event) noexcept;
    void killSound(GameEvent& event) noexcept;
    void autoBuy(GameEvent* event) noexcept;
    void purchaseList(GameEvent* event = nullptr) noexcept;
    void oppositeHandKnife(FrameStage stage) noexcept;
    void runReportbot() noexcept;
    void resetReportbot() noexcept;
    void preserveKillfeed(bool roundStart = false) noexcept;
    void voteRevealer(GameEvent& event) noexcept;
    void drawOffscreenEnemies(ImDrawList* drawList) noexcept;
    void autoAccept(const char* soundEntry) noexcept;

    void updateEventListeners(bool forceRemove = false) noexcept;
    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
