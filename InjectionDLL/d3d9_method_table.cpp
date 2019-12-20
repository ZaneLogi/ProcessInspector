#include "stdafx.h"
#include <d3d9.h>
#include <cassert>
#include "Logger.h"
#include "d3d9_method_table.h"
#include "../minhook/include/MinHook.h"

d3d9_method_table* d3d9_method_table::instance()
{
    static d3d9_method_table instance;
    return &instance;
}

d3d9_method_table::~d3d9_method_table()
{
    deinit();
}

bool d3d9_method_table::init()
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

    HMODULE libD3D9;
    if ((libD3D9 = ::GetModuleHandle(_T("d3d9.dll"))) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: GetModuleHandle('d3d9.dll') returns NULL.\n";
        return false;
    }

    void* Direct3DCreate9;
    if ((Direct3DCreate9 = ::GetProcAddress(libD3D9, "Direct3DCreate9")) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: GetProcAddress('Direct3DCreate9') returns NULL.\n";
        return false;
    }

    LPDIRECT3D9 direct3D9;
    if ((direct3D9 = ((LPDIRECT3D9(__stdcall*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION)) == NULL)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: Direct3DCreate9 returns NULL.\n";
        return false;
    }

    D3DDISPLAYMODE displayMode;
    if (direct3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode) < 0)
    {
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: IDirect3DDevice9::GetAdapterDisplayMode returns err.\n";
        return false;
    }

    D3DPRESENT_PARAMETERS params;
    params.BackBufferWidth = 0;
    params.BackBufferHeight = 0;
    params.BackBufferFormat = displayMode.Format;
    params.BackBufferCount = 0;
    params.MultiSampleType = D3DMULTISAMPLE_NONE;
    params.MultiSampleQuality = NULL;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = hwnd;
    params.Windowed = 1;
    params.EnableAutoDepthStencil = 0;
    params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    params.Flags = NULL;
    params.FullScreen_RefreshRateInHz = 0;
    params.PresentationInterval = 0;

    LPDIRECT3DDEVICE9 device;
    if (direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device) < 0)
    {
        direct3D9->Release();
        ::DestroyWindow(hwnd);
        ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        LOG << "ERR: IDirect3DDevice9::CreateDevice returns err.\n";
        return false;
    }

    m_methodsTable = (DWORD_PTR*)::calloc(119, sizeof(DWORD_PTR));
    ::memcpy(m_methodsTable, *(DWORD_PTR**)device, 119 * sizeof(DWORD_PTR));

    direct3D9->Release();
    direct3D9 = NULL;

    device->Release();
    device = NULL;

    ::DestroyWindow(hwnd);
    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    return true;
}

void d3d9_method_table::deinit()
{
    if (m_methodsTable)
    {
        free(m_methodsTable);
        m_methodsTable = nullptr;
    }
}

bool d3d9_method_table::bind(uint16_t index, void** _original, void* _function)
{
    assert(index >= 0 && _original != NULL && _function != NULL);
    void* target = (void*)m_methodsTable[index];
    if (MH_CreateHook(target, _function, _original) != MH_OK || MH_EnableHook(target) != MH_OK)
    {
        return false;
    }
    return true;
}

void d3d9_method_table::unbind(uint16_t index)
{
    assert(index >= 0);
    MH_DisableHook((void*)m_methodsTable[index]);
}

DWORD_PTR* d3d9_method_table::operator[] (int index)
{
    return (DWORD_PTR*)m_methodsTable[index];
}
