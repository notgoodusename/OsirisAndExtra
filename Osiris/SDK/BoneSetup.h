#pragma once
#include "Entity.h"
#include "IKContext.h"
#include "matrix3x4.h"

namespace BoneSetup {
    void build(Entity* entity, matrix3x4* boneToWorld, int maxBones, int boneMask) noexcept;
    void getSkeleton(Entity* entity, CStudioHdr* studioHdr, Vector* pos, Quaternion* q, int boneMask, IKContext* ik) noexcept;
    void updateDispatchLayer(AnimationLayer* layer, CStudioHdr* studioHdr) noexcept;
    void buildMatrices(Entity* entity, CStudioHdr* studioHdr, Vector* pos, Quaternion* q, matrix3x4* boneToWorld, int boneMask) noexcept;
    void concatTransforms(const matrix3x4& m0, const matrix3x4& m1, matrix3x4& out) noexcept;
};