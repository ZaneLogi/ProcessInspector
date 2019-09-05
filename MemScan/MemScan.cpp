// MemScan.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CommandLine.h"
#include <fstream>
#include <strstream>
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

std::vector<uint8_t> get_pattern(const std::string& data, const std::string& type)
{
    char* endptr;
    std::vector<uint8_t> pattern;
    if (type == "int8")
    {
        int8_t value = (int8_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(*(uint8_t*)&value);
    }
    else if (type == "uint8")
    {
        uint8_t value = (uint8_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(value);
    }
    else if (type == "int16")
    {
        int16_t value = (int16_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
    }
    else if (type == "uint16")
    {
        uint16_t value = (uint16_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
    }
    else if (type == "int32")
    {
        int32_t value = (int32_t)strtol(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
        pattern.push_back(((uint8_t*)&value)[2]);
        pattern.push_back(((uint8_t*)&value)[3]);
    }
    else if (type == "uint32")
    {
        uint32_t value = (uint32_t)strtoul(data.c_str(), &endptr, 0);
        pattern.push_back(((uint8_t*)&value)[0]);
        pattern.push_back(((uint8_t*)&value)[1]);
        pattern.push_back(((uint8_t*)&value)[2]);
        pattern.push_back(((uint8_t*)&value)[3]);
    }
    else
    {
        // treat it as string
        std::for_each(data.begin(), data.end(), [&pattern](auto c) { pattern.push_back(*(uint8_t*)&c); });
    }

    return pattern;
}

template <class InIter1, class InIter2>
std::vector<uint8_t*> find_all(uint8_t *base, InIter1 buf_start, InIter1 buf_end, InIter2 pat_start, InIter2 pat_end)
{
    std::vector<uint8_t*> out;

    for (InIter1 pos = buf_start;
        buf_end != (pos = std::search(pos, buf_end, pat_start, pat_end));
        ++pos)
    {
        out.push_back(base + (pos - buf_start));
    }

    return out;
}

void InitSearch(HANDLE hProcess, const std::string& init_value, const std::string& type)
{
    std::ofstream init_log("init.log", std::ios_base::out);
    std::ofstream file("memscan.bin", std::ios_base::binary | std::ios_base::out);

    auto pattern = get_pattern(init_value, type);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    auto proc_cur_address = (uint8_t*)sys_info.lpMinimumApplicationAddress;
    auto proc_max_address = (uint8_t*)sys_info.lpMaximumApplicationAddress;

    std::vector<uint8_t> buffer;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mbi, sizeof(mbi));
        if (size == 0)
        {
            break;
        }

        if (mbi.Protect == PAGE_READWRITE &&
            mbi.State == MEM_COMMIT &&
            (mbi.Type == MEM_MAPPED || mbi.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(mbi.RegionSize);
            ReadProcessMemory(hProcess, proc_cur_address, buffer.data(), mbi.RegionSize, &bytes_read);
            buffer.resize(bytes_read);

            auto results = find_all(proc_cur_address, buffer.begin(), buffer.end(), pattern.begin(), pattern.end());
            uint32_t count = (uint32_t)results.size();
            if (count > 0)
            {
                // number of addresses
                file.write((const char*)&count, sizeof(count));
                // the memory base address
                uint64_t mem_base_address = (uint64_t)proc_cur_address;
                file.write((const char*)&mem_base_address, sizeof(void*));

                init_log << format_string("%p", (PBYTE)mbi.BaseAddress) << std::endl;

                std::for_each(results.begin(), results.end(), [&file, &init_log](auto d) {
                    // address which the data found
                    uint64_t v = (uint64_t)d;
                    file.write((const char*)&v, sizeof(void*));

                    init_log << format_string("    %p", d) << std::endl;
                });
            }
        }

        proc_cur_address += mbi.RegionSize;
    }

    file.close();
}

void NextSearch(HANDLE hProcess, const std::string& next_value, const std::string& type)
{
    std::ofstream next_log("next.log", std::ios_base::out);
    auto pattern = get_pattern(next_value, type);

    // read in the previous data
    std::ifstream in_file("memscan.log", std::ios_base::binary | std::ios_base::in);
    if (!in_file.is_open())
    {
        return;
    }
    in_file.seekg(0, std::ios_base::end);
    uint32_t file_size = (uint32_t)in_file.tellg();
    if (file_size == 0)
    {
        return;
    }

    std::vector<uint8_t> buffer;
    buffer.resize(file_size);
    in_file.seekg(0, std::ios_base::beg);
    in_file.read((char*)buffer.data(), buffer.size());
    in_file.close();

    std::strstream tmp_file;
    tmp_file.write((const char*)buffer.data(), buffer.size());

    // prepare the current write out
    std::ofstream out_file("memscan.log", std::ios_base::binary | std::ios_base::out);

    while (!tmp_file.eof() && file_size > 0)
    {
        uint32_t count;
        tmp_file.read((char*)&count, sizeof(count));
        file_size -= sizeof(count);

        uint64_t mem_base_address = 0;
        tmp_file.read((char*)&mem_base_address, sizeof(void*));
        file_size -= sizeof(void*);

        std::vector<uint8_t*> candidates;
        for (uint32_t i = 0; i < count; i++)
        {
            uint64_t address = 0;
            tmp_file.read((char*)&address, sizeof(void*));
            file_size -= sizeof(void*);
            candidates.push_back((uint8_t*)address);
        }

        uint8_t* proc_cur_address = (uint8_t*)mem_base_address;

        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mbi, sizeof(mbi));
        if (size == 0)
        {
            break; // something wrong
        }

        if (mbi.Protect == PAGE_READWRITE &&
            mbi.State == MEM_COMMIT &&
            (mbi.Type == MEM_MAPPED || mbi.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(mbi.RegionSize);
            ReadProcessMemory(hProcess, proc_cur_address, buffer.data(), mbi.RegionSize, &bytes_read);
            buffer.resize(bytes_read);

            std::vector<uint8_t*> results;
            for (int i = 0; i < (int)candidates.size(); i++)
            {
                if (0 == memcmp(buffer.data() + (candidates[i] - proc_cur_address), pattern.data(), pattern.size()))
                {
                    results.push_back(candidates[i]);
                }
            }

            uint32_t count = (uint32_t)results.size();
            if (count > 0)
            {
                // number of addresses
                out_file.write((const char*)&count, sizeof(count));
                // the memory base address
                uint64_t mem_base_address = (uint64_t)proc_cur_address;
                out_file.write((const char*)&mem_base_address, sizeof(void*));

                next_log << format_string("%p", (PBYTE)mbi.BaseAddress) << std::endl;

                std::for_each(results.begin(), results.end(), [&out_file, &next_log](auto d) {
                    // address which the data found
                    uint64_t v = (uint64_t)d;
                    out_file.write((const char*)&v, sizeof(void*));

                    next_log << format_string("    %p", d) << std::endl;
                });
            }

        }
    }

    out_file.close();
}

int main(int argc, char** argv)
{
    CommandLine cl(argc, argv);
    auto pid = cl.get_value("pid");
    auto find = cl.get_value("find");
    auto replace = cl.get_value("replace");
    auto init_value = cl.get_value("init");
    auto next_value = cl.get_value("next");
    auto type = cl.get_value("type");

    if (!pid.empty())
    {
        if (replace.length() != find.length())
        {
            replace.clear(); // both length should be same, otherwise, don't do replacement
        }

        if (!init_value.empty() || !next_value.empty())
        {
            replace.clear(); // no replacement
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

        if (!init_value.empty())
        {
            InitSearch(hProcess, init_value, type);
        }
        else if (!next_value.empty())
        {
            NextSearch(hProcess, next_value, type);
        }
        else if (!find.empty())
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

