#pragma once

namespace Tickbase
{
	bool canRun() noexcept;
	bool canFire(int shiftAmount) noexcept;

	int getCorrectTickbase(int commandNumber) noexcept;
	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;
}