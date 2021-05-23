#pragma once

#include <array>
#include <deque>

#include "../ConfigStructs.h"

#include "../SDK/NetworkChannel.h"
#include "../SDK/matrix3x4.h"
#include "../SDK/Vector.h"

enum class FrameStage;
struct UserCmd;

#define OSIRIS_BACKTRACK() true

namespace Backtrack
{
    void update(FrameStage) noexcept;
    void run(UserCmd*) noexcept;

    void addLatencyToNetwork(NetworkChannel*, float) noexcept;
    void updateIncomingSequences() noexcept;

    float getLerp() noexcept;

    struct Record {
        Vector origin;
        Vector mins;
        Vector maxs;
        float simulationTime;
        matrix3x4 matrix[256];
    };

    struct incomingSequence {
        int inreliablestate;
        int sequencenr;
        float servertime;
    };

    const std::deque<Record>* getRecords(std::size_t index) noexcept;
    bool valid(float simtime) noexcept;
    void init() noexcept;
    float getMaxUnlag() noexcept;
}
