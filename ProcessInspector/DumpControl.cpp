// DumpControl.cpp : implementation file

#include "stdafx.h"
#include "DumpControl.h"
#include "HexString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT WM_FINDREPLACE = -1;

/////////////////////////////////////////////////////////////////////////////
// CDumpControl

CDumpControl::CDumpControl()
{
    if ( WM_FINDREPLACE == -1 )
    {
        WM_FINDREPLACE = RegisterWindowMessage(FINDMSGSTRING);
    }

    m_pFindDlg = NULL;
    m_nLastSearchPosition = 0;

    m_nBytesAdded = 0;
    m_nMarkStart = -1;
    m_nMarkEnd = -1;

    m_pvStartAddress = nullptr;

    m_bMouseCaptured = false;

    m_bEditable = false;
}

CDumpControl::~CDumpControl()
{
}

BEGIN_MESSAGE_MAP(CDumpControl, CCustomDrawListCtrl)
    //{{AFX_MSG_MAP(CDumpControl)
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
    ON_NOTIFY_REFLECT(NM_CLICK, OnNMClick)
    ON_COMMAND(IDM_DUMPCTRL_COPY, OnCopy)
    ON_COMMAND(IDM_DUMPCTRL_COPY_HEX_ONLY, OnCopyHexOnly)
    ON_COMMAND(IDM_DUMPCTRL_SELECTION_START, OnMarkStart )
    ON_COMMAND(IDM_DUMPCTRL_SELECTION_END, OnMarkEnd )
    ON_COMMAND(IDM_DUMPCTRL_SELECTION_CANCEL, OnCancelSelection )
    ON_COMMAND(IDM_DUMPCTRL_FIND, OnFind )
    ON_REGISTERED_MESSAGE( WM_FINDREPLACE, OnFindMsgString )
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDumpControl message handlers

void CDumpControl::PreSubclassWindow()
{
    // TODO: Add your specialized code here and/or call the base class
    CListCtrl::PreSubclassWindow();
    ModifyStyle( LVS_TYPEMASK | LVS_SORTASCENDING | LVS_SORTDESCENDING | LVS_SINGLESEL,
        LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS );

    SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_INFOTIP|LVS_EX_FULLROWSELECT);

    CClientDC dc(this);
    dc.SelectStockObject( ANSI_FIXED_FONT );
    dc.GetTextMetrics( &m_tm );

    m_nHexPartUnitWidth = m_tm.tmMaxCharWidth * 3;
    m_nHexPartTextWidth = m_nHexPartUnitWidth * 16;

    m_nAscPartUnitWidth = m_tm.tmMaxCharWidth;
    m_nAscPartTextWidth = m_nAscPartUnitWidth * 16;

    InsertColumn( 0, _T("Address") );
    InsertColumn( 1, _T("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F") );
    InsertColumn( 2, _T("0123456789ABCDEF") );
    InsertColumn( 3, _T("") );

    SetFont( CFont::FromHandle( (HFONT)GetStockObject(ANSI_FIXED_FONT) ), FALSE );

    SetColumnWidth( 0, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 1, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 2, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 3, LVSCW_AUTOSIZE_USEHEADER );

    RECT rcThis, rcHdr;
    GetWindowRect(&rcThis);
    GetHeaderCtrl()->GetWindowRect(&rcHdr);
    m_nHeaderCtrlOffset = rcHdr.left - rcThis.left;
}

void CDumpControl::EnableEdit( bool bEnable )
{
    m_bEditable = bEnable;
}

#define ITEMS_PER_LINE 16 // must be power of 2

