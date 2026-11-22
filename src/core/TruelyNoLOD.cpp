
#include <Windows.h>
#include <detours.h>
#include <intrin.h>
#include "core/HookChain.hpp"
#include "core/Hooks.hpp"
#include "core/Name.hpp"
#include "core/Patch.hpp"
#include "core/Relocation.hpp"
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace ModAPI {
typedef void (*InitFunction)();
__declspec(dllimport) void AddPostInitFunction(InitFunction);
}

extern "C" LONG WINAPI DetourTransactionAbort();

namespace {
uintptr_t gameBase;
uint32_t gameSize, gameShift;
const unsigned char* relocations;
size_t relocationsSize;
using SetLodFn = void(__thiscall*)(void*, void*, int);
using EffectLevelFn = void(__thiscall*)(void*, void*, int, int);
using ModelBatchFn = int(__thiscall*)(void*, void*, const uint32_t*, int*);
using ModelInstanceFn = int(__thiscall*)(void*, uint32_t, uint32_t, int*, int);
using GetPropertyListFn = bool(__thiscall*)(void*, uint32_t, uint32_t, void**);
using GetKeyFn = bool(__cdecl*)(const void*, uint32_t, uint32_t*);
using ReleaseFn = int(__thiscall*)(void*);
using TerrainSetupFn = int(__thiscall*)(void*, int);
using DistGridUpdateFn = void(__thiscall*)(void*, const float*, const void*);
using CurveFn = float(__cdecl*)(int, const float*, float);
using ShadowUpdateFn = void(__thiscall*)(void*);
SetLodFn setLod;
EffectLevelFn effectLevel;
ModelBatchFn modelBatch;
ModelInstanceFn modelInstance;
TerrainSetupFn terrainSetup;
DistGridUpdateFn distGridUpdate;
CurveFn curve;
ShadowUpdateFn shadowUpdate;

void* At(uint32_t address) {
    return reinterpret_cast<void*>(gameBase + address - 0x400000);
}

uint32_t GameAddress(void* code) {
    return uint32_t(reinterpret_cast<uintptr_t>(code) - gameBase + 0x400000);
}

void __fastcall SetLodHook(void* world, void*, void* model, int lod) {
    setLod(world, model, lod > 0 && lod < 4 ? 0 : lod);
}

using ComponentDescriptionFn = const char*(__thiscall*)(void*, int, int, int);

char* EffectDescription(void* world, const uint32_t* effect, char*& manager, int& directory) {
    int index = int(effect[0x20 / 4]);
    if (index < 0)
        return nullptr;
    manager = *reinterpret_cast<char**>(static_cast<char*>(world) + 0x14);
    directory = index >> 24;
    auto* directories = *reinterpret_cast<char***>(manager + 0x2c);
    return *reinterpret_cast<char**>(directories[directory] + 0x34) + (index & 0xffffff) * 0x58;
}

const char* ComponentDescription(char* manager, int type, int component, int directory) {
    auto get = *reinterpret_cast<ComponentDescriptionFn*>(*reinterpret_cast<char**>(manager) + 0x70);
    return get(manager, type, component, directory);
}

bool LowersDetailByLevel(char* manager, char* desc, int directory) {
    auto* blocks = *reinterpret_cast<char**>(desc + 0x44);
    auto* blocksEnd = *reinterpret_cast<char**>(desc + 0x48);
    bool onlyDistributes = true;
    for (char* b = blocks; b < blocksEnd; b += 0x84)
        switch (*reinterpret_cast<uint8_t*>(b)) {
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x09:
        case 0x0b:
        case 0x2d:
            return false;
        case 0x0d:
            break;
        default:
            onlyDistributes = false;
        }

    bool models = false, scatteredModels = false, scattered = false;
    for (char* b = blocks; b < blocksEnd; b += 0x84) {
        unsigned type = *reinterpret_cast<uint8_t*>(b), from = uint8_t(b[0x40]), to = uint8_t(b[0x41]);
        int component = *reinterpret_cast<int*>(b + 0x80);
        if (component < 0 || from > 1 || to <= 1 || to == 0xff)
            continue;
        if (type == 0x08 || type == 0x25 || type == 0x03) {
            models = true;
        } else if (type == 0x0d) {
            scattered = true;
            const char* c = ComponentDescription(manager, 0x0d, component, directory);
            scatteredModels |= c && *reinterpret_cast<const uint32_t*>(c + 8) & 0x100;
        }
    }
    return models || scatteredModels || (onlyDistributes && scattered);
}

struct DescriptionSet {
    static constexpr unsigned Size = 8192;
    uintptr_t key[Size];
    volatile long lock;
};

DescriptionSet checkedDescriptions;

bool CheckedBefore(const char* desc) {
    auto key = reinterpret_cast<uintptr_t>(desc);
    bool found = false;
    while (_InterlockedExchange(&checkedDescriptions.lock, 1))
        _mm_pause();
    for (unsigned i = unsigned(key >> 4) % DescriptionSet::Size, n = 0; n < DescriptionSet::Size;
         i = (i + 1) % DescriptionSet::Size, ++n) {
        if (checkedDescriptions.key[i] == key) {
            found = true;
            break;
        }
        if (!checkedDescriptions.key[i]) {
            checkedDescriptions.key[i] = key;
            break;
        }
    }
    _InterlockedExchange(&checkedDescriptions.lock, 0);
    return found;
}

void KeepDetailAtEveryLevel(char* desc) {
    auto* blocks = *reinterpret_cast<char**>(desc + 0x44);
    auto* blocksEnd = *reinterpret_cast<char**>(desc + 0x48);
    auto detail = [](unsigned type) { return type == 0x03 || type == 0x08 || type == 0x25 || type == 0x0d; };
    for (char* b = blocks; b < blocksEnd; b += 0x84)
        if (detail(uint8_t(b[0])) && uint8_t(b[0x40]) > 1)
            return;
    for (char* b = blocks; b < blocksEnd; b += 0x84) {
        unsigned type = uint8_t(b[0]), from = uint8_t(b[0x40]), to = uint8_t(b[0x41]);
        auto* scales = *reinterpret_cast<char**>(b + 0x44);
        auto* scalesEnd = *reinterpret_cast<char**>(b + 0x48);
        if (type == 0x0d || !detail(type) || *reinterpret_cast<int*>(b + 0x80) < 0 || from > 1 || to <= 1 ||
            to == 0xff || scalesEnd - scales > 12)
            continue;
        b[0x41] = char(0xff);
    }
}

void __fastcall EffectLevelHook(void* world, void*, void* effect, int level, int hard) {
    char* manager;
    int directory;
    if (char* desc = EffectDescription(world, static_cast<const uint32_t*>(effect), manager, directory)) {
        auto* distances = *reinterpret_cast<float**>(desc + 0x1c);
        auto* distancesEnd = *reinterpret_cast<float**>(desc + 0x20);
        if (distancesEnd - distances >= 2) {
            if (distances[1] != FLT_MAX && !CheckedBefore(desc)) {
                if (LowersDetailByLevel(manager, desc, directory)) {
                    for (float* d = distances + 1; d < distancesEnd; ++d)
                        *d = FLT_MAX;
                } else {
                    KeepDetailAtEveryLevel(desc);
                }
            }
            if (distances[1] == FLT_MAX && level > 1)
                level = 1;
        }
    }
    effectLevel(world, effect, level, hard);
}

bool LevelZeroMesh(uint32_t instance, uint32_t group, uint32_t& mesh, uint32_t& meshGroup, int& level) {
    level = int(group & 0xff);
    if (level < 1 || level > 3)
        return false;
    void* manager = reinterpret_cast<void* (*)()>(At(PropManagerGetAddress))();
    if (!manager)
        return false;
    void* list = nullptr;
    auto getList = *reinterpret_cast<GetPropertyListFn*>(*static_cast<char**>(manager) + GetPropertyListSlot);
    if (!getList(manager, instance, group - level, &list) || !list)
        return false;
    uint32_t reduced[3] = {}, full[3] = {};
    auto getKey = reinterpret_cast<GetKeyFn>(At(PropertyGetKeyAddress));
    bool found = getKey(list, ModelMeshLod0, full) && full[0] && getKey(list, ModelMeshLod0 + level, reduced) &&
                 ((reduced[0] == instance && reduced[2] == group) || (reduced[0] == full[0] && reduced[2] == full[2]));
    (*reinterpret_cast<ReleaseFn*>(*static_cast<char**>(list) + 4))(list);
    mesh = full[0];
    meshGroup = full[2];
    return found;
}

struct MeshSwap {
    static constexpr unsigned Max = 1024;
    uint32_t key[Max][2], to[Max][2];
    unsigned count;
    volatile long lock;
};

MeshSwap swaps;

void LockSwaps() {
    while (_InterlockedExchange(&swaps.lock, 1))
        _mm_pause();
}

void UnlockSwaps() {
    _InterlockedExchange(&swaps.lock, 0);
}

bool KnownSwap(const uint32_t* key, uint32_t& mesh, uint32_t& group) {
    for (unsigned i = 0; i < swaps.count; ++i)
        if (swaps.key[i][0] == key[0] && swaps.key[i][1] == key[1]) {
            mesh = swaps.to[i][0];
            group = swaps.to[i][1];
            return true;
        }
    return false;
}

void FullMesh(uint32_t& instance, uint32_t& group) {
    const uint32_t key[2] = {instance, group};
    LockSwaps();
    bool known = KnownSwap(key, instance, group);
    UnlockSwaps();
    if (known)
        return;
    uint32_t full = 0, fullGroup = 0;
    int level = 0;
    if (LevelZeroMesh(key[0], key[1], full, fullGroup, level)) {
        instance = full;
        group = fullGroup;
    }
    LockSwaps();
    uint32_t stored, storedGroup;
    if (!KnownSwap(key, stored, storedGroup)) {
        if (swaps.count < MeshSwap::Max) {
            unsigned i = swaps.count++;
            swaps.key[i][0] = key[0];
            swaps.key[i][1] = key[1];
            swaps.to[i][0] = instance;
            swaps.to[i][1] = group;
        }
    }
    UnlockSwaps();
}

int __fastcall ModelBatchHook(void* renderer, void*, void* streamer, const uint32_t* slot, int* handle) {
    if (!slot)
        return modelBatch(renderer, streamer, slot, handle);
    uint32_t mesh = slot[0], group = slot[1];
    FullMesh(mesh, group);
    if (mesh == slot[0] && group == slot[1])
        return modelBatch(renderer, streamer, slot, handle);
    uint32_t copy[8];
    memcpy(copy, slot, sizeof(copy));
    copy[0] = mesh;
    copy[1] = group;
    return modelBatch(renderer, streamer, copy, handle);
}

int __fastcall ModelInstanceHook(void* renderer, void*, uint32_t mesh, uint32_t group, int* handle, int extra) {
    FullMesh(mesh, group);
    return modelInstance(renderer, mesh, group, handle, extra);
}

void RaisePlanetQuality() {
    void* props = *static_cast<void**>(At(AppPropertiesAddress));
    if (!props)
        return;
    auto getInt = reinterpret_cast<int(__thiscall*)(void*, uint32_t)>(At(GetIntAddress));
    auto getFloat = reinterpret_cast<float(__thiscall*)(void*, uint32_t)>(At(GetFloatAddress));
    auto getBool = reinterpret_cast<bool(__thiscall*)(void*, uint32_t)>(At(GetBoolAddress));
    auto setInt = reinterpret_cast<void(__thiscall*)(void*, uint32_t, int)>(At(SetIntAddress));
    auto setFloat = reinterpret_cast<void(__thiscall*)(void*, uint32_t, float)>(At(SetFloatAddress));
    auto setBool = reinterpret_cast<void(__thiscall*)(void*, uint32_t, bool)>(At(SetBoolAddress));
    if (getInt(props, PlanetDetailLevel) < PlanetDetailHigh)
        return;
    if (getInt(props, PlanetChunkResolution) < PlanetChunkResolutionValue)
        setInt(props, PlanetChunkResolution, PlanetChunkResolutionValue);
    if (!(getFloat(props, PlanetQualityRange) >= PlanetQualityRangeValue))
        setFloat(props, PlanetQualityRange, PlanetQualityRangeValue);
    if (!getBool(props, PlanetSeabed))
        setBool(props, PlanetSeabed, true);
}

int __fastcall TerrainSetupHook(void* terrain, void*, int argument) {
    RaisePlanetQuality();
    return terrainSetup(terrain, argument);
}

struct DistLevels {
    static constexpr unsigned Max = 128;
    float* levels[Max];
    float original[Max][DistLevelCount][3];
    unsigned count;
    volatile long lock;
};

DistLevels distLevels;

float Reach(float d, float height, float rise) {
    float across = d + height < d * DistReachMax ? d + height : d * DistReachMax;
    return sqrtf(across * across + rise * rise);
}

void __fastcall DistGridUpdateHook(void* grid, void*, const float* camera, const void* matrix) {
    auto* g = static_cast<char*>(grid);
    auto* levels = *reinterpret_cast<float**>(g + DistGridLevels);
    if (levels && camera) {
        float step = *reinterpret_cast<float*>(g + DistGridStep);
        float surface = *reinterpret_cast<float*>(g + DistGridBase) + 32768.0f * step;
        float height = sqrtf(camera[0] * camera[0] + camera[1] * camera[1] + camera[2] * camera[2]) - surface;
        if (!(height > 0))
            height = 0;
        while (_InterlockedExchange(&distLevels.lock, 1))
            _mm_pause();
        unsigned i = 0;
        while (i < distLevels.count && distLevels.levels[i] != levels)
            ++i;
        if (i == distLevels.count && i < DistLevels::Max) {
            distLevels.levels[i] = levels;
            for (size_t l = 0; l < DistLevelCount; ++l)
                for (int k = 0; k < 3; ++k)
                    distLevels.original[i][l][k] = levels[l * (DistLevelSize / 4) + k];
            distLevels.count++;
        }
        if (i < distLevels.count)
            for (size_t l = 0; l < DistLevelCount; ++l) {
                float* level = levels + l * (DistLevelSize / 4);
                const float* d = distLevels.original[i][l];
                level[0] = d[0] > 0 ? Reach(d[0], height, height) : d[0];
                level[1] = d[1] > 0 ? Reach(d[1], height, height * d[DistLevelWeight]) : d[1];
            }
        _InterlockedExchange(&distLevels.lock, 0);
    }
    distGridUpdate(grid, camera, matrix);
}

bool shadowsHigh, effectsFar = true;
int gameMapSize = 512, gameBasisSize = 512;

bool ShadowsHigh(void* props) {
    auto getBool = reinterpret_cast<bool(__thiscall*)(void*, uint32_t)>(At(GetBoolAddress));
    return getBool(props, ShadowEnabled) && getBool(props, ShadowNestedMaps);
}

// Only the shadow map grows: the basis map keeps the game's size, since with any other basis size (1024 and 2048
// tried) the editors' Paint mode shows the creation black.
void SetShadowMapSizes(void* props, int map, int basis) {
    auto setInt = reinterpret_cast<void(__thiscall*)(void*, uint32_t, int)>(At(SetIntAddress));
    setInt(props, ShadowMapSize, map);
    setInt(props, ShadowBasisMapSize, basis);
}

void SetEffectsFar(char* layer, bool on) {
    if (on == effectsFar)
        return;
    effectsFar = on;
    const char* original = ShadowEffectsBlockBytes + (ShadowEffectsPatchAddress - ShadowEffectsBlockAddress);
    Patch::Write(At(ShadowEffectsPatchAddress), on ? ShadowEffectsPatchBytes : original,
                 sizeof(ShadowEffectsPatchBytes) - 1);
    if (!layer)
        return;
    auto* begin = *reinterpret_cast<uint32_t**>(layer + ShadowCasters);
    auto* end = *reinterpret_cast<uint32_t**>(layer + ShadowCasters + 4);
    if (begin && end > begin && !((end - begin) % 3) && end - begin <= 3 * 256)
        for (uint32_t* caster = begin; caster < end; caster += 3)
            if (caster[0] && *reinterpret_cast<void**>(caster[0]) == At(ShadowEffectsLayerVtable) &&
                !(caster[1] & ShadowOuterEffects))
                caster[2] = on ? caster[1] : caster[1] | ShadowOuterEffects;
}

__declspec(noinline) void FollowShadowOption(char* layer) {
    void* props = *static_cast<void**>(At(AppPropertiesAddress));
    if (!props)
        return;
    bool high = ShadowsHigh(props);
    if (high == shadowsHigh)
        return;
    shadowsHigh = high;
    if (high)
        SetShadowMapSizes(props, gameMapSize > ShadowMapSizeValue ? gameMapSize : ShadowMapSizeValue, gameBasisSize);
    else
        SetShadowMapSizes(props, gameMapSize, gameBasisSize);
    SetEffectsFar(layer, high);
    *static_cast<float*>(At(ShadowDepthFactor)) = high ? ShadowDepthFactorValue : 0;
}

void RemakeShadowMaps(char* shadows) {
    reinterpret_cast<void(__thiscall*)(void*)>(At(ShadowDisposeAddress))(shadows);
    reinterpret_cast<void(__thiscall*)(void*)>(At(ShadowCreateAddress))(shadows);
    if (void* targets = *reinterpret_cast<void**>(shadows + ShadowTargets)) {
        void* manager = reinterpret_cast<void*(__cdecl*)()>(At(RenderManagerGetAddress))();
        auto add =
            (*reinterpret_cast<void(__thiscall***)(void*, void*, int, int)>(manager))[RenderManagerAddTargets / 4];
        add(manager, targets, *reinterpret_cast<int*>(shadows + ShadowPass), 0);
    }
}

__declspec(noinline) bool RaiseShadowMaps() {
    void* props = *static_cast<void**>(At(AppPropertiesAddress));
    auto* layer = *static_cast<char**>(At(ShadowLayerAddress));
    if (!props)
        return false;
    char* shadows = layer && *reinterpret_cast<void**>(layer) == At(ShadowLayerVtable) ? layer - 8 : nullptr;
    if (layer && !shadows)
        return false;
    auto getInt = reinterpret_cast<int(__thiscall*)(void*, uint32_t)>(At(GetIntAddress));
    int map = getInt(props, ShadowMapSize), basis = getInt(props, ShadowBasisMapSize);
    gameMapSize = map > 0 ? map : 512;
    gameBasisSize = basis > 0 ? basis : 512;
    shadowsHigh = ShadowsHigh(props);
    if (!shadowsHigh)
        return true;
    if (gameMapSize >= ShadowMapSizeValue)
        return true;
    SetShadowMapSizes(props, ShadowMapSizeValue, gameBasisSize);
    if (shadows && *reinterpret_cast<void**>(shadows + ShadowTargets))
        RemakeShadowMaps(shadows);
    return true;
}

__declspec(noinline) void LetEffectsShadowFar() {
    auto* layer = *static_cast<char**>(At(ShadowLayerAddress));
    if (!layer || *reinterpret_cast<void**>(layer) != At(ShadowLayerVtable))
        return;
    auto* begin = *reinterpret_cast<uint32_t**>(layer + ShadowCasters);
    auto* end = *reinterpret_cast<uint32_t**>(layer + ShadowCasters + 4);
    if (!begin || end < begin || (end - begin) % 3 || end - begin > 3 * 256)
        return;
    for (uint32_t* caster = begin; caster < end; caster += 3)
        if (caster[0] && *reinterpret_cast<void**>(caster[0]) == At(ShadowEffectsLayerVtable) &&
            (caster[2] & ShadowOuterEffects) && !(caster[1] & ShadowOuterEffects))
            caster[2] &= ~ShadowOuterEffects;
}

void __fastcall ShadowUpdateHook(char* layer, void*) {
    if (*reinterpret_cast<void**>(layer) == At(ShadowLayerVtable))
        FollowShadowOption(layer);
    shadowUpdate(layer);
}

void CentreShadowDepth() {
    auto* factor = static_cast<float*>(At(ShadowDepthFactor));
    if (*factor == 0 && shadowsHigh)
        *factor = ShadowDepthFactorValue;
}

constexpr float Pi = 3.14159265f;

float shadowWidest, shadowDistance;

float CurveMax(int count, const float* points) {
    float most = points[0];
    for (int i = 1; i < count; ++i)
        if (points[i] > most)
            most = points[i];
    return most;
}

float __cdecl CurveHook(int count, const float* points, float input) {
    uint32_t caller = GameAddress(_ReturnAddress());
    size_t call = 0;
    while (call < ShadowCurveCallerCount && ShadowCurveCallers[call] != caller)
        ++call;
    if (call == ShadowCurveCallerCount || count < 1 || !points || !shadowsHigh)
        return curve(count, points, input);
    float result;
    if (caller == ShadowAngleCaller) {
        result = CurveMax(count, points);
        shadowWidest = result;
    } else if (caller == ShadowZoomCaller) {
        result = CurveMax(count, points);
        if (!(result >= ShadowZoomMin))
            result = ShadowZoomMin;
    } else if (caller == ShadowZoomInverseCaller) {
        float least = 0;
        for (int i = 0; i < count; ++i)
            if (points[i] > 0 && (least == 0 || points[i] < least))
                least = points[i];
        float zoom = least > 0 && 1 / least >= ShadowZoomMin ? 1 / least : ShadowZoomMin;
        result = 1 / zoom;
    } else if (caller == ShadowStrengthCaller) {
        result = curve(count, points, 0.0f);
    } else {
        result = curve(count, points, 1.0f);
        if (caller == ShadowDistanceCaller) {
            shadowDistance = result;
        } else if (caller == ShadowDepthCaller) {
            float half = shadowDistance > 0 && shadowWidest > 0 && shadowWidest < 180
                             ? tanf(shadowWidest * Pi / 360) * shadowDistance
                             : 0;
            if (half > result)
                result = half;
        }
    }
    return result;
}

bool HostMatches() {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(gameBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(gameBase + dos->e_lfanew);
    gameSize = nt->OptionalHeader.SizeOfImage;
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        (gameSize != HostImageSize && gameSize != SteamHostImageSize) || nt->FileHeader.TimeDateStamp != HostTimestamp)
        return false;
    const auto& table = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (table.VirtualAddress && table.Size <= gameSize && table.VirtualAddress <= gameSize - table.Size) {
        relocations = reinterpret_cast<const unsigned char*>(gameBase + table.VirtualAddress);
        relocationsSize = table.Size;
    }
    return true;
}

bool Same(uint32_t address, const char* expected, size_t size) {
    unsigned char code[MaxCheckedSize];
    if (size > sizeof(code))
        return false;
    Relocation::ReadAsFile(reinterpret_cast<const unsigned char*>(gameBase), relocations, relocationsSize, gameShift,
                           address, code, size);
    return !memcmp(code, expected, size);
}

__declspec(noinline) bool Matches(const BytePatch& p) {
    return Same(p.block, p.expected, p.size);
}

__declspec(noinline) bool Apply(const BytePatch& p) {
    return Patch::Write(At(p.address), p.bytes, p.length);
}

__declspec(noinline) void Undo(const BytePatch& p) {
    Patch::Write(At(p.address), p.expected + (p.address - p.block), p.length);
}

bool Hook(uint32_t address, const char* expected, size_t size, void** original, void* detour, bool chainable) {
    auto* code = At(address);
    if (Same(address, expected, size)) {
        *original = code;
        LONG err = DetourTransactionBegin();
        if (err == NO_ERROR)
            err = DetourUpdateThread(GetCurrentThread());
        if (err == NO_ERROR)
            err = DetourAttach(original, detour);
        if (err != NO_ERROR) {
            DetourTransactionAbort();
        } else {
            err = DetourTransactionCommit();
        }
        return err == NO_ERROR;
    }
    uintptr_t next = chainable ? HookChain::ForeignTarget(code, gameBase, gameSize) : 0;
    if (next) {
        *original = reinterpret_cast<void*>(next);
        return HookChain::JumpTo(code, detour);
    }
    return false;
}

template <typename Fn> bool Hook(uint32_t address, const char* expected, Fn& original, void* detour, bool chainable) {
    return Hook(address, expected, 16, reinterpret_cast<void**>(&original), detour, chainable);
}

int Install() {
    int failed = 0;

    if (Same(LodBlockAddress, LodBlockBytes, LodBlockSize) &&
        Patch::Write(At(LodPatchAddress), LodPatchBytes, LodPatchSize)) {
        if (!Hook(SetLodAddress, SetLodBytes, setLod, reinterpret_cast<void*>(&SetLodHook), true)) {
            Patch::Write(At(LodPatchAddress), LodBlockBytes + LodPatchOffset, LodPatchSize);
            ++failed;
        }
    } else {
        ++failed;
    }

    if (Matches(HiMeshUpdatePatch) && Matches(HiMeshDrawPatch)) {
        if (Apply(HiMeshUpdatePatch)) {
            if (!Apply(HiMeshDrawPatch)) {
                Undo(HiMeshUpdatePatch);
                ++failed;
            }
        } else {
            ++failed;
        }
    } else {
        ++failed;
    }

    if (!Matches(MaterialLodPatch) || !Apply(MaterialLodPatch))
        ++failed;
    if (!Matches(EffectRangePatch) || !Apply(EffectRangePatch))
        ++failed;

    if (!Hook(DistGridUpdateAddress, DistGridUpdateBytes, distGridUpdate, reinterpret_cast<void*>(&DistGridUpdateHook),
              true))
        ++failed;

    if (!Hook(TerrainSetupAddress, TerrainSetupBytes, terrainSetup, reinterpret_cast<void*>(&TerrainSetupHook), true))
        ++failed;

    if (Matches(ShadowEffectsPatch) && Apply(ShadowEffectsPatch))
        LetEffectsShadowFar();
    else
        ++failed;
    if (Same(ShadowDisposeAddress, ShadowDisposeBytes, 16) && Same(ShadowCreateAddress, ShadowCreateBytes, 16) &&
        RaiseShadowMaps()) {
        if (!shadowsHigh)
            SetEffectsFar(*static_cast<char**>(At(ShadowLayerAddress)), false);
        if (Hook(CurveAddress, CurveBytes, curve, reinterpret_cast<void*>(&CurveHook), true))
            CentreShadowDepth();
        else
            ++failed;
        if (!Hook(ShadowUpdateAddress, ShadowUpdateBytes, shadowUpdate, reinterpret_cast<void*>(&ShadowUpdateHook),
                  true))
            ++failed;
    } else {
        ++failed;
    }

    if (!Hook(ModelBatchAddress, ModelBatchBytes, modelBatch, reinterpret_cast<void*>(&ModelBatchHook), true))
        ++failed;
    if (!Hook(ModelInstanceAddress, ModelInstanceBytes, modelInstance, reinterpret_cast<void*>(&ModelInstanceHook),
              true))
        ++failed;
    if (!Hook(EffectLevelAddress, EffectLevelBytes, effectLevel, reinterpret_cast<void*>(&EffectLevelHook), true))
        ++failed;

    return failed;
}

void Init() {
    gameBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    gameShift = uint32_t(gameBase - Relocation::PreferredBase);
    bool host = HostMatches();
    int failed = host ? Install() : 0;
    if (host && !failed)
        return;
    const wchar_t* text =
        !host ? L"" TRUELY_NO_LOD_NAME " could not start: this version of Spore is not supported. The game keeps its "
                L"normal level of detail.\n\n" TRUELY_NO_LOD_NAME " supports only Spore Galactic Adventures with the "
                L"March 2017 patch (SporeApp_ModAPIFix.exe, or Steam's SporeApp.exe)."
              : L"" TRUELY_NO_LOD_NAME " started only partly: another mod has already changed some of the same game "
                L"code, which keeps its normal level of detail.";
    MessageBoxW(nullptr, text, ModNameWide, MB_OK | MB_ICONERROR);
}
}

BOOL WINAPI DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        ModAPI::AddPostInitFunction(Init);
    }
    return TRUE;
}

