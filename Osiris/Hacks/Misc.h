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
    void gatherDataOnTick(UserCmd* cmd) noexcept;
    void handleKeyEvent(int keynum, const char* currentBinding) noexcept;
    void drawKeyDisplay(ImDrawList* drawList) noexcept;
    void drawVelocity(ImDrawList* drawList) noexcept;
    void gotJump() noexcept;
    void jumpStats(UserCmd* cmd) noexcept;
    void miniJump(UserCmd* cmd) noexcept;
    void autoPixelSurf(UserCmd* cmd) noexcept;
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
    void initHiddenCvars() noexcept;
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