void CDumpControl::Add(PBYTE pData, DWORD dwLength)
{
    int nItemCount = GetItemCount();

    CString strTmp, strHex, strChar;
    int nRemained = (m_nBytesAdded % ITEMS_PER_LINE);
    if ( nRemained != 0 ) {
        strHex  = GetItemText( nItemCount - 1, 1 );
        for ( unsigned i = nRemained; i < ITEMS_PER_LINE && i < dwLength; i++ ) {
            strTmp.Format( _T("%02X "), pData[i] );
            strHex += strTmp;
            m_nBytesAdded ++;
        }
        SetItemText( nItemCount-1 , 1, strHex );

        strChar = GetItemText( nItemCount - 1, 1 );
        for ( DWORD i = nRemained; i < ITEMS_PER_LINE && i < dwLength; i++ ) {
            if ( pData[i] < 32 || pData[i] > 127 ) {
                strChar += _T('.');
            }
            else {
                strTmp.Format( _T("%c"), pData[i] );
                strChar += strTmp;
            }
        }
        SetItemText( nItemCount-1, 2, strChar );

        int n = ITEMS_PER_LINE - nRemained;
        dwLength -= n;
        pData += n;
    }

    while ( dwLength > 0 ) {
        strTmp.Format( _T("%p"), (PBYTE)m_pvStartAddress + nItemCount * ITEMS_PER_LINE );
        InsertItem( nItemCount, strTmp );

        strHex.Empty();
        for ( unsigned i = 0; i < ITEMS_PER_LINE && i < dwLength; i++ ) {
            strTmp.Format( _T("%02X "), pData[i] );
            strHex += strTmp;
            m_nBytesAdded ++;
        }
        SetItemText( nItemCount, 1, strHex );

        strChar.Empty();
        for ( DWORD i = 0; i < ITEMS_PER_LINE && i < dwLength; i++ ) {
            if ( pData[i] < 32 || pData[i] > 127 ) {
                strChar += _T('.');
            }
            else {
                strTmp.Format( _T("%c"), pData[i] );
                strChar += strTmp;
            }
        }
        SetItemText( nItemCount, 2, strChar );

        if ( dwLength < ITEMS_PER_LINE )
            break;

        nItemCount++;
        dwLength -= ITEMS_PER_LINE;
        pData += ITEMS_PER_LINE;
    }

    SetColumnWidth( 0, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 1, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 2, LVSCW_AUTOSIZE_USEHEADER );
    SetColumnWidth( 3, LVSCW_AUTOSIZE_USEHEADER );

    CRect rc;
    GetSubItemRect( 0, 1, LVIR_BOUNDS, rc );
    m_nHexPartBoundWidth = rc.Width();
    m_nHexPartLeft = (m_nHexPartBoundWidth - m_nHexPartTextWidth)/2;

    GetSubItemRect( 0, 2, LVIR_BOUNDS, rc );
    m_nAscPartBoundWidth = rc.Width();
    m_nAscPartLeft = (m_nAscPartBoundWidth - m_nAscPartTextWidth)/2;
}

void CDumpControl::SetStartAddress( PVOID pvAddress )
{
    m_pvStartAddress = pvAddress;
    CString strTmp;
    int nItemCount = GetItemCount();
    for ( int i = 0; i < nItemCount; i++ )
    {
        strTmp.Format( _T("%p"), (PBYTE)m_pvStartAddress + i * ITEMS_PER_LINE );
        SetItemText( i, 0, strTmp );
    }
}

void CDumpControl::Empty()
{
    DeleteAllItems();
    m_nBytesAdded = 0;
    m_nMarkStart = -1;
    m_nMarkEnd = -1;
}

int CDumpControl::GetData( DWORD dwIndex ) const
{
    UCHAR auc[ITEMS_PER_LINE];
    CString str;

    if ( dwIndex >= (DWORD)m_nBytesAdded )
        return -1;

    str = GetItemText( dwIndex/ITEMS_PER_LINE, 1 );
    HexStringToArray( str, auc, sizeof(auc) );

    return auc[dwIndex % ITEMS_PER_LINE];
}

