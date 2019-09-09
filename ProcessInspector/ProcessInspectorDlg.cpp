
// ProcessInspectorDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psapi.h>
#include <string>
#include <set>
#include "ProcessInspector.h"
#include "ProcessInspectorDlg.h"
#include "afxdialogex.h"
#include "ProcessMemInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef std::basic_string<TCHAR> tstring;


struct PROCESS_INFO
{
    tstring m_name;
    DWORD   m_pid;

    PROCESS_INFO(PCTSTR name, DWORD pid) : m_name(name), m_pid(pid) {}
};

bool operator < (const PROCESS_INFO& lhs, const PROCESS_INFO& rhs)
{
    if (lhs.m_name != rhs.m_name)
        return _tcsicmp(lhs.m_name.c_str(), rhs.m_name.c_str()) < 0;
    else
        return lhs.m_pid < rhs.m_pid;
}

/*
https://www.unknowncheats.me/forum/programming-for-beginners/52562-simple-write-process-memory-function-explained.html

This is where the magic happens. WriteProcessMemory is the function we use if we want to overwrite memory values stored.
Sometimes memory values are protected from altering so we would unprotect it first. In that case we would do something like this:

unsigned long		Protection;
VirtualProtect		( (void*) Offset, dataSize, PAGE_EXECUTE_READWRITE, &Protection );
WriteProcessMemory	( hProcess, (void*) Offset, &data, dataSize, 0 );
VirtualProtect		( (void*) Offset, dataSize, Protection, &Protection );

Below the address we are writing to is 0x12345678, if say the first 3 values are 0's like in this example: 00012345,
then we can just write 0x12345

So we write in our offset that we are patching, then we write the data and data size we are writing to the offset.
*/

// sample code from
// "http://edisonx.pixnet.net/blog/post/89107497-%5B%E9%87%8E%E6%88%B0%E5%A4%96%E6%8E%9B%5D-read---write-process-memory"
BOOL RaisePrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
            AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, 0);
    if (hToken) {// success
        CloseHandle(hToken);
        return TRUE;
    }
    else {
        TRACE(_T("last error : %d\n"), GetLastError());
        return FALSE;
    }
}

// sample code from MSDN "Enabling and Disabling Privileges in C++"
BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup
        &luid))        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        printf("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
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


// CProcessInspectorDlg dialog



CProcessInspectorDlg::CProcessInspectorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PROCESSINSPECTOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcessInspectorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROCESS_LIST, m_lcProcess);
}

BEGIN_MESSAGE_MAP(CProcessInspectorDlg, CDialogEx)
    ON_WM_MOVE()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDOK, &CProcessInspectorDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CProcessInspectorDlg::OnBnClickedCancel)
    ON_NOTIFY(NM_DBLCLK, IDC_PROCESS_LIST, &CProcessInspectorDlg::OnDblclkProcessList)
END_MESSAGE_MAP()


// CProcessInspectorDlg message handlers

BOOL CProcessInspectorDlg::OnInitDialog()
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
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Add extra initialization here
    m_lcProcess.InsertColumn(0, _T("Name"));
    m_lcProcess.InsertColumn(1, _T("PID"));
    m_lcProcess.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    PopulateProcesses();

    BEGIN_OBJ_MAP(CProcessInspectorDlg);
    OBJ_DEFINE(IDC_PROCESS_LIST, 0, 100, 0, 100);
    END_OBJ_MAP();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProcessInspectorDlg::OnMove(int x, int y)
{
    SAVE_WINDOW_PLACEMENT();
}

void CProcessInspectorDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    UPDATE_OBJ_POSITION(cx, cy);
    SAVE_WINDOW_PLACEMENT();
}

void CProcessInspectorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    UPDATE_MINMAX_INFO(lpMMI);
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CProcessInspectorDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CProcessInspectorDlg::OnPaint()
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
HCURSOR CProcessInspectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CProcessInspectorDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnOK();
}


void CProcessInspectorDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
}

void CProcessInspectorDlg::PopulateProcesses()
{
    m_lcProcess.DeleteAllItems();

    std::set<PROCESS_INFO> processList;

    int nCurIndex = 0;
    DWORD aProcesses[1024];
    DWORD cbNeeded;

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        TRACE(_T("Failed to call EnumProcesses(), err = 0x%08x\r\n"), GetLastError());
        return;
    }

    // Calculate how many process identifiers were returned.
    DWORD cProcesses = cbNeeded / sizeof(DWORD);

    // Print the names of the modules for each process.

    for (DWORD pi = 0; pi < cProcesses; pi++)
    {
        HMODULE hMods[1024];
        HANDLE hProcess;
        DWORD cbNeededMods;
        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

        auto pid = aProcesses[pi];
        if (pid == 0)
            continue;

        // Get a handle to the process.
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, pid);
        if (NULL == hProcess)
        {
            TRACE(_T("Failed to call OpenProces(), pid=0x%08x, err=%s\r\n"), pid, GetErrorString(_T("OpenProcess")));
            continue;
        }

        // Get a list of all the modules in this process.

        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeededMods))
        {
            GetModuleBaseName(hProcess, hMods[0], szProcessName,
                sizeof(szProcessName) / sizeof(TCHAR));

            processList.emplace(szProcessName, pid);
        }

        // Release the handle to the process.
        CloseHandle(hProcess);
    }

    for (auto item : processList)
    {
        TCHAR szProcessId[16];

        m_lcProcess.InsertItem(nCurIndex, item.m_name.c_str());

        wsprintf(szProcessId, _T("%d"), item.m_pid);
        m_lcProcess.SetItemText(nCurIndex, 1, szProcessId);

        nCurIndex++;
    }

    m_lcProcess.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    m_lcProcess.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
}

void CProcessInspectorDlg::OnDblclkProcessList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // Add your control notification handler code here
    *pResult = 0;

    auto iItem = pNMItemActivate->iItem;

    CString processName = m_lcProcess.GetItemText(iItem, 0);
    CString processId = m_lcProcess.GetItemText(iItem, 1);

    DWORD pid = _ttol(processId);

    CProcessMemInfoDlg dlg(processName, pid, this);
    dlg.DoModal();
}
