#include "stdafx.h"
#include "local_server_connection_win.h"

local_server_connection_win::local_server_connection_win(void* socket, int readBufferSize)
    : local_server_connection(socket, readBufferSize)
{
    m_hPipe = (HANDLE)socket;

    memset(&m_thisOverlap, 0, sizeof(m_thisOverlap));
    m_thisOverlap.pThis = this;

    initialize();
}

local_server_connection_win::~local_server_connection_win(void)
{
}

bool local_server_connection_win::initialize(void)
{
    // Schedule a pending read (synchronously for now)
    if (!ReadFileEx(m_hPipe, m_read_buffer.data(), (DWORD)m_read_buffer.size(), &m_thisOverlap.ov, _on_read_completed))
    {
        return false;
    }
    return true;
}

void WINAPI local_server_connection_win::_on_read_completed(DWORD dwErr, DWORD cbRead,
    LPOVERLAPPED lpOverLap)
{
    if (dwErr != ERROR_SUCCESS)
        return; // something is wrong,

    PTHISOVERLAP pThisOV = (PTHISOVERLAP)lpOverLap;
    pThisOV->pThis->on_read_completed(dwErr, cbRead);
}

void local_server_connection_win::on_read_completed(DWORD dwErr, DWORD cbRead)
{
    if (ERROR_SUCCESS != dwErr)
    {
        close();
        return;
    }

    // Allow derived classes to handle the data
    on_read_bytes(m_read_buffer.data(), cbRead);

    // Start the next read
    if ((ERROR_SUCCESS == dwErr) && (INVALID_HANDLE_VALUE != m_hPipe))
    {
        if (!ReadFileEx(m_hPipe, m_read_buffer.data(), (DWORD)m_read_buffer.size(), &m_thisOverlap.ov, _on_read_completed))
        {
            close();
        }
    }
}

bool local_server_connection_win::write(const char*data, int size)
{
    // Just write synchronously into the pipe for now
    //#pragma TODO("Should writing into a window pipe be synchronous")
    DWORD dwBytesToWrite = 0;
    if (!WriteFile(m_hPipe, data, size, &dwBytesToWrite, NULL))
    {
        //Because the writes are performed on another thread then the reads,
        // Let's not close the connection here. The overlapped read will take
        // of the close for us
        return false;
    }
    return true;
}

void local_server_connection_win::close(void)
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        HANDLE hPipe = m_hPipe;
        m_hPipe = INVALID_HANDLE_VALUE;

        CancelIo(hPipe);

        DWORD dwBytesTransferred = 0;
        while (!GetOverlappedResult(hPipe, &m_thisOverlap.ov, &dwBytesTransferred, FALSE)
            && (ERROR_IO_INCOMPLETE == GetLastError()))
        {
            TRACE("Socket %d has outstanding IO!\n", hPipe);
        }

        // Now we can close
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }

        // Let derived classes deal with this
        on_disconnected();
    }
}

void* local_server_connection_win::socket()
{
    return (void*)m_hPipe;
}
