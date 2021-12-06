#pragma once

#include "VirtualMethod.h"
#include "Pad.h"
#include "MemAlloc.h"
#include "../Memory.h"

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

inline int bitByte(int bits) {

	return (bits + 7) >> 3;

}

struct bfWrite {
	void startWriting(void* data, int bytes, int startBit = 0, int bits = -1) {

		bytes &= ~3;

		this->data = (unsigned char*)data;
		dataBytes = bytes;

		if (bits == -1)
			dataBits = bytes << 3;
		else
			dataBits = bits;

		curBit = startBit;
		overflow = false;

	}

	inline unsigned char* getData() {

		return data;

	}

	inline int getNumBytesWritten() const {

		return bitByte(curBit);

	}

	unsigned char* data;
	int dataBytes;
	int dataBits;
	int curBit;

private:
	bool overflow;
	bool assertOnOverflow;
	const char* debugName;
};


struct clMsgMove
{

	clMsgMove() {
		netMessageVtable = memory->clSendMove + 0x76;
		clMsgMoveVtable = memory->clSendMove + 0x82;
		allocatedMemory = reinterpret_cast<void*>(memory->clSendMove + 0x7b);

		unknown = 15;

		flags = 3;

		unknown1 = 0;
		unknown2 = 0;
		unknown3 = 0;
		unknown4 = 0;
		unknown5 = 0;

	}

	~clMsgMove() {
		//memory->clMsgMoveDescontructor(this);
	}

	inline auto setNumBackupCommands(int backupCommands) {
		this->backupCommands = backupCommands;
	}

	inline auto setNumNewCommands(int newCommands) {
		this->newCommands = newCommands;
	}

	inline auto setData(unsigned char* data, int numBytesWritten) {

		flags |= 4;

		if (allocatedMemory == reinterpret_cast<void*>(memory->clSendMove + 0x7b)) {
			
			void* newMemory = memory->memalloc->Alloc(24);
			if (newMemory) {
				*((unsigned long*)newMemory + 0x14) = 15;
				*((unsigned long*)newMemory + 0x10) = 0;
				*(unsigned char*)newMemory = 0;
			}

			allocatedMemory = (void*)newMemory;
		}

		return memory->clMsgMoveSetData(allocatedMemory, data, numBytesWritten);
	}

	int netMessageVtable; // 0x58 88 0
	int clMsgMoveVtable; // 0x54 84 4
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
};