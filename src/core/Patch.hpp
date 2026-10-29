
#pragma once
#include <Windows.h>
#include <cstring>

namespace Patch {
inline bool Write(void* code, const void* bytes, size_t size) {
    DWORD old = 0;
    if (!VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &old))
        return false;
    memcpy(code, bytes, size);
    VirtualProtect(code, size, old, &old);
    FlushInstructionCache(GetCurrentProcess(), code, size);
    return true;
}
}

