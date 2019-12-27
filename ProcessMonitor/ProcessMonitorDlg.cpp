
// ProcessMonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ProcessMonitor.h"
#include "ProcessMonitorDlg.h"
#include "afxdialogex.h"
#include <algorithm>
#include <psapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define APPLICATION_EVENT_MSG (WM_USER+1)

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process = nullptr;

BOOL IsWow64(HANDLE hProcess)
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    if (!fnIsWow64Process)
    {
        fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
            GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
    }

    if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(hProcess, &bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CProcessMonitorDlg dialog



CProcessMonitorDlg::CProcessMonitorDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_PROCESSMONITOR_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcessMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PROCESS, m_lcProcess);
    DDX_Control(pDX, IDC_EC_LOG, m_ecLog);
}

BEGIN_MESSAGE_MAP(CProcessMonitorDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_MOVE()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(APPLICATION_EVENT_MSG, &CProcessMonitorDlg::OnApplicationEvent)
END_MESSAGE_MAP()


// CProcessMonitorDlg message handlers

BOOL CProcessMonitorDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);            // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    BEGIN_OBJ_MAP(CProcessMonitorDlg);
    OBJ_DEFINE_SCALEABLE(IDC_LIST_PROCESS);
    OBJ_DEFINE_BOTTOM(IDC_EC_LOG);
    OBJ_DEFINE_BOTTOM_RIGHT(IDOK);
    OBJ_DEFINE_BOTTOM_RIGHT(IDCANCEL);
    END_OBJ_MAP();

    m_ecLog.SetFont(CFont::FromHandle((HFONT)::GetStockObject(ANSI_FIXED_FONT)));

    InitProcessListControl();
    process_watcher::instance()->set_window_handle(GetSafeHwnd(), APPLICATION_EVENT_MSG);

    CTime t = CTime::GetCurrentTime();
    CString s = t.Format(_T("%F %T"));
    Log(_T("%s\r\n"), s);

    enable_privilege(TRUE);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

#define COL_PID 0
#define COL_BIT 1
#define COL_STS 2
#define COL_NAM 3

void CProcessMonitorDlg::InitProcessListControl()
{
    m_lcProcess.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lcProcess.InsertColumn(COL_PID, _T("Process ID"));
    m_lcProcess.InsertColumn(COL_BIT, _T("Bitness"));
    m_lcProcess.InsertColumn(COL_STS, _T("Status"));
    m_lcProcess.InsertColumn(COL_NAM, _T("Name"));

    int numCols = m_lcProcess.GetHeaderCtrl()->GetItemCount();
    for (int i = 0; i < numCols; i++)
    {
        m_lcProcess.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void CProcessMonitorDlg::Log(LPCTSTR lpszFormat, ...)
{
    static std::vector<TCHAR> Buffer;
    static int nBufSize = 16;

    const int MAX_SIZE = 65536;

    va_list args;
    va_start(args, lpszFormat);

    int nSize = -1;
    while (1)
    {
        Buffer.resize(nBufSize);
        nSize = _vsntprintf_s(&Buffer[0], nBufSize - 1, nBufSize - 2, lpszFormat, args);
        if (nSize == -1 && nBufSize < MAX_SIZE)
            nBufSize *= 2;
        else
            break;
    }

    va_end(args);

    if (nSize >= 0)
    {
        Buffer[nSize] = _T('\0');
        int len = m_ecLog.GetWindowTextLength();
        m_ecLog.SetSel(len, len);
        m_ecLog.ReplaceSel(&Buffer[0]);
    }
    else
    {
        OutputDebugString(_T("###Cannot create a debugging string###"));
    }
}

void CProcessMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProcessMonitorDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProcessMonitorDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CProcessMonitorDlg::OnMove(int x, int y)
{
    SAVE_WINDOW_PLACEMENT();
}

void CProcessMonitorDlg::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy); // need to change to CDialogEx if the base is CDialogEx
    UPDATE_OBJ_POSITION(cx, cy);
    SAVE_WINDOW_PLACEMENT();
}

void CProcessMonitorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    UPDATE_MINMAX_INFO(lpMMI);
    __super::OnGetMinMaxInfo(lpMMI); // need to change to CDialogEx if the base is CDialogEx
}

