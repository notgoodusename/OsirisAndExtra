#include <memory>
#include <clocale>
#include <Windows.h>

#include "Hooks.h"
#include <string>

inline bool exist_chk(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

BOOL APIENTRY DllEntryPoint(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
    if (!_CRT_INIT(moduleHandle, reason, reserved))
        return FALSE;

    if (reason == DLL_PROCESS_ATTACH) {
        std::setlocale(LC_CTYPE, ".utf8");
        hooks = std::make_unique<Hooks>(moduleHandle);
        if (!exist_chk("csgo/resource/flash/images/hitbox.png"))
            MessageBoxA(NULL,"Failed to find hitbox picture", "Osility", MB_OK | MB_ICONWARNING);
    }
    return TRUE;
}
