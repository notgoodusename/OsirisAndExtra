#pragma once

#include "../SDK/FrameStage.h"
#include "../SDK/Vector.h"

#include <array>

struct UserCmd;
struct Vector;

static const float epsilon = 0.03125f;

namespace EnginePrediction
{
    void run(UserCmd* cmd) noexcept;

    void store() noexcept;
    void apply(FrameStage) noexcept;
    
    int getFlags() noexcept;
    Vector getVelocity() noexcept;

	struct NetvarData
	{
		int tickbase = -1;

		Vector aimPunchAngle{};
		Vector aimPunchAngleVelocity{};
		Vector viewPunchAngle{};
		Vector viewOffset{};

		static float checkDifference(float predicted, float original) noexcept
		{
			float delta = predicted - original;

			if (fabsf(delta) < epsilon)
				return original;
			return predicted;
		}

		static Vector checkDifference(Vector predicted, Vector original) noexcept
		{
			Vector delta = predicted - original;

			if (fabsf(delta.x) < epsilon
				&& fabsf(delta.y) < epsilon
				&& fabsf(delta.z) < epsilon)
			{
				return original;
			}
			return predicted;
		}
	};
}
