#include "IKContext.h"
#include "MemAlloc.h"

#pragma optimize( "", off)
#pragma runtime_checks("", off) //Disable runtime checks to prevent ESP error
IKContext::IKContext() noexcept
{
    memory->ikContextConstruct(this);
}

IKContext::~IKContext() noexcept
{
    memory->ikContextDeconstructor(this);
}
#pragma runtime_checks("", restore) //Restore runtime checks
#pragma optimize( "", on )

void IKContext::init(const CStudioHdr* hdr, const Vector& localAngles, const Vector& localOrigin, float currentTime, int frameCount, int boneMask) noexcept
{
    memory->ikContextInit(this, hdr, localAngles, localOrigin, currentTime, frameCount, boneMask);
}

void IKContext::updateTargets(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept
{
    memory->ikContextUpdateTargets(this, pos, q, boneCache, computed);
}

void IKContext::solveDependencies(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept
{
    memory->ikContextSolveDependencies(this, pos, q, boneCache, computed);
}

struct IKTarget {
    int frameCount;
};

void IKContext::clearTargets() noexcept
{
    const int targetCount = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0xFF0);
    auto IkTarget = reinterpret_cast<IKTarget*>(reinterpret_cast<uintptr_t>(this) + 0xD0);
    for (int i = 0; i < targetCount; i++) {
        IkTarget->frameCount = -9999;
        IkTarget++;
    }
}

void IKContext::addDependencies(StudioSeqdesc& seqdesc, int sequence, float cycle, const float poseParameters[], float weight) noexcept
{
    memory->ikContextAddDependencies(this, seqdesc, sequence, cycle, poseParameters, weight);
}

void IKContext::copyTo(IKContext* other, const unsigned short* remapping) noexcept
{
    memory->ikContextCopyTo(this, other, remapping);
}