int CDumpControl::GetData( PBYTE pBuf, DWORD dwSize, int nStart ) const
{
    if ( nStart < 0 || dwSize == 0 )
        return 0;

    UCHAR auc[ITEMS_PER_LINE];
    CString str;
    int nItemCount = GetItemCount();
    int nRet = 0;

    int nLine = nStart / ITEMS_PER_LINE;
    if ( nItemCount <= nLine )
        return 0;

    int nRemainder = (nStart % ITEMS_PER_LINE);
    if ( nRemainder != 0 )
    {
        str = GetItemText( nLine, 1 );
        int n = HexStringToArray( str, auc, sizeof(auc) );
        n -= nRemainder;
        if ( n > (int)dwSize )
            n = dwSize;

        memcpy( pBuf, &auc[nRemainder], n );
        pBuf += n;
        dwSize -= n;
        nRet += n;

        nLine++;
    }

    int i;
    for ( i = nLine; i < nItemCount && dwSize >= ITEMS_PER_LINE; i++ )
    {
        str = GetItemText( i, 1 );
        int n = HexStringToArray( str, auc, sizeof(auc) );
        memcpy( pBuf, auc, sizeof(auc) );
        dwSize -= n;
        pBuf += n;
        nRet += n;
    }

    if ( dwSize > 0 && i < nItemCount )
    {
        str = GetItemText( i, 1 );
        int n = HexStringToArray( str, auc, sizeof(auc) );
        if ( n > (int)dwSize )
            n = dwSize;

        memcpy( pBuf, auc, n );
        pBuf += n;
        dwSize -= n;
        nRet += n;
    }

    return nRet;
}

int CDumpControl::GetDataLength() const
{
    return m_nBytesAdded;
}

int CDumpControl::GetItemIndex( CPoint hit, bool bScreenCoord )
{
    if ( bScreenCoord )
        ScreenToClient(&hit);

    CRect rcCol1, rcCol2;
    GetHeaderCtrl()->GetItemRect( 1, &rcCol1 );
    GetHeaderCtrl()->GetItemRect( 2, &rcCol2 );

    RECT rcThis, rcHdr;
    GetWindowRect(&rcThis);
    GetHeaderCtrl()->GetWindowRect(&rcHdr);

    int nOffset = rcHdr.left - (rcThis.left + m_nHeaderCtrlOffset);

    rcCol1.OffsetRect(nOffset,0);
    rcCol2.OffsetRect(nOffset,0);

    int nHitIndex = -1;

    UINT uFlags = LVHT_ONITEM;
    int nItem = HitTest( hit, &uFlags );
    if ( nItem >= 0 )
    {
        if ( rcCol1.left <= hit.x && hit.x < rcCol1.right )
        {
            nHitIndex = nItem * 16;
            if ( hit.x < rcCol1.left + m_nHexPartLeft + m_nHexPartUnitWidth ) {
                // first one
            }
            else if ( hit.x >= rcCol1.right - m_nHexPartLeft - m_nHexPartUnitWidth ) {
                // last one
                nHitIndex += 15;
            }
            else {
                nHitIndex += (hit.x - rcCol1.left - m_nHexPartLeft)/m_nHexPartUnitWidth;
            }
        }
        else if ( rcCol2.left <= hit.x && hit.x < rcCol2.right )
        {
            nHitIndex = nItem * 16;
            if ( hit.x < rcCol2.left + m_nAscPartLeft + m_nAscPartUnitWidth ) {
                // first one
            }
            else if ( hit.x >= rcCol2.right - m_nAscPartLeft - m_nAscPartUnitWidth ) {
                // last one
                nHitIndex += 15;
            }
            else {
                nHitIndex += (hit.x - rcCol2.left - m_nAscPartLeft)/m_nAscPartUnitWidth;
            }
        }
    }

    return nHitIndex;
}

