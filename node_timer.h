#pragma once

#include "node.h"
#include "common.h"

class node_timer :public node
{
public:
	node_timer(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"�^�C�}�[");
		pl->description = L"�w��b�ԑ҂��܂��B";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"�w��b��");
			pi->sethelp(L"�b�����w�肵�Ă��������B");
			pi->setdescription(L"�w�肳�ꂽ�b���҂��܂��B");
			pi->setvalue(L"3");
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
	virtual bool execute() const override {
		// �������Ȃ�

		if (pl->l[0]->value && pl->l[0]->value->size() > 0)
		{
			int sec = _wtoi(pl->l[0]->value->c_str());
			Sleep(sec * 1000);
		}

		return true;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};
