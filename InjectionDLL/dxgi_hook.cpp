#include "stdafx.h"

#include <d3d11.h>

#include "dxgi_hook.h"
#include "d3d11_method_table.h"
#include "d3d10_method_table.h"

extern long __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);


dxgi_hook* dxgi_hook::instance()
{
    static dxgi_hook this_instance;
    return &this_instance;
}

dxgi_hook::~dxgi_hook()
{
}

bool dxgi_hook::init()
{
    if (d3d11_method_table::instance()->init())
    {
        auto target8 = (*d3d11_method_table::instance())[8];
        m_dxgiSwapChainPresentHook.reset(new ApiHook<DXGISwapChainPresentType>(L"DXGISwapChainPresentType",
            target8, (DWORD_PTR *)SwapChainPresent));
        m_dxgiSwapChainPresentHook->activeHook();

        auto target13 = (*d3d11_method_table::instance())[13];
        m_dxgiSwapChainResizeBuffersHook.reset(new ApiHook<DXGISwapChainResizeBuffersType>(L"DXGISwapChainResizeBuffersType",
            target13, (DWORD_PTR *)SwapChainResizeBuffers));
        m_dxgiSwapChainResizeBuffersHook->activeHook();

        auto target14 = (*d3d11_method_table::instance())[14];
        m_dxgiSwapChainResizeTargetHook.reset(new ApiHook<DXGISwapChainResizeTargetType>(L"DXGISwapChainReiszeTargetType",
            target14, (DWORD_PTR *)SwapChainResizeTarget));
        m_dxgiSwapChainResizeTargetHook->activeHook();

        auto target_present1 = d3d11_method_table::instance()->swapchain_present1();
        if (target_present1)
        {
            m_dxgiSwapChainPresent1Hook.reset(new ApiHook<DXGISwapChainPresent1Type>(L"DXGISwapChainPresent1Type",
                target_present1, (DWORD_PTR *)SwapChainPresent1));
            m_dxgiSwapChainPresent1Hook->activeHook();
        }

        LOG << "d3d11_method_table enabled\n";

        return true;
    }
    else if (d3d10_method_table::instance()->init())
    {
        auto target8 = (*d3d10_method_table::instance())[8];
        m_dxgiSwapChainPresentHook.reset(new ApiHook<DXGISwapChainPresentType>(L"DXGISwapChainPresentType",
            target8, (DWORD_PTR *)SwapChainPresent));
        m_dxgiSwapChainPresentHook->activeHook();

        auto target13 = (*d3d10_method_table::instance())[13];
        m_dxgiSwapChainResizeBuffersHook.reset(new ApiHook<DXGISwapChainResizeBuffersType>(L"DXGISwapChainResizeBuffersType",
            target13, (DWORD_PTR *)SwapChainResizeBuffers));
        m_dxgiSwapChainResizeBuffersHook->activeHook();

        auto target14 = (*d3d10_method_table::instance())[14];
        m_dxgiSwapChainResizeTargetHook.reset(new ApiHook<DXGISwapChainResizeTargetType>(L"DXGISwapChainReiszeTargetType",
            target14, (DWORD_PTR *)SwapChainResizeTarget));
        m_dxgiSwapChainResizeTargetHook->activeHook();

        LOG << "d3d10_method_table enabled\n";

        return true;
    }

    return false;
}

void dxgi_hook::deinit()
{
    if (m_dxgiSwapChainPresentHook)
    {
        m_dxgiSwapChainPresentHook.reset(nullptr);
    }
    if (m_dxgiSwapChainResizeBuffersHook)
    {
        m_dxgiSwapChainResizeBuffersHook.reset(nullptr);
    }
    if (m_dxgiSwapChainResizeTargetHook)
    {
        m_dxgiSwapChainResizeTargetHook.reset(nullptr);
    }
    if (m_dxgiSwapChainPresent1Hook)
    {
        m_dxgiSwapChainPresent1Hook.reset(nullptr);
    }
}

HRESULT dxgi_hook::SwapChainPresent(IDXGISwapChain *p, UINT a, UINT b)
{
    return dxgi_hook::instance()->present_hook(p, a, b);
}

HRESULT dxgi_hook::SwapChainResizeBuffers(IDXGISwapChain *p, UINT a, UINT b, UINT c, DXGI_FORMAT d, UINT e)
{
    return dxgi_hook::instance()->resize_buffers_hook(p, a, b, c, d, e);
}

HRESULT dxgi_hook::SwapChainResizeTarget(IDXGISwapChain *p, const DXGI_MODE_DESC *a)
{
    return dxgi_hook::instance()->resize_target_hook(p, a);
}

HRESULT dxgi_hook::SwapChainPresent1(IDXGISwapChain1 *p, UINT a, UINT b, const DXGI_PRESENT_PARAMETERS *c)
{
    return dxgi_hook::instance()->present1_hook(p, a, b, c);
}

HRESULT dxgi_hook::present_hook(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags)
{
    // do someting
    // ...
    hookD3D11Present(pSwapChain, SyncInterval, Flags);

    HRESULT hr = m_dxgiSwapChainPresentHook->callOrginal<HRESULT>(pSwapChain, SyncInterval, Flags);

    if (FAILED(hr))
    {
        LOG << "dxgi_hook::present_hook: " << hr << "\n";
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            ID3D11Device* pd3d11dev;
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pd3d11dev)))
            {
                hr = pd3d11dev->GetDeviceRemovedReason();
                LOG << "dxgi_hook::present_hook: GetDeviceRemovedReason: " << hr << "\n";
                pd3d11dev->Release();
            }
        }
    }

    // do something
    // ...

    return hr;
}

HRESULT dxgi_hook::present1_hook(IDXGISwapChain1 *pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters)
{
    // do someting
    // ...

    HRESULT hr = m_dxgiSwapChainPresent1Hook->callOrginal<HRESULT>(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);

    if (FAILED(hr))
    {
        LOG << "dxgi_hook::present_hook: " << hr << "\n";
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            ID3D11Device* pd3d11dev;
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pd3d11dev)))
            {
                hr = pd3d11dev->GetDeviceRemovedReason();
                LOG << "dxgi_hook::present_hook: GetDeviceRemovedReason: " << hr << "\n";
                pd3d11dev->Release();
            }
        }
    }

    // do something
    // ...

    return hr;
}

HRESULT dxgi_hook::resize_buffers_hook(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // do something
    // ...
    LOG << "dxgi_hook::resize_buffers_hook\n";

    return m_dxgiSwapChainResizeBuffersHook->callOrginal<HRESULT>(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT dxgi_hook::resize_target_hook(IDXGISwapChain *pSwapChain, const DXGI_MODE_DESC *pNewTargetParameters)
{
    // do something
    // ...
    LOG << "dxgi_hook::resize_target_hook\n";

    return m_dxgiSwapChainResizeTargetHook->callOrginal<HRESULT>(pSwapChain, pNewTargetParameters);
}

