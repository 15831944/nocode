#pragma once

#define MONITOR_CENTER     0x0001        // center rect to monitor 
#define MONITOR_CLIP     0x0000        // clip rect to monitor
#define MONITOR_WORKAREA 0x0002        // use monitor work area
#define MONITOR_AREA     0x0000        // use monitor entire area

class util
{
public:
	static void DrawArrow(HDC hdc, const POINT* p1, const POINT* p2, const double* z);
	static void ClipOrCenterRectToMonitor(LPRECT prc, UINT flags);
};

