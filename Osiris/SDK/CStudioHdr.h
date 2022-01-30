#pragma once

#include "Pad.h"
#include "ModelInfo.h"

//Note: this isnt the same as StudioHdr, there are 2 StudioHdr classes, CStudioHdr and studiohdr_t

struct StudioSeqdesc
{
	PAD(104); //0
	float fadeInTime; //104
	float fadeOutTime; //108
	PAD(84); //112
	int numAnimTags; // 196
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

	StudioSeqdesc seqdesc(int sequence) noexcept
	{
		return *reinterpret_cast<StudioSeqdesc*>(memory->seqdesc(this, sequence));
	}
};