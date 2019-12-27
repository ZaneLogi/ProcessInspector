#include "stdafx.h"
#include "demo_server.h"

// NEED TO DO
// set the server name for the client connection
#define SERVER_NAME "PROCESS_MONITOR_SERVER"

demo_server::demo_server() : local_server_win(false)
{
    m_pipeName = SERVER_NAME;
}

demo_server::~demo_server(void)
{
}

void demo_server::set_main_window(HWND hwnd)
{
    m_mainWindow = hwnd;
}

void demo_server::notify_main_window(WPARAM wParam, LPARAM lParam)
{
    if (m_mainWindow)
        ::PostMessage(m_mainWindow, WM_PIPE_SERVER, wParam, lParam);
}

void demo_server::on_new_connection(void* socket)
{
    // add the connection to our list
    demo_connection_ptr connection = demo_connection_ptr(new demo_connection(socket, this));

    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.push_back(connection);
}

void demo_server::on_stopping(void)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_connections.empty())
    {
        demo_connection_ptr connection = m_connections.back();
        m_connections.pop_back();

        m_mutex.unlock();
        connection->close();
        m_mutex.lock();
    }
    local_server_win::on_stopping();
}
