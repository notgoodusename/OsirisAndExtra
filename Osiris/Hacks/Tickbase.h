#pragma once

struct UserCmd;

namespace Tickbase
{
	void run(UserCmd* cmd, bool sendPacket) noexcept;

	bool canRun() noexcept;
	bool canFire(int shiftAmount) noexcept;

	int getCorrectTickbase(int commandNumber) noexcept;
	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;
	void reset() noexcept;
}