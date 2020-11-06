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

UINT CalcConstantBufferByteSize(UINT bytesize)
{
    //256 倍数
    return (bytesize + 255) & ~255;
}

//------------------uploadbuffer helper class
template<typename T>
class UploadBuffer
{
    public:
        UploadBuffer(ID3D12Device* device, UINT element_count, bool is_constant_buffer):
        is_constant_buffer(is_constant_buffer)
        {
            element_bytesize = sizeof(T);
            if(is_constant_buffer)
            {
                element_bytesize = CalcConstantBufferByteSize(sizeof(T));
            }

            const auto& heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto& resource_desc = CD3DX12_RESOURCE_DESC::Buffer(element_bytesize);
            ThrowIfFailed(device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IDD_PPV_ARGS(upload_buffer.GetAddressof())
            ));

            ThrowIfFailed(upload_buffer->Map(
                0,
                nullptr,
                reinterpret_cast<void**>(&upload_buffer)
            ));
        }

        UploadBuffer(const UploadBuffer& rhs) = delete;
        UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

        ~UploadBuffer()
        {
            if(upload_buffer != nullptr)
            {
                upload_buffer->Unmap(0, nullptr);
            }

            mapped_data = nullptr;
        }

        ID3D12Resource* Resource() const
        {
            return upload_buffer.Get();
        }

        void CopyData(int element_index, const T& data)
        {
            memcpy(&mapped_data[element_index * element_bytesize], &data, sizeof(T));
        }
    private:
        ComPtr<ID3D12Resource> upload_buffer;
        BYTE* mapped_data = nullptr;
        UINT element_bytesize = 0;
        bool is_constant_buffer = false;
};