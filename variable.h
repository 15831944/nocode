#pragma once

#include <windows.h>
#include <string>
#include <atlcomcli.h>
//#include <atlstr.h>
//#include <atlsafe.h>
#include "object.h"

class variable : public object
{
public:
	std::wstring name;
	CComVariant value;
	LPWSTR lpszValueForDisplay;

	variable(UINT64 initborn) : object(initborn), lpszValueForDisplay(0){}

	~variable() {
		GlobalFree(lpszValueForDisplay);
		value.Clear();
	}

    variable(const variable* src, UINT64 initborn) : object(src, initborn) {
        name = src->name;
        this->value.Copy(&src->value);
    }

    virtual variable* copy(UINT64 generation) const override {
        variable* newvariable = new variable(this, generation);
        return newvariable;
    }

    virtual void paint(const graphic*, const trans*, bool, UINT64, const point*) const override {}

    virtual void save(HANDLE) const override {}
    virtual void open(HANDLE) const override {}


	LPWSTR displayValue() {
		GlobalFree(lpszValueForDisplay);

		switch (value.vt) {
        case VT_EMPTY:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_EMPTY");
            break;
        case VT_NULL:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_NULL");
            break;
        case VT_I2:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_I2");
            break;
        case VT_I4:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_I4");
            break;
        case VT_R4:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_R4");
            break;
        case VT_R8:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_R8");
            break;
        case VT_CY:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_CY");
            break;
        case VT_DATE:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_DATE");
            break;
        case VT_BSTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_BSTR");
            break;
        case VT_DISPATCH:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 12);
            lstrcpy(lpszValueForDisplay, L"VT_DISPATCH");
            break;
        case VT_ERROR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_ERROR");
            break;
        case VT_BOOL:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_BOOL");
            break;
        case VT_VARIANT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_VARIANT");
            break;
        case VT_UNKNOWN:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_UNKNOWN");
            break;
        case VT_DECIMAL:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_DECIMAL");
            break;
        case VT_I1:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_I1");
            break;
        case VT_UI1:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_UI1");
            break;
        case VT_UI2:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_UI2");
            break;
        case VT_UI4:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_UI4");
            break;
        case VT_I8:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_I8");
            break;
        case VT_UI8:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_UI8");
            break;
        case VT_INT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_INT");
            break;
        case VT_UINT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_UINT");
            break;
        case VT_VOID:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_VOID");
            break;
        case VT_HRESULT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_HRESULT");
            break;
        case VT_PTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 7);
            lstrcpy(lpszValueForDisplay, L"VT_PTR");
            break;
        case VT_SAFEARRAY:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 13);
            lstrcpy(lpszValueForDisplay, L"VT_SAFEARRAY");
            break;
        case VT_CARRAY:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 10);
            lstrcpy(lpszValueForDisplay, L"VT_CARRAY");
            break;
        case VT_USERDEFINED:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 15);
            lstrcpy(lpszValueForDisplay, L"VT_USERDEFINED");
            break;
        case VT_LPSTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_LPSTR");
            break;
        case VT_LPWSTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 10);
            lstrcpy(lpszValueForDisplay, L"VT_LPWSTR");
            break;
        case VT_RECORD:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 10);
            lstrcpy(lpszValueForDisplay, L"VT_RECORD");
            break;
        case VT_INT_PTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_INT_PTR");
            break;
        case VT_UINT_PTR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 12);
            lstrcpy(lpszValueForDisplay, L"VT_UINT_PTR");
            break;
        case VT_FILETIME:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 12);
            lstrcpy(lpszValueForDisplay, L"VT_FILETIME");
            break;
        case VT_BLOB:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 8);
            lstrcpy(lpszValueForDisplay, L"VT_BLOB");
            break;
        case VT_STREAM:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 10);
            lstrcpy(lpszValueForDisplay, L"VT_STREAM");
            break;
        case VT_STORAGE:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_STORAGE");
            break;
        case VT_STREAMED_OBJECT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 19);
            lstrcpy(lpszValueForDisplay, L"VT_STREAMED_OBJECT");
            break;
        case VT_STORED_OBJECT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 17);
            lstrcpy(lpszValueForDisplay, L"VT_STORED_OBJECT");
            break;
        case VT_BLOB_OBJECT:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 15);
            lstrcpy(lpszValueForDisplay, L"VT_BLOB_OBJECT");
            break;
        case VT_CF:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 6);
            lstrcpy(lpszValueForDisplay, L"VT_CF");
            break;
        case VT_CLSID:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_CLSID");
            break;
        case VT_VERSIONED_STREAM:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 20);
            lstrcpy(lpszValueForDisplay, L"VT_VERSIONED_STREAM");
            break;
        case VT_BSTR_BLOB:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 13);
            lstrcpy(lpszValueForDisplay, L"VT_BSTR_BLOB");
            break;
        case VT_VECTOR:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 10);
            lstrcpy(lpszValueForDisplay, L"VT_VECTOR");
            break;
        case VT_ARRAY:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_ARRAY");
            break;
        case VT_BYREF:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 9);
            lstrcpy(lpszValueForDisplay, L"VT_BYREF");
            break;
        case VT_RESERVED:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 12);
            lstrcpy(lpszValueForDisplay, L"VT_RESERVED");
            break;
        case VT_ILLEGAL:
            lpszValueForDisplay = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * 11);
            lstrcpy(lpszValueForDisplay, L"VT_ILLEGAL");
            break;
//                case VT_ILLEGALMASKED:
//                    break;
//                case VT_TYPEMASK:
//                    break;
		}
		return lpszValueForDisplay;
	}

	virtual OBJECT_KIND getobjectkind() const {
		return OBJECT_VARIABLE;
	}
};

