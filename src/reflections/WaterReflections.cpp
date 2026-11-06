
#include <Windows.h>
#include <d3d9.h>
#include <wincrypt.h>
#include <detours.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "core/HookChain.hpp"
#include "core/Hooks.hpp"
#include "reflections/PlanarMath.hpp"
#include "reflections/SeamlessLink.hpp"
#include "reflections/WaterReflections.hpp"
#include "ui/Options.hpp"

namespace WaterReflections {
namespace {
struct PixelCase {
    const char* name;
    unsigned char signature[32];
    unsigned signatureSize, originalSize;
    unsigned char originalHash[32];
    const DWORD* code;
    unsigned codeSize;
};

#include "WaterShaders.hpp"

struct Raster {
    DWORD format;
    unsigned short flags, volume;
    void* object;
    unsigned short width, height;
    unsigned char depth, levels;
    unsigned short face;
    void* parent;
    void* swapChain;
    void* data;
};

static_assert(sizeof(Raster) == 0x20);

struct PixelRecord {
    unsigned char signature[32];
    IDirect3DPixelShader9* shader;
    unsigned char rest[0x130 - 36];
};

struct ShaderVector {
    PixelRecord* begin;
    PixelRecord* end;
    PixelRecord* capacity;
};

struct WaterShader {
    IDirect3DPixelShader9* original;
    IDirect3DPixelShader9* reflected;
};

using MirrorPassFn = void(__thiscall*)(void*, void**, void*, int);
using TerrainWaterFn = void(__thiscall*)(void*, void*, int);
using SeabedFn = void(__thiscall*)(void*, void*, void*);
using SubRenderFn = void(__thiscall*)(void*, void*, int, int, void**, bool, int);
using DrawFn = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
using PrimitiveFn = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
using ResetFn = HRESULT(WINAPI*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

constexpr D3DFORMAT DepthTextureFormat = D3DFORMAT(MAKEFOURCC('I', 'N', 'T', 'Z'));
constexpr uint32_t ReflectionPassAddress = 0xfa5c70, TerrainWaterAddress = 0xf97fc0, SeabedAddress = 0xf984d0;
constexpr unsigned char ReflectionPassBytes[16] = {0x51, 0x53, 0x55, 0x56, 0x57, 0x8b, 0x3d, 0x18,
                                                   0xd9, 0x5f, 0x01, 0x8b, 0xf1, 0x68, 0x50, 0x4e};
constexpr unsigned char TerrainWaterBytes[16] = {0x83, 0xec, 0x18, 0xf6, 0x05, 0xa0, 0xa5, 0x6c,
                                                 0x01, 0x01, 0x56, 0x57, 0x8b, 0x3d, 0x18, 0xd9};
constexpr unsigned char SeabedBytes[16] = {0x53, 0x55, 0x56, 0x57, 0x8b, 0xf1, 0x8b, 0x0d,
                                           0x18, 0xd9, 0x5f, 0x01, 0x68, 0x21, 0xc3, 0xad};
constexpr uint32_t ViewerConstruct = 0x7c4040, ViewerInitialize = 0x7c4ea0, ViewerCopy = 0x7c5160,
                   ViewerSetViewport = 0x7c4b30, ViewerSetWindow = 0x7c4bd0, ViewerSetTransform = 0x7c4d10,
                   ViewerClear = 0x7c3d20, FrustumFromMatrix = 0x6ffe90, FrustumTestBox = 0x7001b0,
                   RenderManagerGlobal = 0x15fd8c0, ShaderTable = 0x16198a0, DeviceGlobal = 0x16f89d0;
constexpr size_t TerrainFrustum = 0x92c, TerrainFrustumSize = 0xf0, TerrainState = 0x4f8, TerrainStateSize = 0x260,
                 TerrainWaterLevel = 0x524, TerrainSphereRadius = 0x538, TerrainChunks = 0x190, ChunkSize = 16,
                 QuadBox = 0x70, QuadBoxMax = 0x7c, QuadWaterIndices = 0x9c, ViewerCamera = 0x170,
                 ViewerBackground = 0x140, CameraRasters = 0x40, CameraDepth = 0x50, CameraWindow = 0x5c,
                 CameraViewport = 0x78, FrustumFromViewer = 0xc0;
constexpr int SceneLayers[] = {0x0a, 0x0c, 0x0d, 0x0e, 0x12, 0x14, 0x15};
constexpr int TerrainLayer = 8, TerrainFlags = 0x02000000;
constexpr int PlanetWorldLayer = 0x0d;
constexpr uint32_t ModelLayerVtable = 0x140d7c8, PlanetWorldLayerVtable = 0x14998f4, PlanetModelWorld = 0xb3d620,
                   SpacePlayerData = 0x16dda8c;
constexpr size_t WorldAsLayer = 0x13c, SpaceContext = 0x10;
constexpr size_t ModelLayerHorizon = 0x2b0;
constexpr int ModelPassFlags = 0x7b00;
constexpr unsigned MaxModelLayers = 8;
constexpr unsigned MaxWidth = 1024;
constexpr float WindowScale = 1.2f, SkyFloor = 0.06f, ObjectFloor = 0.55f, RippleAmount = 0.12f;
constexpr float MaxAltitude = 1.2f, FadeAltitude = 0.8f;
constexpr unsigned long long IdleFrames = 30;
constexpr unsigned MaxShaders = 8, ParameterRegister = 28, ClipRegister = 50, ClipRegisters = 10;

uintptr_t base;
IDirect3DDevice9* device;
void** deviceVtable;
bool installed, failed, shuttingDown, capturing, ready, prepared;
unsigned waterDepth, shaderCount;
WaterShader shaders[MaxShaders]{};
unsigned long long frame, lastWaterFrame, captureDraws;
void* lastTerrain;
float strength;

alignas(16) unsigned char viewerStorage[0x174];
char* viewer;
IDirect3DTexture9* colorTexture;
IDirect3DTexture9* depthTexture;
IDirect3DSurface9* depthSurface;
Raster colorRaster{}, depthRaster{};

PlanarMath::Mirror mirror{};
float mirrorClip[16]{}, mirrorInverse[16]{}, seenView[16]{}, seenProjection[16]{}, clipPlane[4]{}, depthParameters[4]{};
bool clipValid, mirrorFrozen, terrainPass, clipForTerrain;
DWORD savedClipEnable;

MirrorPassFn originalPass;
TerrainWaterFn originalWater;
SeabedFn originalSeabed;
DrawFn originalDraw;
PrimitiveFn originalPrimitive;
ResetFn originalReset;

void* At(uint32_t address) {
    return reinterpret_cast<void*>(base + address - 0x400000);
}

template <class T> T& Global(uint32_t address) {
    return *static_cast<T*>(At(address));
}

char* PlanetWorldLayerObject() {
    char* player = Global<char*>(SpacePlayerData);
    if (!player || *reinterpret_cast<int*>(player + SpaceContext) != 0)
        return nullptr;
    void* world = reinterpret_cast<void* (*)()>(At(PlanetModelWorld))();
    if (!world)
        return nullptr;
    auto asLayer = *reinterpret_cast<char*(__thiscall**)(void*)>(*static_cast<char**>(world) + WorldAsLayer);
    char* layer = asLayer(world);
    if (!layer || *reinterpret_cast<uintptr_t*>(layer) != reinterpret_cast<uintptr_t>(At(ModelLayerVtable)))
        return nullptr;
    return layer;
}

void Disable() {
    failed = true;
    ready = false;
}

bool Hash(const void* data, DWORD size, unsigned char result[32]) {
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    DWORD count = 32;
    bool ok = CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT) &&
              CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) &&
              CryptHashData(hash, static_cast<const BYTE*>(data), size, 0) &&
              CryptGetHashParam(hash, HP_HASHVAL, result, &count, 0) && count == 32;
    if (hash)
        CryptDestroyHash(hash);
    if (provider)
        CryptReleaseContext(provider, 0);
    return ok;
}

bool Known(IDirect3DPixelShader9* shader) {
    for (unsigned i = 0; i < shaderCount; ++i)
        if (shaders[i].original == shader)
            return true;
    return false;
}

void AddShader(IDirect3DPixelShader9* original, const DWORD* code, const char* name) {
    (void)name;
    IDirect3DPixelShader9* reflected = nullptr;
    if (!original || Known(original) || shaderCount == MaxShaders ||
        FAILED(device->CreatePixelShader(code, &reflected)))
        return;
    original->AddRef();
    shaders[shaderCount++] = {original, reflected};
}

void LinkSeamlessWater() {
    HMODULE module = GetModuleHandleW(L"SeamlessWater.dll");
    if (!module)
        return;
    auto link =
        reinterpret_cast<SeamlessWaterLinkFn>(reinterpret_cast<void*>(GetProcAddress(module, "SeamlessWater_Link")));
    const SeamlessWaterLink* water = link ? link() : nullptr;
    if (!water) {
        return;
    }
    if (water->contract != ReflectionContract) {
        return;
    }
    for (unsigned i = 0; i < water->count; ++i)
        AddShader(water->shaders[i].shader, water->shaders[i].reflection, water->shaders[i].name);
}

void AddGameShaders() {
    const auto& list = Global<ShaderVector>(ShaderTable);
    if (!list.begin || list.end < list.begin || list.end - list.begin > 16384)
        return;
    for (PixelRecord* record = list.begin; record != list.end; ++record) {
        bool water = false;
        for (const auto& s : kPixelCases)
            water = water || !memcmp(record->signature, s.signature, s.signatureSize);
        if (!water || !record->shader || Known(record->shader))
            continue;
        UINT size = 0;
        if (FAILED(record->shader->GetFunction(nullptr, &size)) || size < 8 || size > 65536)
            continue;
        void* bytes = HeapAlloc(GetProcessHeap(), 0, size);
        if (!bytes)
            continue;
        unsigned char hash[32];
        bool ok = SUCCEEDED(record->shader->GetFunction(bytes, &size)) && Hash(bytes, size, hash);
        HeapFree(GetProcessHeap(), 0, bytes);
        const PixelCase* match = nullptr;
        for (const auto& s : kPixelCases)
            if (ok && size == s.originalSize && !memcmp(hash, s.originalHash, 32) &&
                !memcmp(record->signature, s.signature, s.signatureSize))
                match = &s;
        if (match)
            AddShader(record->shader, match->code, match->name);
    }
}

bool PrepareShaders() {
    if (!prepared) {
        prepared = true;
        LinkSeamlessWater();
        AddGameShaders();
    }
    return shaderCount != 0;
}

void ReleaseTargets() {
    ready = false;
    if (viewer) {
        char* camera = *reinterpret_cast<char**>(viewer + ViewerCamera);
        if (camera) {
            *reinterpret_cast<Raster**>(camera + CameraRasters) = nullptr;
            *reinterpret_cast<Raster**>(camera + CameraDepth) = nullptr;
        }
    }
    if (depthSurface)
        depthSurface->Release();
    if (depthTexture)
        depthTexture->Release();
    if (colorTexture)
        colorTexture->Release();
    depthSurface = nullptr;
    depthTexture = nullptr;
    colorTexture = nullptr;
    colorRaster = {};
    depthRaster = {};
}

bool EnsureTargets(unsigned short width, unsigned short height) {
    if (colorTexture && colorRaster.width == width && colorRaster.height == height)
        return true;
    ReleaseTargets();
    if (FAILED(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                                     &colorTexture, nullptr)) ||
        FAILED(device->CreateTexture(width, height, 1, D3DUSAGE_DEPTHSTENCIL, DepthTextureFormat, D3DPOOL_DEFAULT,
                                     &depthTexture, nullptr)) ||
        FAILED(depthTexture->GetSurfaceLevel(0, &depthSurface))) {
        ReleaseTargets();
        return false;
    }
    colorRaster.format = D3DFMT_A8R8G8B8;
    colorRaster.flags = 9;
    colorRaster.object = colorTexture;
    colorRaster.width = width;
    colorRaster.height = height;
    colorRaster.depth = 32;
    colorRaster.levels = 1;
    depthRaster.format = D3DFMT_D24S8;
    depthRaster.flags = 2;
    depthRaster.object = depthSurface;
    depthRaster.width = width;
    depthRaster.height = height;
    depthRaster.depth = 32;
    depthRaster.levels = 1;
    depthParameters[2] = 1.0f / width;
    depthParameters[3] = 1.0f / height;
    return true;
}

