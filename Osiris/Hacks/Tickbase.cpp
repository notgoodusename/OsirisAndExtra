#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"

#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"

int Tickbase::getCorrectTickbase(int commandNumber) noexcept
{
	const int tickBase = localPlayer->tickBase();

	return tickBase;
}