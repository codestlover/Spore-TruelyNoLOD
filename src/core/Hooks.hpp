
#pragma once
#include <cstddef>
#include <cstdint>

constexpr unsigned HostImageSize = 26271744;
// Steam's SporeApp.exe is the same build with SteamStub's section on top: the same time stamp, a larger image.
constexpr unsigned SteamHostImageSize = 26443776;
constexpr unsigned HostTimestamp = 1727737552;

constexpr uint32_t SetLodAddress = 0x744cf0;
constexpr const char SetLodBytes[] = "\x8b\x44\x24\x04\x33\xc9\x56\x3b\xc1\x74\x05\x8d\x70\xf8\xeb\x02";

constexpr uint32_t LodBlockAddress = 0x75116a;
constexpr const char LodBlockBytes[] =
    "\x8b\x8e\xd4\x00\x00\x00\x8b\x44\x24\x50\x89\x86\x30\x01\x00\x00\x85\xc9\x74\x61\x80\xbe\x28\x01\x00\x00\x00"
    "\xf3\x0f\x5e\x46\x20\x76\x24\x8d\x64\x24\x00\x8a\x86\x28\x01\x00\x00\x0f\xb6\xd0\xf3\x0f\x10\x4c\x91\xfc\x0f"
    "\x2f\xc8\x76\x0c\xfe\xc8\x88\x86\x28\x01\x00\x00\x84\xc0\x77\xe0\x8a\x86\x28\x01\x00\x00\x3a\x86\x29\x01\x00"
    "\x00\x73\x21\x8b\xff\x8a\x86\x28\x01\x00\x00\x0f\xb6\xd0\x0f\x2f\x04\x91\x76\x10\xfe\xc0\x88\x86\x28\x01\x00"
    "\x00\x3a\x86\x29\x01\x00\x00\x72\xe1\x8a\x86\x28\x01\x00\x00\x3c\x04";

constexpr uint32_t LodPatchAddress = 0x75117e;
constexpr const char LodPatchBytes[] = "\xc6\x86\x28\x01\x00\x00\x00\xeb\x58";

constexpr uint32_t HiMeshUpdateAddress = 0x751238;
constexpr const char HiMeshUpdateBytes[] =
    "\x80\xbe\x28\x01\x00\x00\x00\x75\x17\x8b\x4e\x0c\xc1\xe9\x0d\xf6\xc1\x01\x74\x0c\x8b\x86\xb0\x00\x00\x00\x85\xc0"
    "\x74\x02\x8b\xf8";
constexpr uint32_t HiMeshUpdatePatchAddress = 0x75124a;
constexpr uint32_t HiMeshDrawAddress = 0x74fa30;
constexpr const char HiMeshDrawBytes[] =
    "\x80\xbe\x28\x01\x00\x00\x00\x75\x19\x8b\x4e\x0c\xc1\xe9\x0d\xf6\xc1\x01\x74\x0e\x8b\x86\xb0\x00\x00\x00\x3b\xc5"
    "\x74\x04\x89\x44\x24\x14";
constexpr uint32_t HiMeshDrawPatchAddress = 0x74fa42;
constexpr const char HiMeshPatchBytes[] = "\x90\x90";

constexpr uint32_t MaterialLodBlockAddress = 0x74fd3b;
constexpr const char MaterialLodBlockBytes[] =
    "\xf3\x0f\x5c\xdc\xf3\x0f\x11\x9c\x24\x80\x00\x00\x00\x8b\x94\x24\x80\x00\x00\x00\xc1\xea\x1f\x02\xc2\x88\x44"
    "\x24\x13";
constexpr uint32_t MaterialLodPatchAddress = 0x74fd52;
constexpr const char MaterialLodPatchBytes[] = "\x32\xc0";

constexpr uint32_t EffectRangeBlockAddress = 0x7529e5;
constexpr const char EffectRangeBlockBytes[] =
    "\xf3\x0f\x10\x86\x24\x01\x00\x00\x0f\x2e\x05\xb4\x52\x48\x01\x9f\xf6\xc4\x44\x7b\x1a\xf3\x0f\x10\x8e\x90"
    "\x00\x00\x00\xf3\x0f\x59\x8c\x24\xbc\x00\x00\x00\x0f\x2f\xc1\x77\x04\x32\xdb\xeb\x02\xb3\x01";
constexpr uint32_t EffectRangePatchAddress = 0x752a10;
constexpr const char EffectRangePatchBytes[] = "\xb3\x01";

constexpr uint32_t DistGridUpdateAddress = 0xf66b50;
constexpr const char DistGridUpdateBytes[] = "\x83\xec\x0c\x53\x8b\xd9\x8b\x83\xa0\x00\x00\x00\x56\x85\xc0\x74";
constexpr size_t DistGridLevels = 0x0c, DistGridBase = 0x2c, DistGridStep = 0x30, DistLevelSize = 0x28,
                 DistLevelCount = 5, DistLevelWeight = 2;
