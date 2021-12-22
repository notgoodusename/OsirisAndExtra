#pragma once

#include "VirtualMethod.h"

class Entity;
class MoveData;

class GameMovement {
public:
    VIRTUAL_METHOD_V(void, processMovement, 1, (Entity* localPlayer, MoveData* moveData), (this, localPlayer, moveData))
    VIRTUAL_METHOD(Vector&, getPlayerViewOffset, 8, (bool ducked), (this, ducked))

	PAD(4)
	Entity* player;
	MoveData* moveData;
	int oldWaterLevel;
	float waterEntryTime;
	int onLadder;
	Vector vecForward;
	Vector vecRight;
	Vector vecUp;
	int cachedGetPointContents[64][3];
	Vector cachedGetPointContentsPoint[64][3];
	int speedCropped;
	bool processingMovement;
	bool inStuckTest;
	float stuckCheckTime[64 + 1][2];
	void* traceListData;
	int traceCount;
};
