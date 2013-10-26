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

			HFONT hFont = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("����"));
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
					lstrcpy(szTip, TEXT("\r\n����������������ѡ���ȡ��Χ\r\n\r\n")
					TEXT("������Ҽ������˵�\r\n\r\n����ESC���˳�"));
				break;

			case 2:
				wsprintf(szTip, TEXT("\r\n���ɿ�������ȷ����ȡ��Χ\r\n\r\n")
					TEXT("����ESC��ȡ����ȡ\r\n\r\n����ǰ�����С��%d * %d"), nWidth, nHeigth);
				break;

			case 3:
				if ( nWidth != 0 && nHeigth != 0 )
					wsprintf(szTip, TEXT("���������΢��λ��\r\n\r\n������϶�������С��λ��\r\n\r\n")
					TEXT("����ESC�����½�ȡ����\r\n\r\n����ǰ�����С��%d * %d"), nWidth, nHeigth);
				else
					lstrcpy(szTip, TEXT("\r\n���������΢��λ��\r\n\r\n������϶�������С��λ��\r\n\r\n����ESC�����½�ȡ����"));
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