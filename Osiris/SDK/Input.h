#pragma once

#include "UserCmd.h"
#include "Pad.h"
#include "Vector.h"

class Input {
public:
    PAD(12)
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    PAD(154)
    bool isCameraInThirdPerson;
    PAD(2)
    Vector cameraOffset;
    PAD(56)
    UserCmd* commands;
    VerifiedUserCmd* verifiedCommands;

    VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))

    VerifiedUserCmd* getVerifiedUserCmd(int sequenceNumber) noexcept
    {
        return &verifiedCommands[sequenceNumber % 150];
    }
};