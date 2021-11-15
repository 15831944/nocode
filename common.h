#pragma once

#include <Windows.h>
#include "graphic.h"

class common {
public:
	HFONT hUIFont;
	HWND hTool;
	HWND hPropContainer;
	HWND hNodeBox;
	HWND hMainWnd;
	HWND hOutput;
	HWND hVariableList;
	HWND hList;
	UINT uDpiX;
	UINT uDpiY;
	graphic* g;
	void* app;
	bool bShowGrid;
	common()
		: hTool(0)
		, hUIFont(0)
		, hPropContainer(0)
		, hNodeBox(0)
		, hMainWnd(0)
		, hOutput(0)
		, hVariableList(0)
		, hList(0)
		, uDpiX(DEFAULT_DPI)
		, uDpiY(DEFAULT_DPI)
		, g(0)
		, app(0)
		, bShowGrid(TRUE)
	{
		//CreateFont
	}
	~common() {
	}
};

extern common g_c;