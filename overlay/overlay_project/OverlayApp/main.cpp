#include <windows.h>
#include <iostream>
#include <vector>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <sstream>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Structure to represent a moving object
struct MovingObject {
    int x, y;
    int velocityX, velocityY;
};

// Global list of moving objects
std::vector<MovingObject> objects = {
    {50, 50, 5, 3},
    {100, 100, -3, 4},
    {200, 150, 4, -2}
};

// DirectX variables
IDXGISwapChain* swapChain = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* deviceContext = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;

// DirectX variables for desktop duplication
Microsoft::WRL::ComPtr<IDXGIOutputDuplication> outputDuplication;
Microsoft::WRL::ComPtr<ID3D11Texture2D> acquiredDesktopImage;

// Buffer to store previous frame
Microsoft::WRL::ComPtr<ID3D11Texture2D> previousFrame;

// Vertex structure
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

// DirectX shader variables
ID3D11VertexShader* vertexShader = nullptr;
ID3D11PixelShader* pixelShader = nullptr;
ID3D11InputLayout* inputLayout = nullptr;

// Vertex shader source
const char* vertexShaderSource = R"(
cbuffer ConstantBuffer : register(b0) {
    matrix modelViewProjection;
};
struct VS_INPUT {
    float3 position : POSITION;
    float4 color : COLOR;
};
struct PS_INPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}
)";

// Pixel shader source
const char* pixelShaderSource = R"(
struct PS_INPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
float4 main(PS_INPUT input) : SV_TARGET {
    return input.color;
}
)";

// Function to convert an integer to a string
std::string IntToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Function to set DPI awareness
void SetDPIAwareness() {
    HMODULE hUser32 = LoadLibraryA("user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
        SetProcessDPIAwareFunc setDPIAware = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (setDPIAware) {
            setDPIAware();
        }
        FreeLibrary(hUser32);
    }
}

// Function to log errors
void LogError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    MessageBoxA(nullptr, message.c_str(), "Error", MB_OK | MB_ICONERROR);
}

// Function to log information
void LogInfo(const std::string& message) {
    std::cout << "Info: " << message << std::endl;
}

// Function to update positions of all objects
void UpdateObjectPositions() {
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    for (auto& obj : objects) {
        obj.x += obj.velocityX;
        obj.y += obj.velocityY;

        // Bounce off the window edges
        if (obj.x < 0 || obj.x > rect.right - 50) obj.velocityX = -obj.velocityX;
        if (obj.y < 0 || obj.y > rect.bottom - 50) obj.velocityY = -obj.velocityY;
    }
}

// Function to initialize DirectX
bool InitDirectX(HWND hwnd) {
    LogInfo("Initializing DirectX...");
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &swapChain,
        &device,
        nullptr,
        &deviceContext
    );

    if (FAILED(hr)) {
        LogError("Failed to create DirectX device and swap chain.");
        return false;
    }
    LogInfo("DirectX device and swap chain created successfully.");

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    if (FAILED(hr)) {
        LogError("Failed to get back buffer.");
        return false;
    }
    LogInfo("Back buffer obtained successfully.");

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    if (FAILED(hr)) {
        LogError("Failed to create render target view.");
        return false;
    }
    LogInfo("Render target view created successfully.");

    backBuffer->Release();

    deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    return true;
}

// Function to compile shader
HRESULT CompileShader(const char* source, const char* entryPoint, const char* target, ID3DBlob** blob) {
    return D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entryPoint, target, 0, 0, blob, nullptr);
}

