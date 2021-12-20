#pragma once

#include "VirtualMethod.h"
#include "Vector.h"

//Virtual method doesnt work here for some reason
class DebugOverlay
{
public:
	virtual void entityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char* format, ...) = 0;
	virtual void boxOverlay(const Vector& origin, const Vector& mins, const Vector& max, Vector const& orientation, int r, int g, int b, int a, float duration) = 0;
};