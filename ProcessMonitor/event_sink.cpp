#include "stdafx.h"
#include "event_sink.h"
#include "process_watcher.h"

#pragma comment(lib, "Wbemuuid.lib")

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

application_info to_application(IWbemClassObjectPtr process_object);


ULONG EventSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
    LONG lRef = InterlockedDecrement(&m_lRef);
    if (lRef == 0)
        delete this;
    return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = reinterpret_cast<IWbemObjectSink *>(this);
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else return E_NOINTERFACE;
}

HRESULT EventSink::Indicate(long, IWbemClassObject **)
{
    return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
    /* [in] */ LONG lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR,
    /* [in] */ IWbemClassObject __RPC_FAR *
)
{
    return WBEM_S_NO_ERROR;
}

ProcCreationEventSink::ProcCreationEventSink(process_watcher* process_watcher)
{
    m_process_watcher = process_watcher;
}

ProcCreationEventSink::~ProcCreationEventSink(void)
{
}

HRESULT STDMETHODCALLTYPE ProcCreationEventSink::Indicate(long lObjectCount, IWbemClassObject **apObjArray)
{
    for (int i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject *pObj = apObjArray[i];

        HRESULT hres;
        VARIANT vInst_;
        hres = pObj->Get(L"TargetInstance", 0, &vInst_, NULL, NULL);
        if (FAILED(hres))
        {
            continue;
        }

        try
        {
            _variant_t vInst;
            vInst.Attach(vInst_);
            IWbemClassObjectPtr pWin32ProcessObj(vInst);
            auto app_info = to_application(pWin32ProcessObj);
            if (app_info.process_id != 0)
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                if (m_process_watcher)
                {
                    m_process_watcher->on_applications_event(app_info, APPLICATION_START);
                }
            }
        }
        catch (_com_error &)
        {
            TRACE("Failed to indicate creation for a process with 0x%08x\n", hres);
        }
    }
    return WBEM_S_NO_ERROR;
}

ProcDeletionEventSink::ProcDeletionEventSink(process_watcher* process_watcher)
{
    m_process_watcher = process_watcher;
}

ProcDeletionEventSink::~ProcDeletionEventSink()
{
}

HRESULT STDMETHODCALLTYPE ProcDeletionEventSink::Indicate(long lObjectCount, IWbemClassObject **apObjArray)
{
    for (int i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject *pObj = apObjArray[i];

        HRESULT hres;
        VARIANT vInst_;
        hres = pObj->Get(L"TargetInstance", 0, &vInst_, NULL, NULL);
        if (FAILED(hres))
        {
            continue;
        }

        try
        {
            _variant_t vInst;
            vInst.Attach(vInst_);
            IWbemClassObjectPtr pWin32ProcessObj(vInst);
            auto app_info = to_application(pWin32ProcessObj);
            if (app_info.process_id != 0)
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                if (m_process_watcher)
                {
                    m_process_watcher->on_applications_event(app_info, APPLICATION_STOP);
                }
            }
        }
        catch (_com_error &)
        {
            TRACE("Failed to indicate deletion for a process with 0x%08x\n", hres);
        }
    }
    return WBEM_S_NO_ERROR;
}

application_info to_application(IWbemClassObjectPtr process_object)
{
    /* Note:
    * A WbemClassObject does not directly expose all of its properties. It offers a GetNames method that
    * writes all of the properties of an object. A typical executable will probably have these properties:
    *      Caption
    *      CommandLine
    *      CreationClassName
    *      CreationDate
    *      CSCreationClassName
    *      CSName
    *      Description
    *      ExecutablePath
    *      ExecutionState
    *      Handle
    *      HandleCount
    *      InstallDate
    *      KernelModeTime
    *      MaximumWorkingSetSize
    *      MinimumWorkingSetSize
    *      Name
    *      OSCreationClassName
    *      OSName
    *      OtherOperationCount
    *      OtherTransferCount
    *      PageFaults
    *      PageFileUsage
    *      ParentProcessId
    *      PeakPageFileUsage
    *      PeakVirtualSize
    *      PeakWorkingSetSize
    *      Priority
    *      PrivatePageCount
    *      ProcessId
    *      QuotaNonPagedPoolUsage
    *      QuotaPagedPoolUsage
    *      QuotaPeakNonPagedPoolUsage
    *      QuotaPeakPagedPoolUsage
    *      ReadOperationCount
    *      ReadTransferCount
    *      SessionId
    *      Status
    *      TerminationDate
    *      ThreadCount
    *      UserModeTime
    *      VirtualSize
    *      WindowsVersion
    *      WorkingSetSize
    *      WriteOperationCount
    *      WriteTransferCount
    */

    application_info info;

    CComVariant process_object_variant;

    if (!FAILED(process_object->Get(L"ApplicationPath", 0, &process_object_variant, NULL, NULL)) && VT_BSTR == process_object_variant.vt && NULL != process_object_variant.bstrVal)
    {
        info.path = process_object_variant.bstrVal;
    }
    else if (!FAILED(process_object->Get(L"CommandLine", 0, &process_object_variant, NULL, NULL)) && VT_BSTR == process_object_variant.vt && NULL != process_object_variant.bstrVal)
    {
        info.command_line = process_object_variant.bstrVal;
    }
    if (!FAILED(process_object->Get(L"Name", 0, &process_object_variant, NULL, NULL)) && VT_BSTR == process_object_variant.vt && NULL != process_object_variant.bstrVal)
    {
        info.name = process_object_variant.bstrVal;
    }
    if (!FAILED(process_object->Get(L"ProcessId", 0, &process_object_variant, NULL, NULL)) && VT_NULL != process_object_variant.vt)
    {
        info.process_id = process_object_variant.intVal;
    }
    if (!FAILED(process_object->Get(L"SessionId", 0, &process_object_variant, NULL, NULL)) && VT_NULL != process_object_variant.vt)
    {
        info.session_id = process_object_variant.intVal;
    }
    if (!FAILED(process_object->Get(L"InstallDate", 0, &process_object_variant, NULL, NULL)) && VT_BSTR == process_object_variant.vt && NULL != process_object_variant.bstrVal)
    {
        info.install_date = process_object_variant.bstrVal;
    }

    return info;
}