void CDumpControl::OnContextMenu(CWnd* pWnd, CPoint point)
{
    CDWordArray adwSelected;
    POSITION pos = GetFirstSelectedItemPosition();
    while ( pos != NULL ) {
        int nItem = GetNextSelectedItem(pos);
        adwSelected.Add(nItem);
    }

    if ( adwSelected.GetSize() <= 0 )
        return;

    CMenu menu;
    BOOL b = menu.CreatePopupMenu();
    if ( !b )
        return;

    int nMenuIndex = 0;

    menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_COPY, _T("&Copy") );
    menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_COPY_HEX_ONLY, _T("Copy Hex &Only") );

    if ( (m_nMarkHit = GetItemIndex(point,true)) >= 0 )
    {
        menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_SEPARATOR );
        menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_SELECTION_START, _T("Mark &Start") );
        menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_SELECTION_END,   _T("Mark &End") );
        menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_SELECTION_CANCEL,_T("C&ancel Selection") );
    }

    menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_SEPARATOR );
    menu.InsertMenu( nMenuIndex++, MF_BYPOSITION|MF_STRING, IDM_DUMPCTRL_FIND, _T("&Find...") );

    menu.TrackPopupMenu( TPM_CENTERALIGN, point.x, point.y, this );
}

void CDumpControl::OnCopy()
{
    CopyToClipboard(COPY_FLAG_ALL);
}

void CDumpControl::OnCopyHexOnly()
{
    CopyToClipboard(COPY_FLAG_HEX);
}

void CDumpControl::OnMarkStart()
{
    m_nMarkStart = m_nMarkHit;
    if ( m_nMarkStart > m_nMarkEnd || m_nMarkEnd == -1 ) {
        m_nMarkEnd = m_nMarkStart;
    }

    Invalidate();
}

void CDumpControl::OnMarkEnd()
{
    m_nMarkEnd = m_nMarkHit;
    if ( m_nMarkStart > m_nMarkEnd || m_nMarkStart == -1 ) {
        m_nMarkStart = m_nMarkEnd;
    }

    Invalidate();
}

void CDumpControl::OnCancelSelection()
{
    m_nMarkStart = -1;
    m_nMarkEnd = -1;
    Invalidate();
}

void CDumpControl::OnFind()
{
    if ( m_pFindDlg == NULL )
    {
        m_pFindDlg = new CFindReplaceDialog();
        if ( m_pFindDlg )
        {
            m_pFindDlg->Create( TRUE, _T(""), 0, FR_DOWN, this );
        }
    }
    else
    {
        m_pFindDlg->SetFocus();
    }
}

LRESULT CDumpControl::OnFindMsgString( WPARAM wParam, LPARAM lParam )
{
    LPFINDREPLACE lpFindReplace = (LPFINDREPLACE)lParam;

    if ( lpFindReplace->Flags & FR_DIALOGTERM )
    {
        m_pFindDlg = NULL;
    }
    else if ( lpFindReplace->Flags & FR_FINDNEXT )
    {
        CString strFind = m_pFindDlg->GetFindString();
        UCHAR aucData[16];
        int nCount = HexStringToArray( strFind, aucData, sizeof(aucData) );
        if ( nCount > 0 )
        {
            int nFound = FindBlock( aucData, nCount, m_nLastSearchPosition, m_pFindDlg->SearchDown() );
            if ( nFound >= 0 )
            {
                m_nMarkStart = nFound;
                m_nMarkEnd = nFound + nCount - 1;
                Invalidate();

                m_nLastSearchPosition = nFound + (m_pFindDlg->SearchDown() ? 1 : -1);
                if ( m_nLastSearchPosition < 0 )
                    m_nLastSearchPosition = 0;
                else if ( m_nLastSearchPosition >= m_nBytesAdded )
                    m_nLastSearchPosition = m_nBytesAdded - 1;

                EnsureVisible( nFound / ITEMS_PER_LINE, FALSE );
            }
        }
    }

    return S_OK;
}

