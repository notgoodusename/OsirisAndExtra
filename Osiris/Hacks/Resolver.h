#pragma once

#include "Animations.h"

#include "../SDK/GameEvent.h"
#include "../SDK/Entity.h"

namespace Resolver
{
	void reset() noexcept;

	void processMissedShots() noexcept;
	void saveRecord(int playerIndex, float playerSimulationTime) noexcept;
	void getEvent(GameEvent* event) noexcept;

	void runPreUpdate(Animations::Players player, Entity* entity) noexcept;
	void runPostUpdate(Animations::Players player, Entity* entity) noexcept;

	float ResolveShot(Animations::Players player, Entity* entity);

	void detect_side(Entity* entity, int* side);

	void setup_detect(Animations::Players player, Entity* entity);

	void Antionetap(Animations::Players player, Entity* entity, Vector shot);

	void CmdGrabber(UserCmd* cmd);

	void ResolveEntity(Animations::Players player, Entity* entity);

	void updateEventListeners(bool forceRemove = false) noexcept;

	struct SnapShot
	{
		Animations::Players player;
		const Model* model{ };
		Vector eyePosition{};
		Vector bulletImpact{};
		bool gotImpact{ false };
		float time{ -1 };
		int playerIndex{ -1 };
		int backtrackRecord{ -1 };
	};
}