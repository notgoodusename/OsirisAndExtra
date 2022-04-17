#pragma once

#include "../SDK/GameEvent.h"

namespace Resolver
{
	void runPlayer(int index) noexcept;
	void saveRecord(int playerIndex, float playerSimulationTime) noexcept;
	void getEvent(GameEvent* event) noexcept;
	void updateEventListeners(bool forceRemove = false) noexcept;
}