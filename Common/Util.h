#pragma once

#include <string>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXCollision.h>

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

UINT CalcConstantBufferByteSize(UINT bytesize);


ComPtr<ID3DBlob> CompileShader(const std::wstring& filename,
                               const D3D_SHADER_MACRO* defines,
                               const std::string& entrypoint,
                               const std::string& target);

ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

//-----------------------------MeshGeometry--------------------------------
struct SubmeshGeometry
{
    UINT index_count = 0;
    UINT start_index_location = 0;
    INT base_vertex_location =0;

    DirectX::BoundingBox bounds;
};

struct MeshGeometry
{
    std::string name;

    ComPtr<ID3DBlob> vertex_buffer_cpu = nullptr;
    ComPtr<ID3DBlob> index_buffer_cpu = nullptr;

    ComPtr<ID3D12Resource> vertex_buffer_gpu = nullptr;
    ComPtr<ID3D12Resource> index_buffer_gpu = nullptr;

    ComPtr<ID3D12Resource> vertex_buffer_uploader = nullptr;
    ComPtr<ID3D12Resource> index_buffer_uploader = nullptr;

    UINT vertex_byte_stride = 0;
    UINT vertex_buffer_bytesize = 0;
    DXGI_FORMAT index_format = DXGI_FORMAT_R16_UINT;
    UINT index_buffer_bytesize = 0;

    std::unordered_map<std::string, SubmeshGeometry> drawargs;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertex_buffer_gpu->GetGPUVirtualAddress();
        vbv.StrideInBytes = vertex_byte_stride;
        vbv.SizeInBytes = vertex_buffer_bytesize;

        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = index_buffer_gpu->GetGPUVirtualAddress();
        ibv.Format = index_format;
        ibv.SizeInBytes = index_buffer_bytesize;

        return ibv;
    }

    void DisposeUploaders()
    {
        vertex_buffer_uploader = nullptr;
        index_buffer_uploader = nullptr;
    }
};