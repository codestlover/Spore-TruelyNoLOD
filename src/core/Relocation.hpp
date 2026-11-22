
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// The game executable is relocatable. Windows with ASLR maps it at another base than 0x400000 (Wine does not), and
// the loader then adds the shift to every dword the executable's relocation table names, the absolute addresses in
// the game's code included. ReadAsFile reads the game's code as the executable file has it, those dwords shifted
// back, so the original bytes a change is checked against are the file's at any base. With no shift it is a copy.
namespace Relocation {
constexpr uint32_t PreferredBase = 0x400000;
constexpr uint32_t PageSize = 0x1000;
constexpr unsigned HighLow = 3;

inline uint32_t Dword(const unsigned char* p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

// image: the mapped executable; table and tableSize: its base relocation directory; shift: the image's base minus
// 0x400000, modulo 2^32. Copies size bytes of the game's code at the game address into out. A relocated dword that
// sticks out of the span is read whole, so up to three bytes either side of the span must be readable.
inline void ReadAsFile(const unsigned char* image, const unsigned char* table, size_t tableSize, uint32_t shift,
                       uint32_t address, unsigned char* out, size_t size) {
    uint32_t start = address - PreferredBase, end = start + uint32_t(size);
    memcpy(out, image + start, size);
    if (!shift || !table)
        return;
    for (size_t block = 0; tableSize - block >= 8;) {
        uint32_t page = Dword(table + block), blockSize = Dword(table + block + 4);
        if (blockSize < 8 || blockSize > tableSize - block)
            return;
        if (page < end && start < page + PageSize + 3)
            for (size_t entry = block + 8; entry + 2 <= block + blockSize; entry += 2) {
                unsigned value = table[entry] | unsigned(table[entry + 1]) << 8;
                uint32_t at = page + (value & 0xfff);
                if (value >> 12 != HighLow || at >= end || at + 4 <= start)
                    continue;
                uint32_t original = Dword(image + at) - shift;
                unsigned char bytes[4];
                memcpy(bytes, &original, sizeof(bytes));
                for (uint32_t i = 0; i < 4; ++i)
                    if (at + i >= start && at + i < end)
                        out[at + i - start] = bytes[i];
            }
        block += blockSize;
    }
}
}

