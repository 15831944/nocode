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
private:
	bool select;
public:
	WCHAR name[16];
	int k;
	point p;
	size s;
	node* next;
	node* prev;
	UINT64 born;
	UINT64 dead;
	node(UINT64 initborn) : k(0), next{}, prev{}, p{ 0.0, 0.0 }, s{ 0.0, 0.0 }, name{}, select(false), born(initborn), dead(UINT64_MAX) {}
	node(const node* src, UINT64 initborn) {
		select = src->select;
		lstrcpy(name, src->name);
		k = src->k;
		p = src->p;
		s = src->s;
		next = src->next;
		prev = src->prev;
		born = initborn;
		dead = UINT64_MAX;
	}
	~node() {}
	node* copy(UINT64 generation) const {
		node* newnode = new node(this, generation);
		return newnode;
	}
	virtual bool isalive(UINT64 generation) const {
		return born <= generation && generation < dead;
	}
	virtual void kill(UINT64 generation) {
		dead = generation;
	}
	virtual void paint(HDC hdc, const trans* t, UINT64 generation, const point* offset = nullptr) const {
		if (!isalive(generation)) return;
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
	virtual bool hit(const point* p, UINT64 generation) const {
		return
			isalive(generation) &&
			p->x >= this->p.x - s.w / 2 &&
			p->x <= this->p.x + s.w / 2 &&
			p->y >= this->p.y - s.h / 2 &&
			p->y <= this->p.y + s.h / 2;
	}
	virtual bool inrect(const point* p1, const point* p2, UINT64 generation) const {
		return
			isalive(generation) &&
			p.x - s.w / 2 >= p1->x &&
			p.y - s.h / 2 >= p1->y &&
			p.x + s.w / 2 <= p2->x &&
			p.y + s.h / 2 <= p2->y;
	}
	virtual bool isselect(UINT64 generation) {
		return select && isalive(generation);
	}
	virtual void setselect(bool b, UINT64 generation) {
		if (isalive(generation)) {
			select = b;
		}
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
	void setbornanddead(UINT64 generation) {
		for (auto i : l) {
			if (i->born > generation) {
				i->born = UINT64_MAX;
			}
			if (i->dead >= generation) {
				i->dead = UINT64_MAX;
			}
		}
	}
	void paint(HDC hdc, trans* t, UINT64 generation, point* dragoffset = nullptr) {
		if (dragoffset) {
			for (auto i : l) {
				if (!i->isselect(generation))
					i->paint(hdc, t, generation);
			}
			for (auto i : l) {
				if (i->isselect(generation))
					i->paint(hdc, t, generation, dragoffset);
			}
		}
		else {
			for (auto i : l) {
				i->paint(hdc, t, generation);
			}
		}
	}
	node* hit(const point* p, UINT64 generation, const node* without = nullptr) const {
		if (without) {
			for (auto i : l) {
				if (without != i && i->hit(p, generation))
					return i;
			}
		} else {
			for (auto i : l) {
				if (i->hit(p, generation))
					return i;
			}
		}
		return 0;
	}
	void select(const node* n, UINT64 generation) { // 指定した一つのみ選択状態にする
		for (auto i : l) {
			i->setselect(i == n, generation);
		}
	}
	void unselect(UINT64 generation) {
		for (auto i : l) {
			i->setselect(false, generation);
		}
	}
	void rectselect(const point* p1, const point* p2, UINT64 generation) {
		for (auto i : l) {
			i->setselect(i->inrect(p1, p2, generation), generation);
		}
	}
	void allselect(UINT64 generation) {
		for (auto i : l) {
			i->setselect(true, generation);
		}
	}
	void del(UINT64 generation) {
		for (auto i : l) {
			if (i->isselect(generation)) {
				i->dead = generation;
			}
		}
	}
	int selectcount(UINT64 generation) const {
		int count = 0;
		for (auto i : l) {
			if (i->isselect(generation)) {
				count++;
			}
		}
		return count;
	}
	bool isselect(const node* n, UINT64 generation) const {
		for (auto i : l) {
			if (i == n) {
				return i->isselect(generation);
			}
		}
		return false;
	}
	void selectlistup(std::list<node*>* selectnode, UINT64 generation) const {
		for (auto i : l) {
			if (i->isselect(generation)) {
				selectnode->push_back(i);
			}
		}
	}
	void selectoffsetmerge(const point* dragoffset, UINT64 generation) {
		std::list<node*> selectnode;
		selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			i->kill(generation);
			node* newnode = new node(i, generation);
			newnode->p.x += dragoffset->x;
			newnode->p.y += dragoffset->y;
			l.push_back(newnode);
		}
	}
	void allnodemargin(point* p1, point* p2, UINT64 generation) const {
		p1->x = DBL_MAX;
		p1->y = DBL_MAX;
		p2->x = -DBL_MAX;
		p2->y = -DBL_MAX;
		for (auto i : l) {
			if (i->isalive(generation)) {
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
	void disconnectselectnode(UINT64 generation)
	{
		// まず繋げる
		for (auto i : l) {
			if (i->isselect(generation)) {
			}
			else {
				if (i->next) {
					if (i->next->isselect(generation)) {
						node* next = i->next;
						while (next) {
							if (!next->isselect(generation))
							{
								i->next = next;
								break;
							}
							next = next->next;
						}
					}
				}
				if (i->prev) {
					if (i->prev->isselect(generation)) {
						node* prev = i->prev;
						while (prev) {
							if (!prev->isselect(generation))
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
			if (i->isselect(generation)) {
				if (i->next) {
					if (!i->next->isselect(generation)) {
						i->next = nullptr;
					}
				}
				if (i->prev) {
					if (!i->prev->isselect(generation)) {
						i->prev = nullptr;
					}
				}
			} else {
				if (i->next) {
					if (i->next->isselect(generation)) {
						i->next = nullptr;
					}
				}
				if (i->prev) {
					if (i->prev->isselect(generation)) {
						i->prev = nullptr;
					}
				}
			}
		}
	}
	void insert(node *s, UINT64 generation) { // 指定したノードの下に選択したノードを入れ込む
		if (s) {
			point p1[] = {
				{ s->p.x - s->s.w / 2,s->p.y - s->s.h / 2 },
				{ s->p.x + s->s.w / 2,s->p.y - s->s.h / 2 },
			};
			node* prev = nullptr;
			for (auto i : p1) {
				prev = hit(&i, generation, s);
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
	rdown = 4,
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
	UINT64 generation;
	UINT64 maxgeneration;

	void beginedit() {
		if (generation < maxgeneration)
		{
			l.setbornanddead(generation);
			maxgeneration = generation + 1;
		}
		else
		{
			++maxgeneration;
		}
		generation = maxgeneration;
	}

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
		, generation(0)
		, maxgeneration(0)
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
		dd = l.hit(&p2, generation);
		if (dd) {
			if (!l.isselect(dd, generation)) {
				l.select(dd, generation); // 選択されていないときは1つ選択する
			}
			dragstartP = { x, y };
			dragstartL = p2;
			mode = dragnode;
		}
		else {
			//矩形選択モード
			l.unselect(generation);
			dragstartP = { x, y };
			mode = rectselect;
		}
		InvalidateRect(hWnd, NULL, TRUE);
		SetCapture(hWnd);
	}

	void OnLButtonUp(int x, int y) {
		if (mode == dragnode) {
			if (dragjudge) {
				beginedit();
				l.selectoffsetmerge(&dragoffset, generation);
				dragjudge = false;
				if (dd) l.insert(dd, generation);
			}
			else {
				if (dd) l.select(dd, generation);
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

	void OnRButtonDown(int x, int y) {
		point p1{ (double)x, (double)y };
		point p2;
		t.d2l(&p1, &p2);
		dd = l.hit(&p2, generation);
		if (dd) {
			if (!l.isselect(dd, generation)) {
				l.select(dd, generation); // 選択されていないときは1つ選択する
			}
			dragstartP = { x, y };
			dragstartL = p2;
		}
		else {
			l.unselect(generation);
		}

		mode = rdown;
		InvalidateRect(hWnd, NULL, TRUE);
		SetCapture(hWnd);
	}

	void OnRButtonUp(int x, int y) {
		ReleaseCapture();
		if (mode == rdown) {
			POINT point = { x, y };
			ClientToScreen(hWnd, &point);
			HMENU hMenu;
			if (l.selectcount(generation) > 0) {
				hMenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_NODEPOPUP));
			} else {
				hMenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_CLIENTPOPUP));
			}
			HMENU hSubMenu = GetSubMenu(hMenu, 0);
			TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, point.x, point.y, 0, hWnd, NULL);
		}
		mode = none;
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
				l.disconnectselectnode(generation);
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
			l.rectselect(&p3, &p4, generation);
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
			OnShrink();
		}
		else {
			OnZoom();
		}
	}

	void OnDestroy() {
		l.clear();
		DeleteObject(hObjectFont);
	}

	void OnPaint(HDC hdc) {
		HFONT hOldFont = (HFONT)SelectObject(hdc, hObjectFont);
		l.paint(hdc, &t, generation, mode == dragnode ? &dragoffset : nullptr);
		if (nn) nn->paint(hdc, &t, generation);
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
			beginedit();
			l.unselect(generation);

			nn->born = generation;

			l.add(nn);
			l.insert(nn, generation);
			nn = nullptr;
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	void OnHome() {
		point p1, p2;
		l.allnodemargin(&p1, &p2, generation);
		t.settransfromrect(&p1, &p2, POINT2PIXEL(10));
		resetFont();
	}

	void OnDelete() {
		beginedit();
		l.disconnectselectnode(generation);
		l.del(generation);
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnUnselect() {
		l.unselect(generation);
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnAllselect() {
		l.allselect(generation);
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnNew() {
		if (modify) {
			if (MessageBox(hWnd, L"新規作成しますか？", L"確認", MB_YESNO) == IDNO)
				return;
		}
		// 初期化
		generation = 0;
		maxgeneration = 0;
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

	void OnUndo() {
		if (generation > 0) {
			--generation;
		}
		InvalidateRect(hWnd, NULL, TRUE);
	}

	void OnRedo() {
		if (generation < maxgeneration)
		{
			++generation;
		}
		InvalidateRect(hWnd, NULL, TRUE);
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
				node* nn = new node(0);
				nn->s.w = 100;
				nn->s.h = 50;
				nn->setselect(true, 0);
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
	case WM_RBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_RBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
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
		case ID_UNDO:
			if (pNoCodeApp) {
				pNoCodeApp->OnUndo();
			}
			break;
		case ID_REDO:
			if (pNoCodeApp) {
				pNoCodeApp->OnRedo();
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
		MAKEINTRESOURCE(IDR_MAIN),
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
		{FVIRTKEY | FCONTROL, 'Z', ID_UNDO},
		{FVIRTKEY | FCONTROL, 'Y', ID_REDO},
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

