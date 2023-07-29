#include "BoneMergeCache.h"

BoneMergeCache::BoneMergeCache() noexcept
{
	init(NULL);
}

#pragma optimize( "", off)
#pragma runtime_checks("", off) //Disable runtime checks to prevent ESP error
void BoneMergeCache::init(Entity* owner) noexcept
{
	memory->boneMergeCacheInit(this, owner);
}
#pragma runtime_checks("", restore) //Restore runtime checks
#pragma optimize( "", on )

void BoneMergeCache::updateCache() noexcept
{
	memory->boneMergeCacheUpdateCache(this);
}

void BoneMergeCache::mergeMatchingBones(int boneMask) noexcept
{
	memory->boneMergeCacheMergeMatchingBones(this, boneMask);
}

void BoneMergeCache::mergeMatchingPoseParams() noexcept
{
	memory->boneMergeCacheMergeMatchingPoseParams(this);
}

void BoneMergeCache::copyFromFollow(const Vector followPos[], const Quaternion followQ[], int boneMask, Vector myPos[], Quaternion myQ[]) noexcept
{
	updateCache();

	// If this is set, then all the other cache data is set.
	if (!ownerHdr || mergedBones.size == 0)
		return;

	// Now copy the bone matrices.
	for (int i = 0; i < mergedBones.size; i++)
	{
		int ownerBone = mergedBones[i].myBone;
		int parentBone = mergedBones[i].parentBone;

		// Only update bones reference by the bone mask.
		if (!(ownerHdr->boneFlags[ownerBone] & boneMask))
			continue;

		myPos[ownerBone] = followPos[parentBone];
		myQ[ownerBone] = followQ[parentBone];
	}
}

void BoneMergeCache::copyToFollow(const Vector myPos[], const Quaternion myQ[], int boneMask, Vector followPos[], Quaternion followQ[]) noexcept
{
	updateCache();

	// If this is set, then all the other cache data is set.
	if (!ownerHdr || mergedBones.size == 0)
		return;

	// Now copy the bone matrices.
	for (int i = 0; i < mergedBones.size; i++)
	{
		int ownerBone = mergedBones[i].myBone;
		int parentBone = mergedBones[i].parentBone;

		// Only update bones reference by the bone mask.
		if (!(ownerHdr->boneFlags[ownerBone] & boneMask))
			continue;

		followPos[parentBone] = myPos[ownerBone];
		followQ[parentBone] = myQ[ownerBone];
	}
	copiedFramecount = memory->globalVars->framecount;

}

bool BoneMergeCache::isCopied() noexcept
{
	return copiedFramecount == memory->globalVars->framecount;
}

bool BoneMergeCache::getAimEntOrigin(Vector* absOrigin, Vector* absAngles) noexcept
{
	return memory->boneMergeCacheGetAimEntOrigin(this, absOrigin, absAngles);
}

bool BoneMergeCache::getRootBone(matrix3x4& rootBone) noexcept
{
	return memory->boneMergeCacheGetRootBone(this, rootBone);
}