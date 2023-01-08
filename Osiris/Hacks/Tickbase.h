#pragma once

struct UserCmd;

namespace Tickbase
{
	void start(UserCmd* cmd) noexcept;
	void end(UserCmd* cmd) noexcept;

	bool shift(UserCmd* cmd, int shiftAmount, bool forceShift = false) noexcept;

	bool canRun() noexcept;
	bool canShift(int shiftAmount, bool forceShift = false) noexcept;

	int getCorrectTickbase(int commandNumber) noexcept;

	int& pausedTicks() noexcept;
	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;

	bool& isFinalTick() noexcept;
	bool& isShifting() noexcept;

	void updateInput() noexcept;
	void reset() noexcept;
}