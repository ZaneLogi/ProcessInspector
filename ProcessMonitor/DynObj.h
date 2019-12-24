#ifndef __DYNOBJ_H__
#define __DYNOBJ_H__

/*
Usage:
1.
class someClass : public CDialog
{
	DECLARE_OBJ_MAP();
	afx_msg void OnMove( int, int );					// ON_WM_MOVE()
	afx_msg void OnSize( UINT, int, int );				// ON_WM_SIZE()
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);	// ON_WM_GETMINMAXINFO()
}

2.
BEGIN_MESSAGE_MAP(someClass, CDialog)
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

3.
BOOL someClass::OnInitDialog()
{
	BEGIN_OBJ_MAP(someClass);
	OBJ_DEFINE(IDC_CB_DEVICES,   0, 100,   0,   0 );
	OBJ_DEFINE(...);
	...
	END_OBJ_MAP();
}

4.
void someClass::OnMove( int x, int y )
{
	SAVE_WINDOW_PLACEMENT();
}

void someClass::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy); // need to change to CDialogEx if the base is CDialogEx
	UPDATE_OBJ_POSITION(cx,cy);
	SAVE_WINDOW_PLACEMENT();
}

void someClass::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	UPDATE_MINMAX_INFO(lpMMI);
	__super::OnGetMinMaxInfo(lpMMI); // need to change to CDialogEx if the base is CDialogEx
}
*/


#define DECLARE_OBJ_MAP() \
    CArray<OBJINFO> _apObjInfo_; \
    CString m_strRegistryEntryName; \
	CRect m_rcInitialWindow; \
	CRect m_rcInitialClient;

#define BEGIN_OBJ_MAP(name) OBJINFO _tmpObjInfo_; m_strRegistryEntryName = _T(#name)

// x: id of a dialog item
// a: ratio of the left   border ( 0 - 100 )
// b: ratio of the right  border ( 0 - 100 )
// c: ratio of the top    border ( 0 - 100 )
// d: ratio of the bottom border ( 0 - 100 )
#define OBJ_DEFINE(x,a,b,c,d) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = a; \
	_tmpObjInfo_.x2frac = b; \
	_tmpObjInfo_.y1frac = c; \
	_tmpObjInfo_.y2frac = d; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_SCALEABLE(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_TOP(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 0; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_BOTTOM(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 100; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_LEFT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 0; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_RIGHT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 100; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_TOP_LEFT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 0; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 0; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_TOP_RIGHT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 100; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 0; \
	_tmpObjInfo_.y2frac = 0; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_BOTTOM_LEFT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 0; \
	_tmpObjInfo_.x2frac = 0; \
	_tmpObjInfo_.y1frac = 100; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define OBJ_DEFINE_BOTTOM_RIGHT(x) \
    _tmpObjInfo_.id = x; \
	_tmpObjInfo_.x1frac = 100; \
	_tmpObjInfo_.x2frac = 100; \
	_tmpObjInfo_.y1frac = 100; \
	_tmpObjInfo_.y2frac = 100; \
	GetDlgItem(x)->GetWindowRect(&_tmpObjInfo_.rc); \
	ScreenToClient(&_tmpObjInfo_.rc); \
    _apObjInfo_.Add(_tmpObjInfo_)

#define END_OBJ_MAP() \
    CRect __rcParent(0,0,0,0); \
    if (GetParent()) { GetParent()->GetWindowRect(&__rcParent); } \
	GetClientRect(&m_rcInitialClient); \
	GetWindowRect(&m_rcInitialWindow); \
	UINT __x = AfxGetApp()->GetProfileInt(m_strRegistryEntryName,_T("WindowX"),0); \
	UINT __y = AfxGetApp()->GetProfileInt(m_strRegistryEntryName,_T("WindowY"),0); \
	UINT __w = AfxGetApp()->GetProfileInt(m_strRegistryEntryName,_T("WindowWidth"),0); \
	UINT __h = AfxGetApp()->GetProfileInt(m_strRegistryEntryName,_T("WindowHeight"),0); \
    if (__x == 0 && __y == 0 && __w == 0 && __h == 0) {} else { \
        MoveWindow( __x + __rcParent.left, __y + __rcParent.top, __w, __h, FALSE ); }

// cx: new width
// cy: new height
#define UPDATE_OBJ_POSITION(cx,cy) \
    int __count = (int)_apObjInfo_.GetCount(); \
	if (__count > 0) { \
		int __nDeltaX = cx - ( m_rcInitialClient.right - m_rcInitialClient.left ); \
		int __nDeltaY = cy - ( m_rcInitialClient.bottom - m_rcInitialClient.top ); \
		for ( int __i = 0; __i < __count; __i++ ) { \
			OBJINFO* p = &_apObjInfo_[__i]; \
			RECT rcNew; \
			rcNew.left   = p->rc.left   + ( p->x1frac * __nDeltaX ) / 100; \
			rcNew.right  = p->rc.right  + ( p->x2frac * __nDeltaX ) / 100; \
			rcNew.top    = p->rc.top    + ( p->y1frac * __nDeltaY ) / 100; \
			rcNew.bottom = p->rc.bottom + ( p->y2frac * __nDeltaY ) / 100; \
			GetDlgItem(p->id)->MoveWindow( &rcNew, FALSE ); \
		} \
		Invalidate(); \
	}

#define UPDATE_MINMAX_INFO(lpMMI) \
	if (_apObjInfo_.GetCount() > 0) { \
		lpMMI->ptMinTrackSize.x = m_rcInitialWindow.right - m_rcInitialWindow.left; \
		lpMMI->ptMinTrackSize.y = m_rcInitialWindow.bottom - m_rcInitialWindow.top; \
	}

#define SAVE_WINDOW_PLACEMENT() \
	if (_apObjInfo_.GetCount() > 0) { \
        CRect __rcParent(0,0,0,0); \
        if (GetParent()) { GetParent()->GetWindowRect(&__rcParent); } \
		CRect __rc; \
		GetWindowRect(&__rc); \
        __rc.left -= __rcParent.left; __rc.top -= __rcParent.top; \
        __rc.right -= __rcParent.left; __rc.bottom -= __rcParent.top; \
		AfxGetApp()->WriteProfileInt(m_strRegistryEntryName,_T("WindowX"),__rc.left); \
		AfxGetApp()->WriteProfileInt(m_strRegistryEntryName,_T("WindowY"),__rc.top); \
		AfxGetApp()->WriteProfileInt(m_strRegistryEntryName,_T("WindowWidth"),__rc.Width()); \
		AfxGetApp()->WriteProfileInt(m_strRegistryEntryName,_T("WindowHeight"),__rc.Height()); \
	}


typedef struct OBJINFO
{
	int		id;
	RECT	rc;
	BYTE	x1frac;
	BYTE	x2frac;
	BYTE	y1frac;
	BYTE	y2frac;
} OBJINFO;



#endif