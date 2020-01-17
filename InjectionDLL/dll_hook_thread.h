#pragma once

#include <mutex>
#include <thread>

class dll_hook_thread
{
public:
    static dll_hook_thread* instance();
    ~dll_hook_thread();

    void start();
    void stop();

private:
    dll_hook_thread();
    dll_hook_thread(const dll_hook_thread&) = delete;

    static DWORD WINAPI _thread_routine(PVOID);
    void _event_loop(void);

    static VOID WINAPI _timer_routine(PVOID, BOOLEAN);
    void _on_timer(BOOLEAN);

private:
    std::mutex m_mutex;
    HANDLE m_stop_event;
    HANDLE m_thread_handle = nullptr;
    DWORD m_thread_id = 0;
    HANDLE m_timer_queue = nullptr;
    HANDLE m_timer = nullptr;
};
