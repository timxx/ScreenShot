
//========================================================================================================
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "\
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
//========================================================================================================

#include <shlwapi.h>

#include "WinMain.h"
#include "myMessage.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
	if (!Register(hInst))
	{
		ReportErr(GetLastError());
		return 0;
	}
	if (!Create())
	{
		ReportErr(GetLastError());
		return 0;
	}

	MSG msg;

	while(GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return int(msg.wParam);
}

bool Register(HINSTANCE hInst)
{
	WNDCLASSEX wcex = {0};

	g_hInst = hInst;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR1));
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAIN_APP));
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInst;
	wcex.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszClassName = szClsName;

	if (!RegisterClassEx(&wcex))	return false;

	wcex.lpszClassName = TEXT("InfoWnd");
	wcex.hIcon = 0;
	wcex.lpfnWndProc = InfoWndProc;

	return RegisterClassEx(&wcex) ? true : false;
}

bool Create()
{
	HWND hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW/* | WS_EX_LAYERED*/,
		szClsName, szTitle, WS_POPUP,\
		0, 0, _xScr, _yScr, NULL, NULL, g_hInst, NULL);

	if (!hWnd)	return false;

	//先将整个屏幕截取下来
	_srcBmp = CaptureSrc();
	
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	_hwndInfo = CreateWindowExW(WS_EX_TOPMOST, TEXT("InfoWnd"), NULL, WS_POPUP,\
		0, 0, 174, 146, hWnd, NULL, g_hInst, NULL);

	if (!_hwndInfo)	return false;

//	SetLayeredWindowAttributes(hWnd, 0, 255 * (100 - 70) / 100, LWA_ALPHA);

	ShowWindow(_hwndInfo, SW_SHOW);
	UpdateWindow(_hwndInfo);

	//初始化提示内容
	SendMessage(_hwndInfo, MM_TIPINFO, 1, 0);

	return true;
}

void ReportErr(DWORD dwCode)
{
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf, 0, NULL );

	MessageBox(GetActiveWindow(), (TCHAR*)lpMsgBuf, TEXT("ErrInfo"), MB_ICONERROR);

	LocalFree(lpMsgBuf);
}
//////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch(uMsg)
	{
	case WM_CREATE:

		_bDrawing = false;
		_bDrew = false;
		_startPt.x = 0;
		_startPt.y = 0;

		_rectTracker.m_nStyle = RectTracker::solidLine | RectTracker::resizeMiddle;
		_rectTracker.setRect(-1, -1, -1, -1);

		break;

	case WM_COMMAND:
		doCommand(hWnd, LOWORD(wParam));
		break;

	case WM_LBUTTONDOWN:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			doLButtonDown(hWnd, pt);

			SendMessage(_hwndInfo, MM_TIPINFO, 2, (LPARAM)&_rectTracker.m_rect);
		}
		break;

	case WM_LBUTTONUP:
//		SetRectEmpty(&_infoRect);
		SendMessage(_hwndInfo, MM_TIPINFO, 3, (LPARAM)&_rectTracker.m_rect);
		_bDrawing = false;
		InvalidateRect(hWnd, NULL, TRUE);

		break;

	case WM_LBUTTONDBLCLK:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			doLButtonDblClk(hWnd, pt);
		}
		break;

	case WM_RBUTTONUP:
		showPopupMenu(hWnd);
		break;

	case WM_MOUSEMOVE:
		{
			POINT point = { LOWORD(lParam), HIWORD(lParam) };
			doMouseMove(hWnd, point);
		}
		break;

	case WM_KEYDOWN:
		doKeyDown(hWnd, (UINT)wParam);
		break;

	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			if (_bDrawing)
				DrawRectInfo(_rectTracker.m_rect, hdc);

			if(_bDrew)
				_rectTracker.Draw(hdc);

