// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation 

#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <cassert>

#include "RectTracker.h"

#define offsetof(s,m)   (size_t)&(((s *)0)->m)

//#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// RectTracker global state

// various GDI objects we need to draw
extern __declspec(selectany)HCURSOR _afxCursors[10] = { 0, };
extern __declspec(selectany)HBRUSH _afxHatchBrush = 0;
extern __declspec(selectany)HPEN _afxBlackDottedPen = 0;
extern __declspec(selectany)int _afxHandleSize = 0;

void __cdecl AfxTrackerTerm()
{
	//AfxDeleteObject((HGDIOBJ*)&_afxHatchBrush);
	//AfxDeleteObject((HGDIOBJ*)&_afxBlackDottedPen);
	DeleteObject(_afxHatchBrush);
	DeleteObject(_afxBlackDottedPen);
}
char _afxTrackerTerm = 0;

// the struct below is used to determine the qualities of a particular handle
struct AFX_HANDLEINFO
{
	size_t nOffsetX;    // offset within RECT for X coordinate
	size_t nOffsetY;    // offset within RECT for Y coordinate
	int nCenterX;       // adjust X by Width()/2 * this number
	int nCenterY;       // adjust Y by Height()/2 * this number
	int nHandleX;       // adjust X by handle size * this number
	int nHandleY;       // adjust Y by handle size * this number
	int nInvertX;       // handle converts to this when X inverted
	int nInvertY;       // handle converts to this when Y inverted
};

// this array describes all 8 handles (clock-wise)
extern __declspec(selectany) const AFX_HANDLEINFO _afxHandleInfo[] =
{
	// corner handles (top-left, top-right, bottom-right, bottom-left
	{ offsetof(RECT, left), offsetof(RECT, top),        0, 0,  0,  0, 1, 3 },
	{ offsetof(RECT, right), offsetof(RECT, top),       0, 0, -1,  0, 0, 2 },
	{ offsetof(RECT, right), offsetof(RECT, bottom),    0, 0, -1, -1, 3, 1 },
	{ offsetof(RECT, left), offsetof(RECT, bottom),     0, 0,  0, -1, 2, 0 },

	// side handles (top, right, bottom, left)
	{ offsetof(RECT, left), offsetof(RECT, top),        1, 0,  0,  0, 4, 6 },
	{ offsetof(RECT, right), offsetof(RECT, top),       0, 1, -1,  0, 7, 5 },
	{ offsetof(RECT, left), offsetof(RECT, bottom),     1, 0,  0, -1, 6, 4 },
	{ offsetof(RECT, left), offsetof(RECT, top),        0, 1,  0,  0, 5, 7 }
};

// the struct below gives us information on the layout of a RECT struct and
//  the relationship between its members
struct AFX_RECTINFO
{
	size_t nOffsetAcross;   // offset of opposite point (ie. left->right)
	int nSignAcross;        // sign relative to that point (ie. add/subtract)
};

// this array is indexed by the offset of the RECT member / sizeof(int)
extern __declspec(selectany)const AFX_RECTINFO _afxRectInfo[] =
{
	{ offsetof(RECT, right), +1 },
	{ offsetof(RECT, bottom), +1 },
	{ offsetof(RECT, left), -1 },
	{ offsetof(RECT, top), -1 },
};

/////////////////////////////////////////////////////////////////////////////
// RectTracker intitialization

RectTracker::RectTracker()
{
	Construct();
}

RectTracker::RectTracker(LPCRECT lpSrcRect, UINT nStyle)
{
	//ASSERT(AfxIsValidAddress(lpSrcRect, sizeof(RECT), FALSE));

	Construct();
	CopyRect(&m_rect, lpSrcRect);
	m_nStyle = nStyle;
}