// Function to initialize shaders
bool InitShaders() {
    LogInfo("Initializing shaders...");
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    // Compile vertex shader
    if (FAILED(CompileShader(vertexShaderSource, "main", "vs_5_0", &vsBlob))) {
        LogError("Failed to compile vertex shader.");
        return false;
    }
    LogInfo("Vertex shader compiled successfully.");

    // Compile pixel shader
    if (FAILED(CompileShader(pixelShaderSource, "main", "ps_5_0", &psBlob))) {
        LogError("Failed to compile pixel shader.");
        return false;
    }
    LogInfo("Pixel shader compiled successfully.");

    // Create vertex shader
    if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader))) {
        LogError("Failed to create vertex shader.");
        return false;
    }
    LogInfo("Vertex shader created successfully.");

    // Create pixel shader
    if (FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader))) {
        LogError("Failed to create pixel shader.");
        return false;
    }
    LogInfo("Pixel shader created successfully.");

    // Define input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // Create input layout
    if (FAILED(device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout))) {
        LogError("Failed to create input layout.");
        return false;
    }
    LogInfo("Input layout created successfully.");

    // Release shader blobs
    vsBlob->Release();
    psBlob->Release();

    return true;
}

// Function to render a frame with DirectX
void RenderFrame() {
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Ensure fully transparent background
    deviceContext->ClearRenderTargetView(renderTargetView, clearColor);

    // Set shaders
    deviceContext->VSSetShader(vertexShader, nullptr, 0);
    deviceContext->PSSetShader(pixelShader, nullptr, 0);

    // Set input layout
    deviceContext->IASetInputLayout(inputLayout);

    // Draw each moving object
    for (const auto& obj : objects) {
        // Define vertices for a rectangle (box)
        Vertex vertices[] = {
            { { static_cast<float>(obj.x), static_cast<float>(obj.y), 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f } }, // Fully transparent
            { { static_cast<float>(obj.x + 50), static_cast<float>(obj.y), 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f } },
            { { static_cast<float>(obj.x), static_cast<float>(obj.y + 50), 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f } },
            { { static_cast<float>(obj.x + 50), static_cast<float>(obj.y + 50), 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f } },
        };

        // Create vertex buffer
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(vertices);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
        ID3D11Buffer* vertexBuffer;
        device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);

        // Set vertex buffer
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        // Draw rectangle
        deviceContext->Draw(4, 0);

        // Release vertex buffer
        vertexBuffer->Release();
    }

    swapChain->Present(0, 0);
}

// Function to initialize desktop duplication
bool InitDesktopDuplication(ID3D11Device* device) {
    LogInfo("Initializing desktop duplication...");
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgiAdapter.GetAddressOf()));

    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf());

    Microsoft::WRL::ComPtr<IDXGIOutput1> dxgiOutput1;
    dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(dxgiOutput1.GetAddressOf()));

    if (!dxgiDevice) {
        LogError("Failed to get DXGI device.");
        return false;
    }
    LogInfo("DXGI device obtained successfully.");

    if (!dxgiAdapter) {
        LogError("Failed to get DXGI adapter.");
        return false;
    }
    LogInfo("DXGI adapter obtained successfully.");

    if (!dxgiOutput) {
        LogError("Failed to get DXGI output.");
        return false;
    }
    LogInfo("DXGI output obtained successfully.");

    if (!dxgiOutput1) {
        LogError("Failed to get DXGI output1.");
        return false;
    }
    LogInfo("DXGI output1 obtained successfully.");

    HRESULT hr = dxgiOutput1->DuplicateOutput(device, outputDuplication.GetAddressOf());
    if (FAILED(hr)) {
        LogError("Failed to initialize desktop duplication.");
        return false;
    }
    LogInfo("Desktop duplication initialized successfully.");
    return true;
}

// Function to capture a frame
bool CaptureFrame() {
    LogInfo("Capturing frame...");
    if (!outputDuplication) {
        LogError("Output duplication not initialized.");
        return false;
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;
    HRESULT hr = outputDuplication->AcquireNextFrame(0, &frameInfo, desktopResource.GetAddressOf());
    if (FAILED(hr)) {
        LogError("Failed to acquire next frame.");
        return false;
    }
    LogInfo("Next frame acquired successfully.");

    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(acquiredDesktopImage.GetAddressOf()));
    if (FAILED(hr)) {
        LogError("Failed to query interface for acquired desktop image.");
        outputDuplication->ReleaseFrame();
        return false;
    }
    LogInfo("Acquired desktop image queried successfully.");

    // Ensure the acquired image is in the correct state
    D3D11_TEXTURE2D_DESC desc;
    acquiredDesktopImage->GetDesc(&desc);
    LogInfo("Acquired image width: " + IntToString(desc.Width) + ", height: " + IntToString(desc.Height) + ", format: " + IntToString(desc.Format) + ", usage: " + IntToString(desc.Usage) + ", CPU access flags: " + IntToString(desc.CPUAccessFlags));

    // Process the frame (e.g., detect changes)

    outputDuplication->ReleaseFrame();
    return true;
}

