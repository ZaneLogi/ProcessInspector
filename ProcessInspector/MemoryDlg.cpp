// MemoryDlg.cpp : implementation file
//

#include "stdafx.h"
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
    DDX_Control(pDX, IDC_SCROLLBAR1, m_page_scroller);
    DDX_Control(pDX, IDC_PAGE_INFO, m_page_info);
}


BEGIN_MESSAGE_MAP(CMemoryDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CMemoryDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CMemoryDlg::OnBnClickedCancel)
    ON_WM_HSCROLL()
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
    strTitle.Format(_T("Memory: %s - pid %d - %p"), m_processName, m_processId, m_baseAddress);
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
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T size = VirtualQueryEx(hProcess, pDataAddress, &mbi, sizeof(mbi));
    if (size == 0)
    {
        CloseHandle(hProcess);
        AfxMessageBox(GetErrorString(_T("VirtualQueryEx")));
        PostMessage(WM_CLOSE);
        return TRUE;
    }

    SIZE_T bytes_read;
    m_buffer.resize(mbi.RegionSize);

    BOOL b = ReadProcessMemory(hProcess, mbi.BaseAddress, m_buffer.data(), mbi.RegionSize, &bytes_read);
    CloseHandle(hProcess);

    if (b)
    {
        m_pbStartAddress = (PBYTE)mbi.BaseAddress;
        m_total_pages = (int)((m_buffer.size() + BYTES_PER_PAGE - 1) / BYTES_PER_PAGE);
        m_current_page = 0;

        SCROLLINFO si = {
            sizeof(SCROLLINFO),
            SIF_RANGE|SIF_POS|SIF_PAGE,
            0, // min
            m_total_pages - 1, // max
            1, // page
            0, // pos
            0 // track pos
        };
        m_page_scroller.SetScrollInfo(&si);

        m_page_jump = 1;
        if (m_total_pages >= 1000)
            m_page_jump = 100;
        else if (m_total_pages >= 100)
            m_page_jump = 10;

        UpdatePage();
    }
    else
    {
        AfxMessageBox(GetErrorString(_T("ReadProcessMemory")));
        OnOK();
    }


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CMemoryDlg::UpdatePage()
{
    CString s;
    s.Format(_T("%d / %d"), m_current_page + 1, m_total_pages);
    m_page_info.SetWindowText(s);

    m_lcData.SetRedraw(FALSE);
    m_lcData.Empty();
    m_lcData.Add(m_buffer.data() + m_current_page * BYTES_PER_PAGE,
        (DWORD)(m_current_page == m_total_pages - 1 ? m_buffer.size() - m_current_page * BYTES_PER_PAGE : BYTES_PER_PAGE));
    m_lcData.SetStartAddress(m_pbStartAddress + m_current_page * BYTES_PER_PAGE);
    m_lcData.SetRedraw(TRUE);
}


void CMemoryDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // TODO: Add your message handler code here and/or call default
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);

    if (pScrollBar != &m_page_scroller)
        return;

    bool changed = false;
    switch (nSBCode) {
    case SB_PAGELEFT:
        m_current_page -= m_page_jump;
        changed = true;
        break;
    case SB_PAGERIGHT:
        m_current_page += m_page_jump;
        changed = true;
        break;
    case SB_LINELEFT:
        m_current_page--;
        changed = true;
        break;
    case SB_LINERIGHT:
        m_current_page++;
        changed = true;
        break;
    case SB_THUMBTRACK:
        m_current_page = nPos;
        UpdatePage();
        break;
    case SB_THUMBPOSITION:
        SCROLLINFO si = {
            sizeof(SCROLLINFO),
            SIF_POS,
            0, // min
            m_total_pages - 1, // max
            0, // page
            m_current_page, // pos
            0 // track pos
        };
        m_page_scroller.SetScrollInfo(&si);
        break;
    }

    if (changed)
    {
        if (m_current_page < 0)
            m_current_page = 0;
        else if (m_current_page + 1 >= m_total_pages)
            m_current_page = m_total_pages - 1;

        UpdatePage();

        SCROLLINFO si = {
            sizeof(SCROLLINFO),
            SIF_POS,
            0, // min
            m_total_pages - 1, // max
            0, // page
            m_current_page, // pos
            0 // track pos
        };
        m_page_scroller.SetScrollInfo(&si);
    }
}
