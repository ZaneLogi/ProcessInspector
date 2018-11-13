
// ProcessInspectorDlg.h : header file
//

#pragma once

#include "afxcmn.h"
#include "DynObj.h"

// CProcessInspectorDlg dialog
class CProcessInspectorDlg : public CDialogEx
{
// Construction
public:
	CProcessInspectorDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESSINSPECTOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
    CListCtrl m_lcProcess;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnDblclkProcessList(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

    DECLARE_OBJ_MAP();
    afx_msg void OnMove(int, int);					// ON_WM_MOVE()
    afx_msg void OnSize(UINT, int, int);				// ON_WM_SIZE()
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);	// ON_WM_GETMINMAXINFO()

private:
    void PopulateProcesses();
};