void RectTracker::Construct()
{
	// do one-time initialization if necessary
//	AfxLockGlobals(CRIT_RECTTRACKER);
	static BOOL bInitialized;
	if (!bInitialized)
	{


		// sanity checks for assumptions we make in the code
		assert(sizeof(((RECT*)NULL)->left) == sizeof(int));
		assert(offsetof(RECT, top) > offsetof(RECT, left));
		assert(offsetof(RECT, right) > offsetof(RECT, top));
		assert(offsetof(RECT, bottom) > offsetof(RECT, right));

		if (_afxHatchBrush == NULL)
		{
			// create the hatch pattern + bitmap
			WORD hatchPattern[8];
			WORD wPattern = 0x1111;
			for (int i = 0; i < 4; i++)
			{
				hatchPattern[i] = wPattern;
				hatchPattern[i+4] = wPattern;
				wPattern <<= 1;
			}
			HBITMAP hatchBitmap = CreateBitmap(8, 8, 1, 1, hatchPattern);
/*			if (hatchBitmap == NULL)
			{
				AfxUnlockGlobals(CRIT_RECTTRACKER);
				AfxThrowResourceException();
			}
*/
			// create black hatched brush
			_afxHatchBrush = CreatePatternBrush(hatchBitmap);
			DeleteObject(hatchBitmap);
/*			if (_afxHatchBrush == NULL)
			{
				AfxUnlockGlobals(CRIT_RECTTRACKER);
				AfxThrowResourceException();
			}*/
		}

		if (_afxBlackDottedPen == NULL)
		{
			// create black dotted pen
			_afxBlackDottedPen = CreatePen(PS_DOT, 0, RGB(0, 0, 0));
/*			if (_afxBlackDottedPen == NULL)
			{
				AfxUnlockGlobals(CRIT_RECTTRACKER);
				AfxThrowResourceException();
			}*/
		}
		_afxCursors[0] = ::LoadCursor(NULL, IDC_SIZENWSE);
		_afxCursors[1] = ::LoadCursor(NULL, IDC_SIZENESW);
		_afxCursors[2] = _afxCursors[0];
		_afxCursors[3] = _afxCursors[1];
		_afxCursors[4] = ::LoadCursor(NULL, IDC_SIZENS);
		_afxCursors[5] = ::LoadCursor(NULL, IDC_SIZEWE);
		_afxCursors[6] = _afxCursors[4];
		_afxCursors[7] = _afxCursors[5];
		_afxCursors[8] = ::LoadCursor(NULL, IDC_SIZEALL);
		_afxCursors[9] = ::LoadCursor(NULL, IDC_SIZEALL);//

		// get default handle size from Windows profile setting
		static const TCHAR szWindows[] = _T("windows");
		static const TCHAR szInplaceBorderWidth[] =
			_T("oleinplaceborderwidth");
		_afxHandleSize = GetProfileInt(szWindows, szInplaceBorderWidth, 4);
		bInitialized = TRUE;
	}
	if (!_afxTrackerTerm)
		_afxTrackerTerm = (char)!atexit(&AfxTrackerTerm);
//	AfxUnlockGlobals(CRIT_RECTTRACKER);

	m_nStyle = 0;
	m_nHandleSize = _afxHandleSize;
	m_sizeMin.cy = m_sizeMin.cx = m_nHandleSize*2;

	SetRectEmpty(&m_rectLast);
	m_sizeLast.cx = m_sizeLast.cy = 0;
	m_bErase = FALSE;
	m_bFinalErase =  FALSE;
}

RectTracker::~RectTracker()
{
}

/////////////////////////////////////////////////////////////////////////////
// RectTracker operations

