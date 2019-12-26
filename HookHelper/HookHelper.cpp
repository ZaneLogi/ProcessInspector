// HookHelper.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <ctime>
#include "HookHelper.h"
#include "CommandLine.h"



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

bool InjectDll(DWORD pid, LPCTSTR dllPath)
{
    DWORD threadID = 0;
    HHOOK hook = nullptr;
    HMODULE module = LoadLibrary(dllPath);

    if (module == nullptr)
    {
        TRACE(L"Can't load the dll module!!!\n");
        goto err;
    }

#ifdef _WIN64
    HOOKPROC proc = (HOOKPROC)GetProcAddress(module, "CallWndProc");
#else
    HOOKPROC proc = (HOOKPROC)GetProcAddress(module, "_CallWndProc@12");
#endif

    if (proc == nullptr)
    {
        TRACE(L"Can't get the address of the function CallWndProc from the dll module!!!\n");
        goto err;
    }

    if (pid != 0)
    {
        threadID = GetProcessMainThreadID(pid);
        if (threadID == 0)
        {
            TRACE(L"Can't get the main thread id from the pid %d!!!", pid);
            goto err;
        }
    }

    hook = SetWindowsHookEx(WH_GETMESSAGE, proc, module, threadID);

    if (hook == nullptr)
    {
        TRACE(L"Failed to call SetWindowsHookEx!!!\n");
        goto err;
    }

    // before UnhookWindowsHookEx, need to make sure the injection be processed
    const int WAIT_COUNT = 10;
    for (auto i = 0; i < WAIT_COUNT; i++)
    {
        // post the message to the thread to make sure the hook function in the dll be called
        PostThreadMessage(threadID, WM_USER + 0x123, 0, (LPARAM)hook);
        Sleep(500);
    }

    UnhookWindowsHookEx(hook);

    return true;

err:
    if (module)
    {
        FreeLibrary(module);
    }

    return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef _WIN64
    const auto LOG_FILE_NAME = L"HookHelper64.log";
    const auto DEFAULT_DLL_NAME = L"InjectionDll64.dll";
#else
    const auto LOG_FILE_NAME = L"HookHelper32.log";
    const auto DEFAULT_DLL_NAME = L"InjectionDll32.dll";
#endif

    CommandLineParams cmdLineParams;

    int nCmdLineArgs = 0;
    LPWSTR* pAllocCmdLine = CommandLineToArgvW(lpCmdLine, &nCmdLineArgs);
    if (pAllocCmdLine)
    {
        CommandLine::parse(nCmdLineArgs, pAllocCmdLine, cmdLineParams);
        LocalFree(pAllocCmdLine);
    }

    if (cmdLineParams.createLog)
    {
        // Enable file logging
        PWSTR pszDocumentsPath = NULL;
        SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pszDocumentsPath);
        TCHAR szLogFilePath[MAX_PATH];
        ZeroMemory(szLogFilePath, sizeof(szLogFilePath));
        if (pszDocumentsPath)
        {
            wcscpy_s(szLogFilePath, pszDocumentsPath);
            PathAppend(szLogFilePath, LOG_FILE_NAME);
            LogiTraceEnableFileLogging(szLogFilePath, TRUE);
            CoTaskMemFree(pszDocumentsPath);

            time_t ltime;
            tm current_tm;
            wchar_t buf[64];
            time(&ltime);
            localtime_s(&current_tm, &ltime);
            wcsftime(buf, _countof(buf), L"%m/%d/%Y %H:%M:%S", &current_tm);
            TRACE(L"\n\n[%s]\n\n", buf);
        }
    }

    if (cmdLineParams.dll_path.empty())
    {
        cmdLineParams.dll_path = DEFAULT_DLL_NAME;
    }

    if (cmdLineParams.process_id == 0)
    {
        TRACE(L"Missing the process id!!!\n");
        return -1;
    }

    TRACE(L"Inject '%s' to process %d\n", cmdLineParams.dll_path.c_str(), cmdLineParams.process_id);

    if (!InjectDll(cmdLineParams.process_id, cmdLineParams.dll_path.c_str()))
    {
        TRACE(L"Failed :(\n");
        return -2;
    }

    TRACE(L"Success :)\n");

    return 0;

}