int CDumpControl::FindChr( UCHAR ucTarget, int nStart, BOOL bSearchDown ) const
{
    if ( nStart < 0 || nStart >= m_nBytesAdded )
        return -1;

    UCHAR auc[ITEMS_PER_LINE];
    CString str;
    int nItemCount = GetItemCount();

    int nLine = nStart / ITEMS_PER_LINE;
    if ( nLine >= nItemCount )
        return -1;

    if ( bSearchDown )
    {
        int nRemainder = nStart % ITEMS_PER_LINE;
        if ( nRemainder > 0 )
        {
            str = GetItemText( nLine, 1 );
            int n = HexStringToArray( str, auc, sizeof(auc) );
            PUCHAR p = (PUCHAR)memchr( auc + nRemainder, ucTarget, n - nRemainder );
            if ( p )
            {
                return (int)(nLine * ITEMS_PER_LINE + (p-auc));
            }
            nLine++;
        }

        for ( int i = nLine; i < nItemCount; i++ )
        {
            str = GetItemText( i, 1 );
            int n = HexStringToArray( str, auc, sizeof(auc) );
            PUCHAR p = (PUCHAR)memchr( auc, ucTarget, n );
            if ( p )
            {
                return (int)(i * ITEMS_PER_LINE + (p-auc));
            }
        }
    }
    else // Search Up
    {
        int nRemainder = nStart % ITEMS_PER_LINE;
        str = GetItemText( nLine, 1 );
        int n = HexStringToArray( str, auc, sizeof(auc) );
        for ( int i = nRemainder; i >= 0; i-- )
        {
            if ( auc[i] == ucTarget )
                return nLine * ITEMS_PER_LINE + i;
        }
        nLine--;

        for ( int i = nLine; i >= 0; i-- )
        {
            str = GetItemText( i, 1 );
            int n = HexStringToArray( str, auc, sizeof(auc) );
            for ( int j = n - 1; j >= 0; j-- )
            {
                if ( auc[j] == ucTarget )
                    return i * ITEMS_PER_LINE + j;
            }
        }
    }

    return -1;
}

int CDumpControl::FindBlock( PUCHAR pucTarget, int nSize, int nStart, BOOL bSearchDown ) const
{
    CWaitCursor cur;

    if ( nSize <= 0 )
        return -1;

    ASSERT( nSize <= 16 );
    if ( nSize > 16 )
        nSize = 16;

    if ( bSearchDown && ( nStart < 0 || nStart + nSize > m_nBytesAdded ) )
        return -1;

    if ( !bSearchDown && ( nStart < 0 || nStart - nSize + 1 < 0 ) )
        return -1;

    if ( !bSearchDown )
        nStart = nStart - nSize + 1;


    BYTE auc[16];
    int nDir = bSearchDown ? 1 : -1;

    while ( nStart >= 0 && nStart < m_nBytesAdded )
    {
        int nIndex = FindChr( *pucTarget, nStart, bSearchDown );

        if ( nIndex < 0 )
            break;
        else if ( nSize == GetData( auc, nSize, nIndex ) && 0 == memcmp( pucTarget, auc, nSize ) )
        {
            return nIndex;
        }
        nStart = nIndex + nDir;
    }

    return -1;
}

