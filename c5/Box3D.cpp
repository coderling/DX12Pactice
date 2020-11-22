#include <DirectXColors.h>
#include <DirectXPackedVector.h>
#include <array>

#include "../Common/D3DApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

struct VPositionData
{
    XMFLOAT3 pos;
};

struct VColorData
{
    XMFLOAT4 color;
};

struct ObjectConstants
{
    XMFLOAT4X4 world_view_proj = MathHelper::Identity4x4();
    float gtime;
};

class Box3D : public D3DApp
{
    public:
        Box3D(HINSTANCE instance);
        ~Box3D();

        Box3D(const Box3D& rhs) = delete;
        Box3D& operator=(const Box3D& rhs) = delete;

        bool Initialize() override;

    private:
        std::unique_ptr<UploadBuffer<ObjectConstants>> object_upload_buffer;
        ComPtr<ID3D12DescriptorHeap> cbv_heap = nullptr;
        ComPtr<ID3D12RootSignature> root_signature = nullptr;

        std::unique_ptr<MeshGeometry> box_geometry = nullptr;

        ComPtr<ID3DBlob> mvs_bytecode = nullptr;
        ComPtr<ID3DBlob> mps_bytecode = nullptr;

        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layouts;

        ComPtr<ID3D12PipelineState> pso = nullptr;

        XMFLOAT4X4 world = MathHelper::Identity4x4();
        XMFLOAT4X4 view = MathHelper::Identity4x4();
        XMFLOAT4X4 proj = MathHelper::Identity4x4();

        float theta = 1.5f * XM_PI;
        float phi = XM_PIDIV4;
        float radius = 5.0f;

        POINT last_mouse_pos;

    private:
        void Update() override;

        void Draw() override;
        
        void OnResize() override;

        void OnMouseMove(WPARAM btn_state, int x, int y) override;
        void OnMouseUp(WPARAM btn_state, int x, int y) override;
        void OnMouseDown(WPARAM btn_state, int x, int y) override;
    
        void BuildDescriptorHeaps();
        void BuildConstantBuffers();
        void BuildRootSignature();
        void BuildShaderAndInputLayout();
        void BuildPSO();
        void BuildBoxGeometry();
};


Box3D::Box3D(HINSTANCE instance): D3DApp(instance){
    caption = L"Box3D";
}

Box3D::~Box3D(){}

bool Box3D::Initialize()
{
    if(!D3DApp::Initialize())
        return false;
    
    ThrowIfFailed(command_list->Reset(command_allocator.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShaderAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    ThrowIfFailed(command_list->Close());
    ID3D12CommandList* cmds[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(_countof(cmds), cmds);

    FlushCommandQueue();

    return true;
}

void Box3D::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX p = XMMatrixPerspectiveFovLH(0.25 * MathHelper::PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&proj, p);
}

void Box3D::Update()
{
    XMVECTOR pos = MathHelper::SphericalToCartesian(radius, theta, phi);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&this->view, view);

    XMMATRIX world = XMLoadFloat4x4(&this->world);
    XMMATRIX proj = XMLoadFloat4x4(&this->proj);
    XMMATRIX world_view_proj = world * view * proj;

    ObjectConstants objconstants;
    XMStoreFloat4x4(&objconstants.world_view_proj, XMMatrixTranspose(world_view_proj));
    objconstants.gtime = timer.TotalTime();
    object_upload_buffer->CopyData(0, objconstants);
}

void Box3D::Draw()
{
    // 通过reset重用记录命令的内存
    ThrowIfFailed(command_allocator->Reset());

    // command list 可以在命令提交到command_queue （执行ExecuteCommandList）后进行Reset操作
    // 重用command list 和内存
    ThrowIfFailed(command_list->Reset(command_allocator.Get(), pso.Get()));

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);

    // 改成当前backbuffer的状态
    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                            CurrentBackbuffer(),
                            D3D12_RESOURCE_STATE_PRESENT,
                            D3D12_RESOURCE_STATE_RENDER_TARGET);

    command_list->ResourceBarrier(1, &barrier);

    const auto& back_buffer_view = CurrentBackbufferView();
    const auto& depth_stencil_view = DepthStencilBufferView();
    // 清理backbuffer depth stencil buffer
    command_list->ClearRenderTargetView(
                    back_buffer_view,
                    Colors::LightSteelBlue,
                    0,
                    nullptr);
    command_list->ClearDepthStencilView(
                    depth_stencil_view,
                    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                    1.0f,
                    0,
                    0,
                    nullptr);

    // 设置rendertarget
    command_list->OMSetRenderTargets(1, &back_buffer_view, true, &depth_stencil_view);

    ID3D12DescriptorHeap* descriptor_heaps[] = { cbv_heap.Get() };
    command_list->SetDescriptorHeaps(_countof(descriptor_heaps), cbv_heap.GetAddressOf());

    // rootsignature 设置shader所需资源信息
    command_list->SetGraphicsRootSignature(root_signature.Get());

    D3D12_VERTEX_BUFFER_VIEW views[2];
    box_geometry->VertexBufferView(views);
    command_list->IASetVertexBuffers(0, 2, views); 
    const auto& index_buffer_view = box_geometry->IndexBufferView();
    command_list->IASetIndexBuffer(&index_buffer_view);
    command_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    command_list->SetGraphicsRootDescriptorTable(0, cbv_heap->GetGPUDescriptorHandleForHeapStart());

    command_list->DrawIndexedInstanced(
                    box_geometry->drawargs["box"].index_count,
                    1,
                    0,
                    0, 
                    0);
    
    //present buffer
    const auto& present_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                                    CurrentBackbuffer(),
                                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                                    D3D12_RESOURCE_STATE_PRESENT);
    command_list->ResourceBarrier(1, &present_barrier);

    //完成命令记录
    ThrowIfFailed(command_list->Close());

    ID3D12CommandList* cmdlist[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(_countof(cmdlist), cmdlist);

    //交换backbuffer
    ThrowIfFailed(swapchain->Present(0, 0));

    current_backbuffer_index = (current_backbuffer_index + 1) % swap_buffer_count;

    FlushCommandQueue();

}

