#include <algorithm>
#include <Windows.h>

#include "VmtSwap.h"

static auto calculateVmtLength(std::uintptr_t* vmt) noexcept
{
    std::size_t length = 0;
    MEMORY_BASIC_INFORMATION memoryInfo;
    while (VirtualQuery(LPCVOID(vmt[length]), &memoryInfo, sizeof(memoryInfo)) && memoryInfo.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        length++;
    return length;
}

void VmtSwap::init(void* base) noexcept
{
    this->base = base;
    oldVmt = *reinterpret_cast<std::uintptr_t**>(base);
    std::size_t length = calculateVmtLength(oldVmt) + dynamicCastInfoLength;
    newVmt = std::make_unique<std::uintptr_t[]>(length);
    std::copy(oldVmt - dynamicCastInfoLength, oldVmt - dynamicCastInfoLength + length, newVmt.get());
    *reinterpret_cast<std::uintptr_t**>(base) = newVmt.get() + dynamicCastInfoLength;
}
