// ProcessMemInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ProcessInspector.h"
#include "ProcessMemInfoDlg.h"
#include "MemoryDlg.h"
#include "afxdialogex.h"

#include <vector>
#include <algorithm>


const int ADDRESS_COLUMN = 0;
const int STATE_COLUMN = 1;
const int PROTECT_COLUMN = 2;
const int TYPE_COLUMN = 3;
const int SIZE_COLUMN = 4;
const int BASE_COLUMN = 5;


template <class InIter1, class InIter2>
void find_all(unsigned char *base, InIter1 buf_start, InIter1 buf_end, InIter2 pat_start, InIter2 pat_end) {
    for (InIter1 pos = buf_start;
        buf_end != (pos = std::search(pos, buf_end, pat_start, pat_end));
        ++pos)
    {
        TRACE(_T("Found %p\r\n"), base + (pos - buf_start));
    }
}

void find_locs(HANDLE process, const std::vector<BYTE>& pattern)
{
    unsigned char *p = NULL;
    MEMORY_BASIC_INFORMATION info;

    for (p = NULL;
        VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info);
        p += info.RegionSize)
    {
        std::vector<char> buffer;
        std::vector<char>::iterator pos;

        if (info.State == MEM_COMMIT &&
            (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(info.RegionSize);
            ReadProcessMemory(process, p, &buffer[0], info.RegionSize, &bytes_read);
            buffer.resize(bytes_read);
            find_all(p, buffer.begin(), buffer.end(), pattern.begin(), pattern.end());
        }
    }
}

void Test(HANDLE hProcess, const std::vector<BYTE>& pattern)
{
    find_locs(hProcess, pattern);
}


bool FindAndReplace(DWORD dwProcessID, const std::vector<BYTE>& findWhat, const std::vector<BYTE>& replaceWith)
{
    ASSERT(findWhat.size() == replaceWith.size());

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
    if (!hProcess)
    {
        TRACE(_T("FindAndReplace: %s\r\n"), GetErrorString(_T("OpenProcess")));
        return false;
    }

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    PBYTE proc_cur_address = (PBYTE)sys_info.lpMinimumApplicationAddress;
    PBYTE proc_max_address = (PBYTE)sys_info.lpMaximumApplicationAddress;

    std::vector<BYTE> buffer;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mem_basic_info;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mem_basic_info, sizeof(mem_basic_info));
        if (size == 0)
        {
            break;
        }

        if (mem_basic_info.Protect == PAGE_READWRITE &&
            mem_basic_info.State == MEM_COMMIT &&
            (mem_basic_info.Type == MEM_MAPPED || mem_basic_info.Type == MEM_PRIVATE))
        {
            SIZE_T bytes_read;
            buffer.resize(mem_basic_info.RegionSize);
            ReadProcessMemory(hProcess, proc_cur_address, &buffer[0], mem_basic_info.RegionSize, &bytes_read);
            buffer.resize(bytes_read);

            bool found = false;
            for (auto pos = buffer.begin();
                buffer.end() != (pos = std::search(pos, buffer.end(), findWhat.begin(), findWhat.end()));
                ++pos)
            {
                memcpy(&(*pos), replaceWith.data(), findWhat.size());
                found = true;
                break;
            }

            if (found)
            {
                SIZE_T bytes_written;
                BOOL b = WriteProcessMemory(hProcess, proc_cur_address, buffer.data(), mem_basic_info.RegionSize, &bytes_written);
                if (!b)
                {
                    TRACE(_T("FindAndReplace: %s\r\n"), GetErrorString(_T("WriteProcessMemory")));
                }
                break;
            }
        }

        proc_cur_address += mem_basic_info.RegionSize;
    }

    CloseHandle(hProcess);

    return true;
}



