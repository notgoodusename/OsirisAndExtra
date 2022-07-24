#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"

int tickShift{ 0 };

int Tickbase::getCorrectTickbase(int commandNumber) noexcept
{
	const int tickBase = localPlayer->tickBase();

	return tickBase;
}

int Tickbase::getTickshift() noexcept
{
	return tickShift;
}

void Tickbase::resetTickshift() noexcept
{
	tickShift = 0;
}