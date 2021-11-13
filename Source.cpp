#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <regex>
#include "util.h"
#include <d2d1_3.h>
#include <dwrite.h>
#include <wincodec.h>
#include <Richedit.h>
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
#define FONT_NAME L"Consolas"
#define FONT_SIZE 14
#define NODE_WIDTH 200
#define NODE_HEIGHT 100
#define GRID_WIDTH 25
#define ZOOM_MAX 10.0
#define ZOOM_MIN 0.5
#define ZOOM_STEP 1.20
#define CONNECT_POINT_WIDTH 8.0f

WCHAR szClassName[] = L"nocode";
HHOOK g_hHook;
WNDPROC DefaultRichEditProc;
LRESULT CALLBACK RichEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

template<class Interface> inline void SafeRelease(Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

class graphic {
public:
	ID2D1Factory* m_pD2DFactory;
	IWICImagingFactory* m_pWICFactory;
	IDWriteFactory* m_pDWriteFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	IDWriteTextFormat* m_pTextFormat;
	ID2D1SolidColorBrush* m_pNormalBrush;
	ID2D1SolidColorBrush* m_pSelectBrush;
	ID2D1SolidColorBrush* m_pNormalBkBrush;
	ID2D1SolidColorBrush* m_pSelectBkBrush;
	ID2D1SolidColorBrush* m_pGridBrush;
	ID2D1SolidColorBrush* m_pConnectPointBrush;
	ID2D1SolidColorBrush* m_pConnectPointBkBrush;
	graphic(HWND hWnd)
		: m_pD2DFactory(0)
		, m_pWICFactory(0)
		, m_pDWriteFactory(0)
		, m_pRenderTarget(0)
		, m_pTextFormat(0)
		, m_pNormalBrush(0)
		, m_pSelectBrush(0)
		, m_pNormalBkBrush(0)
		, m_pSelectBkBrush(0)
		, m_pGridBrush(0)
		, m_pConnectPointBrush(0)
		, m_pConnectPointBkBrush(0)
	{
		static const FLOAT msc_fontSize = 25;

		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
		if (SUCCEEDED(hr))
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (SUCCEEDED(hr))
			hr = m_pDWriteFactory->CreateTextFormat(FONT_NAME, 0, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, msc_fontSize, L"", &m_pTextFormat);
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
		SafeRelease(&m_pNormalBkBrush);
		SafeRelease(&m_pSelectBkBrush);
		SafeRelease(&m_pGridBrush);
		SafeRelease(&m_pConnectPointBrush);
		SafeRelease(&m_pConnectPointBkBrush);
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
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.75f), &m_pNormalBkBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.25f), &m_pSelectBkBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.80f, 0.80f, 0.80f, 1.00f), &m_pGridBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.00f), &m_pConnectPointBrush);
			if (SUCCEEDED(hr))
				hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.75f), &m_pConnectPointBkBrush);
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
			SafeRelease(&m_pNormalBkBrush);
			SafeRelease(&m_pSelectBkBrush);
			SafeRelease(&m_pGridBrush);
			SafeRelease(&m_pConnectPointBrush);
			SafeRelease(&m_pConnectPointBkBrush);
		}
	}
};

class common {
public:
	HFONT hUIFont;
	HWND hTool;
	HWND hPropContainer;
	HWND hNodeBox;
	HWND hMainWnd;
	HWND hOutput;
	HWND hVariableList;
	HWND hList;
	UINT uDpiX;
	UINT uDpiY;
	graphic* g;
	void* app;
	bool bShowGrid;
	common()
		: hTool(0)
		, hUIFont(0)
		, hPropContainer(0)
		, hNodeBox(0)
		, hMainWnd(0)
		, hOutput(0)
		, hVariableList(0)
		, hList(0)
		, uDpiX(DEFAULT_DPI)
		, uDpiY(DEFAULT_DPI)
		, g(0)
		, app(0)
		, bShowGrid(TRUE)
	{
		//CreateFont
	}
	~common() {
	}
};