bool EnsureViewer() {
    if (viewer)
        return true;
    reinterpret_cast<void(__thiscall*)(void*)>(At(ViewerConstruct))(viewerStorage);
    if (!reinterpret_cast<bool(__thiscall*)(void*, bool)>(At(ViewerInitialize))(viewerStorage, false))
        return false;
    viewer = reinterpret_cast<char*>(viewerStorage);
    return true;
}

bool WaterInView(char* terrain, char* mainViewer) {
    alignas(16) unsigned char frustum[TerrainFrustumSize + 16];
    reinterpret_cast<void(__thiscall*)(void*, void*)>(At(FrustumFromMatrix))(frustum, mainViewer + FrustumFromViewer);
    char* begin = *reinterpret_cast<char**>(terrain + TerrainChunks);
    char* end = *reinterpret_cast<char**>(terrain + TerrainChunks + 4);
    if (!begin || end < begin || size_t(end - begin) > 16384 * ChunkSize)
        return false;
    auto test = reinterpret_cast<int(__thiscall*)(void*, const void*, const void*, int)>(At(FrustumTestBox));
    for (char* chunk = begin; chunk < end; chunk += ChunkSize) {
        char* quad = *reinterpret_cast<char**>(chunk);
        if (quad && *reinterpret_cast<int*>(quad + QuadWaterIndices) > 0 &&
            !(test(frustum, quad + QuadBox, quad + QuadBoxMax, 0) & 0x40))
            return true;
    }
    return false;
}

