#include "BoneSetup.h"
#include "BoneMergeCache.h"
#include "CBoneSetup.h"

#define BONE_USED_BY_BONE_MERGE		0x00040000

void BoneSetup::build(Entity* entity, matrix3x4* boneToWorld, int maxBones, int boneMask) noexcept
{
    interfaces->mdlCache->beginLock();
    {
        auto studioHdr = entity->getModelPtr();

        if (studioHdr) {
            alignas(16) Vector pos[MAXSTUDIOBONES];
            alignas(16) Quaternion q[MAXSTUDIOBONES];

            // Allocate our own
            IKContext* ik = new IKContext;

            // Backup game ptr and set game to ours
            IKContext* backup = entity->IK();
            entity->IK() = ik;

            uint8_t boneComputed[32]{};
            RtlSecureZeroMemory(&boneComputed, sizeof boneComputed);

            ik->init(studioHdr, entity->getAbsAngle(), entity->getAbsOrigin(), entity->simulationTime(), memory->globalVars->tickCount, boneMask);
            getSkeleton(entity, studioHdr, pos, q, boneMask, ik);

            ik->updateTargets(pos, q, boneToWorld, &boneComputed);
            entity->calculateIKLocks(entity->simulationTime()); // Need to set game's ik ptr for this
            ik->solveDependencies(pos, q, boneToWorld, &boneComputed);

            // reset and cleanup
            entity->IK() = backup;
            delete ik;

            // TODO: phage - Implement MoveParent handling
            // Useless in csgo, never seen a single instance of MoveParents
            buildMatrices(entity, studioHdr, pos, q, entity->getBoneCache().memory, boneMask);
        }

        if (boneToWorld && maxBones >= entity->getBoneCache().size) {
            memcpy(boneToWorld, entity->getBoneCache().memory, sizeof(matrix3x4) * entity->getBoneCache().size);
        }
    }
    interfaces->mdlCache->endLock();
}

