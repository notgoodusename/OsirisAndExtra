#pragma once

#include "VirtualMethod.h"

class Entity;
class MoveData
{
public:
    bool firstRunOfFunctions;
    bool gameCodeMovedPlayer;
    int playerHandle;
    int impulseCommand;
    Vector viewAngles;
    Vector absViewAngles;
    int buttons;
    int oldButtons;
    float forwardMove;
    float sideMove;
    float upMove;
    float maxSpeed;
    float clientMaxSpeed;
    Vector velocity;
    Vector angles;
    Vector oldAngles;
    float outStepHeight;
    Vector outWishVel;
    Vector outJumpVel;
    Vector constraintCenter;
    float constraintRadius;
    float constraintWidth;
    float constraintSpeedFactor;
    PAD(20)
    Vector absOrigin;
};

class GameMovement {
public:
    VIRTUAL_METHOD_V(void, processMovement, 1, (Entity* localPlayer, MoveData* moveData), (this, localPlayer, moveData))
    VIRTUAL_METHOD(void, reset, 2, (), (this))
    VIRTUAL_METHOD(void, startTrackPredictionErrors, 3, (Entity* localPlayer), (this, localPlayer))
    VIRTUAL_METHOD(void, finishTrackPredictionErrors, 4, (Entity* localPlayer), (this, localPlayer))
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