// Function to initialize frame buffers
bool InitFrameBuffers() {
    LogInfo("Initializing frame buffers...");
    if (!acquiredDesktopImage) {
        LogError("Acquired desktop image is not initialized.");
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    acquiredDesktopImage->GetDesc(&desc);
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, previousFrame.GetAddressOf());
    if (FAILED(hr)) {
        LogError("Failed to create previous frame buffer.");
        return false;
    }
    LogInfo("Previous frame buffer created successfully.");
    return true;
}

// Function to detect movement
std::vector<RECT> DetectMovement() {
    LogInfo("Detecting movement...");
    std::vector<RECT> movingAreas;

    // Verify acquiredDesktopImage is not null
    if (!acquiredDesktopImage) {
        LogError("Acquired desktop image is null.");
        return movingAreas;
    }

    // Map the current and previous frames for reading
    D3D11_MAPPED_SUBRESOURCE currentMapped, previousMapped;
    HRESULT hr = deviceContext->Map(acquiredDesktopImage.Get(), 0, D3D11_MAP_READ, 0, &currentMapped);
    if (FAILED(hr)) {
        LogError("Failed to map current frame. HRESULT: " + IntToString(hr));
        return movingAreas;
    }
    hr = deviceContext->Map(previousFrame.Get(), 0, D3D11_MAP_READ, 0, &previousMapped);
    if (FAILED(hr)) {
        LogError("Failed to map previous frame. HRESULT: " + IntToString(hr));
        deviceContext->Unmap(acquiredDesktopImage.Get(), 0);
        return movingAreas;
    }

    // Compare pixel data to detect changes
    BYTE* currentPixels = static_cast<BYTE*>(currentMapped.pData);
    BYTE* previousPixels = static_cast<BYTE*>(previousMapped.pData);
    D3D11_TEXTURE2D_DESC desc;
    acquiredDesktopImage->GetDesc(&desc);
    int width = desc.Width;
    int height = desc.Height;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * currentMapped.RowPitch) + (x * 4);
            if (memcmp(&currentPixels[index], &previousPixels[index], 4) != 0) {
                // Movement detected, calculate bounding box
                RECT rect = { x, y, x + 50, y + 50 }; // Example size
                movingAreas.push_back(rect);
            }
        }
    }

    // Unmap the resources
    deviceContext->Unmap(acquiredDesktopImage.Get(), 0);
    deviceContext->Unmap(previousFrame.Get(), 0);

    // Update previous frame
    deviceContext->CopyResource(previousFrame.Get(), acquiredDesktopImage.Get());

    LogInfo("Movement detection completed.");
    return movingAreas;
}

