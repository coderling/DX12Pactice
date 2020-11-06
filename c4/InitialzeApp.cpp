#include "../Common/D3DApp.h"

#include <DirectXColors.h>

using namespace DirectX;

class InitialzeApp : public D3DApp
{
    public:
        InitialzeApp(HINSTANCE instance);
        ~InitialzeApp();

        bool Initialize() override;

        void Update() override;

        void Draw() override;
};

InitialzeApp::InitialzeApp(HINSTANCE instance): D3DApp(instance){}

InitialzeApp::~InitialzeApp(){}

bool InitialzeApp::Initialize()
{
    return D3DApp::Initialize();
}

void InitialzeApp::Update()
{

}

void InitialzeApp::Draw()
{
    //重用command 内存
    ThrowIfFailed(command_allocator->Reset());

    //当指令提交到comond_queue后（ExecuteCommandList），可以进行重置
    ThrowIfFailed(command_list->Reset(command_allocator.Get(), nullptr));

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackbuffer(),
                                                        D3D12_RESOURCE_STATE_PRESENT,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);

    command_list->ResourceBarrier(1, &barrier);

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);

    const auto& backbuffer_view = CurrentBackbufferView();
    const auto& depthstencil_view = DepthStencilBufferView();
    command_list->ClearRenderTargetView(backbuffer_view, Colors::LightSteelBlue, 0, nullptr);
    command_list->ClearDepthStencilView(depthstencil_view,
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.0f, 0, 0, nullptr);
    
    command_list->OMSetRenderTargets(1, &backbuffer_view, true, &depthstencil_view);
    
    auto present_barrier =  CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackbuffer(),
                                                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                 D3D12_RESOURCE_STATE_PRESENT);
    command_list->ResourceBarrier(1, &present_barrier);

    command_list->Close();

    ID3D12CommandList* cmd_list[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(_countof(cmd_list), cmd_list);
    ThrowIfFailed(swapchain->Present(0, 0)); 
    current_backbuffer_index = (current_backbuffer_index + 1) % swap_buffer_count;

    
    FlushCommandQueue();
}

int main(int argc, char** argv)
{
    #if defined (DEBUG) || defined (_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    try
    {
        InitialzeApp app(GetModuleHandle(NULL));
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


