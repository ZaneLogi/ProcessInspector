#pragma once

#include "DumpControl.h"
#include "afxcmn.h"

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
    CDumpControl m_lcData;
    CString m_processName;
    DWORD m_processId;
    ULONGLONG m_baseAddress;
};
