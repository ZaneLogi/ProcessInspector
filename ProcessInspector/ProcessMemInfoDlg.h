#pragma once
#include "afxcmn.h"
#include "DynObj.h"

// CProcessMemInfoDlg dialog

class CProcessMemInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProcessMemInfoDlg)

public:
	CProcessMemInfoDlg(PCTSTR processName, DWORD pid, CWnd* pParent = NULL);   // standard constructor
	virtual ~CProcessMemInfoDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MEM_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedButtonGo();

	DECLARE_MESSAGE_MAP()

    DECLARE_OBJ_MAP();
    afx_msg void OnMove(int, int);					// ON_WM_MOVE()
    afx_msg void OnSize(UINT, int, int);				// ON_WM_SIZE()
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);	// ON_WM_GETMINMAXINFO()

private:
    CString m_processName;
    DWORD m_processId;
    CListCtrl m_lcMemInfo;

public:
    afx_msg void OnDblclkMemInfoList(NMHDR *pNMHDR, LRESULT *pResult);
};