LRESULT CProcessMonitorDlg::OnApplicationEvent(WPARAM wParam, LPARAM lParam)
{
    int process_id = (int)wParam;
    int event_type = (int)lParam;

    if (process_id != 0)
    {
        auto itr = std::find_if(m_process_id_list.begin(), m_process_id_list.end(),
            [process_id](const auto& a)->bool { return a.process_id == process_id; });

        if (itr == m_process_id_list.end())
        {
            if (event_type == APPLICATION_START || event_type == APPLICATION_FOCUS)
            {
                if (event_type == APPLICATION_FOCUS)
                {
                    if (m_focus_index >= 0)
                        m_lcProcess.SetItemText(m_focus_index, COL_STS, _T(""));
                }

                application_info info;
                info.process_id = process_id;
                info.path = get_executable_path_by_process_id(process_id);
                if (info.path.empty())
                {
                    Log(_T("path err = %s\r\n"), get_error_string(L"get_executable_path_by_process_id").c_str());
                }

                CString id_str, path_str;
                id_str.Format(_T("%d"), process_id);
                path_str.Format(_T("%s"), info.path.c_str());
                m_lcProcess.InsertItem((int)m_process_id_list.size(), id_str);
                m_lcProcess.SetItemText((int)m_process_id_list.size(), COL_NAM, path_str);

                BOOL hasD3D11 = FALSE;
                BOOL iswow64 = FALSE;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                    PROCESS_VM_READ,
                    FALSE, process_id);
                if (NULL != hProcess)
                {
                    HMODULE hMods[1024];
                    DWORD cbNeeded;

                    // Get a list of all the modules in this process.
                    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
                    {
                        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                        {
                            TCHAR moduleName[MAX_PATH];

                            // Get the full path to the module's file.

                            if (GetModuleFileNameEx(hProcess, hMods[i], moduleName,
                                sizeof(moduleName) / sizeof(TCHAR)))
                            {
                                if (0 == _tcsicmp(_tcsrchr(moduleName, _T('\\')), _T("\\d3d11.dll")))
                                    hasD3D11 = TRUE;
                            }
                        }
                    }

                    iswow64 = IsWow64(hProcess);
                    m_lcProcess.SetItemText((int)m_process_id_list.size(), COL_BIT, iswow64 ? _T("32") : _T("64"));
                    CloseHandle(hProcess);
                }

                m_process_id_list.push_back(info);

                if (event_type == APPLICATION_FOCUS)
                {
                    m_focus_index = (int)m_process_id_list.size() - 1;
                    m_lcProcess.SetItemText(m_focus_index, COL_STS, _T("Focus"));
                }


#if 1
                if (!info.path.empty() && hasD3D11)
                {
                    Log(_T("Process %d, Name %s has d3d11.dll\r\n"), process_id, info.path.c_str());
                    const auto targets =
                    {
                        L"D3D11Application.exe",
                        L"D3DApp.exe"
                    };
                    auto found_itr = std::find_if(targets.begin(), targets.end(),
                        [&info](const auto& i) { return wcsstr(info.path.c_str(), i) != nullptr; });

                    if (found_itr != targets.end())
                    {
                        TCHAR command_line[MAX_PATH];
                        if (iswow64)
                        {
                            _stprintf_s(command_line, _T("hookhelper32.exe /PID=%d /LOG"), process_id);
                        }
                        else
                        {
                            _stprintf_s(command_line, _T("hookhelper64.exe /PID=%d /LOG"), process_id);
                        }

                        TRACE(_T("CreatProcess(%s)\n"), command_line);

                        STARTUPINFO si;
                        PROCESS_INFORMATION pi;

                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));

                        BOOL b = CreateProcess(nullptr, command_line, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

                        //WaitForSingleObject(pi.hProcess, INFINITE);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);

                        TRACE(_T("done %d\n"), b);
                    }
                }

#endif


            }
        }
        else
        {
            int index = (int)(itr - m_process_id_list.begin());
            if (event_type == APPLICATION_STOP)
            {
                m_lcProcess.DeleteItem(index);
                m_process_id_list.erase(itr);
            }
            else if (event_type == APPLICATION_FOCUS)
            {
                if (m_focus_index >= 0)
                    m_lcProcess.SetItemText(m_focus_index, COL_STS, _T(""));
                m_focus_index = index;
                m_lcProcess.SetItemText(m_focus_index, COL_STS, _T("Focus"));
            }
        }

        Log(_T("PID %d, %s\r\n"), process_id,
            event_type == APPLICATION_START ? _T("start") :
            event_type == APPLICATION_STOP ? _T("stop") :
            event_type == APPLICATION_FOCUS ? _T("focus") : _T("unknown"));

    }

    return 0;
}