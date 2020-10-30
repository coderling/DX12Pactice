#include <cassert>
#include <windowsx.h>
#include <iostream>
#include "D3DApp.h"

LRESULT CALLBACK 
MainWndProc(HWND hwnd, UINT msg, WPARAM param, LPARAM l_param)
{
    return D3DApp::GetApp()->MsgProc(hwnd, msg, param, l_param);
}

D3DApp* D3DApp::app = nullptr;
D3DApp* D3DApp::GetApp()
{
    return app;
}


D3DApp::~D3DApp()
{

}

D3DApp::D3DApp(HINSTANCE instance): hinstance(instance)
{
    app = this;
}

bool D3DApp::Initialize()
{
    if(!InitWindow())
        return false;
    
    if(!InitDirect3D())
        return false;

    OnResize();
    return true;
}

bool D3DApp::InitWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName= 0;
    wc.lpszClassName = L"D3D12Practice";

    if(!RegisterClass(&wc))
    {
        MessageBox(0, L"Register class failed ", 0, 0);
        return false;
    }

    RECT rect = {0, 0, window_width, window_height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;

    hwnd = CreateWindow(wc.lpszClassName, caption.c_str(),
           WS_OVERLAPPEDWINDOW, 0, 0, w, h, NULL, NULL, hinstance, 0);

    if(!hwnd)
    {
        MessageBox(0, L"Create window failed.", 0, 0);
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return true;
}

void D3DApp::OnResize()
{
    assert(device);
    assert(swapchain);
    assert(command_allocator);

    FlushCommandQueue();

    ThrowIfFailed(command_list->Reset(command_allocator.Get(), nullptr));
    for(int i = 0; i < swap_buffer_count; ++i)
    {
        swapchain_buffers[i].Reset();
    }
    depth_stencil_buffer.Reset();

    ThrowIfFailed(swapchain->ResizeBuffers(swap_buffer_count,
                                           window_width,
                                           window_height,
                                           backbuffer_format,
                                           DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    current_backbuffer_index = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for(UINT i = 0; i < swap_buffer_count; ++i)
    {
        ThrowIfFailed(swapchain->GetBuffer(i, IID_PPV_ARGS(swapchain_buffers[i].GetAddressOf())));
        device->CreateRenderTargetView(swapchain_buffers[i].Get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, RTVDescriptor_size);
    }

    // depth&stencil buffer
    D3D12_RESOURCE_DESC depth_stencil_desc;
    depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_stencil_desc.Width = window_width;
    depth_stencil_desc.Height = window_height;
    depth_stencil_desc.DepthOrArraySize = 1;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.Alignment = 0;

    // ssao read SRV 需要格式DXGI_FORMAT_R24G8_TYPELESS
    // 1. SRV format: DXGI_FORMAT_R24G8_TYPELESS
    // 2. DSV format:DXGI_FORMAT_D24_UNORM_S8_UINT
    depth_stencil_desc.Format =DXGI_FORMAT_R24G8_TYPELESS;
    
    depth_stencil_desc.SampleDesc.Count = enable_4x_msaa ? 4 : 1;
    depth_stencil_desc.SampleDesc.Quality = enable_4x_msaa ? msaa_4x_quality - 1 : 0;
    depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE opt_clear;
    opt_clear.Format = depth_stcenil_format;
    opt_clear.DepthStencil.Depth = 1.0f;
    opt_clear.DepthStencil.Stencil = 0;
    CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
                          &properties,
                          D3D12_HEAP_FLAG_NONE,
                          &depth_stencil_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear, IID_PPV_ARGS(depth_stencil_buffer.GetAddressOf())));

    // depth stencil view
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Format = depth_stcenil_format;
    dsv_desc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(depth_stencil_buffer.Get(), &dsv_desc, DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 通知GPU同步资源状态
    auto trans = CD3DX12_RESOURCE_BARRIER::Transition(depth_stencil_buffer.Get(),
                                 D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    command_list->ResourceBarrier(1, &trans);
    // close 表示command_list完成了记录
    command_list->Close();
    ID3D12CommandList* cmds_list[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(_countof(cmds_list), cmds_list);

    //等待指令完成
    FlushCommandQueue();

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(window_width);
    viewport.Height = static_cast<float>(window_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    scissor_rect = {0, 0, window_width, window_height};
}

bool D3DApp::InitDirect3D()
{
    // debug 模式下开始日志信息输出
    #if defined (DEBUG) || defined (_DEBUG)
    ComPtr<ID3D12Debug> debug;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
    debug->EnableDebugLayer();
    #endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

    // 创建设备
    HRESULT hardware_result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    // 如果失败，software adapter
    if(FAILED(hardware_result))
    {
        ComPtr<IDXGIAdapter> wrap_adapter;
        ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&wrap_adapter)));

        ThrowIfFailed(D3D12CreateDevice(wrap_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
    }

    //cpu gpu 同步
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    //获取资源描述结构体便宜字节
    RTVDescriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DSVDescriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CB_SR_UA_VDescriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //检查 4X MSAA 支持
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels;
    ms_quality_levels.Format = backbuffer_format;
    ms_quality_levels.SampleCount = 4;
    ms_quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    ms_quality_levels.NumQualityLevels = 0;
    ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                              &ms_quality_levels,
                                              sizeof(ms_quality_levels)));
    msaa_4x_quality = ms_quality_levels.NumQualityLevels;

    assert(msaa_4x_quality > 0 && "Unexpected msaa quality level.");

    #if _DEBUG
    LogAdapters();
    #endif
    
    CreateCommandObjects();
    CreateSwapChain();
    CreateRTV_DSV_DescriptorHeap();


    return true;
}

void D3DApp::FlushCommandQueue()
{
    current_fence++;
    ThrowIfFailed(command_queue->Signal(fence.Get(), current_fence));
    if(fence->GetCompletedValue() < current_fence)
    {
        HANDLE evt_handle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence->SetEventOnCompletion(current_fence, evt_handle))
        WaitForSingleObject(evt_handle, INFINITE);
        CloseHandle(evt_handle);
    }
}

void D3DApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(device->CreateCommandQueue(&queue_desc,
                                             IID_PPV_ARGS(command_queue.GetAddressOf())));

    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(command_allocator.GetAddressOf())));

    ThrowIfFailed(device->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            command_allocator.Get(),
                                            nullptr,
                                            IID_PPV_ARGS(command_list.GetAddressOf())));

    command_list->Close();
}

