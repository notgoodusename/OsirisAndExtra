#pragma once
#include "../SDK/matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/ModelInfo.h"

#include <array>

struct UserCmd;
enum class FrameStage;

namespace Animations
{
	void init() noexcept;

	void update(UserCmd*, bool& sendPacket) noexcept;

	void real(FrameStage) noexcept;
	void fake() noexcept;

	void packetStart() noexcept;
	void packetEnd() noexcept;

	void handlePlayers(FrameStage) noexcept;

	bool isLocalUpdating() noexcept;
	bool isEntityUpdating() noexcept;
	bool isFakeUpdating() noexcept;

	bool gotFakeMatrix() noexcept;
	std::array<matrix3x4, MAXSTUDIOBONES> getFakeMatrix() noexcept;

	struct Players
	{
		Players()
		{
			this->clear();
		}
		std::array<matrix3x4, MAXSTUDIOBONES> matrix;
		std::array<AnimationLayer, 13> layers { };

		Vector mins{}, maxs{};
		Vector lastOrigin{};
		Vector velocity{};

		float simulationTime{ 0.f };
		int chokedPackets{ 0 };
		bool gotMatrix{ false };

		void clear()
		{
			simulationTime = 0.f;
			chokedPackets = 0;
			gotMatrix = false;
			lastOrigin = Vector{};
			velocity = Vector{};
			mins = Vector{};
			maxs = Vector{};
		}
	};
	Players getPlayer(int index) noexcept;
}