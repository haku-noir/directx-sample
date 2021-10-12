#include <Windows.h>
#include <tchar.h>
#include <vector>
#ifdef _DEBUG
    #include <iostream>
#endif // _DEBUG

using namespace std;

#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define WINDOW_CLASS _T("DX12Sample")
#define WINDOW_TITLE _T("DX12ƒeƒXƒg")
#define	WINDOW_STYLE WS_OVERLAPPEDWINDOW
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;
ID3D12Fence* _fence = nullptr;
UINT _fenceVal = 0;

D3D_FEATURE_LEVEL levels[] = {
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
};

#define ASSERT_RES(res, s) \
    do{ \
        if (res != S_OK) { \
            cout << "Erorr of res at " << s << endl; \
            return -1; \
        } \
    }while(0)
#define ASSERT_PTR(ptr, s) \
    do{ \
        if (ptr == nullptr) { \
            cout << "Erorr of ptr at " << s << endl; \
            return -1; \
        } \
    }while(0)

void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    auto res = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    debugLayer->EnableDebugLayer();
    debugLayer->Release();
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif // _DEBUG
    WNDCLASSEX w = {};
    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProcedure;
    w.lpszClassName = WINDOW_CLASS;
    w.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&w);

    RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wrc, WINDOW_STYLE, false);

    HWND hwnd = CreateWindow(
        w.lpszClassName,
        WINDOW_TITLE,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        w.hInstance,
        nullptr
    );

#ifdef _DEBUG
    EnableDebugLayer();
    auto res = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
    auto res = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif // _DEBUG

    std::vector<IDXGIAdapter*> adapters;
    IDXGIAdapter* tmpAdapter = nullptr;
    for(int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i){
        adapters.push_back(tmpAdapter);
    }

    for (auto adpt : adapters) {
        DXGI_ADAPTER_DESC adesc = {};
        adpt->GetDesc(&adesc);
        std::wstring strDesc = adesc.Description;

        if (strDesc.find(L"NVIDIA") != std::string::npos) {
            tmpAdapter = adpt;
            break;
        }
    }

    D3D_FEATURE_LEVEL featureLevel;
    for (auto lv : levels) {
        if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
            featureLevel = lv;
            break;
        }
    }
    ASSERT_PTR(_dev, "D3D12CreateDevice");

    res = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
    ASSERT_RES(res, "CreateCommandAllocator");
    res = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
    ASSERT_RES(res, "CreateCommandList");

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQueueDesc.NodeMask = 0;
    cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    res = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
    ASSERT_RES(res, "CreateCommandQueue");

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = WINDOW_WIDTH;
    swapchainDesc.Height = WINDOW_HEIGHT;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.Stereo = false;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
    swapchainDesc.BufferCount = 2;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    res = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);
    ASSERT_RES(res, "CreateSwapChainForHwnd");

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NodeMask = 0;
    heapDesc.NumDescriptors = 2;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ID3D12DescriptorHeap* rtvHeaps = nullptr;
    res = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
    ASSERT_RES(res, "CreateDescriptorHeap");

    DXGI_SWAP_CHAIN_DESC swcDesc = {};
    res = _swapchain->GetDesc(&swcDesc);
    ASSERT_RES(res, "GetDesc");
    const UINT descHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < swcDesc.BufferCount; ++i) {
        res = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
        ASSERT_RES(res, "GetBuffer");
        _dev->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
        handle.ptr += descHeapSize;
    }

    res = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    ASSERT_RES(res, "CreateFence");

    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
        auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr += bbIdx * descHeapSize;
        _cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

        float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
        _cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
        _cmdList->Close();

        ID3D12CommandList* cmdlists[] = { _cmdList };
        _cmdQueue->ExecuteCommandLists(1, cmdlists);

        _cmdQueue->Signal(_fence, ++_fenceVal);
        if (_fence->GetCompletedValue() != _fenceVal) {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            _fence->SetEventOnCompletion(_fenceVal, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        _cmdAllocator->Reset();
        _cmdList->Reset(_cmdAllocator, nullptr);

        _swapchain->Present(1, 0);
    }
    UnregisterClass(w.lpszClassName, w.hInstance);

    return 0;
}