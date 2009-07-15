#include "../../global.h"
#include "Keyboard.h"
#include "../../Unicode.h"
#include "../../vm.h"
#include "../../Config.h"

int GetKey(ScriptValue *args) {
	if (args->type == SCRIPT_LIST && args->listVal->numVals) {
		if (args->listVal->vals[0].type == SCRIPT_STRING) {
			if (args->listVal->vals[0].stringVal->len) {
				int junk;
				unsigned long c = NextUnicodeChar(args->listVal->vals[0].stringVal->value, &junk);
				if (c > 0xFFFF) return 0;
				return 0xFF&VkKeyScanW((wchar_t)c);
			}
			return 0;
		}
		else {
			ScriptValue sv;
			CoerceIntNoRelease(args->listVal->vals[0], sv);
			return sv.i32;
		}
	}
	return 0;
}

void ScriptGetKeyState(ScriptValue &s, ScriptValue *args) {
	int i = GetKey(args);
	CreateIntValue(s, GetKeyState(i));
}

void KeyDown(ScriptValue &s, ScriptValue *args) {
	int i = GetKey(args);
	if (i && i <= 254) {
		int extend = 0;
		if (args->type == SCRIPT_LIST && args->listVal->numVals >= 2) {
			ScriptValue sv;
			CoerceIntNoRelease(args->listVal->vals[1], sv);
			if (sv.intVal == 1) {
				extend = KEYEVENTF_EXTENDEDKEY;
			}
		}
		keybd_event(i, 0, extend, 0);
	}
}

void KeyUp(ScriptValue &s, ScriptValue *args) {
	int i = GetKey(args);
	if (i && i <= 254) {
		int extend = KEYEVENTF_KEYUP;
		if (args->type == SCRIPT_LIST && args->listVal->numVals >= 2) {
			ScriptValue sv;
			CoerceIntNoRelease(args->listVal->vals[1], sv);
			if (sv.intVal == 1) {
				extend = KEYEVENTF_KEYUP|KEYEVENTF_EXTENDEDKEY;
			}
		}
		keybd_event(i, 0, extend, 0);
	}
}

HHOOK hook;

struct KeyboardEvents {
	unsigned int total;
	unsigned char keys[16*256];
};

KeyboardEvents *keyEvents = 0;


LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if (hook) {
		KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*) lParam;

		int vk = key->vkCode;
		if (vk != 120 && vk != 116) {
			vk = vk;
		}
		if (vk >= 0 && vk <= 255) {
			if (*((unsigned int*)&keyEvents->keys[vk*16     ]) |
				*((unsigned int*)&keyEvents->keys[vk*16 +  4]) |
				*((unsigned int*)&keyEvents->keys[vk*16 +  8]) |
				*((unsigned int*)&keyEvents->keys[vk*16 + 12])) {

					int flags = 0;
					int shift;
					if (GetKeyState(VK_CONTROL)&0x8000)
						flags = KEY_CONTROL;
					if ((shift = GetKeyState(VK_SHIFT))&0x8000)
						flags |= KEY_SHIFT;
					if (GetKeyState(VK_MENU)&0x8000)
						flags |= KEY_ALT;
					if ((GetKeyState(VK_RWIN) | GetKeyState(VK_LWIN))&0x8000)
						flags |= KEY_WIN;
					if (keyEvents->keys[vk*16+flags]) {
						int down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
						int scancode = key->scanCode;
						unsigned char s[256];
						wchar_t key[5];
						memset(s, 0, 256);
						//int test = GetAsyncKeyState(VK_CAPITAL);
						//GetKeyboardState(s);
						s[VK_SHIFT] = (shift>>8)&0x80;
						s[VK_CAPITAL] = (GetKeyState(VK_CAPITAL))>>8;
						s[VK_NUMLOCK] = (GetKeyState(VK_TAB)>>8);
						s[VK_SCROLL] = (GetKeyState(VK_SCROLL))>>8;
						//s[VK_CONTROL] = (char) GetKeyState(VK_CONTROL);
						int wParam = (vk &0xFFFF) + (flags<<16) + (down<<24);
						//HKL layout = GetKeyboardLayout(0);
						s[vk] = 0x80;
						char temp = ToUnicode(vk, scancode, s, key, 5, 0);
						int lParam = 0;
						if (temp) {
							lParam = key[0];
							if (temp >= 2) {
								lParam = (key[1]<<16);
							}
						}
						PostMessage(ghWnd, WMU_EATEN_KEY, wParam, lParam);
						PostMessage(ghWnd, WM_KEYDOWN, wParam, lParam);
						return 1;
					}
			}
		}
		return CallNextHookEx(hook, code, wParam, lParam);
	}
	return 0;
}

static void UpdateKeyEvents() {
	if (!keyEvents) return;
	if (!keyEvents->total && hook) {
		UnhookWindowsHookEx(hook);
		hook = 0;
		free(keyEvents);
		keyEvents = 0;
	}
	else if (!hook) {
		hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, ghInst, 0);
		if (!hook) errorPrintf(0, "Hook creation failed: %i\n", GetLastError());
	}
}

void CleanKeyEvents() {
	if (keyEvents) {
		UnhookWindowsHookEx(hook);
		hook = 0;
		free(keyEvents);
		keyEvents = 0;
	}
}

void RegisterKeyEvent(ScriptValue &s, ScriptValue *args) {
 	if (args->type != SCRIPT_LIST) return;
	ListValue *list = args->listVal;
	int len = list->numVals;
	if (len < 2) return;
	ScriptValue sv;
	CoerceIntNoRelease(list->vals[0], sv);
	int dCare = (sv.i32>>16)&15;
	sv.intVal -= (dCare<<16);
	if (sv.intVal < 0 || sv.intVal > 15 || (dCare&sv.i32)) return;
	if (!keyEvents) {
		keyEvents = (KeyboardEvents*)calloc(1, sizeof(KeyboardEvents));
		if (!keyEvents) return;
	}
	int state = sv.i32;
	int count = 0;
	for (int j=0; j<16; j++) {
		if (!(j&~dCare)) {
			int state2 = state+j;
			for (int i=1; i < len; i++) {
				CoerceIntNoRelease(list->vals[i], sv);
				if (sv.intVal < 0 || sv.intVal > 255) continue;
				int t = sv.i32*16 + state2;
				if (keyEvents->keys[t] == 255) continue;
				keyEvents->keys[t]++;
				keyEvents->total++;
				count ++;
			}
		}
	}
	UpdateKeyEvents();
	if (count) {
		CreateIntValue(s, count);
	}
}

void UnregisterKeyEvent(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST) return;
	ListValue *list = args->listVal;
	int len = list->numVals;
	if (len < 2) return;
	ScriptValue sv;
	CoerceIntNoRelease(list->vals[0], sv);
	int dCare = (sv.i32>>16)&15;
	sv.intVal -= (dCare<<16);
	if (sv.intVal < 0 || sv.intVal > 15 || (dCare&sv.i32)) return;
	if (!keyEvents) return;
	int state = sv.i32;
	int count = 0;
	for (int j=0; j<16; j++) {
		if (!(j&~dCare)) {
			int state2 = state+j;
			for (int i=1; i < len; i++) {
				CoerceIntNoRelease(list->vals[i], sv);
				if (sv.intVal < 0 || sv.intVal > 255) continue;
				int t = sv.i32*16 + state2;
				if (!keyEvents->keys[t]) continue;
				keyEvents->keys[t]--;
				keyEvents->total--;
				count ++;
			}
		}
	}
	UpdateKeyEvents();
	if (count) {
		CreateIntValue(s, count);
	}
}

