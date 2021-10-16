#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <DirectXMath.h>
#ifdef _DEBUG
    #include <iostream>
#endif // _DEBUG

using namespace DirectX;

#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

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
            std::cout << "Erorr of res at " << s << std::endl; \
            return -1; \
        } \
    }while(0)
#define ASSERT_PTR(ptr, s) \
    do{ \
        if (ptr == nullptr) { \
            std::cout << "Erorr of ptr at " << s << std::endl; \
            return -1; \
        } \
    }while(0)

size_t AlignmentedSize(size_t size, size_t alignment) {
    return size + alignment - size % alignment;
}

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

    auto res = CoInitializeEx(0, COINIT_MULTITHREADED);
    ASSERT_RES(res, "CoInitializeEx");

#ifdef _DEBUG
    EnableDebugLayer();
    res = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
    res = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
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

    D3D12_DESCRIPTOR_HEAP_DESC rtvheapDesc = {};
    rtvheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvheapDesc.NodeMask = 0;
    rtvheapDesc.NumDescriptors = 2;
    rtvheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ID3D12DescriptorHeap* rtvHeap = nullptr;
    res = _dev->CreateDescriptorHeap(&rtvheapDesc, IID_PPV_ARGS(&rtvHeap));
    ASSERT_RES(res, "CreateDescriptorHeap");

    DXGI_SWAP_CHAIN_DESC swcDesc = {};
    res = _swapchain->GetDesc(&swcDesc);
    ASSERT_RES(res, "GetDesc");

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    const UINT rtvheapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int i = 0; i < swcDesc.BufferCount; ++i) {
        res = _swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
        ASSERT_RES(res, "GetBuffer");
        _dev->CreateRenderTargetView(backBuffers[i], &rtvDesc, handle);
        handle.ptr += rtvheapSize;
    }

    struct Vertex{
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 vu;
    };

    Vertex vertices[] = {
        {{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},
        {{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}},
    };

    D3D12_HEAP_PROPERTIES vertheapProp = {};
    vertheapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    vertheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    vertheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC vertresDesc = {};
    vertresDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertresDesc.Width = sizeof(vertices);
    vertresDesc.Height = 1;
    vertresDesc.DepthOrArraySize = 1;
    vertresDesc.MipLevels = 1;
    vertresDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertresDesc.SampleDesc.Count = 1;
    vertresDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    vertresDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* vertBuff = nullptr;
    res = _dev->CreateCommittedResource(&vertheapProp, D3D12_HEAP_FLAG_NONE, &vertresDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));
    ASSERT_RES(res, "CreateCommittedResource");

    Vertex* vertMap = nullptr;
    res = vertBuff->Map(0, nullptr, (void**)&vertMap);
    ASSERT_RES(res, "Map");
    std::copy(std::begin(vertices), std::end(vertices), vertMap);
    vertBuff->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
    vbView.SizeInBytes = sizeof(vertices);
    vbView.StrideInBytes = sizeof(vertices[0]);

    unsigned short indices[] = {
        0, 1, 2,
        2, 1, 3
    };

    ID3D12Resource* idxBuff = nullptr;
    vertresDesc.Width = sizeof(indices);
    res = _dev->CreateCommittedResource(&vertheapProp, D3D12_HEAP_FLAG_NONE, &vertresDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));
    ASSERT_RES(res, "CreateCommittedResource");

    unsigned short* idxMap = nullptr;
    res = idxBuff->Map(0, nullptr, (void**)&idxMap);
    std::copy(std::begin(indices), std::end(indices), idxMap);
    idxBuff->Unmap(0, nullptr);

    D3D12_INDEX_BUFFER_VIEW ibView = {};
    ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = sizeof(indices);

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    res = D3DCompileFromFile(
        L"BasicVertexShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "BasicVS", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &vsBlob, &errorBlob
    );
    if (FAILED(res)) {
        if (res == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            OutputDebugStringA("File not found");
        }
        else {
            std::string errstr;
            errstr.resize(errorBlob->GetBufferSize());
            std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
            OutputDebugStringA(errstr.c_str());
        }
    }

    res = D3DCompileFromFile(
        L"BasicPixelShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "BasicPS", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &psBlob, &errorBlob
    );
    if (FAILED(res)) {
        if (res == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            OutputDebugStringA("File not found");
        }
        else {
            std::string errstr;
            errstr.resize(errorBlob->GetBufferSize());
            std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
            OutputDebugStringA(errstr.c_str());
        }
    }

    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratchImg = {};

    res = DirectX::LoadFromWICFile(L"img/textest.png", DirectX::WIC_FLAGS_NONE, &metadata, scratchImg);
    ASSERT_RES(res, "LoadFromWICFile");
    auto img = scratchImg.GetImage(0, 0, 0);

    D3D12_HEAP_PROPERTIES texuploadheapProp = {};
    texuploadheapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    texuploadheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    texuploadheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    texuploadheapProp.CreationNodeMask = 0;
    texuploadheapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC texresDesc = {};
    texresDesc.Format = DXGI_FORMAT_UNKNOWN;
    texresDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;
    texresDesc.Height = 1;
    texresDesc.DepthOrArraySize = 1;
    texresDesc.MipLevels = 1;
    texresDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    texresDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    texresDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    texresDesc.SampleDesc.Count = 1;
    texresDesc.SampleDesc.Quality = 0;

    ID3D12Resource* texuploadBuff = nullptr;
    res = _dev->CreateCommittedResource(&texuploadheapProp, D3D12_HEAP_FLAG_NONE, &texresDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texuploadBuff));
    ASSERT_RES(res, "CreateCommittedResource");

    D3D12_HEAP_PROPERTIES texheapProp = {};
    texheapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    texheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    texheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    texheapProp.CreationNodeMask = 0;
    texheapProp.VisibleNodeMask = 0;

    texresDesc.Format = metadata.format;
    texresDesc.Width = metadata.width;
    texresDesc.Height = metadata.height;
    texresDesc.DepthOrArraySize = metadata.arraySize;
    texresDesc.MipLevels = metadata.mipLevels;
    texresDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    texresDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texresDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* texBuff = nullptr;
    res = _dev->CreateCommittedResource(&texheapProp, D3D12_HEAP_FLAG_NONE, &texresDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texBuff));
    ASSERT_RES(res, "CreateCommittedResource");

    uint8_t* texMap = nullptr;
    res = texuploadBuff->Map(0, nullptr, (void**)&texMap);

    auto srcAddr = img->pixels;
    auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    for (int y = 0; y < img->height; ++y) {
        std::copy_n(srcAddr, img->rowPitch, texMap);
        srcAddr += img->rowPitch;
        texMap += rowPitch;
    }
    texuploadBuff->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION srctexLocation = {};
    srctexLocation.pResource = texuploadBuff;
    srctexLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srctexLocation.PlacedFootprint.Offset = 0;
    srctexLocation.PlacedFootprint.Footprint.Width = metadata.width;
    srctexLocation.PlacedFootprint.Footprint.Height = metadata.height;
    srctexLocation.PlacedFootprint.Footprint.Depth = metadata.depth;
    srctexLocation.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    srctexLocation.PlacedFootprint.Footprint.Format = img->format;

    D3D12_TEXTURE_COPY_LOCATION dsttexLocation = {};
    dsttexLocation.pResource = texBuff;
    dsttexLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dsttexLocation.SubresourceIndex = 0;

    {
        _cmdList->CopyTextureRegion(&dsttexLocation, 0, 0, 0, &srctexLocation, nullptr);

        D3D12_RESOURCE_BARRIER barrierDesc = {};
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.Transition.pResource = texBuff;
        barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        _cmdList->ResourceBarrier(1, &barrierDesc);
        _cmdList->Close();

        ID3D12CommandList* cmdlists[] = { _cmdList };
        _cmdQueue->ExecuteCommandLists(1, cmdlists);

        ID3D12Fence* fence = nullptr;
        UINT fenceVal = 0;
        res = _dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        ASSERT_RES(res, "CreateFence");

        _cmdQueue->Signal(fence, ++fenceVal);
        if (fence->GetCompletedValue() != fenceVal) {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        _cmdAllocator->Reset();
        _cmdList->Reset(_cmdAllocator, nullptr);
    }

    DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();

    D3D12_HEAP_PROPERTIES constheapProp = {};
    constheapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    constheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    constheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    constheapProp.CreationNodeMask = 0;
    constheapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC constresDesc = {};
    constresDesc.Format = DXGI_FORMAT_UNKNOWN;
    constresDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constresDesc.Width = (sizeof(matrix) + 0xff) & ~0xff;
    constresDesc.Height = 1;
    constresDesc.DepthOrArraySize = 1;
    constresDesc.MipLevels = 1;
    constresDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    constresDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    constresDesc.SampleDesc.Count = 1;
    constresDesc.SampleDesc.Quality = 0;

    ID3D12Resource* constBuff = nullptr;
    res = _dev->CreateCommittedResource(&constheapProp, D3D12_HEAP_FLAG_NONE, &constresDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuff));
    ASSERT_RES(res, "CreateCommittedResource");

    DirectX::XMMATRIX* matrixMap;
    res = constBuff->Map(0, nullptr, (void**)&matrixMap);
    *matrixMap = matrix;

    D3D12_DESCRIPTOR_HEAP_DESC basicheapDesc = {};
    basicheapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    basicheapDesc.NodeMask = 0;
    basicheapDesc.NumDescriptors = 2;
    basicheapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ID3D12DescriptorHeap* basicHeap = nullptr;
    res = _dev->CreateDescriptorHeap(&basicheapDesc, IID_PPV_ARGS(&basicHeap));
    ASSERT_RES(res, "CreateDescriptorHeap");

    auto basicheapHandle = basicHeap->GetCPUDescriptorHandleForHeapStart();
    const UINT basicheapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    _dev->CreateShaderResourceView(texBuff, &srvDesc, basicheapHandle);

    basicheapHandle.ptr += basicheapSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constBuff->GetDesc().Width;
    _dev->CreateConstantBufferView(&cbvDesc, basicheapHandle);

    D3D12_DESCRIPTOR_RANGE descRanges[2] = {};
    descRanges[0].NumDescriptors = 1;
    descRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRanges[0].BaseShaderRegister = 0;
    descRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    descRanges[1].NumDescriptors = 1;
    descRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descRanges[1].BaseShaderRegister = 0;
    descRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootparams[2] = {};
    rootparams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootparams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootparams[0].DescriptorTable.pDescriptorRanges = &descRanges[0];
    rootparams[0].DescriptorTable.NumDescriptorRanges = 1;

    rootparams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootparams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootparams[1].DescriptorTable.pDescriptorRanges = &descRanges[1];
    rootparams[1].DescriptorTable.NumDescriptorRanges = 1;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    D3D12_ROOT_SIGNATURE_DESC rootsigDesc = {};
    rootsigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootsigDesc.pParameters = rootparams;
    rootsigDesc.NumParameters = 2;
    rootsigDesc.pStaticSamplers = &samplerDesc;
    rootsigDesc.NumStaticSamplers = 1;

    ID3DBlob* rootsigBlob = nullptr;
    res = D3D12SerializeRootSignature(&rootsigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootsigBlob, &errorBlob);
    ASSERT_RES(res, "D3D12SerializeRootSignature");

    ID3D12RootSignature* rootsignature;
    res = _dev->CreateRootSignature(0, rootsigBlob->GetBufferPointer(), rootsigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
    ASSERT_RES(res, "CreateRootSignature");
    rootsigBlob->Release();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipelineDesc = {};

    gpipelineDesc.pRootSignature = rootsignature;
    gpipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    gpipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    gpipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    gpipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

    gpipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    gpipelineDesc.RasterizerState.MultisampleEnable = false;
    gpipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    gpipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gpipelineDesc.RasterizerState.DepthClipEnable = true;

    gpipelineDesc.BlendState.AlphaToCoverageEnable = false;
    gpipelineDesc.BlendState.IndependentBlendEnable = false;
    D3D12_RENDER_TARGET_BLEND_DESC rtvblendDesc = {};
    rtvblendDesc.BlendEnable = false;
    rtvblendDesc.LogicOpEnable = false;
    rtvblendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    gpipelineDesc.BlendState.RenderTarget[0] = rtvblendDesc;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    gpipelineDesc.InputLayout.pInputElementDescs = inputLayout;
    gpipelineDesc.InputLayout.NumElements = _countof(inputLayout);

    gpipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    gpipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gpipelineDesc.NumRenderTargets = 1;
    gpipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    gpipelineDesc.SampleDesc.Count = 1;
    gpipelineDesc.SampleDesc.Quality = 0;

    ID3D12PipelineState* piplinestate = nullptr;
    res = _dev->CreateGraphicsPipelineState(&gpipelineDesc, IID_PPV_ARGS(&piplinestate));
    ASSERT_RES(res, "CreateGraphicsPipelineState");

    D3D12_VIEWPORT viewport = {};
    viewport.Width = WINDOW_WIDTH;
    viewport.Height = WINDOW_HEIGHT;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MaxDepth = 1.0f;
    viewport.MinDepth = 0.0f;

    D3D12_RECT scissorrect = {};
    scissorrect.top = viewport.TopLeftY;
    scissorrect.left = viewport.TopLeftX;
    scissorrect.right = scissorrect.left + viewport.Width;
    scissorrect.bottom = scissorrect.top + viewport.Height;

    res = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    ASSERT_RES(res, "CreateFence");

    ShowWindow(hwnd, SW_SHOW);

    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Transition.Subresource = 0;

    MSG msg = {};

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        _cmdList->SetPipelineState(piplinestate);
        _cmdList->SetGraphicsRootSignature(rootsignature);

        _cmdList->SetDescriptorHeaps(1, &basicHeap);
        auto basicH = basicHeap->GetGPUDescriptorHandleForHeapStart();
        _cmdList->SetGraphicsRootDescriptorTable(0, basicH);
        basicH.ptr += basicheapSize;
        _cmdList->SetGraphicsRootDescriptorTable(1, basicH);

        _cmdList->RSSetViewports(1, &viewport);
        _cmdList->RSSetScissorRects(1, &scissorrect);

        _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _cmdList->IASetVertexBuffers(0, 1, &vbView);
        _cmdList->IASetIndexBuffer(&ibView);

        auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

        barrierDesc.Transition.pResource = backBuffers[bbIdx];
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        _cmdList->ResourceBarrier(1, &barrierDesc);

        auto rtvH = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr += bbIdx * rtvheapSize;
        _cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        _cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

        _cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        _cmdList->ResourceBarrier(1, &barrierDesc);

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