// CBaseAnimatingOverlay::GetSkeleton
void BoneSetup::getSkeleton(Entity* entity, CStudioHdr* studioHdr, Vector* pos, Quaternion* q, int boneMask, IKContext* ik) noexcept
{
    CBoneSetup boneSetup(studioHdr, boneMask, entity->poseParameters().data());
    boneSetup.initPose(pos, q);
    boneSetup.accumulatePose(pos, q, entity->sequence(), entity->cycle(), 1.f, entity->simulationTime(), ik);

    // sort the layers
    constexpr int MAX_LAYER_COUNT = 15;
    int layer[MAX_LAYER_COUNT]{};
    for (int i = 0; i < entity->getAnimationLayersCount(); i++) {
        layer[i] = MAX_LAYER_COUNT;
    }

    for (int i = 0; i < entity->getAnimationLayersCount(); i++) {
        auto layer_ = &entity->animOverlays()[i];
        if ((layer_->weight > 0.f) && layer_->sequence != -1 && layer_->order >= 0u && static_cast<int>(layer_->order) < entity->getAnimationLayersCount()) {
            layer[layer_->order] = i;
        }
    }

    Entity* weapon = nullptr;
    Entity* weaponWorldModel = nullptr;
    BoneMergeCache* boneMergeCache = nullptr;
    BoneMergeCache* boneMergeBackup = nullptr;

    bool doWeaponSetup = false;

    if (entity->getClientClass()->classId == ClassId::CSPlayer) {
        weapon = entity->getActiveWeapon();
        if (weapon) {
            weaponWorldModel = interfaces->entityList->getEntityFromHandle(weapon->weaponWorldModel());

            if (weaponWorldModel && weaponWorldModel->getModelPtr()) {
                boneMergeBackup = weaponWorldModel->boneMergeCache();

                boneMergeCache = new BoneMergeCache;
                weaponWorldModel->boneMergeCache() = boneMergeCache;
                weaponWorldModel->boneMergeCache()->init(weaponWorldModel);

                if (weaponWorldModel->boneMergeCache())
                    doWeaponSetup = true;
            }
        }
    }

    if (doWeaponSetup) {
        CStudioHdr* weaponStudioHdr = weaponWorldModel->getModelPtr();

        weaponWorldModel->boneMergeCache()->mergeMatchingPoseParams();

        IKContext* weaponIK = new IKContext;
        weaponIK->init(weaponStudioHdr, entity->getAbsAngle(), entity->getAbsOrigin(), entity->simulationTime(), 0, BONE_USED_BY_BONE_MERGE);

        CBoneSetup weaponSetup(weaponStudioHdr, BONE_USED_BY_BONE_MERGE, weaponWorldModel->poseParameters().data());
        Vector weaponPos[256];
        Quaternion weaponQ[256];

        weaponSetup.initPose(weaponPos, weaponQ);

        for (int i = 0; i < entity->getAnimationLayersCount(); i++) {
            AnimationLayer* layer = &entity->animOverlays()[i];
            if (layer->sequence <= 1 || layer->weight <= 0.f)
                continue;

            // TODO: phage - This should be moved to the end of animation update,
            // will break shit if we happen to call setupbones more than once per
            // tick on an entity
            // Alternatively, we ensure that the above condition never happens
            updateDispatchLayer(layer, weaponStudioHdr);

            if (layer->dispatchedDst <= 0 || layer->dispatchedDst >= studioHdr->getNumSeq()) {
                boneSetup.accumulatePose(pos, q, layer->sequence, layer->cycle, layer->weight, entity->simulationTime(), ik);
            }
            else {
                weaponWorldModel->boneMergeCache()->copyFromFollow(pos, q, BONE_USED_BY_BONE_MERGE, weaponPos, weaponQ);

                auto seqdesc = studioHdr->seqdesc(layer->sequence);
                ik->addDependencies(seqdesc, layer->sequence, layer->cycle, entity->poseParameters().data(), layer->weight);

                weaponSetup.accumulatePose(weaponPos, weaponQ, layer->dispatchedDst, layer->cycle, layer->weight, entity->simulationTime(), weaponIK);

                weaponWorldModel->boneMergeCache()->copyToFollow(weaponPos, weaponQ, BONE_USED_BY_BONE_MERGE, pos, q);

                weaponIK->copyTo(ik, weaponWorldModel->boneMergeCache()->getRawIndexMapping());
            }
        }

        delete weaponIK;
    }
    else {
        for (int i = 0; i < entity->getAnimationLayersCount(); i++) {
            if (layer[i] >= 0 && layer[i] < entity->getAnimationLayersCount()) {
                AnimationLayer layer_ = entity->animOverlays()[layer[i]];
                boneSetup.accumulatePose(pos, q, layer_.sequence, layer_.cycle, layer_.weight, entity->simulationTime(), ik);
            }
        }
    }

    IKContext* autoIk = new IKContext;
    autoIk->init(studioHdr, entity->getAbsAngle(), entity->getAbsOrigin(), entity->simulationTime(), 0, boneMask);
    boneSetup.calcAutoplaySequences(pos, q, entity->simulationTime(), autoIk);
    delete autoIk;

    if (studioHdr->hdr->numBoneControllers > 0) {
        boneSetup.calcBoneAdj(pos, q, entity->encodedController().data());
    }

    if (weaponWorldModel && boneMergeCache) {
        weaponWorldModel->boneMergeCache() = boneMergeBackup;
        delete boneMergeCache;
    }
}

// Left commented out, you can choose to rebuild this or sig the client's, they're identical 
void BoneSetup::updateDispatchLayer(AnimationLayer* layer, CStudioHdr* studioHdr) noexcept
{
    if (!studioHdr || !layer)
    {
        if (layer)
        {
            layer->dispatchedDst = ACT_INVALID;
        }
        return;
    }

    if (layer->dispatchedStudioHdr != studioHdr || layer->dispatchedSrc != layer->sequence || layer->dispatchedDst >= studioHdr->getNumSeq())
    {
        layer->dispatchedStudioHdr = studioHdr;
        layer->dispatchedSrc = layer->sequence;

        const char* layerName = studioHdr->getSequenceName(layer->sequence);
        layer->dispatchedDst = studioHdr->lookupSequence(layerName);
    }
}

