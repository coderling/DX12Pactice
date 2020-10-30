#pragma once

#include <string>
#include <Windows.h>

#include <dxgi1_6.h>
#include "d3dx12.h"

using namespace Microsoft::WRL;

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException
{
    public:
        DxException() = default;
        DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number);
        std::wstring ToString()const;
        HRESULT error_code = S_OK;
        std::wstring function_name;
        std::wstring filename;
        int line_number = -1;
};


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)\
{\
    HRESULT hr__ = (x);\
    std::wstring wfn = AnsiToWString(__FILE__);\
    if(FAILED(hr__)){throw DxException(hr__, L#x, wfn, __LINE__);}\
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* command_list,
    const void* init_data,
    UINT64 byte_size,
    ComPtr<ID3D12Resource>& upload_buffer
);