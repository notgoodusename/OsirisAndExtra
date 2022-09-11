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

	void reset() noexcept;

	void update(UserCmd*, bool& sendPacket) noexcept;

	void renderStart(FrameStage) noexcept;

	void fake() noexcept;

	void packetStart() noexcept;
	void postDataUpdate() noexcept;

	void saveCorrectAngle(int entityIndex, Vector correctAngle) noexcept;

	int& buildTransformationsIndex() noexcept;

	Vector* getCorrectAngle() noexcept;
	Vector* getViewAngles() noexcept;
	Vector* getLocalAngle() noexcept;

	void handlePlayers(FrameStage) noexcept;

	bool isLocalUpdating() noexcept;
	bool isEntityUpdating() noexcept;
	bool isFakeUpdating() noexcept;

	bool gotFakeMatrix() noexcept;
	std::array<matrix3x4, MAXSTUDIOBONES> getFakeMatrix() noexcept;

	bool gotFakelagMatrix() noexcept;
	std::array<matrix3x4, MAXSTUDIOBONES> getFakelagMatrix() noexcept;

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
			std::deque<Vector> positions;
			Vector origin;
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
		Vector origin{}, oldOrigin{}, absAngle{};
		Vector velocity{}, oldVelocity{};

		float spawnTime{ 0.f };

		float simulationTime{ -1.0f };
		int chokedPackets{ 0 };
		int misses{ 0 };
		bool gotMatrix{ false };

		float moveWeight{ 0.0f };

		float duckAmount{ 0.0f };
		float oldDuckAmount{ 0.0f };

		int flags{ 0 };

		void clear()
		{
			gotMatrix = false;
			simulationTime = -1.0f;
			moveWeight = 0.0f;

			duckAmount = 0.0f;
			oldDuckAmount = 0.0f;

			flags = 0;

			origin = Vector{};
			oldOrigin = Vector{};
			absAngle = Vector{};
			velocity = Vector{};
			oldVelocity = Vector{};
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
			misses = 0;
		}
	};
	Players getPlayer(int index) noexcept;
	Players* setPlayer(int index) noexcept;
	std::array<Animations::Players, 65> getPlayers() noexcept;
	std::array<Animations::Players, 65>* setPlayers() noexcept;
	const std::deque<Players::Record>* getBacktrackRecords(int index) noexcept;
}