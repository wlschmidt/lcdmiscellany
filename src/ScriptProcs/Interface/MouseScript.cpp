#include "MouseScript.h"
#include "../../global.h"
#include "../../vm.h"
#include "../../unicode.h"

void ScriptSetCursorPos(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, SetCursorPos(args[0].i32, args[1].i32));
}

void ScriptGetCursorPos(ScriptValue &s, ScriptValue *args) {
	POINT p;
	if (GetCursorPos(&p) && CreateListValue(s, 2)) {
		CreateIntValue(s.listVal->vals[0], p.x);
		CreateIntValue(s.listVal->vals[1], p.y);
		s.listVal->numVals = 2;
	}
}

void ScriptClipCursor(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, ClipCursor(0));
	if (args[0].i32 ||
		args[1].i32 ||
		args[2].i32 ||
		args[3].i32) {
		RECT r;
		r.left = args[0].i32;
		r.top = args[1].i32;
		r.right = args[2].i32;
		r.bottom = args[3].i32;
		CreateIntValue(s, ClipCursor(&r));
	}
}

void MouseMove(ScriptValue &s, ScriptValue *args) {
	unsigned int v = (unsigned int)args[2].i32;
	if (!(v & MOUSEEVENTF_ABSOLUTE)) v |= MOUSEEVENTF_MOVE;
	mouse_event(v, args[0].i32, args[1].i32, args[3].i32, args[4].i32);
}

void MouseDown(ScriptValue &s, ScriptValue *args) {
	int button;
	int button2 = 0;
	switch(args[0].intVal) {
		case 0:
		case 1:
			button = MOUSEEVENTF_LEFTDOWN;
			break;
		case 2:
			button = MOUSEEVENTF_RIGHTDOWN;
			break;
		case 3:
			button = MOUSEEVENTF_MIDDLEDOWN;
			break;
		case 4:
			button = MOUSEEVENTF_XDOWN;
			button2 = XBUTTON1;
			break;
		case 5:
			button = MOUSEEVENTF_XDOWN;
			button2 = XBUTTON2;
			break;
		default:
			return;
	}
	mouse_event(button, 0, 0, button2, 0);
}

void MouseUp(ScriptValue &s, ScriptValue *args) {
	int button;
	int button2 = 0;
	switch(args[0].intVal) {
		case 0:
		case 1:
			button = MOUSEEVENTF_LEFTUP;
			break;
		case 2:
			button = MOUSEEVENTF_RIGHTUP;
			break;
		case 3:
			button = MOUSEEVENTF_MIDDLEUP;
			break;
		case 4:
			button = MOUSEEVENTF_XUP;
			button2 = XBUTTON1;
			break;
		case 5:
			button = MOUSEEVENTF_XUP;
			button2 = XBUTTON2;
			break;
		default:
			return;
	}
	mouse_event(button, 0, 0, button2, 0);
}

void MouseScroll(ScriptValue &s, ScriptValue *args) {
	int wheel = MOUSEEVENTF_WHEEL;
	if (args[1].i32 == 1) {
		wheel = MOUSEEVENTF_HWHEEL;
	}
	mouse_event(wheel, 0, 0, args[0].i32 * WHEEL_DELTA, 0);
}

struct ContextMenuThreadData {
	HMENU hMenu;
	Stack *stack;
	int x, y;
};

void ContextMenuProc(void *data) {
	ContextMenuThreadData *threadData = (ContextMenuThreadData*) data;

	SetForegroundWindow(ghWnd);
	int res = TrackPopupMenu(threadData->hMenu, TPM_NONOTIFY|TPM_RETURNCMD, threadData->x, threadData->y, 0, ghWnd, 0)-1;
	PostMessage(ghWnd, WM_NULL, 0, 0);

	DestroyMenu(threadData->hMenu);

	threadData->stack->PushInt(res);
	RunStack(threadData->stack);

	free(threadData);
}

int ContextMenuWait(ScriptValue *args, Stack *stack) {
	if (args->type == SCRIPT_LIST && args->listVal->numVals > 2) {
		HMENU hMenu = CreatePopupMenu();
		ScriptValue sv;

		if (hMenu) {
			MENUITEMINFO info;
			memset(&info, 0, sizeof(info));
			info.cbSize = sizeof(info);

			for (int i=2; i<args->listVal->numVals; i++) {
				args->listVal->vals[i].AddRef();
				CoerceString(args->listVal->vals[i], sv);
				wchar_t *string = UTF8toUTF16Alloc(sv.stringVal->value);
				sv.Release();
				if (!string[0] || (string[0] == '-' && !string[1])) {
					info.fType = MFT_SEPARATOR;
					info.fMask = MIIM_FTYPE;
				}
				else {
					info.fMask = MIIM_STRING | MIIM_ID;
					info.wID = i-1;
					info.cch = (UINT) wcslen(string);
					info.dwTypeData = string;
				}
				InsertMenuItem(hMenu, i, 1, &info);
				free(string);
			}
			ContextMenuThreadData *data = (ContextMenuThreadData*) malloc(sizeof(ContextMenuThreadData));
			if (data) {
				data->hMenu = hMenu;
				data->stack = stack;

				CoerceIntNoRelease(args->listVal->vals[0], sv);
				data->x = sv.i32;
				CoerceIntNoRelease(args->listVal->vals[1], sv);
				data->y = sv.i32;

				if (PostMessage(ghWnd, WMU_CALL_PROC, (WPARAM)data, (LPARAM)ContextMenuProc)) {
					return 1;
				}
				free(data);
			}
			DestroyMenu(hMenu);
		}
	}
	stack->PushInt(-1);
	return -1;
}