// Knuth–Morris–Pratt algorithm in c++
int find_substring(std::string str, std::string pattern)
{
    // Step 0. Should not be empty string
    if (str.size() == 0 || pattern.size() == 0)
        return -1;

    // Step 1. Compute failure function
    std::vector<int> failure(pattern.size());
    std::fill(failure.begin(), failure.end(), -1);

    for (int r = 1, l = -1; r < pattern.size(); r++)
    {
        while (l != -1 && pattern[l + 1] != pattern[r])
            l = failure[l];

        // assert( l == -1 || pattern[l+1] == pattern[r]);
        if (pattern[l + 1] == pattern[r])
            failure[r] = ++l;
    }

    // Step 2. Search pattern
    int tail = -1;
    for (int i = 0; i < str.size(); i++)
    {
        while (tail != -1 && str[i] != pattern[tail + 1])
            tail = failure[tail];

        if (str[i] == pattern[tail + 1])
            tail++;

        if (tail == pattern.size() - 1)
            return i - tail;
    }

    return -1;
}

void test_kmp()
{
    auto a1 = find_substring("abcd", "abcd");
    auto a2 = find_substring("abcd", "ab");
    auto a3 = find_substring("ab", "abcd");
    auto a4 = find_substring("ababc", "abc");
}

// CProcessMemInfoDlg dialog

IMPLEMENT_DYNAMIC(CProcessMemInfoDlg, CDialogEx)

CProcessMemInfoDlg::CProcessMemInfoDlg(PCTSTR processName, DWORD pid, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MEM_INFO, pParent), m_processName(processName), m_processId(pid)
{
}

CProcessMemInfoDlg::~CProcessMemInfoDlg()
{
}

void CProcessMemInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MEM_INFO_LIST, m_lcMemInfo);
}


BEGIN_MESSAGE_MAP(CProcessMemInfoDlg, CDialogEx)
    ON_WM_MOVE()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_BN_CLICKED(IDOK, &CProcessMemInfoDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CProcessMemInfoDlg::OnBnClickedCancel)
    ON_NOTIFY(NM_DBLCLK, IDC_MEM_INFO_LIST, &CProcessMemInfoDlg::OnDblclkMemInfoList)
END_MESSAGE_MAP()


// CProcessMemInfoDlg message handlers


void CProcessMemInfoDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnOK();
}


void CProcessMemInfoDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
}


void CProcessMemInfoDlg::OnMove(int x, int y)
{
    SAVE_WINDOW_PLACEMENT();
}


void CProcessMemInfoDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    UPDATE_OBJ_POSITION(cx, cy);
    SAVE_WINDOW_PLACEMENT();
}


void CProcessMemInfoDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    UPDATE_MINMAX_INFO(lpMMI);
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}


