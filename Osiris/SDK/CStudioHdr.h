#pragma once
#include "Pad.h"
//Note: this isnt the same as StudioHdr, there are 2 StudioHdr classes, CStudioHdr and studiohdr_t

struct StudioSeqdesc
{
	PAD(196);
	int numAnimTags;
};

struct CStudioHdr
{
	StudioSeqdesc seqdesc(int sequence) noexcept
	{
		return *reinterpret_cast<StudioSeqdesc*>(memory->seqdesc(this, sequence));
	}
};