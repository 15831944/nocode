#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <list>
#include "resource.h"

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

#define ID_ALLSELECT 1000
#define ID_ALLUNSELECT 1001
#define ID_DELETE 1002
#define ID_HOMEPOSITION 1003
#define DRAGJUDGEWIDTH POINT2PIXEL(4)
#define FONT_NAME L"Yu Gothic UI"
#define FONT_SIZE 12

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

class trans {
public:
	trans() : c{ 0,0 }, z(1), l{ 0,0 } {}
	// クライアントのサイズ
	point p; // クライアント左上座標
	size c;
	// ズーム
	double z;
	//　論理座標の注視点。これが画面上の中央に表示
	point l;
	void l2d(const point* p1, point* p2) const {
		p2->x = p.x + c.w / 2 + (p1->x - l.x) * z;
		p2->y = p.y + c.h / 2 + (p1->y - l.y) * z;
	}
	void d2l(const point* p1, point* p2) const {
		p2->x = l.x + (p1->x - p.x - c.w / 2) / z;
		p2->y = l.y + (p1->y - p.y - c.h / 2) / z;
	}
	void settransfromrect(const point* p1, const point* p2, const int margin) {
		l.x = p1->x + (p2->x - p1->x) / 2;
		l.y = p1->y + (p2->y - p1->y) / 2;
		if (p2->x - p1->x == 0.0 || p2->y - p1->y == 0.0)
			z = 1.0;
		else
			z = min((c.w - margin * 2) / (p2->x - p1->x), (c.h - margin * 2) / (p2->y - p1->y));
	}
};

