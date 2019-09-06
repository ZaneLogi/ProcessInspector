#include "stdafx.h"
#include <fstream>
#include <algorithm>
#include <vector>

const char* memscan_find_logfile = "find.log";

void FindAndReplace(HANDLE hProcess, const std::string& find, const std::string& replace)
{
    std::ofstream os(memscan_find_logfile, std::ios_base::out);

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
