#pragma once

#include <dxgi1_2.h>
#include <memory>

#include "../minhook/include/MinHook.h"
#include "apihook.hpp"

typedef HRESULT(STDMETHODCALLTYPE *DXGISwapChainPresentType)(IDXGISwapChain *, UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE *DXGISwapChainResizeBuffersType)(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT);
typedef HRESULT(STDMETHODCALLTYPE *DXGISwapChainResizeTargetType)(IDXGISwapChain *, const DXGI_MODE_DESC *);
typedef HRESULT(STDMETHODCALLTYPE *DXGISwapChainPresent1Type)(IDXGISwapChain1 *swapChain, UINT SyncInterval, UINT PresentFlags, _In_ const DXGI_PRESENT_PARAMETERS *pPresentParameters);


class dxgi_hook
{
public:
    static dxgi_hook* instance();
    ~dxgi_hook();

    bool init();
    void deinit();

private:
    dxgi_hook() = default;
    dxgi_hook(const dxgi_hook&) = delete;

    static HRESULT STDMETHODCALLTYPE SwapChainPresent(IDXGISwapChain *, UINT, UINT);
    static HRESULT STDMETHODCALLTYPE SwapChainResizeBuffers(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    static HRESULT STDMETHODCALLTYPE SwapChainResizeTarget(IDXGISwapChain *, const DXGI_MODE_DESC *);
    static HRESULT STDMETHODCALLTYPE SwapChainPresent1(IDXGISwapChain1 *, UINT, UINT, const DXGI_PRESENT_PARAMETERS *);

    HRESULT present_hook(IDXGISwapChain *swap, UINT SyncInterval, UINT Flags);
    HRESULT present1_hook(IDXGISwapChain1 *swap, UINT SyncInterval, UINT PresentFlags, _In_ const DXGI_PRESENT_PARAMETERS *pPresentParameters);
    HRESULT resize_buffers_hook(IDXGISwapChain *swap, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    HRESULT resize_target_hook(IDXGISwapChain *swap, __in const DXGI_MODE_DESC *pNewTargetParameters);

private:
    std::unique_ptr<ApiHook<DXGISwapChainPresentType>> m_dxgiSwapChainPresentHook;
    std::unique_ptr<ApiHook<DXGISwapChainResizeBuffersType>> m_dxgiSwapChainResizeBuffersHook;
    std::unique_ptr<ApiHook<DXGISwapChainResizeTargetType>> m_dxgiSwapChainResizeTargetHook;
    std::unique_ptr<ApiHook<DXGISwapChainPresent1Type>> m_dxgiSwapChainPresent1Hook;
};
