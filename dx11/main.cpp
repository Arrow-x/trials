#include <d3d11.h>
#include <windows.h>

// Global COM pointers (Initialized to nullptr)
IDXGISwapChain *g_pSwapChain = nullptr;
ID3D11Device *g_pd3dDevice = nullptr;
ID3D11DeviceContext *g_pImmediateContext = nullptr;
ID3D11RenderTargetView *g_pRenderTargetView = nullptr;
ID3D11VertexShader *g_pVertexShader = nullptr;
ID3D11PixelShader *g_pPixelShader = nullptr;
ID3D11InputLayout *g_pVertexLayout = nullptr;
ID3D11Buffer *g_pVertexBuffer = nullptr;

struct SimpleVertex {
  float x, y, z;
  float r, g, b, a;
};

// Typedef for the D3DCompile function signature from d3dcompiler.h
typedef HRESULT(WINAPI *pfnD3DCompile)(
    LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName,
    const void *pDefines, void *pInclude, // simplified macros/includes pointers
    LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2,
    ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);

// Simple HLSL Shaders embedded as strings
const char *devShaders =
    "struct VS_OUTPUT { float4 Pos : SV_POSITION; float4 Color : COLOR; };\n"
    "VS_OUTPUT VS(float4 Pos : POSITION, float4 Color : COLOR) {\n"
    "    VS_OUTPUT output = (VS_OUTPUT)0;\n"
    "    output.Pos = Pos;\n"
    "    output.Color = Color;\n"
    "    return output;\n"
    "}\n"
    "float4 PS(VS_OUTPUT input) : SV_Target { return input.Color; }\n";

HRESULT InitDevice(HWND hWnd) {
  RECT rc;
  GetClientRect(hWnd, &rc);
  UINT width = rc.right - rc.left;
  UINT height = rc.bottom - rc.top;

  DXGI_SWAP_CHAIN_DESC sd{
      .BufferDesc.Width = width,
      .BufferDesc.Height = height,
      .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .BufferDesc.RefreshRate.Numerator = 60,
      .BufferDesc.RefreshRate.Denominator = 1,
      .SampleDesc.Count = 1,
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 1,
      .OutputWindow = hWnd,
      .Windowed = TRUE,
  };

  UINT createDeviceFlags = 0;
#ifdef _DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
  D3D_FEATURE_LEVEL featureLevel;

  // Fallback logic for Wine execution environment
  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevels, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
      &featureLevel, &g_pImmediateContext);

  if (FAILED(hr) && (createDeviceFlags & D3D11_CREATE_DEVICE_DEBUG)) {
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 1,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
        &g_pImmediateContext);
  }
  if (FAILED(hr))
    return hr;

  ID3D11Texture2D *pBackBuffer = nullptr;
  g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer);
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                       &g_pRenderTargetView);
  pBackBuffer->Release();

  g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

  D3D11_VIEWPORT vp = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
  g_pImmediateContext->RSSetViewports(1, &vp);

  // Dynamically load Wine's shader compiler DLL to bypass missing SDK linker
  // lib
  HMODULE hCompiler = LoadLibraryA("d3dcompiler_47.dll");
  if (!hCompiler)
    return E_FAIL;

  pfnD3DCompile DynamicD3DCompile =
      (pfnD3DCompile)GetProcAddress(hCompiler, "D3DCompile");
  if (!DynamicD3DCompile) {
    FreeLibrary(hCompiler);
    return E_FAIL;
  }

  // Compile Vertex Shader
  ID3DBlob *pVSBlob = nullptr;
  DynamicD3DCompile(devShaders, strlen(devShaders), nullptr, nullptr, nullptr,
                    "VS", "vs_4_0", 0, 0, &pVSBlob, nullptr);
  g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),
                                   pVSBlob->GetBufferSize(), nullptr,
                                   &g_pVertexShader);

  // Setup input layout descriptor
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  g_pd3dDevice->CreateInputLayout(layout, 2, pVSBlob->GetBufferPointer(),
                                  pVSBlob->GetBufferSize(), &g_pVertexLayout);
  pVSBlob->Release();

  // Compile Pixel Shader
  ID3DBlob *pPSBlob = nullptr;
  DynamicD3DCompile(devShaders, strlen(devShaders), nullptr, nullptr, nullptr,
                    "PS", "ps_4_0", 0, 0, &pPSBlob, nullptr);
  g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),
                                  pPSBlob->GetBufferSize(), nullptr,
                                  &g_pPixelShader);
  pPSBlob->Release();

  FreeLibrary(hCompiler);

  // Vertex data (-1.0 to 1.0 Normalized Device Coordinates)
  SimpleVertex vertices[] = {{0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
                             {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
                             {-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}};

  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = sizeof(SimpleVertex) * 3;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData = {};
  InitData.pSysMem = vertices;
  g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);

  return S_OK;
}

void Render() {
  float clearColor[] = {0.1f, 0.2f, 0.3f, 1.0f};
  g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

  UINT stride = sizeof(SimpleVertex);
  UINT offset = 0;
  g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride,
                                          &offset);
  g_pImmediateContext->IASetInputLayout(g_pVertexLayout);
  g_pImmediateContext->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
  g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

  g_pImmediateContext->Draw(3, 0);
  g_pSwapChain->Present(0, 0);
}

void CleanUp() {
  if (g_pVertexBuffer)
    g_pVertexBuffer->Release();
  if (g_pVertexLayout)
    g_pVertexLayout->Release();
  if (g_pVertexShader)
    g_pVertexShader->Release();
  if (g_pPixelShader)
    g_pPixelShader->Release();
  if (g_pRenderTargetView)
    g_pRenderTargetView->Release();
  if (g_pSwapChain)
    g_pSwapChain->Release();
  if (g_pImmediateContext)
    g_pImmediateContext->Release();
  if (g_pd3dDevice)
    g_pd3dDevice->Release();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  if (message == WM_DESTROY) {
    CleanUp();
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
  WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                   CS_HREDRAW | CS_VREDRAW,
                   WndProc,
                   0,
                   0,
                   hInstance,
                   nullptr,
                   nullptr,
                   nullptr,
                   nullptr,
                   "DX11App",
                   nullptr};
  RegisterClassEx(&wc);
  HWND hWnd = CreateWindow("DX11App", "Zig + DX11 Cross Compile on Linux",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           800, 600, nullptr, nullptr, hInstance, nullptr);

  if (FAILED(InitDevice(hWnd)))
    return 0;
  ShowWindow(hWnd, nCmdShow);

  MSG msg = {};
  while (WM_QUIT != msg.message) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      Render();
    }
  }
  return (int)msg.wParam;
}
