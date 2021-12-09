#pragma once

#include "VirtualMethod.h"

struct ConVar;

class Cvar {
public:
    VIRTUAL_METHOD(ConVar*, findVar, 15, (const char* name), (this, name))
};

class conCommandBase
{
public:
	void* vmt;
	conCommandBase* next;
	bool registered;
	const char* name;
	const char* helpString;
	int flags;
	conCommandBase* conCommandBases;
	void* accessor;
};