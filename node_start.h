#pragma once

#include "node.h"
#include "common.h"

class node_start :public node
{
public:
	node_start(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"開始");
		pl->description = L"プログラムの開始点";
	}
	node_start(const node_start* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_start(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_START;
	};
	virtual bool execute(bool*) const override {
		// 何もしない
		Sleep(100);
		return true;
	}
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
		D2D1_POINT_2F point = { (float)(p.x + (offset ? offset->x : 0.0)), (float)(p.y + (offset ? offset->y : 0.0)) };
		float rx = (float)(s.w / 2.0);
		float ry = (float)(s.h / 2.0);
		D2D1_ELLIPSE tEllipse = D2D1::Ellipse(point, (float)rx, (float)ry);
		g->m_pRenderTarget->FillEllipse(tEllipse, running ? g->m_pRunningBkBrush : select ? g->m_pSelectBkBrush : g->m_pNormalBkBrush);
		g->m_pRenderTarget->DrawEllipse(tEllipse, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush, 2.0f);
		g->m_pRenderTarget->DrawText(name, lstrlenW(name), g->m_pTextFormat, &rect, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush);
		g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		if (drawconnectpoint && select && offset == nullptr) {
			this->drawconnectpoint(g, t, generation);
		}
	}
	virtual void save(HANDLE hFile) const override {};
	virtual void open(HANDLE hFile) const override {};
};
