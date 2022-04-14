#pragma once
#include <thread>
#include <chrono>

#include "Memory.h"

/*
The function must be __cdecl otherwise it could crash or return garbage
Example of creating a thread
	threadObject.createThread<Arguments>(&function, arguments);
Example of usage:
	Thread funtionTest;
	funtionTest.createThread<char*>(&function, (char*)"b");
	funtionTest.releaseThread();
*/

class Thread
{
public:
	Thread() noexcept { }

	template<typename ...Args>
	void createThread(void* function, Args... args) noexcept
	{
		currentThread = memory->createSimpleThread(function, args..., 0U);
	}
	
	void releaseThread() noexcept
	{
		if (currentThread)
			memory->releaseThreadHandle(currentThread);
	}

	void* getThreadHandle() noexcept
	{
		return currentThread;
	}

private:
	void* currentThread = nullptr;
};