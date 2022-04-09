#pragma once

#include <deque>
#include <string>
#include <vector>

class GameEvent;

namespace Logger
{
	/*
	Options
	Log on console and on developer 1 mode
	Color + decayTime

	Damage dealt (done)
	Damage received (done)

	Spread misses (todo)
	Misc (lagcomp, occlusion) misses (todo)
	Resolver misses (todo)
	
	Hostage taken (needs testing)
	Bomb plants (needs testing)
	*/


	struct Log
	{
		float time{ 0.0f };
		std::string text{ "" };
	};

	void getEvent(GameEvent* event) noexcept;
	void process() noexcept;
	void render() noexcept;

	enum
	{
		Console,
		EventLog
	};

	enum
	{
		DamageDealt,
		DamageReceived,
		HostageTaken,
		BombPlants
	};

	static std::deque<Log> logs;
}