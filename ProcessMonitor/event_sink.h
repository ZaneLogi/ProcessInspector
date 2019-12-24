#pragma once
#include <comdef.h>
#include <Wbemidl.h>
#include <mutex>

class process_watcher;

class EventSink : public IWbemObjectSink
{
    LONG m_lRef;
    bool bDone;

public:
    EventSink() { m_lRef = 0; }
    virtual ~EventSink() { bDone = true; }

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT
        STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    virtual HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
    );

    virtual HRESULT STDMETHODCALLTYPE SetStatus(
        /* [in] */ LONG lFlags,
        /* [in] */ HRESULT hResult,
        /* [in] */ BSTR strParam,
        /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
    );
};

class ProcCreationEventSink : public EventSink
{
public:
    ProcCreationEventSink(process_watcher* process_watcher);
    virtual ~ProcCreationEventSink();

    virtual HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
    );

    void ClearProcessWatcher(void)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_process_watcher = NULL;
    }

private:
    std::mutex m_mutex;
    process_watcher *m_process_watcher;
};


class ProcDeletionEventSink : public EventSink
{
public:
    ProcDeletionEventSink(process_watcher* process_watcher);
    ~ProcDeletionEventSink();

    virtual HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
    );

    void ClearProcessWatcher(void)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_process_watcher = NULL;
    }

private:
    std::mutex m_mutex;
    process_watcher *m_process_watcher;
};
