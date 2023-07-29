#pragma once

#include "Pad.h"
#include "ModelInfo.h"

//Note: this isnt the same as StudioHdr, there are 2 StudioHdr classes, CStudioHdr and studiohdr_t

struct StudioSeqdesc
{

	inline char* const label() const noexcept { return ((char*)this) + labelIndex; }
	inline char* const activityName() const noexcept { return ((char*)this) + activityNameIndex; }

	void* basePtr;

	int labelIndex;

	int activityNameIndex;

	int flags;

	int activity;
	int actWeight;

	int numEvents;
	int eventIndex;

	Vector bbMin;
	Vector bbMax;

	int numBlends;

	int animIndexIndex; 

	int movementIndex;
	int groupSize[2];
	int paramIndex[2];
	float paramStart[2];
	float paramEnd[2];
	int paramParent;

	float fadeInTime;
	float fadeOutTime;

	int localEntryNode;
	int localExitNode;
	int nodeFlags;

	float entryPhase;
	float exitPhase;

	float lastFrame;

	int nextSeq;
	int pose;

	int numIkRules;

	int numAutoLayers;
	int autoLayerIndex;

	int weightListIndex;

	int poseKeyIndex;

	int numIkLocks;
	int ikLockIndex;


	int keyValueIndex;
	int keyValueSize;

	int cyclePoseIndex;

	int activityModifierIndex;
	int numActivityModifiers;

	int animTagIndex;
	int numAnimTags;

	int rootDriverIndex;
	PAD(8)
};

struct CStudioHdr
{
	StudioHdr* hdr;
	void* virtualModel;
	void* softBody;
	UtlVector<const StudioHdr*> hdrCache;
	int	numFrameUnlockCounter;
	int* frameUnlockCounter;
	PAD(8);
	UtlVector<int> boneFlags;
	UtlVector<int> boneParent;
	void* activityToSequence;


	const char* getSequenceName(int sequence) noexcept
	{
		if (!this || sequence < 0 || sequence >= getNumSeq())
			return "Unknown";
		return seqdesc(sequence).label();
	}

	int lookupSequence(const char* name) noexcept
	{
		return memory->CStudioHdrLookupSequence(this, name);
	}

	StudioSeqdesc seqdesc(int sequence) noexcept
	{
		return *reinterpret_cast<StudioSeqdesc*>(memory->seqdesc(this, sequence));
	}

	int getNumSeq() noexcept
	{
		return memory->getNumSeq(this);
	}
};