common g_c;

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
	PROPERTY_SINGLELINESTRING,
	PROPERTY_MULTILINESTRING,
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
	propertyitem() : kind(PROPERTY_NONE), required(false), name(0), help(0), description(0), value(0), unit(0), match(0) {}
	propertyitem(const propertyitem* src) : kind(src->kind), required(src->required), name(0), help(0), description(0), value(0), unit(0), match(0) {
		if (src->name) name = new std::wstring(*src->name);
		if (src->help) help = new std::wstring(*src->help);
		if (src->description) description = new std::wstring(*src->description);
		if (src->value) value = new std::wstring(*src->value);
		if (src->unit) unit = new std::wstring(*src->unit);
		if (src->match) match = new std::wstring(*src->match);
	}
	~propertyitem() {
		delete name;
		delete help;
		delete description;
		delete value;
		delete unit;
		delete match;
	}
	propertyitem* copy() const {
		return new propertyitem(this);
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

enum OBJECT_KIND {
	OBJECT_PRIMITIVE,
	OBJECT_NODE,
	OBJECT_ARROW,
	OBJECT_COMMENT,
	NODE_NONE,
	NODE_OUTPUT,
	NODE_NORMAL2,
	NODE_NORMAL3,
	NODE_NORMAL4,
	NODE_NORMAL5,
	NODE_NORMAL6,
	NODE_NORMAL7,
	NODE_NORMAL8,
	NODE_NORMAL9,
	NODE_START,
	NODE_END,
	NODE_IF,
	NODE_LOOP,
	NODE_GOTO,
	NODE_CUSTOM,
	NODE_MULTI

};

class object {
public:
	OBJECT_KIND kind;
	bool select;
	UINT64 born;
	UINT64 dead;
	point p;
	size s;
	propertyitemlist* pl;
	object(UINT64 initborn) :kind(OBJECT_PRIMITIVE), select(false), born(initborn), dead(UINT64_MAX), p{}, s{}, pl(0)
	{
		pl = new propertyitemlist;
	}
	object(const object* src, UINT64 initborn) : kind(src->kind), select(src->select), born(initborn), dead(UINT64_MAX), p(src->p), s(src->s) {
		pl = src->pl->copy();
	}
	virtual ~object() {
		delete pl;
	}
	virtual bool isselect(UINT64 generation) {
		return select && isalive(generation);
	}
	virtual void setselect(bool b, UINT64 generation) {
		if (isalive(generation)) {
			select = b;
		}
	}
	virtual bool isalive(UINT64 generation) const {
		return born <= generation && generation < dead;
	}
	virtual void kill(UINT64 generation) {
		if (isalive(generation)) {
			dead = generation;
		}
	}
	virtual void paint(const graphic* g, const trans* t, bool drawconnectpoint, UINT64 generation, const point* offset = nullptr) const = 0;
	virtual int hitconnectpoint(const point* p, point* cp, UINT64 generation) const { return 0; }
	virtual bool hit(const point* p, UINT64 generation) const = 0;
	virtual bool inrect(const point* p1, const point* p2, UINT64 generation) const = 0;
	virtual object* copy(UINT64 generation) const = 0;
	virtual void move(const point* pt) {
		this->p.x += pt->y;
		this->p.y += pt->y;
	}
	virtual void setpropertyvalue(int index, const propertyitem* pi) {
		if (pl) {
			*(pl->l[index]->value) = *(pi->value);
		}
	}
	virtual void save(HANDLE hFile) const = 0;
	virtual void open(HANDLE hFile) const = 0;
};

class node : public object {
public:
	WCHAR name[16];
	node(UINT64 initborn) : object(initborn), name{} {
		kind = NODE_NONE;
		s = { NODE_WIDTH, NODE_HEIGHT };
	}
	node(const node* src, UINT64 initborn) : object(src, initborn) {
		lstrcpy(name, src->name);
	}
	virtual ~node() {
	}
	virtual node* copy(UINT64 generation) const = 0;
	virtual void paint(const graphic* g, const trans* t, bool drawconnectpoint, UINT64 generation, const point* offset = nullptr) const override {
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
		g->m_pRenderTarget->FillRoundedRectangle(&rrect, select ? g->m_pSelectBkBrush : g->m_pNormalBkBrush);
		g->m_pRenderTarget->DrawRoundedRectangle(&rrect, select ? g->m_pSelectBrush : g->m_pNormalBrush, 2.0f);
		g->m_pRenderTarget->DrawText(name, lstrlenW(name), g->m_pTextFormat, &rect, select ? g->m_pSelectBrush : g->m_pNormalBrush);
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		if (drawconnectpoint && select && offset == nullptr) {
			this->drawconnectpoint(g, t, generation);
		}
	}
	virtual void drawconnectpoint(const graphic* g, const trans* t, UINT64 generation) const {
		if (!isalive(generation)) return;
		g->m_pRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
			D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
		);
		D2D1_ELLIPSE top1 = { { (float)p.x, (float)(p.y - s.h / 2.0) }, CONNECT_POINT_WIDTH, CONNECT_POINT_WIDTH };
		D2D1_ELLIPSE bottom1 = { { (float)p.x, (float)(p.y + s.h / 2.0) }, CONNECT_POINT_WIDTH, CONNECT_POINT_WIDTH };
		D2D1_ELLIPSE left1 = { { (float)(p.x - s.w / 2.0), (float)p.y }, CONNECT_POINT_WIDTH, CONNECT_POINT_WIDTH };
		D2D1_ELLIPSE right1 = { { (float)(p.x + s.w / 2.0), (float)p.y }, CONNECT_POINT_WIDTH, CONNECT_POINT_WIDTH };
		g->m_pRenderTarget->FillEllipse(&top1, g->m_pConnectPointBkBrush);
		g->m_pRenderTarget->DrawEllipse(&top1, g->m_pConnectPointBrush);
		g->m_pRenderTarget->FillEllipse(&bottom1, g->m_pConnectPointBkBrush);
		g->m_pRenderTarget->DrawEllipse(&bottom1, g->m_pConnectPointBrush);
		g->m_pRenderTarget->FillEllipse(&left1, g->m_pConnectPointBkBrush);
		g->m_pRenderTarget->DrawEllipse(&left1, g->m_pConnectPointBrush);
		g->m_pRenderTarget->FillEllipse(&right1, g->m_pConnectPointBkBrush);
		g->m_pRenderTarget->DrawEllipse(&right1, g->m_pConnectPointBrush);
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	}
	virtual int hitconnectpoint(const point* p, point* cp, UINT64 generation) const override { // top->1, bottom->2, left->3, right->4, else->0
		if (isalive(generation)) {
			D2D1_POINT_2F points[] = {
				{ (float)this->p.x, (float)(this->p.y - s.h / 2.0) },
				{ (float)this->p.x, (float)(this->p.y + s.h / 2.0) },
				{ (float)(this->p.x - s.w / 2.0), (float)this->p.y },
				{ (float)(this->p.x + s.w / 2.0), (float)this->p.y }
			};
			int index = 0;
			for (auto i : points) {
				index++;
				if (sqrt((i.x - p->x) * (i.x - p->x) + (i.y - p->y) * (i.y - p->y)) < CONNECT_POINT_WIDTH) {
					if (cp) {
						cp->x = i.x;
						cp->y = i.y;
					}
					return index;
				}
			}
		}
		return 0;
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
	virtual bool execute() const = 0;
};

typedef enum { CONNECT_NONE, CONNECT_TOP, CONNECT_BOTTOM, CONNECT_LEFT, CONNECT_RIGHT } CONNECT_POSITION;

class arrow : public object {
public:
	node* start;
	node* end;
	CONNECT_POSITION start_pos;
	CONNECT_POSITION end_pos;

	const float nArrowSize = 16.0;
	const float nArrowWidth = 8.0;

	arrow(UINT64 initborn) : object(initborn), start(0), end(0), start_pos(CONNECT_NONE), end_pos(CONNECT_NONE) {}

	arrow(const arrow* src, UINT64 initborn) : object(initborn) {
		start = src->start;
		end = src->end;
		start_pos = src->start_pos;
		end_pos = src->end_pos;
	}

	virtual void paint(const graphic* g, const trans* t, bool drawconnectpoint, UINT64 generation, const point* offset = nullptr) const override { // 書き直す必要あり
		if (isalive(generation) && start && start->isalive(generation) && end && end->isalive(generation)
			&& start_pos != CONNECT_NONE && end_pos != CONNECT_NONE) {
			g->m_pRenderTarget->SetTransform(
				D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
				D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
			);
			D2D1_POINT_2F c1, c2, c3, c4;
			D2D1_POINT_2F t1, t2, t3;
			if (abs(start->p.x - end->p.x) - NODE_WIDTH < abs(start->p.y - end->p.y) - NODE_HEIGHT) {
				// たて⇔たて
				if (start->p.y < end->p.y) {
					c1 = { (float)(start->p.x + (offset ? offset->x : 0.0)), (float)(start->p.y + NODE_HEIGHT / 2.0 + (offset ? offset->y : 0.0)) };
					c4 = { (float)(end->p.x + (offset ? offset->x : 0.0)), (float)(end->p.y - NODE_HEIGHT / 2.0 - nArrowSize - 5.0f + (offset ? offset->y : 0.0)) };
					t1 = { c4.x, c4.y + nArrowSize};
					t2 = { t1.x - nArrowWidth,t1.y - nArrowSize };
					t3 = { t1.x + nArrowWidth,t1.y - nArrowSize };
				}
				else {
					c4 = { (float)(start->p.x + (offset ? offset->x : 0.0)), (float)(start->p.y - NODE_HEIGHT / 2.0 + (offset ? offset->y : 0.0)) };
					c1 = { (float)(end->p.x + (offset ? offset->x : 0.0)), (float)(end->p.y + NODE_HEIGHT / 2.0 + nArrowSize + 5.0f + (offset ? offset->y : 0.0)) };
					t1 = { c1.x, c1.y - nArrowSize };
					t2 = { t1.x - nArrowWidth,t1.y + nArrowSize };
					t3 = { t1.x + nArrowWidth,t1.y + nArrowSize };
				}
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			}
			else {
				// よこ⇔よこ
				if (start->p.x < end->p.x) {
					c1 = { (float)(start->p.x + NODE_WIDTH / 2.0 + (offset ? offset->x : 0.0)), (float)(start->p.y + (offset ? offset->y : 0.0)) };
					c4 = { (float)(end->p.x - NODE_WIDTH / 2.0 - nArrowSize - 5.0f + (offset ? offset->x : 0.0)), (float)(end->p.y + (offset ? offset->y : 0.0)) };
					t1 = { c4.x + nArrowSize, c4.y};
					t2 = { t1.x - nArrowSize,t1.y + nArrowWidth };
					t3 = { t1.x - nArrowSize,t1.y - nArrowWidth };
				}
				else {
					c4 = { (float)(start->p.x - NODE_WIDTH / 2.0 + (offset ? offset->x : 0.0)), (float)(start->p.y + (offset ? offset->y : 0.0)) };
					c1 = { (float)(end->p.x + NODE_WIDTH / 2.0 + nArrowSize + 5.0f + (offset ? offset->x : 0.0)), (float)(end->p.y + (offset ? offset->y : 0.0)) };
					t1 = { c1.x - nArrowSize, c1.y };
					t2 = { t1.x + nArrowSize,t1.y + nArrowWidth };
					t3 = { t1.x + nArrowSize,t1.y - nArrowWidth };
				}
				c2 = { (float)((3.0 * c4.x + c1.x) / 4.0),  c1.y };
				c3 = { (float)((3.0 * c1.x + c4.x) / 4.0), c4.y };
			}
			ID2D1PathGeometry* pPathGeometry;
			g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
			if (pPathGeometry) {
				ID2D1GeometrySink* pSink;
				pPathGeometry->Open(&pSink);
				pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
				pSink->BeginFigure(c1, D2D1_FIGURE_BEGIN_FILLED);
				pSink->AddBezier(D2D1::BezierSegment(c2, c3, c4));
				pSink->EndFigure(D2D1_FIGURE_END_OPEN);
				pSink->Close();
				SafeRelease(&pSink);
				g->m_pRenderTarget->DrawGeometry(pPathGeometry, select ? g->m_pSelectBrush : g->m_pNormalBrush, 4.0f);
				SafeRelease(&pPathGeometry);
			}
			g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
			if (pPathGeometry) {
				ID2D1GeometrySink* pSink;
				pPathGeometry->Open(&pSink);
				pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
				pSink->BeginFigure(t1, D2D1_FIGURE_BEGIN_FILLED);
				D2D1_POINT_2F points[] = {t2,t3};
				pSink->AddLines(points, ARRAYSIZE(points));
				pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
				pSink->Close();
				SafeRelease(&pSink);
				g->m_pRenderTarget->FillGeometry(pPathGeometry, select ? g->m_pSelectBrush : g->m_pNormalBrush);
				g->m_pRenderTarget->DrawGeometry(pPathGeometry, select ? g->m_pSelectBrush : g->m_pNormalBrush, 4.0f);
				SafeRelease(&pPathGeometry);
			}
			g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		}
	}
	virtual bool hit(const point* p, UINT64 generation) const override { // 書き直す必要あり
		if (isalive(generation) && start && start->isalive(generation) && end && end->isalive(generation)) {
			D2D1_POINT_2F c1 = {
				(float)(start->p.x),
				(float)(start->p.y)
			};
			D2D1_POINT_2F c2 = {
				(float)(end->p.x),
				(float)(start->p.y)
			};
			D2D1_POINT_2F c3 = {
				(float)(start->p.x),
				(float)(end->p.y)
			};
			D2D1_POINT_2F c4 = {
				(float)(end->p.x),
				(float)(end->p.y)
			};
			ID2D1PathGeometry* pPathGeometry;
			g_c.g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
			ID2D1GeometrySink* pSink;
			pPathGeometry->Open(&pSink);
			pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
			pSink->BeginFigure(c1, D2D1_FIGURE_BEGIN_FILLED);
			pSink->AddBezier(D2D1::BezierSegment(c2, c3, c4));
			pSink->EndFigure(D2D1_FIGURE_END_OPEN);
			pSink->Close();
			BOOL containsPoint = FALSE;
			pPathGeometry->StrokeContainsPoint(
				D2D1::Point2F((FLOAT)p->x, (FLOAT)p->y),
				16,     // stroke width
				NULL,   // stroke style
				NULL,   // world transform
				&containsPoint
			);
			SafeRelease(&pSink);
			SafeRelease(&pPathGeometry);			
			return (bool)containsPoint;
		}
		return false;
	}
	virtual bool inrect(const point* p1, const point* p2, UINT64 generation) const override { // 書き直す必要あり
		if (isalive(generation) && start && start->isalive(generation) && end && end->isalive(generation)) {
			if (
				p1->x <= min(start->p.x, end->p.x) && max(start->p.x, end->p.x) <= p2->x &&
				p1->y <= min(start->p.y, end->p.y) && max(start->p.y, end->p.y) <= p2->y
				) {
				return true;
			}
		}
		return false;
	}
	virtual arrow* copy(UINT64 generation) const {
		arrow* n = new arrow(this, generation);
		return n;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};

class node_output :public node
{
public:
	node_output(UINT64 initborn) : node(initborn) {
		kind = NODE_OUTPUT;
		lstrcpy(name, L"出力");
		pl->description = L"入力値を[出力]に表示します。";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_MULTILINESTRING;
			pi->setname(L"入力値");
			pi->sethelp(L"テキストを入力してください。変数名を入力することも可能です。");
			pi->setdescription(L"入力された文字列を「出力」に表示します。");
			pi->setvalue(L"");
			pl->l.push_back(pi);
		}
	}
	node_output(const node_output* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_output(this, generation);
		return newnode;
	}
	virtual bool execute() const override {
		if (g_c.hOutput && IsWindow(g_c.hOutput)) {
			HWND hEdit = GetTopWindow(g_c.hOutput);
			if (hEdit && pl->l[0]->value && pl->l[0]->value->size() > 0) {
				const int index = GetWindowTextLength(hEdit);
				SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
				SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)pl->l[0]->value->c_str());
				SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
				SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
				return true;
			}
		}
		return false;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};