// 			RECT rect;
// 			GetWindowRect(hWnd, &rect);
// 			HRGN hrgnAll, hrgnCap;
// 			hrgnAll = CreateRectRgnIndirect(&rect);
// 			hrgnCap = CreateRectRgnIndirect(&_rectTracker.m_rect);
// 			CombineRgn(hrgnAll, hrgnAll, hrgnCap, RGN_XOR);
// 			GetWindowRect(_hwndInfo, &rect);
// 			hrgnCap = CreateRectRgnIndirect(&_infoRect);
// 			CombineRgn(hrgnAll, hrgnAll, hrgnCap, RGN_XOR);
// 
// 			FillRgn(hdc, hrgnAll, CreateSolidBrush(RGB(10, 10, 10)));
// 
// 			DeleteObject(hrgnAll);
// 			DeleteObject(hrgnCap);

			EndPaint(hWnd, &ps);
		}

		break;

	case WM_SETCURSOR:
		{
			if (!_bDrawing && _bDrew &&
				_rectTracker.SetCursor(hWnd, LOWORD(lParam))
				)
			{
				return TRUE; 
			}

			SetCursor(LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_CURSOR1)));
		}
		return TRUE;

	case WM_ERASEBKGND:
		return doEraseBkgnd((HDC)wParam);

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (_srcBmp)	DeleteObject(_srcBmp);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

HBITMAP CaptureSrc()
{
	HDC hdcSrc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL); 
	HDC hdcMem = CreateCompatibleDC(hdcSrc); 

	HBITMAP hbmp = CreateCompatibleBitmap(hdcSrc, _xScr, _yScr);

	HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcMem, hbmp);

	if (!hbmpOld) 
	{
		ReportErr(GetLastError());
		return NULL;
	}

	if (!BitBlt(hdcMem, 0, 0, _xScr, _yScr, hdcSrc, 0, 0, SRCCOPY))
	{
		ReportErr(GetLastError());
		return NULL;
	}

	SelectObject(hdcMem, hbmpOld);
	DeleteDC(hdcSrc);
	DeleteDC(hdcMem);

	return hbmp;
}

HBITMAP CaptureRect(HWND hWnd, HBITMAP hBitmap, RECT rect)
{
	if (IsRectEmpty(&rect))	return NULL;

	HDC hdc = GetDC(hWnd);
	SelectObject(hdc, hBitmap);

	HDC hdcMem = CreateCompatibleDC(hdc); 

	int nWidth = rect.right - rect.left;
	int nHeight = rect.bottom - rect.top;

	HBITMAP hbmp = CreateCompatibleBitmap(hdc, nWidth, nHeight);

	HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcMem, hbmp);

	if (!hbmpOld) 
	{
		ReportErr(GetLastError());
		return NULL;
	}

	if (!BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdc, rect.left, rect.top, SRCCOPY))
	{
		ReportErr(GetLastError());
		return NULL;
	}

	SelectObject(hdcMem, hbmpOld);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);

	return hbmp;
}
bool saveImageToFile(wchar_t *filePath, HBITMAP hbmp, imgType type )
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Status stat;
	CLSID  clsid;

	Bitmap* bitmap = new Bitmap(hbmp, NULL);

	switch(type)
	{
	case bmp:	GetEncoderClsid(L"image/bmp", &clsid);	break;
	case jpg:	GetEncoderClsid(L"image/jpeg", &clsid);	break;
	case gif:	GetEncoderClsid(L"image/gif", &clsid);	break;
	case tiff:	GetEncoderClsid(L"image/tiff", &clsid);	break;
	case png:	GetEncoderClsid(L"image/png", &clsid);	break;
	}

	EncoderParameters encoderParameters;
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;

	ULONG quality = 0;
	encoderParameters.Parameter[0].Value = &quality;

	stat = bitmap->Save(filePath, &clsid, &encoderParameters);

	delete bitmap;

	GdiplusShutdown(gdiplusToken);

	return stat==Ok ? true : false;
}
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

