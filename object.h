#pragma once

#include <windows.h>
#include "point.h"
#include "size.h"
#include "propertyitemlist.h"
#include "graphic.h"
#include "trans.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

enum OBJECT_KIND {
	OBJECT_NONE,
	OBJECT_PRIMITIVE,
	OBJECT_NODE,
	OBJECT_ARROW,
	OBJECT_COMMENT,
	OBJECT_VARIABLE,
	OBJECT_MULTI,
};

class object {
public:
	static UINT64 initid;
	UINT64 id;
	bool select;
	bool running;
	UINT64 born;
	UINT64 dead;
	point p;
	size s;
	propertyitemlist* pl;
	object(UINT64 initborn) :id(++initid), select(false), running(false), born(initborn), dead(UINT64_MAX), p{}, s{}, pl(0)
	{
		pl = new propertyitemlist;
	}
	object(const object* src, UINT64 initborn) : id(src->id), select(src->select), running(src->running), born(initborn), dead(UINT64_MAX), p(src->p), s(src->s) {
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
	virtual void setrunning(bool b, UINT64 generation) {
		if (isalive(generation)) {
			running = b;
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
	virtual int hitconnectpoint(const point* p, point* cp, UINT64 generation, const float offset = 16.0f) const { return 0; }
	virtual bool hit(const graphic* g, const point* p, UINT64 generation, const float offset = 16.0f) const { // ポイントが矩形に含まれるか
		if (this->isalive(generation) &&
			this->p.x - this->s.w / 2 <= p->x && this->p.y - this->s.h / 2 <= p->y &&
			this->p.x + this->s.w / 2 >= p->x && this->p.y + this->s.h / 2 >= p->y)
			return true;
		else
			return false;
	}
	virtual bool hit(const graphic* g, const object* o, UINT64 generation, const float offset = 16.0f) const { // 矩形同士を想定
		if (this->isalive(generation) &&
			(this->s.w + o->s.w) / 2 >= abs(this->p.x - o->p.x) &&
			(this->s.h + o->s.h) / 2 >= abs(this->p.y - o->p.y))
			return true;
		else
			return false;
	}
	virtual bool inrect(const point* p1, const point* p2, UINT64 generation, const float offset = 16.0f) const {
		if (this->isalive(generation) &&
			this->p.x - this->s.w / 2 >= p1->x && this->p.y - this->s.h / 2 >= p1->y && // 右上がp1左下がp2と仮定している
			this->p.x + this->s.w / 2 <= p2->x && this->p.y + this->s.h / 2 <= p2->y)
			return true;
		else
			return false;

	};
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
	virtual OBJECT_KIND getobjectkind() const = 0;
	virtual void save(HANDLE hFile) const = 0;
	virtual void open(HANDLE hFile) const = 0;
};