void RectTracker::Draw(HDC hDC)	const
{
	// set initial DC state
	assert(hDC !=0 );
	int nOldDC = SaveDC(hDC);
	SetMapMode(hDC, MM_TEXT);
	SetViewportOrgEx(hDC, 0, 0, NULL);
	SetWindowOrgEx(hDC, 0, 0, NULL);

	// get normalized rectangle
	RECT rect = m_rect;
	NormalizeRect(&rect);

	HPEN hOldPen = NULL;
	HBRUSH hOldBrush = NULL;
	HGDIOBJ hTemp;

	int nOldROP;

	// draw lines
	if ((m_nStyle & (dottedLine|solidLine)) != 0)
	{
		if (m_nStyle & dottedLine)
			hOldPen = (HPEN) SelectObject(hDC, _afxBlackDottedPen);
		else
			hOldPen = (HPEN) SelectObject(hDC, GetStockObject(BLACK_PEN));
		hOldBrush = (HBRUSH) SelectObject(hDC, GetStockObject(NULL_BRUSH));
		nOldROP = SetROP2(hDC, R2_COPYPEN);
		InflateRect(&rect, +1, +1);
		Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
		SetROP2(hDC, nOldROP);
	}

	// if hatchBrush is going to be used, need to unrealize it
	if ((m_nStyle & (hatchInside|hatchedBorder)) != 0)
		UnrealizeObject(_afxHatchBrush);

	// hatch inside
	if ((m_nStyle & hatchInside) != 0)
	{
		hTemp = SelectObject(hDC, GetStockObject(NULL_PEN));
		if (hOldPen == NULL)
			hOldPen = (HPEN)hTemp;
		hTemp = SelectObject(hDC, _afxHatchBrush);
		if (hOldBrush == NULL )
			hOldBrush = (HBRUSH) hTemp;
		SetBkMode(hDC, TRANSPARENT);
		nOldROP = SetROP2(hDC, R2_MASKNOTPEN);
		Rectangle(hDC, rect.left+1, rect.top+1, rect.right, rect.bottom);
		SetROP2(hDC, nOldROP);
	}

	// draw hatched border
	if ((m_nStyle & hatchedBorder) != 0)
	{
		hTemp = SelectObject(hDC, _afxHatchBrush);
		if (hOldBrush == NULL)
			hOldBrush = (HBRUSH)hTemp;
		SetBkMode(hDC, OPAQUE);
		RECT rectTrue;
		GetTrueRect(&rectTrue);
		PatBlt(hDC, rectTrue.left, rectTrue.top, rectTrue.right - rectTrue.left,
			rect.top-rectTrue.top, 0x000F0001 /* Pn */);
		PatBlt(hDC, rectTrue.left, rect.bottom,
			rectTrue.right - rectTrue.left, rectTrue.bottom-rect.bottom, 0x000F0001 /* Pn */);
		PatBlt(hDC, rectTrue.left, rect.top, rect.left-rectTrue.left,
			rect.bottom - rect.top, 0x000F0001 /* Pn */);
		PatBlt(hDC, rect.right, rect.top, rectTrue.right-rect.right,
			rect.bottom - rect.top, 0x000F0001 /* Pn */);
	}

	// draw resize handles
	if ((m_nStyle & (resizeInside|resizeOutside)) != 0)
	{
		UINT mask = GetHandleMask();
		for (int i = 0; i < 8; ++i)
		{
			if (mask & (1<<i))
			{
				GetHandleRect((TrackerHit)i, &rect);
				SetBkColor(hDC, RGB(0, 0, 0));
				ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
			}
		}
	}

	// cleanup pDC state
	if (hOldPen != NULL)
		SelectObject(hDC, hOldPen);
	if (hOldBrush != NULL)
		SelectObject(hDC, hOldBrush);
	RestoreDC(hDC, nOldDC);
	//assert(RestoreDC(hDC, nOldDC));
}

BOOL RectTracker::SetCursor(HWND hWnd, UINT nHitTest) const
{
	// trackers should only be in client area
	if (nHitTest != HTCLIENT)
		return FALSE;

	// convert cursor position to client co-ordinates
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(hWnd, &point);

	// do hittest and normalize hit
	int nHandle = HitTestHandles(point);
	if (nHandle < 0)
		return FALSE;

	// need to normalize the hittest such that we get proper cursors
	nHandle = NormalizeHit(nHandle);

	// handle special case of hitting area between handles
	//  (logically the same -- handled as a move -- but different cursor)
	if (nHandle == hitMiddle && ! PtInRect(&m_rect, point))
	{
		// only for trackers with hatchedBorder (ie. in-place resizing)
		if (m_nStyle & hatchedBorder)
			nHandle = (TrackerHit)9;
	}

	assert(nHandle < _countof(_afxCursors));
	::SetCursor(_afxCursors[nHandle]);
	return TRUE;
}

