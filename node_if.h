#pragma once

#include "node.h"
#include "common.h"

class node_if :public node
{
public:
	node_if(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"”»’f");
		pl->description = L"ðŒ•ªŠò";
	}
	node_if(const node_if* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_if(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_IF;
	};
	virtual bool execute(bool*) const override {
		// ‰½‚à‚µ‚È‚¢
		Sleep(100);
		return true;
	}
	virtual void paint(const graphic* g, const trans* t, bool drawconnectpoint, UINT64 generation, const point* offset = nullptr) const override {
		if (!isalive(generation)) return;
		g->m_pRenderTarget->SetTransform(
			D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
			D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
		);
		D2D1_POINT_2F t1 = { (float)(p.x + (offset ? offset->x : 0.0)), (float)(p.y - s.h / 2 + (offset ? offset->y : 0.0)) };
		D2D1_POINT_2F t2 = { (float)(p.x - s.w / 2 + (offset ? offset->x : 0.0)), (float)(p.y + (offset ? offset->y : 0.0)) };
		D2D1_POINT_2F t3 = { (float)(p.x + (offset ? offset->x : 0.0)), (float)(p.y + s.h / 2 + (offset ? offset->y : 0.0)) };
		D2D1_POINT_2F t4 = { (float)(p.x + s.w / 2 + (offset ? offset->x : 0.0)), (float)(p.y + (offset ? offset->y : 0.0)) };
		ID2D1PathGeometry* pPathGeometry;
		g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
		if (pPathGeometry) {
			ID2D1GeometrySink* pSink;
			pPathGeometry->Open(&pSink);
			pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
			pSink->BeginFigure(t1, D2D1_FIGURE_BEGIN_FILLED);
			D2D1_POINT_2F points[] = { t2,t3,t4 };
			pSink->AddLines(points, ARRAYSIZE(points));
			pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
			pSink->Close();
			SafeRelease(&pSink);
			g->m_pRenderTarget->FillGeometry(pPathGeometry, running ? g->m_pRunningBkBrush : select ? g->m_pSelectBkBrush : g->m_pNormalBkBrush);
			g->m_pRenderTarget->DrawGeometry(pPathGeometry, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush, 2.0f);
			SafeRelease(&pPathGeometry);
		}
		D2D1_RECT_F rect = {
			(float)(p.x - s.w / 2 + (offset ? offset->x : 0.0)),
			(float)(p.y - s.h / 2 + (offset ? offset->y : 0.0)),
			(float)(p.x + s.w / 2 + (offset ? offset->x : 0.0)),
			(float)(p.y + s.h / 2 + (offset ? offset->y : 0.0))
		};
		g->m_pRenderTarget->DrawText(name, lstrlenW(name), g->m_pTextFormat, &rect, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush);
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		if (drawconnectpoint && select && offset == nullptr) {
			this->drawconnectpoint(g, t, generation);
		}
	}
	virtual void save(HANDLE hFile) const override {};
	virtual void open(HANDLE hFile) const override {};

	virtual object* next_arrow(std::list<object*> l, UINT64 generation) override;
};