void Box3D::OnMouseDown(WPARAM btn_state, int x, int y)
{
    last_mouse_pos.x = x;
    last_mouse_pos.y = y;
    SetCapture(hwnd);
}

void Box3D::OnMouseUp(WPARAM btn_state, int x, int y)
{
    ReleaseCapture();
}

void Box3D::OnMouseMove(WPARAM btn_state, int x, int y)
{
    if((btn_state & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - last_mouse_pos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - last_mouse_pos.y));

        theta += dx;
        phi += dy;
        phi = MathHelper::Clamp(phi, 0.1f, MathHelper::PI - 0.1f);
    }
    else if((btn_state & MK_RBUTTON) != 0)
    {
        float dx = (0.005f * static_cast<float>(x - last_mouse_pos.x));
        float dy = (0.005f * static_cast<float>(y - last_mouse_pos.y));
        radius += dx - dy; 

        radius = MathHelper::Clamp(radius, 3.0f, 15.0f);
    }

    last_mouse_pos.x = x;
    last_mouse_pos.y = y;
}

void Box3D::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc;
    cbv_heap_desc.NumDescriptors =  1;
    cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbv_heap_desc.NodeMask = 0;

    ThrowIfFailed(device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&cbv_heap)));
}

void Box3D::BuildConstantBuffers()
{
    object_upload_buffer = std::make_unique<UploadBuffer<ObjectConstants>>(device.Get(), 1, true);

    UINT obj_cbb_bytesize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
    
    D3D12_GPU_VIRTUAL_ADDRESS cb_address = object_upload_buffer->Resource()->GetGPUVirtualAddress();
    int index = 0;
    cb_address += index * obj_cbb_bytesize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = cb_address;
    cbv_desc.SizeInBytes = obj_cbb_bytesize;

    device->CreateConstantBufferView(&cbv_desc, cbv_heap->GetCPUDescriptorHandleForHeapStart());
}
        
void Box3D::BuildRootSignature()
{
    // 定义shader程序需要什么杨的输入资源
    // 输入的资源就好像函数参数，root signature就好比函数签名

    CD3DX12_ROOT_PARAMETER slot_root_parameter[1];

    CD3DX12_DESCRIPTOR_RANGE cbv_table;
    cbv_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slot_root_parameter[0].InitAsDescriptorTable(1, &cbv_table);

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(1, slot_root_parameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    ComPtr<ID3DBlob> serialize_root_signature = nullptr;
    ComPtr<ID3DBlob> error_blob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        serialize_root_signature.GetAddressOf(),
        error_blob.GetAddressOf());
    
    if(error_blob != nullptr)
    {
        OutputDebugStringA((char*)error_blob->GetBufferPointer());
    }

    ThrowIfFailed(hr);

    ThrowIfFailed(device->CreateRootSignature(
        0,
        serialize_root_signature->GetBufferPointer(),
        serialize_root_signature->GetBufferSize(),
        IID_PPV_ARGS(&root_signature)));
}
    