int RectTracker::HitTest(POINT point) const
{
	TrackerHit hitResult = hitNothing;

	RECT rectTrue;
	GetTrueRect(&rectTrue);
	assert(rectTrue.left <= rectTrue.right);
	assert(rectTrue.top <= rectTrue.bottom);
	if (PtInRect(&rectTrue, point))
	{
		if ((m_nStyle & (resizeInside|resizeOutside)) != 0)
			hitResult = (TrackerHit)HitTestHandles(point);
		else
			hitResult = hitMiddle;
	}
	return hitResult;
}

int RectTracker::NormalizeHit(int nHandle) const
{
	assert(nHandle <= 8 && nHandle >= -1);
	if (nHandle == hitMiddle || nHandle == hitNothing)
		return nHandle;
	assert(0 <= nHandle && nHandle < _countof(_afxHandleInfo));
	const AFX_HANDLEINFO* pHandleInfo = &_afxHandleInfo[nHandle];
	if ((m_rect.right - m_rect.left) < 0)
	{
		nHandle = (TrackerHit)pHandleInfo->nInvertX;
		assert(0 <= nHandle && nHandle < _countof(_afxHandleInfo));
		pHandleInfo = &_afxHandleInfo[nHandle];
	}
	if ((m_rect.bottom - m_rect.top) < 0)
		nHandle = (TrackerHit)pHandleInfo->nInvertY;
	return nHandle;
}

BOOL RectTracker::Track(HWND hWnd, POINT point, BOOL bAllowInvert,
	HWND hWndClipTo)

{
	// perform hit testing on the handles
	int nHandle = HitTestHandles(point);
	if (nHandle < 0)
	{
		// didn't hit a handle, so just return FALSE
		return FALSE;
	}

	// otherwise, call helper function to do the tracking
	m_bAllowInvert = bAllowInvert;
	return TrackHandle(nHandle, hWnd, point, hWndClipTo);
}

BOOL RectTracker::TrackRubberBand(HWND hWnd, POINT point, BOOL bAllowInvert)
{
	// simply call helper function to track from bottom right handle
	m_bAllowInvert = bAllowInvert;
	SetRect(&m_rect, point.x, point.y, point.x, point.y);
	return TrackHandle(hitBottomRight, hWnd, point, NULL);
}

void RectTracker::DrawTrackerRect(
	LPCRECT lpRect, HWND hWndClipTo, HDC hDC, HWND hWnd)
{
	// first, normalize the rectangle for drawing
	RECT rect = *lpRect;
	NormalizeRect(&rect);

	// convert to client coordinates
	if (hWndClipTo != NULL)
	{
		ClientToScreen(hWnd, (LPPOINT)&rect);
		ClientToScreen(hWnd, ((LPPOINT)&rect)+1);

		ScreenToClient(hWndClipTo, (LPPOINT)&rect);
		ScreenToClient(hWndClipTo, ((LPPOINT)&rect)+1);
	}

	SIZE size = {0};
	if (!m_bFinalErase)
	{
		// otherwise, size depends on the style
		if (m_nStyle & hatchedBorder)
		{
			size.cx = size.cy = max(1, GetHandleSize(&rect)-1);
			InflateRect(&rect, size.cx, size.cy);
		}
		else
		{
			size.cx = CX_BORDER;
			size.cy = CY_BORDER;
		}
	}

	// and draw it
	if (m_bFinalErase || !m_bErase)
		DrawDragRect(hDC, &rect, size, &m_rectLast, m_sizeLast);

	// remember last rectangles
	m_rectLast = rect;
	m_sizeLast = size;
}