// Function to render overlay based on detected changes
void RenderOverlay() {
    LogInfo("Rendering overlay...");
    std::vector<RECT> movingAreas = DetectMovement();

    // Use DirectX to draw boxes around detected changes
    for (const RECT& area : movingAreas) {
        // Define vertices for a rectangle (box)
        Vertex vertices[] = {
            { { static_cast<float>(area.left), static_cast<float>(area.top), 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // Red
            { { static_cast<float>(area.right), static_cast<float>(area.top), 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { static_cast<float>(area.left), static_cast<float>(area.bottom), 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { static_cast<float>(area.right), static_cast<float>(area.bottom), 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        };

        // Create vertex buffer
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(vertices);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
        ID3D11Buffer* vertexBuffer;
        device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);

        // Set vertex buffer
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        // Draw rectangle
        deviceContext->Draw(4, 0);

        // Release vertex buffer
        vertexBuffer->Release();
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        std::cout << "WM_DESTROY received." << std::endl;
        PostQuitMessage(0);
        return 0;
    case WM_TIMER:
        std::cout << "WM_TIMER received." << std::endl;
        UpdateObjectPositions();
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_PAINT:
        std::cout << "WM_PAINT received." << std::endl;
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Perform any necessary painting here
            // For now, just clear the area with a solid color
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            std::cout << "Painting completed." << std::endl;
            EndPaint(hwnd, &ps);
        }
        return 0;
    default:
        std::cout << "Unhandled message: " << uMsg << std::endl;
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitializeConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::ios::sync_with_stdio();
    std::cout.clear();
    std::cerr.clear();
    std::cout << "Console initialized successfully." << std::endl;
}

void WaitForExit() {
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    InitializeConsole();
    LogInfo("Application started.");
    SetDPIAwareness();

    const wchar_t CLASS_NAME[] = L"OverlayWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    std::cout << "Registering window class..." << std::endl;
    if (!RegisterClass(&wc)) {
        LogError("Failed to register window class.");
        WaitForExit();
        return 0;
    }
    std::cout << "Window class registered successfully." << std::endl;

    std::cout << "Creating window..." << std::endl;
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr) {
        LogError("Failed to create window.");
        WaitForExit();
        return 0;
    }
    LogInfo("Window created successfully.");

    // Attempt to make the overlay "screenshot-resistant"
    BOOL affinityResult = SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
    if (!affinityResult) {
        DWORD lastError = GetLastError();
        // Not necessarily fatal; just log it
        LogError("SetWindowDisplayAffinity failed with error code: " + IntToString(lastError));
    }

    // Continue as before
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    std::cout << "Initializing DirectX..." << std::endl;
    if (!InitDirectX(hwnd)) {
        LogError("DirectX initialization failed.");
        WaitForExit();
        return 0;
    }
    std::cout << "DirectX initialized successfully." << std::endl;

    std::cout << "Initializing desktop duplication..." << std::endl;
    if (!InitDesktopDuplication(device)) {
        LogError("Desktop duplication initialization failed.");
        WaitForExit();
        return 0;
    }
    std::cout << "Desktop duplication initialized successfully." << std::endl;

    // Capture an initial frame before initializing frame buffers
    std::cout << "Capturing initial frame..." << std::endl;
    if (!CaptureFrame()) {
        LogError("Failed to capture initial frame.");
        WaitForExit();
        return 0;
    }
    std::cout << "Initial frame captured successfully." << std::endl;

    std::cout << "Initializing frame buffers..." << std::endl;
    if (!InitFrameBuffers()) {
        LogError("Frame buffer initialization failed.");
        WaitForExit();
        return 0;
    }
    std::cout << "Frame buffers initialized successfully." << std::endl;

    std::cout << "Initializing shaders..." << std::endl;
    if (!InitShaders()) {
        LogError("Shader initialization failed.");
        WaitForExit();
        return 0;
    }
    std::cout << "Shaders initialized successfully." << std::endl;

    std::cout << "Setting layered window attributes..." << std::endl;
    if (!SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY)) {
        LogError("Failed to set layered window attributes.");
        WaitForExit();
        return 0;
    }
    std::cout << "Layered window attributes set successfully." << std::endl;

    ShowWindow(hwnd, nShowCmd);

    std::cout << "Setting timer..." << std::endl;
    if (!SetTimer(hwnd, 1, 30, nullptr)) {
        LogError("Failed to set timer.");
        WaitForExit();
        return 0;
    }
    std::cout << "Timer set successfully." << std::endl;

    std::cout << "Entering message loop..." << std::endl;
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        LogInfo("Message received: " + IntToString(msg.message));
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Capture a frame
        if (CaptureFrame()) {
            LogInfo("Frame captured successfully.");
            // Render overlay based on captured frame
            RenderOverlay();
        } else {
            LogError("Failed to capture frame.");
        }

        // Render a frame
        RenderFrame();
    }

    LogInfo("Exiting message loop.");

    // Clean up DirectX
    if (swapChain) swapChain->Release();
    if (device) device->Release();
    if (deviceContext) deviceContext->Release();
    if (renderTargetView) renderTargetView->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (inputLayout) inputLayout->Release();

    WaitForExit();
    return 0;
} 