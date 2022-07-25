#pragma once

struct UserCmd;

namespace Tickbase
{
	void start() noexcept;
	void end(UserCmd* cmd) noexcept;

	bool shift(UserCmd* cmd, int shiftAmount) noexcept;

	bool canRun() noexcept;
	bool canShift(int shiftAmount) noexcept;

	int getCorrectTickbase(int commandNumber) noexcept;
	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;
	void reset() noexcept;
}