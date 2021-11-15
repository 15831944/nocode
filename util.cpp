#include <windows.h>
#include "util.h"
#include "common.h"

void util::DrawArrow(HDC hdc, const POINT* p1, const POINT* p2, const double* z)
{
	MoveToEx(hdc, p1->x, p1->y, 0);
	LineTo(hdc, p2->x, p2->y);
	POINT p[] = { { p2->x, p2->y }, { (int)(p2->x - 5 * *z), (int)(p2->y - 10 * *z)}, { (int)(p2->x + 5 * *z), (int)(p2->y - 10 * *z) } };
	HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	Polygon(hdc, p, _countof(p));
	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
}

void util::ClipOrCenterRectToMonitor(LPRECT prc, UINT flags)
{
    HMONITOR hMonitor;
    MONITORINFO mi;
    RECT        rc;
    int         w = prc->right - prc->left;
    int         h = prc->bottom - prc->top;

    // 
    // get the nearest monitor to the passed rect. 
    // 
    hMonitor = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);

    // 
    // get the work area or entire monitor rect. 
    // 
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    if (flags & MONITOR_WORKAREA)
        rc = mi.rcWork;
    else
        rc = mi.rcMonitor;

    // 
    // center or clip the passed rect to the monitor rect 
    // 
    if (flags & MONITOR_CENTER)
    {
        prc->left = rc.left + (rc.right - rc.left - w) / 2;
        prc->top = rc.top + (rc.bottom - rc.top - h) / 2;
        prc->right = prc->left + w;
        prc->bottom = prc->top + h;
    }
    else
    {
        prc->left = max(rc.left, min(rc.right - w, prc->left));
        prc->top = max(rc.top, min(rc.bottom - h, prc->top));
        prc->right = prc->left + w;
        prc->bottom = prc->top + h;
    }
}

BOOL util::GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibraryW(L"SHCORE");
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

BOOL CALLBACK util::EnumChildSetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)g_c.hUIFont, TRUE);
	return TRUE;
}

