#pragma once

#include "VirtualMethod.h"

class NetworkMessage
{
public:
    VIRTUAL_METHOD(int, getType, 7, (), (this))
};
