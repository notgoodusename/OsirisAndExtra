#pragma once

#include <memory>
#include <string>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;
    void handleToggle() noexcept;
    bool isOpen() noexcept { return open; }
private:
    bool open = true;

    void renderGuiStyle() noexcept;
    void renderLegitbotWindow() noexcept;
    void renderRagebotWindow() noexcept;
    void renderTriggerbotWindow() noexcept;
    void renderFakelagWindow() noexcept;
    void renderTickbaseWindow() noexcept;
    void renderLegitAntiAimWindow() noexcept;
    void renderRageAntiAimWindow() noexcept;
    void renderFakeAngleWindow() noexcept;
    void renderBacktrackWindow() noexcept;
    void renderChamsWindow() noexcept;
    void renderStreamProofESPWindow() noexcept;
    void renderVisualsWindow() noexcept;
    void renderSkinChangerWindow() noexcept;
    void renderMiscWindow() noexcept;
    void renderConfigWindow() noexcept;

    struct {
        ImFont* normal15px = nullptr;
        ImFont* tahoma34 = nullptr;
    } fonts;

    float timeToNextConfigRefresh = 0.1f;
};

inline std::unique_ptr<GUI> gui;
