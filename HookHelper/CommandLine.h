//******    *******************************************************************
//
// CommandLine.h
//
//*************************************************************************

#include <string>

struct CommandLineParams
{
    CommandLineParams(void)
    {

    }

    bool force = false;
    std::wstring forceDir;
    bool skipDevice = false;
    int failDeviceCount = 0;
    bool createLog = true;

    unsigned long process_id = 0;
    std::wstring dll_path;
};

class CommandLine
{
public:
    CommandLine(void)
    {

    }

    static bool parse(unsigned int argc, wchar_t** argv, CommandLineParams& params)
    {
        const wchar_t* forceOption = L"/FORCE";
        const wchar_t* forceDirOption = L"/FORCEDIR=";
        const wchar_t* skipDevice = L"/SKIPDEVICE";
        const wchar_t* failDevice = L"/FAILDEVICE=";
        const wchar_t* generateLog = L"/LOG";

        const wchar_t* pidOption = L"/PID=";
        const wchar_t* dllOption = L"/DLL=";
        wchar_t* arg = NULL;

        if ((argc == 0) || (NULL == argv))
        {
            return false;
        }

        for (size_t i = 0; i < argc; i++)
        {
            if (nullptr != (arg = wcsstr(argv[i], pidOption)))
            {
                if (wcslen(arg) == wcslen(pidOption))
                {
                    // No PID given
                    continue;
                }

                params.process_id = std::stoi(arg + wcslen(pidOption), nullptr, 0);
                continue;
            }

            if (nullptr != (arg = wcsstr(argv[i], dllOption)))
            {
                if (wcslen(arg) == wcslen(dllOption))
                {
                    // No DLL given
                    continue;
                }

                params.dll_path = std::wstring(arg + wcslen(dllOption));
                continue;
            }

            if (0 == _wcsicmp(argv[i], forceOption))
            {
                params.force = true;
                continue;
            }
            if (0 == _wcsicmp(argv[i], skipDevice))
            {
                params.skipDevice = true;
                continue;
            }
            if (0 == _wcsicmp(argv[i], generateLog))
            {
                params.createLog = true;
                continue;
            }

            if (NULL != (arg = wcsstr(argv[i], forceDirOption)))
            {
                if (wcslen(arg) == wcslen(forceDirOption))
                {
                    // No directory given
                    continue;
                }

                params.forceDir = std::wstring(arg + wcslen(forceDirOption));
                params.force = true;
                continue;
            }
            if (NULL != (arg = wcsstr(argv[i], failDevice)))
            {
                if (wcslen(arg) == wcslen(failDevice))
                {
                    // No fail count given
                    continue;
                }

                params.failDeviceCount = std::stoi(std::wstring(arg + wcslen(failDevice)));
                continue;
            }
        }
        return true;
    }
};