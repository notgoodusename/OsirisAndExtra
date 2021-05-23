#pragma once

#include "VirtualMethod.h"

class NetworkChannel {
public:
    VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))

    std::byte pad[24];
    int outSequenceNr;
    int inSequenceNr;
    int outSequenceNrAck;
    int outReliableState;
    int inReliableState;
    int chokedPackets;
};