void RenderMirror(char* terrain, char* mainViewer, void** mainViewers, void* stats) {
    ready = false;
    ++frame;
    if (terrain != lastTerrain) {
        lastTerrain = terrain;
        lastWaterFrame = frame;
    }
    if (frame - lastWaterFrame > IdleFrames)
        return;
    char* mainCamera = *reinterpret_cast<char**>(mainViewer + ViewerCamera);
    float radius = *reinterpret_cast<float*>(terrain + TerrainSphereRadius) +
                   *reinterpret_cast<float*>(terrain + TerrainWaterLevel);
    if (!mainCamera || !(radius > 1 && radius < 1.0e7f))
        return;
    const auto* source = reinterpret_cast<const float*>(mainViewer);
    float altitude = std::sqrt(PlanarMath::Dot(source + 12, source + 12)) - radius;
    if (altitude > MaxAltitude * radius)
        return;
    float fade = (MaxAltitude * radius - altitude) / ((MaxAltitude - FadeAltitude) * radius);
    strength = fade < 1 ? fade : 1;
    if (!PlanarMath::MirrorCamera(source, radius, mirror) || !WaterInView(terrain, mainViewer))
        return;
    const auto* viewport = reinterpret_cast<const D3DVIEWPORT9*>(mainCamera + CameraViewport);
    if (!viewport->Width || !viewport->Height)
        return;
    float scale = viewport->Width > MaxWidth ? float(MaxWidth) / viewport->Width : 1.0f;
    auto width = static_cast<unsigned short>(viewport->Width * scale);
    auto height = static_cast<unsigned short>(viewport->Height * scale);
    if (!EnsureTargets(width, height)) {
        Disable();
        return;
    }
    if (!EnsureViewer()) {
        Disable();
        return;
    }
    static unsigned char savedFrustum[TerrainFrustumSize], savedState[TerrainStateSize];
    memcpy(savedFrustum, terrain + TerrainFrustum, TerrainFrustumSize);
    memcpy(savedState, terrain + TerrainState, TerrainStateSize);

    reinterpret_cast<bool(__thiscall*)(void*, void*, bool, bool)>(At(ViewerCopy))(viewer, mainViewer, true, true);
    char* camera = *reinterpret_cast<char**>(viewer + ViewerCamera);
    auto** rasters = reinterpret_cast<Raster**>(camera + CameraRasters);
    rasters[0] = &colorRaster;
    rasters[1] = rasters[2] = rasters[3] = nullptr;
    *reinterpret_cast<Raster**>(camera + CameraDepth) = &depthRaster;
    const int rectangle[4] = {0, 0, width, height};
    reinterpret_cast<void(__thiscall*)(void*, const int*)>(At(ViewerSetViewport))(viewer, rectangle);
    const auto* window = reinterpret_cast<const float*>(mainCamera + CameraWindow);
    reinterpret_cast<void(__thiscall*)(void*, float, float)>(At(ViewerSetWindow))(viewer, window[0] * WindowScale,
                                                                                  window[1] * WindowScale);
    reinterpret_cast<void(__thiscall*)(void*, const float*)>(At(ViewerSetTransform))(viewer, mirror.camera);
    memset(viewer + ViewerBackground, 0, 16);
    reinterpret_cast<bool(__thiscall*)(void*, int)>(At(ViewerClear))(viewer, 7);

    device->GetRenderState(D3DRS_CLIPPLANEENABLE, &savedClipEnable);
    clipValid = false;
    mirrorFrozen = false;
    capturing = true;
    unsigned long long before = captureDraws;
    void* manager = Global<void*>(RenderManagerGlobal);
    auto draw = *reinterpret_cast<SubRenderFn*>(*static_cast<char**>(manager) + 0x40);
    void* viewers[4];
    for (unsigned i = 0; i < 4; ++i)
        viewers[i] = mainViewers[i] ? viewer : nullptr;
    terrainPass = true;
    draw(manager, stats, TerrainLayer, TerrainLayer, viewers, false, TerrainFlags);
    terrainPass = false;
    constexpr unsigned SceneCount = sizeof(SceneLayers) / sizeof(SceneLayers[0]);
    int passFlags[SceneCount]{};
    bool otherLayer[SceneCount]{}, skipLayer[SceneCount]{};
    char* modelLayers[MaxModelLayers];
    float horizons[MaxModelLayers];
    unsigned modelCount = 0;
    const auto modelVtable = reinterpret_cast<uintptr_t>(At(ModelLayerVtable));
    char* entry = *reinterpret_cast<char**>(static_cast<char*>(manager) + 0x34c);
    char* entriesEnd = *reinterpret_cast<char**>(static_cast<char*>(manager) + 0x350);
    for (; entry && entry < entriesEnd; entry += 20) {
        char* layerObject = *reinterpret_cast<char**>(entry);
        int index = *reinterpret_cast<int*>(entry + 4);
        unsigned scene = 0;
        while (scene < SceneCount && SceneLayers[scene] != index)
            ++scene;
        if (scene == SceneCount)
            continue;
        if (index == PlanetWorldLayer) {
            bool forwarding = layerObject && *reinterpret_cast<uintptr_t*>(layerObject) ==
                                                 reinterpret_cast<uintptr_t>(At(PlanetWorldLayerVtable));
            layerObject = forwarding ? PlanetWorldLayerObject() : nullptr;
            if (!layerObject) {
                skipLayer[scene] = true;
                continue;
            }
        }
        if (!layerObject || *reinterpret_cast<uintptr_t*>(layerObject) != modelVtable) {
            otherLayer[scene] = true;
            continue;
        }
        passFlags[scene] = ModelPassFlags;
        bool known = false;
        for (unsigned i = 0; i < modelCount; ++i)
            known = known || modelLayers[i] == layerObject;
        if (known || modelCount == MaxModelLayers)
            continue;
        modelLayers[modelCount] = layerObject;
        horizons[modelCount] = *reinterpret_cast<float*>(layerObject + ModelLayerHorizon);
        *reinterpret_cast<float*>(layerObject + ModelLayerHorizon) = 0;
        ++modelCount;
    }
    for (unsigned scene = 0; scene < SceneCount; ++scene) {
        int layer = SceneLayers[scene];
        if (skipLayer[scene])
            continue;
        draw(manager, stats, layer, layer, viewers, false, otherLayer[scene] ? 0 : passFlags[scene]);
    }
    for (unsigned i = 0; i < modelCount; ++i)
        *reinterpret_cast<float*>(modelLayers[i] + ModelLayerHorizon) = horizons[i];
    capturing = false;
    device->SetRenderState(D3DRS_CLIPPLANEENABLE, savedClipEnable);
    memcpy(terrain + TerrainFrustum, savedFrustum, TerrainFrustumSize);
    memcpy(terrain + TerrainState, savedState, TerrainStateSize);
    ready = mirrorFrozen && captureDraws != before;
}

