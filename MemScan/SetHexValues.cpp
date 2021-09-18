#include "stdafx.h"
#include <vector>

void SetAddressHexValues(HANDLE hProcess, const std::string& address_string, const std::string& hex_string)
{
    char* endptr;
    std::vector<uint8_t> hexValues;

    uint64_t address = strtoull(address_string.c_str(), &endptr, 16);

    for (int i = 0; i < hex_string.length(); i += 2)
    {
        uint8_t value = (uint8_t)strtoul(hex_string.substr(i, 2).c_str(), &endptr, 16);
        hexValues.push_back(value);
    }

    SIZE_T bytes_written;
    BOOL b;
    unsigned long oldProtect;
    // need PROCESS_VM_OPERATION
    b = VirtualProtectEx(hProcess, (void*)address, hexValues.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
    // need PROCESS_VM_WRITE
    b = WriteProcessMemory(hProcess, (void*)address, hexValues.data(), hexValues.size(), &bytes_written);
    b = VirtualProtectEx(hProcess, (void*)address, hexValues.size(), oldProtect, &oldProtect);
}