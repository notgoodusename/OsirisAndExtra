#pragma once
#include "Entity.h"
#include "CStudioHdr.h"
#include "matrix3x4.h"

struct CBoneSetup {
    CBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters) noexcept;

    void initPose(Vector pos[], Quaternion q[]) noexcept;
    void accumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float weight, float time, void* IKContext) noexcept;
    void calcAutoplaySequences(Vector pos[], Quaternion q[], float realTime, void* IKContext) noexcept;
    void calcBoneAdj(Vector pos[], Quaternion q[], const float controllers[]) noexcept;

    const CStudioHdr* studioHdr;
    int boneMask;
    float* poseParameter;
    void* poseDebugger;
};

struct IBoneSetup {
    CBoneSetup* bone_setup;
};