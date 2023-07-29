#pragma once
#include "Entity.h"
#include "matrix3x4.h"

class IKContext
{
	// Not sure of the correct size, also don't care
	PAD(4208)

public:

	IKContext() noexcept;
	~IKContext() noexcept;

	void init(const CStudioHdr* hdr, const Vector& localangles, const Vector& localOrigin, float currentTime, int frameCount, int boneMask) noexcept;
	void updateTargets(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept;
	void solveDependencies(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept;
	void clearTargets() noexcept;
	void addDependencies(StudioSeqdesc& seqdesc, int sequence, float cycle, const float poseParameters[], float weight) noexcept;
	void copyTo(IKContext* other, const unsigned short* remapping) noexcept;
};