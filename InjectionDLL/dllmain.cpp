// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

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

static void hook(void* originalFunction, void* hookFunction, BYTE* oldCode)
{
    JmpCode code((uintptr_t)originalFunction, (uintptr_t)hookFunction);
    DWORD oldProtect, oldProtect2;
    VirtualProtect(originalFunction, sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(oldCode, originalFunction, sizeof(code));
    memcpy(originalFunction, &code, sizeof(code));
    VirtualProtect(originalFunction, sizeof(code), oldProtect, &oldProtect2);
}


static void unhook(void* originalFunction, BYTE* oldCode)
{
    DWORD oldProtect, oldProtect2;
    VirtualProtect(originalFunction, sizeof(JmpCode), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(originalFunction, oldCode, sizeof(JmpCode));
    VirtualProtect(originalFunction, sizeof(JmpCode), oldProtect, &oldProtect2);
}





BYTE terminateProcessOldCode[sizeof(JmpCode)];

BOOL WINAPI MyTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
    return FALSE; // force not to teminate the process
}


#if 0

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
    TCHAR processPath[MAX_PATH];

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileName(GetModuleHandle(NULL), processPath, MAX_PATH);

        MessageBox(NULL, _T("DLL injection"), processPath, MB_OK);

        hook(GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "TerminateProcess"), MyTerminateProcess, terminateProcessOldCode);

        if (GetModuleHandle(_T("d3d9.dll")))
        {
            MessageBox(NULL, _T("has d3d9.dll"), processPath, MB_OK);
        }
        if (GetModuleHandle(_T("d3d11.dll")))
        {
            MessageBox(NULL, _T("has d3d11.dll"), processPath, MB_OK);
        }

        break;

    case DLL_PROCESS_DETACH:
        GetModuleFileName(GetModuleHandle(NULL), processPath, MAX_PATH);
        MessageBox(NULL, _T("DLL unload"), processPath, MB_OK);

        unhook(GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "TerminateProcess"), terminateProcessOldCode);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

#endif
