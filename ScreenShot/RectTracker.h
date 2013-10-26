﻿/////////////////////////////////////////////////////////////////////////////

#ifndef RECT_TRACKER_H
#define RECT_TRACKER_H

#define CX_BORDER   1
#define CY_BORDER   1

//#define CRIT_RECTTRACKER    5

class RectTracker
{
public:
// Constructors
	RectTracker();
	RectTracker(LPCRECT lpSrcRect, UINT nStyle);

// Style Flags
	enum StyleFlags
	{
		solidLine = 1, dottedLine = 2, hatchedBorder = 4,
		resizeInside = 8, resizeOutside = 16, hatchInside = 32,
		resizeMiddle = 80
	};

// Hit-Test codes
	enum TrackerHit
	{
		hitNothing = -1,
		hitTopLeft = 0, hitTopRight = 1, hitBottomRight = 2, hitBottomLeft = 3,
		hitTop = 4, hitRight = 5, hitBottom = 6, hitLeft = 7, hitMiddle = 8
	};

// Attributes
	UINT  m_nStyle;			// current state
	RECT  m_rect;			// current position (always in pixels)
	SIZE  m_sizeMin;		// minimum X and Y size during track operation
	int	  m_nHandleSize;	// size of resize handles (default from WIN.INI)

// Operations
	void Draw(HDC hDC)	const;
	void GetTrueRect(LPRECT lpTrueRect) const;
	BOOL SetCursor(HWND hWnd, UINT nHitTest)	const;
	BOOL Track(HWND hWnd, POINT point, BOOL bAllowInvert = FALSE, 
		HWND hWndClipTo = NULL);
	BOOL TrackRubberBand(HWND hWnd, POINT point, BOOL bAllowInvert = TRUE);
	int HitTest(POINT point)	const;
	int NormalizeHit(int nHandle) const;
//////////////////////////////////////////////////////////////////////////////////////////
	void setRect(RECT rect)	{	CopyRect(&m_rect, &rect);	}
	void setRect(int left, int top, int right, int bottom)	{	::SetRect(&m_rect, left, top, right, bottom);	}
	void NormalizeRect(LPRECT lpRect) const;

	void DrawDragRect(HDC hDC, LPCRECT lpRect, SIZE size, LPCRECT lpRectLast,
		SIZE sizeLast, HBRUSH hBrush = NULL, HBRUSH hBrushLast = NULL);
//////////////////////////////////////////////////////////////////////////////////////////
// Overridables
	virtual void DrawTrackerRect(LPCRECT lpRect, HWND hWndClipTo,
		HDC hDC, HWND hWnd);
	virtual void AdjustRect(int nHandle, LPRECT lpRect);
	virtual void OnChangedRect(const RECT& rectOld);
	virtual UINT GetHandleMask() const;

// Implementation
public:
	virtual ~RectTracker();

protected:
	BOOL m_bAllowInvert;    // flag passed to Track or TrackRubberBand
	RECT m_rectLast;
	SIZE m_sizeLast;
	BOOL m_bErase;          // TRUE if DrawTrackerRect is called for erasing
	BOOL m_bFinalErase;     // TRUE if DragTrackerRect called for final erase

	// implementation helpers
	int HitTestHandles(POINT point) const;
	void GetHandleRect(int nHandle, RECT* pHandleRect) const;
	void GetModifyPointers(int nHandle, int**ppx, int**ppy, int* px, int*py);
	virtual int GetHandleSize(LPCRECT lpRect = NULL) const;
	BOOL TrackHandle(int nHandle, HWND hWnd, POINT point, HWND hWndClipTo);
	void Construct();
};
#endif