void RectTracker::AdjustRect(int nHandle, LPRECT)
{
	if (nHandle == hitMiddle)
		return;

	// convert the handle into locations within m_rect
	int *px, *py;
	GetModifyPointers(nHandle, &px, &py, NULL, NULL);

	// enforce minimum width
	int nNewWidth = m_rect.right - m_rect.left;
	int nAbsWidth = m_bAllowInvert ? abs(nNewWidth) : nNewWidth;
	if (px != NULL && nAbsWidth < m_sizeMin.cx)
	{
		nNewWidth = nAbsWidth != 0 ? nNewWidth / nAbsWidth : 1;
		ptrdiff_t iRectInfo = (int*)px - (int*)&m_rect;
		assert(0 <= iRectInfo && iRectInfo < _countof(_afxRectInfo));
		const AFX_RECTINFO* pRectInfo = &_afxRectInfo[iRectInfo];
		*px = *(int*)((BYTE*)&m_rect + pRectInfo->nOffsetAcross) +
			nNewWidth * m_sizeMin.cx * -pRectInfo->nSignAcross;
	}

	// enforce minimum height
	int nNewHeight = m_rect.bottom - m_rect.top;
	int nAbsHeight = m_bAllowInvert ? abs(nNewHeight) : nNewHeight;
	if (py != NULL && nAbsHeight < m_sizeMin.cy)
	{
		nNewHeight = nAbsHeight != 0 ? nNewHeight / nAbsHeight : 1;
		ptrdiff_t iRectInfo = (int*)py - (int*)&m_rect;
		assert(0 <= iRectInfo && iRectInfo < _countof(_afxRectInfo));
		const AFX_RECTINFO* pRectInfo = &_afxRectInfo[iRectInfo];
		*py = *(int*)((BYTE*)&m_rect + pRectInfo->nOffsetAcross) +
			nNewHeight * m_sizeMin.cy * -pRectInfo->nSignAcross;
	}
}

void RectTracker::GetTrueRect(LPRECT lpTrueRect) const
{
	//ASSERT(AfxIsValidAddress(lpTrueRect, sizeof(RECT)));

	RECT rect = m_rect;
	NormalizeRect(&rect);
	int nInflateBy = 0;
	if ((m_nStyle & (resizeOutside|hatchedBorder)) != 0)
		nInflateBy += GetHandleSize() - 1;
	if ((m_nStyle & (solidLine|dottedLine)) != 0)
		++nInflateBy;
	InflateRect(&rect, nInflateBy, nInflateBy);
	*lpTrueRect = rect;
}

void RectTracker::OnChangedRect(const RECT& /*rectOld*/)
{
	// no default implementation, useful for derived classes
}

/////////////////////////////////////////////////////////////////////////////
// RectTracker implementation helpers

void RectTracker::GetHandleRect(int nHandle, RECT* pHandleRect) const
{
	assert(nHandle < 8);

	// get normalized rectangle of the tracker
	RECT rectT = m_rect;
	NormalizeRect(&rectT);
	if ((m_nStyle & (solidLine|dottedLine)) != 0)
		InflateRect(&rectT, +1, +1);

	// since the rectangle itself was normalized, we also have to invert the
	//  resize handles.
	nHandle = NormalizeHit(nHandle);

	// handle case of resize handles outside the tracker
	int size = GetHandleSize();
	if (m_nStyle & resizeOutside)/////////////////
	{
		if (m_nStyle & 1000000)	//middle
			InflateRect(&rectT, size-size/2-1, size-size/2-1);
		else					//outside
			InflateRect(&rectT, size-1, size-1);
	}
	// calculate position of the resize handle
	int nWidth = rectT.right - rectT.left;
	int nHeight = rectT.bottom - rectT.top;
	RECT rect;
	const AFX_HANDLEINFO* pHandleInfo = &_afxHandleInfo[nHandle];
	rect.left = *(int*)((BYTE*)&rectT + pHandleInfo->nOffsetX);
	rect.top = *(int*)((BYTE*)&rectT + pHandleInfo->nOffsetY);
	rect.left += size * pHandleInfo->nHandleX;
	rect.top += size * pHandleInfo->nHandleY;
	rect.left += pHandleInfo->nCenterX * (nWidth - size) / 2;
	rect.top += pHandleInfo->nCenterY * (nHeight - size) / 2;
	rect.right = rect.left + size;
	rect.bottom = rect.top + size;

	*pHandleRect = rect;
}

