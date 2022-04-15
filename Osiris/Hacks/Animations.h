#pragma once
#include "../SDK/matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/ModelInfo.h"

#include <array>
#include <deque>

struct UserCmd;
enum class FrameStage;

namespace Animations
{
	void init() noexcept;

	void update(UserCmd*, bool& sendPacket) noexcept;
	void restore() noexcept;

	void renderStart(FrameStage) noexcept;
	void updateBacktrack() noexcept;
	void fake() noexcept;

	void packetStart() noexcept;
	void postDataUpdate() noexcept;

	Vector getLocalAngle() noexcept;

	void handlePlayers(FrameStage) noexcept;

	bool isLocalUpdating() noexcept;
	bool isEntityUpdating() noexcept;
	bool isFakeUpdating() noexcept;

	bool gotFakeMatrix() noexcept;
	std::array<matrix3x4, MAXSTUDIOBONES> getFakeMatrix() noexcept;

	bool gotRealMatrix() noexcept;
	std::array<matrix3x4, MAXSTUDIOBONES> getRealMatrix() noexcept;

	float getFootYaw() noexcept;
	std::array<float, 24> getPoseParameters() noexcept;
	std::array<AnimationLayer, 13> getAnimLayers() noexcept;

	struct Players
	{
		Players()
		{
			this->clear();
		}

		struct Record {
			Vector origin;
			Vector head;
			Vector absAngle;
			Vector mins;
			Vector maxs;
			float simulationTime;
			matrix3x4 matrix[MAXSTUDIOBONES];
		};

		std::deque<Record> backtrackRecords;

		std::array<matrix3x4, MAXSTUDIOBONES> matrix;
		std::array<AnimationLayer, 13> layers { };
		std::array<AnimationLayer, 13> oldlayers { };

		Vector mins{}, maxs{};
		Vector origin{}, absAngle{};
		Vector velocity{};

		float spawnTime{ 0.f };

		float simulationTime{ 0.f };
		int chokedPackets{ 0 };
		bool gotMatrix{ false };

		void clear()
		{
			gotMatrix = false;
			simulationTime = -1.0f;
			origin = Vector{};
			absAngle = Vector{};
			velocity = Vector{};
			mins = Vector{};
			maxs = Vector{};

			backtrackRecords.clear();
		}

		void reset()
		{
			clear();
			oldlayers = {};
			layers = {};
			chokedPackets = 0;
		}
	};
	Players getPlayer(int index) noexcept;
	const std::deque<Players::Record>* getBacktrackRecords(int index) noexcept;
}