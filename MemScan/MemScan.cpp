// MemScan.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CommandLine.h"
#include <psapi.h>


void InitSearch(HANDLE hProcess, const std::string& init_value, const std::string& type);
void NextSearch(HANDLE hProcess, const std::string& next_value, const std::string& type);

void DumpProcessMemory(HANDLE hProcess);

void DumpMemory(HANDLE hProcess, const std::string& dump, const std::string& dump_size);

void FindAndReplace(HANDLE hProcess, const std::string& find, const std::string& replace);


DWORD FindProcessId(const std::string& process_name)
{
    int nCurIndex = 0;
    DWORD aProcesses[1024];
    DWORD cbNeeded;

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return 0;
    }

    // Calculate how many process identifiers were returned.
    DWORD cProcesses = cbNeeded / sizeof(DWORD);

    DWORD found_pid = 0;
    for (DWORD pi = 0; pi < cProcesses && found_pid == 0; pi++)
    {
        HANDLE hProcess;

        auto pid = aProcesses[pi];
        if (pid == 0)
            continue;

        // Get a handle to the process.
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, pid);
        if (NULL == hProcess)
        {
            continue;
        }

        char path[MAX_PATH];
        DWORD path_length = MAX_PATH;
        if (QueryFullProcessImageNameA(hProcess, 0, path, &path_length) && path_length >= process_name.length())
        {
            if (0 == _stricmp(process_name.c_str(), &path[path_length - process_name.length()]))
            {
                found_pid = pid;
            }
        }

        // Release the handle to the process.
        CloseHandle(hProcess);
    }

    return found_pid;
}


int main(int argc, char** argv)
{
    CommandLine cl(argc, argv);
    auto process_name = cl.get_value("process");
    auto pid = cl.get_value("pid");
    auto find = cl.get_value("find");
    auto replace = cl.get_value("replace");
    auto init_value = cl.get_value("init");
    auto next_value = cl.get_value("next");
    auto type = cl.get_value("type");
    auto dump = cl.get_value("dump");
    auto dump_size = cl.get_value("size");

    DWORD target_pid = 0;
    if (!process_name.empty())
    {
        target_pid = FindProcessId(process_name);
    }
    else if (!pid.empty())
    {
        char* endptr = nullptr;
        target_pid = strtoul(pid.c_str(), &endptr, 0);
    }

    if (target_pid !=0)
    {
        if (replace.length() != find.length())
        {
            replace.clear(); // both length should be same, otherwise, don't do replacement
        }

        if (!init_value.empty() || !next_value.empty() || !dump.empty())
        {
            replace.clear(); // no replacement
        }

        DWORD dwDesiredAccess = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
        if (!replace.empty())
        {
            dwDesiredAccess = PROCESS_ALL_ACCESS;
        }

        HANDLE hProcess = OpenProcess(
            dwDesiredAccess,
            false,
            target_pid);

        if (hProcess == NULL)
        {
            printf("%ls\n", GetErrorString(_T("OpenProcess")).c_str());
            return -1;
        }

        if (!dump.empty())
        {
            DumpMemory(hProcess, dump, dump_size);
        }
        else if (!init_value.empty())
        {
            InitSearch(hProcess, init_value, type);
        }
        else if (!next_value.empty())
        {
            NextSearch(hProcess, next_value, type);
        }
        else if (!find.empty())
        {
            FindAndReplace(hProcess, find, replace);
        }
        else
        {
            DumpProcessMemory(hProcess);
        }

        CloseHandle(hProcess);
    }
    return 0;
}