BOOL CProcessMemInfoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add extra initialization here
    SetWindowText(CString(_T("Memory Information: ")) + m_processName);

    m_lcMemInfo.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_lcMemInfo.InsertColumn(ADDRESS_COLUMN, _T("Address"));
    m_lcMemInfo.InsertColumn(STATE_COLUMN, _T("State"));
    m_lcMemInfo.InsertColumn(PROTECT_COLUMN, _T("Protect"));
    m_lcMemInfo.InsertColumn(TYPE_COLUMN, _T("Type"));
    m_lcMemInfo.InsertColumn(SIZE_COLUMN, _T("Size"));
    m_lcMemInfo.InsertColumn(BASE_COLUMN, _T("Base"));

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    PBYTE proc_cur_address = (PBYTE)sys_info.lpMinimumApplicationAddress;
    PBYTE proc_max_address = (PBYTE)sys_info.lpMaximumApplicationAddress;

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

    CString s;
    int nCurIndex = 0;

    while (proc_cur_address < proc_max_address)
    {
        MEMORY_BASIC_INFORMATION mem_basic_info;
        SIZE_T size = VirtualQueryEx(hProcess, proc_cur_address, &mem_basic_info, sizeof(mem_basic_info));
        if (size == 0)
        {
            // it is possible that proc_cur_address can't be greater than proc_max_address,
            // and ERROR_INVALID_PARAMETER
            DWORD err = GetLastError();
            ASSERT(err == ERROR_INVALID_PARAMETER);
            break;
        }

        TRACE("%p - %08x\n", mem_basic_info.BaseAddress, mem_basic_info.RegionSize);

        s.Format(_T("%p"), proc_cur_address);
        m_lcMemInfo.InsertItem(nCurIndex, s);

        switch (mem_basic_info.State) {
        case MEM_COMMIT:
            m_lcMemInfo.SetItemText(nCurIndex, STATE_COLUMN, _T("Commit"));
            break;
        case MEM_FREE:
            m_lcMemInfo.SetItemText(nCurIndex, STATE_COLUMN, _T("Free"));
            break;
        case MEM_RESERVE:
            m_lcMemInfo.SetItemText(nCurIndex, STATE_COLUMN, _T("Reserve"));
            break;
        default:
            m_lcMemInfo.SetItemText(nCurIndex, STATE_COLUMN, _T("?"));
            break;
        }

        s = _T("");
        if (mem_basic_info.Protect & PAGE_EXECUTE) s += (_T("E "));
        if (mem_basic_info.Protect & PAGE_EXECUTE_READ) s += (_T("E_R "));
        if (mem_basic_info.Protect & PAGE_EXECUTE_READWRITE) s += (_T("E_RW "));
        if (mem_basic_info.Protect & PAGE_EXECUTE_WRITECOPY) s += (_T("E_WC "));
        if (mem_basic_info.Protect & PAGE_NOACCESS) s += (_T("NA "));
        if (mem_basic_info.Protect & PAGE_READONLY) s += (_T("RO "));
        if (mem_basic_info.Protect & PAGE_READWRITE) s += (_T("RW "));
        if (mem_basic_info.Protect & PAGE_WRITECOPY) s += (_T("WC "));
        if (mem_basic_info.Protect & PAGE_GUARD) s += (_T("G "));
        if (mem_basic_info.Protect & PAGE_NOCACHE) s += (_T("NC "));
        if (mem_basic_info.Protect & PAGE_WRITECOMBINE) s += (_T("WRITECOMBINE "));
        m_lcMemInfo.SetItemText(nCurIndex, PROTECT_COLUMN, s);

        s = _T("");
        if (mem_basic_info.Type & MEM_IMAGE) s += _T("IMAGE ");
        if (mem_basic_info.Type & MEM_MAPPED) s += _T("MAPPED ");
        if (mem_basic_info.Type & MEM_PRIVATE) s += _T("PRIVATE ");
        if (s.GetLength() == 0) s.Format(_T("%08X"), mem_basic_info.Type);
        m_lcMemInfo.SetItemText(nCurIndex, TYPE_COLUMN, s);

        s.Format(_T("%08X"), mem_basic_info.RegionSize);
        m_lcMemInfo.SetItemText(nCurIndex, SIZE_COLUMN, s);

        s.Format(_T("%p"), mem_basic_info.BaseAddress);
        m_lcMemInfo.SetItemText(nCurIndex, BASE_COLUMN, s);

        nCurIndex++;

        // move to the next memory chunk
        proc_cur_address += mem_basic_info.RegionSize;
    }

    CloseHandle(hProcess);

    int columnCount = m_lcMemInfo.GetHeaderCtrl()->GetItemCount();
    for (int i = 0; i < columnCount; i++)
    {
        m_lcMemInfo.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    }

    BEGIN_OBJ_MAP(CProcessMemInfoDlg);
    OBJ_DEFINE(IDC_MEM_INFO_LIST, 0, 100, 0, 100);
    END_OBJ_MAP();


    /* Testing
    std::vector<BYTE> findWhat;
    findWhat.push_back('1');
    findWhat.push_back('2');
    findWhat.push_back('3');
    findWhat.push_back('x');
    findWhat.push_back('y');
    findWhat.push_back('z');

    std::vector<BYTE> replaceWith;
    replaceWith.push_back('0');
    replaceWith.push_back('.');
    replaceWith.push_back('0');
    replaceWith.push_back('.');
    replaceWith.push_back('0');
    replaceWith.push_back('.');

    FindAndReplace(m_processId, findWhat, replaceWith);
    */
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CProcessMemInfoDlg::OnDblclkMemInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // Add your control notification handler code here
    *pResult = 0;

    auto iItem = pNMItemActivate->iItem;
    CString s = m_lcMemInfo.GetItemText(iItem, ADDRESS_COLUMN);
    PTCHAR  endPtr;
    auto address = _tcstoull(s, &endPtr, 16);

    CMemoryDlg dlg(m_processName, m_processId, address);
    dlg.DoModal();
}
