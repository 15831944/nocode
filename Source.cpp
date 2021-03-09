#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <list>
#include <vector>
#include <string>
#include "util.h"
#include <d2d1_3.h>
#include <dwrite.h>
#include <wincodec.h>
#include "resource.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, g_c.uDpiY, 72)
#define DRAGJUDGEWIDTH POINT2PIXEL(4)
#define FONT_NAME L"Yu Gothic UI"
#define FONT_SIZE 14
#define NODE_WIDTH 200
#define NODE_HEIGHT 100
#define ZOOM_MAX 10.0
#define ZOOM_MIN 0.5
#define ZOOM_STEP 1.20

WCHAR szClassName[] = L"NoCode Editor";
HHOOK g_hHook;

template<class Interface> inline void SafeRelease(Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

INT_PTR CALLBACK DialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		{
			HWND hWnd = GetTopWindow(hDlg);
			if (hWnd) {
				MoveWindow(hWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), 0);
			}
		}
		break;
	}
	return FALSE;
}

class common {
public:
	HFONT hUIFont;
	HWND hTool;
	HWND hPropContainer;
	UINT uDpiX = DEFAULT_DPI;
	UINT uDpiY = DEFAULT_DPI;

	common()
	: hTool(0)
	, hUIFont(0)
	, uDpiX(DEFAULT_DPI)
	, uDpiY(DEFAULT_DPI)
	{
		//CreateFont
	}
	~common() {
	}
};

common g_c;

class graphic {
public:
	ID2D1Factory* m_pD2DFactory;
	IWICImagingFactory* m_pWICFactory;
	IDWriteFactory* m_pDWriteFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	IDWriteTextFormat* m_pTextFormat;
	ID2D1SolidColorBrush* m_pNormalBrush;
	ID2D1SolidColorBrush* m_pSelectBrush;
	graphic(HWND hWnd)
		: m_pD2DFactory(0)
		, m_pWICFactory(0)
		, m_pDWriteFactory(0)
		, m_pRenderTarget(0)
		, m_pTextFormat(0)
		, m_pNormalBrush(0)
		, m_pSelectBrush(0)
	{
		static const WCHAR msc_fontName[] = L"Verdana";
		static const FLOAT msc_fontSize = 25;

		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
		if (SUCCEEDED(hr))
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (SUCCEEDED(hr))
			hr = m_pDWriteFactory->CreateTextFormat(msc_fontName, 0, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, msc_fontSize, L"", &m_pTextFormat);
		if (SUCCEEDED(hr))
			hr = m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		if (SUCCEEDED(hr))
			hr = m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		if (FAILED(hr)) {
			MessageBox(hWnd, L"Direct2D の初期化に失敗しました。", 0, 0);
		}
	}
	~graphic() {
		SafeRelease(&m_pD2DFactory);
		SafeRelease(&m_pDWriteFactory);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pTextFormat);
		SafeRelease(&m_pNormalBrush);
		SafeRelease(&m_pSelectBrush);
	}
	bool begindraw(HWND hWnd) {
		HRESULT hr = S_OK;
		if (!m_pRenderTarget)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			D2D1_SIZE_U size = D2D1::SizeU(rect.right, rect.bottom);
			hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), DEFAULT_DPI, DEFAULT_DPI, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT), D2D1::HwndRenderTargetProperties(hWnd, size), &m_pRenderTarget);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pNormalBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &m_pSelectBrush);
		}
		if (SUCCEEDED(hr))
		{
			m_pRenderTarget->BeginDraw();
		}
		return SUCCEEDED(hr);
	}
	void enddraw() {
		HRESULT hr = m_pRenderTarget->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			SafeRelease(&m_pRenderTarget);
			SafeRelease(&m_pNormalBrush);
			SafeRelease(&m_pSelectBrush);
		}
	}
};

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

enum PROPERTY_KIND {
	PROPERTY_NONE,
	PROPERTY_INT,
	PROPERTY_CHECK,
	PROPERTY_STRING,
	PROPERTY_DATE,
	PROPERTY_TIME,
	PROPERTY_IP,
	PROPERTY_PICTURE,
	PROPERTY_COLOR,
	PROPERTY_SELECT,
	PROPERTY_CUSTOM
};

