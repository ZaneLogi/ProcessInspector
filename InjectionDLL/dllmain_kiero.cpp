#include "stdafx.h"
#include "Logger.h"
#include "kiero.h"
#include <d3d9.h>

// Create the type of function that we will hook
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
static EndScene oEndScene = NULL;

// Declare the detour function
long __stdcall hkD3D9EndScene(LPDIRECT3DDEVICE9 pDevice)
{
    // ... Your magic here ...

#if 1
    static bool init = false;
    if (!init)
    {
        //MessageBox(0, _T("Boom! It's works!"), _T("Kiero"), MB_OK);
        LOG << "Boom! It's works!\n";
        init = true;
    }
#endif

    return oEndScene(pDevice);
}

int kieroExampleThread()
{
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success)
        // or if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success)
    {
        // define KIERO_USE_MINHOOK must be 1
        // the index of the required function can be found in the METHODSTABLE.txt
        if (kiero::bind(42, (void**)&oEndScene, hkD3D9EndScene) != kiero::Status::Success)
        {
            LOG << "Failed to call kiero::bind\n";
        }
    }
    else
    {
        LOG << "Failed to hook D3D9\n";
    }

    return 0;
}

#if 1

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    CHAR processPath[MAX_PATH];

    DisableThreadLibraryCalls(hInstance);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Inject DLL to " << processPath << "\n";
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)kieroExampleThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        GetModuleFileNameA(GetModuleHandle(NULL), processPath, MAX_PATH);
        LOG << "Unload DLL from " << processPath << "\n";
        kiero::unbind(42);
        break;
    }

    return TRUE;
}

#endif