void __fastcall ReflectionPass(void* terrain, void*, void** viewers, void* stats, int flags) {
    originalPass(terrain, viewers, stats, flags);
    if (capturing)
        return;
    ready = false;
    if (failed || shuttingDown || !TruelyOptions::Reflections() || !viewers || !viewers[0] || !device)
        return;
    if (!PrepareShaders()) {
        return;
    }
    RenderMirror(static_cast<char*>(terrain), static_cast<char*>(viewers[0]), viewers, stats);
}

void __fastcall TerrainWater(void* terrain, void*, void* stats, int flags) {
    if (capturing)
        return;
    ++waterDepth;
    originalWater(terrain, stats, flags);
    --waterDepth;
}

void __fastcall Seabed(void* terrain, void*, void* seabedViewer, void* stats) {
    if (!capturing)
        originalSeabed(terrain, seabedViewer, stats);
}

void PrepareCaptureDraw(IDirect3DDevice9* self) {
    ++captureDraws;
    float view[16], projection[16];
    if (FAILED(self->GetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(view))) ||
        FAILED(self->GetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(projection))))
        return;
    if (!clipValid || clipForTerrain != terrainPass || memcmp(view, seenView, sizeof(view)) ||
        memcmp(projection, seenProjection, sizeof(projection))) {
        memcpy(seenView, view, sizeof(view));
        memcpy(seenProjection, projection, sizeof(projection));
        float current[16];
        PlanarMath::Multiply(view, projection, current);
        if (!mirrorFrozen) {
            memcpy(mirrorClip, current, sizeof(current));
            PlanarMath::Inverse(mirrorClip, mirrorInverse);
            depthParameters[0] = projection[10];
            depthParameters[1] = projection[14];
            mirrorFrozen = true;
        }
        clipForTerrain = terrainPass;
        clipValid = PlanarMath::ClipPlane(current, terrainPass ? mirror.plane : mirror.clip, clipPlane);
        if (clipValid)
            self->SetClipPlane(0, clipPlane);
    }
    if (clipValid)
        self->SetRenderState(D3DRS_CLIPPLANEENABLE, savedClipEnable | 1);
}

