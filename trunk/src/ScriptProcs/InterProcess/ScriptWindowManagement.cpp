#include "ScriptWindowManagement.h"
#include "../../global.h"
#include <Psapi.h>
#include "../../unicode.h"
#include "../../util.h"
void GetForegroundWindow(ScriptValue &s, ScriptValue *args) {
	__int64 i = (__int64)GetForegroundWindow();
	if (i)
		CreateIntValue(s, i);
}

void MyGetWindowText(ScriptValue &s, ScriptValue *args) {
	int len = GetWindowTextLengthW((HWND)args[0].intVal);
	if (len > 0) {
		len += 200;
		wchar_t *temp = (wchar_t*) malloc(sizeof(wchar_t) * len);
		int len2 = GetWindowTextW((HWND)args[0].intVal, temp, len);
		CreateStringValue(s, temp, len2);
		free(temp);
	}
}

void GetWindowClientRect(ScriptValue &s, ScriptValue *args) {
	RECT r;
	if (GetClientRect((HWND)args[0].intVal, &r) &&
		MapWindowPoints((HWND)args[0].intVal, 0, (POINT*)&r, 2) &&
		CreateListValue(s, 4)) {

			ScriptValue sv2;
			CreateIntValue(sv2, r.left);
			s.listVal->PushBack(sv2);
			CreateIntValue(sv2, r.top);
			s.listVal->PushBack(sv2);
			CreateIntValue(sv2, r.right);
			s.listVal->PushBack(sv2);
			CreateIntValue(sv2, r.bottom);
			s.listVal->PushBack(sv2);
	}
}

void ScriptGetWindowRect(ScriptValue &s, ScriptValue *args) {
	RECT r;
	if (GetWindowRect((HWND)args[0].intVal, &r) && CreateListValue(s, 4)) {
		ScriptValue sv2;
		CreateIntValue(sv2, r.left);
		s.listVal->PushBack(sv2);
		CreateIntValue(sv2, r.top);
		s.listVal->PushBack(sv2);
		CreateIntValue(sv2, r.right);
		s.listVal->PushBack(sv2);
		CreateIntValue(sv2, r.bottom);
		s.listVal->PushBack(sv2);
	}
}

void MyGetProcessFileName(ScriptValue &s, ScriptValue *args) {
	wchar_t temp[MAX_PATH*2];
	unsigned char *name;
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, args[0].i32);
	if (h == 0) {
		getDebugPriv();
		h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, args[0].i32);
	}
	if (!h) return;
	int len = GetModuleFileNameEx(h, 0, temp, MAX_PATH*2);
	CloseHandle(h);

	if (len > 0 && len < MAX_PATH*2 && (name = UTF16toUTF8Alloc(temp))) {
		unsigned char *name2 = name;
		if (args[1].intVal == 0) {
			unsigned char * temp = name;
			while (*temp) {
				if (*temp == '\\') name2 = temp;
				temp++;
			}
		}
		CreateStringValue(s, name2);
		free(name);
	}
}

void MyGetWindowModuleFileName(ScriptValue &s, ScriptValue *args) {
	wchar_t temp[MAX_PATH*2];
	unsigned char *name;
	DWORD id;
	if (GetWindowThreadProcessId((HWND)args[0].intVal, &id)) {
		HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, id);
		if (h == 0) {
			getDebugPriv();
			h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, id);
		}
		if (!h) return;
		int len = GetModuleFileNameEx(h, 0, temp, MAX_PATH*2);
		CloseHandle(h);

		if (len > 0 && len < MAX_PATH*2 && (name = UTF16toUTF8Alloc(temp))) {
			unsigned char *name2 = name;
			if (args[1].intVal == 0) {
				unsigned char * temp = name;
				while (*temp) {
					if (*temp == '\\') name2 = temp+1;
					temp++;
				}
			}
			CreateStringValue(s, name2);
			free(name);
		}
	}
}

