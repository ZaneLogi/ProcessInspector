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
        printf("MainThreadID: %d\n", threadID);

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




HHOOK SetHook(LPCTSTR injected_dll_name, DWORD pid)
{
    // if pid is 0, global hook
    printf("PID = %d\n", pid);

    HHOOK hook = InjectDll(pid, injected_dll_name);
    if (hook == NULL)
        return nullptr;

    return hook;
}

DWORD GetApplicationProcessID(LPCTSTR application_name)
{
    _tprintf(_T("Application Name: %s\n"), application_name);

    DWORD pid = 0;
    HWND hwnd = FindWindow(NULL, application_name);
    if (hwnd == NULL)
    {
        _tprintf(_T("Failed to find the windows, code = %u\n"), GetLastError());
        return 0;
    }

    GetWindowThreadProcessId(hwnd, &pid);

    return pid;
}


int _tmain(int argc, TCHAR** argv)
{
    if (argc != 3)
    {
        _tprintf(_T("Syntax: MsgHook pid= [pid] or title= [application title]\n"));
        return -1;
    }

    DWORD pid = 0;

    if (_tcsicmp(argv[1], _T("pid=")) == 0)
    {
        pid = _tcstoul(argv[2], nullptr, 10);
        _tprintf(_T("pid=%d\n", pid));
    }
    else if (_tcsicmp(argv[1], _T("title=")) == 0)
    {
        pid = GetApplicationProcessID(argv[2]);
        _tprintf(_T("application pid = %d\n"), pid);
    }
    else
    {
        _tprintf(_T("missing'pid=' or 'title='\n"));
        return -2;
    }

    EnablePrivilege(TRUE);
    // target name: "Direct3D 11 Application"  "Our Direct3D Program"
    auto hook = SetHook(_T("InjectionDll.dll"), pid);
    if (hook != nullptr)
    {
        printf("Please hit return/enter key to unload DLL.\n");
        getchar();

        UnhookWindowsHookEx(hook);
    }

    return 0;
}

