#pragma once

#include "VirtualMethod.h"
#include "BitBuffer.h"

struct ClientClass;

class Client {
public:
    VIRTUAL_METHOD(ClientClass*, getAllClasses, 8, (), (this))
    VIRTUAL_METHOD(bool, writeUsercmdDeltaToBuffer, 24, (int slot, bufferWrite* buf, int from, int to, bool isnewcommand), (this, slot, buf, from, to, isnewcommand))
    VIRTUAL_METHOD(bool, dispatchUserMessage, 38, (int messageType, int arg, int arg1, void* data), (this, messageType, arg, arg1, data))
};