HRESULT DrawReflectedWater(IDirect3DDevice9* self, const WaterShader& water, D3DPRIMITIVETYPE type, INT baseVertex,
                           UINT minVertex, UINT vertices, UINT startIndex, UINT primitives) {
    IDirect3DBaseTexture9* oldColor = nullptr;
    IDirect3DBaseTexture9* oldDepth = nullptr;
    const D3DSAMPLERSTATETYPE states[] = {D3DSAMP_MINFILTER, D3DSAMP_MAGFILTER, D3DSAMP_MIPFILTER,
                                          D3DSAMP_ADDRESSU,  D3DSAMP_ADDRESSV,  D3DSAMP_SRGBTEXTURE};
    DWORD colorStates[6], depthStates[6];
    float oldParameters[4], oldClip[4 * ClipRegisters];
    self->GetTexture(6, &oldColor);
    self->GetTexture(9, &oldDepth);
    for (unsigned i = 0; i < 6; ++i) {
        self->GetSamplerState(6, states[i], &colorStates[i]);
        self->GetSamplerState(9, states[i], &depthStates[i]);
    }
    self->GetPixelShaderConstantF(ParameterRegister, oldParameters, 1);
    self->GetPixelShaderConstantF(ClipRegister, oldClip, ClipRegisters);
    const DWORD colorValues[6] = {D3DTEXF_LINEAR,    D3DTEXF_LINEAR,    D3DTEXF_NONE,
                                  D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, 0};
    const DWORD depthValues[6] = {D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE, D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, 0};
    for (unsigned i = 0; i < 6; ++i) {
        self->SetSamplerState(6, states[i], colorValues[i]);
        self->SetSamplerState(9, states[i], depthValues[i]);
    }
    self->SetTexture(6, colorTexture);
    self->SetTexture(9, depthTexture);
    const float parameters[4] = {strength, SkyFloor, RippleAmount, ObjectFloor};
    self->SetPixelShaderConstantF(ParameterRegister, parameters, 1);
    self->SetPixelShaderConstantF(ClipRegister, mirrorClip, 4);
    self->SetPixelShaderConstantF(ClipRegister + 4, depthParameters, 1);
    self->SetPixelShaderConstantF(ClipRegister + 6, mirrorInverse, 4);
    self->SetPixelShader(water.reflected);
    HRESULT result = originalDraw(self, type, baseVertex, minVertex, vertices, startIndex, primitives);
    self->SetPixelShader(water.original);
    self->SetTexture(6, oldColor);
    self->SetTexture(9, oldDepth);
    for (unsigned i = 0; i < 6; ++i) {
        self->SetSamplerState(6, states[i], colorStates[i]);
        self->SetSamplerState(9, states[i], depthStates[i]);
    }
    self->SetPixelShaderConstantF(ParameterRegister, oldParameters, 1);
    self->SetPixelShaderConstantF(ClipRegister, oldClip, ClipRegisters);
    if (oldColor)
        oldColor->Release();
    if (oldDepth)
        oldDepth->Release();
    return result;
}

