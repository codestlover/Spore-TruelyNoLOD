
#pragma once
#include <cstddef>
#include <cstdint>

// The game's code as the executable file has it, at any base (src/core/Relocation.hpp), for the parts of the mod
// outside TruelyNoLOD.cpp. Valid once the game executable has been checked.
namespace GameCode {
bool Is(uint32_t address, const void* expected, size_t size);
uint32_t ImageSize();
}

