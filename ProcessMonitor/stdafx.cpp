
// stdafx.cpp : source file that includes just the standard includes
// ProcessMonitor.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

std::wstring get_executable_path_by_process_id(int process_id)
{
    DWORD application_path_dword = MAX_PATH;
    WCHAR application_path_cstr[MAX_PATH] = {};
    HANDLE process_handle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (QueryFullProcessImageNameW(process_handle, 0, reinterpret_cast<LPWSTR>(application_path_cstr),
        &application_path_dword))
    {
        return application_path_cstr;
    }

    return std::wstring();
}