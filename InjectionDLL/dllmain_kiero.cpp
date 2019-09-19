#include "stdafx.h"
#include "Logger.h"
#include "kiero.h"
#include <d3d9.h>
#include <d3d11.h>

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

typedef long (__stdcall* IDXGISwapChain_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static IDXGISwapChain_Present oD3D11Present = nullptr;

long __stdcall hkD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
#if 1
    static bool init = false;
    if (!init)
    {
        //MessageBox(0, _T("Boom! It's works!"), _T("Kiero"), MB_OK);
        LOG << "Boom! It's works!\n";
        init = true;
    }
#endif

    //return oD3D11Present(pSwapChain, SyncInterval, Flags);
    return S_OK;
}






int kieroExampleThread()
{
    LOG << "enter kieroExampleThread\n";
    auto result = kiero::init(kiero::RenderType::D3D11);
    if ( result == kiero::Status::Success)
    {
        auto render_type = kiero::getRenderType();
        if (render_type == kiero::RenderType::D3D11)
        {
            LOG << "type = d3d11\n";
            // the index of the required function can be found in the METHODSTABLE.txt
            //if (kiero::bind(42, (void**)&oEndScene, hkD3D9EndScene) != kiero::Status::Success)
            //{
            //    LOG << "Failed to call kiero::bind\n";
            //}
            if (kiero::bind(8, (void**)&oD3D11Present, hkD3D11Present) != kiero::Status::Success)
            {
                LOG << "Failed to call kiero::bind\n";
            }
        }
        else
        {
            LOG << "No interested D3D found!\n";
        }
    }
    else
    {
        LOG << "Failed to call kiero::init, err = " << result << "\n";
    }

    LOG << "exit kieroExampleThread\n";

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
        if (oEndScene != nullptr)
        {
            kiero::unbind(42);
            oEndScene = nullptr;
        }
        if (oD3D11Present != nullptr)
        {
            kiero::unbind(8);
            oD3D11Present = nullptr;
        }
        break;
    }

    return TRUE;
}

#endif
