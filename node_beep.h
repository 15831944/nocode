#pragma once
#pragma once

#pragma comment(lib, "shlwapi")
#pragma comment(lib, "winmm")

#include <shlwapi.h>
#include "node.h"
#include "sound.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

class node_beep :public node
{
public:
	node_beep(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"BEEP��炷");
		pl->description = L"���͂��ꂽwave�t�@�C�����Đ����܂��B";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"���g��");
			pi->setunit(L"�w���c");
			pi->sethelp(L"�e�L�X�g����͂��Ă��������B�ϐ�������͂��邱�Ƃ��\�ł��B");
			pi->setdescription(L"���͂��ꂽ��������u�o�́v�ɕ\�����܂��B");
			pi->setvalue(L"500");
			pl->l.push_back(pi);
		}
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"����");
			pi->setunit(L"�~���b");
			pi->sethelp(L"�e�L�X�g����͂��Ă��������B�ϐ�������͂��邱�Ƃ��\�ł��B");
			pi->setdescription(L"���͂��ꂽ��������u�o�́v�ɕ\�����܂��B");
			pi->setvalue(L"500");
			pl->l.push_back(pi);
		}
	}
	node_beep(const node_beep* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_beep(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_BEEP;
	};
	virtual bool execute(bool*) const override {

		if (pl->l[0]->value && pl->l[0]->value->size() > 0 &&
			pl->l[1]->value && pl->l[1]->value->size() > 0)
		{
			int p1 = _wtoi(pl->l[0]->value->c_str());
			int p2 = _wtoi(pl->l[1]->value->c_str());
			Beep(p1, p2);//���g���Ǝ���
		}
		return true;
	}
	virtual void save(HANDLE hFile) const override {};
	virtual void open(HANDLE hFile) const override {};
};
