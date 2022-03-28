#pragma once

#include "VirtualMethod.h"

struct ServerClass
{
public:
	const char* networkName;
	void** table;
	ServerClass* next;
	int classID;
	int instanceBaselineIndex;
};

class Server
{
public:
	VIRTUAL_METHOD(ServerClass*, getAllServerClasses, 10, (), (this))
};
