
// ProcessMonitorDlg.h : header file
//

#pragma once

#include <vector>
#include "DynObj.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "process_watcher.h"
#include "demo_server.h"

// CProcessMonitorDlg dialog
class CProcessMonitorDlg : public CDialogEx
{
// Construction
public:
    CProcessMonitorDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PROCESSMONITOR_DIALOG };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


// Implementation
protected:
    HICON m_hIcon;
    CListCtrl m_lcProcess;
    CEdit m_ecLog;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

    DECLARE_OBJ_MAP();
    afx_msg void OnMove(int, int);                    // ON_WM_MOVE()
    afx_msg void OnSize(UINT, int, int);                // ON_WM_SIZE()
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);    // ON_WM_GETMINMAXINFO()

    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();

    afx_msg LRESULT OnApplicationEvent(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnServerMessage(WPARAM wParam, LPARAM lParam);

    void InitProcessListControl();
    void Log(LPCTSTR lpszFormat, ...);
    void LoadAppList();

private:
    std::vector<application_info> m_process_id_list;
    int m_focus_index = -1;
    demo_server m_server;

    CFont m_font;

};
