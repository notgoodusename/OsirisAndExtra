#pragma once

#include "Pad.h"
#include "Vector.h"

class Input {
public:
    VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
    PAD(12)
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    PAD(178)
    bool isCameraInThirdPerson;
    PAD(1)
    Vector cameraOffset;
};