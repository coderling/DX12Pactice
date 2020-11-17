#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "d3dx12.h"
#include "Util.h"
#include "GameTimer.h"

using namespace Microsoft::WRL;

class D3DApp
{
    protected:
        virtual ~D3DApp();
        D3DApp(HINSTANCE hinstance);
        D3DApp(const D3DApp& rhs) = delete;
        D3DApp& operator=(const D3DApp& rhs) = delete;

    public:
        static D3DApp* GetApp();
        virtual bool Initialize();
        virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM param, LPARAM l_param);

        int Run();

    protected:
        bool InitWindow();
        bool InitDirect3D(); 
        void FlushCommandQueue();
        void CreateCommandObjects();
        void CreateSwapChain();

        void CalculateFrameStats();

        float AspectRatio() const;

        ID3D12Resource* CurrentBackbuffer() const;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackbufferView() const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferView() const;
        
        void LogAdapters();
        void LogAdapterOutputs(IDXGIAdapter* adapter);
        void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    protected:
        virtual void CreateRTV_DSV_DescriptorHeap();
        virtual void OnResize();
        virtual void Update() = 0;
        virtual void Draw() = 0;

        virtual void OnMouseDown(WPARAM btn_state, int x, int y);
        virtual void OnMouseUp(WPARAM btn_state, int x, int y);
        virtual void OnMouseMove(WPARAM btn_state, int x, int y);
    protected:
        static D3DApp* app;

        ComPtr<ID3D12Device> device;
        ComPtr<IDXGIFactory7> dxgi_factory;
        ComPtr<IDXGISwapChain> swapchain;
        static const UINT swap_buffer_count = 2;

        int current_backbuffer_index;
        ComPtr<ID3D12Resource> swapchain_buffers[swap_buffer_count];
        ComPtr<ID3D12Resource> depth_stencil_buffer;

        ComPtr<ID3D12Fence> fence;
        ComPtr<ID3D12CommandQueue> command_queue;
        ComPtr<ID3D12CommandAllocator> command_allocator;
        ComPtr<ID3D12GraphicsCommandList> command_list;

        UINT64 current_fence = 0;

        //UINT rendertarget_view_descriptor_size = 0;
        UINT RTVDescriptor_size = 0;
        //UINT depth_stcenil_view_descriptor_size = 0;
        UINT DSVDescriptor_size = 0;
        //w_descriptor_size = 0;
        UINT CB_SR_UA_VDescriptor_size = 0;

        ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
        ComPtr<ID3D12DescriptorHeap> DSVDescriptorHeap;


        // 4x msaa quality
        UINT msaa_4x_quality = 0;
        bool enable_4x_msaa = false;

        // back buffer 格式
        DXGI_FORMAT backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT depth_stencil_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        int window_width = 800;
        int window_height = 600;

        D3D12_VIEWPORT viewport;
        D3D12_RECT scissor_rect;

        // 窗口句柄
        HWND hwnd;
        HINSTANCE hinstance;
        std::wstring caption = L"D3D12Practice";

        // game loop
        GameTimer timer;
        bool paused = false;
        bool minimized = false;
        bool maximized = false;
        bool resizing = false;
};