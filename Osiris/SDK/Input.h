#pragma once

#include "UserCmd.h"
#include "Pad.h"
#include "Vector.h"

class Input {
public:
    PAD(12)
    bool isTrackIRAvailable;
    bool isMouseInitialized;
    bool isMouseActive;
    PAD(154)
    bool isCameraInThirdPerson;
    PAD(2)
    Vector cameraOffset;
    PAD(56)
    UserCmd* commands;
    VerifiedUserCmd* verifiedCommands;

    VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))

    VerifiedUserCmd* getVerifiedUserCmd(int sequenceNumber) noexcept
    {
        return &verifiedCommands[sequenceNumber % 150];
    }
};

static const char* ButtonCodes[] =
{
	"NONE",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"PAD_0",
	"PAD_1",
	"PAD_2",
	"PAD_3",
	"PAD_4",
	"PAD_5",
	"PAD_6",
	"PAD_7",
	"PAD_8",
	"PAD_9",
	"PAD_DIVIDE",
	"PAD_MULTIPLY",
	"PAD_MINUS",
	"PAD_PLUS",
	"PAD_ENTER",
	"PAD_DECIMAL",
	"LBRACKET",
	"RBRACKET",
	"SEMICOLON",
	"APOSTROPHE",
	"BACKQUOTE",
	"COMMA",
	"PERIOD",
	"SLASH",
	"BACKSLASH",
	"MINUS",
	"EQUAL",
	"ENTER",
	"SPACE",
	"BACKSPACE",
	"TAB",
	"CAPSLOCK",
	"NUMLOCK",
	"ESCAPE",
	"SCROLLLOCK",
	"INSERT",
	"DELETE",
	"HOME",
	"END",
	"PAGEUP",
	"PAGEDOWN",
	"BREAK",
	"LSHIFT",
	"RSHIFT",
	"LALT",
	"RALT",
	"LCONTROL",
	"RCONTROL",
	"LWIN",
	"RWIN",
	"APP",
	"UP",
	"LEFT",
	"DOWN",
	"RIGHT",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"CAPSLOCKTOGGLE",
	"NUMLOCKTOGGLE",
	"SCROLLLOCKTOGGLE",
};
