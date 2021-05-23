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

	void players(FrameStage) noexcept;

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

		float oldSimtime{ 0.f };
		float currentSimtime{ 0.f };
		int chokedPackets{ 0 };
		bool gotMatrix{ false };

		void clear()
		{
			oldSimtime = 0;
			currentSimtime = 0;
			chokedPackets = 0;
			gotMatrix = false;
			lastOrigin = Vector{};
			velocity = Vector{};
		}
	};

	struct Datas
	{
		std::array<AnimationLayer, 13> layers{};
		bool update{ false };
		bool updating{ false };
		bool sendPacket{ true };
		bool gotMatrix{ false };
		Vector viewangles{};
		matrix3x4 fakematrix[MAXSTUDIOBONES]{};
		std::array<Players, 65> player;
	};
	extern Datas data;
}