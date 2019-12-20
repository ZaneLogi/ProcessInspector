#include "stdafx.h"
#include <dxgi.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <cassert>
#include "Logger.h"
#include "d3d10_method_table.h"
#include "../minhook/include/MinHook.h"


d3d10_method_table* d3d10_method_table::instance()
{
    static d3d10_method_table instance;
    return &instance;
}

d3d10_method_table::~d3d10_method_table()
{
    deinit();
}

bool d3d10_method_table::init()
{
    WNDCLASSEX windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = _T("D3D11HOOK_WINDOW_CLASS_NAME");
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);

    HWND hwnd = ::CreateWindow(windowClass.lpszClassName, _T("D3D Window"), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

    HMODULE libDXGI;
    HMODULE libD3D10;
    if ((libDXGI = ::GetModuleHandle(_T("dxgi.dll"))) == NULL || (libD3D10 = ::GetModuleHandle(_T("d3d10.dll"))) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: GetModuleHandle('dxgi.dll') returns NULL.\n";
        return false;
    }

    void* CreateDXGIFactory;
    if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: GetProcAddress('CreateDXGIFactory') returns NULL.\n";
        return false;
    }

    IDXGIFactory* factory;
    if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: CreateDXGIFactory returns err.\n";
        return false;
    }

    IDXGIAdapter* adapter;
    if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: IDXGIFactory::EnumAdapters returns err.\n";
        return false;
    }

    void* D3D10CreateDeviceAndSwapChain;
    if ((D3D10CreateDeviceAndSwapChain = ::GetProcAddress(libD3D10, "D3D10CreateDeviceAndSwapChain")) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: GetProcAddress('D3D10CreateDeviceAndSwapChain') returns NULL.\n";
        return false;
    }

    DXGI_RATIONAL refreshRate;
    refreshRate.Numerator = 60;
    refreshRate.Denominator = 1;

    DXGI_MODE_DESC bufferDesc;
    bufferDesc.Width = 100;
    bufferDesc.Height = 100;
    bufferDesc.RefreshRate = refreshRate;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc = sampleDesc;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    IDXGISwapChain* swapChain;
    ID3D10Device* device;

    if (((long(__stdcall*)(
        IDXGIAdapter*,
        D3D10_DRIVER_TYPE,
        HMODULE,
        UINT,
        UINT,
        DXGI_SWAP_CHAIN_DESC*,
        IDXGISwapChain**,
        ID3D10Device**))(D3D10CreateDeviceAndSwapChain))(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &device) < 0)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: D3D10CreateDeviceAndSwapChain returns err.\n";
        return false;
    }

    m_methodsTable = (DWORD_PTR*)::calloc(116, sizeof(DWORD_PTR));
    ::memcpy(m_methodsTable, *(DWORD_PTR**)swapChain, 18 * sizeof(DWORD_PTR));
    ::memcpy(m_methodsTable + 18, *(DWORD_PTR**)device, 98 * sizeof(DWORD_PTR));

    swapChain->Release();
    swapChain = NULL;

    device->Release();
    device = NULL;

    ::DestroyWindow(hwnd);
    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    return true;
}

void d3d10_method_table::deinit()
{
    if (m_methodsTable)
    {
        free(m_methodsTable);
        m_methodsTable = nullptr;
    }
}

bool d3d10_method_table::bind(uint16_t index, void** _original, void* _function)
{
    assert(index >= 0 && _original != NULL && _function != NULL);
    void* target = (void*)m_methodsTable[index];
    if (MH_CreateHook(target, _function, _original) != MH_OK || MH_EnableHook(target) != MH_OK)
    {
        return false;
    }
    return true;
}

void d3d10_method_table::unbind(uint16_t index)
{
    assert(index >= 0);
    MH_DisableHook((void*)m_methodsTable[index]);
}

DWORD_PTR* d3d10_method_table::operator[] (int index)
{
    return (DWORD_PTR*)m_methodsTable[index];
}
