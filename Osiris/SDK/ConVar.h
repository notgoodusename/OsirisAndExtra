#pragma once

#include <type_traits>

#include "Platform.h"
#include "UtlVector.h"
#include "Pad.h"
#include "VirtualMethod.h"

enum CvarFlags
{
	NONE = 0,
	UNREGISTERED = (1 << 0),
	DEVELOPMENTONLY = (1 << 1),
	GAMEDLL = (1 << 2),
	CLIENTDLL = (1 << 3),
	HIDDEN = (1 << 4),
	PROTECTED = (1 << 5),
	SPONLY = (1 << 6),
	ARCHIVE = (1 << 7),
	NOTIFY = (1 << 8),
	USERINFO = (1 << 9),
	CHEAT = (1 << 14),
	PRINTABLEONLY = (1 << 10),
	UNLOGGED = (1 << 11),
	NEVER_AS_STRING = (1 << 12),
	REPLICATED = (1 << 13),
	DEMO = (1 << 16),
	DONTRECORD = (1 << 17),
	NOT_CONNECTED = (1 << 22),
	ARCHIVE_XBOX = (1 << 24),
	SERVER_CAN_EXECUTE = (1 << 28),
	SERVER_CANNOT_QUERY = (1 << 29),
	CLIENTCMD_CAN_EXECUTE = (1 << 30)
};

struct ConVar {
    VIRTUAL_METHOD(float, getFloat, 12, (), (this))
    VIRTUAL_METHOD(int, getInt, 13, (), (this))
    VIRTUAL_METHOD(void, setValue, 14, (const char* value), (this, value))
    VIRTUAL_METHOD(void, setValue, 15, (float value), (this, value))
    VIRTUAL_METHOD(void, setValue, 16, (int value), (this, value))

    PAD(24)
    std::add_pointer_t<void __cdecl()> changeCallback;
    ConVar* parent;
    const char* defaultValue;
    char* string;
    PAD(44)
    UtlVector<void(__cdecl*)()> onChangeCallbacks;
};