bool getSavePath(HWND hWnd, TCHAR *filePath)
{
	TCHAR *szExt[] = {_T(".bmp"), _T(".jpg"), _T(".gif"), _T(".png"), _T(".tiff")};
	TCHAR szFile[MAX_PATH] = {_T("输入文件名")};
	OPENFILENAME ofn = {0};

	ofn.Flags = OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;
	ofn.hInstance = g_hInst;
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrTitle  = _T("保存截图");
	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = _T("Bitmap(*.bmp)\0*.bmp\0JPEG(*.jpg)\0*.jpg\0")
		_T("GIF(*.gif)\0*.gif\0PNG(*.png)\0*.png\0TIFF(*.tiff)\0*.tiff\0\0");
	ofn.nFilterIndex = 2;

	if (GetSaveFileName(&ofn))
	{
		TCHAR *ext = PathFindExtension(szFile);

		if (lstrcmpi(ext, szExt[0])!=0 &&
			lstrcmpi(ext, szExt[1])!=0 &&
			lstrcmpi(ext, szExt[2])!=0 &&
			lstrcmpi(ext, szExt[3])!=0 &&
			lstrcmpi(ext, szExt[4])!=0 )
		{
			lstrcat(szFile, szExt[ofn.nFilterIndex - 1]);
		}

		lstrcpy(filePath, szFile);
		return true;
	}

	return false;
}

