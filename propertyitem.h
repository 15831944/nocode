#pragma once

#include <windows.h>
#include <string>

enum PROPERTY_KIND {
	PROPERTY_NONE,
	PROPERTY_INT,
	PROPERTY_CHECK,
	PROPERTY_SINGLELINESTRING,
	PROPERTY_MULTILINESTRING,
	PROPERTY_DATE,
	PROPERTY_TIME,
	PROPERTY_IP,
	PROPERTY_PICTURE,
	PROPERTY_COLOR,
	PROPERTY_SELECT,
	PROPERTY_CUSTOM
};

class propertyitem {
public:
	PROPERTY_KIND kind;
	bool required;
	std::wstring* name;
	std::wstring* help;
	std::wstring* description;
	std::wstring* value;
	std::wstring* unit;
	std::wstring* match;
	propertyitem() : kind(PROPERTY_NONE), required(false), name(0), help(0), description(0), value(0), unit(0), match(0) {}
	propertyitem(const propertyitem* src) : kind(src->kind), required(src->required), name(0), help(0), description(0), value(0), unit(0), match(0) {
		if (src->name) name = new std::wstring(*src->name);
		if (src->help) help = new std::wstring(*src->help);
		if (src->description) description = new std::wstring(*src->description);
		if (src->value) value = new std::wstring(*src->value);
		if (src->unit) unit = new std::wstring(*src->unit);
		if (src->match) match = new std::wstring(*src->match);
	}
	virtual ~propertyitem() {
		delete name;
		delete help;
		delete description;
		delete value;
		delete unit;
		delete match;
	}
	propertyitem* copy() const {
		return new propertyitem(this);
	}
	void setname(LPCWSTR lpszName) {
		name = new std::wstring(lpszName);
	}
	void sethelp(LPCWSTR lpszHelp) {
		help = new std::wstring(lpszHelp);
	}
	void setdescription(LPCWSTR lpszDescription) {
		description = new std::wstring(lpszDescription);
	}
	void setvalue(LPCWSTR lpszValue) {
		value = new std::wstring(lpszValue);
	}
	void setunit(LPCWSTR lpszUnit) {
		unit = new std::wstring(lpszUnit);
	}
	void setmatch(LPCWSTR lpszMatch) {
		match = new std::wstring(lpszMatch);
	}
};
