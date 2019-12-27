#pragma once
#include "local_server.h"

class local_server_win : public local_server
{
public:
    local_server_win(bool messageBased = true);
    virtual ~local_server_win(void);

protected:
    virtual bool initialize(void) override;
    virtual bool listen(void) override;
    virtual void on_stopping(void) override;

private:
    // Windows
    HANDLE create_and_connect_pipe_instance(bool &isIOPending);
    HANDLE m_hConnectEvent;
    HANDLE m_hQuitEvent;
    OVERLAPPED m_ov;
    HANDLE m_hCurrentPipe;

protected:
    std::string m_pipeName;
    DWORD m_outputBufSize;
    DWORD m_inputBufSize;

private:
    bool m_message_based;
};