void D3DApp::CreateSwapChain()
{
    swapchain.Reset();

    DXGI_SWAP_CHAIN_DESC swap_desc = {};
    swap_desc.BufferDesc.Width = window_width;
    swap_desc.BufferDesc.Height = window_height;
    swap_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_desc.BufferDesc.Format = backbuffer_format;
    swap_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_desc.SampleDesc.Count = enable_4x_msaa ? 4 : 1;
    swap_desc.SampleDesc.Quality = enable_4x_msaa ? msaa_4x_quality - 1 : 0;

    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = swap_buffer_count;
    swap_desc.OutputWindow = hwnd;
    swap_desc.Windowed = true;
    
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(dxgi_factory->CreateSwapChain(command_queue.Get(), &swap_desc, swapchain.GetAddressOf()));

}

void D3DApp::CreateRTV_DSV_DescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
    rtv_heap_desc.NumDescriptors = swap_buffer_count;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_heap_desc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(RTVDescriptorHeap.GetAddressOf())));
    
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
    dsv_heap_desc.NumDescriptors = 1;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_heap_desc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(DSVDescriptorHeap.GetAddressOf())));

}
        
        void D3DApp::LogAdapters()
        {
            UINT i = 0;
            IDXGIAdapter* adapter = nullptr;
            std::vector<IDXGIAdapter*> adapters;
            while(dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC desc;
                adapter->GetDesc(&desc);

                std::wstring text = L"***Adapter: ";
                text += desc.Description;
                text += L"\n";

                OutputDebugString(text.c_str());

                adapters.push_back(adapter);

                ++i;
            }

            for(size_t i = 0; i < adapters.size(); ++i)
            {
                LogAdapterOutputs(adapters[i]);
                ReleaseCom(adapters[i]);
            }
        }
        
        void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
        {
            UINT i = 0;
            IDXGIOutput* output = nullptr;
            while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_OUTPUT_DESC desc;
                output->GetDesc(&desc);
                std::wstring text = L"***Output: ";
                text += desc.DeviceName;
                text += L"\n";

                OutputDebugString(text.c_str());

                LogOutputDisplayModes(output, backbuffer_format);
                ReleaseCom(output);
                ++i;
            }
        }
        
        void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
        {
            UINT count = 0;
            UINT flags = 0;

            output->GetDisplayModeList(format, flags, &count,   nullptr);
            std::vector<DXGI_MODE_DESC> models(count);
            output->GetDisplayModeList(format, flags, &count, &models[0]);
            std::cout << models.size() << std::endl;
            for(auto& x : models)
            {
                UINT n = x.RefreshRate.Numerator;
                UINT d = x.RefreshRate.Denominator;

                std::wstring text = L"Width: " + std::to_wstring(x.Width) + L" " + 
                                    L"Height: " + std::to_wstring(x.Height) + L" " + 
                                    L"Refresh: " + std::to_wstring(n) + L"/" + std::to_wstring(d) + L"\n";
                
                OutputDebugString(text.c_str());
            }
        }


