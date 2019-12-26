//*************************************************************************
//
// cHighPerformanceTimer.h
//
//*************************************************************************

#pragma once

#include <winbase.h>
#include <string>

class cHighResolutionTimer
{
public:

    cHighResolutionTimer(void)
    {
        LARGE_INTEGER pf;

        if (QueryPerformanceFrequency(&pf))
        {
            m_freq_ = 1.0 / (double)pf.QuadPart;
        }

        // Auto start
        start();
    }

    cHighResolutionTimer(char* functionName, double minEllapsedTimeInMs = 0.0f)
        : m_minEllapsedTime(minEllapsedTimeInMs), m_fnName(functionName)
    {
        LARGE_INTEGER pf;

        if (QueryPerformanceFrequency(&pf))
        {
            m_freq_ = 1.0 / (double)pf.QuadPart;
        }

        // Auto start
        start();
    }

    ~cHighResolutionTimer(void)
    {
        if (!m_fnName.empty())
        {
            double ellapsedTime = ellapsedInMilliseconds();
            if ((0.0f != m_minEllapsedTime) && (ellapsedTime > m_minEllapsedTime))
            {
            }
        }
    }

    void start(void)
    {
        QueryPerformanceCounter(&m_baseTime_);
    }

    double ellapsedInSeconds(void)
    {
        LARGE_INTEGER val;
        QueryPerformanceCounter(&val);
        return (val.QuadPart - m_baseTime_.QuadPart) * m_freq_;
    }

    double ellapsedInMilliseconds(void)
    {
        return ellapsedInSeconds() * 1000.0;
    }

private:
    double m_freq_;
    LARGE_INTEGER m_baseTime_;
    double m_minEllapsedTime;
    std::string m_fnName;
};
