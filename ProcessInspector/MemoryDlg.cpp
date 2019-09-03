// MemoryDlg.cpp : implementation file
//

#include "stdafx.h"
#include <vector>
#include "ProcessInspector.h"
#include "MemoryDlg.h"
#include "afxdialogex.h"


// CMemoryDlg dialog

IMPLEMENT_DYNAMIC(CMemoryDlg, CDialogEx)

CMemoryDlg::CMemoryDlg(PCTSTR processName, DWORD pid, ULONGLONG baseAddress, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MEMORY, pParent), m_processName(processName), m_processId(pid), m_baseAddress(baseAddress)
{

}

CMemoryDlg::~CMemoryDlg()
{
}

void CMemoryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DATA, m_lcData);
}


BEGIN_MESSAGE_MAP(CMemoryDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CMemoryDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CMemoryDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CMemoryDlg message handlers


void CMemoryDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnOK();
}


void CMemoryDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
}


BOOL CMemoryDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CWaitCursor cur;

    // Add extra initialization here
    CString strTitle;
    strTitle.Format(_T("Memory: %s - pid %d - %08x"), m_processName, m_processId, m_baseAddress);
    SetWindowText(strTitle);

    HANDLE hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        false,
        m_processId);

    if (hProcess == NULL)
    {
        AfxMessageBox(GetErrorString(_T("OpenProcess")));
        PostMessage(WM_CLOSE);
        return TRUE;
    }

    PVOID pDataAddress = (PVOID)m_baseAddress;
    MEMORY_BASIC_INFORMATION mem_basic_info;
    SIZE_T size = VirtualQueryEx(hProcess, pDataAddress, &mem_basic_info, sizeof(mem_basic_info));
    if (size == 0)
    {
        CloseHandle(hProcess);
        AfxMessageBox(GetErrorString(_T("VirtualQueryEx")));
        PostMessage(WM_CLOSE);
        return TRUE;
    }

    std::vector<BYTE> buffer;
    SIZE_T bytes_read;
    buffer.resize(mem_basic_info.RegionSize);

    BOOL b = ReadProcessMemory(hProcess, mem_basic_info.BaseAddress, buffer.data(), mem_basic_info.RegionSize, &bytes_read);

    if (b)
    {
        m_lcData.Add(buffer.data(), (DWORD)buffer.size());
    }
    else
    {
        AfxMessageBox(GetErrorString(_T("ReadProcessMemory")));
    }

    CloseHandle(hProcess);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
