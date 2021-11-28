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
		lstrcpy(name, L"BEEPを鳴らす");
		pl->description = L"入力されたwaveファイルを再生します。";
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"周波数");
			pi->setunit(L"ヘルツ");
			pi->sethelp(L"テキストを入力してください。変数名を入力することも可能です。");
			pi->setdescription(L"入力された文字列を「出力」に表示します。");
			pi->setvalue(L"500");
			pl->l.push_back(pi);
		}
		{
			propertyitem* pi = new propertyitem;
			pi->kind = PROPERTY_INT;
			pi->setname(L"時間");
			pi->setunit(L"ミリ秒");
			pi->sethelp(L"テキストを入力してください。変数名を入力することも可能です。");
			pi->setdescription(L"入力された文字列を「出力」に表示します。");
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
			Beep(p1, p2);//周波数と時間
		}
		return true;
	}
	virtual void save(HANDLE hFile) const override {};
	virtual void open(HANDLE hFile) const override {};
};