HRESULT WINAPI Primitive(IDirect3DDevice9* self, D3DPRIMITIVETYPE type, UINT first, UINT count) {
    if (capturing && self == device)
        PrepareCaptureDraw(self);
    return originalPrimitive(self, type, first, count);
}

HRESULT WINAPI Draw(IDirect3DDevice9* self, D3DPRIMITIVETYPE type, INT baseVertex, UINT minVertex, UINT vertices,
                    UINT startIndex, UINT primitives) {
    if (self == device) {
        if (capturing) {
            PrepareCaptureDraw(self);
        } else if (waterDepth && shaderCount) {
            IDirect3DPixelShader9* bound = nullptr;
            if (SUCCEEDED(self->GetPixelShader(&bound)) && bound) {
                const WaterShader* water = nullptr;
                for (unsigned i = 0; i < shaderCount; ++i)
                    if (shaders[i].original == bound)
                        water = &shaders[i];
                bound->Release();
                if (water) {
                    lastWaterFrame = frame;
                    if (ready && !failed && TruelyOptions::Reflections())
                        return DrawReflectedWater(self, *water, type, baseVertex, minVertex, vertices, startIndex,
                                                  primitives);
                }
            }
        }
    }
    return originalDraw(self, type, baseVertex, minVertex, vertices, startIndex, primitives);
}

