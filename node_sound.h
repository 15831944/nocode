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

class node_sound :public node
{
public:
	node_sound(UINT64 initborn) : node(initborn) {
		lstrcpy(name, L"����炷");
		pl->description = L"���͂��ꂽwave�t�@�C�����Đ����܂��B";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_SINGLELINESTRING;
			pi->setname(L"�t�@�C���p�X");
			pi->sethelp(L"�e�L�X�g����͂��Ă��������B�ϐ�������͂��邱�Ƃ��\�ł��B");
			pi->setdescription(L"���͂��ꂽ��������u�o�́v�ɕ\�����܂��B");
			pi->setvalue(L"");
			pl->l.push_back(pi);
		}
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_CHECK;
			pi->setname(L"�������I������܂ő҂�");
			pi->sethelp(L"�e�L�X�g����͂��Ă��������B�ϐ�������͂��邱�Ƃ��\�ł��B");
			pi->setdescription(L"���͂��ꂽ��������u�o�́v�ɕ\�����܂��B");
			pi->setvalue(L"");
			pl->l.push_back(pi);
		}
	}
	node_sound(const node_sound* src, UINT64 initborn) : node(src, initborn) {}
	virtual node* copy(UINT64 generation) const override {
		node* newnode = new node_sound(this, generation);
		return newnode;
	}
	virtual NODE_KIND getnodekind() const {
		return NODE_SOUND;
	};
	virtual bool execute(bool*) const override {
		if (pl->l[0]->value && pl->l[0]->value->size() > 0) {
			LPCWSTR lpszFilePath = pl->l[0]->value->c_str();
			if (PathFileExists(lpszFilePath))
			{
				sound* s = new sound();
				s->play(lpszFilePath);
				delete s;
			}
		}
		return true;
	}
	virtual void save(HANDLE hFile) const override {};
	virtual void open(HANDLE hFile) const override {};
};
