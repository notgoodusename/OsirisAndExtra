#pragma once

#include "Vector.h"
#include "Pad.h"

struct ViewSetup {
    PAD(172);
    void* csm;
    float fov;
    PAD(4);
    Vector origin;
    Vector angles;
    PAD(4);
    float farZ;
    PAD(8)
    float aspectRatio;
};
