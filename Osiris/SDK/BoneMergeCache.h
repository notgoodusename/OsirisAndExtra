#pragma once
#include "Entity.h"
#include "matrix3x4.h"
#include "CStudioHdr.h"

class BoneMergeCache
{
public:

	BoneMergeCache() noexcept;

	void init(Entity* owner) noexcept;

	// Updates the lookups that let it merge bones quickly.
	void updateCache() noexcept;

	// This copies the transform from all bones in the followed entity that have 
	// names that match our bones.
	void mergeMatchingBones(int boneMask) noexcept;

	void mergeMatchingPoseParams() noexcept;

	void copyFromFollow(const Vector followPos[], const Quaternion followQ[], int boneMask, Vector myPos[], Quaternion myQ[]) noexcept;
	void copyToFollow(const Vector myPos[], const Quaternion myQ[], int boneMask, Vector followPos[], Quaternion followQ[]) noexcept;
	bool isCopied() noexcept;

	// Returns true if the specified bone is one that gets merged in MergeMatchingBones.
	//int isBoneMerged(int bone) const noexcept;

	// Gets the origin for the first merge bone on the parent.
	bool getAimEntOrigin(Vector* absOrigin, Vector* absAngles) noexcept;
	bool getRootBone(matrix3x4& rootBone) noexcept;

	void ForceCacheClear() noexcept { forceCacheClear = true; updateCache(); }

	const unsigned short* getRawIndexMapping() noexcept { return &rawIndexMapping[0]; }

protected:

	// This is the entity that we're keeping the cache updated for.
	Entity* owner;

	// All the cache data is based off these. When they change, the cache data is regenerated.
	// These are either all valid pointers or all NULL.
	Entity* follow;
	CStudioHdr* followHdr;
	const StudioHdr* followRenderHdr;
	CStudioHdr* ownerHdr;
	const StudioHdr* ownerRenderHdr;

	// keeps track if this entity is part of a reverse bonemerge
	int copiedFramecount;

	// This is the mask we need to use to set up bones on the followed entity to do the bone merge
	int followBoneSetupMask;

	// Cache data.
	class MergedBone
	{
	public:
		unsigned short myBone;		// index of MergeCache's owner's bone
		unsigned short parentBone;	// index of m_pFollow's matching bone
	};

	// This is an array of pose param indices on the follower to pose param indices on the owner
	int ownerToFollowPoseParamMapping[MAXSTUDIOPOSEPARAM];

	UtlVector<MergedBone> mergedBones;
	PAD(12)

	unsigned short rawIndexMapping[MAXSTUDIOBONES];
	bool forceCacheClear;
};