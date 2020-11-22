338

## 概念

1. ID3D12Device 虚拟设备
2. IDXGIFactory 用于创建DXGI对象，IDXGISwapChain
3. IDXGISwapChain 缓冲区
4. ID3D12Resource 资源
5. ID3D12Fence cpu/gpu同步接口
6. ID3D12CommandQueue 渲染命令队列
7. ID3D12CommandAllocator 渲染命令内存管理
8. ID3D12GraphicsCommandList 渲染命令记录，使用Id3d12CommandAllocator申请内存，通过execute.... 提交命令到ID3D12CommandQueue中
9. ID3D12DescriptorHeap Descriptor/resource view 资源描述保存的地方
10. ID3D12RootSignature shader需要的资源描述
11. ID3DBlob 资源内存块
12. D3D12_INPUT_ELEMENT_DESC
13. ID3D12PipelineState pipelinestate  objet