constexpr float DistReachMax = 2.0f;

constexpr uint32_t TerrainSetupAddress = 0xfa0f40;
constexpr const char TerrainSetupBytes[] = "\xff\x05\x64\x9e\x6c\x01\x83\xec\x20\x53\x55\x56\x57\x6a\x00\x6a";
constexpr uint32_t AppPropertiesAddress = 0x15fd918;
constexpr uint32_t GetBoolAddress = 0x6a25a0, GetIntAddress = 0x6a2660, GetFloatAddress = 0x6a2710;
constexpr uint32_t SetBoolAddress = 0x6a17e0, SetIntAddress = 0x6a1880, SetFloatAddress = 0x6a1910;
constexpr uint32_t PlanetChunkResolution = 0xe8172372, PlanetQualityRange = 0x21, PlanetSeabed = 0x4badc321,
                   PlanetReflections = 0xf1a9135a;
constexpr uint32_t PlanetDetailLevel = 0x3ffad0a;
constexpr int PlanetDetailHigh = 2;
constexpr int PlanetChunkResolutionValue = 32;
constexpr float PlanetQualityRangeValue = 120000.0f;

constexpr uint32_t ModelBatchAddress = 0x6f0390;
constexpr const char ModelBatchBytes[] = "\x6a\xff\x68\x73\xf3\x20\x01\x64\xa1\x00\x00\x00\x00\x50\x64\x89";
constexpr uint32_t ModelInstanceAddress = 0x6f0610;
constexpr const char ModelInstanceBytes[] = "\x6a\xff\x68\x93\xf3\x20\x01\x64\xa1\x00\x00\x00\x00\x50\x64\x89";
constexpr uint32_t PropManagerGetAddress = 0x67ddf0;
constexpr unsigned GetPropertyListSlot = 0x2c;
constexpr uint32_t PropertyGetKeyAddress = 0x6a1250;
constexpr uint32_t ModelMeshLod0 = 0xf9efbb;

constexpr uint32_t EffectLevelAddress = 0xa719a0;
constexpr const char EffectLevelBytes[] = "\x81\xec\xb8\x00\x00\x00\x53\x55\x8b\xac\x24\xc4\x00\x00\x00\x8b";

constexpr size_t SetLodSize = sizeof(SetLodBytes) - 1;
constexpr size_t LodBlockSize = sizeof(LodBlockBytes) - 1;
constexpr size_t LodPatchSize = sizeof(LodPatchBytes) - 1;
constexpr size_t LodPatchOffset = LodPatchAddress - LodBlockAddress;
static_assert(SetLodSize == 16 && LodBlockSize == 0x7d && LodPatchSize == 9, "Hook table");
static_assert(LodPatchOffset + LodPatchSize <= LodBlockSize, "The patch stays inside the checked block");
static_assert(LodPatchAddress + LodPatchSize + uint8_t(LodPatchBytes[8]) == 0x7511df,
              "The jump lands on the native mesh selection");
constexpr uint32_t CurveAddress = 0x77ef60;
constexpr const char CurveBytes[] = "\x8b\x44\x24\x04\x83\xf8\x01\x75\x07\x8b\x44\x24\x08\xd9\x00\xc3";
constexpr uint32_t ShadowCurveCallers[] = {0x780e36, 0x780e58, 0x78104e, 0x781070, 0x7813b6, 0x7813e7};
constexpr size_t ShadowCurveCallerCount = sizeof(ShadowCurveCallers) / sizeof(ShadowCurveCallers[0]);
constexpr uint32_t ShadowStrengthCaller = 0x780e36, ShadowAngleCaller = 0x780e58, ShadowDistanceCaller = 0x781070;
constexpr uint32_t ShadowZoomCaller = 0x7813e7, ShadowZoomInverseCaller = 0x7813b6;
constexpr float ShadowZoomMin = 1.5f;
constexpr uint32_t ShadowEnabled = 0x276abef, ShadowNestedMaps = 0x276abf8;
constexpr uint32_t ShadowDepthCaller = 0x78104e, ShadowDepthFactor = 0x1633bc4;
constexpr float ShadowDepthFactorValue = 0.5f;
constexpr uint32_t ShadowEffectsBlockAddress = 0x78279c;
constexpr const char ShadowEffectsBlockBytes[] = "\x8b\x44\x24\x2c\x89\x44\x24\x10\x0d\x00\x00\x04\x00\x89\x44\x24\x14";
constexpr uint32_t ShadowEffectsPatchAddress = 0x7827a4;
constexpr const char ShadowEffectsPatchBytes[] = "\x0d\x00\x00\x00\x00";
constexpr uint32_t ShadowEffectsLayerVtable = 0x140e9bc, ShadowOuterEffects = 0x40000;
constexpr size_t ShadowCasters = 0x14;
constexpr uint32_t ShadowUpdateAddress = 0x780c20;
constexpr const char ShadowUpdateBytes[] = "\x55\x8b\xec\x83\xe4\xf0\x81\xec\xf4\x00\x00\x00\x53\x56\x8b\x35";
constexpr uint32_t ShadowMapSize = 0x276ac00, ShadowBasisMapSize = 0x33cb1dad;
constexpr int ShadowMapSizeValue = 2048;
constexpr uint32_t ShadowLayerAddress = 0x15fd8dc, ShadowLayerVtable = 0x140ea20;
constexpr size_t ShadowTargets = 0x1bc, ShadowPass = 0x220, RenderManagerAddTargets = 0x4c;
constexpr uint32_t ShadowDisposeAddress = 0x780750;
constexpr const char ShadowDisposeBytes[] = "\x83\xec\x0c\x53\x55\x56\x57\x8b\xf9\x80\x7f\x0d\x00\x74\x2f\x6a";
constexpr uint32_t ShadowCreateAddress = 0x780b60;
constexpr const char ShadowCreateBytes[] = "\x56\x57\x8b\xf1\xe8\xb7\xd1\xef\xff\x8b\x0d\xe8\xa2\x53\x01\x8b";
constexpr uint32_t RenderManagerGetAddress = 0x67dd10;