int D3DApp::Run()
{
    MSG msg = {0};

    timer.Reset();

    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }   
        else
        {
            timer.Tick();
            if(!paused)
            {
                CalculateFrameStats();
                Update();
                Draw();
            }
            else
            {
                Sleep(100);
            }
        }
    }

    return (int)msg.wParam;
}

void D3DApp::CalculateFrameStats()
{
    static int frame_count = 0;
    static float time_elapsed = 0;
    frame_count++;

    if(timer.TotalTime() - time_elapsed >= 1.f)
    {
        float fps = frame_count;
        float mspf = 1000.0f / fps;

        std::wstring fps_str = std::to_wstring(fps);
        std::wstring mspf_str = std::to_wstring(mspf);

        std::wstring window_text = caption + L" FPS: " + fps_str + L" MSPF: " + mspf_str;
        
        SetWindowText(hwnd, window_text.c_str());

        frame_count = 0;
        time_elapsed += 1.0f;
    }
}
        
ID3D12Resource* D3DApp::CurrentBackbuffer() const
{
    return swapchain_buffers[current_backbuffer_index].Get();
}
        
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackbufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
            , current_backbuffer_index, RTVDescriptor_size);
}
        
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilBufferView() const
{
    return DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM param, LPARAM l_param)
{
    switch(msg)
    {
        case WM_ACTIVATE:
            if(LOWORD(param) == WA_ACTIVE)
            {
                paused = true;
                timer.Stop();
            }
            else
            {
                paused = false;
                timer.Start();
            }
            return 0;
        // when resize
        case WM_SIZE:
            window_width = LOWORD(l_param);
            window_height = HIWORD(l_param);
            if(device)
            {
                if(param == SIZE_MINIMIZED)
                {
                    minimized = true;
                    maximized = false;
                    paused = true;
                }
                else if(param == SIZE_MAXIMIZED)
                {
                    maximized = true;
                    minimized = false;
                    paused = false;
                    OnResize();
                }
                else if(param == SIZE_RESTORED)
                {
                    // 从最小化还原
                    if(minimized)
                    {
                        paused = false;
                        minimized = false;
                        OnResize();
                    }
                    else if(maximized)
                    {
                        paused = false;
                        maximized = false;
                        OnResize();
                    }
                    else if(resizing)
                    {
                        // 用户拖动改变窗口大小，会收到一系列的resize消息，在用户停止拖动的时候处理
                    }
                    else
                    {
                        OnResize();
                    }

                }
            }
            return 0;
        case WM_ENTERSIZEMOVE:
            paused = true;
            resizing = true;
            timer.Stop();
            return 0;
        case WM_EXITSIZEMOVE:
            paused = false;
            resizing = false;
            timer.Start();
            OnResize();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_MENUCHAR:
            return MAKELRESULT(0, MNC_CLOSE);
        case WM_GETMINMAXINFO:
            // 避免窗口变得过小
            ((MINMAXINFO*)l_param)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)l_param)->ptMinTrackSize.y = 200;
            return 0;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            OnMouseDown(param, GET_X_LPARAM(l_param), GET_X_LPARAM(l_param));
            return 0;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            OnMouseUp(param, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
            return 0;
        case WM_MOUSEMOVE:
            OnMouseMove(param, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
            return 0;
        case WM_KEYUP:
            if(param == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            else if((int)param == VK_F2)
            {
                enable_4x_msaa = !enable_4x_msaa;
            }
            return 0;
    }

    return DefWindowProc(hwnd, msg, param, l_param);
}


void D3DApp::OnMouseDown(WPARAM btn_state, int x, int y)
{

}

void D3DApp::OnMouseUp(WPARAM btn_state, int x, int y)
{

}

void D3DApp::OnMouseMove(WPARAM btn_state, int x, int y)
{

}