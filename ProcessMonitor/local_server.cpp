#include "stdafx.h"
#include "local_server.h"

local_server::local_server() : m_is_quitting(false), m_is_listening(false)
{
}

local_server::~local_server()
{
}

std::string local_server::server_name() const
{
    return m_server_name;
}

void local_server::start()
{
    m_thread = std::thread(&local_server::run, this);
    wait_for_listener();
}

void local_server::stop()
{
    on_stopping();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void local_server::run(void)
{
    if (initialize())
    {
        listen();
    }
}

void local_server::on_listener_ready(void)
{
    std::unique_lock<std::mutex> lock(m_is_listening_lock);
    m_is_listening = true;
    lock.unlock();
    m_ready_for_listening.notify_all();
}

bool local_server::is_quitting() const
{
    return m_is_quitting;
}

void local_server::wait_for_listener(void)
{
    std::unique_lock<std::mutex> lock(m_is_listening_lock);
    if (!m_is_listening)
    {
        m_ready_for_listening.wait(lock);
    }
}

