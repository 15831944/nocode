#pragma once

#include <list>
#include "object.h"
#include "connectpoint.h"
#include "arrow.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

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
	object* hit(const graphic* g, const point* p, UINT64 generation, const object* without = nullptr) const {
		for (auto i = l.rbegin(), e = l.rend(); i != e; ++i) {
			if (without != *i && (*i)->hit(g, p, generation))
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
	void rectselect(const point* p1, const point* p2, UINT64 generation, std::vector<object*>* selectnode) {
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
	void allselect(bool select, UINT64 generation) {
		for (auto i : l) {
			i->setselect(select, generation);
		}
	}
	void delselectobject(UINT64 generation) {
		std::vector<object*> select_object;
		selectlistup(&select_object, generation);
		for (auto i : select_object) {
			if (i->isselect(generation)) {
				// 紐づくarrowも消す
				if (i->getobjectkind() == OBJECT_NODE) {
					std::vector<arrow*> arrow_start;
					std::vector<arrow*> arrow_end;
					for (auto j : l) {
						if (j->isalive(generation) && j->getobjectkind() == OBJECT_ARROW) {
							arrow* a = (arrow*)j;
							if (a->start == i || a->end == i) {
								a->kill(generation);
								if (a->start == i) {
									arrow_start.push_back(a);
								}
								if (a->end == i) {
									arrow_end.push_back(a);
								}
							}
						}
					}
					if (arrow_start.size() == 1 && arrow_end.size() == 1) {
						arrow* newarrow = arrow_end[0]->copy(generation);
						newarrow->end = arrow_start[0]->end;
						newarrow->end_pos = arrow_start[0]->end_pos;
						l.push_back(newarrow);
					}
				}
				i->kill(generation);
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
	void selectlistup(std::vector<object*>* selectnode, UINT64 generation) const {
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
	void selectoffsetmerge(const graphic* g, const point* p/*マウスカーソルの座標*/, const point* dragoffset, UINT64 generation) {
		std::vector<object*> selectnode_old;
		std::vector<object*> selectnode_new;
		std::vector<arrow*> arrow_new;
		selectlistup(&selectnode_old, generation);
		for (auto i : selectnode_old) {
			if (i->getobjectkind() == OBJECT_ARROW) continue;
			object* newnode = i->copy(generation);
			newnode->p.x += dragoffset->x;
			newnode->p.y += dragoffset->y;
			l.push_back(newnode);
			selectnode_new.push_back(newnode);
		}
		for (auto i : l) {
			if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW) {
				arrow* a = (arrow*)i;
				if (a->start && a->end && (a->start->isselect(generation) || a->end->isselect(generation))) {
					a->kill(generation);
					arrow* newarrow = a->copy(generation);
					for (unsigned int j = 0; j < selectnode_old.size(); j++) {
						if (newarrow->start == selectnode_old[j]) {
							newarrow->start = (node*)selectnode_new[j];
						}
						if (newarrow->end == selectnode_old[j]) {
							newarrow->end = (node*)selectnode_new[j];
						}
					}
					arrow_new.push_back(newarrow);
				}
			}
		}
		for (auto i : selectnode_old) {
			if (i->getobjectkind() == OBJECT_ARROW) continue;
			i->kill(generation);
		}
		for (auto i : arrow_new) {
			l.push_back(i);
		}

		if (selectnode_new.size() == 1) {
			// 入ってくるarrowも出ていくarrowもない場合で、mainselectがarrow（arrow_newでない）の上の場合は繋げる
			std::vector<arrow*> arrow_in;
			std::vector<arrow*> arrow_out;
			for (auto i : arrow_new) {
				if (i->start && std::find(selectnode_new.begin(), selectnode_new.end(), i->start) == selectnode_new.end()) { // 見つからない場合
					arrow_out.push_back(i);
				}
				
				if (i->end && std::find(selectnode_new.begin(), selectnode_new.end(), i->end) == selectnode_new.end()) { // 見つからない場合
					arrow_in.push_back(i);
				}
			}
			if (arrow_in.size() == 0 && arrow_out.size() == 0) {
				arrow* a1 = 0;
				arrow* a2 = 0;
				for (auto i : l) {
					if (i->isalive(generation) && i->getobjectkind() == OBJECT_ARROW) {
						arrow* a = (arrow*)i;
						if (std::find(arrow_new.begin(), arrow_new.end(), a) == arrow_new.end()) { // 見つからない場合
							if (a->hit(g, &selectnode_new[0]->p, generation, 64.0f)) {
								a->kill(generation);
								a1 = a->copy(generation);
								a2 = a->copy(generation);
								a1->end = (node*)selectnode_new[0];
								a1->end_pos = CONNECT_TOP;
								a2->start = (node*)selectnode_new[0];
								a2->start_pos = CONNECT_BOTTOM;
								break;
							}
						}
					}
				}
				if (a1) {
					l.push_back(a1);
				}
				if (a2) {
					l.push_back(a2);
				}
			}
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
