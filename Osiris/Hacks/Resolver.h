#pragma once
#include <vector>
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

	void run(UserCmd* cmd) noexcept;
	void basic(Entity* entity) noexcept;

	inline std::vector<Record> records;
    inline Record invalidRecord;
}