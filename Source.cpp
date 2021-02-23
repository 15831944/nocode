#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <windowsx.h>
#include <list>
#include "resource.h"

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

#define ID_ALLSELECT 1000
#define ID_ALLUNSELECT 1001
#define ID_DELETE 1002

WCHAR szClassName[] = L"NoCode Editor";

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
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

struct point {
	double x;
	double y;
};

struct size {
	double w;
	double h;
};

struct trans {
	trans() : c{ 0,0 }, z(1), l{ 0,0 } {}
	// クライアントのサイズ
	size c;
	// ズーム
	double z;
	//　論理座標の注視点。これが画面上の中央に表示
	point l;
	void l2d(const point* p1, point* p2) const {
		p2->x = c.w / 2 + (p1->x - l.x) * z;
		p2->y = c.h / 2 + (p1->y - l.y) * z;
	}
	void d2l(const point* p1, point* p2) const {
		p2->x = l.x + (p1->x - c.w / 2) / z;
		p2->y = l.y + (p1->y - c.h / 2) / z;
	}
};

struct node {
	WCHAR name[16];
	int k;
	point p;
	size s;
	node* next;
	node* prev;
	bool select;
	bool del;
	node() : k(0), next(0), prev(0), p{ 0.0, 0.0 }, s{ 0.0, 0.0 }, name{}, select(false), del(false) {}
	~node() {}
	void paint(HDC hdc, const trans* t) const {
		if (del) return;
		point p1 = { p.x - s.w / 2, p.y - s.h / 2 };
		point p2 = { p.x + s.w / 2, p.y + s.h / 2 };
		point p3, p4;
		t->l2d(&p1, &p3);
		t->l2d(&p2, &p4);
		HPEN hPen = CreatePen(PS_SOLID, 1, select ? RGB(255, 0, 0) : RGB(0, 0, 0));
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
		Rectangle(hdc, (int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y);
		SelectObject(hdc, hOldPen);
		RECT rect{ (int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y };
		COLORREF oldColor = SetTextColor(hdc, select ? RGB(255, 0, 0) : RGB(0, 0, 0));
		DrawText(hdc, name, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		SetTextColor(hdc, oldColor);
	}
	bool hit(const point* p) const {
		return
			!del &&
			p->x >= this->p.x - s.w / 2 &&
			p->x <= this->p.x + s.w / 2 &&
			p->y >= this->p.y - s.h / 2 &&
			p->y <= this->p.y + s.h / 2;
	}
	bool inrect(const point* p1, const point* p2) const {
		return
			!del &&
			p.x - s.w / 2 >= p1->x &&
			p.y - s.h / 2 >= p1->y &&
			p.x + s.w / 2 <= p2->x &&
			p.y + s.h / 2 <= p2->y;
	}
};

struct nodelist {
	std::list<node*> l;
	void add(node* n) {
		l.push_back(n);
	}
	void del(node* n) {
		for (auto i = l.begin(); i != l.end();) {
			if (*i == n) {
				delete (*i);
				l.erase(i);
				break;
			}
		}
	}
	void clear() {
		for (auto i : l) {
			delete i;
		}
		l.clear();
	}
	void paint(HDC hdc, trans* t) {
		for (auto i : l) {
			i->paint(hdc, t);
		}
	}
	node* hit(point* p) {
		for (auto i : l) {
			if (i->hit(p))
				return i;
		}
		return 0;
	}
	void select(node* n) { // 指定した一つのみ選択状態にする
		for (auto i : l) {
			i->select = (i == n);
		}
	}
	void unselect() {
		for (auto i : l) {
			i->select = false;
		}
	}
	void rectselect(const point* p1, const point* p2) {
		for (auto i : l) {
			i->select = i->inrect(p1, p2);
		}
	}
	void allselect() {
		for (auto i : l) {
			i->select = true;
		}
	}
	void del() {
		for (auto i : l) {
			if (i->select) {
				i->del = true;
			}
		}
	}
};

enum Mode {
	none = 0,
	dragclient = 1,
	dragnode = 2,
	rectselect = 3,
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	static trans t;
	static nodelist l;
	static int mode;
	static POINT dragstartP;
	static point dragstartL;
	static node* node1;
	switch (msg)
	{
	case WM_CREATE:
		mode = 0;
		{
			// test add node
			node* n1 = new node();
			n1->p.x = 0;
			n1->p.y = 0;
			n1->s.w = 100;
			n1->s.h = 50;
			lstrcpyW(n1->name, L"ノード1");
			l.add(n1);
			node* n2 = new node();
			n2->p.x = 0;
			n2->p.y = 100;
			n2->s.w = 100;
			n2->s.h = 50;
			lstrcpyW(n2->name, L"ノード2");
			l.add(n2);
		}
		SendMessage(hWnd, WM_DPICHANGED, 0, 0);
		break;
	case WM_LBUTTONDOWN:
		{
			point p1{ (double)GET_X_LPARAM(lParam), (double)GET_Y_LPARAM(lParam) };
			point p2;
			t.d2l(&p1, &p2);
			node* n = l.hit(&p2);
			if (n) {
				l.select(n);
				InvalidateRect(hWnd, NULL, TRUE);
				dragstartP = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				dragstartL = n->p;
				mode = dragnode;
				node1 = n;
				SetCapture(hWnd);
			}
			else
			{
				//矩形選択モードに遷移
				l.unselect();
				InvalidateRect(hWnd, NULL, TRUE);
				dragstartP = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				mode = rectselect;
				SetCapture(hWnd);
			}
		}
		break;
	case WM_LBUTTONUP:
		mode = none;
		ReleaseCapture();
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_MBUTTONDOWN:
		{
			dragstartP = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			dragstartL = t.l;
			mode = dragclient;
			SetCapture(hWnd);
		}
		break;
	case WM_MBUTTONUP:
		mode = none;
		ReleaseCapture();
		break;
	case WM_MOUSEMOVE:
		if (mode == dragclient) {
			point p1{ (double)GET_X_LPARAM(lParam), (double)GET_Y_LPARAM(lParam) };
			t.l.x = dragstartL.x - (p1.x - dragstartP.x) / t.z;
			t.l.y = dragstartL.y - (p1.y - dragstartP.y) / t.z;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		else if (mode == dragnode) {
			point p1{ (double)GET_X_LPARAM(lParam), (double)GET_Y_LPARAM(lParam) };
			node1->p.x = dragstartL.x + (p1.x - dragstartP.x) / t.z;
			node1->p.y = dragstartL.y + (p1.y - dragstartP.y) / t.z;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		else if (mode == rectselect) {
			point p1{ (double)min(dragstartP.x,GET_X_LPARAM(lParam)), (double)min(dragstartP.y,GET_Y_LPARAM(lParam)) };
			point p2{ (double)max(dragstartP.x,GET_X_LPARAM(lParam)), (double)max(dragstartP.y,GET_Y_LPARAM(lParam)) };
			point p3, p4;
			t.d2l(&p1, &p3);
			t.d2l(&p2, &p4);
			l.rectselect(&p3, &p4);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
			{
				HDC hdc = GetDC(hWnd);
				HPEN hPen = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
				HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
				int nOldRop = SetROP2(hdc, R2_NOTXORPEN);
				Rectangle(hdc, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y);
				SetROP2(hdc, nOldRop);
				SelectObject(hdc, hOldPen);
				ReleaseDC(hWnd, hdc);
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta < 0) {
				t.z *= 1.0 / 1.20;
				if (t.z < 0.5) {
					t.z = 0.5;
					return 0;
				}
			}
			else {
				t.z *= 1.20;
				if (t.z > 10) {
					t.z = 10;
					return 0;
				}
			}
			DeleteObject(hFont);
			hFont = CreateFontW(-POINT2PIXEL((int)(10 * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"MS Shell Dlg");
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
			l.paint(hdc, &t);
			SelectObject(hdc, hOldFont);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		t.c.w = LOWORD(lParam);
		t.c.h = HIWORD(lParam);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_ALLSELECT:
			l.allselect();
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case ID_ALLUNSELECT:
			l.unselect();
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case ID_DELETE:
			l.del();
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandleW(L"user32.dll");
			if (hModUser32)
			{
				typedef BOOL(WINAPI* fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DPICHANGED:
		GetScaling(hWnd, &uDpiX, &uDpiY);
		DeleteObject(hFont);
		hFont = CreateFontW(-POINT2PIXEL((int)(10 * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"MS Shell Dlg");
		break;
	case WM_DESTROY:
		l.clear();
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindowW(
		szClassName,
		L"NoCode Editor",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = {
		{FVIRTKEY | FCONTROL, 0x41, ID_ALLSELECT},
		{FVIRTKEY, VK_ESCAPE, ID_ALLUNSELECT},
		{FVIRTKEY, VK_DELETE, ID_DELETE},
	};
	HACCEL hAccel = CreateAcceleratorTable(Accel, _countof(Accel));
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
