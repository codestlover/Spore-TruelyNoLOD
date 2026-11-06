
#include <Windows.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include "ShaderSources.hpp"

class ShaderInclude final : public ID3DInclude {
public:
    explicit ShaderInclude(const wchar_t* directory) : folder(directory) {}

    HRESULT __stdcall Open(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* data, UINT* bytes) noexcept override {
        wchar_t path[2048];
        swprintf_s(path, L"%s/%hs", folder, name);
        HANDLE file =
            CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return E_FAIL;
        DWORD size = GetFileSize(file, nullptr), read = 0;
        char* source = static_cast<char*>(HeapAlloc(GetProcessHeap(), 0, size));
        bool loaded = source && ReadFile(file, source, size, &read, nullptr) && read == size;
        CloseHandle(file);
        if (!loaded)
            return E_FAIL;
        *data = source;
        *bytes = size;
        return S_OK;
    }

    HRESULT __stdcall Close(LPCVOID data) noexcept override {
        HeapFree(GetProcessHeap(), 0, const_cast<void*>(data));
        return S_OK;
    }

private:
    const wchar_t* folder;
};

bool Write(const wchar_t* path, const void* bytes, DWORD size) {
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    bool result = WriteFile(f, bytes, size, &written, nullptr) && written == size;
    CloseHandle(f);
    return result;
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 4)
        return 2;
    HMODULE library = LoadLibraryW(argv[3]);
    if (!library)
        return 3;
    auto compile = reinterpret_cast<pD3DCompile>(GetProcAddress(library, "D3DCompile"));
    if (!compile)
        return 3;
    CreateDirectoryW(argv[2], nullptr);
    wchar_t path[2048];
    swprintf_s(path, L"%s/WaterShaders.hpp", argv[2]);
    FILE* header = nullptr;
    if (_wfopen_s(&header, path, L"wb") || !header)
        return 4;
    fprintf(header, "\n");
    unsigned sizes[_countof(kSources)]{};
    for (unsigned i = 0; i < _countof(kSources); ++i) {
        const auto& s = kSources[i];
        swprintf_s(path, L"%s/%s", argv[1], s.file);
        HANDLE file =
            CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            fclose(header);
            return 5;
        }
        DWORD size = GetFileSize(file, nullptr), read = 0;
        char* source = static_cast<char*>(HeapAlloc(GetProcessHeap(), 0, size));
        bool loaded = source && ReadFile(file, source, size, &read, nullptr) && read == size;
        CloseHandle(file);
        if (!loaded) {
            fclose(header);
            return 5;
        }
        ID3DBlob* code = nullptr;
        ID3DBlob* messages = nullptr;
        ShaderInclude include(argv[1]);
        HRESULT hr = compile(source, size, "TruelyNoLOD-Water.hlsl", nullptr, &include, "main", s.profile,
                             D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &messages);
        HeapFree(GetProcessHeap(), 0, source);
        if (messages) {
            swprintf_s(path, L"%s/%hs.compiler.txt", argv[2], s.name);
            Write(path, messages->GetBufferPointer(), DWORD(messages->GetBufferSize()));
            messages->Release();
        }
        if (FAILED(hr) || !code) {
            fclose(header);
            return 6;
        }
        sizes[i] = unsigned(code->GetBufferSize());
        swprintf_s(path, L"%s/%hs.bin", argv[2], s.name);
        if (!Write(path, code->GetBufferPointer(), sizes[i])) {
            code->Release();
            fclose(header);
            return 7;
        }
        fprintf(header, "static const DWORD code_%s[] = {", s.name);
        const DWORD* words = static_cast<const DWORD*>(code->GetBufferPointer());
        for (unsigned n = 0; n < sizes[i] / 4; ++n)
            fprintf(header, "%s%lu", n ? "," : "", words[n]);
        fprintf(header, "};\n");
        code->Release();
    }
    fprintf(header, "static const PixelCase kPixelCases[] = {\n");
    for (unsigned i = 0; i < _countof(kSources); ++i) {
        const auto& s = kSources[i];
        unsigned signatureSize = unsigned(strlen(s.signature)) + 1;
        if (strlen(s.hash) != 64) {
            fclose(header);
            return 8;
        }
        fprintf(header, "{\"%s\",{", s.name);
        for (unsigned j = 0; j < signatureSize; ++j)
            fprintf(header, "%s%u", j ? "," : "", unsigned(static_cast<unsigned char>(s.signature[j])));
        fprintf(header, "},%u,%u,{", signatureSize, s.originalSize);
        for (unsigned j = 0; j < 32; ++j) {
            unsigned byte = 0;
            if (sscanf_s(s.hash + 2 * j, "%2x", &byte) != 1) {
                fclose(header);
                return 8;
            }
            fprintf(header, "%s%u", j ? "," : "", byte);
        }
        fprintf(header, "},code_%s,%u},\n", s.name, sizes[i]);
    }
    fprintf(header, "};\n\n");
    bool saved = fclose(header) == 0;
    FreeLibrary(library);
    return saved ? 0 : 7;
}

