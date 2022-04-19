#pragma once

#include "Animations.h"

#include "../SDK/GameEvent.h"

namespace Resolver
{
	void runPlayer(int index) noexcept;
	void processMissedShots() noexcept;
	void saveRecord(int playerIndex, float playerSimulationTime) noexcept;
	void getEvent(GameEvent* event) noexcept;
	void updateEventListeners(bool forceRemove = false) noexcept;

	struct SnapShot
	{
		Animations::Players player;
		Vector eyePosition{};
		bool getImpact{ true };
		Vector bulletImpact{};
		float time{ -1 };
		int playerIndex{ -1 };
	};
}