class node_normal_2 :public node
{
public:
	node_normal_2(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL2;
		for (int i = 0; i < 2; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_SINGLELINESTRING;
			pi->setname(L"名前2");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"2");
			pi->setunit(L"個");
			pi->setmatch(L"[+-]?\\d+");
			pl->l.push_back(pi);
		}
	}
	node_normal_2(const node_normal_2* src, UINT64 initborn) : node(src, initborn) {
	}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_normal_2(this, generation);
		return newnode;
	}
	virtual bool execute() const override {
		return true;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};

class node_normal_3 :public node
{
public:
	node_normal_3(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL3;
		for (int i = 0; i < 3; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_SINGLELINESTRING;
			pi->setname(L"名前3");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"3");
			pi->setunit(L"個");
			pi->setmatch(L"個");
			pl->l.push_back(pi);
		}
	}
	node_normal_3(const node_normal_3* src, UINT64 initborn) : node(src, initborn) {
	}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_normal_3(this, generation);
		return newnode;
	}
	virtual bool execute() const override {
		return true;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};

class node_normal_4 :public node
{
public:
	node_normal_4(UINT64 initborn) : node(initborn) {
		kind = NODE_NORMAL4;
		for (int i = 0; i < 4; i++)
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_SINGLELINESTRING;
			pi->setname(L"名前4");
			pi->sethelp(L"名前");
			pi->setdescription(L"名前");
			pi->setvalue(L"4");
			pi->setunit(L"個");
			pi->setmatch(L"個");
			pl->l.push_back(pi);
		}
	}
	node_normal_4(const node_normal_4* src, UINT64 initborn) : node(src, initborn) {
	}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_normal_4(this, generation);
		return newnode;
	}
	virtual bool execute() const override {
		return true;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};

struct connectpoint {
	object* n;
	point pt;
	CONNECT_POSITION connect_position;
};

