#pragma once
#include <array>

#include "../imgui/imgui.h"

#include "../SDK/EngineTrace.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"

namespace NadePrediction {
	void run(UserCmd* cmd) noexcept;
	void draw() noexcept;
};