#include "CBoneSetup.h"

CBoneSetup::CBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters) noexcept
    : studioHdr(studioHdr)
    , boneMask(boneMask)
    , poseParameter(poseParameters)
    , poseDebugger(nullptr)
{
}

void CBoneSetup::initPose(Vector pos[], Quaternion q[]) noexcept
{
    auto hdr = studioHdr->hdr;

    for (int i = 0; i < hdr->numBones; i++) {
        auto bone = hdr->getBone(i);

        if (bone->flags & boneMask) {
            pos[i] = bone->pos;
            q[i] = bone->quat;
        }
    }
}

void CBoneSetup::accumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float weight, float time, void* IKContext) noexcept
{
    memory->boneSetupAccumulatePose(this, pos, q, sequence, cycle, weight, time, IKContext);
}

void CBoneSetup::calcAutoplaySequences(Vector pos[], Quaternion q[], float realTime, void* IKContext) noexcept
{
    auto address = memory->boneSetupCalcAutoplaySequences;

    // Thanks clang!
    __asm
    {
        mov ecx, this
        movss xmm0, realTime
        push IKContext
        push q
        push pos
        call address
    }
}

void CBoneSetup::calcBoneAdj(Vector pos[], Quaternion q[], const float controllers[]) noexcept
{
    auto address = memory->boneSetupCalcBoneAdj;

    // Thanks clang!
    __asm
    {
        mov ecx, studioHdr
        mov edx, this
        push boneMask
        push controllers
        push q
        push pos
        call address
    }
}