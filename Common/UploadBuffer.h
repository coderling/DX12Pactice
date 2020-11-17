#pragma once

#include "d3dx12.h"
#include "Util.h"

using namespace Microsoft::WRL;

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
                IID_PPV_ARGS(upload_buffer.GetAddressOf())
            ));

            ThrowIfFailed(upload_buffer->Map(
                0,
                nullptr,
                reinterpret_cast<void**>(&mapped_data)
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