void CloseWindow(ScriptValue &s, ScriptValue *args) {
	PostMessage((HWND)args[0].intVal, WM_CLOSE, 0,0);
}
void GetWindowProcessId (ScriptValue &s, ScriptValue *args) {
	DWORD test = 0;
	if (!GetWindowThreadProcessId((HWND)args[0].intVal, &test)) {
		CreateIntValue(s, 0);
	}
	else
		CreateIntValue(s, test);
}

void KillProcess(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, KillProcess((int)args[0].intVal));
}

void ScriptPostMessage(ScriptValue &s, ScriptValue *args) {
	if ((HWND)args[0].intVal) {
		CreateIntValue(s, PostMessage((HWND)args[0].intVal, (UINT)args[1].intVal, (WPARAM)args[2].intVal, (LPARAM)args[3].intVal));
	}
}

void ScriptSendMessage(ScriptValue &s, ScriptValue *args) {
	if ((HWND)args[0].intVal) {
		CreateIntValue(s, SendMessage((HWND)args[0].intVal, (UINT)args[1].intVal, (WPARAM)args[2].intVal, (LPARAM)args[3].intVal));
	}
}

void ScriptIsWindow(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, IsWindow((HWND)args[0].intVal));
}

struct EnumChildParams {
	int visible;
	ScriptValue sv;
};

BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam) {
	EnumChildParams *params = (EnumChildParams*) lParam;
	if (!params->visible || IsWindowVisible(hWnd)) {
		params->sv.listVal->PushBack((__int64)hWnd);
	}
	return 1;
}

void ScriptEnumChildWindows(ScriptValue &s, ScriptValue *args) {
	EnumChildParams params;
	params.visible = args[1].i32;
	if (CreateListValue(params.sv, 100)) {
		EnumChildWindows((HWND)args[0].intVal, EnumChildProc, (LPARAM)&params);
		s = params.sv;
	}
}

void ScriptEnumWindows(ScriptValue &s, ScriptValue *args) {
	EnumChildParams params;
	memset(&params, 0, sizeof(params));
	if (CreateListValue(params.sv, 100)) {
		EnumChildWindows(0, EnumChildProc, (LPARAM)&params);
		s = params.sv;
	}
}

void GetScriptWindow(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (__int64) ghWnd);
}

void ScriptFindWindow(ScriptValue &s, ScriptValue *args) {
	wchar_t *p1 = 0, *p2 = 0;
	if (args[0].stringVal->len) {
		p1 = UTF8toUTF16Alloc(args[0].stringVal->value);
	}
	if (args[1].stringVal->len) {
		p2 = UTF8toUTF16Alloc(args[1].stringVal->value);
	}
	__int64 res = (__int64)FindWindowExW((HWND)args[2].intVal, (HWND)args[3].intVal, p1, p2);
	if (res) {
		CreateIntValue(s, res);
	}
	if (p1) free(p1);
	if (p2) free(p2);
}


const static int priority[] = {
	IDLE_PRIORITY_CLASS,
	BELOW_NORMAL_PRIORITY_CLASS,
	NORMAL_PRIORITY_CLASS,
	ABOVE_NORMAL_PRIORITY_CLASS,
	HIGH_PRIORITY_CLASS,
	REALTIME_PRIORITY_CLASS
};

void ScriptSetProcessPriority(ScriptValue &s, ScriptValue *args) {
	int p = args[1].i32;
	if (p >= 0 && p < 6) {
		p = priority[p];
		CreateIntValue(s, SetProcessPriority(args[0].i32, p));
	}
	else {
		CreateIntValue(s, 0);
	}
}

void ScriptGetProcessPriority(ScriptValue &s, ScriptValue *args) {
	int p;
	if (GetProcessPriority(args[0].i32, p)) {
		int i;
		for (i=0; i<6; i++) {
			if (p == priority[i]) {
				CreateIntValue(s, i);
				return;
			}
		}
	}
}

