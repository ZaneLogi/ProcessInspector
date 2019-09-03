#pragma once

#include <string>
#include <map>

class CommandLine
{
public:
    CommandLine(int argc, char** argv)
    {
        if (argc == 0 || argv == nullptr)
            return;

        m_params["application_path"] = argv[0];

        for (int i = 1; i < argc; i++)
        {
            std::string arg(argv[i]);
            auto pos = arg.find('=');
            if (pos == std::string::npos)
            {
                m_params[arg] = "";
            }
            else
            {
                m_params[arg.substr(0, pos)] = arg.substr(pos + 1);
            }
        }
    }

    bool exists(const std::string& param_name)
    {
        return m_params.find(param_name) != m_params.end();
    }

    std::string get_value(const std::string& param_name)
    {
        if (exists(param_name))
        {
            return m_params[param_name];
        }
        else
        {
            return std::string();
        }
    }

private:
    std::map<std::string, std::string> m_params;
};
