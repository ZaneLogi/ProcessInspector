// RemoteThreadInjection.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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

// Inject the DLL, and return the handle of the module (64bit process can only return low 32bits of the handle)
HMODULE InjectDll(HANDLE process, LPCTSTR dllPath)
{
    DWORD dllPathSize = ((DWORD)_tcslen(dllPath) + 1) * sizeof(TCHAR);

    // allocate memory for the dll path
    void* remoteMemory = VirtualAllocEx(process, NULL, dllPathSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (remoteMemory == NULL)
    {
        printf("Failed to call VirtualAllocEx, code = %u\n", GetLastError());
        return 0;
    }

    // write the dll path to the allocated memory
    if (!WriteProcessMemory(process, remoteMemory, dllPath, dllPathSize, NULL))
    {
        printf("Failed to call WriteProcessMemory, code = %u\n", GetLastError());
        return 0;
    }

    // create the remote thread to call LoadLibrary
    HANDLE remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, remoteMemory, 0, NULL);
    if (remoteThread == NULL)
    {
        printf("Failed to call CreateRemoteThread, code = %u\n", GetLastError());
        return NULL;
    }

    // wait the end of the remote thread
    WaitForSingleObject(remoteThread, INFINITE);

    // get the handle of the DLL in the remote thread
    DWORD remoteModule;
    GetExitCodeThread(remoteThread, &remoteModule);

    // cloase the handle of the remote thread
    CloseHandle(remoteThread);
    VirtualFreeEx(process, remoteMemory, dllPathSize, MEM_DECOMMIT);

    return (HMODULE)remoteModule;
}

BOOL FreeRemoteDll(HANDLE process, HMODULE remoteModule)
{
    // create a remote thread to call FreeLibrary
    HANDLE remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, (LPVOID)remoteModule, 0, NULL);
    if (remoteThread == NULL)
    {
        printf("Failed to call CreateRemoteThread, code = %u\n", GetLastError());
        return FALSE;
    }
    // wait the end of the remote thread
    WaitForSingleObject(remoteThread, INFINITE);
    // get the result
    DWORD result;
    GetExitCodeThread(remoteThread, &result);

    // free the handle of the thread
    CloseHandle(remoteThread);
    return result != 0;
}

#ifdef _WIN64
#include <tlhelp32.h>
HMODULE GetRemoteModuleHandle(DWORD pid, LPCTSTR moduleName)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 moduleentry;
    moduleentry.dwSize = sizeof(moduleentry);

    BOOL hasNext = Module32First(snapshot, &moduleentry);
    HMODULE handle = NULL;
    do
    {
        if (_tcsicmp(moduleentry.szModule, moduleName) == 0)
        {
            handle = moduleentry.hModule;
            break;
        }
        hasNext = Module32Next(snapshot, &moduleentry);
    } while (hasNext);

    CloseHandle(snapshot);
    return handle;
}
#endif




int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Syntax: RemoteThreadInjection [Window Title]\n");
        return -1;
    }

    EnablePrivilege(TRUE);

    const auto application_name = argv[1];
    //const auto application_name = _T("Direct3D 11 Application");
    //const auto application_name = _T("Our Direct3D Program");
    HWND hwnd = FindWindowA(NULL, application_name);
    if (hwnd == nullptr)
    {
        printf("Can't find the window '%s'!!!\n", application_name);
        return -2;
    }

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (process == NULL)
    {
        printf("Failed to call OpenProcess, pid = %d, code = %u\n", pid, GetLastError());
        return -3;
    }

    // InjectionDll.dll should be at the same folder of this process
    TCHAR dllPath[MAX_PATH]; // need the absolute path
    GetCurrentDirectory(_countof(dllPath), dllPath);
    _tcscat_s(dllPath, _T("\\InjectionDll.dll"));

    HMODULE remoteModule = InjectDll(process, dllPath);
    if (remoteModule == NULL)
    {
        CloseHandle(process);
        printf("Failed to inject DLL\n");
        return -4;
    }

#ifdef _WIN64
    remoteModule = GetRemoteModuleHandle(pid, _T("InjectionDll.dll"));
    printf("module handle:0x%08X%08X\n", *((DWORD*)&remoteModule + 1), *(DWORD*)&remoteModule);
#else
    printf("module handle:0x%08X\n", (DWORD)remoteModule);
#endif


    printf("Press enter/return to unload DLL\n");
    getchar();

    if (!FreeRemoteDll(process, remoteModule))
    {
        CloseHandle(process);
        return -5;
    }

    CloseHandle(process);

    return 0;
}