void CDumpControl::CopyToClipboard(DWORD dwFlags)
{
    CDWordArray adwSelected;
    POSITION pos = GetFirstSelectedItemPosition();
    while ( pos != NULL ) {
        int nItem = GetNextSelectedItem(pos);
        adwSelected.Add(nItem);
    }

    if ( adwSelected.GetSize() <= 0 ) {
        AfxMessageBox( _T("Nothing to copy.") );
        return;
    }

    if ( !OpenClipboard() ) {
        AfxMessageBox( _T("Cannot open the Clipboard") );
        return;
    }

    // Remove the current Clipboard contents
    if( !EmptyClipboard() ) {
        AfxMessageBox( _T("Cannot empty the Clipboard" ) );
        return;
    }

    // Get the currently selected data
    int nItemCount = GetItemCount();
    if ( adwSelected[adwSelected.GetSize()-1] == nItemCount-1 && (m_nBytesAdded % ITEMS_PER_LINE) != 0 ) {
        // last line is selected and not a multiple of ITEMS_PER_LINE
        nItemCount = (int)adwSelected.GetSize()-1;
    }
    else {
        nItemCount = (int)adwSelected.GetSize();
    }

    CString strText;
    if ( dwFlags == COPY_FLAG_ALL ) {
        strText = _T("Address |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |0123456789ABCDEF\r\n");
        strText+= _T("--------+------------------------------------------------+----------------\r\n");
    }

    for ( int i = 0; i < nItemCount; i++ ) {
        if ( dwFlags & COPY_FLAG_ADR ) {
            strText += GetItemText( adwSelected[i], 0 );
        }
        if ( dwFlags & COPY_FLAG_HEX ) {
            if ( dwFlags & COPY_FLAG_ADR )
                strText += _T("|");
            strText += GetItemText( adwSelected[i], 1 );
        }
        if ( dwFlags & COPY_FLAG_ASC ) {
            if ( dwFlags & (COPY_FLAG_ADR|COPY_FLAG_HEX) )
                strText += _T("|");
            strText += GetItemText( adwSelected[i], 2 );
        }
        strText += _T("\r\n");
    }

    if ( adwSelected.GetSize() != nItemCount ) {
        // last line is selected and not a multiple of ITEMS_PER_LINE
        int i = GetItemCount() -1 ;

        if ( dwFlags & COPY_FLAG_ADR ) {
            strText += GetItemText( i, 0 );
        }

        if ( dwFlags & COPY_FLAG_HEX ) {
            if ( dwFlags & COPY_FLAG_ADR )
                strText += _T("|");
            CString strHex = GetItemText( i, 1 );
            CString strSpaces1( _T(' '), ITEMS_PER_LINE * 3 );
            int n1 = strSpaces1.GetLength() - strHex.GetLength();
            strText += strHex;
            strText += strSpaces1.Left(n1);
        }

        if ( dwFlags & COPY_FLAG_ASC ) {
            if ( dwFlags & (COPY_FLAG_ADR|COPY_FLAG_HEX) )
                strText += _T("|");
            CString strChr = GetItemText( i, 2 );
            CString strSpaces2( _T(' '), ITEMS_PER_LINE );
            int n2 = strSpaces2.GetLength() - strChr.GetLength();
            strText += strChr;
            strText += strSpaces2.Left(n2);
        }
        strText += _T("\r\n");
    }

    int nLen = strText.GetLength();

    // Allocate a global memory object for the text.
    HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, ((nLen+1) * sizeof(TCHAR)));
    if (hglbCopy == NULL) {
        CloseClipboard();
        AfxMessageBox( _T("Failed to allocate memory!") );
        return;
    }

    // Lock the handle and copy the text to the buffer.
    LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
    CopyMemory( lptstrCopy, strText.GetBuffer(nLen), (nLen * sizeof(TCHAR)) );
    lptstrCopy[nLen] = (TCHAR)0;    // null character
    GlobalUnlock(hglbCopy);

    // Place the handle on the clipboard.
    UINT uiFormat = (sizeof(TCHAR) == sizeof(char))
        ? CF_TEXT : CF_UNICODETEXT;

    if ( ::SetClipboardData(uiFormat, hglbCopy) == NULL ) {
        AfxMessageBox( _T("Unable to set Clipboard data") );
        CloseClipboard();
        return;
    }

    CloseClipboard();
}

