#if !defined(AFX_CUSTOMDRAWLISTCTRL_H__E1EFD2E8_45DC_43C1_85D7_9115D4802AEF__INCLUDED_)
#define AFX_CUSTOMDRAWLISTCTRL_H__E1EFD2E8_45DC_43C1_85D7_9115D4802AEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CustomDrawListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCustomDrawListCtrl window

class CCustomDrawListCtrl : public CListCtrl
{
// Construction
public:
	CCustomDrawListCtrl();

// Attributes
public:
	CFont* m_pOldItemFont;
	CFont* m_pOldSubItemFont;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomDrawListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCustomDrawListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCustomDrawListCtrl)
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	virtual bool IsDraw() { return false; }
	virtual bool IsPostDraw() { return false; }

	virtual bool IsItemDraw(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool IsItemPostDraw() { return false; }

	virtual bool IsSubItemDraw(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/)  { return false; }
	virtual bool IsSubItemPostDraw() { return false; }

	virtual bool IsNotifyItemDraw() { return false; }
	virtual bool IsNotifyItemPostPaint(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool IsNotifyPostPaint() { return false; }

	virtual bool IsNotifySubItemDraw(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool IsNotifySubItemPostPaint(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }

	virtual bool OnDraw(CDC* /*pDC*/, const CRect& /*rc*/) { return false; }
	virtual bool OnPostDraw(CDC* /*pDC*/, const CRect& /*rc*/) { return false; }

	virtual bool OnItemDraw(CDC* /*pDC*/, int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool OnItemPostDraw(CDC* /*pDC*/, int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }

	virtual bool OnSubItemDraw(CDC* /*pDC*/, int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }
	virtual bool OnSubItemPostDraw(CDC* /*pDC*/, int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return false; }

	virtual CFont* FontForItem(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return NULL; }
	virtual COLORREF TextColorForItem(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }
	virtual COLORREF BkColorForItem(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }

	virtual CFont* FontForSubItem(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return NULL; }
	virtual COLORREF TextColorForSubItem(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }
	virtual COLORREF BkColorForSubItem(int /*nItem*/, int /*nSubItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return CLR_DEFAULT; }

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUSTOMDRAWLISTCTRL_H__E1EFD2E8_45DC_43C1_85D7_9115D4802AEF__INCLUDED_)
