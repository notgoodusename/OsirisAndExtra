#pragma once

struct UserCmd;

namespace Tickbase
{
	void shiftTicks(int, UserCmd*) noexcept;
	void run(UserCmd* cmd) noexcept;

	struct Tick
	{
		/*
		*SendMove clamp
			* SendMove sends a batch of moves, which is clamped to(1 << NUM_BACKUP_COMMAND_BITS(3)) - 1 = 1000(binary)-1 = 16 - 1 = 15
			* This limit is innecessary, to remove it we can rebuild sendMove, allowing us to fakelag a lot more ticks
			* Info https ://www.unknowncheats.me/forum/counterstrike-global-offensive/307478-real-fakelag.html
		*
			*How server adjusts tickbase
			* Each time it gets a command it process it and checks if it is equal to the current tick count
			* if not then it does
			*
			* ++ticksAllowedForProcessing(how many commands it can process, its limited to maxusrcmdticks)
			* if (simulation_ticks > 0)
			* {
			*NOTE: simulation_ticks = number_of_commands + dropped_packets(fakelag) which is then clampped to ticksAllowedForProcessing
				* AdjustTimePlayerBase(simulation_ticks);
			*
		}
		*If the tickcount is out of sync the command doesnt get executed
			* And when running the command it does
			* ticksAllowedForProcessing--
			* So in conclusion for every new command that is out of sync the tickbase gets adjustedand if the tickcount is out of sync the command doesnt get executed
			*
			* How to Tickbase shift
			* Our goal to tickbase shift is to make simulation_ticks be bigger, so that when we send a bunch of commands the tickbase shifts
			* So to tickbase shift just send a bunch of invalid / valid commands in which the simulation_ticks gets incremented but the command isnt executed
			* But the problem comes after that tickbase shift
			* Example
			* ticksAllowedForProcessing = 0  //Remember that simulation_ticks is clampped by ticksAllowedForProcessing
			* tickbase = 28
			* 16 invalid ticks
			* ticksAllowedForProcessing = 16
			* tickbase = 13 (28 - 16 + 1)
			* 16 invalid ticks
			* ticksAllowedForProcessing = 16
			* tickbase = 14  (29 - 16 + 1)
			* This happens because the tickbase is limited
			* tickbase = tickcount - simulation_ticks(this is with 1 player, with more than 1 it works differently) for more info look at
			* AdjustTimePlayerBase https ://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/server/player.cpp#L3079
		*To fix this we can let the server run the tickbase,
			* basically dont run any commandsand let the tickbase catch up(recharge method), which will make ticksAllowedForProcessing decrement
			* Reason this works, is because the server doesn't simulate players who are timing out with null packets, thus making the tickbase stuck,
			* although theres is a limit which is the anti airstuck method by valve and other hvh servers, but most community servers allows you to be airstuck forever
			* Example
			* tickbase = 30
			* ticksAllowedForProcessing = 0
			* 16 invalid ticks
			* ticksAllowedForProcessing = 16
			* tickbase = 15 (30 - 16 + 1) //Note: m_flSimulationTime is also affected by this
			* Dont send any commands for 16 ticks
			* Server runs 16 null commands which do
			*ticksAllowedForProcessing--
			* ticksAllowedForProcessing = 0
			* tickbase = 32
			* Now we can shift another 16 ticks
			*
			*Why Tickbase manipulation also works as backtrack
			* But im going to make it even simpler
			* Lagcomp uses curtime which is calculated like this
			* gpGlobals->curtime = player->m_nTickBase * TICK_INTERVAL;
		*Remember that tickbase = tickcount - simulation_ticks
			* We can already tell that if the tickbase gets pushed back then we will be backtracking
			* This UC post gives a better explanation about why Tickbase manipulation works like backtrack
			* https://www.unknowncheats.me/forum/2697004-post21.html
		*
			*Some related info https ://www.unknowncheats.me/forum/2760482-post19.html
		*Why tickbase manipulation works https ://www.unknowncheats.me/forum/counterstrike-global-offensive/331325-tickbase-manipulation.html
		*/
		int	maxUsercmdProcessticks{ 16 }; // Not networked >:C
		int chokedCommands{ 0 };
		int recharge{ 0 };
		int tickshift{ 0 };
		int lastTickShift{ 0 };
		int tickCount{ 0 };
		int tickBase{ 0 };
		int commandNumber{ 0 };
		float lastTime{ 0.f };
	};
	extern Tick tick;
}