void BoneSetup::buildMatrices(Entity* entity, CStudioHdr* studioHdr, Vector* pos, Quaternion* q, matrix3x4* boneToWorld, int boneMask) noexcept
{
    auto angleMatrix = [](const Vector& angles, const Vector& position, matrix3x4& matrix)
    {
        float sr, sp, sy, cr, cp, cy;

        Helpers::sinCos(Helpers::deg2rad(angles[1]), &sy, &cy);
        Helpers::sinCos(Helpers::deg2rad(angles[0]), &sp, &cp);
        Helpers::sinCos(Helpers::deg2rad(angles[2]), &sr, &cr);

        matrix[0][0] = cp * cy;
        matrix[1][0] = cp * sy;
        matrix[2][0] = -sp;

        float crcy = cr * cy;
        float crsy = cr * sy;
        float srcy = sr * cy;
        float srsy = sr * sy;
        matrix[0][1] = sp * srcy - crsy;
        matrix[1][1] = sp * srsy + crcy;
        matrix[2][1] = sr * cp;

        matrix[0][2] = (sp * crcy + srsy);
        matrix[1][2] = (sp * crsy - srcy);
        matrix[2][2] = cr * cp;

        matrix[0][3] = position.x;
        matrix[1][3] = position.y;
        matrix[2][3] = position.z;
    };

    auto quaternionMatrix = [](const Quaternion& q, const Vector& pos, matrix3x4& matrix)
    {
        matrix[0][0] = 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z;
        matrix[1][0] = 2.0f * q.x * q.y + 2.0f * q.w * q.z;
        matrix[2][0] = 2.0f * q.x * q.z - 2.0f * q.w * q.y;

        matrix[0][1] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
        matrix[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
        matrix[2][1] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;

        matrix[0][2] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
        matrix[1][2] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
        matrix[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;

        matrix[0][3] = pos.x;
        matrix[1][3] = pos.y;
        matrix[2][3] = pos.z;
    };

    int i = 0, j = 0;
    int chain[MAXSTUDIOBONES] = {};
    int chainLength = studioHdr->hdr->numBones;

    for (i = 0; i < studioHdr->hdr->numBones; i++) {
        chain[chainLength - i - 1] = i;
    }

    matrix3x4 boneMatrix = {};
    matrix3x4 rotationMatrix = {};
    angleMatrix(entity->getAbsAngle(), entity->getAbsOrigin(), rotationMatrix);

    for (j = chainLength - 1; j >= 0; j--) {
        i = chain[j];

        auto bone = studioHdr->boneFlags[i];
        if (bone & boneMask) {
            quaternionMatrix(q[i], pos[i], boneMatrix);

            int boneParent = studioHdr->boneParent[i];
            if (boneParent == -1) {
                concatTransforms(rotationMatrix, boneMatrix, boneToWorld[i]);
            }
            else {
                concatTransforms(boneToWorld[boneParent], boneMatrix, boneToWorld[i]);
            }
        }
    }
}

void BoneSetup::concatTransforms(const matrix3x4& m0, const matrix3x4& m1, matrix3x4& out) noexcept
{
    for (int i = 0; i < 3; i++) {
        // Normally, you can't just multiply 2 3x4 matrices together, so translation is done separately
        out[i][3] = m1[0][3] * m0[i][0] + m1[1][3] * m0[i][1] + m1[2][3] * m0[i][2] + m0[i][3]; // translation

        for (int j = 0; j < 3; j++) // rotation/scale
        {
            out[i][j] = m0[i][0] * m1[0][j] + m0[i][1] * m1[1][j] + m0[i][2] * m1[2][j];
        }
    }
}