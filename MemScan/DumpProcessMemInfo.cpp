#include "stdafx.h"
#include <cassert>
#include <fstream>

const char* memscan_process_logfile = "process.log";

void DumpProcessMemory(HANDLE hProcess)
{
    std::ofstream os(memscan_process_logfile, std::ios_base::out);

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
