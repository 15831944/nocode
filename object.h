#pragma once

#include <windows.h>
#include "point.h"
#include "size.h"
#include "propertyitemlist.h"
#include "graphic.h"
#include "trans.h"

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
	static UINT64 initid;
	UINT64 id;
	OBJECT_KIND kind;
	bool select;
	UINT64 born;
	UINT64 dead;
	point p;
	size s;
	propertyitemlist* pl;
	object(UINT64 initborn, OBJECT_KIND kind1) :id(++initid), kind(kind1), select(false), born(initborn), dead(UINT64_MAX), p{}, s{}, pl(0)
	{
		pl = new propertyitemlist;
	}
	object(const object* src, UINT64 initborn) : id(src->id), kind(src->kind), select(src->select), born(initborn), dead(UINT64_MAX), p(src->p), s(src->s) {
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
	virtual bool hit(const graphic* g, const point* p, UINT64 generation) const = 0;
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