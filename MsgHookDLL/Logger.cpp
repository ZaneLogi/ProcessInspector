#include "stdafx.h"
#include "Logger.h"

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::addListener(Logger::Listener& listener)
{
    listeners.push_back(&listener);
}

void Logger::notifyListeners(const std::string& text)
{
    for (auto l : listeners)
    {
        l->onLog(text);
    }
}

void Logger::removeListener(Logger::Listener& listener)
{
    listeners.remove(&listener);
}





FileLogger::FileLogger(const char* log_file_name)
{
    m_file.open(log_file_name, std::ios_base::out | std::ios_base::app);
    LOG.addListener(*this);
}

FileLogger::~FileLogger()
{
    LOG.removeListener(*this);
    m_file.close();
}

void FileLogger::onLog(const std::string& text)
{
    m_file << text;
}





void OdsLogger::onLog(const std::string& text)
{
    OutputDebugStringA(text.c_str());
}

OdsLogger::OdsLogger()
{
    LOG.addListener(*this);
}

OdsLogger::~OdsLogger()
{
    LOG.removeListener(*this);
}