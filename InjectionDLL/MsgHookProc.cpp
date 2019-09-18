// MsgHookDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MsgHookProc.h"


extern "C" MSGHOOKDLL_API
LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(NULL, code, wParam, lParam);
}