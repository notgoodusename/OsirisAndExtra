#pragma once

#include "UserCmd.h"
#include "Pad.h"
#include "Vector.h"
#include "VirtualMethod.h"

class Input {
public:
    PAD(12)
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    PAD(158)
    bool isCameraInThirdPerson;
    bool cameraMovingWithMouse;
    Vector cameraOffset;

    VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
};
