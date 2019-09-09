#pragma once

#include "DumpControl.h"
#include "afxcmn.h"
#include "afxwin.h"
#include <vector>

// CMemoryDlg dialog

class CMemoryDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMemoryDlg)

public:
	CMemoryDlg(PCTSTR processName, DWORD pid, ULONGLONG baseAddress, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMemoryDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MEMORY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();

private:
    void UpdatePage();

private:
    CStatic m_page_info;
    CScrollBar m_page_scroller;
    CDumpControl m_lcData;
    CString m_processName;
    DWORD m_processId;
    ULONGLONG m_baseAddress;

    const int BYTES_PER_PAGE = 1024;

    int m_total_pages;
    int m_current_page;
    int m_page_jump;

    std::vector<BYTE> m_buffer;
    PBYTE m_pbStartAddress;

public:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
