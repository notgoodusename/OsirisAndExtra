#pragma once

namespace Tickbase
{
	int getCorrectTickbase(int commandNumber) noexcept;
	
	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;
}