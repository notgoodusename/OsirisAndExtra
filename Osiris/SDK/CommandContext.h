#pragma once
#include "UserCmd.h"

class CommandContext
{
public:
	bool needsProcessing;
	UserCmd cmd;
	int commandNumber;
};