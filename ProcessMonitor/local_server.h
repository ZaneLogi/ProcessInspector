#pragma once
#include <string>
#include <thread>
#include <mutex>

class local_server
{
public:
    local_server();
    virtual ~local_server();

    virtual std::string server_name() const;
    virtual void start();
    virtual void stop();

protected:
    virtual void run(void);
    virtual void on_listener_ready();
    bool is_quitting() const;
    void wait_for_listener();

    virtual bool initialize() = 0;
    virtual bool listen() = 0;
    virtual void on_stopping() = 0;
    virtual void on_new_connection(void* socket) = 0;

protected:
    std::thread m_thread;
    bool m_is_quitting = false;
    std::string m_server_name;
    std::condition_variable m_ready_for_listening;
    std::mutex m_is_listening_lock;
    bool m_is_listening = false;
};