void Box3D::BuildShaderAndInputLayout()
{
    mvs_bytecode = CompileShader(
                    L"c5/Shaders/color.hlsl",
                    nullptr,
                    "VS",
                    "vs_5_0");
    mps_bytecode = CompileShader(
                    L"c5/Shaders/color.hlsl",
                    nullptr,
                    "PS",
                    "ps_5_0");
    
    input_layouts = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void Box3D::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
    ZeroMemory(&pso_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    pso_desc.InputLayout.pInputElementDescs = input_layouts.data();
    pso_desc.InputLayout.NumElements = (UINT)input_layouts.size();

    pso_desc.pRootSignature = root_signature.Get();
    pso_desc.VS.pShaderBytecode = reinterpret_cast<BYTE*>(mvs_bytecode->GetBufferPointer());
    pso_desc.VS.BytecodeLength = mvs_bytecode->GetBufferSize();

    pso_desc.PS.pShaderBytecode = reinterpret_cast<BYTE*>(mps_bytecode->GetBufferPointer());
    pso_desc.PS.BytecodeLength = mps_bytecode->GetBufferSize();

    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    //pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = backbuffer_format;
    
    pso_desc.SampleDesc.Count = enable_4x_msaa ? 4 : 1;
    pso_desc.SampleDesc.Quality = enable_4x_msaa ? (msaa_4x_quality - 1) : 0;
    
    pso_desc.DSVFormat = depth_stencil_format;

    ThrowIfFailed(device->CreateGraphicsPipelineState(
        &pso_desc,
        IID_PPV_ARGS(pso.GetAddressOf())));
}

void Box3D::BuildBoxGeometry()
{
    std::array<VPositionData, 8> vertices = {
        VPositionData({XMFLOAT3(-1.0f, -1.0f, -1.0f)}), 
        VPositionData({XMFLOAT3(-1.0f, +1.0f, -1.0f)}), 
        VPositionData({XMFLOAT3(+1.0f, +1.0f, -1.0f)}), 
        VPositionData({XMFLOAT3(+1.0f, -1.0f, -1.0f)}), 
        VPositionData({XMFLOAT3(-1.0f, -1.0f, +1.0f)}), 
        VPositionData({XMFLOAT3(-1.0f, +1.0f, +1.0f)}), 
        VPositionData({XMFLOAT3(+1.0f, +1.0f, +1.0f)}), 
        VPositionData({XMFLOAT3(+1.0f, -1.0f, +1.0f)})
    };

    std::array<VColorData, 8> colors = {
        VColorData({XMFLOAT4(Colors::White)}),
        VColorData({XMFLOAT4(Colors::Black)}),
        VColorData({XMFLOAT4(Colors::Red)}),
        VColorData({XMFLOAT4(Colors::Green)}),
        VColorData({XMFLOAT4(Colors::Blue)}),
        VColorData({XMFLOAT4(Colors::Yellow)}),
        VColorData({XMFLOAT4(Colors::Cyan)}),
        VColorData({XMFLOAT4(Colors::Magenta)})
    };

    std::array<std::uint16_t, 36> indices = {
        // front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
    };

    const UINT vb_bytesize = (UINT)vertices.size() * sizeof(VPositionData);
    const UINT vb_color_bytesize = (UINT)colors.size() * sizeof(VColorData);
    const UINT ib_bytesize = (UINT)indices.size() * sizeof(std::uint16_t);

    box_geometry = std::make_unique<MeshGeometry>();
    box_geometry->name = "box";

    ThrowIfFailed(D3DCreateBlob(vb_bytesize, box_geometry->vertex_buffer_cpu.GetAddressOf()));
    CopyMemory(box_geometry->vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vb_bytesize);
    
    ThrowIfFailed(D3DCreateBlob(vb_color_bytesize, box_geometry->vertex_color_buffer_cpu.GetAddressOf()));
    CopyMemory(box_geometry->vertex_color_buffer_cpu->GetBufferPointer(), colors.data(), vb_color_bytesize);

    ThrowIfFailed(D3DCreateBlob(ib_bytesize, box_geometry->index_buffer_cpu.GetAddressOf()));
    CopyMemory(box_geometry->index_buffer_cpu->GetBufferPointer(), indices.data(), ib_bytesize);

    box_geometry->vertex_buffer_gpu = CreateDefaultBuffer(device.Get(), command_list.Get(), vertices.data(), vb_bytesize, box_geometry->vertex_buffer_uploader);
    box_geometry->vertex_color_buffer_gpu = CreateDefaultBuffer(device.Get(), command_list.Get(), colors.data(), vb_color_bytesize, box_geometry->vertex_buffer_uploader);
    box_geometry->index_buffer_gpu = CreateDefaultBuffer(device.Get(), command_list.Get(), indices.data(), ib_bytesize, box_geometry->index_buffer_uploader);

    box_geometry->vertex_byte_stride = sizeof(VPositionData);
    box_geometry->vertex_color_byte_stride= sizeof(VColorData);
    box_geometry->vertex_buffer_bytesize = vb_bytesize;
    box_geometry->vertex_color_byte_stride = vb_color_bytesize;
    box_geometry->index_format = DXGI_FORMAT_R16_UINT;
    box_geometry->index_buffer_bytesize = ib_bytesize;

    SubmeshGeometry submesh;
    submesh.index_count = (UINT)indices.size();
    submesh.start_index_location = 0;
    submesh.base_vertex_location = 0;
    box_geometry->drawargs["box"] = submesh;
}

int main(int argc, char** argv)
{
    #if defined (DEBUG) || defined (_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    try
    {
        Box3D app(GetModuleHandle(NULL));
        if(!app.Initialize())
            return 0;
        
        return app.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR FAILED", MB_OK);
        return 0;
    }
}