#pragma once

namespace Tickbase
{
	int getCorrectTickbase(int commandNumber) noexcept;
	
	int getTickshift() noexcept;

	void resetTickshift() noexcept;
}