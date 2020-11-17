#include <comdef.h>

#include "Util.h"

DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number)
:error_code(hr), function_name(function_name), filename(filename), line_number(line_number)
{
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(error_code);
    std::wstring msg = err.ErrorMessage();

    return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
}


//-------------------------------------util-----------------------------------
ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* command_list,
    const void* init_data,
    UINT64 byte_size,
    ComPtr<ID3D12Resource>& upload_buffer
)
{
    // 创建bufffer
    ComPtr<ID3D12Resource> default_buffer;
    const auto& properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const auto& desc = CD3DX12_RESOURCE_DESC::Buffer(byte_size);
    ThrowIfFailed(device->CreateCommittedResource(
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(default_buffer.GetAddressOf())));

    // 创建upload buffer上传cpu数据到buffer
    const auto& properties_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(device->CreateCommittedResource(
        &properties_upload,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(upload_buffer.GetAddressOf())));
    
    // 数据描述
    D3D12_SUBRESOURCE_DATA subresource_data = {};
    subresource_data.pData = init_data;
    subresource_data.RowPitch = byte_size;
    subresource_data.SlicePitch = subresource_data.RowPitch;

    // 将buffer状态变成copy destation, 等待upload
    const auto& trans = CD3DX12_RESOURCE_BARRIER::Transition(
        default_buffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST);
    
    command_list->ResourceBarrier(1, &trans);

    // 上传数据到GPU
    UpdateSubresources<1>(
        command_list,
        default_buffer.Get(),
        upload_buffer.Get(),
        0,
        0,
        1,
        &subresource_data);

    // 将buffer状态变成读取
    const auto& cp_dst_to_read = CD3DX12_RESOURCE_BARRIER::Transition(
        default_buffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    command_list->ResourceBarrier(1, &cp_dst_to_read);

    
    return default_buffer;
}

UINT CalcConstantBufferByteSize(UINT bytesize)
{
    //256 倍数
    return (bytesize + 255) & ~255;
}

ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ComPtr<ID3DBlob> CompileShader(const std::wstring& filename,
                               const D3D_SHADER_MACRO* defines,
                               const std::string& entrypoint,
                               const std::string& target)
{
    UINT compile_flags = 0;
    #if defined (DEBUG) || defined(_DEBUG)
    compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byte_code = nullptr;
    ComPtr<ID3DBlob> errors;

    hr = D3DCompileFromFile(
        filename.c_str(),
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(),
        target.c_str(),
        compile_flags,
        0,
        byte_code.GetAddressOf(),
        errors.GetAddressOf());
    
    if(errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());
    
    ThrowIfFailed(hr);

    return byte_code;
}