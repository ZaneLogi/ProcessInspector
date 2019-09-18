// MsgHook.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tlhelp32.h>

BOOL EnablePrivilege(BOOL enable)
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
        return FALSE;

    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
        return FALSE;

    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
        return FALSE;

    CloseHandle(hToken);

    return TRUE;
}

DWORD GetProcessThreadID(DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    THREADENTRY32 threadentry;
    threadentry.dwSize = sizeof(threadentry);

    BOOL hasNext = Thread32First(snapshot, &threadentry);
    DWORD threadID = 0;
    do
    {
        if (threadentry.th32OwnerProcessID == pid)
        {
            threadID = threadentry.th32ThreadID;
            break;
        }
        hasNext = Thread32Next(snapshot, &threadentry);
    } while (hasNext);

    CloseHandle(snapshot);
    return threadID;
}


HHOOK InjectDll(DWORD pid, LPCTSTR dllPath)
{
    HMODULE module = LoadLibrary(dllPath);
    if (module == NULL)
    {
        printf("Failed to load DLL, code = %u\n", GetLastError());
        return NULL;
    }

    HOOKPROC proc = (HOOKPROC)GetProcAddress(module, "CallWndProc");
    if (proc == NULL)
    {
        printf("Failed to get the hook function address, code = %u\n", GetLastError());
        return NULL;
    }

    DWORD threadID = 0;
    if (pid != 0)
    {
        threadID = GetProcessThreadID(pid);
        if (threadID == 0)
        {
            printf("Failed to get the thread ID.\n");
            return NULL;
        }
    }

    HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, proc, module, threadID);

    FreeLibrary(module);

    return hook;
}




HHOOK SetHook(LPCTSTR injected_dll_name, LPCTSTR application_name = nullptr)
{
    DWORD pid = 0;

    if (application_name != nullptr)
    {
        HWND hwnd = FindWindow(NULL, application_name);
        if (hwnd == NULL)
        {
            printf("Failed to find the windows, code = %u\n", GetLastError());
            return nullptr;
        }

        GetWindowThreadProcessId(hwnd, &pid);
    }
    else
    {
        pid = 0; // global hook
    }

    HHOOK hook = InjectDll(pid, injected_dll_name);
    if (hook == NULL)
        return nullptr;

    return hook;
}


int main()
{
    EnablePrivilege(TRUE);
    // target name: "Direct3D 11 Application"  "Our Direct3D Program"
    auto hook = SetHook(_T("InjectionDll.dll"), _T("Our Direct3D Program"));
    if (hook != nullptr)
    {
        printf("Please hit return/enter key to unload DLL.\n");
        getchar();

        UnhookWindowsHookEx(hook);
    }

    return 0;
}