int RectTracker::GetHandleSize(LPCRECT lpRect) const
{
	if (lpRect == NULL)
		lpRect = &m_rect;

	int size = m_nHandleSize;
	if (!(m_nStyle & resizeOutside))
	{
		// make sure size is small enough for the size of the rect
		int sizeMax = min(abs(lpRect->right - lpRect->left),
			abs(lpRect->bottom - lpRect->top));
		if (size * 2 > sizeMax)
			size = sizeMax / 2;
	}
	return size;
}

int RectTracker::HitTestHandles(POINT point) const
{
	RECT rect;
	UINT mask = GetHandleMask();

	// see if hit anywhere inside the tracker
	GetTrueRect(&rect);
	if (!PtInRect(&rect, point))
		return hitNothing;  // totally missed

	// see if we hit a handle
	for (int i = 0; i < 8; ++i)
	{
		if (mask & (1<<i))
		{
			GetHandleRect((TrackerHit)i, &rect);
			if (PtInRect(&rect, point))
				return (TrackerHit)i;
		}
	}

	// last of all, check for non-hit outside of object, between resize handles
	if ((m_nStyle & hatchedBorder) == 0)
	{
		RECT rect = m_rect;
		NormalizeRect(&rect);
		if ((m_nStyle & dottedLine|solidLine) != 0)
			InflateRect(&rect, +1, +1);
		if (!PtInRect(&rect, point))
			return hitNothing;  // must have been between resize handles
	}
	return hitMiddle;   // no handle hit, but hit object (or object border)
}

