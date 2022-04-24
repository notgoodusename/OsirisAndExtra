#pragma once

#include <deque>
#include <string>
#include <vector>

#include "imgui/imgui.h"

class GameEvent;

namespace Logger
{
	/*
	Options
	Log on console and on event log
	Color + decayTime

	Damage dealt (done)
	Damage received (done)

	Spread misses (todo)
	Misc (lagcomp, occlusion) misses (todo)
	Resolver misses (todo)
	
	Hostage taken (done)
	Bomb plants (done)
	*/


	struct Log
	{
		float time{ 0.0f };
		float alpha{ 255.0f };
		std::string text{ "" };
	};

	void reset() noexcept;

	void addLog(std::string log) noexcept;
	void getEvent(GameEvent* event) noexcept;
	void process(ImDrawList* drawList) noexcept;
	void console() noexcept;
	void render(ImDrawList* drawList) noexcept;

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
	static std::deque<Log> renderLogs;
}