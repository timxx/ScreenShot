
#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <gdiplus.h>

#include "RectTracker.h"

using namespace Gdiplus;
//============================================================
const int _xScr = GetSystemMetrics(SM_CXSCREEN);
const int _yScr = GetSystemMetrics(SM_CYSCREEN);
//============================================================
const TCHAR szClsName[] = TEXT("ScrShot");
const TCHAR szTitle[]	= TEXT("Screen Shot");

HINSTANCE g_hInst;

HWND _hwndInfo = NULL;

HBITMAP _srcBmp = NULL;	//ȫ����ͼ

RectTracker _rectTracker;
// RECT	_infoRect;

bool	_bDrawing;				//�Ƿ����ڽ�ͼ
bool	_bDrew;					//�Ƿ��Ѿ���ͼ
POINT	_startPt;				//�������Ͻ�

//////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InfoWndProc(HWND, UINT, WPARAM, LPARAM);

bool Register(HINSTANCE hInst);
bool Create();

void ReportErr(DWORD dwCode);	//��Ϣ����ʾ��������Ϣ

HBITMAP CaptureSrc();	//����ȫ��
HBITMAP CaptureRect(HWND hWnd, HBITMAP hBitmap, RECT rect);//��hBitmap�Ͻ�ȡrect����

bool doSaveFile(HWND hWnd);//�����ļ�
bool getSavePath(HWND hWnd, TCHAR *filePath);

enum imgType { bmp, jpg, gif, tiff, png};
bool saveImageToFile(wchar_t *filePath, HBITMAP hbmp, imgType type );
int	 GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

bool saveToClipbrd(HWND hWnd, HBITMAP hBitmap);	//��hBitmap���Ƶ����а�

void doLButtonDown(HWND hWnd, POINT point);
void doLButtonDblClk(HWND hWnd, POINT point);
void doMouseMove(HWND hWnd, POINT point);
BOOL doEraseBkgnd(HDC hDC);
void doKeyDown(HWND hWnd, UINT nKey);
void doCommand(HWND hWnd, UINT uID);

void DrawRectInfo(const RECT &inRect, HDC hDC);	//�϶�����ʱ���������Ϣ

void showPopupMenu(HWND hWnd);					//�����˵