
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

HBITMAP _srcBmp = NULL;	//全屏截图

RectTracker _rectTracker;
// RECT	_infoRect;

bool	_bDrawing;				//是否正在截图
bool	_bDrew;					//是否已经截图
POINT	_startPt;				//矩形左上角

//////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InfoWndProc(HWND, UINT, WPARAM, LPARAM);

bool Register(HINSTANCE hInst);
bool Create();

void ReportErr(DWORD dwCode);	//消息框提示最后出错信息

HBITMAP CaptureSrc();	//捕获全屏
HBITMAP CaptureRect(HWND hWnd, HBITMAP hBitmap, RECT rect);//在hBitmap上截取rect部分

bool doSaveFile(HWND hWnd);//保存文件
bool getSavePath(HWND hWnd, TCHAR *filePath);

enum imgType { bmp, jpg, gif, tiff, png};
bool saveImageToFile(wchar_t *filePath, HBITMAP hbmp, imgType type );
int	 GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

bool saveToClipbrd(HWND hWnd, HBITMAP hBitmap);	//将hBitmap复制到剪切板

void doLButtonDown(HWND hWnd, POINT point);
void doLButtonDblClk(HWND hWnd, POINT point);
void doMouseMove(HWND hWnd, POINT point);
BOOL doEraseBkgnd(HDC hDC);
void doKeyDown(HWND hWnd, UINT nKey);
void doCommand(HWND hWnd, UINT uID);

void DrawRectInfo(const RECT &inRect, HDC hDC);	//拖动矩形时画出相关信息

void showPopupMenu(HWND hWnd);					//弹出菜单