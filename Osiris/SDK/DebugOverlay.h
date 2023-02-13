#pragma once

#include "VirtualMethod.h"
#include "Vector.h"

//Virtual method doesnt work here for some reason
class DebugOverlay
{
public:
	virtual void entityTextOverlay(int entityIndex, int lineOffset, float duration, int r, int g, int b, int a, const char* format, ...) = 0;
	virtual void boxOverlay(const Vector& origin, const Vector& mins, const Vector& max, Vector const& angles, int r, int g, int b, int a, float duration) = 0;
	virtual void sphereOverlay(const Vector& origin, float radius, int theta, int phi, int r, int g, int b, int a, float duration) = 0;
	virtual void triangleOverlay(const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
	virtual void lineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration) = 0;
	virtual void textOverlay(const Vector& origin, float duration, const char* format, ...) = 0;
	virtual void textOverlay(const Vector& origin, int lineOffset, float duration, const char* format, ...) = 0;
	virtual void screenTextOverlay(float posX, float PosY, float duration, int r, int g, int b, int a, const char* text) = 0;
	virtual void sweptBoxOverlay(const Vector& start, const Vector& end, const Vector& mins, const Vector& max, const Vector& angles, int r, int g, int b, int a, float duration) = 0;
	virtual void gridOverlay(const Vector& origin) = 0;
	virtual void coordFrameOverlay(const matrix3x4& frame, float scale, int colorTable[3][3] = NULL) = 0;
	virtual int screenPosition(const Vector& point, Vector& screen) = 0;
	virtual int screenPosition(float posX, float posY, Vector& screen) = 0;
	virtual void* getFirst(void) = 0;
	virtual void* getNext(void* current) = 0;
	virtual void clearDeadOverlays(void) = 0;
	virtual void clearAllOverlays() = 0;
	virtual void textOverlayRGB(const Vector& origin, int lineOffset, float duration, float r, float g, float b, float alpha, const char* format, ...) = 0;
	virtual void textOverlayRGB(const Vector& origin, int lineOffset, float duration, int r, int g, int b, int a, const char* format, ...) = 0;
	virtual void lineOverlayAlpha(const Vector& origin, const Vector& dest, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
	virtual void boxOverlay2(const Vector& origin, const Vector& mins, const Vector& max, Vector const& orientation, const Vector faceColor, const Vector edgeColor, float duration) = 0;
	virtual void addLineOverlay(const Vector& origin, const Vector& unk, int r, int g, int b, int a, float duration, float unk1) = 0;
	virtual void purgeTextOverlays() = 0;
	virtual void capsuleOverlay(const Vector& start, const Vector& end, const float& radius, int r, int g, int b, int a, float duration, int unk, int occlude) = 0;
};