class propertyitem {
public:
	PROPERTY_KIND kind;
	bool required;
	std::wstring* name;
	std::wstring* help;
	std::wstring* description;
	std::wstring* value;
	std::wstring* unit;
	std::wstring* match;
	propertyitem() : kind(PROPERTY_NONE), required(false), name(0), help(0), description(0), value(0), unit(0), match(0){}
	~propertyitem() {
		delete name;
		delete help;
		delete description;
		delete value;
		delete unit;
		delete match;
	}
	propertyitem* copy() const {
		propertyitem* n = new propertyitem;
		n->kind = kind;
		n->required = required;
		if (name) n->name = new std::wstring(*name);
		if (help) n->help = new std::wstring(*help);
		if (description) n->description = new std::wstring(*description);
		if (value) n->value = new std::wstring(*value);
		if (unit) n->unit = new std::wstring(*unit);
		if (match) n->match = new std::wstring(*match);
		return n;
	}
	void setname(LPCWSTR lpszName) {
		name = new std::wstring(lpszName);
	}
	void sethelp(LPCWSTR lpszHelp) {
		help = new std::wstring(lpszHelp);
	}
	void setdescription(LPCWSTR lpszDescription) {
		description = new std::wstring(lpszDescription);
	}
	void setvalue(LPCWSTR lpszValue) {
		value = new std::wstring(lpszValue);
	}
	void setunit(LPCWSTR lpszUnit) {
		unit = new std::wstring(lpszUnit);
	}
	void setmatch(LPCWSTR lpszMatch) {
		match = new std::wstring(lpszMatch);
	}
};

class propertyitemlist {
public:
	std::wstring description;
	std::vector<propertyitem*> l;
	propertyitemlist() : l{} {}
	~propertyitemlist() {
		for (auto i : l) {
			delete i;
		}
		l.clear();
	}
	propertyitemlist* copy() const {
		propertyitemlist* nl = new propertyitemlist;
		nl->description = description;
		for (auto i : l) {
			nl->l.push_back(i->copy());
		}
		return nl;
	}
};

enum NODE_KIND {
	NODE_NONE,
	NODE_NORMAL1,
	NODE_NORMAL2,
	NODE_NORMAL3,
	NODE_NORMAL4,
	NODE_START,
	NODE_END,
	NODE_IF,
	NODE_LOOP,
	NODE_GOTO,
	NODE_CUSTOM,

	NODE_MULTI
};