class node {
public:
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
	void paint(HDC hdc, const trans* t, const point* offset = nullptr) const {
		if (del) return;
		const point p1 = { p.x - s.w / 2 + (offset ? offset->x : 0.0), p.y - s.h / 2 + (offset ? offset->y : 0.0) };
		const point p2 = { p.x + s.w / 2 + (offset ? offset->x : 0.0), p.y + s.h / 2 + (offset ? offset->y : 0.0) };
		point p3, p4;
		t->l2d(&p1, &p3);
		t->l2d(&p2, &p4);
		HPEN hPen = CreatePen(PS_SOLID, 1, select ? RGB(255, 0, 0) : RGB(0, 0, 0));
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
		Rectangle(hdc, (int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y);
		SelectObject(hdc, hOldPen);
		DeleteObject(hPen);
		RECT rect{ (int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y };
		COLORREF oldColor = SetTextColor(hdc, select ? RGB(255, 0, 0) : RGB(0, 0, 0));
		DrawText(hdc, name, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		SetTextColor(hdc, oldColor);
		if (next) {
			const point p1 = { p.x + (offset ? offset->x : 0.0), p.y + s.h / 2 + (offset ? offset->y : 0.0) };
			const point p2 = { next->p.x + (offset ? offset->x : 0.0), next->p.y - next->s.h / 2 + (offset ? offset->y : 0.0) };
			point p3, p4;
			t->l2d(&p1, &p3);
			t->l2d(&p2, &p4);
			MoveToEx(hdc, (int)p3.x, (int)p3.y, 0);
			LineTo(hdc, (int)p4.x, (int)p4.y);
		}
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

class nodelist {
public:
	std::list<node*> l;
	nodelist() {}
	void add(node* n) {
		l.push_back(n);
	}
	void clear() {
		for (auto i : l) {
			delete i;
		}
		l.clear();
	}
	void paint(HDC hdc, trans* t, point* dragoffset = nullptr) {
		if (dragoffset) {
			for (auto i : l) {
				if (!i->select)
					i->paint(hdc, t);
			}
			for (auto i : l) {
				if (i->select)
					i->paint(hdc, t, dragoffset);
			}
		}
		else {
			for (auto i : l) {
					i->paint(hdc, t);
			}
		}
	}
	node* hit(const point* p, const node* without = nullptr) const {
		if (without) {
			for (auto i : l) {
				if (without != i && i->hit(p))
					return i;
			}
		} else {
			for (auto i : l) {
				if (i->hit(p))
					return i;
			}
		}
		return 0;
	}
	void select(const node* n) { // 指定した一つのみ選択状態にする
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
	int selectcount() const {
		int count = 0;
		for (auto i : l) {
			if (i->select) {
				count++;
			}
		}
		return count;
	}
	bool isselect(const node* n) const {
		for (auto i : l) {
			if (i == n) {
				return i->select;
			}
		}
		return false;
	}
	void selectlistup(std::list<node*>* selectnode) const {
		for (auto i : l) {
			if (i->select) {
				selectnode->push_back(i);
			}
		}
	}
	void selectoffsetmerge(const point* dragoffset) {
		for (auto i : l) {
			if (i->select) {
				i->p.x += dragoffset->x;
				i->p.y += dragoffset->y;
			}
		}
	}
	void allnodemargin(point* p1, point* p2) const {
		p1->x = DBL_MAX;
		p1->y = DBL_MAX;
		p2->x = -DBL_MAX;
		p2->y = -DBL_MAX;
		for (auto i : l) {
			if (!i->del) {
				if (i->p.x - i->s.w / 2 < p1->x) p1->x = i->p.x - i->s.w / 2;
				if (i->p.y - i->s.h / 2 < p1->y) p1->y = i->p.y - i->s.h / 2;
				if (i->p.x + i->s.w / 2 > p2->x) p2->x = i->p.x + i->s.w / 2;
				if (i->p.y + i->s.h / 2 > p2->y) p2->y = i->p.y + i->s.h / 2;
			}
		}
		if (
			p1->x == DBL_MAX ||
			p1->y == DBL_MAX ||
			p2->x == DBL_MIN ||
			p2->y == DBL_MIN
			) {
			p1->x = 0.0;
			p1->y = 0.0;
			p2->x = 0.0;
			p2->y = 0.0;
		}
	}
	void disconnectselectnode()
	{
		// まず繋げる
		for (auto i : l) {
			if (i->select) {
			}
			else {
				if (i->next) {
					if (i->next->select) {
						node* next = i->next;
						while (next) {
							if (!next->select)
							{
								i->next = next;
								break;
							}
							next = next->next;
						}
					}
				}
				if (i->prev) {
					if (i->prev->select) {
						node* prev = i->prev;
						while (prev) {
							if (!prev->select)
							{
								i->prev = prev;
								break;
							}
							prev = prev->prev;
						}
					}
				}
				else {
					normalization(i);
				}
			}
		}
		// 次に断ち切る
		for (auto i : l) {
			if (i->select) {
				if (i->next) {
					if (!i->next->select) {
						i->next = nullptr;
					}
				}
				if (i->prev) {
					if (!i->prev->select) {
						i->prev = nullptr;
					}
				}
			} else {
				if (i->next) {
					if (i->next->select) {
						i->next = nullptr;
					}
				}
				if (i->prev) {
					if (i->prev->select) {
						i->prev = nullptr;
					}
				}
			}
		}
	}
	void insert(node *s) { // 指定したノードの下に選択したノードを入れ込む
		if (s) {
			point p1[] = {
				{ s->p.x - s->s.w / 2,s->p.y - s->s.h / 2 },
				{ s->p.x + s->s.w / 2,s->p.y - s->s.h / 2 },
			};
			node* prev = nullptr;
			for (auto i : p1) {
				prev = hit(&i, s);
				if (prev) {
					break;
				}
			}
			if (prev) {
				if (prev->next) {
					node* last = s;
					while (last->next)
						last = last->next;
					prev->next->prev = last;
					last->next = prev->next;
				}
				s->prev = prev;
				prev->next = s;
				normalization(prev);
			}
		}
	}
	void normalization(node* n) {
		node* prev = n;
		if (!prev) return;
		node* next = n->next;
		while (next) {
			next->p.x = prev->p.x;
			next->p.y = prev->p.y + 100;
			prev = next;
			next = next->next;
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
	static HFONT hObjectFont, hUIFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	static trans t;
	static nodelist l;
	static int mode;
	static POINT dragstartP;
	static point dragstartL;
	static point dragoffset;
	static bool dragjudge;
	static bool modify;
	static UINT dragMsg;
	static HWND hList;
	static node* nn; // 新規作成ノード
	static node* dd; // ドラッグ中ノード
	if (msg == dragMsg) {
		static int nDragItem;
		LPDRAGLISTINFO lpdli = (LPDRAGLISTINFO)lParam;
		switch (lpdli->uNotification) {
		case DL_BEGINDRAG:
			nDragItem = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);
			if (nDragItem != -1) {
				nn = new node();
				nn->s.w = 100;
				nn->s.h = 50;
				nn->select = true;
				SendMessage(lpdli->hWnd, LB_GETTEXT, nDragItem, (LPARAM)nn->name);
			}
			return TRUE;
		case DL_DRAGGING:
			if (nn) {
				POINT p0 = { lpdli->ptCursor.x, lpdli->ptCursor.y };
				ScreenToClient(hWnd, &p0);
				point p1{ (double)p0.x, (double)p0.y };
				point p2;
				t.d2l(&p1, &p2);
				nn->p.x = p2.x;
				nn->p.y = p2.y;
				InvalidateRect(hWnd, NULL, TRUE);
			}
			return 0;
		case DL_CANCELDRAG:
			nDragItem = -1;
			if (nn) {
				delete nn;
				nn = nullptr;
			}
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case DL_DROPPED:
			if (nn) {
				if (WindowFromPoint(lpdli->ptCursor) == hWnd) {
					l.unselect();
					l.add(nn);
					l.insert(nn);
				} else {
					delete nn;
				}
			}
			nn = nullptr;
			nDragItem = -1;
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		return 0;
	}
	switch (msg)
	{
	case WM_CREATE:
		dragMsg = RegisterWindowMessage(DRAGLISTMSGSTRING);
		hList = CreateWindow(L"LISTBOX", 0, WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		MakeDragList(hList);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード1"));
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード2"));
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード3"));
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード4"));
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
			dd = l.hit(&p2);
			if (dd) {
				if (!l.isselect(dd)) {
					l.select(dd); // 選択されていないときは1つ選択する
				}
				InvalidateRect(hWnd, NULL, TRUE);
				dragstartP = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				dragstartL = p2;
				mode = dragnode;
				SetCapture(hWnd);
			} else {
				//矩形選択モード
				l.unselect();
				InvalidateRect(hWnd, NULL, TRUE);
				dragstartP = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				mode = rectselect;
				SetCapture(hWnd);
			}
		}
		break;
	case WM_LBUTTONUP:
		if (mode == dragnode) {
			if (dragjudge) {
				l.selectoffsetmerge(&dragoffset);
				dragjudge = false;
				if (dd) l.insert(dd);
			}
			else {
				if (dd) l.select(dd);
			}
			dd = nullptr;
			dragoffset = { 0.0, 0.0 };
		}
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
			point p2;
			t.d2l(&p1, &p2);
			dragoffset.x = p2.x - dragstartL.x;
			dragoffset.y = p2.y - dragstartL.y;
			if (abs(p1.x - dragstartP.x) >= DRAGJUDGEWIDTH || abs(p1.y - dragstartP.y) >= DRAGJUDGEWIDTH)
			{
				// ドラッグモードになった時は関係を断ち切る
				l.disconnectselectnode();
				dragjudge = true;
			}
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
				DeleteObject(hPen);
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
			DeleteObject(hObjectFont);
			hObjectFont = CreateFontW(-POINT2PIXEL((int)(FONT_SIZE * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hObjectFont);
			l.paint(hdc, &t, mode == dragnode ? &dragoffset : nullptr);
			if (nn) nn->paint(hdc, &t);
			SelectObject(hdc, hOldFont);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		MoveWindow(hList, 0, 0, POINT2PIXEL(128), HIWORD(lParam), 1);
		t.p.x = POINT2PIXEL(128);
		t.p.y = 0;
		t.c.w = LOWORD(lParam) - POINT2PIXEL(128);
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
			l.disconnectselectnode();
			l.del();
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		case ID_HOMEPOSITION:
			{
				point p1, p2;
				l.allnodemargin(&p1, &p2);
				t.settransfromrect(&p1, &p2, POINT2PIXEL(10));
			}
			DeleteObject(hObjectFont);
			hObjectFont = CreateFontW(-POINT2PIXEL((int)(FONT_SIZE * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
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
		DeleteObject(hObjectFont);
		hObjectFont = CreateFontW(-POINT2PIXEL((int)(FONT_SIZE * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
		DeleteObject(hUIFont);
		hUIFont = CreateFontW(-POINT2PIXEL(FONT_SIZE), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
		SendMessage(hList, WM_SETFONT, (WPARAM)hUIFont, 0);
		break;
	case WM_DESTROY:
		l.clear();
		DeleteObject(hObjectFont);
		DeleteObject(hUIFont);
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
		{FVIRTKEY, VK_HOME, ID_HOMEPOSITION},
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

