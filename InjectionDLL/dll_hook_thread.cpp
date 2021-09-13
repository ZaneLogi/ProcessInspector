#include "stdafx.h"
#include "dll_hook_thread.h"
#include "dxgi_hook.h"

extern DWORD WINAPI HookD3D(LPVOID lpThreadParameter);


dll_hook_thread* dll_hook_thread::instance()
{
    static dll_hook_thread this_instance;
    return &this_instance;
}

dll_hook_thread::dll_hook_thread()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

dll_hook_thread::~dll_hook_thread()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    CloseHandle(m_stop_event);
}

void dll_hook_thread::start()
{
    // std::thread cause deadlock in DLLMain
    // https://stackoverflow.com/questions/32252143/stdthread-cause-deadlock-in-dllmain
    const std::lock_guard<std::mutex> lock(m_mutex);
    LOG << "START dll hook thread.\n";
    m_thread_handle = CreateThread(nullptr, 0, _thread_routine, this, 0, &m_thread_id);
}

void dll_hook_thread::stop()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    LOG << "STOP dll hook thread.\n";
    // signal the thread to stop, and then wait
    SetEvent(m_stop_event);
    if (m_thread_handle != nullptr)
    {
        LOG << "waiting the hook thread.\n";
        WaitForSingleObject(m_thread_handle, INFINITE);
        CloseHandle(m_thread_handle);
        m_thread_handle = nullptr;
    }
}

DWORD dll_hook_thread::_thread_routine(PVOID pv)
{
    dll_hook_thread* pThis = (dll_hook_thread*)pv;
    pThis->_event_loop();
    return 0;
}

VOID dll_hook_thread::_timer_routine(PVOID pv, BOOLEAN TimerOrWaitFired)
{
    dll_hook_thread* pThis = (dll_hook_thread*)pv;
    pThis->_on_timer(TimerOrWaitFired);
}

void dll_hook_thread::_on_timer(BOOLEAN)
{
    ::PostThreadMessage(m_thread_id, WM_TIMER, 0, 0);
}

void dll_hook_thread::_event_loop(void)
{
    LOG << ">>> Enter _event_loop\n";

    MH_Initialize();
    if (!dxgi_hook::instance()->init())
    {
        HookD3D(0);
    }

    const DWORD dueTime = 1000;
    const DWORD period = 5000;
    m_timer_queue = CreateTimerQueue();
    CreateTimerQueueTimer(&m_timer, m_timer_queue, _timer_routine, this, dueTime, period, 0);

    HANDLE events[] = { m_stop_event };

    bool active = true;
    while (active)
    {
        switch (MsgWaitForMultipleObjects(_countof(events), events, FALSE, INFINITE, QS_ALLINPUT))
        {
        case WAIT_OBJECT_0:
            // stop event
            active = false;
            LOG << "stop the thread loop 1.\n";
            break;

        case WAIT_OBJECT_0 + _countof(events):
            // other event
            MSG message;
            while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    PostQuitMessage(static_cast<int>(message.wParam));
                    active = false;
                    LOG << "stop the thread loop 2.\n";
                    break;
                }
                else if (message.message == WM_TIMER)
                {
                    LOG << "timer event!\n";
                }
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
            break;

        default:
            break;
        }
    }

    DeleteTimerQueueTimer(m_timer_queue, m_timer, INVALID_HANDLE_VALUE);
    DeleteTimerQueue(m_timer_queue);

    dxgi_hook::instance()->deinit();
    MH_Uninitialize();

    LOG << "<<< Exit _event_loop\n";
}
