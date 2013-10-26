//============================================================

#include <Windows.h>

#include "myMessage.h"
#include "resource.h"
//============================================================

extern HINSTANCE g_hInst;

extern LRESULT CALLBACK InfoWndProc(HWND, UINT, WPARAM, LPARAM);
//////////////////////////////////////////////////////////////
bool _fLeft = true;
TCHAR szTip[256] = {0};

//////////////////////////////////////////////////////////////
void ShowCaptureInfo();
//////////////////////////////////////////////////////////////
LRESULT CALLBACK InfoWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HDC hdcMem;

	HBITMAP hbmp;
	HBITMAP hbmpOld;

	switch(uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			rect.top += 40;

			SetBkMode(hdc, TRANSPARENT);

			HFONT hFont = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("宋体"));
			HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);

			DrawText(hdc, szTip, lstrlen(szTip), &rect, DT_LEFT);

			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		EndPaint(hWnd, &ps);
		break;

	case WM_MOUSEMOVE:
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);

			int xScreen = GetSystemMetrics(SM_CXSCREEN);

			if(_fLeft)
			{
				MoveWindow(hWnd, 10, 10, rect.right - rect.left, rect.bottom - rect.top, TRUE);
				
				_fLeft = false;
			}
			else
			{
				MoveWindow(hWnd, xScreen-180, 10, rect.right - rect.left, rect.bottom - rect.top, TRUE);
				_fLeft = true;
			}		
		}
		break;

	case WM_ERASEBKGND:

		hbmp = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_INFO_BK));
		if (!hbmp)	return 0;

		hdc = (HDC)wParam;
		hdcMem = CreateCompatibleDC(hdc);

		hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

		BitBlt(hdc, 0, 0, 174, 146, hdcMem, 0, 0, SRCCOPY);

		DeleteObject(hbmp);
		SelectObject(hdcMem, hbmpOld);
		DeleteDC(hdcMem);
		ReleaseDC(hWnd, hdc);

		return TRUE;

	case MM_TIPINFO:
		{
			LPRECT lpRect = (RECT*)lParam;

			int nWidth = 0 ;	
			int nHeigth = 0;

			if (lpRect)
			{
				nWidth  = abs(lpRect->right - lpRect->left);
				nHeigth = abs(lpRect->bottom - lpRect->top);
			}
			switch(wParam)
			{
			case 1:
					lstrcpy(szTip, TEXT("\r\n·按下鼠标左键不放选择截取范围\r\n\r\n")
					TEXT("·鼠标右键弹出菜单\r\n\r\n·按ESC键退出"));
				break;

			case 2:
				wsprintf(szTip, TEXT("\r\n·松开鼠标左键确定截取范围\r\n\r\n")
					TEXT("·按ESC键取消截取\r\n\r\n·当前区域大小：%d * %d"), nWidth, nHeigth);
				break;

			case 3:
				if ( nWidth != 0 && nHeigth != 0 )
					wsprintf(szTip, TEXT("·按方向键微调位置\r\n\r\n·鼠标拖动调整大小和位置\r\n\r\n")
					TEXT("·按ESC键重新截取区域\r\n\r\n·当前区域大小：%d * %d"), nWidth, nHeigth);
				else
					lstrcpy(szTip, TEXT("\r\n·按方向键微调位置\r\n\r\n·鼠标拖动调整大小和位置\r\n\r\n·按ESC键重新截取区域"));
				break;
			}

			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}