///***********************************************************************
///
/// LTrace.cpp
///
///***********************************************************************

#include "StdAfx.h"
#include <wtypes.h> // for LPOLESTR
#include <Objbase.h> // for StringFromCLSID
#include <atlbase.h> // for CW2T
#include "LTrace.h"
#include "cHighResolutionTimer.h"


#ifdef USE_LGD_EXCEPTION_ASSERT
#include <LGD_Exception.h>
#endif
namespace LTrace
{
    LPOUTPUTDEBUGSTRINGFCN pOutputDebugStringFcn = NULL;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;
}

static DWORD s_dwTraceFlags = 0;

cHighResolutionTimer g_hiresTimer;

// =======================================================================
//    Updates the trace flags
// =======================================================================
void SetTraceFlags(DWORD dwFlags)
{
    s_dwTraceFlags = dwFlags;
}


// =======================================================================
// Outputs a line of debug output, optionally handling it off to a user-
// supplied hook function as well.
// =======================================================================
static void LogiTraceOutput(LPCTSTR sString)
{
    if (LTrace::pOutputDebugStringFcn)
        LTrace::pOutputDebugStringFcn(sString);
    OutputDebugString(sString);
}


// =======================================================================
// Formats debugging output
// =======================================================================
void LogiTrace(LPCTSTR lpszFormat, ...)
{
// If file logging is enabled, but no hook exists, do nothing.
#if defined (LTRACE_LOG_FILE)
    if (NULL == LTrace::pOutputDebugStringFcn)
    {
        return;
    }
#endif

    TCHAR szBuffer[1024];

    va_list args;
    va_start(args, lpszFormat);

    // Add a timestamp in seconds with fixed float format
    // This is always 9+2 characters
    const int timestamp_width = 11;
    swprintf(szBuffer, _countof(szBuffer), L"%09.3f: ", g_hiresTimer.ellapsedInSeconds());

#if defined (_DEBUG) || defined(FORCE_LTRACE_DEBUG) || defined(LTRACE_LOG_FILE)
    int nBuf =
#endif
        _vsntprintf_s(
            szBuffer + timestamp_width,
            _countof(szBuffer) - timestamp_width,
            sizeof(szBuffer) / sizeof(TCHAR),
            lpszFormat,
            args);

    // was there an error? was the expanded string too long?
    ASSERT(nBuf >= 0);
    va_end(args);

    if(0 != s_dwTraceFlags)
    {
        TCHAR sProcThread[32];
        if((TRACE_INCLUDE_THREADID | TRACE_INCLUDE_PROCESSID) ==
               (s_dwTraceFlags & (TRACE_INCLUDE_THREADID | TRACE_INCLUDE_PROCESSID)))
        {
            _stprintf_s(
                sProcThread,
                _countof(sProcThread),
                _T("P%d.T%d:"),
                GetCurrentProcessId(),
                GetCurrentThreadId());
        }
        else if(TRACE_INCLUDE_PROCESSID == (s_dwTraceFlags & TRACE_INCLUDE_PROCESSID))
        {
            _stprintf_s(sProcThread, _countof(sProcThread), _T("P%d:"), GetCurrentProcessId());
        }
        else if(TRACE_INCLUDE_THREADID == (s_dwTraceFlags & TRACE_INCLUDE_THREADID))
        {
            _stprintf_s(sProcThread, _countof(sProcThread), _T("T%d:"), GetCurrentThreadId());
        }
        else
        {
            sProcThread[0] = 0;
        }

        LogiTraceOutput(sProcThread);
    }

    LogiTraceOutput(szBuffer);
}


// =======================================================================
// implementation of the assert.
// =======================================================================
bool LogiAssert(bool bExpression, LPCTSTR sExpression, LPCTSTR sFile, int nLine)
{
    if(!bExpression)
    {
        TRACE(_T("%s(%d): ASSERT FAILURE! Expression: %s\n"), sFile, nLine, sExpression);

#ifdef USE_LGD_EXCEPTION_ASSERT
        CLGD_Exception &rException = CLGD_Exception::GetInstance();
        rException.SetAssertData(sExpression, sFile, nLine);
#endif
        return true;
    }

    return false;
}

