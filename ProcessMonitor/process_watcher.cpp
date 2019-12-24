#include "stdafx.h"
#include "process_watcher.h"

#define SAFE_RELEASE(x) if (nullptr != x) { x->Release(); x = nullptr;}

process_watcher* process_watcher::instance()
{
    static process_watcher this_instance;
    return &this_instance;
}

process_watcher::process_watcher()
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hres))
    {
        TRACE("CoInitializeEx already set to the proper mode\n");
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                           // COM negotiates service
        NULL,                         // Authentication services
        NULL,                         // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,    // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE,  // Default Impersonation
        NULL,                         // Authentication info
        EOAC_NONE,                    // Additional capabilities
        NULL                          // Reserved
    );

    // This will FAIL if the user runs it under a Guest Account
    // OR it has already been called
    if (FAILED(hres) && (RPC_E_TOO_LATE != hres))
    {
        return;                      // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    m_pLoc = nullptr;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<LPVOID *>(&m_pLoc));

    if (FAILED(hres))
    {
        return;                 // Program has failed.
    }

    // Step 4: ---------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    m_pSvc = nullptr;

    // Connect to the local root\cimv2 namespace
    // and obtain pointer m_pSvc to make IWbemServices calls.
    hres = m_pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &m_pSvc
    );

    if (FAILED(hres))
    {
        SAFE_RELEASE(m_pLoc);
        return;                // Program has failed.
    }

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        m_pSvc,                       // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,            // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,             // RPC_C_AUTHZ_xxx
        NULL,                         // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,       // RPC_C_AUTHN_LEVEL_xxx
        RPC_C_IMP_LEVEL_IMPERSONATE,  // RPC_C_IMP_LEVEL_xxx
        NULL,                         // client identity
        EOAC_NONE                     // proxy capabilities
    );

    if (FAILED(hres))
    {
        SAFE_RELEASE(m_pSvc);
        SAFE_RELEASE(m_pLoc);
        return;               // Program has failed.
    }

    // Step 6: -------------------------------------------------
    // Receive event notifications -----------------------------

    // Use an unsecured apartment for security
    m_pUnsecApp = NULL;

    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
        CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
        (void**)&m_pUnsecApp);

    if (FAILED(hres))
    {
        TRACE("Failed to create unsecured apartment with 0x%08x\n", hres);
        SAFE_RELEASE(m_pSvc);
        SAFE_RELEASE(m_pLoc);
        return;
    }

    m_pCreationSink = new ProcCreationEventSink(this);
    m_pCreationSink->AddRef();

    m_pStubCreationUnk = NULL;
    m_pUnsecApp->CreateObjectStub(m_pCreationSink, &m_pStubCreationUnk);

    m_pStubCreationSink = NULL;
    m_pStubCreationUnk->QueryInterface(IID_IWbemObjectSink,
        (void **)&m_pStubCreationSink);

    // The ExecNotificationQueryAsync method will call
    // The EventQuery::Indicate method when an event occurs
    hres = m_pSvc->ExecNotificationQueryAsync(
        _bstr_t("WQL"),
        _bstr_t("SELECT * "
            "FROM __InstanceCreationEvent WITHIN 10 "
            "WHERE TargetInstance ISA 'Win32_Process'"),
        WBEM_FLAG_SEND_STATUS,
        NULL,
        m_pStubCreationSink);

    // Check for errors.
    if (FAILED(hres))
    {
        SAFE_RELEASE(m_pSvc);
        SAFE_RELEASE(m_pLoc);
        SAFE_RELEASE(m_pUnsecApp);
        SAFE_RELEASE(m_pStubCreationUnk);
        SAFE_RELEASE(m_pCreationSink);
        SAFE_RELEASE(m_pStubCreationSink);
        SAFE_RELEASE(m_pStubDeletionUnk);
        SAFE_RELEASE(m_pDeletionSink);
        SAFE_RELEASE(m_pStubDeletionSink);
        return;
    }

    m_pDeletionSink = new ProcDeletionEventSink(this);
    m_pDeletionSink->AddRef();

    m_pStubDeletionUnk = NULL;
    m_pUnsecApp->CreateObjectStub(m_pDeletionSink, &m_pStubDeletionUnk);

    m_pStubDeletionSink = NULL;
    m_pStubDeletionUnk->QueryInterface(IID_IWbemObjectSink,
        (void **)&m_pStubDeletionSink);

    // The ExecNotificationQueryAsync method will call
    // The EventQuery::Indicate method when an event occursw
    hres = m_pSvc->ExecNotificationQueryAsync(
        _bstr_t("WQL"),
        _bstr_t("SELECT * "
            "FROM __InstanceDeletionEvent WITHIN 10 "
            "WHERE TargetInstance ISA 'Win32_Process'"),
        WBEM_FLAG_SEND_STATUS,
        NULL,
        m_pStubDeletionSink);

    // Check for errors.
    if (FAILED(hres))
    {
        SAFE_RELEASE(m_pSvc);
        SAFE_RELEASE(m_pLoc);
        SAFE_RELEASE(m_pUnsecApp);
        SAFE_RELEASE(m_pStubCreationUnk);
        SAFE_RELEASE(m_pCreationSink);
        SAFE_RELEASE(m_pStubCreationSink);
        SAFE_RELEASE(m_pStubDeletionUnk);
        SAFE_RELEASE(m_pDeletionSink);
        SAFE_RELEASE(m_pStubDeletionSink);
        return;
    }

    // start our thread
    m_event_thread = std::thread(&process_watcher::_event_hook_thread, this);
    m_is_initialized = true;
}

