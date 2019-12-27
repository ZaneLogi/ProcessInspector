#include "stdafx.h"
#include <cassert>
#include <sddl.h>
#include "local_server_win.h"

local_server_win::local_server_win(bool messageBased) : local_server(), m_message_based(messageBased)
{
    m_hConnectEvent = CreateEvent(
        NULL,   // no security attribute
        FALSE,  // automatic reset event
        FALSE,  // initial state = not signaled
        NULL);  // unnamed event object
    m_hQuitEvent = CreateEvent(
        NULL,   // no security attribute
        FALSE,  // automatic reset event
        FALSE,  // initial state = not signaled
        NULL);  // unnamed event object
    ZeroMemory(&m_ov, sizeof(m_ov));
    m_ov.hEvent = m_hConnectEvent;
    m_hCurrentPipe = INVALID_HANDLE_VALUE;
}

local_server_win::~local_server_win(void)
{
}

bool local_server_win::initialize()
{
    char name[64];
    sprintf_s(name, "\\\\.\\pipe\\%s-%08x", m_pipeName.c_str(), WTSGetActiveConsoleSessionId());
    m_server_name = name;
    return true;
}

bool local_server_win::listen()
{
    bool isIOPending = false;
    HANDLE hCurrentPipe = NULL;

    // Create our first instance
    hCurrentPipe = create_and_connect_pipe_instance(isIOPending);
    if (NULL == hCurrentPipe)
    {
        return false;
    }

    // Inform the outside that our listener is ready
    on_listener_ready();

    HANDLE events[] = { m_hConnectEvent, m_hQuitEvent };
    DWORD dwEvents = _countof(events);

    bool listenResult = true;
    while (NULL != hCurrentPipe)
    {
        DWORD dwWait = WaitForMultipleObjectsEx(dwEvents, events, FALSE, INFINITE, TRUE);

        switch (dwWait)
        {
        case WAIT_OBJECT_0:
            // The wait conditions are satisfied by a completed connect
            // operation.
            // If an operation is pending, get the result of the
            // connect operation.
            if (isIOPending)
            {
                DWORD dwBytes = 0;
                if (GetOverlappedResult(hCurrentPipe, &m_ov, &dwBytes, false))
                {
                    on_new_connection((void*)hCurrentPipe);
                }
                else
                {
                    // The current pipe is useless. Let's dump it.
                    CloseHandle(hCurrentPipe);
                    hCurrentPipe = NULL;
                }
            }
            else
            {
                on_new_connection((void*)hCurrentPipe);
            }

            // Create a new pipe to wait on
            hCurrentPipe = create_and_connect_pipe_instance(isIOPending);
            if (NULL == hCurrentPipe)
            {
                listenResult = false;
            }
            break;

        case WAIT_IO_COMPLETION:
            // The wait is satisfied by a completed read or write
            // operation. This allows the system to execute the
            // completion routine.
            break;

        case WAIT_OBJECT_0 + 1:
            // Quit event
            CancelIo(hCurrentPipe);
            CloseHandle(hCurrentPipe);
            hCurrentPipe = NULL;
            listenResult = true;
            break;

        case WAIT_TIMEOUT:
            // The timeout happened. The state of the items waiting on didn't change.
            break;

        default:
            assert(false);
            return false;
        }
    }

    return listenResult;
}

HANDLE local_server_win::create_and_connect_pipe_instance(bool &isIOPending)
{
    HANDLE hPipe = NULL;
    const int pipeTimeoutInMilliseconds = 500;

    isIOPending = false;

    // Security attribute needed for UDM driver to connect to the pipe
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;
    if (ConvertStringSecurityDescriptorToSecurityDescriptor(L"D:(A;;GA;;;LS)(A;;GA;;;BA)(A;;GA;;;IU)",
        SDDL_REVISION_1,
        &(sa.lpSecurityDescriptor),
        NULL))
    {
        hPipe = CreateNamedPipeA(
            m_server_name.c_str(),                      // pipe name
            PIPE_ACCESS_DUPLEX |                        // read/write access
            FILE_FLAG_OVERLAPPED,                       // overlapped mode
            m_message_based ?
            PIPE_TYPE_MESSAGE |                         // message-type pipe
            PIPE_READMODE_MESSAGE |                     // message read mode
            PIPE_WAIT :                                 // blocking mode
            PIPE_TYPE_BYTE |                            // byte-type pipe
            PIPE_READMODE_BYTE |                        // byte read mode
            PIPE_WAIT,                                  // blocking mode
            PIPE_UNLIMITED_INSTANCES,                   // unlimited instances
            m_outputBufSize,                            // output buffer size
            m_inputBufSize,                             // input buffer size
            pipeTimeoutInMilliseconds,                  // client time-out
            &sa);                                       // no security attributes

        if (INVALID_HANDLE_VALUE == hPipe)
        {
            return NULL;
        }
    }

    // Now connect
    BOOL bConnected = ConnectNamedPipe(hPipe, &m_ov);
    // from MSDN:
    // If a client connects before the function is called, the function returns zero
    // and GetLastError returns ERROR_PIPE_CONNECTED. This can happen if a client
    // connects in the interval between the call to CreateNamedPipe and the call to
    // ConnectNamedPipe. In this situation, there is a good connection between client
    // and server, even though the function returns zero.
    if (!bConnected && (ERROR_PIPE_CONNECTED != GetLastError()) && (ERROR_IO_PENDING != GetLastError()))
    {
        CloseHandle(hPipe);
        return NULL;
    }

    switch (GetLastError())
    {
        // The overlapped connection in progress.
    case ERROR_IO_PENDING:
        isIOPending = true;
        break;

        // Client is already connected, so signal an event.
    case ERROR_PIPE_CONNECTED:
        if (SetEvent(m_ov.hEvent))
        {
            break;
        }
        // otherwise, fall through to error case since we couldn't set the event

        // If an error occurs during the connect operation...
    default:
        return NULL;
        break;
    }

    return hPipe;
}

void local_server_win::on_stopping(void)
{
    SetEvent(m_hQuitEvent);
}
