#include "stdafx.h"
#include <fstream>
#include <vector>

const char* memscan_memory_dumpfile = "memory_dump.log";

void DumpMemory(HANDLE hProcess, const std::string& dump, const std::string& dump_size)
{
    char* end_ptr = nullptr;
    uint64_t address = strtoull(dump.c_str(), &end_ptr, 16);
    uint32_t size = strtoul(dump_size.c_str(), &end_ptr, 0);

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T mbi_size = VirtualQueryEx(hProcess, (uint8_t*)address, &mbi, sizeof(mbi));
    if (mbi_size == 0)
    {
        return; // something wrong
    }

    if (mbi.Protect == PAGE_READWRITE &&
        mbi.State == MEM_COMMIT &&
        (mbi.Type == MEM_MAPPED || mbi.Type == MEM_PRIVATE))
    {
        SIZE_T bytes_read;
        std::vector<uint8_t> buffer;
        buffer.resize(mbi.RegionSize);
        ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytes_read);
        buffer.resize(bytes_read);

        std::ofstream file(memscan_memory_dumpfile, std::ios_base::out);

        char exeName[MAX_PATH];
        DWORD nameSize = MAX_PATH;
        bool b = QueryFullProcessImageNameA(hProcess, 0, exeName, &nameSize);
        if (b)
        {
            file << "Process Name: " << exeName << "\n";
        }

        auto current = buffer.data() + (address - (uint64_t)mbi.BaseAddress);
        while (size > 0)
        {
            file << format_string("%p ", (uint8_t*)mbi.BaseAddress + (current - buffer.data()));
            for (int i = 0; i < 16; i++)
            {
                file << format_string("%02X ", *current++);
            }
            file << std::endl;
            size -= 16;
        }
    }
}