void doLButtonDown(HWND hWnd, POINT point)
{
	//点击在矩形外部
	if(_rectTracker.HitTest(point) == RectTracker::hitNothing)
	{
		if(!_bDrew)
		{
			//第一次画矩形
			_startPt	= point;
			_bDrawing	= true;
			_bDrew = true;

			_rectTracker.setRect(point.x, point.y, point.x + 4, point.y + 4);	

			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
	else	//内部则拖动它
	{
		if(_bDrew)
		{
			_rectTracker.Track(hWnd, point, TRUE);
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
}

void doLButtonDblClk(HWND hWnd, POINT point)
{
	//
	if (_rectTracker.HitTest(point) != RectTracker::hitMiddle)
		return ;

	doSaveFile(hWnd);
	SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void doMouseMove(HWND hWnd, POINT point)
{
	RECT rect;
	GetWindowRect(_hwndInfo, &rect);

	if (PtInRect(&rect, point))
	{
		SendMessage(_hwndInfo, WM_MOUSEMOVE, 0, 0);
	}

	if(_bDrawing)
	{
		SendMessage(_hwndInfo, MM_TIPINFO, 2, (LPARAM)&_rectTracker.m_rect);
		_rectTracker.setRect( _startPt.x, _startPt.y, point.x, point.y);
		InvalidateRect(hWnd, NULL, TRUE);
	}
}

BOOL doEraseBkgnd(HDC hDC)
{
	HDC hdcMem;

	if(_srcBmp)
	{
		hdcMem = CreateCompatibleDC(hDC);
		HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcMem, _srcBmp);
		BITMAP bmp;
		GetObject(_srcBmp, sizeof(BITMAP), &bmp);

		BitBlt(hDC, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);

		SelectObject(hdcMem, hbmpOld);
		DeleteDC(hdcMem);
	}

	return TRUE;
}

void doKeyDown(HWND hWnd, UINT nKey)
{
	RECT rect = _rectTracker.m_rect;
	//按下Shift键时只移动一边
	bool isShifted = (GetKeyState(VK_SHIFT) & 0x8000);
	switch(nKey)
	{
	case VK_UP:
		if (rect.top <= 0)	//不允许“越界”
			break;
		if (!isShifted)
			rect.top -= 1;
		rect.bottom -= 1;
		break;

	case VK_DOWN:
		if (rect.bottom >= _yScr)
			break;
		if (!isShifted)
			rect.bottom +=1;
		rect.top += 1;
		break;

	case VK_LEFT:
		if (rect.left <= 0)	
			break;
		if (!isShifted)
			rect.left -= 1;
		rect.right -=1;
		break;

	case VK_RIGHT:
		if (rect.right >= _xScr)	
			break;
		if (!isShifted)
			rect.right += 1;
		rect.left += 1;
		break;

	case VK_ESCAPE://Esc键
		if(_bDrew)	//已经截取过区域，则重选
		{
			_bDrew = false;
			_rectTracker.setRect(-1, -1, -1, -1);
			InvalidateRect(hWnd, NULL, TRUE);

			SendMessage(_hwndInfo, MM_TIPINFO, 1, 0);
		}
		else	//否则退出
			SendMessage(hWnd, WM_CLOSE, 0, 0);

		return ;
	}
	_rectTracker.setRect(rect);
	InvalidateRect(hWnd, NULL, TRUE);
}

void DrawRectInfo(const RECT &inRect, HDC hDC)
{    
	HPEN hPen = CreatePen(PS_SOLID, 1, RGB(147, 147, 147));
	HGDIOBJ hOldPen = SelectObject(hDC, hPen);

	HFONT hFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("宋体"));
	HFONT hOldFont = (HFONT) SelectObject(hDC, hFont);

	int nOldBkMode = SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, RGB(0, 0, 200));

	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);
	//字高
	int charHeight = tm.tmHeight + tm.tmExternalLeading;

	SIZE size;
	GetTextExtentPoint32(hDC, _T("XXXXXXXX：(XXXX, XXXX)"), lstrlen(_T("XXXXXXXX：(XXXX, XXXX)")), &size);
	int lineLength = size.cx;	//一行字的最大长度
    
	POINT pt;
	GetCursorPos(&pt);

	const int space = 3;
	const int rowSpace = 5;//两行字之间距离
	RECT rect = {pt.x + space, pt.y - (charHeight + rowSpace)*3 - rowSpace, \
		pt.x + lineLength + space ,pt.y - space};
    
    RECT rectTemp;
	//矩形右边到达屏幕右边时
	if((pt.x + rect.right - rect.left)>= _xScr)
	{
		rectTemp = rect;
		rectTemp.left = rect.left - (rect.right - rect.left) - space*2;
		rectTemp.right = rect.right - (rect.right - rect.left) - space*2;;
		rect = rectTemp;
	}
	//矩形下方到达屏幕底边
	if((pt.y - (rect.bottom - rect.top)) <= 0)
	{
		rectTemp = rect;
		rectTemp.top = rect.top + rect.bottom - rect.top + space*2;;
		rectTemp.bottom = rect.bottom + rect.bottom - rect.top + space*2;;
		rect = rectTemp;	
	}

	HBRUSH hOldBrush;
    hOldBrush = (HBRUSH) SelectObject(hDC, GetStockObject(NULL_BRUSH));
	
	Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

//	CopyRect(&_infoRect, &rect);
	//
	const int left = rect.left + 5;
	const int right = left + lineLength;
	int top = rect.top + 5;
	int bottom = top + charHeight;

	RECT outRect = {left, top, right, bottom};

	TCHAR szInfo[256] = {0};
    
	wsprintf(szInfo, _T("顶点位置：(%d, %d)"), inRect.left, inRect.top);
	DrawText(hDC, szInfo, lstrlen(szInfo), &outRect, DT_LEFT);
	
	top += charHeight + rowSpace;
	bottom += top + charHeight;
	SetRect(&outRect, left, top, right, bottom);
	wsprintf(szInfo, _T("矩形大小：(%d, %d)"), abs(inRect.right - inRect.left), abs(inRect.bottom - inRect.top));
    DrawText(hDC, szInfo, lstrlen(szInfo), &outRect, DT_LEFT);

	top += charHeight + rowSpace;
	bottom += top + charHeight;
	SetRect(&outRect, left, top, right, bottom);
	wsprintf(szInfo, _T("光标坐标：(%d, %d)"), pt.x, pt.y);
	DrawText(hDC, szInfo, lstrlen(szInfo), &outRect, DT_LEFT);
    
	SetBkMode(hDC, nOldBkMode);

	DeleteObject(hFont);
	DeleteObject(hPen);

	SelectObject(hDC, hOldPen);
	SelectObject(hDC, hOldFont);
	SelectObject(hDC, hOldBrush);
}

void showPopupMenu(HWND hWnd)
{
	HMENU hMenu = NULL;
	hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
	
	if (!hMenu)	return ;

	HMENU hmenuSub = GetSubMenu(hMenu, 0);

	MENUITEMINFO mii = {0};

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_BITMAP;

	mii.hbmpItem = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CLOSE));;
	SetMenuItemInfo(hmenuSub, IDM_EXIT, MF_BYCOMMAND, &mii);

	mii.hbmpItem = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SAVE));
	SetMenuItemInfo(hmenuSub, IDM_SAVE, MF_BYCOMMAND, &mii);

	mii.hbmpItem = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_RECAP));
	SetMenuItemInfo(hmenuSub, IDM_RECAP, MF_BYCOMMAND, &mii);

	mii.hbmpItem = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CAPALL));
	SetMenuItemInfo(hmenuSub, IDM_CAP_SCR, MF_BYCOMMAND, &mii);

	POINT pt;
	GetCursorPos(&pt);

	if (!_bDrew)//没选过区域，禁用
	{
		EnableMenuItem(hmenuSub, IDM_SAVE, MF_BYCOMMAND | MF_DISABLED);
		EnableMenuItem(hmenuSub, IDM_CLIPBRD, MF_BYCOMMAND | MF_DISABLED);
		EnableMenuItem(hmenuSub, IDM_RECAP, MF_BYCOMMAND | MF_DISABLED);
	}

	TrackPopupMenu(hmenuSub, TPM_HORPOSANIMATION | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

	DestroyMenu(hMenu);
}

