#pragma once

#include "../Memory.h"

#include "VirtualMethod.h"
#include "Pad.h"
#include "BitBuffer.h"
#include "MemAlloc.h"

enum ECstrike15UserMessages
{
	CS_UM_VGUIMenu = 1,
	CS_UM_Geiger = 2,
	CS_UM_Train = 3,
	CS_UM_HudText = 4,
	CS_UM_SayText = 5,
	CS_UM_SayText2 = 6,
	CS_UM_TextMsg = 7,
	CS_UM_HudMsg = 8,
	CS_UM_ResetHud = 9,
	CS_UM_GameTitle = 10,
	CS_UM_Shake = 12,
	CS_UM_Fade = 13,
	CS_UM_Rumble = 14,
	CS_UM_CloseCaption = 15,
	CS_UM_CloseCaptionDirect = 16,
	CS_UM_SendAudio = 17,
	CS_UM_RawAudio = 18,
	CS_UM_VoiceMask = 19,
	CS_UM_RequestState = 20,
	CS_UM_Damage = 21,
	CS_UM_RadioText = 22,
	CS_UM_HintText = 23,
	CS_UM_KeyHintText = 24,
	CS_UM_ProcessSpottedEntityUpdate = 25,
	CS_UM_ReloadEffect = 26,
	CS_UM_AdjustMoney = 27,
	CS_UM_UpdateTeamMoney = 28,
	CS_UM_StopSpectatorMode = 29,
	CS_UM_KillCam = 30,
	CS_UM_DesiredTimescale = 31,
	CS_UM_CurrentTimescale = 32,
	CS_UM_AchievementEvent = 33,
	CS_UM_MatchEndConditions = 34,
	CS_UM_DisconnectToLobby = 35,
	CS_UM_PlayerStatsUpdate = 36,
	CS_UM_DisplayInventory = 37,
	CS_UM_WarmupHasEnded = 38,
	CS_UM_ClientInfo = 39,
	CS_UM_XRankGet = 40,
	CS_UM_XRankUpd = 41,
	CS_UM_CallVoteFailed = 45,
	CS_UM_VoteStart = 46,
	CS_UM_VotePass = 47,
	CS_UM_VoteFailed = 48,
	CS_UM_VoteSetup = 49,
	CS_UM_ServerRankRevealAll = 50,
	CS_UM_SendLastKillerDamageToClient = 51,
	CS_UM_ServerRankUpdate = 52,
	CS_UM_ItemPickup = 53,
	CS_UM_ShowMenu = 54,
	CS_UM_BarTime = 55,
	CS_UM_AmmoDenied = 56,
	CS_UM_MarkAchievement = 57,
	CS_UM_MatchStatsUpdate = 58,
	CS_UM_ItemDrop = 59,
	CS_UM_GlowPropTurnOff = 60,
	CS_UM_SendPlayerItemDrops = 61,
	CS_UM_RoundBackupFilenames = 62,
	CS_UM_SendPlayerItemFound = 63,
	CS_UM_ReportHit = 64,
	CS_UM_XpUpdate = 65,
	CS_UM_QuestProgress = 66,
	CS_UM_ScoreLeaderboardData = 67,
	CS_UM_PlayerDecalDigitalSignature = 68,
	MAX_ECSTRIKE15USERMESSAGES
};

class NetworkChannel {
public:
    VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))
	VIRTUAL_METHOD(bool, sendNetMsg, 40, (void* msg, bool forceReliable = false, bool voice = false), (this, msg, forceReliable, voice))

    std::byte pad[24];
    int outSequenceNr;
    int inSequenceNr;
    int outSequenceNrAck;
    int outReliableState;
    int inReliableState;
    int chokedPackets;
};

struct clMsgMove
{
	clMsgMove() {
		netMessageVtable = *reinterpret_cast<std::uint32_t*>(memory->clSendMove + 126);
		clMsgMoveVtable = *reinterpret_cast<std::uint32_t*>(memory->clSendMove + 138);
		allocatedMemory = *reinterpret_cast<void**>(memory->clSendMove + 131);
		unknown = 15;

		flags = 3;

		unknown1 = 0;
		unknown2 = 0;
		unknown3 = 0;
		unknown4 = 0;
		unknown5 = 0;

	}

	~clMsgMove() {
		memory->clMsgMoveDescontructor(this);
	}

	inline auto setNumBackupCommands(int backupCommands) {
		this->backupCommands = backupCommands;
	}

	inline auto setNumNewCommands(int newCommands) {
		this->newCommands = newCommands;
	}

	inline auto setData(unsigned char* data, int numBytesWritten) {

		flags |= 4;

		//Why does this work??
		if (allocatedMemory == reinterpret_cast<void*>(memory->clSendMove + 131)) {
			
			void* newMemory = memory->memalloc->Alloc(24);
			if (newMemory) {
				*((unsigned long*)newMemory + 0x14) = 15;
				*((unsigned long*)newMemory + 0x10) = 0;
				*(unsigned char*)newMemory = 0;
			}

			allocatedMemory = newMemory;
		}

		return memory->clMsgMoveSetData(allocatedMemory, data, numBytesWritten);
	}

	std::uint32_t netMessageVtable; // 0x58 88 0
	std::uint32_t clMsgMoveVtable; // 0x54 84 4
	int unknown1; // 0x4c 80 8
	int backupCommands; // 0x4c 76 12
	int newCommands; // 0x48 72 16
	void* allocatedMemory; // 0x44 68 20
	int unknown2; // 0x40 64 24
	int flags; // 0x3c 60 28
	char unknown3; // 0x38 64 32
	PAD(3); // 65 33
	char unknown4; // 0x34 68 36
	PAD(15); // 69 37
	int unknown5; // 0x24 84 52
	int unknown; // 0x20 88 56
	bufferWrite dataOut;
};