#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <list>
#include "util.h"
#include "resource.h"

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)
#define DRAGJUDGEWIDTH POINT2PIXEL(4)
#define FONT_NAME L"Yu Gothic UI"
#define FONT_SIZE 12

WCHAR szClassName[] = L"NoCode Editor";
HHOOK g_hHook;

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

enum NODE_KIND {
	NODE_NORMAL,
	NODE_START,
	NODE_END,
	NODE_IF,
	NODE_LOOP,
	NODE_GOTO,

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
	node() : k(0), next{}, prev{}, p{ 0.0, 0.0 }, s{ 0.0, 0.0 }, name{}, select(false), del(false) {}
	~node() {}
	virtual void paint(HDC hdc, const trans* t, const point* offset = nullptr) const {
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
			POINT p5 = { (int)p3.x, (int)p3.y }, p6 = { (int)p4.x, (int)p4.y };
			util::DrawArrow(hdc, &p5, &p6, &t->z);
		}
	}
	virtual bool hit(const point* p) const {
		return
			!del &&
			p->x >= this->p.x - s.w / 2 &&
			p->x <= this->p.x + s.w / 2 &&
			p->y >= this->p.y - s.h / 2 &&
			p->y <= this->p.y + s.h / 2;
	}
	virtual bool inrect(const point* p1, const point* p2) const {
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

class NoCodeApp {
public:
	HFONT hObjectFont;
	trans t;
	nodelist l;
	Mode mode;
	POINT dragstartP;
	point dragstartL;
	point dragoffset;
	bool dragjudge;
	bool modify;
	node* dd; // ドラッグ中ノード
	HWND hWnd;
	UINT uDpiX, uDpiY;
	node* nn; // 新規作成ノード

	NoCodeApp(HWND hMainWnd)
		:hObjectFont(0)
		, t{}
		, l{}
		, mode(none)
		, dragstartP{}
		, dragstartL{}
		, dragoffset{}
		, dragjudge(false)
		, modify(false)
		, dd(nullptr)
		, hWnd(hMainWnd)
		, uDpiX(DEFAULT_DPI)
		, uDpiY(DEFAULT_DPI)
		, nn(nullptr)
	{}

	void resetFont() {
		if (hWnd) {
			GetScaling(hWnd, &uDpiX, &uDpiY);
			DeleteObject(hObjectFont);
			hObjectFont = CreateFontW(-POINT2PIXEL((int)(FONT_SIZE * t.z)), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	void OnLButtonDown(int x, int y) {
		point p1{ (double)x, (double)y };
		point p2;
		t.d2l(&p1, &p2);
		dd = l.hit(&p2);
		if (dd) {
			if (!l.isselect(dd)) {
				l.select(dd); // 選択されていないときは1つ選択する
			}
			dragstartP = { x, y };
			dragstartL = p2;
			mode = dragnode;
		}
		else {
			//矩形選択モード
			l.unselect();
			dragstartP = { x, y };
			mode = rectselect;
		}
		InvalidateRect(hWnd, NULL, TRUE);
		SetCapture(hWnd);
	}

	void OnLButtonUp(int x, int y) {
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
	}

	void OnMButtonDown(int x, int y) {
		dragstartP = { x, y };
		dragstartL = t.l;
		mode = dragclient;
		SetCapture(hWnd);
	}

	void OnMButtonUp(int x, int y) {
		mode = none;
		ReleaseCapture();
	}

	void OnMouseMove(int x, int y) {
		if (mode == dragclient) {
			point p1{ (double)x, (double)y };
			t.l.x = dragstartL.x - (p1.x - dragstartP.x) / t.z;
			t.l.y = dragstartL.y - (p1.y - dragstartP.y) / t.z;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		else if (mode == dragnode) {
			point p1{ (double)x, (double)y };
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
			point p1{ (double)min(dragstartP.x,x), (double)min(dragstartP.y,y) };
			point p2{ (double)max(dragstartP.x,x), (double)max(dragstartP.y,y) };
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
	}
	void OnDpiChanged() {
		resetFont();
	}

	void OnMouseWheel(int delta) {
		if (delta < 0) {
			t.z *= 1.0 / 1.20;
			if (t.z < 0.5) {
				t.z = 0.5;
				return;
			}
		}
		else {
			t.z *= 1.20;
			if (t.z > 10) {
				t.z = 10;
				return;
			}
		}
		resetFont();
	}

	void OnDestroy() {
		l.clear();
		DeleteObject(hObjectFont);
	}

	void OnPaint(HDC hdc) {
		HFONT hOldFont = (HFONT)SelectObject(hdc, hObjectFont);
		l.paint(hdc, &t, mode == dragnode ? &dragoffset : nullptr);
		if (nn) nn->paint(hdc, &t);
		SelectObject(hdc, hOldFont);
	}

	void OnSize(int x, int y, int w, int h) {
		t.p.x = x;
		t.p.y = y;
		t.c.w = w;
		t.c.h = h;
	}

	void OnBeginDrag(node* n) {
		nn = n;
	}

	void OnDragging(int x, int y) {
		if (nn) {
			point p1{ (double)x, (double)y };
			point p2;
			t.d2l(&p1, &p2);
			nn->p.x = p2.x;
			nn->p.y = p2.y;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	void OnCancelDrag() {
		if (nn) {
			delete nn;
			nn = nullptr;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}
	
	void OnDropped() {
		if (nn) {
			l.unselect();
			l.add(nn);
			l.insert(nn);
			nn = nullptr;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	void OnHome() {
		point p1, p2;
		l.allnodemargin(&p1, &p2);
		t.settransfromrect(&p1, &p2, POINT2PIXEL(10));
		resetFont();
	}

	void OnDelete() {
		l.disconnectselectnode();
		l.del();
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnUnselect() {
		l.unselect();
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnAllselect() {
		l.allselect();
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnNew() {
		if (modify) {
			if (MessageBox(hWnd, L"新規作成しますか？", L"確認", MB_YESNO) == IDNO)
				return;
		}
		l.clear();
		t.l = { 0.0, 0.0 };
		t.z = 1.0;
		resetFont();
	}

	void OnZoom() {
		t.z *= 1.20;
		if (t.z > 10) {
			t.z = 10;
			return;
		}
		resetFont();
	}

	void OnShrink() {
		t.z *= 1.0 / 1.20;
		if (t.z < 0.5) {
			t.z = 0.5;
			return;
		}
		resetFont();
	}
};

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	switch (nCode) {
	case HCBT_ACTIVATE:
	{
		UnhookWindowsHookEx(g_hHook);
		HWND hMes = (HWND)wParam;
		HWND hWnd = GetParent(hMes);
		RECT rectMessageBox, rectParentWindow;
		GetWindowRect(hMes, &rectMessageBox);
		GetWindowRect(hWnd, &rectParentWindow);
		RECT rect = {
			(rectParentWindow.right + rectParentWindow.left - rectMessageBox.right + rectMessageBox.left) / 2,
			(rectParentWindow.bottom + rectParentWindow.top - rectMessageBox.bottom + rectMessageBox.top) / 2,
			(rectParentWindow.right + rectParentWindow.left - rectMessageBox.right + rectMessageBox.left) / 2 + rectMessageBox.right - rectMessageBox.left,
			(rectParentWindow.bottom + rectParentWindow.top - rectMessageBox.bottom + rectMessageBox.top) / 2 + rectMessageBox.bottom - rectMessageBox.top
		};
		util::ClipOrCenterRectToMonitor(&rect, MONITOR_CLIP | MONITOR_WORKAREA);
		SetWindowPos(hMes, hWnd, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	static UINT dragMsg;
	static HWND hList;
	static HFONT hUIFont;
	static NoCodeApp* pNoCodeApp;
	if (msg == dragMsg) {
		static int nDragItem;
		LPDRAGLISTINFO lpdli = (LPDRAGLISTINFO)lParam;
		switch (lpdli->uNotification) {
		case DL_BEGINDRAG:
			nDragItem = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);
			if (nDragItem != -1) {
				node* nn = new node();
				nn->s.w = 100;
				nn->s.h = 50;
				nn->select = true;
				SendMessage(lpdli->hWnd, LB_GETTEXT, nDragItem, (LPARAM)nn->name);
				if (pNoCodeApp) {
					pNoCodeApp->OnBeginDrag(nn);
				}
			}
			return TRUE;
		case DL_DRAGGING:
			if (pNoCodeApp) {
				POINT p = { lpdli->ptCursor.x, lpdli->ptCursor.y };
				ScreenToClient(hWnd, &p);
				pNoCodeApp->OnDragging(p.x, p.y);
			}
			return 0;
		case DL_CANCELDRAG:
			if (pNoCodeApp) {
				pNoCodeApp->OnCancelDrag();
			}
			break;
		case DL_DROPPED:
			if (pNoCodeApp) {
				if (WindowFromPoint(lpdli->ptCursor) == hWnd) {
					pNoCodeApp->OnDropped();
				} else {
					pNoCodeApp->OnCancelDrag();
				}
			}
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
		pNoCodeApp = new NoCodeApp(hWnd);
		SendMessage(hWnd, WM_DPICHANGED, 0, 0);
		break;
	case WM_LBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnMButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnMButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		if (pNoCodeApp) {
			pNoCodeApp->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MOUSEWHEEL:
		if (pNoCodeApp) {
			pNoCodeApp->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (pNoCodeApp) {
				pNoCodeApp->OnPaint(hdc);
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		MoveWindow(hList, 0, 0, POINT2PIXEL(128), HIWORD(lParam), 1);
		if (pNoCodeApp) {
			pNoCodeApp->OnSize(POINT2PIXEL(128), 0, LOWORD(lParam) - POINT2PIXEL(128), HIWORD(lParam));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_NEW:
			if (pNoCodeApp) {
				pNoCodeApp->OnNew();
			}
			break;
		case ID_ALLSELECT:
			if (pNoCodeApp) {
				pNoCodeApp->OnAllselect();
			}
			break;
		case ID_UNSELECT:
			if (pNoCodeApp) {
				pNoCodeApp->OnUnselect();
			}
			break;
		case ID_DELETE:
			if (pNoCodeApp) {
				pNoCodeApp->OnDelete();
			}
			break;
		case ID_HOME:
			if (pNoCodeApp) {
				pNoCodeApp->OnHome();
			}
			break;
		case ID_EXIT:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case ID_ZOOM:
			if (pNoCodeApp) {
				pNoCodeApp->OnZoom();
			}
			break;
		case ID_SHRINK:
			if (pNoCodeApp) {
				pNoCodeApp->OnShrink();
			}
			break;
		}
		break;
	case WM_CLOSE:
		g_hHook = SetWindowsHookEx(WH_CBT, CBTProc, NULL, ::GetCurrentThreadId());
		if (MessageBox(hWnd, L"終了しますか？", L"確認", MB_YESNO) == IDYES) {
			DestroyWindow(hWnd);
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
		if (pNoCodeApp)
		{
			pNoCodeApp->OnDpiChanged();
		}
		DeleteObject(hUIFont);
		hUIFont = CreateFontW(-POINT2PIXEL(FONT_SIZE), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
		SendMessage(hList, WM_SETFONT, (WPARAM)hUIFont, 0);
		break;
	case WM_DESTROY:
		if (pNoCodeApp) {
			pNoCodeApp->OnDestroy();
		}
		DeleteObject(hUIFont);
		delete pNoCodeApp;
		pNoCodeApp = nullptr;
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
		MAKEINTRESOURCE(IDR_MENU1),
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
		{FVIRTKEY | FCONTROL, 'A', ID_ALLSELECT},
		{FVIRTKEY, VK_ESCAPE, ID_UNSELECT},
		{FVIRTKEY, VK_DELETE, ID_DELETE},
		{FVIRTKEY, VK_HOME, ID_HOME},
		{FVIRTKEY | FCONTROL, 'W', ID_EXIT},
		{FVIRTKEY | FCONTROL, VK_OEM_PLUS, ID_ZOOM},
		{FVIRTKEY | FCONTROL, VK_OEM_MINUS, ID_SHRINK},
		{FVIRTKEY | FCONTROL, VK_ADD, ID_ZOOM},
		{FVIRTKEY | FCONTROL, VK_SUBTRACT, ID_SHRINK},
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

