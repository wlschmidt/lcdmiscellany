#include "MonitorScript.h"
#include "../../Global.h"

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	ScriptValue sv;
	MONITORINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	if (GetMonitorInfo(hMonitor, &info) && CreateListValue(sv, 4)) {
		ScriptValue sv2;
		CreateIntValue(sv2, info.rcMonitor.left);
		sv.listVal->PushBack(sv2);
		CreateIntValue(sv2, info.rcMonitor.top);
		sv.listVal->PushBack(sv2);
		CreateIntValue(sv2, info.rcMonitor.right);
		sv.listVal->PushBack(sv2);
		CreateIntValue(sv2, info.rcMonitor.bottom);
		sv.listVal->PushBack(sv2);
		((ScriptValue*)dwData)->listVal->PushBack(sv);
	}
	return 1;
}

void ScriptEnumDisplayMonitors(ScriptValue &s, ScriptValue *args) {
	if (CreateListValue(s, 4)) {
		RECT r;
		RECT *pr = 0;
		if (args[0].i32 | args[1].i32 | args[2].i32 | args[3].i32) {
			pr = &r;
			r.left = args[0].i32;
			r.top = args[1].i32;
			r.right = args[2].i32;
			r.bottom = args[3].i32;
		}
		EnumDisplayMonitors(0, pr, MonitorEnumProc, (LPARAM)&s);
		if (s.listVal->numVals == 0) {
			s.listVal->Release();
			CreateNullValue(s);
		}
	}
}
