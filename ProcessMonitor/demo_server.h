#pragma once

#include <list>
#include "local_server_win.h"
#include "demo_connection.h"

#define WM_PIPE_SERVER (WM_USER+1001)

class demo_server : public local_server_win
{
public:
    demo_server();
    virtual ~demo_server();

    std::list<demo_connection_ptr> connections(void) { return m_connections; }

    void set_main_window(HWND);
    void notify_main_window(WPARAM, LPARAM);

protected:
    virtual void on_new_connection(void* socket) override;
    virtual void on_stopping(void) override;


private:
    std::mutex m_mutex;
    std::list<demo_connection_ptr> m_connections;
    HWND m_mainWindow = nullptr;
};
