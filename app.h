#pragma once

#include <windows.h>
#include <commctrl.h>
#include <Richedit.h>
#include <InitGuid.h>
#include <regex>
#include "trans.h"
#include "objectlist.h"
#include "mode.h"
#include "object.h"
#include "arrow.h"
#include "node.h"
#include "common.h"
#include "util.h"
#include "resource.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define ZOOM_MAX 100.0
#define ZOOM_MIN 0.1
#define ZOOM_STEP 1.20
#define POINT2PIXEL(PT) MulDiv(PT, g_c.uDpiY, 72)
#define DRAGJUDGEWIDTH POINT2PIXEL(4)
#define GRID_WIDTH 25

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class app {
public:
	trans t;
	objectlist nl;
	Mode mode;
	POINT dragstartP;
	point dragstartL;
	point dragoffset;
	bool dragjudge;
	object* dd; // ドラッグ中ノード
	HWND hWnd;
	UINT uDpiX, uDpiY;
	object* nn; // 新規作成ノード
	UINT64 generation;
	UINT64 maxgeneration;
	std::vector<object*> selectnode;
	LONG_PTR editkind;
	WCHAR szFilePath[512];
	connectpoint cp1 = {};
	connectpoint cp2 = {};
	point pt1;
	point pt2;
	bool bIsRunning;
	bool bForceExit;
	bool bIsSuspend;
	HANDLE hThread;

	object* GetFirstSelectObject() const {
		return nl.getfirstselectobject(generation);
	}

	void RefreshToolBar() {
		if (bIsRunning)
		{
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_NEW, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_OPEN, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SAVE, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_EXIT, FALSE);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_UNDO, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_REDO, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_COPY, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_CUT, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_PASTE, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_DELETE, FALSE);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_RUN, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SUSPEND, TRUE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_STOP, TRUE);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_ZOOM, ZOOM_MAX > t.z);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SHRINK, ZOOM_MIN < t.z);
		}
		else
		{
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_NEW,  TRUE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_OPEN, TRUE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SAVE, TRUE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_EXIT, TRUE);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_UNDO, CanUndo());
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_REDO, CanRedo());
			bool selected = nl.selectcount(generation) > 0;
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_COPY, selected);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_CUT, selected);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_PASTE, selected);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_DELETE, selected);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_RUN, TRUE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SUSPEND, FALSE);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_STOP, FALSE);

			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_ZOOM, ZOOM_MAX > t.z);
			SendMessage(g_c.hTool, TB_ENABLEBUTTON, ID_SHRINK, ZOOM_MIN < t.z);
		}
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

	app()
		: t{}
		, nl{}
		, mode(none)
		, dragstartP{}
		, dragstartL{}
		, dragoffset{}
		, dragjudge(false)
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
		, bIsRunning(false)
		, bForceExit(false)
		, bIsSuspend(false)
		, hThread(0)
	{
	}

	~app()
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
			pt1 = cp1.pt;
			pt2 = cp1.pt;
			OnProperty();
			mode = connection;
		}
		else if ((dd = nl.hit(g_c.g, &p2, generation)) != nullptr) {
			if (GetKeyState(VK_CONTROL) < 0) {
				dd->setselect(!nl.isselect(dd, generation), generation); // Ctrlを押されているときは選択を追加
			}
			else if (!dd->isselect(generation)/*!l.isselect(dd, generation)*/) {
				nl.select(dd, generation); // 選択されていないときは1つ選択する
			}
			RefreshToolBar();
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
		ReleaseCapture();
		if (mode == dragnode) {
			if (dragjudge) {
				dragjudge = false;
				beginedit();
				point p1{ (double)x, (double)y };
				point p2;
				t.d2l(&p1, &p2);
				nl.selectoffsetmerge(g_c.g, &p2, &dragoffset, generation);
			}
			else if (dd && dd->isselect(generation))
			{
				if (GetKeyState(VK_CONTROL) < 0) {}
				else
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
		else if (mode == connection)
		{
			point p1{ (double)x, (double)y };
			point p2;
			t.d2l(&p1, &p2);
			if ((dd = nl.hit(g_c.g, &p2, generation)) != nullptr && dd->getobjectkind() != OBJECT_ARROW && dd != cp1.n)
			{
				bool bExistSameArrow = false;
				for (auto i : nl.l)
				{
					if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW)
					{
						arrow* a = (arrow*)i;
						if (a->start == (node*)cp1.n &&
							a->end == (node*)dd)
						{
							bExistSameArrow = true;
							break;
						}
					}
				}
				if (!bExistSameArrow) {
					beginedit();
					nl.unselect(generation);
					arrow* newarrow = new arrow(generation);
					newarrow->start = (node*)cp1.n;
					newarrow->end = (node*)dd;
					objectlist::getconnectpoint(&newarrow->start->p, &newarrow->end->p, &newarrow->start_pos, &newarrow->end_pos);
					newarrow->select = true;
					nl.add(newarrow);
					OnProperty();
				}
			}
		}
		mode = none;
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
		dd = nl.hit(g_c.g, &p2, generation);
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
				object* dd2 = nl.hit(g_c.g, &p2, generation);
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
				p2.x = ((int)(p2.x - dragstartL.x) / GRID_WIDTH) * GRID_WIDTH + dragstartL.x;
				p2.y = ((int)(p2.y - dragstartL.y) / GRID_WIDTH) * GRID_WIDTH + dragstartL.y;
			}
			dragoffset.x = p2.x - dragstartL.x;
			dragoffset.y = p2.y - dragstartL.y;
			if (abs(p1.x - dragstartP.x) >= DRAGJUDGEWIDTH || abs(p1.y - dragstartP.y) >= DRAGJUDGEWIDTH)
			{
				dragjudge = true;
			}
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
		else if (mode == connection)
		{
			point p1{ (double)x, (double)y };
			point p2;
			t.d2l(&p1, &p2);

			static object* temp_select = 0;

			object* dd = nl.hit(g_c.g, &p2, generation);
			if (dd && dd->getobjectkind() != OBJECT_ARROW && dd != cp1.n)
			{
				if (temp_select && dd != temp_select)
				{
					temp_select->setselect(false, generation);
				}
				dd->setselect(true, generation);
				temp_select = dd;
			}
			else if (temp_select)
			{
				temp_select->setselect(false, generation);
			}

			pt2 = p2;
			InvalidateRect(hWnd, NULL, FALSE);
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
				if (mode == connection)
				{
					g_c.g->m_pRenderTarget->SetTransform(
						D2D1::Matrix3x2F::Translation((FLOAT)(t.c.w / 2 + t.p.x - t.l.x), (FLOAT)(t.c.h / 2 + t.p.y - t.l.y)) *
						D2D1::Matrix3x2F::Scale((FLOAT)t.z, (FLOAT)t.z, D2D1::Point2F((FLOAT)(t.c.w / 2 + t.p.x), (FLOAT)(t.c.h / 2 + t.p.y)))
					);
					D2D1_POINT_2F s = { (float)pt1.x, (float)pt1.y };
					D2D1_POINT_2F e = { (float)pt2.x, (float)pt2.y };
					//if (g_c.g->m_pDeviceContext) {
					//	ID2D1Effect* gaussianBlurEffect = 0;
					//	g_c.g->m_pDeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &gaussianBlurEffect);
					//	if (gaussianBlurEffect) {
					//		gaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_SOFT);
					//		gaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 50.0f);
					//		g_c.g->m_pDeviceContext->DrawLine(s, e, g_c.g->m_pSelectBkBrush, 8.0f);
					//		gaussianBlurEffect->Release();
					//		gaussianBlurEffect = 0;
					//	}
					//}
					//else
					//{
						g_c.g->m_pRenderTarget->DrawLine(s, e, g_c.g->m_pSelectBkBrush, 8.0f);
					//}
					g_c.g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				}
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
		nl.delselectobject(generation);
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
		nl.allselect(true, generation);
		OnProperty();
		RefreshToolBar();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	void OnNew() {
		if (generation > 0) {
			if (MessageBox(hWnd, L"新規作成しますか？", L"確認", MB_YESNOCANCEL | MB_ICONINFORMATION) != IDYES)
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
		if (bIsRunning) {
			if (bIsSuspend) {
				bIsSuspend = false;
				RefreshToolBar();
			}
		} else {
			bIsRunning = true;
			bIsSuspend = false;
			bForceExit = false;
			RefreshToolBar();

			DWORD dwParam;
			hThread = CreateThread(0, 0, ThreadFunc, (LPVOID)this, 0, &dwParam);
		}
	}

	void OnEnd() { // 実際にプログラムの停止時に呼び出される
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = 0;
		bIsRunning = false;
		bIsSuspend = false;
		bForceExit = false;
		RefreshToolBar();
	}

	void OnSuspend() { // 停止フラグをONにする
		if (bIsRunning) {
			bIsSuspend = true;
			RefreshToolBar();
		}
	}

	void OnStop() { // 止めるフラグをONにする
		if (bIsRunning) {
			DWORD dwParam;
			GetExitCodeThread(hThread, &dwParam);
			if (dwParam == STILL_ACTIVE) {
				bForceExit = true;
				RefreshToolBar();
			}
			else {
				WaitForSingleObject(hThread, INFINITE);
				CloseHandle(hThread);
				hThread = 0;
				bIsRunning = false;
				bIsSuspend = false;
				bForceExit = false;
				RefreshToolBar();
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

	static INT_PTR CALLBACK PropertyDialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
	{
		static app* pNoCodeApp;
		static bool enable_edit_event;
		switch (msg)
		{
		case WM_INITDIALOG:
			enable_edit_event = false;
			pNoCodeApp = (app*)lParam;
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
							{
								HWND hEdit = CreateWindowEx(WS_EX_STATICEDGE, RICHEDIT_CLASSW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NOHIDESEL, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
								SendMessage(hEdit, EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT);
								SendMessage(hEdit, EM_SETEVENTMASK, 0, ENM_UPDATE);
								SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
								util::DefaultRichEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
								SetWindowText(hEdit, (i->value) ? (i->value->c_str()) : 0);
							}
							CreateWindowEx(0, L"STATIC", (i->unit) ? (i->unit->c_str()) : 0, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, POINT2PIXEL(64 + 128 + 32), nYtop, POINT2PIXEL(32), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
							nYtop += POINT2PIXEL(32);
							break;
						case PROPERTY_CHECK:
							CreateWindowEx(0, L"BUTTON", (i->name) ? (i->name->c_str()) : 0, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, POINT2PIXEL(8), nYtop, POINT2PIXEL(256) - POINT2PIXEL(8), POINT2PIXEL(28), hDlg, 0, GetModuleHandle(0), 0);
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
								HWND hEdit = CreateWindowEx(WS_EX_STATICEDGE, RICHEDIT_CLASSW, 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_NOHIDESEL, POINT2PIXEL(64), nYtop + POINT2PIXEL(3), POINT2PIXEL(128 + 32), POINT2PIXEL(28) - POINT2PIXEL(3), hDlg, (HMENU)(1000 + index), GetModuleHandle(0), 0);
								SendMessage(hEdit, EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT);
								SendMessage(hEdit, EM_SETEVENTMASK, 0, ENM_UPDATE);
								SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
								util::DefaultRichEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
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
								util::DefaultRichEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)RichEditProc);
								SetWindowText(hEdit, (i->value) ? (i->value->c_str()) : 0);
							}
							nYtop += POINT2PIXEL(32 + 256 + 25);
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
			EnumChildWindows(hDlg, util::EnumChildSetFontProc, 0);
			return TRUE;
		case WM_COMMAND:
			
			break;
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
		std::vector<object*> selectnode;
		nl.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			object* newnode = i;
			if (firstedit) {
				std::vector<arrow*> arrow_start;
				std::vector<arrow*> arrow_end;
				if (newnode->getobjectkind() == OBJECT_NODE) {
					for (auto j : nl.l) {
						if (j->isalive(generation) && j->getobjectkind() == OBJECT_ARROW) {
							arrow* a = (arrow*)j;
							if (a->start == i) {
								arrow_start.push_back(a);
							}
							if (a->end == i) {
								arrow_end.push_back(a);
							}
						}
					}
				}
				newnode->kill(generation);
				newnode = i->copy(generation);
				for (auto j : arrow_start) {
					arrow* a = j->copy(generation);
					j->kill(generation);
					a->start = (node*)newnode;
					nl.add(a);
				}
				for (auto j : arrow_end) {
					arrow* a = j->copy(generation);
					j->kill(generation);
					a->end = (node*)newnode;
					nl.add(a);
				}
			}
			newnode->setpropertyvalue(index, pi);
			if (firstedit) {
				nl.add(newnode);
			}
		}
	}

	void OnProperty() {
		switch (GetSelectObjectKind()) {
		case OBJECT_NODE:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NORMAL), g_c.hPropContainer, PropertyDialogProc, (LPARAM)this);
			break;
		case OBJECT_MULTI:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_MULTI), g_c.hPropContainer, PropertyDialogProc, (LPARAM)0);
			break;
		case OBJECT_NONE:
		default:
			DestroyWindow(GetTopWindow(g_c.hPropContainer));
			CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PROPERTY_NONE), g_c.hPropContainer, PropertyDialogProc, (LPARAM)0);
			break;
		}
		SetFocus(g_c.hMainWnd);//ダイアログの方にフォーカスが取られてアクセラレータが効かなくなるため
	}

	OBJECT_KIND GetSelectObjectKind() const {
		const int selcount = nl.selectcount(generation);
		if (selcount > 1)
			return OBJECT_MULTI;
		if (selcount == 0)
			return OBJECT_NONE;
		std::vector<object*> selectnode;
		nl.selectlistup(&selectnode, generation);
		for (auto i : selectnode) {
			return i->getobjectkind();
		}
		return OBJECT_NONE;
	}

	void Error(LPCWSTR lpszMessage) {
		if (g_c.hMainWnd && IsWindow(g_c.hMainWnd))
		{
			MessageBox(g_c.hMainWnd, lpszMessage, 0, 0);
		}
	}

	static DWORD WINAPI ThreadFunc(LPVOID p)
	{
		bool bIsEndNodeLast = true;
		app* a = (app*)p;
		std::vector<object*> select;
		if (a) {
			a->nl.selectlistup(&select, a->generation);
			a->nl.allselect(false, a->generation);
			node* start = 0;
			for (auto i : a->nl.l) {
				if (i->isalive(a->generation) && i->getobjectkind() == OBJECT_NODE) {
					node* n = (node*)i;
					if (n->getnodekind() == NODE_START) {
						if (start == 0) {
							start = n;
						}
						else {
							a->Error(L"2つ以上開始ノードが配置されています。開始ノードは1つのみにしてください。");
							goto EXIT0;
						}
					}
				}
			}
			if (start == 0) {
				a->Error(L"開始ノードが見つかりませんでした。開始ノードを配置してください。");
				goto EXIT0;
			}
			bIsEndNodeLast = false;
			while (start)
			{
				start->setrunning(true, a->generation);
				if (a->hWnd && IsWindow(a->hWnd)) {
					InvalidateRect(a->hWnd, 0, FALSE);
				}
				start->execute(&a->bForceExit);
				start->setrunning(false, a->generation);
				if (a->hWnd && IsWindow(a->hWnd)) {
					InvalidateRect(a->hWnd, 0, FALSE);
				}
				if (start->getnodekind() == NODE_END) {
					bIsEndNodeLast = true;
					break;
				}
				if (a->bForceExit) {
					break;
				}
				arrow* nexta = (arrow*)start->next_arrow(a->nl.l, a->generation);
				if (nexta) {
					nexta->setrunning(true, a->generation);
					if (a->hWnd && IsWindow(a->hWnd)) {
						InvalidateRect(a->hWnd, 0, FALSE);
					}
					Sleep(100);
					nexta->setrunning(false, a->generation);
					if (a->hWnd && IsWindow(a->hWnd)) {
						InvalidateRect(a->hWnd, 0, FALSE);
					}
					start = start->next_node(a->nl.l, a->generation);
				}
				else
				{
					break;
				}
			}
		}
	EXIT0:
		if (a) {
			for (auto i : select) {
				i->setselect(true, a->generation);
			}
			if (bIsEndNodeLast || a->bForceExit) {
				if (a->hWnd && IsWindow(a->hWnd)) {
					PostMessage((HWND)a->hWnd, WM_APP + 1, 0, 0);
				}
			}
		}

		ExitThread(0);
	}
};