BOOL RectTracker::TrackHandle(int nHandle, HWND hWnd, POINT point,
	HWND hWndClipTo)
{
	assert(nHandle >= 0);
	assert(nHandle <= 8);   // handle 8 is inside the rect

	// don't handle if capture already set
	if (::GetCapture() != NULL)
		return FALSE;

	//AfxLockTempMaps();  // protect maps while looping

	assert(!m_bFinalErase);

	// save original width & height in pixels
	int nWidth = m_rect.right - m_rect.left;
	int nHeight = m_rect.bottom - m_rect.top;

	// set capture to the window which received this message
	SetCapture(hWnd);
	assert(hWnd == GetCapture());
	UpdateWindow(hWnd);
	if (hWndClipTo != NULL)
		UpdateWindow(hWndClipTo);
	RECT rectSave = m_rect;

	// find out what x/y coords we are supposed to modify
	int *px, *py;
	int xDiff, yDiff;
	GetModifyPointers(nHandle, &px, &py, &xDiff, &yDiff);
	xDiff = point.x - xDiff;
	yDiff = point.y - yDiff;

	// get DC for drawing
	HDC hDrawDC;
	if (hWndClipTo != NULL)
	{
		// clip to arbitrary window by using adjusted Window DC
		hDrawDC = GetDCEx(hWndClipTo, NULL, DCX_CACHE);
	}
	else
	{
		// otherwise, just use normal DC
		hDrawDC = GetDC(hWnd);
	}
	assert(hDrawDC);

	RECT rectOld;
	BOOL bMoved = FALSE;

	// get messages until capture lost or cancelled/accepted
	for (;;)
	{
		MSG msg;
		//assert(::GetMessage(&msg, NULL, 0, 0));
		::GetMessage(&msg, NULL, 0, 0);

		if (GetCapture() != hWnd)
			break;

		switch (msg.message)
		{
		// handle movement/accept messages
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			rectOld = m_rect;
			// handle resize cases (and part of move)
			if (px != NULL)
				*px = GET_X_LPARAM(msg.lParam) - xDiff;
			if (py != NULL)
				*py = GET_Y_LPARAM(msg.lParam) - yDiff;

			// handle move case
			if (nHandle == hitMiddle)
			{
				m_rect.right = m_rect.left + nWidth;
				m_rect.bottom = m_rect.top + nHeight;
			}
			// allow caller to adjust the rectangle if necessary
			AdjustRect(nHandle, &m_rect);

			// only redraw and callback if the rect actually changed!
			m_bFinalErase = (msg.message == WM_LBUTTONUP);
			if (!EqualRect(&rectOld, &m_rect) || m_bFinalErase)
			{
				if (bMoved)
				{
					m_bErase = TRUE;
					DrawTrackerRect(&rectOld, hWndClipTo, hDrawDC, hWnd);
				}
				OnChangedRect(rectOld);
				if (msg.message != WM_LBUTTONUP)
					bMoved = TRUE;
			}
			if (m_bFinalErase)
				goto ExitLoop;

			if (!EqualRect(&rectOld, &m_rect))
			{
				m_bErase = FALSE;
				DrawTrackerRect(&m_rect, hWndClipTo, hDrawDC, hWnd);
			}
			break;

		// handle cancel messages
		case WM_KEYDOWN:
			if (msg.wParam != VK_ESCAPE)
				break;
		case WM_RBUTTONDOWN:
			if (bMoved)
			{
				m_bErase = m_bFinalErase = TRUE;
				DrawTrackerRect(&m_rect, hWndClipTo, hDrawDC, hWnd);
			}
			m_rect = rectSave;
			goto ExitLoop;

		// just dispatch rest of the messages
		default:
			DispatchMessage(&msg);
			break;
		}
	}

ExitLoop:
	if (hWndClipTo != NULL)
		ReleaseDC(hWndClipTo, hDrawDC);
	else
		ReleaseDC(hWnd, hDrawDC);
	ReleaseCapture();

	//AfxUnlockTempMaps(FALSE);

	// restore rect in case bMoved is still FALSE
	if (!bMoved)
		m_rect = rectSave;
	m_bFinalErase = FALSE;
	m_bErase = FALSE;

	// return TRUE only if rect has changed
	return !EqualRect(&rectSave, &m_rect);
}

void RectTracker::GetModifyPointers(
	int nHandle, int** ppx, int** ppy, int* px, int* py)
{
	assert(nHandle >= 0);
	assert(nHandle <= 8);

	if (nHandle == hitMiddle)
		nHandle = hitTopLeft;   // same as hitting top-left

	*ppx = NULL;
	*ppy = NULL;

	// fill in the part of the rect that this handle modifies
	//  (Note: handles that map to themselves along a given axis when that
	//   axis is inverted don't modify the value on that axis)

	const AFX_HANDLEINFO* pHandleInfo = &_afxHandleInfo[nHandle];
	if (pHandleInfo->nInvertX != nHandle)
	{
		*ppx = (int*)((BYTE*)&m_rect + pHandleInfo->nOffsetX);
		if (px != NULL)
			*px = **ppx;
	}
	else
	{
		// middle handle on X axis
		if (px != NULL)
			*px = m_rect.left + abs(m_rect.right - m_rect.left) / 2;
	}
	if (pHandleInfo->nInvertY != nHandle)
	{
		*ppy = (int*)((BYTE*)&m_rect + pHandleInfo->nOffsetY);
		if (py != NULL)
			*py = **ppy;
	}
	else
	{
		// middle handle on Y axis
		if (py != NULL)
			*py = m_rect.top + abs(m_rect.bottom - m_rect.top) / 2;
	}
}

UINT RectTracker::GetHandleMask() const
{
	UINT mask = 0x0F;   // always have 4 corner handles
	int size = m_nHandleSize*3;
	if (abs(m_rect.right - m_rect.left) - size > 4)
		mask |= 0x50;
	if (abs(m_rect.bottom - m_rect.top) - size > 4)
		mask |= 0xA0;
	return mask;
}