static_assert(sizeof(EffectLevelBytes) == 17 && sizeof(ModelBatchBytes) == 17 && sizeof(TerrainSetupBytes) == 17 &&
                  sizeof(DistGridUpdateBytes) == 17 && sizeof(ModelInstanceBytes) == 17 && sizeof(CurveBytes) == 17 &&
                  sizeof(ShadowDisposeBytes) == 17 && sizeof(ShadowCreateBytes) == 17 &&
                  sizeof(ShadowUpdateBytes) == 17,
              "Hook table");

constexpr size_t MaxCheckedSize = 128;
static_assert(LodBlockSize <= MaxCheckedSize && sizeof(HiMeshUpdateBytes) - 1 <= MaxCheckedSize &&
                  sizeof(HiMeshDrawBytes) - 1 <= MaxCheckedSize &&
                  sizeof(MaterialLodBlockBytes) - 1 <= MaxCheckedSize &&
                  sizeof(EffectRangeBlockBytes) - 1 <= MaxCheckedSize &&
                  sizeof(ShadowEffectsBlockBytes) - 1 <= MaxCheckedSize,
              "Every checked block fits the check's buffer");

// No patch covers a byte of a dword the game's relocation table names, so a patch and its original bytes are the
// same at any base.
struct BytePatch {
    uint32_t block;
    const char* expected;
    size_t size;
    uint32_t address;
    const char* bytes;
    size_t length;
};

#define TRUENOLOD_PATCH(block, address, bytes)                                                                         \
    BytePatch{block##Address, block##Bytes, sizeof(block##Bytes) - 1, address, bytes, sizeof(bytes) - 1}
constexpr BytePatch HiMeshUpdatePatch = TRUENOLOD_PATCH(HiMeshUpdate, HiMeshUpdatePatchAddress, HiMeshPatchBytes);
constexpr BytePatch HiMeshDrawPatch = TRUENOLOD_PATCH(HiMeshDraw, HiMeshDrawPatchAddress, HiMeshPatchBytes);
constexpr BytePatch EffectRangePatch =
    TRUENOLOD_PATCH(EffectRangeBlock, EffectRangePatchAddress, EffectRangePatchBytes);
constexpr BytePatch MaterialLodPatch =
    TRUENOLOD_PATCH(MaterialLodBlock, MaterialLodPatchAddress, MaterialLodPatchBytes);
constexpr BytePatch ShadowEffectsPatch =
    TRUENOLOD_PATCH(ShadowEffectsBlock, ShadowEffectsPatchAddress, ShadowEffectsPatchBytes);
#undef TRUENOLOD_PATCH

constexpr bool Inside(const BytePatch& p) {
    return p.address >= p.block && p.address + p.length <= p.block + p.size;
}

static_assert(Inside(HiMeshUpdatePatch) && Inside(HiMeshDrawPatch) && Inside(MaterialLodPatch) &&
                  Inside(EffectRangePatch) && Inside(ShadowEffectsPatch),
              "Every patch stays inside its checked block");
static_assert(HiMeshUpdateBytes[HiMeshUpdatePatchAddress - HiMeshUpdateAddress] == '\x74' &&
                  HiMeshDrawBytes[HiMeshDrawPatchAddress - HiMeshDrawAddress] == '\x74' &&
                  MaterialLodBlockBytes[MaterialLodPatchAddress - MaterialLodBlockAddress] == '\x02' &&
                  EffectRangeBlockBytes[EffectRangePatchAddress - EffectRangeBlockAddress] == '\x32' &&
                  ShadowEffectsBlockBytes[ShadowEffectsPatchAddress - ShadowEffectsBlockAddress] == '\x0d' &&
                  ShadowEffectsBlockBytes[ShadowEffectsPatchAddress - ShadowEffectsBlockAddress + 3] == '\x04',
              "The patches replace the intended instructions");

