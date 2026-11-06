
#pragma once
#include <Windows.h>

struct BuildSpec {
    const wchar_t* file;
    const char* name;
    const char* profile;
    const char* signature;
    unsigned originalSize;
    const char* hash;
};

static const BuildSpec kSources[] = {
    {L"water_vanilla_medium.ps.hlsl", "3_ps_456_vanilla", "ps_3_0", "`d", 3784,
     "791db0c53672eb8984598edca4c3ef9f70a85115595833e611afdaa050865e0f"},
    {L"water_vanilla_high.ps.hlsl", "3_ps_457_vanilla", "ps_3_0", "ab<e", 6208,
     "896531bba7b40632f3b280d4bc4cae6bce93411acc3261ed2eba9da19f3eb5c2"},
};