process_watcher::~process_watcher()
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    // signal the thread to stop, and then wait
    SetEvent(m_stop_event);
    if (m_event_thread.joinable())
    {
        m_event_thread.join();
    }
    CloseHandle(m_stop_event);

    if (m_pSvc)
    {
        m_pSvc->CancelAsyncCall(m_pStubCreationSink);
    }

    // Cleanup
    // ========
    if (m_pCreationSink)
    {
        m_pCreationSink->ClearProcessWatcher();
    }
    if (m_pDeletionSink)
    {
        m_pDeletionSink->ClearProcessWatcher();
    }
    SAFE_RELEASE(m_pSvc);
    SAFE_RELEASE(m_pLoc);
    SAFE_RELEASE(m_pUnsecApp);
    SAFE_RELEASE(m_pStubCreationUnk);
    SAFE_RELEASE(m_pCreationSink);
    SAFE_RELEASE(m_pStubCreationSink);
    SAFE_RELEASE(m_pStubDeletionUnk);
    SAFE_RELEASE(m_pDeletionSink);
    SAFE_RELEASE(m_pStubDeletionSink);

    CoUninitialize();
    m_is_initialized = false;
}

void process_watcher::set_window_handle(HWND hwnd, UINT msg_id)
{
    m_hwnd = hwnd;
    m_msg_id = msg_id;
}

void process_watcher::on_applications_event(const application_info& application, application_event type)
{
    if (m_hwnd && m_msg_id)
    {
        std::wstring name = !application.path.empty() ? application.path : !application.command_line.empty() ? application.command_line : application.name;
        TRACE("PROCESS %ls (id %d) %s\n",
            name.c_str(),
            application.process_id,
            type == APPLICATION_START ? "start" :
            type == APPLICATION_STOP ? "stop" :
            type == APPLICATION_FOCUS ? "focus" : "unknown");
        ::PostMessage(m_hwnd, m_msg_id, application.process_id, type);
    }
}

void process_watcher::_event_hook_thread(void)
{
    // The WinEventHook requires a message loop on the same thread in order to receive events, so we create our own after setting up the event hook.
    HWINEVENTHOOK hWinEventHook =
        SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_MINIMIZEEND, nullptr, _WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

    if (!hWinEventHook)
    {
        TRACE("Failed to SetWinEventHook.\n");
    }

    // force an event for the current foreground app since WinEvents are only sent on CHANGE
    _WinEventProc(hWinEventHook, EVENT_SYSTEM_FOREGROUND, GetForegroundWindow(), 0, 0, 0, 0);

    bool active = true;
    while (active)
    {
        switch (MsgWaitForMultipleObjects(1, &m_stop_event, FALSE, INFINITE, QS_ALLINPUT))
        {
        case WAIT_OBJECT_0:
            // stop event
            active = false;
            break;

        case WAIT_OBJECT_0 + 1:
            // other event
            MSG message;
            while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    PostQuitMessage(static_cast<int>(message.wParam));
                    active = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

        default:
            break;
        }
    }

    UnhookWinEvent(hWinEventHook);
}

void CALLBACK process_watcher::_WinEventProc(HWINEVENTHOOK event_hook, DWORD event_id, HWND window_handle, LONG object_id, LONG child_id, DWORD event_thread, DWORD event_time_ms)
{
    if (!process_watcher::instance()->m_is_initialized)
    {
        return;
    }

    if (EVENT_SYSTEM_FOREGROUND == event_id || EVENT_SYSTEM_MINIMIZEEND == event_id)
    {
        // Get the process id
        DWORD process_id_dword = 0;
        auto foreground_window_handle = GetForegroundWindow();
        GetWindowThreadProcessId(foreground_window_handle, &process_id_dword);
        if (0 == process_id_dword)
        {
            TRACE("Unable to get process id for window 0x%08x\n", foreground_window_handle);
            return;
        }

        application_info info;
        info.process_id = process_id_dword;
        info.path = get_executable_path_by_process_id(process_id_dword);
        process_watcher::instance()->on_applications_event(info, APPLICATION_FOCUS);
    }
}
