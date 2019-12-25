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

DWORD GetProcessMainThreadID(DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    THREADENTRY32 threadentry;
    threadentry.dwSize = sizeof(threadentry);

    BOOL hasNext = Thread32First(snapshot, &threadentry);
    DWORD mainThreadID = 0;
    ULONGLONG minCreateTime = MAXULONGLONG;

    do
    {
        // find the thread owned by the process id and its creation time is the earliest one.
        if (threadentry.th32OwnerProcessID == pid)
        {
            HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,
                TRUE, threadentry.th32ThreadID);
            if (hThread)
            {
                FILETIME afTimes[4] = { 0 };
                if (GetThreadTimes(hThread,
                    &afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3]))
                {
                    ULARGE_INTEGER time = { afTimes[0].dwLowDateTime, afTimes[0].dwHighDateTime };
                    if (time.QuadPart && time.QuadPart < minCreateTime)
                    {
                        minCreateTime = time.QuadPart;
                        mainThreadID = threadentry.th32ThreadID;
                    }
                }
                CloseHandle(hThread);
            }
        }
        hasNext = Thread32Next(snapshot, &threadentry);
    } while (hasNext);

    CloseHandle(snapshot);

    return mainThreadID;
}


HHOOK InjectDll(DWORD pid, LPCTSTR dllPath)
{
    HMODULE module = LoadLibrary(dllPath);
    if (module == NULL)
    {
        printf("Failed to load DLL, code = %u\n", GetLastError());
        return NULL;
    }

#ifdef _WIN64
    HOOKPROC proc = (HOOKPROC)GetProcAddress(module, "CallWndProc");
#else
    HOOKPROC proc = (HOOKPROC)GetProcAddress(module, "_CallWndProc@12");
#endif

    if (proc == NULL)
    {
        printf("Failed to get the hook function address, code = %u\n", GetLastError());
        return NULL;
    }

    DWORD threadID = 0;
    if (pid != 0)
    {
        threadID = GetProcessMainThreadID(pid);
        printf("MainThreadID: %d\n", threadID);

        if (threadID == 0)
        {
            printf("Failed to get the thread ID.\n");
            return NULL;
        }
    }

    HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, proc, module, threadID);
    printf("hook: %p\n", hook);

    FreeLibrary(module);

    // before UnhookWindowsHookEx, need to make sure the injection is done
    if (hook  != nullptr)
    {
        const int WAIT_COUNT = 10;
        for (auto i = 0; i < WAIT_COUNT; i++)
        {
            // post the message to the thread to make sure the hook function in the dll be called
            PostThreadMessage(threadID, WM_USER + 0x123, 0, (LPARAM)hook);
            Sleep(500);
        }
    }

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
        _tprintf(_T("pid=%d\n"), pid);
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

    if (pid == 0)
    {
        _tprintf(_T("pid == 0\n"));
        return -3;
    }

    EnablePrivilege(TRUE);

#ifdef _WIN64
    PCWSTR dllname = L"InjectionDll64.dll";
#else
    PCTSTR dllname = _T("InjectionDll32.dll");
#endif


    // target name: "Direct3D 11 Application"  "Our Direct3D Program"
    auto hook = SetHook(dllname, pid);
    if (hook != nullptr)
    {
        //printf("Please hit return/enter key to unload DLL.\n");
        //getchar();
        //Sleep(5 * 1000);
        UnhookWindowsHookEx(hook);
    }
    else
    {
        printf("Failed too hook!!!\n");
    }

    return 0;
}