void RectTracker::NormalizeRect(LPRECT lpRect) const
{
	int nTemp;
	if (lpRect->left > lpRect->right)
	{
		nTemp = lpRect->left;
		lpRect->left = lpRect->right;
		lpRect->right = nTemp;
	}
	if (lpRect->top > lpRect->bottom)
	{
		nTemp = lpRect->top;
		lpRect->top = lpRect->bottom;
		lpRect->bottom = nTemp;
	}
}

void RectTracker::DrawDragRect(HDC hDC, LPCRECT lpRect, SIZE size, LPCRECT lpRectLast,
	SIZE sizeLast, HBRUSH hBrush, HBRUSH hBrushLast)
{
	//ASSERT(AfxIsValidAddress(lpRect, sizeof(RECT), FALSE));
	//ASSERT(lpRectLast == NULL ||
	//	AfxIsValidAddress(lpRectLast, sizeof(RECT), FALSE));

	assert(lpRectLast != NULL);

	// first, determine the update region and select it

	HRGN rgnNew;
	HRGN rgnOutside, rgnInside;
	rgnOutside = CreateRectRgnIndirect(lpRect);
	RECT rect = *lpRect;
	InflateRect(&rect, -size.cx, -size.cy);
	IntersectRect(&rect, &rect, lpRect);
	rgnInside = CreateRectRgnIndirect(&rect);
	rgnNew = CreateRectRgn(0, 0, 0, 0);
	CombineRgn(rgnNew, rgnOutside, rgnInside, RGN_XOR);

	HBRUSH hBrushOld = NULL;
	if (hBrush == NULL)
	{
		WORD grayPattern[8];
		for (int i = 0; i < 8; i++)
			grayPattern[i] = (WORD)(0x5555 << (i & 1));
		HBITMAP grayBitmap = CreateBitmap(8, 8, 1, 1, grayPattern);
		if (grayBitmap != NULL)
		{
			hBrush = CreatePatternBrush(grayBitmap);
			DeleteObject(grayBitmap);
		}
	}

	assert(hBrush);
	if (hBrushLast == NULL)
	{
		hBrushLast = hBrush;
	}

	HRGN rgnLast, rgnUpdate;
	if (lpRectLast != NULL)
	{
		// find difference between new region and old region
		rgnLast = CreateRectRgn(0, 0, 0, 0);
		SetRectRgn(rgnOutside, lpRectLast->left, lpRectLast->top, lpRectLast->right, lpRectLast->bottom);
		rect = *lpRectLast;
		InflateRect(&rect, -sizeLast.cx, -sizeLast.cy);
		IntersectRect(&rect, &rect, lpRectLast);
		SetRectRgn(rgnInside, rect.left, rect.top, rect.right, rect.bottom);
		CombineRgn(rgnLast, rgnOutside, rgnInside, RGN_XOR);

		// only diff them if brushes are the same
		if (hBrush == hBrushLast)
		{
			rgnUpdate = CreateRectRgn(0, 0, 0, 0);
			CombineRgn(rgnUpdate, rgnLast, rgnNew, RGN_XOR);
		}
	}
	if (hBrush != hBrushLast && lpRectLast != NULL)
	{
		// brushes are different -- erase old region first
		SelectClipRgn(hDC, rgnLast);
		GetClipBox(hDC, &rect);
		hBrushOld = (HBRUSH) SelectObject(hDC, hBrushLast);
		PatBlt(hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PATINVERT);
		SelectObject(hDC, hBrushOld);
		hBrushOld = NULL;
	}

	// draw into the update/new region
	SelectClipRgn(hDC, rgnUpdate != NULL ? rgnUpdate : rgnNew);
	GetClipBox(hDC, &rect);
	hBrushOld = (HBRUSH) SelectObject(hDC, hBrush);
	PatBlt(hDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PATINVERT);

	// cleanup DC
	if (hBrushOld != NULL)
		SelectObject(hDC, hBrushOld);
	SelectClipRgn(hDC, NULL);
}
/////////////////////////////////////////////////////////////////////////////