class node {
private:
	bool select;
public:
	WCHAR name[16];
	NODE_KIND kind;
	point p;
	size s;
	node* next;
	node* prev;
	UINT64 born;
	UINT64 dead;
	propertyitemlist* pl;
	void setpropertyvalue(int index, const propertyitem* pi) {
		if (pl) {
			*(pl->l[index]->value) = *(pi->value);
		}
	}
	node(UINT64 initborn) : kind(NODE_NONE), next{}, prev{}, p{ 0.0, 0.0 }, s{NODE_WIDTH, NODE_HEIGHT}, name{}, select(false), born(initborn), dead(UINT64_MAX), pl(0) {
		pl = new propertyitemlist;
	}
	node(const node* src, UINT64 initborn) {
		select = src->select;
		lstrcpy(name, src->name);
		kind = src->kind;
		p = src->p;
		s = src->s;
		next = src->next;
		prev = src->prev;
		born = initborn;
		dead = UINT64_MAX;
		pl = src->pl->copy();
	}
	virtual ~node() {
		delete pl;
	}
	virtual node* copy(UINT64 generation) const {
		node* newnode = new node(this, generation);
		return newnode;
	}
	virtual bool isalive(UINT64 generation) const {
		return born <= generation && generation < dead;
	}
	virtual void kill(UINT64 generation) {
		if (isalive(generation)) {
			dead = generation;
		}
	}
	virtual void paint(const graphic* g, const trans* t, UINT64 generation, const point* offset = nullptr) const {
		if (!isalive(generation)) return;
		g->m_pRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
			D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
		);
		D2D1_RECT_F rect = {
			(float)(p.x - s.w / 2 + (offset ? offset->x : 0.0)),
			(float)(p.y - s.h / 2 + (offset ? offset->y : 0.0)),
			(float)(p.x + s.w / 2 + (offset ? offset->x : 0.0)),
			(float)(p.y + s.h / 2 + (offset ? offset->y : 0.0))
		};
		D2D1_ROUNDED_RECT rrect;
		rrect.radiusX = 3.0f;
		rrect.radiusY = 3.0f;
		rrect.rect = rect;
		g->m_pRenderTarget->DrawRoundedRectangle(&rrect, select ? g->m_pSelectBrush : g->m_pNormalBrush);
		g->m_pRenderTarget->DrawText(name, lstrlenW(name), g->m_pTextFormat, &rect, select ? g->m_pSelectBrush : g->m_pNormalBrush);
		if (next) {
			const point p1 = { p.x + (offset ? offset->x : 0.0), p.y + s.h / 2 + (offset ? offset->y : 0.0) };
			const point p2 = { next->p.x + (offset ? offset->x : 0.0), next->p.y - next->s.h / 2 + (offset ? offset->y : 0.0) };
			point p3, p4;
			t->l2d(&p1, &p3);
			t->l2d(&p2, &p4);
			POINT p5 = { (int)p3.x, (int)p3.y }, p6 = { (int)p4.x, (int)p4.y };
			//util::DrawArrow(hdc, &p5, &p6, &t->z);
		}
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
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

class node_normal_1 :public node
{
public:
	node_normal_1(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL1;
		pl->description = L"[入力値1]と[入力値2]を加算します。";
		propertyitem* pi = new propertyitem;
		pi->kind = PROPERTY_CHECK;
		pi->setname(L"名前1");
		pi->sethelp(L"名前");
		pi->setdescription(L"名前");
		pi->setvalue(L"1");
		pi->setunit(L"個");
		pi->setmatch(L"個");
		pl->l.push_back(pi);
	}
};

class node_normal_2 :public node
{
public:
	node_normal_2(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL2;
		for (int i = 0; i < 2; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_STRING;
			pi->setname(L"名前2");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"2");
			pi->setunit(L"個");
			pi->setmatch(L"個");
			pl->l.push_back(pi);
		}
	}
};

class node_normal_3 :public node
{
public:
	node_normal_3(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL3;
		for (int i = 0; i < 3; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_STRING;
			pi->setname(L"名前3");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"3");
			pi->setunit(L"個");
			pi->setmatch(L"個");
			pl->l.push_back(pi);
		}
	}
};