HRESULT WINAPI Reset(IDirect3DDevice9* self, D3DPRESENT_PARAMETERS* parameters) {
    if (self == device)
        ReleaseTargets();
    return originalReset(self, parameters);
}

bool Vtable(void** slot, void* value) {
    DWORD protection;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection))
        return false;
    InterlockedExchangePointer(slot, value);
    DWORD ignored;
    VirtualProtect(slot, sizeof(void*), protection, &ignored);
    return true;
}

template <class Fn> bool Hook(uint32_t address, const unsigned char* expected, Fn& original, void* detour) {
    void* code = At(address);
    if (!memcmp(code, expected, 16)) {
        original = reinterpret_cast<Fn>(code);
        LONG result = DetourTransactionBegin();
        if (!result)
            result = DetourUpdateThread(GetCurrentThread());
        if (!result)
            result = DetourAttach(reinterpret_cast<void**>(&original), detour);
        if (result) {
            DetourTransactionAbort();
            return false;
        }
        return DetourTransactionCommit() == NO_ERROR;
    }
    uintptr_t next = HookChain::ForeignTarget(code, base, HostImageSize);
    if (!next)
        return false;
    original = reinterpret_cast<Fn>(next);
    return HookChain::JumpTo(code, detour);
}
}

bool Install(uintptr_t gameBase) {
    base = gameBase;
    device = Global<IDirect3DDevice9*>(DeviceGlobal);
    if (!device)
        return false;
    D3DCAPS9 caps{};
    if (FAILED(device->GetDeviceCaps(&caps)) || !caps.MaxUserClipPlanes) {
        return false;
    }
    IDirect3D9* d3d = nullptr;
    D3DDEVICE_CREATION_PARAMETERS creation{};
    D3DDISPLAYMODE mode{};
    bool depthTextures = SUCCEEDED(device->GetDirect3D(&d3d)) && SUCCEEDED(device->GetCreationParameters(&creation)) &&
                         SUCCEEDED(device->GetDisplayMode(0, &mode)) &&
                         SUCCEEDED(d3d->CheckDeviceFormat(creation.AdapterOrdinal, creation.DeviceType, mode.Format,
                                                          D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, DepthTextureFormat));
    if (d3d)
        d3d->Release();
    if (!depthTextures) {
        return false;
    }
    if (!Hook(ReflectionPassAddress, ReflectionPassBytes, originalPass, reinterpret_cast<void*>(&ReflectionPass)) ||
        !Hook(TerrainWaterAddress, TerrainWaterBytes, originalWater, reinterpret_cast<void*>(&TerrainWater)) ||
        !Hook(SeabedAddress, SeabedBytes, originalSeabed, reinterpret_cast<void*>(&Seabed))) {
        return false;
    }
    deviceVtable = *reinterpret_cast<void***>(device);
    originalReset = reinterpret_cast<ResetFn>(deviceVtable[16]);
    originalPrimitive = reinterpret_cast<PrimitiveFn>(deviceVtable[81]);
    originalDraw = reinterpret_cast<DrawFn>(deviceVtable[82]);
    if (!Vtable(deviceVtable + 16, reinterpret_cast<void*>(&Reset)) ||
        !Vtable(deviceVtable + 81, reinterpret_cast<void*>(&Primitive)) ||
        !Vtable(deviceVtable + 82, reinterpret_cast<void*>(&Draw))) {
        Disable();
        return false;
    }
    installed = true;
    return true;
}

bool Capturing() {
    return capturing;
}

void Shutdown() {
    shuttingDown = true;
    ready = false;
    if (!installed)
        return;
    if (deviceVtable[82] == reinterpret_cast<void*>(&Draw))
        Vtable(deviceVtable + 82, reinterpret_cast<void*>(originalDraw));
    if (deviceVtable[81] == reinterpret_cast<void*>(&Primitive))
        Vtable(deviceVtable + 81, reinterpret_cast<void*>(originalPrimitive));
    if (deviceVtable[16] == reinterpret_cast<void*>(&Reset))
        Vtable(deviceVtable + 16, reinterpret_cast<void*>(originalReset));
    ReleaseTargets();
    for (unsigned i = 0; i < shaderCount; ++i) {
        shaders[i].original->Release();
        shaders[i].reflected->Release();
    }
    shaderCount = 0;
    installed = false;
}
}