void CDumpControl::DeleteData( int nStart, int nCount )
{
    if ( nStart < 0 )
    {
        nCount += nStart;
        nStart = 0;
    }

    if ( nCount < 0 )
        return;

    if ( nStart + nCount > m_nBytesAdded )
    {
        nCount = m_nBytesAdded - nStart;
    }

    int src;
    int dst;

    for ( src = nStart + nCount, dst = nStart; src < m_nBytesAdded; src++, dst++ )
    {
        int nSrcLine = src / 16;
        int nSrcChar = src % 16;

        int nDstLine = dst / 16;
        int nDstChar = dst % 16;

        CString strSrc = GetItemText( nSrcLine, 1 ).Mid( nSrcChar * 3, 3 );

        CString strDst = GetItemText( nDstLine, 1 );
        strDst.Delete(nDstChar*3,3);
        strDst.Insert(nDstChar*3,strSrc);

        SetItemText( nDstLine, 1, strDst );

        strSrc = GetItemText( nSrcLine, 2 ).Mid( nSrcChar );

        strDst = GetItemText( nDstLine, 2 );
        strDst.Delete(nDstChar,1);
        strDst.Insert(nDstChar,strSrc);

        SetItemText( nDstLine, 2, strDst );
    }

    if ( dst % 16 != 0 )
    {
        int nLine = dst / 16;
        int nChar = dst % 16;

        CString str = GetItemText( nLine, 1 ).Left( nChar * 3 );
        SetItemText( nLine, 1, str );

        str = GetItemText( nLine, 2 ).Left( nChar );
        SetItemText( nLine, 2, str );
    }

    m_nBytesAdded = dst;

    if ( m_nBytesAdded == 0 )
    {
        DeleteAllItems();
    }
    else
    {
        int nLastLine = m_nBytesAdded / 16;
        while ( GetItemCount() > nLastLine + 1 )
        {
            DeleteItem( nLastLine + 1 );
        }
    }
}

void CDumpControl::InsertData( int nStart, BYTE ucVal )
{
    if ( nStart < 0 )
    {
        nStart = 0;
    }

    if ( nStart > m_nBytesAdded )
    {
        nStart = m_nBytesAdded;
    }

    if ( m_nBytesAdded % 16 == 0 )
    {
        int nLine = m_nBytesAdded / 16;

        CString strTmp;
        strTmp.Format( _T("%p"), (PBYTE)m_pvStartAddress + nLine * ITEMS_PER_LINE );
        InsertItem( nLine, strTmp );
        SetItemText( nLine, 1, _T("   ") );
        SetItemText( nLine, 2, _T(" ") );
    }
    else
    {
        int nLine = m_nBytesAdded / 16;
        CString strTmp = GetItemText( nLine, 1 );
        strTmp += _T("   ");
        SetItemText( nLine, 1, strTmp );

        strTmp = GetItemText( nLine, 2 );
        strTmp += _T(" ");
        SetItemText( nLine, 2, strTmp );
    }

    int src;
    int dst;

    for ( src = m_nBytesAdded -1, dst = m_nBytesAdded; src >= nStart; src--, dst-- )
    {
        int nSrcLine = src / 16;
        int nSrcChar = src % 16;

        int nDstLine = dst / 16;
        int nDstChar = dst % 16;

        CString strSrc = GetItemText( nSrcLine, 1 ).Mid( nSrcChar * 3, 3 );

        CString strDst = GetItemText( nDstLine, 1 );
        strDst.Delete(nDstChar*3,3);
        strDst.Insert(nDstChar*3,strSrc);

        SetItemText( nDstLine, 1, strDst );

        strSrc = GetItemText( nSrcLine, 2 ).Mid( nSrcChar );

        strDst = GetItemText( nDstLine, 2 );
        strDst.Delete(nDstChar,1);
        strDst.Insert(nDstChar,strSrc);

        SetItemText( nDstLine, 2, strDst );
    }

    int nLine = nStart / 16;
    int nChar = nStart % 16;

    CString str = GetItemText( nLine, 1 );
    str.Delete(nChar*3,3);
    str.Insert(nChar*3,_T("FF "));
    SetItemText( nLine, 1, str );

    str = GetItemText( nLine, 2 );
    str.Delete(nChar,1);
    str.Insert(nChar,_T("*"));
    SetItemText( nLine, 2, str );

    m_nBytesAdded++;
}

void CDumpControl::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;
    LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)pNMHDR;

    if ( m_bMouseCaptured ) {
        m_bMouseCaptured = false;
        ReleaseCapture();
    }
}

