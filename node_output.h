#pragma once

#include "node.h"
#include "common.h"

class node_output :public node
{
public:
	node_output(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"�o��");
		pl->description = L"���͒l��[�o��]�ɕ\�����܂��B";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_MULTILINESTRING;
			pi->setname(L"���͒l");
			pi->sethelp(L"�e�L�X�g����͂��Ă��������B�ϐ�������͂��邱�Ƃ��\�ł��B");
			pi->setdescription(L"���͂��ꂽ��������u�o�́v�ɕ\�����܂��B");
			pi->setvalue(L"");
			pl->l.push_back(pi);
		}
	}
	node_output(const node_output* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_output(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_OUTPUT;
	};
	virtual bool execute() const override {
		if (g_c.hOutput && IsWindow(g_c.hOutput)) {
			HWND hEdit = GetTopWindow(g_c.hOutput);
			if (hEdit && pl->l[0]->value && pl->l[0]->value->size() > 0) {
				const int index = GetWindowTextLength(hEdit);
				SendMessage(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
				SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)pl->l[0]->value->c_str());
				SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
				SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
				return true;
			}
		}
		return false;
	}
	virtual void save(HANDLE hFile) const {};
	virtual void open(HANDLE hFile) const {};
};
