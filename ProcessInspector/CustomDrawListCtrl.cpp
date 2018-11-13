// CustomDrawListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "CustomDrawListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustomDrawListCtrl

CCustomDrawListCtrl::CCustomDrawListCtrl()
{
}

CCustomDrawListCtrl::~CCustomDrawListCtrl()
{
}


BEGIN_MESSAGE_MAP(CCustomDrawListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CCustomDrawListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustomDrawListCtrl message handlers

void CCustomDrawListCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVCUSTOMDRAW* pNMLVCUSTOMDRAW = (NMLVCUSTOMDRAW*)pNMHDR;

	HDC  hdc = pNMLVCUSTOMDRAW->nmcd.hdc;
	CDC* pDC = NULL;

	int    nItem  = (int)pNMLVCUSTOMDRAW->nmcd.dwItemSpec;
	UINT   nState = pNMLVCUSTOMDRAW->nmcd.uItemState;
	LPARAM lParam = pNMLVCUSTOMDRAW->nmcd.lItemlParam;

	bool bNotifyPostPaint = false;
	bool bNotifyItemDraw = false;
	bool bNotifySubItemDraw = false;
	bool bSkipDefault = false;
	bool bNewFont = false;

	switch (pNMLVCUSTOMDRAW->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		{
			m_pOldItemFont = NULL;
			m_pOldSubItemFont = NULL;

			bNotifyPostPaint = IsNotifyPostPaint();
			bNotifyItemDraw = IsNotifyItemDraw();
  
			if (IsDraw()) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				CRect r(pNMLVCUSTOMDRAW->nmcd.rc);

				if (OnDraw(pDC,r)) {
					bSkipDefault = true;
				}
			}
		}
		break;

	case CDDS_ITEMPREPAINT:
		{
			m_pOldItemFont = NULL;

			bNotifyPostPaint = IsNotifyItemPostPaint(nItem,nState,lParam);
			bNotifySubItemDraw = IsNotifySubItemDraw(nItem,nState,lParam);

			pNMLVCUSTOMDRAW->clrText = TextColorForItem(nItem,nState,lParam);
			pNMLVCUSTOMDRAW->clrTextBk = BkColorForItem(nItem,nState,lParam);

			// set up a different font to use, if any
			CFont* pNewFont = FontForItem(nItem,nState,lParam);
			if (pNewFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				m_pOldItemFont = pDC->SelectObject(pNewFont);
				bNotifyPostPaint = true; // need to restore font
			}

			if (IsItemDraw(nItem,nState,lParam)) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				if (OnItemDraw(pDC,nItem,nState,lParam)) {
					bSkipDefault = true;
				}
			}
		}
		break;

	case CDDS_ITEMPREPAINT|CDDS_SUBITEM:
		{
			int nSubItem = pNMLVCUSTOMDRAW->iSubItem;

			m_pOldSubItemFont = NULL;
			
			bNotifyPostPaint = IsNotifySubItemPostPaint(nItem, nSubItem, nState, lParam);

			pNMLVCUSTOMDRAW->clrText = TextColorForSubItem(nItem,nSubItem,nState,lParam);
			pNMLVCUSTOMDRAW->clrTextBk = BkColorForSubItem(nItem,nSubItem,nState,lParam);

			CFont* pNewFont = FontForSubItem(nItem, nSubItem, nState, lParam);
			if (pNewFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				m_pOldSubItemFont = pDC->SelectObject(pNewFont);
				bNotifyPostPaint = true;    // need to restore font
			}

			if ( IsSubItemDraw(nItem,nSubItem,nState,lParam) ) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				if (OnSubItemDraw(pDC,nItem,nSubItem,nState,lParam)) {
					bSkipDefault = true;
				}
			}
		}
		break;

	case CDDS_ITEMPOSTPAINT|CDDS_SUBITEM:
		{
			int nSubItem = pNMLVCUSTOMDRAW->iSubItem;

			// restore old font if any
			if (m_pOldSubItemFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				pDC->SelectObject(m_pOldSubItemFont);
				m_pOldSubItemFont = NULL;
			}

			// do we want to do any extra drawing?
			if (IsSubItemPostDraw()) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				OnSubItemPostDraw(pDC,nItem,nSubItem,nState,lParam);
			}
		}
		break;

	case CDDS_ITEMPOSTPAINT:
		{
			if (m_pOldItemFont) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				pDC->SelectObject(m_pOldItemFont);
				m_pOldItemFont = NULL;
			}

			// do we want to do any extra drawing?
			if (IsItemPostDraw()) {
				if (! pDC) pDC = CDC::FromHandle(hdc);
				OnItemPostDraw(pDC,nItem,nState,lParam);
			}
		}
		break;

	case CDDS_POSTPAINT:
		{
			if (IsPostDraw()) {
				if (!pDC) pDC = CDC::FromHandle(hdc);
				CRect r(pNMLVCUSTOMDRAW->nmcd.rc);
				OnPostDraw(pDC,r);
			}
		}
		break;
	}

	ASSERT(CDRF_DODEFAULT==0);
	*pResult = 0;
	
	if (bNotifyPostPaint) {
		*pResult |= CDRF_NOTIFYPOSTPAINT;
	}

	if (bNotifyItemDraw) {
		*pResult |= CDRF_NOTIFYITEMDRAW;
	}

	if (bNotifySubItemDraw) {
		*pResult |= CDRF_NOTIFYSUBITEMDRAW;
	}

	if (bNewFont) {
		*pResult |= CDRF_NEWFONT;
	}

	if (bSkipDefault) {
		*pResult |= CDRF_SKIPDEFAULT;
	}

	if (*pResult == 0) {
		// redundant as CDRF_DODEFAULT==0 anyway
		// but shouldn't depend on this in our code
		*pResult = CDRF_DODEFAULT;
	}
}
