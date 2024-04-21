#include <iostream>
#include <windows.h>
#include <stdint.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

LARGE_INTEGER TICKS_PER_SECOND;

int const WINDOW_WIDTH  = 1920;
int const WINDOW_HEIGHT = 1080;

void update(float deltaTime){
    UNREFERENCED_PARAMETER(deltaTime);
};
void render(){};

LRESULT CALLBACK 
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            if(LOWORD(wParam) == VK_ESCAPE) { PostQuitMessage(0); } 
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd); 
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, 
                   HINSTANCE hInstPrev, 
                   PSTR      cmdline, 
                   int       cmdshow) {

    UNREFERENCED_PARAMETER(hInstPrev);
    UNREFERENCED_PARAMETER(cmdline);

    WNDCLASS WindowClass{};
    WindowClass.lpfnWndProc   = WindowProc;
    WindowClass.hInstance     = hInst;
    WindowClass.lpszClassName = "Window Class"; 
    WindowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&WindowClass);

    HWND hwnd = CreateWindowEx(
        0,
        "Window Class",
        "Game of Life",
        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInst,
        NULL
    );

    if (hwnd == NULL) { return 0; }

    ShowWindow(hwnd, cmdshow);

    // init direct3d
    RECT dimensions;
    GetClientRect(hwnd, &dimensions);

    unsigned int width  = dimensions.right  - dimensions.left;
    unsigned int height = dimensions.bottom - dimensions.top;

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    D3D_FEATURE_LEVEL featureLevels[] = { 
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    uint8_t numberOfFeatureLevels = 3;
    IDXGISwapChain* swapChain;
    ID3D11Device* device;
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11DeviceContext* d3dContext;
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   nullptr,
                                                   0,
                                                   featureLevels,
                                                   numberOfFeatureLevels,
                                                   D3D11_SDK_VERSION,
                                                   &swapChainDesc,
                                                   &swapChain,
                                                   &device,
                                                   &featureLevel,
                                                   &d3dContext);

    if(FAILED(result)) {
        std::cout << "Failed to create the Direct3D device and swapchain\n";
        return false;
    }

    ID3D11RenderTargetView* backBufferTarget;
    ID3D11Texture2D* backBufferTexture;

    result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                  (LPVOID*)&backBufferTexture);

    if(FAILED(result)) {
        std::cout << "Failed to get the get the swap chain buffer\n";
    }

    result = device->CreateRenderTargetView(backBufferTexture,
                                            0,
                                            &backBufferTarget);

    if(backBufferTexture) {
        backBufferTexture->Release();
    }

    if(FAILED(result)) {
        std::cout << "Failed to create the render target view\n";
        return false;
    }

    d3dContext->OMSetRenderTargets(1, &backBufferTarget, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    d3dContext->RSSetViewports(1, &viewport);

    // render vertices
    struct VertexPos {
        DirectX::XMFLOAT3 pos;
    };

    VertexPos vertices[] = {
        DirectX::XMFLOAT3( 0.5f,  0.5f, 0.5f),
        DirectX::XMFLOAT3( 0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f)
    };

    D3D11_BUFFER_DESC vertexDesc{};
    
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(VertexPos) * 3;

    D3D11_SUBRESOURCE_DATA resourceData{};

    resourceData.pSysMem = vertices;

    ID3D11Buffer* vertexBuffer = nullptr;
    result = device->CreateBuffer(&vertexDesc, &resourceData, &vertexBuffer);

    if(FAILED(result)) {
        std::cout << "Failed to create the vertex buffer\n";
    }

    ID3D11VertexShader* solidColorVS;
    ID3D11PixelShader* solidColorPS;
    ID3D11InputLayout* inputLayout;

    ID3DBlob* vsBuffer = 0;
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    ID3DBlob* errorBuffer = 0;
    result = D3DCompileFromFile(L"vertex.fx",
                                nullptr, nullptr,
                                "VS_Main",
                                "vs_4_0",
                                shaderFlags,
                                0, 
                                &vsBuffer,
                                &errorBuffer);

    if(FAILED(result)) {
        if(errorBuffer != 0) {
            OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
        }
    }
    
    result = device->CreateVertexShader(vsBuffer->GetBufferPointer(),
                                        vsBuffer->GetBufferSize(),
                                        0,
                                        &solidColorVS);

    if(FAILED(result)) {
        std::cout << "Failed to create the vertex shader\n";
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC vertexLayout[] = {
        "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D11_INPUT_PER_VERTEX_DATA, 0 
    };

    unsigned int totalLayoutElements = ARRAYSIZE(vertexLayout);

    result = device->CreateInputLayout(vertexLayout,
                                       totalLayoutElements,
                                       vsBuffer->GetBufferPointer(),
                                       vsBuffer->GetBufferSize(),
                                       &inputLayout);

    if(FAILED(result)) {
        std::cout << "Failed to create the input layout\n";
        return false;
    }

    ID3DBlob* psBuffer = nullptr;

    result = D3DCompileFromFile(L"pixel.fx",
                                nullptr,
                                nullptr,
                                "PS_Main",
                                "ps_4_0",
                                shaderFlags,
                                0,
                                &psBuffer,
                                &errorBuffer);

    if(FAILED(result)) {
        if(errorBuffer != 0) {
            OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
        }
    }

    result = device->CreatePixelShader(psBuffer->GetBufferPointer(),
                                       psBuffer->GetBufferSize(),
                                       0,
                                       &solidColorPS);

    if(FAILED(result)) {
        std::cout << "Failed to create the pixel shader\n";
        return false;
    }

    // render code start
    float clearColor[4] = { 0.25f, 0.25f, 1.0f, 1.0f };
    d3dContext->ClearRenderTargetView(backBufferTarget, clearColor);

    unsigned int stride = sizeof(VertexPos);
    unsigned int offset = 0;

    d3dContext->IASetInputLayout(inputLayout);
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    d3dContext->VSSetShader(solidColorVS, 0, 0);
    d3dContext->PSSetShader(solidColorPS, 0, 0);
    d3dContext->Draw(3, 0);

    swapChain->Present(0, 0);
    // render code end

    LARGE_INTEGER previousTickCount;
    LARGE_INTEGER currentTickCount;

    QueryPerformanceFrequency(&TICKS_PER_SECOND);
    QueryPerformanceCounter(&previousTickCount);

    bool running = true; 
    MSG msg = {};
    float deltaTime;
    uint64_t elapsedTicks;
    uint64_t elapsedTimeInMicroseconds;

    while(running) {
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // get delta time
        QueryPerformanceCounter(&currentTickCount);
        elapsedTicks = currentTickCount.QuadPart - previousTickCount.QuadPart;
        elapsedTimeInMicroseconds = (elapsedTicks * 1000000) / TICKS_PER_SECOND.QuadPart;

        // deltaTime in milliseconds
        deltaTime = elapsedTimeInMicroseconds / 1000.0f;
        previousTickCount = currentTickCount;

        // update game state below
        update(deltaTime);
        render();
    }
    return 0;
}