class objectlist {
public:
	std::list<object*> l;
	objectlist() {}
	void add(object* n) {
		l.push_back(n);
	}
	void clear() {
		for (auto i : l) {
			delete i;
		}
		l.clear();
	}
	void setbornanddead(UINT64 generation) {
		for (std::list<object*>::iterator it = l.begin(); it != l.end(); ) {
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

		bool drawconnectpoint = (selectcount(generation) == 1);

		for (auto i : l) {
			if (!i->isselect(generation))
				i->paint(g, t, drawconnectpoint, generation);
		}
		for (auto i : l) {
			if (i->isselect(generation))
				i->paint(g, t, drawconnectpoint, generation, dragoffset);
		}
	}
	bool hitconnectpoint(const point* p, connectpoint* out, UINT64 generation, bool all = false) const {
		if (selectcount(generation) != 1) return 0;
		for (auto i = l.rbegin(), e = l.rend(); i != e; ++i) {
			if (!all && !(*i)->isselect(generation)) continue;
			point pt;
			int connectpoint = (*i)->hitconnectpoint(p, &pt, generation);
			if (connectpoint != 0) {
				if (out) {
					out->connect_position = (CONNECT_POSITION)connectpoint;
					out->pt = pt;
					out->n = *i;
				}
				return true;
			}
		}
		return 0;
	}
	object* hit(const point* p, UINT64 generation, const object* without = nullptr) const {
		for (auto i = l.rbegin(), e = l.rend(); i != e; ++i) {
			if (without != *i && (*i)->hit(p, generation))
				return *i;
		}
		return nullptr;
	}
	void select(const object* n, UINT64 generation) { // 指定した一つのみ選択状態にする
		for (auto i : l) {
			i->setselect(i == n, generation);
		}
	}
	void unselect(UINT64 generation) {
		for (auto i : l) {
			i->setselect(false, generation);
		}
	}
	void rectselect(const point* p1, const point* p2, UINT64 generation, std::list<object*>* selectnode) {
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
	bool isselect(const object* n, UINT64 generation) const {
		for (auto i : l) {
			if (i == n) {
				return i->isselect(generation);
			}
		}
		return false;
	}
	void selectlistup(std::list<object*>* selectnode, UINT64 generation) const {
		for (auto i : l) {
			if (i->isselect(generation)) {
				selectnode->push_back(i);
			}
		}
	}
	object* getfirstselectobject(UINT64 generation) const {
		for (auto i : l) {
			if (i->isselect(generation)) {
				return i;
			}
		}
		return 0;
	}
	void selectoffsetmerge(const point* dragoffset, UINT64 generation) {
		std::list<object*> selectnode;
		selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			i->kill(generation);
			object* newnode = i->copy(generation);
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
	void save(UINT64 generation) const {};
	void open(UINT64 generation) const {};
};

enum Mode {
	none = 0,
	dragclient = 1,
	dragnode = 2,
	rectselect = 3,
	rdown = 4,
};

BOOL CALLBACK EnumChildSetFontProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)g_c.hUIFont, TRUE);
	return TRUE;
}

class NoCodeApp {
public:
	trans t;
	objectlist nl;
	Mode mode;
	POINT dragstartP;
	point dragstartL;
	point dragoffset;
	bool dragjudge;
	bool modify;
	object* dd; // ドラッグ中ノード
	HWND hWnd;
	UINT uDpiX, uDpiY;
	object* nn; // 新規作成ノード
	UINT64 generation;
	UINT64 maxgeneration;
	std::list<object*> selectnode;
	LONG_PTR editkind;
	WCHAR szFilePath[512];
	connectpoint cp1 = {};
	connectpoint cp2 = {};


	object* GetFirstSelectObject() const {
		return nl.getfirstselectobject(generation);
	}

