#pragma once

#include "../SDK/FrameStage.h"
#include "../SDK/Vector.h"

#include <array>

struct UserCmd;
struct Vector;

static const float epsilon = 0.03125f;

namespace EnginePrediction
{
	void reset() noexcept;

	void update() noexcept;
	void run(UserCmd* cmd) noexcept;

	void store() noexcept;
    void apply(FrameStage) noexcept;
    
	int getFlags() noexcept;
	Vector getVelocity() noexcept;
	bool isInPrediction() noexcept;

	struct NetvarData
	{
		int tickbase = -1;

		Vector aimPunchAngle{ };
		Vector aimPunchAngleVelocity{ };
		Vector baseVelocity{ };
		float duckAmount{ 0.0f };
		float duckSpeed{ 0.0f };
		float fallVelocity{ 0.0f };
		float thirdPersonRecoil{ 0.0f };
		Vector velocity{ };
		float velocityModifier{ 0.0f };
		Vector viewPunchAngle{ };
		Vector viewOffset{ };

		static float checkDifference(float predicted, float original) noexcept
		{
			float delta = predicted - original;

			if (fabsf(delta) <= epsilon)
				return original;
			return predicted;
		}

		static Vector checkDifference(Vector predicted, Vector original) noexcept
		{
			Vector delta = predicted - original;

			if (fabsf(delta.x) <= epsilon
				&& fabsf(delta.y) <= epsilon
				&& fabsf(delta.z) <= epsilon)
			{
				return original;
			}
			return predicted;
		}
	};
}