class node_normal_4 :public node
{
public:
	node_normal_4(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL4;
		for (int i = 0; i < 4; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_STRING;
			pi->setname(L"名前4");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"4");
			pi->setunit(L"個");
			pi->setmatch(L"個");
			pl->l.push_back(pi);
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
		for (std::list<node*>::iterator it = l.begin(); it != l.end(); ) {
			if ((*it)->born > generation) {
				delete (*it); // 未来に産まれる人たちは削除
				it = l.erase(it);
				continue;
			}
			it++;
		}
		for (auto i : l) {
			if (i->dead > generation) {
				i->dead = UINT64_MAX; // 今生きている人たちには永遠の命を与える
			}
		}
	}
	void paint(const graphic* g, trans* t, UINT64 generation, point* dragoffset = nullptr) {
		if (dragoffset) {
			for (auto i : l) {
				if (!i->isselect(generation))
					i->paint(g, t, generation);
			}
			for (auto i : l) {
				if (i->isselect(generation))
					i->paint(g, t, generation, dragoffset);
			}
		}
		else {
			for (auto i : l) {
				i->paint(g, t, generation);
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
	void rectselect(const point* p1, const point* p2, UINT64 generation, std::list<node*>* selectnode) {
		if (selectnode->size() == 0) {
			for (auto i : l) {
				i->setselect(i->inrect(p1, p2, generation), generation);
			}
		}
		else {
			for (auto i : l) {
				if (std::find(selectnode->begin(), selectnode->end(), i) != selectnode->end())
				{
					i->setselect(!i->inrect(p1, p2, generation), generation);
				}
				else
				{
					i->setselect(i->inrect(p1, p2, generation), generation);
				}
			}
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
	node* getfirstselectobject(UINT64 generation) const {
		for (auto i : l) {
			if (i->isselect(generation)) {
				return i;
			}
		}
		return 0;
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
			next->p.y = prev->p.y + NODE_HEIGHT*2;
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
	graphic* g;
	std::list<node*> selectnode;
	LONG_PTR editkind;

	node* GetFirstSelectObject() const {
		return l.getfirstselectobject(generation);
	}

	void RefreshToolBar() {

		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_UNDO, CanUndo());
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_REDO, CanRedo());
		bool selected = l.selectcount(generation) > 0;
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_COPY, selected);
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_CUT, selected);
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_PASTE, selected);
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_DELETE, selected);

		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_ZOOM, ZOOM_MAX > t.z);
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SHRINK, ZOOM_MIN < t.z);
	}

	bool beginedit(LONG_PTR kind = -1) {
		if (kind == -1 || kind != editkind) {
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
			RefreshToolBar();
			editkind = kind;
			return true;
		}
		return false;
	}

	NoCodeApp()
		: t{}
		, l{}
		, mode(none)
		, dragstartP{}
		, dragstartL{}
		, dragoffset{}
		, dragjudge(false)
		, modify(false)
		, dd(nullptr)
		, hWnd(0)
		, uDpiX(DEFAULT_DPI)
		, uDpiY(DEFAULT_DPI)
		, nn(nullptr)
		, generation(0)
		, maxgeneration(0)
		, g(nullptr)
		, selectnode{}
		, editkind(-1)
	{
	}

	void OnCreate(HWND hWnd) {
		this->hWnd = hWnd;
		g = new graphic(hWnd);
		OnProperty();
		RefreshToolBar();
	}

	void OnLButtonDown(int x, int y) {
		point p1{ (double)x, (double)y };
		point p2;
		t.d2l(&p1, &p2);
		dd = l.hit(&p2, generation);
		if (dd) {
			if (GetKeyState(VK_CONTROL) < 0) {
				dd->setselect(!l.isselect(dd, generation), generation); // Ctrlを押されているときは選択を追加
			}
			else if (!dd->isselect(generation)/*!l.isselect(dd, generation)*/){
				l.select(dd, generation); // 選択されていないときは1つ選択する
			}
			OnProperty();
			dragstartP = { x, y };
			dragstartL = p2;
			mode = dragnode;
		}
		else {
			//矩形選択モード
			if (GetKeyState(VK_CONTROL) < 0) {
				l.selectlistup(&selectnode, generation);
			}
			else {
				l.unselect(generation);
				OnProperty();
			}
			dragstartP = { x, y };
			mode = rectselect;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		SetCapture(hWnd);
	}

	void OnLButtonUp(int x, int y) {
		if (mode == dragnode) {
			if (dragjudge) {
				dragjudge = false;
				beginedit();
				l.selectoffsetmerge(&dragoffset, generation);
				if (dd) l.insert(dd, generation);
			}
			else if (dd && dd->isselect(generation))
			{
				if (GetKeyState(VK_CONTROL) < 0) {} else
				{
					l.select(dd, generation);
					OnProperty();
				}
			}
			dd = nullptr;
			dragoffset = { 0.0, 0.0 };
		}
		else if (mode == rectselect) {
			selectnode.clear();
		}
		mode = none;
		ReleaseCapture();
		InvalidateRect(hWnd, NULL, FALSE);
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
				OnProperty();
			}
			dragstartP = { x, y };
			dragstartL = p2;
		}
		else {
			l.unselect(generation);
			OnProperty();
		}

		mode = rdown;
		InvalidateRect(hWnd, NULL, FALSE);
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
			InvalidateRect(hWnd, NULL, FALSE);
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
			RefreshToolBar();
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (mode == rectselect) {
			point p1{ (double)min(dragstartP.x,x), (double)min(dragstartP.y,y) };
			point p2{ (double)max(dragstartP.x,x), (double)max(dragstartP.y,y) };
			point p3, p4;
			t.d2l(&p1, &p3);
			t.d2l(&p2, &p4);
			l.rectselect(&p3, &p4, generation, &selectnode);
			OnProperty();
			RefreshToolBar();
			InvalidateRect(hWnd, NULL, FALSE);
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
		delete g;
		g = nullptr;
	}

	void OnPaint() {
		if (g) {
			if (g->begindraw(hWnd)) {
				g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				g->m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
				l.paint(g, &t, generation, mode == dragnode ? &dragoffset : nullptr);
				if (nn) nn->paint(g, &t, generation);
				g->enddraw();
			}
		}
		ValidateRect(hWnd, NULL);
	}

	void OnSize(int x, int y, int w, int h) {
		t.p.x = x;
		t.p.y = y;
		t.c.w = w;
		t.c.h = h;
		if (g && g->m_pRenderTarget)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			D2D1_SIZE_U size = { (UINT32)rect.right, (UINT32)rect.bottom };
			g->m_pRenderTarget->Resize(size);
		}
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
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}

	void OnCancelDrag() {
		if (nn) {
			delete nn;
			nn = nullptr;
			InvalidateRect(hWnd, NULL, FALSE);
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
			OnProperty();
			RefreshToolBar();
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}

	void OnHome() {
		point p1, p2;
		l.allnodemargin(&p1, &p2, generation);
		t.settransfromrect(&p1, &p2, POINT2PIXEL(10));
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnDelete() {
		beginedit();
		l.disconnectselectnode(generation);
		l.del(generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnUnselect() {
		l.unselect(generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnAllselect() {
		l.allselect(generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
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
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnZoom() {
		double oldz = t.z;
		t.z *= ZOOM_STEP;
		if (t.z > ZOOM_MAX) {
			t.z = ZOOM_MAX;
		}
		if (oldz != t.z) {
			InvalidateRect(hWnd, NULL, FALSE);
		}
		RefreshToolBar();
	}

	void OnShrink() {
		double oldz = t.z;
		t.z *= 1.0 / ZOOM_STEP;
		if (t.z < ZOOM_MIN) {
			t.z = ZOOM_MIN;
		}
		if (oldz != t.z) {
			InvalidateRect(hWnd, NULL, FALSE);
		}
		RefreshToolBar();
	}

	bool CanUndo() {
		return generation > 0;
	}

	bool CanRedo() {
		return generation < maxgeneration;
	}

	void OnUndo() {
		if (generation > 0) {
			--generation;
		}
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnRedo() {
		if (generation < maxgeneration)
		{
			++generation;
		}
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnDisplayChange() {
		InvalidateRect(hWnd, 0, 0);
	}

	static BOOL CALLBACK EnumChildSetFontProc(HWND hWnd, LPARAM lParam)
	{
		SendMessage(hWnd, WM_SETFONT, (WPARAM)g_c.hUIFont, TRUE);
		return TRUE;
	}

	static INT_PTR CALLBACK DialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
	{
		static NoCodeApp* pNoCodeApp;
		static bool enable_edit_event;
		switch (msg)
		{
		case  WM_CTLCOLORDLG:
		case  WM_CTLCOLORSTATIC:
			return (INT_PTR)((HBRUSH)GetStockObject(WHITE_BRUSH));
		case WM_INITDIALOG:
			enable_edit_event = false;
			pNoCodeApp = (NoCodeApp*)lParam;
			if (pNoCodeApp) {
				RECT rect;
				GetClientRect(GetParent(hDlg), &rect);
				MoveWindow(hDlg, 0, 0, rect.right, rect.bottom, 0);
				node* n = pNoCodeApp->GetFirstSelectObject();
				if (n && n->pl) {
					CreateWindowEx(0, L"STATIC", n->pl->description.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, POINT2PIXEL(256), POINT2PIXEL(64), hDlg, 0, GetModuleHandle(0), 0);
					int nYtop = POINT2PIXEL(64);
					LONG_PTR index = 0;
					for (auto i : n->pl->l) {
						switch (i->kind) {
						case PROPERTY_NONE:
							break;
						case PROPERTY_INT:
							CreateWindowEx(0, L"STATIC", (i->name) ? ((*i->name + L":").c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, 0, nYtop, POINT2PIXEL(64), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", (i->value) ? (i->value->c_str()) : 0, WS_CHILD | WS_VISIBLE, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
							CreateWindowEx(0, L"STATIC", (i->unit) ? (i->unit->c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, POINT2PIXEL(64 + 128 + 32), nYtop, POINT2PIXEL(32), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							break;
						case PROPERTY_CHECK:
							CreateWindowEx(0, L"BUTTON", (i->name) ? (i->name->c_str()) : 0, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, POINT2PIXEL(8), nYtop, POINT2PIXEL(256)- POINT2PIXEL(8), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							if (i->value) {
								if (i->value->_Equal(L"0")) {

								}
								else
								{

								}
							}
							break;
						case PROPERTY_STRING:
							CreateWindowEx(0, L"STATIC", (i->name) ? ((*i->name + L":").c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, 0, nYtop, POINT2PIXEL(64), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", (i->value) ? (i->value->c_str()) : 0, WS_CHILD | WS_VISIBLE, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
							CreateWindowEx(0, L"STATIC", (i->unit) ? (i->unit->c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, POINT2PIXEL(64 + 128 + 32), nYtop, POINT2PIXEL(32), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							break;
						case PROPERTY_DATE:
							break;
						case PROPERTY_TIME:
							break;
						case PROPERTY_IP:
							break;
						case PROPERTY_PICTURE:
							break;
						case PROPERTY_COLOR:
							break;
						case PROPERTY_SELECT:
							break;
						case PROPERTY_CUSTOM:
							break;
						}
						nYtop += POINT2PIXEL(32);
						index++;
					}
				}
			}
			enable_edit_event = true;
			EnumChildWindows(hDlg, EnumChildSetFontProc, 0);
			//SetFocus(GetTopWindow(hDlg));
			return TRUE;
		case WM_COMMAND:
			if (pNoCodeApp) {
				if (enable_edit_event) {
					if (HIWORD(wParam) == EN_UPDATE) {
						node* n = pNoCodeApp->GetFirstSelectObject();
						if (n)
						{
							if (n->pl) {
								int nIndex = LOWORD(wParam) - 1000;
								propertyitem* pi = n->pl->l[nIndex]->copy();
								{
									DWORD dwSize = GetWindowTextLength((HWND)lParam);
									LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, (dwSize + 1) * sizeof(WCHAR));
									GetWindowText((HWND)lParam, lpszText, dwSize + 1);
									*(pi->value) = lpszText;
									GlobalFree(lpszText);
								}
								pNoCodeApp->UpdateSelectNode(nIndex, pi, (LONG_PTR)lParam); // とりあえずこの項目についてユニークな値が欲しい。
								delete pi;
							}
						}
					}
				}
			}
			break;
		}
		return FALSE;
	}

	void UpdateSelectNode(int index, const propertyitem* pi, LONG_PTR editkind = -1) {
		bool firstedit = beginedit(editkind);
		std::list<node*> selectnode;
		l.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			node* newnode = i;
			if (firstedit) {
				newnode->kill(generation);
				newnode = new node(i, generation);
			}
			newnode->setpropertyvalue(index, pi);
			if (firstedit) {
				l.add(newnode);
			}
		}
	}

	void OnProperty() {
		WCHAR szTitle[32] = {};
		GetWindowText(g_c.hPropContainer, szTitle, _countof(szTitle));
		node* n = 0;
		switch (GetSelectKind(n)) {
		case NODE_NONE:
			//if (lstrcmpi(szTitle, L"NODE_NONE") != 0)
			{
				SetWindowText(g_c.hPropContainer, L"NODE_NONE");
				DestroyWindow(GetTopWindow(g_c.hPropContainer));
				CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NONE), g_c.hPropContainer, DialogProc, (LPARAM)0);
			}
			break;
		case NODE_NORMAL1:
			//if (lstrcmpi(szTitle, L"NODE_NORMAL") != 0)
		{
			SetWindowText(g_c.hPropContainer, L"NODE_NORMAL1");
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, DialogProc, (LPARAM)this);
		}
		break;
		case NODE_NORMAL2:
			//if (lstrcmpi(szTitle, L"NODE_NORMAL") != 0)
		{
			SetWindowText(g_c.hPropContainer, L"NODE_NORMAL2");
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, DialogProc, (LPARAM)this);
		}
		break;
		case NODE_NORMAL3:
			//if (lstrcmpi(szTitle, L"NODE_NORMAL") != 0)
		{
			SetWindowText(g_c.hPropContainer, L"NODE_NORMAL3");
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, DialogProc, (LPARAM)this);
		}
		break;
		case NODE_NORMAL4:
			//if (lstrcmpi(szTitle, L"NODE_NORMAL") != 0)
		{
			SetWindowText(g_c.hPropContainer, L"NODE_NORMAL4");
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, DialogProc, (LPARAM)this);
		}
		break;
		case NODE_MULTI:
			//if (lstrcmpi(szTitle, L"NODE_MULTI") != 0)
			{
				SetWindowText(g_c.hPropContainer, L"NODE_MULTI");
				DestroyWindow(GetTopWindow(g_c.hPropContainer));
				CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_MULTI), g_c.hPropContainer, DialogProc, (LPARAM)0);
			}
			break;
		}
		SetFocus(g_c.hPropContainer);
	}

	NODE_KIND GetSelectKind(node *n) const {
		const int selcount = l.selectcount(generation);
		if (selcount > 1)
			return NODE_MULTI;
		if (selcount == 0)
			return NODE_NONE;
		std::list<node*> selectnode;
		l.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			n = i;
			return i->kind;
		}
		return NODE_NONE;
		// ひと先ず上記の単一選択だけ
		if (0) {
			NODE_KIND nKind = NODE_NONE;
			std::list<node*> selectnode;
			l.selectlistup(&selectnode, generation);
			for (auto i : selectnode) {
				if (nKind != i->kind) {
					if (nKind == NODE_NONE) {
						nKind = i->kind;
					}
					else {
						nKind = NODE_MULTI;
						break;
					}
				}
			}
			return nKind;
		}
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
	static UINT dragMsg;
	static HWND hList;
	static NoCodeApp* pNoCodeApp;
	if (msg == dragMsg) {
		static int nDragItem;
		LPDRAGLISTINFO lpdli = (LPDRAGLISTINFO)lParam;
		switch (lpdli->uNotification) {
		case DL_BEGINDRAG:
			nDragItem = LBItemFromPt(lpdli->hWnd, lpdli->ptCursor, TRUE);
			if (nDragItem != -1) {
				node* n = (node*)SendMessage(lpdli->hWnd, LB_GETITEMDATA, nDragItem, 0);
				if (n) {
					node* nn = new node(n, 0);
					nn->setselect(true, 0);
					SendMessage(lpdli->hWnd, LB_GETTEXT, nDragItem, (LPARAM)nn->name);
					if (pNoCodeApp) {
						pNoCodeApp->OnBeginDrag(nn);
					}
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
		SendMessage(hWnd, WM_APP, 0, 0);
		MakeDragList(hList);
		{
			const int nIndex = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード1"));
			if (nIndex != LB_ERR)
			{
				node* n = new node_normal_1(0);
				SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)n);
			}
		}
		{
			const int nIndex = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード2"));
			if (nIndex != LB_ERR)
			{
				node* n = new node_normal_2(0);
				SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)n);
			}
		}
		{
			const int nIndex = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード3"));
			if (nIndex != LB_ERR)
			{
				node* n = new node_normal_3(0);
				SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)n);
			}
		}
		{
			const int nIndex = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)TEXT("ノード4"));
			if (nIndex != LB_ERR)
			{
				node* n = new node_normal_4(0);
				SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)n);
			}
		}
		{
			TBBUTTON tbb[] = {
				{0,ID_NEW,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{1,ID_OPEN,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{2,ID_SAVE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{3,ID_EXIT,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{4,ID_UNDO,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{5,ID_REDO,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{6,ID_COPY,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{7,ID_CUT,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{8,ID_PASTE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{9,ID_DELETE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{10,ID_RUN,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{11,ID_SUSPEND,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{12,ID_STOP,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{13,ID_ZOOM,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{14,ID_SHRINK,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{15,ID_HOME,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{16,ID_HELP,TBSTATE_ENABLED,TBSTYLE_BUTTON},
			};
			g_c.hTool = CreateToolbarEx(hWnd, WS_CHILD | WS_VISIBLE, 0, _countof(tbb), ((LPCREATESTRUCT)lParam)->hInstance, IDR_TOOLBAR1, tbb, _countof(tbb), 0, 0, 64, 64, sizeof(TBBUTTON));
			LONG_PTR lStyle = GetWindowLongPtr(g_c.hTool, GWL_STYLE);
			lStyle = (lStyle | TBSTYLE_FLAT) & ~TBSTYLE_TRANSPARENT;
			SetWindowLongPtr(g_c.hTool, GWL_STYLE, lStyle);
		}
		g_c.hPropContainer = CreateDialog(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_PROPERTY), hWnd, DialogProc);
		pNoCodeApp = new NoCodeApp();
		if (pNoCodeApp) {
			pNoCodeApp->OnCreate(hWnd);
		}
		break;
	case WM_APP: // 画面DPIが変わった時、ウィンドウ作成時にフォントの生成を行う
		GetScaling(hWnd, &g_c.uDpiX, &g_c.uDpiY);
		DeleteObject(g_c.hUIFont);
		g_c.hUIFont = CreateFontW(-POINT2PIXEL(FONT_SIZE), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, FONT_NAME);
		SendMessage(hList, WM_SETFONT, (WPARAM)g_c.hUIFont, 0);
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
		if (pNoCodeApp) {
			pNoCodeApp->OnPaint();
		}
		break;
	case WM_SIZE:
		{
			SendMessage(g_c.hTool, TB_AUTOSIZE, 0, 0);
			RECT rect;
			GetWindowRect(g_c.hTool, &rect);
			MoveWindow(hList, 0, rect.bottom - rect.top, POINT2PIXEL(128), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			MoveWindow(g_c.hPropContainer, LOWORD(lParam) - POINT2PIXEL(256), rect.bottom - rect.top, POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			if (pNoCodeApp) {
				pNoCodeApp->OnSize(POINT2PIXEL(128), rect.bottom - rect.top, LOWORD(lParam) - POINT2PIXEL(128) - POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top));
			}
		}
		break;
	case WM_DISPLAYCHANGE:
		if (pNoCodeApp) {
			pNoCodeApp->OnDisplayChange();
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
		case ID_PROPERTY:
			if (pNoCodeApp) {
				pNoCodeApp->OnProperty();
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
		SendMessage(hWnd, WM_APP, 0, 0);
		break;
	case WM_DESTROY:
		if (hList) {
			int nIndexMax = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
			for (int i = 0; i < nIndexMax; i++) {
				node* n = (node*)SendMessage(hList, LB_GETITEMDATA, i, 0);
				if (n) {
					delete n;
				}
			}
		}
		if (pNoCodeApp) {
			pNoCodeApp->OnDestroy();
		}
		DeleteObject(g_c.hUIFont);
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
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif

	HeapSetInformation(0, HeapEnableTerminationOnCorruption, 0, 0);
	if (FAILED(CoInitialize(0))) return 0;
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		0,
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
		{FVIRTKEY | FALT, VK_RETURN, ID_PROPERTY},		
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
	CoUninitialize();
	return (int)msg.wParam;
}