bool saveToClipbrd(HWND hWnd, HBITMAP hBitmap)
{
	if (!OpenClipboard(hWnd)) 
		return false;

	EmptyClipboard();

	SetClipboardData(CF_BITMAP, hBitmap);

	CloseClipboard();

	return true;
}

void doCommand(HWND hWnd, UINT uID)
{
	switch(uID)
	{
	case IDM_CAP_SCR:
		_rectTracker.setRect(0, 0, _xScr, _yScr);
		_bDrew = true;
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case IDM_SAVE:		doSaveFile(hWnd);	SendMessage(hWnd, WM_CLOSE, 0, 0);	break;
		
	case IDM_CLIPBRD:	saveToClipbrd(hWnd, CaptureRect(hWnd, _srcBmp, _rectTracker.m_rect));	break;

	case IDM_RECAP:		SendMessage(hWnd, WM_KEYDOWN, VK_ESCAPE, 0);	break;

	case IDM_EXIT:		SendMessage(hWnd, WM_CLOSE, 0, 0);	break;
	}
}

bool doSaveFile(HWND hWnd)
{
	TCHAR szFile[MAX_PATH] = {0};
	TCHAR *szExt[] = {_T(".bmp"), _T(".jpg"), _T(".gif"), _T(".png"), _T(".tiff")};

	if (getSavePath(hWnd, szFile))
	{
		//只处理指定的扩展名
		TCHAR *ext = PathFindExtension(szFile);
		imgType type = jpg;
		if (lstrcmpi(ext, szExt[0]) == 0)
			type = bmp;
		else if(lstrcmpi(ext, szExt[1]) == 0)
			type = jpg;
		else if(lstrcmpi(ext, szExt[2]) == 0)
			type = gif;
		else if(lstrcmpi(ext, szExt[3]) == 0)
			type = png;
		else if(lstrcmpi(ext, szExt[4]) == 0)
			type = tiff;
#ifdef UNICODE
		return saveImageToFile(szFile, CaptureRect(hWnd, _srcBmp, _rectTracker.m_rect), type);
#else
		wchar_t wszFile[MAX_PATH] = {0};
		int len = MultiByteToWideChar( CP_ACP, 0, szFile, -1, wszFile, 0 );
		MultiByteToWideChar( CP_ACP, 0, szFile, -1, wszFile, len );

		return saveImageToFile(wszFile, CaptureSrc(_rectTracker.m_rect), type);
#endif
	}
	return false;
}