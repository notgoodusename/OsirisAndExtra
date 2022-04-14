#pragma once
#include <thread>
#include <chrono>

#include "Memory.h"

class Thread
{
public:
	Thread() { }

	/*
	The function must be __cdecl otherwise it could crash or return garbage
	Example of creating a thread
		threadObject.createThread<Arguments>(&function, arguments);
	Example of usage:
		Thread funtionTest;
		funtionTest.createThread<char*>(&function, (char*)"b");
		funtionTest.releaseThread();
	*/

	template<typename ...Args>
	void createThread(void* function, Args... args)
	{
		currentThread = memory->createSimpleThread(function, args..., 0U);
	}
	
	void releaseThread()
	{
		if (currentThread)
			memory->releaseThreadHandle(currentThread);
	}

private:
	void* currentThread = nullptr;
};