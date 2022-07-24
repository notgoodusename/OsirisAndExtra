#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"

int tickShift{ 0 };
int shiftCommand{ 0 };
int shiftedTickbase{ 0 };

int Tickbase::getCorrectTickbase(int commandNumber) noexcept
{
	const int tickBase = localPlayer->tickBase();

	if (commandNumber == shiftCommand)
		return tickBase - shiftedTickbase;
	else if (commandNumber == shiftCommand + 1)
		return tickBase + shiftedTickbase;

	return tickBase;
}

int Tickbase::getTickshift() noexcept
{
	return tickShift;
}

void Tickbase::resetTickshift() noexcept
{
	shiftedTickbase = tickShift;
	tickShift = 0;
}