// =======================================================================
// converts a guid to a string
// =======================================================================
LPCTSTR Guid2T(REFGUID rGuid)
{
    static TCHAR sTempGuid[MAX_PATH];
    LPOLESTR lpOleGuid = NULL;
    VERIFY(SUCCEEDED(StringFromCLSID(rGuid, &lpOleGuid)));
    if(NULL == lpOleGuid)
    {
        TRACE(_T("Error converting Guid.\n"));
        return NULL;
    }

    _tcscpy_s(sTempGuid, _countof(sTempGuid), CW2T(lpOleGuid));

    CoTaskMemFree(lpOleGuid);
    lpOleGuid = NULL;

    return sTempGuid;
}

// =======================================================================
// converts a string to a guid
// =======================================================================
void T2Guid(LPCTSTR sGuid, GUID &rGuid)
{
    LPOLESTR lpOleGuid = (LPOLESTR) CoTaskMemAlloc((_tcslen(sGuid) + 1) * sizeof(OLECHAR));
    ASSERT(NULL != lpOleGuid);
    if(NULL != lpOleGuid)
    {
        wcscpy_s(lpOleGuid, (_tcslen(sGuid) + 1), CT2W(sGuid));
        CLSIDFromString(lpOleGuid, &rGuid);
        CoTaskMemFree(lpOleGuid);
    }
}


/// =======================================================================
// LogiTraceEnableFileLogging
// =======================================================================
static void LogiTraceLogFileCallback(LPCTSTR szDbgString)
{
    static TCHAR CRLF[] = _T("\r\n");

    if (LTrace::hLogFile)
    {
        // Do strings one \n at a time
        DWORD dwStrLen = (DWORD)_tcslen(szDbgString);

        // End of string
        LPCTSTR pEndOfString = szDbgString + dwStrLen;
        LPTSTR pCurLine = (LPTSTR)szDbgString;

        while (pCurLine < pEndOfString)
        {
            LPTSTR nextLF = _tcsrchr(pCurLine, '\n');
            DWORD dwStringLen = (NULL == nextLF) ? (DWORD)(pEndOfString - pCurLine) : (DWORD)(nextLF - pCurLine);
            DWORD dwBytesWritten = 0;

            if (dwStringLen && (*pCurLine != '\n'))
            {
                WriteFile(LTrace::hLogFile, pCurLine, dwStringLen*sizeof(TCHAR), &dwBytesWritten, NULL);
                pCurLine += dwStringLen;
            }

            if (NULL != nextLF)
            {
                WriteFile(LTrace::hLogFile, CRLF, 4, &dwBytesWritten, NULL);
                // Advance past the LF
                pCurLine++;
            }
        }
    }
}


/// =======================================================================
// LogiTraceEnableFileLogging
// =======================================================================
void LogiTraceEnableFileLogging(LPCTSTR szFileName, BOOL bEnable)
{
    if (NULL == szFileName)
    {
        ASSERT(FALSE);
        return;
    }

    if (INVALID_HANDLE_VALUE != LTrace::hLogFile)
    {
        CloseHandle(LTrace::hLogFile);
        LTrace::hLogFile = INVALID_HANDLE_VALUE;
    }

    if (bEnable)
    {
        // Open it up
        HANDLE h = CreateFile(szFileName, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == h)
        {
            return;
        }

        LTrace::hLogFile = h;
        LTrace::pOutputDebugStringFcn = LogiTraceLogFileCallback;

        // If the file does not exist, write UTF-16 byte mark
        if (0 == GetFileSize(h, NULL))
        {
            DWORD dwBytesWritten = 0;
            static BYTE UTF16MARK[] = { 0xFF, 0xFE };
            WriteFile(LTrace::hLogFile, UTF16MARK, 2, &dwBytesWritten, NULL);
        }
        else
        {
            // Seek to the end
            SetFilePointer(h, 0, 0, FILE_END);
        }
    }
}

///** end of LTrace.cpp **************************************************
