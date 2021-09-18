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

void SetAddressHexValues(HANDLE hProcess, const std::string& address, const std::string& hex_string);


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

/*
usage examples:

parameters:
process=xxx => xxx is the process name
pid=xxx     => xxx is the pid of the process, it would be get from the task mamanger
choose either 'process' or 'pid' for the specific process

init=xxx    => xxx is the search target
next=xxx    => xxx is the search target after the first init search
type=xxx    => xxx describes what the format of the search target, it would be int8, uint8, int16, uint16, int32, uint32, data
if it is 'data', it means the search target is HEX data array

dump=xxx    => xxx is the memory address for dump
size=xxx    => xxx is the size of the memory for dump

Dump the information of the memory blocks for the specific process
memscan process=xxx

Search the specific data
memscan process=xxx init=12 type=int8
memscan process=xxx next=32 type=int8

memscan dump=000123456 size=0x200

Set hex values on the specific address
memscan process=xxx address=01234567 set=0011223344
*/
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
    auto address = cl.get_value("address");
    auto hex_string = cl.get_value("set");

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
            address.clear();
        }

        DWORD dwDesiredAccess = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
        if (!replace.empty() || !address.empty())
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
        else if (!address.empty())
        {
            SetAddressHexValues(hProcess, address, hex_string);
        }
        else
        {
            DumpProcessMemory(hProcess);
        }

        CloseHandle(hProcess);
    }
    return 0;
}

