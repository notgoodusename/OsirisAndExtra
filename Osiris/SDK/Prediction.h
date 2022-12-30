#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

class Entity;
class MoveData;
class MoveHelper;
struct UserCmd;

class Prediction {
public:
    PAD(4);
    std::uintptr_t lastGround;
    bool inPrediction;
    bool oldCLPredictValue;
    bool isFirstTimePredicted;
    bool enginePaused;
	int previousStartFrame;
	int incomingPacketNumber;
	float lastServerWorldTimeStamp;

	struct Split
	{
		bool		isFirstTimePredicted;
		PAD(3)
		int			commandsPredicted;
		int			serverCommandsAcknowledged;
		int			previousAckHadErrors;
		float		idealPitch;
		int			lastCommandAcknowledged;
		bool		previousAckErrorTriggersFullLatchReset;
		UtlVector<std::uintptr_t> vecEntitiesWithPredictionErrorsInLastAck;
		bool		performedTickShift;
	};

	Split split[1];

    VIRTUAL_METHOD_V(void, update, 3, (int startFrame, bool validFrame, int incAck, int outCmd), (this, startFrame, validFrame, incAck, outCmd))
	VIRTUAL_METHOD_V(void, checkMovingGround, 18, (Entity* localPlayer, double frameTime), (this, localPlayer, frameTime))
    VIRTUAL_METHOD_V(void, setupMove, 20, (Entity* localPlayer, UserCmd* cmd, MoveHelper* moveHelper, MoveData* moveData), (this, localPlayer, cmd, moveHelper, moveData))
    VIRTUAL_METHOD_V(void, finishMove, 21, (Entity* localPlayer, UserCmd* cmd, MoveData* moveData), (this, localPlayer, cmd, moveData))
};
