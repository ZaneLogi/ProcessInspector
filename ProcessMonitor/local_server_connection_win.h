#pragma once
#include "local_server_connection.h"

class local_server_connection_win : public local_server_connection
{
public:
    local_server_connection_win(void* socket, int readBufferSize = 4096);
    virtual ~local_server_connection_win(void);

    virtual bool write(const char*data, int size) override; // synchronous (for now)
    virtual void* socket() override;
    virtual void close(void) override;

private:
    HANDLE m_hPipe;

    typedef struct THISOVERLAP {
        OVERLAPPED ov;
        local_server_connection_win *pThis;
    } THISOVERLAP, *PTHISOVERLAP;
    THISOVERLAP m_thisOverlap;

    bool initialize(void);
    void on_read_completed(DWORD dwErr, DWORD cbRead);

    static void WINAPI _on_read_completed(DWORD dwErr, DWORD cbRead, LPOVERLAPPED lpOverLap);
};
