#if !defined(AFX_DUMPCONTROL_H__62609BA5_A74F_46BB_AF0B_8E261C389A7F__INCLUDED_)
#define AFX_DUMPCONTROL_H__62609BA5_A74F_46BB_AF0B_8E261C389A7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DumpControl.h : header file

#include "CustomDrawListCtrl.h"

#define IDM_DUMPCTRL_COPY             3805
#define IDM_DUMPCTRL_COPY_HEX_ONLY    3806
#define IDM_DUMPCTRL_SELECTION_START  3807
#define IDM_DUMPCTRL_SELECTION_END    3808
#define IDM_DUMPCTRL_SELECTION_CANCEL 3809
#define IDM_DUMPCTRL_FIND             3810

#define COPY_FLAG_ADR 0x00000001
#define COPY_FLAG_HEX 0x00000002
#define COPY_FLAG_ASC 0x00000004

#define COPY_FLAG_ALL 0x00000007

#define COLOR1      RGB(192,128,128)
#define COLOR2      RGB(128,192,128)
#define COLOR_SEL   RGB(128,128,192)
#define COLOR_SEL_R RGB(255,255,  0) // yellow

/////////////////////////////////////////////////////////////////////////////
// CDumpControl window

class CDumpControl : public CCustomDrawListCtrl
{
// Construction
public:
    CDumpControl();

// Attributes
public:
    TEXTMETRIC m_tm;
    PVOID m_pvStartAddress;
    int m_nHexPartUnitWidth;
    int m_nHexPartTextWidth;
    int m_nHexPartBoundWidth;
    int m_nHexPartLeft;
    int m_nAscPartUnitWidth;
    int m_nAscPartTextWidth;
    int m_nAscPartBoundWidth;
    int m_nAscPartLeft;
    int m_nHeaderCtrlOffset;
    int m_nBytesAdded;
    int m_nMarkHit;
    int m_nMarkStart;
    int m_nMarkEnd;

    bool m_bMouseCaptured;
    int  m_nHitCaptured;

    bool m_bEditable;

    CFindReplaceDialog*    m_pFindDlg;
    int  m_nLastSearchPosition;

// Operations
private:
    int FindChr( UCHAR ucTarget, int nStart = 0, BOOL bSearchDown = TRUE ) const;
    int FindBlock( PUCHAR pucTarget, int nSize, int nStart = 0, BOOL bSearchDown = TRUE ) const;

public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDumpControl)
    protected:
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CDumpControl();
    void Empty();
    void Add( PBYTE pData, DWORD dwLength );
    void SetStartAddress( PVOID pvAddress );
    int  GetData( DWORD dwIndex ) const;
    int  GetData( PBYTE pBuf, DWORD dwSize, int nStart = 0 ) const;
    int  GetDataLength() const;
    int  GetItemIndex( CPoint pt, bool bScreenCoord );
    void EnableEdit( bool bEnable = true );
    void DeleteData( int nStart, int nCount );
    void InsertData( int nStart, BYTE ucVal );

    // Generated message map functions
protected:
    //{{AFX_MSG(CDumpControl)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
public:
    void CopyToClipboard(DWORD dwFlags = COPY_FLAG_ALL);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnCopy();
    afx_msg void OnCopyHexOnly();
    afx_msg void OnMarkStart();
    afx_msg void OnMarkEnd();
    afx_msg void OnCancelSelection();
    afx_msg void OnFind();
    afx_msg LRESULT OnFindMsgString( WPARAM, LPARAM );
    afx_msg void OnLButtonDown( UINT, CPoint );
    afx_msg void OnLButtonUp( UINT, CPoint );
    afx_msg void OnMouseMove( UINT, CPoint );
    afx_msg void OnKeyDown( UINT, UINT, UINT );
    afx_msg void OnChar( UINT, UINT, UINT );

