// MemScan.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CommandLine.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <cassert>

void DumpProcessMemory(HANDLE hProcess, std::ostream& os)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    PBYTE proc_cur_address = (PBYTE)sys_info.lpMinimumApplicationAddress;
    PBYTE proc_max_address = (PBYTE)sys_info.lpMaximumApplicationAddress;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mbi, sizeof(mbi));
        if (size == 0)
        {
            // it is possible that proc_cur_address can't be greater than proc_max_address,
            // and ERROR_INVALID_PARAMETER
            DWORD err = GetLastError();
            assert(err == ERROR_INVALID_PARAMETER);
            break;
        }

        auto out = format_string("%p %08x ",
            mbi.BaseAddress,
            mbi.RegionSize);

        os << out;

        switch (mbi.State) {
        case MEM_COMMIT:
            os << "Commit  ";
            break;
        case MEM_FREE:
            os << "Free    ";
            break;
        case MEM_RESERVE:
            os << "Reserve ";
            break;
        default:
            os << "?       ";
            break;
        }

        switch (mbi.Type) {
        case MEM_IMAGE:
            os << "IMAGE   ";
            break;
        case MEM_MAPPED:
            os << "MAPPED  ";
            break;
        case MEM_PRIVATE:
            os << "PRIVATE ";
            break;
        default:
            os << "?       ";
            break;
        }

        if (mbi.Protect & PAGE_EXECUTE)             os << "E ";
        if (mbi.Protect & PAGE_EXECUTE_READ)        os << "E_R ";
        if (mbi.Protect & PAGE_EXECUTE_READWRITE)   os << "E_RW ";
        if (mbi.Protect & PAGE_EXECUTE_WRITECOPY)   os << "E_WC ";
        if (mbi.Protect & PAGE_NOACCESS)            os << "NOACCESS ";
        if (mbi.Protect & PAGE_READONLY)            os << "RO ";
        if (mbi.Protect & PAGE_READWRITE)           os << "RW ";
        if (mbi.Protect & PAGE_WRITECOPY)           os << "WRITECOPY ";
        if (mbi.Protect & PAGE_GUARD)               os << "GUARD ";
        if (mbi.Protect & PAGE_NOCACHE)             os << "NOCACHE ";
        if (mbi.Protect & PAGE_WRITECOMBINE)        os << "WRITECOMBINE ";

        os << std::endl;

        // move to the next memory chunk
        proc_cur_address += mbi.RegionSize;
    }

}

void DumpFindAndReplace(HANDLE hProcess, const std::string& find, const std::string& replace, std::ostream& os)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    PBYTE proc_cur_address = (PBYTE)sys_info.lpMinimumApplicationAddress;
    PBYTE proc_max_address = (PBYTE)sys_info.lpMaximumApplicationAddress;

    std::vector<BYTE> buffer;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mbi, sizeof(mbi));
        if (size == 0)
        {
            break;
        }

        os << format_string("search %p", mbi.BaseAddress) << std::endl;

        if (mbi.Protect == PAGE_READWRITE &&
            mbi.State == MEM_COMMIT &&
            (mbi.Type == MEM_MAPPED || mbi.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(mbi.RegionSize);
            ReadProcessMemory(hProcess, proc_cur_address, &buffer[0], mbi.RegionSize, &bytes_read);
            buffer.resize(bytes_read);

            bool replaced = false;
            for (auto pos = buffer.begin();
                buffer.end() != (pos = std::search(pos, buffer.end(), find.begin(), find.end()));
                ++pos)
            {
                auto out = format_string("%p", (PBYTE)mbi.BaseAddress + (pos - buffer.begin()));
                os << out << std::endl;

                replaced = true;
                memcpy(&(*pos), replace.data(), replace.size());
            }

            if (replaced)
            {
                SIZE_T bytes_written;
                BOOL b = WriteProcessMemory(hProcess, proc_cur_address, buffer.data(), mbi.RegionSize, &bytes_written);
                if (!b)
                {
                    os << format_string("Error %ls", GetErrorString(_T("WriteProcessMemory"))) << std::endl;
                }
            }
        }

        proc_cur_address += mbi.RegionSize;
    }
}


int main(int argc, char** argv)
{
    CommandLine cl(argc, argv);
    auto pid = cl.get_value("pid");
    auto find = cl.get_value("find");
    auto replace = cl.get_value("replace");
    if (!pid.empty())
    {
        if (replace.length() != find.length())
        {
            replace.clear(); // both length should be same, otherwise, don't do replacement
        }

        auto target_pid = atol(pid.c_str());
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

        if (!find.empty())
        {
            std::ofstream f("find.log", std::ios_base::out);
            DumpFindAndReplace(hProcess, find, replace, f);
            f.close();
        }
        else
        {
            std::ofstream f("process.log", std::ios_base::out);
            DumpProcessMemory(hProcess, f);
            f.close();
        }

        CloseHandle(hProcess);
    }
    return 0;
}

