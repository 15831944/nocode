#pragma once

#include "object.h"
#include "node.h"
#include "connectpoint.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

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

	virtual OBJECT_KIND getobjectkind() const {
		return OBJECT_ARROW;
	};

	virtual void paint(const graphic* g, const trans* t, bool drawconnectpoint, UINT64 generation, const point* offset = nullptr) const override { // 書き直す必要あり
		if (isalive(generation) && start && end) {
			g->m_pRenderTarget->SetTransform(
				D2D1::Matrix3x2F::Translation((FLOAT)(t->c.w / 2 + t->p.x - t->l.x), (FLOAT)(t->c.h / 2 + t->p.y - t->l.y)) *
				D2D1::Matrix3x2F::Scale((FLOAT)t->z, (FLOAT)t->z, D2D1::Point2F((FLOAT)(t->c.w / 2 + t->p.x), (FLOAT)(t->c.h / 2 + t->p.y)))
			);
			D2D1_POINT_2F c1, c2, c3, c4;
			D2D1_POINT_2F t1, t2, t3;
			if (start_pos == CONNECT_RIGHT && end_pos == CONNECT_LEFT) {
				c1 = { (float)(start->p.x + NODE_WIDTH / 2.0 + (offset ? offset->x : 0.0)), (float)(start->p.y + (offset ? offset->y : 0.0)) };
				c4 = { (float)(end->p.x - NODE_WIDTH / 2.0 - nArrowSize - 5.0f + (offset ? offset->x : 0.0)), (float)(end->p.y + (offset ? offset->y : 0.0)) };
				t1 = { c4.x + nArrowSize , c4.y};
				t2 = { t1.x - nArrowSize ,t1.y - nArrowWidth };
				t3 = { t1.x - nArrowSize ,t1.y + nArrowWidth };
				c2 = { (float)((3.0 * c4.x + c1.x) / 4.0), c1.y };
				c3 = { (float)((3.0 * c1.x + c4.x) / 4.0), c4.y };
			}
			else if (start_pos == CONNECT_LEFT && end_pos == CONNECT_RIGHT ) {
				c4 = { (float)(start->p.x - NODE_WIDTH / 2.0 + (offset ? offset->x : 0.0)), (float)(start->p.y + (offset ? offset->y : 0.0)) };
				c1 = { (float)(end->p.x + NODE_WIDTH / 2.0 + nArrowSize + 5.0f + (offset ? offset->x : 0.0)), (float)(end->p.y + (offset ? offset->y : 0.0)) };
				t1 = { c1.x - nArrowSize, c1.y };
				t2 = { t1.x + nArrowSize ,t1.y - nArrowWidth };
				t3 = { t1.x + nArrowSize ,t1.y + nArrowWidth };
				c2 = { (float)((3.0 * c4.x + c1.x) / 4.0), c1.y };
				c3 = { (float)((3.0 * c1.x + c4.x) / 4.0), c4.y };
			}
			else if (start_pos == CONNECT_BOTTOM && end_pos == CONNECT_TOP) {
				c1 = { (float)(start->p.x + (offset ? offset->x : 0.0)), (float)(start->p.y + NODE_HEIGHT / 2.0 + (offset ? offset->y : 0.0)) };
				c4 = { (float)(end->p.x + (offset ? offset->x : 0.0)), (float)(end->p.y - NODE_HEIGHT / 2.0 - nArrowSize - 5.0f + (offset ? offset->y : 0.0)) };
				t1 = { c4.x, c4.y + nArrowSize };
				t2 = { t1.x - nArrowWidth,t1.y - nArrowSize };
				t3 = { t1.x + nArrowWidth,t1.y - nArrowSize };
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			}
			else if (start_pos == CONNECT_TOP && end_pos == CONNECT_BOTTOM) {
				c4 = { (float)(start->p.x + (offset ? offset->x : 0.0)), (float)(start->p.y - NODE_HEIGHT / 2.0 + (offset ? offset->y : 0.0)) };
				c1 = { (float)(end->p.x + (offset ? offset->x : 0.0)), (float)(end->p.y + NODE_HEIGHT / 2.0 + nArrowSize + 5.0f + (offset ? offset->y : 0.0)) };
				t1 = { c1.x, c1.y - nArrowSize };
				t2 = { t1.x - nArrowWidth,t1.y + nArrowSize };
				t3 = { t1.x + nArrowWidth,t1.y + nArrowSize };
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			} else {
				if (start->p.y < end->p.y) {
					c1 = { (float)(start->p.x + (offset ? offset->x : 0.0)), (float)(start->p.y + NODE_HEIGHT / 2.0 + (offset ? offset->y : 0.0)) };
					c4 = { (float)(end->p.x + (offset ? offset->x : 0.0)), (float)(end->p.y - NODE_HEIGHT / 2.0 - nArrowSize - 5.0f + (offset ? offset->y : 0.0)) };
					t1 = { c4.x, c4.y + nArrowSize };
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
			ID2D1PathGeometry* pPathGeometry;
			// 線分の描画
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
				g->m_pRenderTarget->DrawGeometry(pPathGeometry, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush, 4.0f);
				SafeRelease(&pPathGeometry);
			}
			// 矢印の描画
			g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
			if (pPathGeometry) {
				ID2D1GeometrySink* pSink;
				pPathGeometry->Open(&pSink);
				pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
				pSink->BeginFigure(t1, D2D1_FIGURE_BEGIN_FILLED);
				D2D1_POINT_2F points[] = { t2,t3 };
				pSink->AddLines(points, ARRAYSIZE(points));
				pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
				pSink->Close();
				SafeRelease(&pSink);
				g->m_pRenderTarget->FillGeometry(pPathGeometry, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush);
				g->m_pRenderTarget->DrawGeometry(pPathGeometry, running ? g->m_pRunningBrush : select ? g->m_pSelectBrush : g->m_pNormalBrush, 4.0f);
				SafeRelease(&pPathGeometry);
			}
			g->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		}
	}
	virtual bool hit(const graphic* g, const point* p, UINT64 generation, const float offset) const override { // 書き直す必要あり
		if (isalive(generation) && start && end) {

			D2D1_POINT_2F c1, c2, c3, c4;
			D2D1_POINT_2F t1, t2, t3;
			if (start_pos == CONNECT_RIGHT && end_pos == CONNECT_LEFT) {
				c1 = { (float)(start->p.x + NODE_WIDTH / 2.0), (float)(start->p.y)};
				c4 = { (float)(end->p.x - NODE_WIDTH / 2.0 - nArrowSize - 5.0f ), (float)(end->p.y ) };
				t1 = { c4.x + nArrowSize , c4.y };
				t2 = { t1.x - nArrowSize ,t1.y - nArrowWidth };
				t3 = { t1.x - nArrowSize ,t1.y + nArrowWidth };
				c2 = { (float)((3.0 * c4.x + c1.x) / 4.0), c1.y };
				c3 = { (float)((3.0 * c1.x + c4.x) / 4.0), c4.y };
			}
			else if (start_pos == CONNECT_LEFT && end_pos == CONNECT_RIGHT) {
				c4 = { (float)(start->p.x - NODE_WIDTH / 2.0 ), (float)(start->p.y ) };
				c1 = { (float)(end->p.x + NODE_WIDTH / 2.0 + nArrowSize + 5.0f ), (float)(end->p.y ) };
				t1 = { c1.x - nArrowSize, c1.y };
				t2 = { t1.x + nArrowSize ,t1.y - nArrowWidth };
				t3 = { t1.x + nArrowSize ,t1.y + nArrowWidth };
				c2 = { (float)((3.0 * c4.x + c1.x) / 4.0), c1.y };
				c3 = { (float)((3.0 * c1.x + c4.x) / 4.0), c4.y };
			}
			else if (start_pos == CONNECT_BOTTOM && end_pos == CONNECT_TOP) {
				c1 = { (float)(start->p.x ), (float)(start->p.y + NODE_HEIGHT / 2.0 ) };
				c4 = { (float)(end->p.x ), (float)(end->p.y - NODE_HEIGHT / 2.0 - nArrowSize - 5.0f ) };
				t1 = { c4.x, c4.y + nArrowSize };
				t2 = { t1.x - nArrowWidth,t1.y - nArrowSize };
				t3 = { t1.x + nArrowWidth,t1.y - nArrowSize };
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			}
			else if (start_pos == CONNECT_TOP && end_pos == CONNECT_BOTTOM) {
				c4 = { (float)(start->p.x ), (float)(start->p.y - NODE_HEIGHT / 2.0 ) };
				c1 = { (float)(end->p.x ), (float)(end->p.y + NODE_HEIGHT / 2.0 + nArrowSize + 5.0f ) };
				t1 = { c1.x, c1.y - nArrowSize };
				t2 = { t1.x - nArrowWidth,t1.y + nArrowSize };
				t3 = { t1.x + nArrowWidth,t1.y + nArrowSize };
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			}
			else {
				if (start->p.y < end->p.y) {
					c1 = { (float)(start->p.x ), (float)(start->p.y + NODE_HEIGHT / 2.0 ) };
					c4 = { (float)(end->p.x ), (float)(end->p.y - NODE_HEIGHT / 2.0 - nArrowSize - 5.0f ) };
					t1 = { c4.x, c4.y + nArrowSize };
					t2 = { t1.x - nArrowWidth,t1.y - nArrowSize };
					t3 = { t1.x + nArrowWidth,t1.y - nArrowSize };
				}
				else {
					c4 = { (float)(start->p.x ), (float)(start->p.y - NODE_HEIGHT / 2.0 ) };
					c1 = { (float)(end->p.x ), (float)(end->p.y + NODE_HEIGHT / 2.0 + nArrowSize + 5.0f ) };
					t1 = { c1.x, c1.y - nArrowSize };
					t2 = { t1.x - nArrowWidth,t1.y + nArrowSize };
					t3 = { t1.x + nArrowWidth,t1.y + nArrowSize };
				}
				c2 = { c1.x, (float)((3.0 * c4.y + c1.y) / 4.0) };
				c3 = { c4.x, (float)((3.0 * c1.y + c4.y) / 4.0) };
			}

			BOOL containsPoint = FALSE;
			{
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
					pPathGeometry->StrokeContainsPoint(
						D2D1::Point2F((FLOAT)p->x, (FLOAT)p->y),
						offset,     // stroke width
						NULL,   // stroke style
						NULL,   // world transform
						&containsPoint
					);
					SafeRelease(&pSink);
					SafeRelease(&pPathGeometry);
				}
			}
			if (!containsPoint) {
				ID2D1PathGeometry* pPathGeometry;
				g->m_pD2DFactory->CreatePathGeometry(&pPathGeometry);
				if (pPathGeometry) {
					ID2D1GeometrySink* pSink;
					pPathGeometry->Open(&pSink);
					pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
					pSink->BeginFigure(t1, D2D1_FIGURE_BEGIN_FILLED);
					D2D1_POINT_2F points[] = { t2,t3 };
					pSink->AddLines(points, ARRAYSIZE(points));
					pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
					pSink->Close();
					pPathGeometry->StrokeContainsPoint(
						D2D1::Point2F((FLOAT)p->x, (FLOAT)p->y),
						offset,     // stroke width
						NULL,   // stroke style
						NULL,   // world transform
						&containsPoint
					);
					SafeRelease(&pSink);
					SafeRelease(&pPathGeometry);
				}
			}
			return (bool)containsPoint;
		}
		return false;
	}
	virtual bool inrect(const point* p1, const point* p2, UINT64 generation, const float offset = 16.0f) const override { // 書き直す必要あり
		if (isalive(generation) && start && end) {
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