	void RefreshToolBar() {

		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_UNDO, CanUndo());
		SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_REDO, CanRedo());
		bool selected = nl.selectcount(generation) > 0;
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
				nl.setbornanddead(generation);
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
		, nl{}
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
		, selectnode{}
		, editkind(-1)
		, szFilePath{}
	{
	}

	void OnCreate(HWND hWnd) {
		this->hWnd = hWnd;
		g_c.g = new graphic(hWnd);
		OnProperty();
		RefreshToolBar();
	}

	void OnOpen(LPCWSTR lpszFilePath = 0) {
		if (lpszFilePath) {
			lstrcpy(szFilePath, lpszFilePath);
		}
		else {
			WCHAR szFilePath[MAX_PATH] = {};
			OPENFILENAME of = {};
			of.lStructSize = sizeof(OPENFILENAME);
			of.hwndOwner = hWnd;
			of.lpstrFilter = L"自動化ファイル (*.ele)\0*.ele\0すべてのファイル(*.*)\0*.*\0\0";
			of.lpstrFile = szFilePath;
			of.nMaxFile = MAX_PATH;
			of.nMaxFileTitle = MAX_PATH;
			of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			of.lpstrTitle = L"ファイルを開く";
			if (GetOpenFileName(&of) == 0)return;
			lstrcpy(this->szFilePath, szFilePath);
		}
	}

	void OnSave(LPCWSTR lpszFilePath = 0) {
		if (lpszFilePath) {
			lstrcpy(szFilePath, lpszFilePath);
		}
		else {
			TCHAR szFilePath[MAX_PATH] = { 0 };
			TCHAR szFileTitle[MAX_PATH] = { 0 };
			OPENFILENAME of = { 0 };
			lstrcpy(szFilePath, TEXT("無題.ele"));
			of.lStructSize = sizeof(OPENFILENAME);
			of.hwndOwner = hWnd;
			of.lpstrFilter = L"自動化ファイル (*.ele)\0*.ele\0すべてのファイル(*.*)\0*.*\0\0";
			of.lpstrFile = szFilePath;
			of.lpstrFileTitle = szFileTitle;
			of.nMaxFile = MAX_PATH;
			of.nMaxFileTitle = MAX_PATH;
			of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			of.lpstrDefExt = TEXT("ele");
			of.lpstrTitle = TEXT("ファイルを保存");
			if (GetSaveFileName(&of) == 0)return;
			lstrcpy(this->szFilePath, szFilePath);
		}

	}

	void OnViewGrid(bool view) {
		g_c.bShowGrid = view;
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnLButtonDown(int x, int y) {
		point p1{ (double)x, (double)y };
		point p2;
		t.d2l(&p1, &p2);
		if (nl.hitconnectpoint(&p2, &cp1, generation))
		{
			MessageBox(hWnd, 0, 0, 0);
		} else if ((dd = nl.hit(&p2, generation)) != nullptr) {
			if (GetKeyState(VK_CONTROL) < 0) {
				dd->setselect(!nl.isselect(dd, generation), generation); // Ctrlを押されているときは選択を追加
			}
			else if (!dd->isselect(generation)/*!l.isselect(dd, generation)*/){
				nl.select(dd, generation); // 選択されていないときは1つ選択する
			}
			OnProperty();
			dragstartP = { x, y };
			dragstartL = p2;
			mode = dragnode;
		}
		else {
			//矩形選択モード
			if (GetKeyState(VK_CONTROL) < 0) {
				nl.selectlistup(&selectnode, generation);
			}
			else {
				nl.unselect(generation);
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
				nl.selectoffsetmerge(&dragoffset, generation);
			}
			else if (dd && dd->isselect(generation))
			{
				if (GetKeyState(VK_CONTROL) < 0) {} else
				{
					nl.select(dd, generation);
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
		dd = nl.hit(&p2, generation);
		if (dd) {
			if (!nl.isselect(dd, generation)) {
				nl.select(dd, generation); // 選択されていないときは1つ選択する
				OnProperty();
			}
			dragstartP = { x, y };
			dragstartL = p2;
		}
		else {
			nl.unselect(generation);
			OnProperty();
		}
		mode = rdown;
		InvalidateRect(hWnd, NULL, FALSE);
		SetCapture(hWnd);
	}

	void OnRButtonUp(int x, int y) {
		ReleaseCapture();
		if (mode == rdown) {
			if (dragjudge) {
				dragjudge = false;
				beginedit();
				point p1{ (double)x, (double)y };
				point p2;
				t.d2l(&p1, &p2);
				object* dd2 = nl.hit(&p2, generation);
				if (dd != dd2) {
					beginedit();
					nl.unselect(generation);
					arrow* aa = new arrow(generation);
					aa->start = (node*)dd;
					aa->end = (node*)dd2;
					aa->select = true;
					nl.add(aa);
					InvalidateRect(hWnd, NULL, FALSE);
				}
				dragoffset = { 0.0, 0.0 };
			}
			else
			{
				POINT point = { x, y };
				ClientToScreen(hWnd, &point);
				HMENU hMenu;
				if (nl.selectcount(generation) > 0) {
					hMenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_NODEPOPUP));
				}
				else {
					hMenu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_CLIENTPOPUP));
				}
				HMENU hSubMenu = GetSubMenu(hMenu, 0);
				TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, point.x, point.y, 0, hWnd, NULL);
			}
		}
		mode = none;
	}

	void OnMouseMove(int x, int y) {
		if (mode == rdown) {
			point p1{ (double)x, (double)y };
			point p2;
			t.d2l(&p1, &p2);
			dragoffset.x = p2.x - dragstartL.x;
			dragoffset.y = p2.y - dragstartL.y;
			if (abs(p1.x - dragstartP.x) >= DRAGJUDGEWIDTH || abs(p1.y - dragstartP.y) >= DRAGJUDGEWIDTH)
			{
				dragjudge = true;
			}
		}
		else if (mode == dragclient) {
			point p1{ (double)x, (double)y };
			t.l.x = dragstartL.x - (p1.x - dragstartP.x) / t.z;
			t.l.y = dragstartL.y - (p1.y - dragstartP.y) / t.z;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (mode == dragnode) {
			point p1{ (double)x, (double)y };
			point p2;
			t.d2l(&p1, &p2);
			if (g_c.bShowGrid) {
				p2.x = ((int)(p2.x- dragstartL.x) / GRID_WIDTH) * GRID_WIDTH + dragstartL.x;
				p2.y = ((int)(p2.y- dragstartL.y) / GRID_WIDTH) * GRID_WIDTH + dragstartL.y;
			}
			dragoffset.x = p2.x - dragstartL.x;
			dragoffset.y = p2.y - dragstartL.y;
			if (abs(p1.x - dragstartP.x) >= DRAGJUDGEWIDTH || abs(p1.y - dragstartP.y) >= DRAGJUDGEWIDTH)
			{
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
			nl.rectselect(&p3, &p4, generation, &selectnode);
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
		nl.clear();
		delete g_c.g;
		g_c.g = nullptr;
	}

	void OnPaint() {
		if (g_c.g) {
			if (g_c.g->begindraw(hWnd)) {
				g_c.g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				g_c.g->m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
				if (g_c.bShowGrid) {
					drawgrid(g_c.g, &t);
				}
				nl.paint(g_c.g, &t, generation, mode == dragnode ? &dragoffset : nullptr);
				if (nn) nn->paint(g_c.g, &t, false, generation);
				g_c.g->enddraw();
			}
		}
		ValidateRect(hWnd, NULL);
	}

	void drawgrid(const graphic* g, const trans* t) {
		g->m_pRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
			D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
		);
		point p1;
		t->d2l(&t->p, &p1);
		point p2 = t->p;
		p2.x += t->c.w;
		p2.y += t->c.h;
		point p3;
		t->d2l(&p2, &p3);
		for (int x = ((int)p1.x / GRID_WIDTH) * GRID_WIDTH; x <= ((int)p3.x / GRID_WIDTH) * GRID_WIDTH; x += GRID_WIDTH)
		{
			D2D1_POINT_2F s = { (float)x, (float)p1.y };
			D2D1_POINT_2F e = { (float)x, (float)p3.y };
			g->m_pRenderTarget->DrawLine(s, e, g->m_pGridBrush, 0.1f);
		}
		for (int y = ((int)p1.y / GRID_WIDTH) * GRID_WIDTH; y <= ((int)p3.y / GRID_WIDTH) * GRID_WIDTH; y += GRID_WIDTH)
		{
			D2D1_POINT_2F s = { (float)p1.x, (float)y };
			D2D1_POINT_2F e = { (float)p3.x, (float)y };
			g->m_pRenderTarget->DrawLine(s, e, g->m_pGridBrush, 0.1f);
		}
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	}

	void OnSize(int x, int y, int w, int h) {
		t.p.x = x;
		t.p.y = y;
		t.c.w = w;
		t.c.h = h;
		if (g_c.g && g_c.g->m_pRenderTarget)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			D2D1_SIZE_U size = { (UINT32)rect.right, (UINT32)rect.bottom };
			g_c.g->m_pRenderTarget->Resize(size);
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
			nl.unselect(generation);
			nn->born = generation;
			if (g_c.bShowGrid) {
				nn->p.x = ((int)(nn->p.x) / GRID_WIDTH) * GRID_WIDTH;
				nn->p.y = ((int)(nn->p.y) / GRID_WIDTH) * GRID_WIDTH;
			}
			nl.add(nn);
			nn = nullptr;
			OnProperty();
			RefreshToolBar();
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}

	void OnHome() {
		point p1, p2;
		nl.allnodemargin(&p1, &p2, generation);
		t.settransfromrect(&p1, &p2, POINT2PIXEL(10));
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnDelete() {
		beginedit();
		nl.del(generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnUnselect() {
		nl.unselect(generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnAllselect() {
		nl.allselect(generation);
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
		nl.clear();
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

	void OnRun() {
		if (nl.selectcount(generation) == 1){
			object* first = nl.getfirstselectobject(generation);
			if (first && NODE_NONE <= first->kind && first->kind <= NODE_MULTI) {
				((node*)first)->execute();
			}
		}
	}

	BOOL IsHighlightText(LPCWSTR lpszText)
	{
		LPCWSTR lpszHighlightText[] = {
			L"あいうえお",
			L"hello",
			L"aaa",
			L"abc",
			L"book",
			L"apple"
		};
		for (auto i : lpszHighlightText) {
			if (lstrcmpi(lpszText, i) == 0) {
				return TRUE;
			}
		}
		return FALSE;
	}

	static INT_PTR CALLBACK DialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
	{
		static NoCodeApp* pNoCodeApp;
		static bool enable_edit_event;
		switch (msg)
		{
		case WM_INITDIALOG:
			enable_edit_event = false;
			pNoCodeApp = (NoCodeApp*)lParam;
			if (pNoCodeApp) {
				RECT rect;
				GetClientRect(GetParent(hDlg), &rect);
				MoveWindow(hDlg, 0, 0, rect.right, rect.bottom, TRUE);
				object* n = pNoCodeApp->GetFirstSelectObject();
				if (n && n->pl) {
					CreateWindowEx(0, L"STATIC", n->pl->description.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, POINT2PIXEL(256), POINT2PIXEL(64), hDlg, 0, GetModuleHandle(0), 0);
					int nYtop = POINT2PIXEL(64);
					LONG_PTR index = 0;
					for (auto i : n->pl->l) {
						switch (i->kind) {
						case PROPERTY_NONE:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_INT:
							CreateWindowEx(0, L"STATIC", (i->name) ? ((*i->name + L":").c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, 0, nYtop, POINT2PIXEL(64), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", (i->value) ? (i->value->c_str()) : 0, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_WANTRETURN, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
							CreateWindowEx(0, L"STATIC", (i->unit) ? (i->unit->c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, POINT2PIXEL(64 + 128 + 32), nYtop, POINT2PIXEL(32), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_CHECK:
							CreateWindowEx(0, L"BUTTON", (i->name) ? (i->name->c_str()) : 0, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, POINT2PIXEL(8), nYtop, POINT2PIXEL(256)- POINT2PIXEL(8), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							if (i->value) {
								if (i->value->_Equal(L"0")) {

								}
								else
								{

								}
							}
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_SINGLELINESTRING:
							CreateWindowEx(0, L"STATIC", (i->name) ? ((*i->name + L":").c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, 0, nYtop, POINT2PIXEL(64), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							{
								HWND hEdit = CreateWindowEx(WS_EX_STATICEDGE, RICHEDIT_CLASSW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL| ES_NOHIDESEL, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
								SendMessage(hEdit, EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT);
								SendMessage(hEdit, EM_SETEVENTMASK, 0, ENM_UPDATE);
								SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
								DefaultRichEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
								SetWindowText(hEdit, (i->value) ? (i->value->c_str()) : 0);
							}
							CreateWindowEx(0, L"STATIC", (i->unit) ? (i->unit->c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, POINT2PIXEL(64 + 128 + 32), nYtop, POINT2PIXEL(32), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_MULTILINESTRING:
							CreateWindowEx(0, L"STATIC", (i->name) ? ((*i->name + L":").c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, 0, nYtop, POINT2PIXEL(64), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							{
								HWND hEdit = CreateWindowEx(WS_EX_STATICEDGE, RICHEDIT_CLASSW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL, POINT2PIXEL(4), nYtop + POINT2PIXEL(28), POINT2PIXEL(256 - 8), POINT2PIXEL(28 + 256) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
								SendMessage(hEdit, EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT);
								SendMessage(hEdit, EM_SETEVENTMASK, 0, ENM_UPDATE);
								SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
								DefaultRichEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
								SetWindowText(hEdit, (i->value) ? (i->value->c_str()) : 0);
							}
							nYtop += POINT2PIXEL(32+256+25);
							break;
						case PROPERTY_DATE:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_TIME:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_IP:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_PICTURE:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_COLOR:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_SELECT:
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_CUSTOM:
							nYtop += POINT2PIXEL(32);
							break;
						}
						index++;
					}
				}
			}
			enable_edit_event = true;
			EnumChildWindows(hDlg, EnumChildSetFontProc, 0);
			return TRUE;
		case WM_APP:
			if (pNoCodeApp) {
				if (HIWORD(wParam) == EN_UPDATE) {
					object* n = pNoCodeApp->GetFirstSelectObject();
					if (n && n->pl) {
						int nIndex = LOWORD(wParam) - 1000;
						if (0 <= nIndex && nIndex < (int)n->pl->l.size()) {
							DWORD dwSize = GetWindowTextLength((HWND)lParam);
							LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, (dwSize + 1) * sizeof(WCHAR));
							GetWindowText((HWND)lParam, lpszText, dwSize + 1);
							if (!n->pl->l[nIndex]->match || std::regex_match(lpszText, std::wregex(*n->pl->l[nIndex]->match))) {
								propertyitem* pi = n->pl->l[nIndex]->copy();
								*(pi->value) = lpszText;
								pNoCodeApp->UpdateSelectNode(nIndex, pi, (LONG_PTR)lParam); // とりあえずこの項目についてユニークな値が欲しい。
								delete pi;
							}
							else if (n->pl->l[nIndex]->help) {
								EDITBALLOONTIP balloonTip = {};
								balloonTip.cbStruct = sizeof(EDITBALLOONTIP);
								balloonTip.pszText = n->pl->l[nIndex]->help->c_str();
								balloonTip.pszTitle = L"入力エラー";
								balloonTip.ttiIcon = TTI_ERROR;
								Edit_ShowBalloonTip((HWND)lParam, &balloonTip);
							}
							GlobalFree(lpszText);
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
		std::list<object*> selectnode;
		nl.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			object* newnode = i;
			if (firstedit) {
				newnode->kill(generation);
				newnode = i->copy(generation);
			}
			newnode->setpropertyvalue(index, pi);
			if (firstedit) {
				nl.add(newnode);
			}
		}
	}

	void OnProperty() {
		node* n = 0;
		switch (GetSelectKind(n)) {
		case NODE_NONE:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NONE), g_c.hPropContainer, DialogProc, (LPARAM)0);
			break;
		case NODE_OUTPUT:
		case NODE_NORMAL2:
		case NODE_NORMAL3:
		case NODE_NORMAL4:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, DialogProc, (LPARAM)this);
			break;
		case NODE_MULTI:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_MULTI), g_c.hPropContainer, DialogProc, (LPARAM)0);
			break;
		}
		SetFocus(g_c.hPropContainer);
	}

	OBJECT_KIND GetSelectKind(object *n) const {
		const int selcount = nl.selectcount(generation);
		if (selcount > 1)
			return NODE_MULTI;
		if (selcount == 0)
			return NODE_NONE;
		std::list<object*> selectnode;
		nl.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			n = i;
			return i->kind;
		}
		return NODE_NONE;
	}
};

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CHAR:
		if (wParam == '[')
		{
			int nTextHeight = 0;
			{
				HDC hdc = GetDC(hWnd);
				SIZE size = {};
				HFONT hOldFont = (HFONT)SelectObject(hdc, g_c.hUIFont);
				GetTextExtentPoint32(hdc, L"[", 1, &size);
				SelectObject(hdc, hOldFont);
				nTextHeight = size.cy;
				ReleaseDC(hWnd, hdc);
			}
			if (nTextHeight)
			{
				POINT point;
				GetCaretPos(&point);
				ClientToScreen(hWnd, &point);
				SetWindowLongPtr(g_c.hList, GWLP_USERDATA, (LONG_PTR)hWnd);
				{
					const int nItemHeight = (int)SendMessage(g_c.hList, LB_GETITEMHEIGHT, 0, 0);
					const int nItemCount = min((int)SendMessage(g_c.hList, LB_GETCOUNT, 0, 0), 16);
					SetWindowPos(g_c.hList, 0, point.x, point.y + nTextHeight, POINT2PIXEL(256), nItemHeight * nItemCount + GetSystemMetrics(SM_CYEDGE) * 2, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				SendMessage(g_c.hList, LB_SETCURSEL, 0, 0);
				ShowWindow(g_c.hList, SW_SHOW);
			}
		}
		else {
			ShowWindow(g_c.hList, SW_HIDE);
		}
		{
			const LONG_PTR ret = CallWindowProc(DefaultRichEditProc, hWnd, msg, wParam, lParam);
			HWND hParent = GetParent(hWnd);
			if (hParent) {
				int id = GetWindowLong(hWnd, GWL_ID);
				SendMessage(hParent, WM_APP, MAKEWPARAM(id, EN_UPDATE), (LPARAM)hWnd);
			}
			return ret;
		}
		break;
	case WM_KILLFOCUS:
		if ((HWND)wParam != g_c.hList) {
			ShowWindow(g_c.hList, SW_HIDE);
		}
		break;
	case WM_MOUSEWHEEL:
		if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
			return 0;
		}
		break;
	case WM_PAINT:
		{
			HideCaret(hWnd);
			CallWindowProc(DefaultRichEditProc, hWnd, msg, wParam, lParam);
			RECT rect;
			SendMessage(hWnd, EM_GETRECT, 0, (LPARAM)&rect);
			LONG_PTR eax = SendMessage(hWnd, EM_CHARFROMPOS, 0, (LPARAM)&rect);
			eax = SendMessage(hWnd, EM_LINEFROMCHAR, eax, 0);
			TEXTRANGE txtrange = {};
			txtrange.chrg.cpMin = (LONG)SendMessage(hWnd, EM_LINEINDEX, eax, 0);
			txtrange.chrg.cpMax = (LONG)SendMessage(hWnd, EM_CHARFROMPOS, 0, (LPARAM)&rect.right);
			LONG_PTR FirstChar = txtrange.chrg.cpMin;
			HRGN hRgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
			HDC hdc = GetDC(hWnd);
			SetBkMode(hdc, TRANSPARENT);
			HRGN hOldRgn = (HRGN)SelectObject(hdc, hRgn);
			DWORD dwTextSize = txtrange.chrg.cpMax - txtrange.chrg.cpMin;
			if (dwTextSize > 0) {
				LPWSTR pBuffer = (LPWSTR)GlobalAlloc(0, (dwTextSize + 1) * sizeof(WCHAR));
				if (pBuffer) {
					txtrange.lpstrText = pBuffer;
					if (SendMessage(hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&txtrange) > 0) {
						LPWSTR lpszStart = wcsstr(txtrange.lpstrText, L"[");
						while (lpszStart) {
							while (*(lpszStart + 1) == L'[') {
								lpszStart++;
							}
							LPWSTR lpszEnd = wcsstr(lpszStart, L"]");
							if (lpszEnd) {
								const int nTextLen = (int)(lpszEnd - lpszStart);
								LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, nTextLen * sizeof(WCHAR));
								if (lpszText) {
									(void)lstrcpyn(lpszText, lpszStart + 1, nTextLen);
									if (g_c.app && ((NoCodeApp*)g_c.app)->IsHighlightText(lpszText)) {
										SendMessage(hWnd, EM_POSFROMCHAR, (WPARAM)&rect, lpszStart - txtrange.lpstrText + FirstChar);
										COLORREF colOld = SetTextColor(hdc, RGB(128, 128, 255));
										HFONT hOldFont = (HFONT)SelectObject(hdc, g_c.hUIFont);
										DrawText(hdc, lpszStart, nTextLen + 1, &rect, 0);
										SelectObject(hdc, hOldFont);
									}
									GlobalFree(lpszText);
								}
							}
							else {
								break;
							}
							lpszStart = wcsstr(lpszEnd, L"[");
						}
					}
					GlobalFree(pBuffer);
				}
			}
			SelectObject(hdc, hOldRgn);
			DeleteObject(hRgn);
			ReleaseDC(hWnd, hdc);
			ShowCaret(hWnd);
			return 0;
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultRichEditProc, hWnd, msg, wParam, lParam);
}

WNDPROC DefaultListWndProc;
LRESULT CALLBACK ListProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
			POINT point;
			GetCursorPos(&point);
			if (WindowFromPoint(point) == hWnd) {
				SendMessage(hWnd, WM_CHAR, VK_RETURN, 0);
			}
		}
		return 0;
	case WM_CHAR:
		if (wParam == VK_RETURN) {
			const int nIndex = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
			if (nIndex != LB_ERR) {
				const int nTextLength = (int)SendMessage(hWnd, LB_GETTEXTLEN, nIndex, 0);
				if (nTextLength != LB_ERR) {
					LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, (nTextLength + 1) * sizeof(WCHAR));
					SendMessage(hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszText);
					HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					if (hEdit) {
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)lpszText);
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"]");
					}
					GlobalFree(lpszText);
				}
			}
			ShowWindow(hWnd, SW_HIDE);
		}
		else if (wParam == VK_ESCAPE) {
			ShowWindow(hWnd, SW_HIDE);
		}
		else {
			return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
		}
		break;
	case WM_KILLFOCUS:
		ShowWindow(hWnd, SW_HIDE);
		{
			HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (hEdit && IsWindow(hEdit) && IsWindowVisible(hEdit)) {
				SetFocus(hEdit);
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
}

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
		break;
	}
	return 0;
}

INT_PTR CALLBACK PropContainerDialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_SIZE:
	{
		HWND hWnd = GetTopWindow(hDlg);
		if (hWnd) {
			MoveWindow(hWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
	}
	break;
	}
	return FALSE;
}

INT_PTR CALLBACK NodeBoxDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static NoCodeApp* pNoCodeApp;
	static HWND hTree;
	static int flag;
	static HIMAGELIST hImageList;
	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (NoCodeApp*)lParam;
		{
			hTree = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, 0, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_LINESATROOT, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);

			hImageList = ImageList_Create(32, 32, ILC_COLOR16, 4, 10);
			HBITMAP hBitMap = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP2));
			ImageList_Add(hImageList, hBitMap, NULL);
			DeleteObject(hBitMap);
			TreeView_SetImageList(hTree, hImageList, TVSIL_NORMAL); 

			HTREEITEM hRootItem = 0;
			{
				SetFocus(hTree);
				TV_INSERTSTRUCT tv = { 0 };
				tv.hParent = TVI_ROOT;
				tv.hInsertAfter = TVI_LAST;
				tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tv.item.pszText = TEXT("Root");
				tv.item.iImage = 0;
				tv.item.iSelectedImage = 1;
				hRootItem = TreeView_InsertItem(hTree, &tv);
			}
			// 子ノード1
			if (hRootItem) {
				node* n = new node_output(0);
				if (n) {
					TV_INSERTSTRUCT tv = { 0 };
					tv.hParent = hRootItem;
					tv.hInsertAfter = TVI_LAST;
					tv.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tv.item.pszText = n->name;
					tv.item.iImage = 2;
					tv.item.iSelectedImage = 3;
					tv.item.lParam = (LPARAM)n;
					HTREEITEM hItem = TreeView_InsertItem(hTree, &tv);
				}
			}
		}
		EnumChildWindows(hWnd, EnumChildSetFontProc, 0);
		return TRUE;
	case WM_NOTIFY:
		{
			LPNMHDR lpNmhdr = (LPNMHDR)lParam;
			if (lpNmhdr->code == TVN_BEGINDRAG) {
				NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)lParam;
				HTREEITEM item = pnmtv->itemNew.hItem;
				TreeView_SelectItem(hTree, item);
				if (pNoCodeApp) {
					node* n = (node*)pnmtv->itemNew.lParam;
					if (n) {
						node* nn = n->copy(0);
						nn->setselect(true, 0);
						pNoCodeApp->OnBeginDrag(nn);
						flag = 1;
						SetCapture(hWnd);
					}
				}
			} else if (lpNmhdr->code == TVN_DELETEITEM) {
				LPTVITEM lp = &((LPNMTREEVIEW)lParam)->itemOld;
				node* n = (node*)lp->lParam;
				delete n;
			}
		}
		break;
	case WM_MOUSEMOVE:
		if (flag) {
			if (pNoCodeApp) {
				POINT point;
				GetCursorPos(&point);
				ScreenToClient(g_c.hMainWnd, &point);
				pNoCodeApp->OnDragging(point.x, point.y);
			}
		}
		break;
	case WM_LBUTTONUP:
		if (flag == 1) {
			ReleaseCapture();
			flag = 0;
			if (pNoCodeApp) {
				POINT point;
				GetCursorPos(&point);
				if (WindowFromPoint(point) == g_c.hMainWnd) {
					pNoCodeApp->OnDropped();
				}
				else {
					pNoCodeApp->OnCancelDrag();
				}
			}
		}
		break;
	case WM_SIZE:
		if (hTree) {
			MoveWindow(hTree, POINT2PIXEL(4), POINT2PIXEL(4), LOWORD(lParam) - POINT2PIXEL(8), HIWORD(lParam) - POINT2PIXEL(8), TRUE);
		}
		break;
	case WM_DESTROY:
		if (hTree) {
			TreeView_DeleteAllItems(hTree);
		}
		ImageList_Destroy(hImageList);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK OutputDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static NoCodeApp* pNoCodeApp;
	static HWND hEdit;
	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (NoCodeApp*)lParam;
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);
		SendMessage(hEdit, EM_LIMITTEXT, 0, 0);
		EnumChildWindows(hWnd, EnumChildSetFontProc, 0);
		return TRUE;
	case WM_SIZE:
		MoveWindow(hEdit, POINT2PIXEL(4), POINT2PIXEL(4), LOWORD(lParam) - POINT2PIXEL(8), HIWORD(lParam) - POINT2PIXEL(8), TRUE);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK AboutDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK SettingsDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK VariableListDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static NoCodeApp* pNoCodeApp;
	static HWND hList;
	static HWND hEdit;
	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (NoCodeApp*)lParam;
		{
			hList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 0, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS, 0, 0, 0, 0, hWnd, (HMENU)1000, GetModuleHandle(0), NULL);
			LV_COLUMN lvcol;
			LV_ITEM item;
			lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvcol.fmt = LVCFMT_LEFT;
			lvcol.cx = 128;
			lvcol.pszText = TEXT("変数名");
			lvcol.iSubItem = 0;
			SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvcol);
			lvcol.fmt = LVCFMT_LEFT;
			lvcol.cx = 128;
			lvcol.pszText = TEXT("値");
			lvcol.iSubItem = 1;
			SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvcol);
			item.mask = LVIF_TEXT;
			{
				TCHAR szText[50];
				for (int i = 0; i < 128; i++)
				{
					wsprintf(szText, TEXT("アイテム %d"), i);
					item.pszText = szText;
					item.iItem = i;
					item.iSubItem = 0;
					item.iImage = i % 4;
					SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&item);
					wsprintf(szText, TEXT("%d"), i);
					item.pszText = szText;
					item.iItem = i;
					item.iSubItem = 1;
					SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&item);
				}
			}
			ListView_SetExtendedListViewStyle(hList, LVS_EX_TWOCLICKACTIVATE);
		}
		EnumChildWindows(hWnd, EnumChildSetFontProc, 0);
		return TRUE;
	case WM_NOTIFY:
		if (LOWORD(wParam) == 1000)
		{
			switch (((LV_DISPINFO*)lParam)->hdr.code)
			{
			case LVN_KEYDOWN:
				if (((NMLVKEYDOWN*)lParam)->wVKey == VK_F2)
				{
					ListView_EditLabel(hList, ListView_GetNextItem(hList, -1, LVNI_FOCUSED));
				}
				break;
			case LVN_ITEMACTIVATE:
				ListView_EditLabel(hList, ListView_GetNextItem(hList, -1, LVNI_FOCUSED));
				break;
			case LVN_BEGINLABELEDIT:    //アイテム編集を開始
				hEdit = ListView_GetEditControl(hList);
				break;
			case LVN_ENDLABELEDIT:
				{
					int nTextLength = GetWindowTextLength(hEdit);
					if (nTextLength) {
						LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (nTextLength + 1));
						GetWindowText(hEdit, lpszText, nTextLength + 1);
						ListView_SetItemText(hList, ((LV_DISPINFO*)lParam)->item.iItem, 0, lpszText);
						GlobalFree(lpszText);
					}
				}
				break;
			}
		}
		break;
	case WM_SIZE:
		MoveWindow(hList, POINT2PIXEL(4), POINT2PIXEL(4), LOWORD(lParam) - POINT2PIXEL(8), HIWORD(lParam) - POINT2PIXEL(8), TRUE);
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static NoCodeApp* pNoCodeApp;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		LoadLibrary(L"riched20");
		pNoCodeApp = new NoCodeApp();
		g_c.app = (void*)pNoCodeApp;
		if (pNoCodeApp) {
			pNoCodeApp->OnCreate(hWnd);
		}
		SendMessage(hWnd, WM_APP, 0, 0);

		g_c.hList = CreateWindowEx(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_NOACTIVATE, L"LISTBOX", NULL, WS_POPUP | WS_VSCROLL, 0, 0, 0, 0, 0, 0, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		SendMessage(g_c.hList, WM_SETFONT, (WPARAM)g_c.hUIFont, 0);
		DefaultListWndProc = (WNDPROC)SetWindowLongPtr(g_c.hList, GWLP_WNDPROC, (LONG_PTR)ListProc1);
		{
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"abc");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"def");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"ghi");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"jkl");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"mno");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"pqr");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"stu");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"vwx");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"yz");
		}

		g_c.hNodeBox = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_NODEBOX), hWnd, NodeBoxDialogProc, (LPARAM)pNoCodeApp);
		g_c.hOutput = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_OUTPUT), hWnd, OutputDialogProc, (LPARAM)pNoCodeApp);
		g_c.hVariableList = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_VARIABLE_LIST), hWnd, VariableListDialogProc, (LPARAM)pNoCodeApp);

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
			g_c.hTool = CreateToolbarEx(hWnd, WS_CHILD | WS_VISIBLE | WS_BORDER | TBSTYLE_FLAT, 0, _countof(tbb), ((LPCREATESTRUCT)lParam)->hInstance, IDR_TOOLBAR1, tbb, _countof(tbb), 0, 0, 64, 64, sizeof(TBBUTTON));
			//LONG_PTR lStyle = GetWindowLongPtr(g_c.hTool, GWL_STYLE);
			//lStyle = (lStyle | TBSTYLE_FLAT) & ~TBSTYLE_TRANSPARENT;
			//SetWindowLongPtr(g_c.hTool, GWL_STYLE, lStyle);
		}

		g_c.hPropContainer = CreateDialog(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_PROPERTY), hWnd, PropContainerDialogProc);

		{
			// メニューを設定する
			HMENU hMenu = GetMenu(hWnd);
			if (hMenu) {
				if (g_c.bShowGrid) {
					CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_CHECKED);
				}
			}
		}

		break;
	case WM_APP: // 画面DPIが変わった時、ウィンドウ作成時にフォントの生成を行う
		GetScaling(hWnd, &g_c.uDpiX, &g_c.uDpiY);
		DeleteObject(g_c.hUIFont);
		g_c.hUIFont = CreateFontW(-POINT2PIXEL(FONT_SIZE), 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, FONT_NAME);
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
			MoveWindow(g_c.hNodeBox, 0, rect.bottom - rect.top, POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			MoveWindow(g_c.hOutput, POINT2PIXEL(256), HIWORD(lParam) - POINT2PIXEL(96), LOWORD(lParam) - POINT2PIXEL(256) - POINT2PIXEL(256) - POINT2PIXEL(200), POINT2PIXEL(96), 1);
			MoveWindow(g_c.hPropContainer, LOWORD(lParam) - POINT2PIXEL(256), rect.bottom - rect.top, POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			MoveWindow(g_c.hVariableList, LOWORD(lParam) - POINT2PIXEL(256)- POINT2PIXEL(200), rect.bottom - rect.top, POINT2PIXEL(200), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			if (pNoCodeApp) {
				pNoCodeApp->OnSize(POINT2PIXEL(256), rect.bottom - rect.top, LOWORD(lParam) - POINT2PIXEL(256) - POINT2PIXEL(256) - POINT2PIXEL(200), HIWORD(lParam) - (rect.bottom - rect.top) - POINT2PIXEL(96));
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
		case ID_RUN:
			if (pNoCodeApp) {
				pNoCodeApp->OnRun();
			}
			break;
		case ID_OUTPUT_CLEAR:
			if (g_c.hOutput) {
				HWND hEdit = GetTopWindow(g_c.hOutput);
				if (hEdit) {
					SetWindowText(hEdit, 0);
				}
			}
			break;
		case ID_ABOUT:
			DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDialogProc);
			break;
		case ID_SETTINGS:
			DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDialogProc);
			break;
		case ID_FEEDBACK:
			ShellExecute(NULL, L"open", L"https://forms.gle/vAFimNY6nWnx6DCDA", NULL, NULL, SW_SHOWNORMAL);
			break;
		case ID_OPEN:
			if (pNoCodeApp) {
				pNoCodeApp->OnOpen();
			}
			break;
		case ID_SAVE:
			if (pNoCodeApp) {
				pNoCodeApp->OnSave();
			}
			break;
		case ID_VIEW_GRID:
			{
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu) {
					g_c.bShowGrid = !g_c.bShowGrid;
					if (g_c.bShowGrid) {
						CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_CHECKED);
					}
					else {
						CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_UNCHECKED);
					}
					if (pNoCodeApp) {
						pNoCodeApp->OnViewGrid(g_c.bShowGrid);
					}
				}
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
			const HMODULE hModUser32 = GetModuleHandleW(L"user32");
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
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
	g_c.hMainWnd = CreateWindowW(
		szClassName,
		L"nocode",
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
	ShowWindow(g_c.hMainWnd, SW_SHOWDEFAULT);
	UpdateWindow(g_c.hMainWnd);
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
		{FVIRTKEY | FCONTROL, VK_F5, ID_RUN},
	};
	HACCEL hAccel = CreateAcceleratorTable(Accel, _countof(Accel));
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(g_c.hMainWnd, &msg) && !TranslateAccelerator(g_c.hMainWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	CoUninitialize();
	return (int)msg.wParam;
}
