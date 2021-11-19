#pragma once

#include "node.h"
#include "common.h"

class node_timer :public node
{
public:
	node_timer(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"タイマー");
		pl->description = L"指定秒間待ちます。";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"指定秒数");
			pi->sethelp(L"秒数を指定してください。");
			pi->setdescription(L"指定された秒数待ちます。");
			pi->setvalue(L"3");
			pi->setmatch(L"[0-9]*");
			pi->setunit(L"秒");
			pl->l.push_back(pi);
		}

	}
	node_timer(const node_timer* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_timer(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_TIMER;
	};
	virtual bool execute(bool* exit) const override {
		// 何もしない

		if (pl->l[0]->value && pl->l[0]->value->size() > 0)
		{
			int sec = _wtoi(pl->l[0]->value->c_str());
			for (int i = 0; i < sec; i++)
			{
				Sleep(1000);
				if (*exit)
				{
					break;
				}
			}
		}

		return true;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};
