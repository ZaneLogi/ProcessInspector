#ifndef LTRACE_H_INCLUDED
#define LTRACE_H_INCLUDED

#ifdef TRACE
#undef TRACE
#endif

#ifdef TRACE1
#undef TRACE1
#endif

#ifdef TRACE2
#undef TRACE2
#endif

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef VERIFY
#undef VERIFY
#endif

enum TRACEOPTIONS {
    TRACE_INCLUDE_THREADID              = 0x0001,
    TRACE_INCLUDE_PROCESSID             = 0x0002,
};


void LogiTrace(LPCTSTR lpszFormat, ...);
typedef void (*LPOUTPUTDEBUGSTRINGFCN)(LPCTSTR szDbgString);

namespace LTrace
{
    extern LPOUTPUTDEBUGSTRINGFCN pOutputDebugStringFcn;
}

void LogiTraceEnableFileLogging(LPCTSTR szPrefix, BOOL bEnable);

#if defined (_DEBUG) || defined(FORCE_LTRACE_DEBUG) || defined(LTRACE_LOG_FILE)

bool LogiAssert(bool bExpression, LPCTSTR sExpression, LPCTSTR sFile, int nLine);
void SetTraceFlags(DWORD dwFlags);

// Debug
#define TRACE                           ::LogiTrace
#define SETTRACEFLAGS(f)                ::SetTraceFlags(f)
#define ASSERT_CS_HELD(cs)              ASSERT((GetCurrentThreadId() == (DWORD_PTR)((cs).OwningThread)) && (0 < (cs).RecursionCount))

#define ASSERT(f)                       if(::LogiAssert((f), _T(#f), _T(__FILE__), __LINE__)) \
                                            DebugBreak();
#define ASSERTFALSE(m)                  if(::LogiAssert(false, m, _T(__FILE__), __LINE__)) \
                                            DebugBreak();
#define VERIFY(f)                       ASSERT(f)
#define UNHANDLED_SWITCH()              TRACE(_T("%s(%d): unhandled switch statement.\n"), _T(__FILE__), __LINE__); break
#define UNHANDLED_SWITCH_ASSERT()       ASSERTFALSE(_T("unhandled switch statement.\n")); break

#else

// Release
#define TRACE                           ::LogiTrace
#define SETTRACEFLAGS(f)                ((void)0)
#define ASSERT_CS_HELD(cs)              ((void)0)
#define ASSERT(f)                       ((void)0)
#define VERIFY(f)                       ((void)(f))
#define ASSERTFALSE(m)                  ((void)0)
#define UNHANDLED_SWITCH()              break
#define UNHANDLED_SWITCH_ASSERT()       break;

#endif

#define TRACE1                          TRACE
#define TRACE2                          TRACE


LPCTSTR Guid2T(REFGUID rGuid);
void T2Guid(LPCTSTR sGuid, GUID &rGuid);

#endif //!LTRACE_H_INCLUDED

//** end of LTrace.h *****************************************************
