// MsgHookDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MsgHookProc.h"
#include "Logger.h"

extern "C" MSGHOOKDLL_API
LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
{
    static bool hooking = true;
    MSG *msg = (MSG *)lParam;

    if (hooking && msg->message == (WM_USER + 0x123))
    {
        hooking = false;
        HHOOK hHook = (HHOOK)msg->lParam;
        HWND hWnd = msg->hwnd;
        LOG << "Hooking the process, hhook = " << msg->lParam << ", hwnd = " << (DWORD_PTR)msg->hwnd << "\n";
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}