protected:
    virtual bool IsNotifyItemDraw() { return true; }
    virtual bool IsNotifySubItemDraw(int /*nItem*/, UINT /*nState*/, LPARAM /*lParam*/) { return true; }
    virtual bool IsSubItemDraw( int nItem, int nSubItem, UINT nState, LPARAM lParam )
    {
        return (nSubItem==1 || nSubItem==2);
    }
    virtual bool OnSubItemDraw( CDC* pDC, int nItem, int nSubItem, UINT nState, LPARAM lParam )
    {
        CRect rcBound;
        GetSubItemRect( nItem, nSubItem, LVIR_BOUNDS, rcBound );

        int nLeftOffset;
        int nChNum;
        int nUnitWidth;

        if ( nSubItem == 1 )
        {
            nLeftOffset = m_nHexPartLeft;
            nChNum = 12;
            nUnitWidth = m_nHexPartUnitWidth;
        }
        else
        {
            nLeftOffset = m_nAscPartLeft;
            nChNum = 4;
            nUnitWidth = m_nAscPartUnitWidth;
        }

        //
        // Fill the ground with COLOR1 or COLOR2 based on its position
        //
        CRect rc;
        rc.left   = rcBound.left;
        rc.right  = rc.left + nLeftOffset + nChNum * m_tm.tmMaxCharWidth;
        rc.top    = rcBound.top;
        rc.bottom = rcBound.bottom;

        // FIX: visual error
        rc.left += 1;
        rc.right -= 1;
        rc.bottom -= 1;

        pDC->FillSolidRect( rc.left, rc.top, rc.Width(), rc.Height(), COLOR1 ); // group 1
        rc.left = rc.right;
        rc.right = rc.left + nChNum * m_tm.tmMaxCharWidth;
        pDC->FillSolidRect( rc.left, rc.top, rc.Width(), rc.Height(), COLOR2 ); // group 2
        rc.left = rc.right;
        rc.right = rc.left + nChNum * m_tm.tmMaxCharWidth;
        pDC->FillSolidRect( rc.left, rc.top, rc.Width(), rc.Height(), COLOR1 ); // group 3
        rc.left = rc.right;
        rc.right = rcBound.right;
        pDC->FillSolidRect( rc.left, rc.top, rc.Width(), rc.Height(), COLOR2 ); // group 4

        int nUnitIndex = nItem * 16;
        if ( m_nMarkStart > nUnitIndex+15 || m_nMarkEnd < nUnitIndex )
        {
            // not overlapped, nothing to do
            // ie, no unit is in the selected range
        }
        else
        {
            if ( m_nMarkStart < nUnitIndex )
                rc.left = rcBound.left + nLeftOffset;
            else
                rc.left = rcBound.left + nLeftOffset + (m_nMarkStart-nUnitIndex) * nUnitWidth;

            if ( m_nMarkEnd > nUnitIndex + 15 )
                rc.right = rcBound.right - nLeftOffset;
            else
                rc.right = rcBound.right - nLeftOffset - (nUnitIndex+15-m_nMarkEnd) * nUnitWidth;

            pDC->FillSolidRect( rc.left, rc.top, rc.Width(), rc.Height(), COLOR_SEL ); // fill a selected color

            //
            // fill the left and right edge
            //
            pDC->FillSolidRect( rc.left, rc.top, 1, rc.Height(), COLOR_SEL_R );
            pDC->FillSolidRect( rc.right - 1, rc.top, 1, rc.Height(), COLOR_SEL_R );

            //
            // fill the top edge
            //
            if ( m_nMarkStart - nUnitIndex >= 0 ) { // first line
                pDC->FillSolidRect( rc.left, rc.top, rc.Width(), 1, COLOR_SEL_R );
            }
            else if ( nUnitIndex == (m_nMarkStart&(~0x0F))+16 ) { // second line
                int nWidth = rc.Width();
                int nLimit = (m_nMarkStart&0x0F) * nUnitWidth;
                if ( nWidth > nLimit )
                    nWidth = nLimit;
                pDC->FillSolidRect( rc.left, rc.top, nWidth, 1, COLOR_SEL_R );
            }

            //
            // fill the bottom edge
            //
            if ( m_nMarkEnd - nUnitIndex < 16 ) { // last line
                pDC->FillSolidRect( rc.left, rc.top + rc.Height() - 2, rc.Width(), 1, COLOR_SEL_R );
            }
            else if ( nUnitIndex == (m_nMarkEnd&(~0x0F))-16 ) { // last second line
                int nStart = rc.left;
                int nLimit = rcBound.left + nLeftOffset + ( (m_nMarkEnd&0x0F) + 1 ) * nUnitWidth;
                if ( nStart < nLimit )
                    nStart = nLimit;
                pDC->FillSolidRect( CRect( CPoint(nStart, rc.top + rc.Height() - 2), CPoint(rc.right,rc.bottom-1) ), COLOR_SEL_R );
            }
        }

        CString str = GetItemText( nItem, nSubItem );
        rcBound.left += m_nHexPartLeft;
        rcBound.right -= m_nHexPartLeft;
        int m = pDC->SetBkMode(TRANSPARENT);
        pDC->DrawText( str, rcBound, 0 );
        pDC->SetBkMode(m);
        return true;
    }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DUMPCONTROL_H__62609BA5_A74F_46BB_AF0B_8E261C389A7F__INCLUDED_)
