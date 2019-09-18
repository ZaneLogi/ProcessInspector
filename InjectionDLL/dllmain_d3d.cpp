#include "stdafx.h"
#include <d3d9.h>

#pragma comment(lib, "d3d9.lib")

#pragma pack(push)
#pragma pack(1)
#ifndef _WIN64
struct JmpCode
{
private:
    const BYTE jmp;
    DWORD address;

public:
    JmpCode(DWORD srcAddr, DWORD dstAddr)
        : jmp(0xE9)
    {
        setAddress(srcAddr, dstAddr);
    }

    void setAddress(DWORD srcAddr, DWORD dstAddr)
    {
        address = dstAddr - srcAddr - sizeof(JmpCode);
    }
};
#else
struct JmpCode
{
private:
    BYTE jmp[6];
    uintptr_t address;

public:
    JmpCode(uintptr_t srcAddr, uintptr_t dstAddr)
    {
        static const BYTE JMP[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
        memcpy(jmp, JMP, sizeof(jmp));
        setAddress(srcAddr, dstAddr);
    }

    void setAddress(uintptr_t srcAddr, uintptr_t dstAddr)
    {
        address = dstAddr;
    }
};
#endif
#pragma pack(pop)

static
void hook(void* originalFunction, void* hookFunction, BYTE* oldCode)
{
    JmpCode code((uintptr_t)originalFunction, (uintptr_t)hookFunction);
    DWORD oldProtect, oldProtect2;
    VirtualProtect(originalFunction, sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(oldCode, originalFunction, sizeof(code));
    memcpy(originalFunction, &code, sizeof(code));
    VirtualProtect(originalFunction, sizeof(code), oldProtect, &oldProtect2);
}

static
void unhook(void* originalFunction, BYTE* oldCode)
{
    DWORD oldProtect, oldProtect2;
    VirtualProtect(originalFunction, sizeof(JmpCode), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(originalFunction, oldCode, sizeof(JmpCode));
    VirtualProtect(originalFunction, sizeof(JmpCode), oldProtect, &oldProtect2);
}

void* endSceneAddr = NULL;
BYTE endSceneOldCode[sizeof(JmpCode)];

HRESULT STDMETHODCALLTYPE MyEndScene(IDirect3DDevice9* thiz)
{
#if 1
    static bool init = false;
    if (!init)
    {
        LOG << "Boom! It's works!\n";
        init = true;
    }
#endif

    unhook(endSceneAddr, endSceneOldCode);
    HRESULT hr = thiz->EndScene();
    hook(endSceneAddr, MyEndScene, endSceneOldCode);
    return hr;
}

DWORD WINAPI initHookThread(LPVOID dllMainThread)
{
    LOG << "enter initHookThread\n";

    // wait DllMain(LoadLibrary thread) stop
    //WaitForSingleObject(dllMainThread, INFINITE);
    CloseHandle(dllMainThread);

    LOG << "initialize...\n";

    // Create a window to initialize D3D

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = _T("DummyWindow");
    if (RegisterClassEx(&wc) == 0)
    {
        MessageBox(NULL, _T("Failed to call RegisterClassEx"), _T(""), MB_OK);
        LOG << "Failed to call RegisterClassEx\n";
        return 0;
    }

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, _T(""), WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, NULL, NULL, wc.hInstance, NULL);
    if (hwnd == NULL)
    {
        MessageBox(NULL, _T("Failed to call CreateWindowEx"), _T(""), MB_OK);
        LOG << "Failed to call CreateWindowEx\n";
        return 0;
    }

    // Initialize D3D

    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d9 == NULL)
    {
        MessageBox(NULL, _T("Failed to call Direct3DCreate9"), _T(""), MB_OK);
        DestroyWindow(hwnd);
        LOG << "Failed to call Direct3DCreate9\n";
        return 0;
    }

    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;

    IDirect3DDevice9* device;
    if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device)))
    {
        MessageBox(NULL, _T("Failed to call IDirect3DDevice9::CreateDevice"), _T(""), MB_OK);
        d3d9->Release();
        DestroyWindow(hwnd);
        LOG << "Failed to call IDirect3DDevice9::CreateDevice\n";
        return 0;
    }

    // hook EndScene
    endSceneAddr = (*(void***)device)[42]; // EndScene is the 43th function in IDirect3DDevice9
    hook(endSceneAddr, MyEndScene, endSceneOldCode);

    LOG << "old EndScene = " << endSceneAddr << ", new EndScene = " << MyEndScene << "\n";

    // release the interfaces
    d3d9->Release();
    device->Release();
    DestroyWindow(hwnd);

    if (!UnregisterClass(_T("DummyWindow"), NULL))
    {
        LOG << "Failed to call UnregisterClass\n";
    }
    return 0;
}



#if 0

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    CHAR processPath[MAX_PATH];

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Inject DLL to " << processPath << "\n";

        // get the current thread handle
        HANDLE curThread;
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &curThread, SYNCHRONIZE, FALSE, 0))
        {
            LOG << "Failed to call DuplicateHandle\n";
            return FALSE;
        }
        // Can't use COM components in DllMain, so create a new thread for the initialization
        CloseHandle(CreateThread(NULL, 0, initHookThread, curThread, 0, NULL));
        LOG << "done to call CreateThread\n";
        break;

    case DLL_PROCESS_DETACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Unload DLL from " << processPath << "\n";

        if (endSceneAddr != NULL)
            unhook(endSceneAddr, endSceneOldCode);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

#endif
