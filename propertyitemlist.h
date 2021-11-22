#pragma once

#include <string>
#include <vector>
#include "propertyitem.h"

class propertyitemlist {
public:
	std::wstring description;
	std::vector<propertyitem*> l;
	propertyitemlist() : l{} {}
	virtual ~propertyitemlist() {
		for (auto i : l) {
			delete i;
		}
		l.clear();
	}
	propertyitemlist* copy() const {
		propertyitemlist* nl = new propertyitemlist;
		nl->description = description;
		for (auto i : l) {
			nl->l.push_back(i->copy());
		}
		return nl;
	}
};