void CDumpControl::OnLButtonDown( UINT nFlags, CPoint point )
{
    int nHit = GetItemIndex(point,false);
    if ( nHit >= 0 )
    {
        m_nHitCaptured = nHit;
        m_nMarkStart = nHit;
        m_nMarkEnd = nHit;
        Invalidate(0);

        m_nLastSearchPosition = nHit;

        SetCapture();
        m_bMouseCaptured = true;
    }
    CCustomDrawListCtrl::OnLButtonDown( nFlags, point );
}

void CDumpControl::OnLButtonUp( UINT nFlags, CPoint point )
{
    if ( m_bMouseCaptured )
    {
        m_bMouseCaptured = false;
        ReleaseCapture();
    }
    CCustomDrawListCtrl::OnLButtonUp( nFlags, point );
}

void CDumpControl::OnMouseMove( UINT nFlags, CPoint point )
{
    if ( m_bMouseCaptured )
    {
        int nHit = GetItemIndex(point,false);
        if ( nHit >= 0 ) {
            if ( nHit >= m_nHitCaptured ) {
                m_nMarkStart = m_nHitCaptured;
                m_nMarkEnd = nHit;
            }
            else {
                m_nMarkStart = nHit;
                m_nMarkEnd = m_nHitCaptured;
            }
            Invalidate(0);
        }
    }

    CCustomDrawListCtrl::OnMouseMove( nFlags, point );
}

void CDumpControl::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
    bool bRefresh = false;
    switch ( nChar ) {
    case VK_LEFT:
        if ( m_nMarkStart == -1 )
        {
            if ( m_nBytesAdded > 0 )
            {
                m_nMarkStart = 0;
                m_nMarkEnd = 0;
                bRefresh = true;
            }
        }
        else
        {
            if ( m_nMarkStart > 0 )
            {
                m_nMarkStart--;
                m_nMarkEnd = m_nMarkStart;
                bRefresh = true;
            }
        }
        break;
    case VK_RIGHT:
        if ( m_nMarkStart == -1 )
        {
            if ( m_nBytesAdded > 0 )
            {
                m_nMarkStart = 0;
                m_nMarkEnd = 0;
                bRefresh = true;
            }
        }
        else
        {
            if ( m_nMarkStart < m_nBytesAdded - 1)
            {
                m_nMarkStart++;
                m_nMarkEnd = m_nMarkStart;
                bRefresh = true;
            }
        }
        break;
    case VK_UP:
        if ( m_nMarkStart == -1 )
        {
            if ( m_nBytesAdded > 0 )
            {
                m_nMarkStart = 0;
                m_nMarkEnd = 0;
                bRefresh = true;
            }
        }
        else
        {
            if ( m_nMarkStart > 16 )
            {
                m_nMarkStart -= 16;
                m_nMarkEnd = m_nMarkStart;
                bRefresh = true;
            }
        }
        break;
    case VK_DOWN:
        if ( m_nMarkStart == -1 )
        {
            if ( m_nBytesAdded > 0 )
            {
                m_nMarkStart = 0;
                m_nMarkEnd = 0;
                bRefresh = true;
            }
        }
        else
        {
            if ( m_nMarkStart + 16 < m_nBytesAdded )
            {
                m_nMarkStart += 16;
                m_nMarkEnd = m_nMarkStart;
                bRefresh = true;
            }
        }
        break;
    case VK_DELETE:
        if ( m_nMarkStart != -1 && m_bEditable )
        {
            DeleteData( m_nMarkStart, m_nMarkEnd - m_nMarkStart + 1 );
            m_nMarkStart = -1;
            m_nMarkEnd = -1;
            bRefresh = true;
        }
        break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
    case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F':
        if ( m_nMarkStart != -1 && m_bEditable )
        {
            InsertData( m_nMarkStart, 0 );
            m_nMarkEnd = m_nMarkStart;
            bRefresh = true;
        }
        break;
    }

    if ( bRefresh )
        Invalidate(0);
    else
        CCustomDrawListCtrl::OnKeyDown( nChar, nRepCnt, nFlags );
}

void CDumpControl::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
}