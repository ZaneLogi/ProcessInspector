#pragma once

#include <mutex>
#include <thread>
#include "event_sink.h"

struct application_info
{
    std::wstring    path;
    std::wstring    command_line;
    std::wstring    name;
    int             process_id = 0;
    int             session_id = 0;
    std::wstring    install_date;
};

enum application_event
{
    APPLICATION_START,
    APPLICATION_STOP,
    APPLICATION_FOCUS
};

class process_watcher
{
public:
    static process_watcher* instance();
    ~process_watcher();

    void set_window_handle(HWND hwnd, UINT msg_id);
    void on_applications_event(const application_info& application, application_event type);

private:
    process_watcher();
    process_watcher(const process_watcher&) = delete;

    void _event_hook_thread(void);
    static void CALLBACK _WinEventProc(HWINEVENTHOOK event_hook, DWORD event_id, HWND window_handle, LONG object_id, LONG child_id, DWORD event_thread, DWORD event_time_ms);


private:
    HWND m_hwnd = nullptr;
    UINT m_msg_id = 0;

    std::mutex m_mutex;
    HANDLE m_stop_event;
    std::thread m_event_thread;
    bool m_is_initialized = false;

    IWbemServices *m_pSvc = nullptr;
    IWbemLocator *m_pLoc = nullptr;
    IUnsecuredApartment *m_pUnsecApp = nullptr;

    IUnknown *m_pStubCreationUnk = nullptr;
    ProcCreationEventSink *m_pCreationSink = nullptr;
    IWbemObjectSink *m_pStubCreationSink;

    IUnknown *m_pStubDeletionUnk = nullptr;
    ProcDeletionEventSink *m_pDeletionSink = nullptr;
    IWbemObjectSink *m_pStubDeletionSink = nullptr;

};
