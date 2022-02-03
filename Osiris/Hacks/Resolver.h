#pragma once
#include <vector>
#include <array>
struct UserCmd;
class Entity;

namespace Resolver
{
	struct Record {
        float prevSimTime = 0.0f;
        int prevhealth = 0;
        int lastworkingshot = -1;
        int missedshots = 0;
        bool wasTargeted = false;
        bool invalid = true;
        bool FiredUpon = false;
        float PreviousEyeAngle = 0.0f;
        float eyeAnglesOnUpdate = 0.0f;
        float PreviousDesyncAng = 0.0f;

        bool wasUpdated = false;
	};

	void frameStageUpdate() noexcept;
	void basic(Entity* entity) noexcept;

	inline std::array<Record, 65> records{};
    inline Record invalidRecord{};
}