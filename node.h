#pragma once

#include <windows.h>
#include "object.h"

#define NODE_WIDTH 200
#define NODE_HEIGHT 100
#define CONNECT_POINT_WIDTH 8.0f

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
