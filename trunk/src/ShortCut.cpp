#include "ShortCut.h"
#include "Config.h"
#include "unicode.h"
#include "stringUtil.h"
#include <shellapi.h>
#include "malloc.h"
#include "vm.h"
ShortcutManager shortcutManager;

void ShortcutManager::ClearShortcuts() {
	for (int i=0; i<numShortcuts; i++) {
		// If already closed the window, no need to unregister.
		if (ghWnd)
			UnregisterHotKey(ghWnd, i);
		free(shortcuts[i].command);
	}
	free(shortcuts);
	shortcuts = 0;
	numShortcuts = 0;
}

void ShortcutManager::HotkeyPressed(WPARAM id) {
	if ((int)id < 0 || (int)id >= numShortcuts) return;
	int l = strlen (shortcuts[id].command);
	if (l && shortcuts[id].command[l-1] == ')') {
		int w = l;
		while (w && shortcuts[id].command[w] != '(') w--;
		if (w) {
			int w2 = w;
			w2--;
			while (w2 && shortcuts[id].command[w] == ' ' || shortcuts[id].command[w] == '\t') w2--;
			if (w2) {
				char t = shortcuts[id].command[w2+1];
				shortcuts[id].command[w2+1] = 0;
				int f = FindFunction(shortcuts[id].command);
				shortcuts[id].command[w2+1] = t;
				if (f >= 0) {
					RunFunction(f);
				}
			}
		}
	}
	//Resolve path == bad.  \\?\<path> doesn't launch explorer window properly.
	wchar_t *path = UTF8toUTF16Alloc(shortcuts[id].command);
	if (path) {
		wchar_t * rPath = path;
		wchar_t * args = 0;
		if (path[0] == '\"') {
			rPath = path + 1;
			args = wcschr(rPath, '\"');
		}
		else {
			args = wcschr(rPath, ' ');
		}
		if (args) {
			args[0] = 0;
			args ++;
		}
		if (args)
			LeftJustify(args);
		//ShellExecuteW(ghWnd, 0, rPath, args, 0, SW_SHOWNORMAL);
		STARTUPINFO si;
		//PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);

		ShellExecuteW(ghWnd, 0, rPath, args, 0, SW_SHOW);
		//CreateProcessW(rPath, args, 0, 0, 0, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);
		//CloseHandle(pi.hProcess);
		//CloseHandle(pi.hThread);
		free(path);
	}
}

void ShortcutManager::Load() {
	unsigned char *key;
	unsigned char *command;
	ClearShortcuts();
	int count = 0;
	while (command = config.GetConfigStringAndKey((unsigned char *)"Shortcuts", key, count)) {
		count++;
		if (command || key) {
			if (key && command && srealloc(shortcuts, sizeof(Shortcut) * (numShortcuts+1))) {
				unsigned int modifiers = 0;
				unsigned int vkey = 0;
				int i = 0;
				while (1) {
					if (key[i] == '@') modifiers |= MOD_WIN;
					else if (key[i] == '!') modifiers |= MOD_ALT;
					else if (key[i] == '^') modifiers |= MOD_CONTROL;
					else if (key[i] == '+') modifiers |= MOD_SHIFT;
					else break;
					i++;
				}
				if (key[i+1] == 0) {
					short code = VkKeyScan(key[i]);
					if (code != 0xFFFF) {
						vkey = code & 0xFF;
					}
					/*if ((UCASE(key[i] >= 'A') && UCASE(key[i] <= 'Z')) || (key[i] >= '0' && key[i] <= '9'))
						vkey = UCASE(key[i]);
					if ((UCASE(key[i] >= 'A') && UCASE(key[i] <= 'Z')) || (key[i] >= '0' && key[i] <= '9'))
					//*/
				}
				else if (CompareSubstringNoCase(key+i, (unsigned char*)"num") == 0) {
					if (CompareSubstringNoCase(key+i, (unsigned char*)"pad") == 0)
						i+=3;
					if (key[i+1] == 0 && '0' <= key[i] && key[i] <= '9') {
						vkey = VK_NUMPAD0 + key[i] - '0';
					}
					else if (stricmp(key+i, (unsigned char*) "*") == 0 || stricmp(key+i, (unsigned char*) "multiply") == 0 || stricmp(key+i, (unsigned char*) "mul") == 0 || stricmp(key+i, (unsigned char*) "times") == 0 || stricmp(key+i, (unsigned char*) "star") == 0)
						vkey = VK_ADD;
					else if (stricmp(key+i, (unsigned char*) "+") == 0 || stricmp(key+i, (unsigned char*) "plus") == 0 || stricmp(key+i, (unsigned char*) "add") == 0)
						vkey = VK_ADD;
					else if (stricmp(key+i, (unsigned char*) "-") == 0 || stricmp(key+i, (unsigned char*) "minus") == 0 || stricmp(key+i, (unsigned char*) "sub") == 0 || stricmp(key+i, (unsigned char*) "subtract") == 0)
						vkey = VK_SUBTRACT;
					else if (stricmp(key+i, (unsigned char*) "/") == 0 || stricmp(key+i, (unsigned char*) "divide") == 0 || stricmp(key+i, (unsigned char*) "div") == 0 || stricmp(key+i, (unsigned char*) "slash") == 0)
						vkey = VK_DIVIDE;
					else if (stricmp(key+i, (unsigned char*) ".") == 0 || stricmp(key+i, (unsigned char*) "dec") == 0 || stricmp(key+i, (unsigned char*) "decimal") == 0)
						vkey = VK_DECIMAL;
					else if (stricmp(key+i, (unsigned char*) ".") == 0 || stricmp(key+i, (unsigned char*) "dec") == 0 || stricmp(key+i, (unsigned char*) "decimal") == 0)
						vkey = VK_DECIMAL;
					else if (stricmp(key+i, (unsigned char*) "lock") == 0 || key[i] == 0)
						vkey = VK_NUMLOCK;
				}
				else if (key[i] == 'f' || key[i] == 'F') {
					i++;
					if ('0' <= key[i] && key[i] <= '9' && (key[i+1] == 0 || (key[i+2] == 0 && '0' <= key[i+1] && key[i+1] <= '9'))) {
						int w = (int) strtoi64((char*)key+i, 0, 0);
						if (w > 0 && w <=24) {
							// 24 f keys?  There are key codes for them...apparently.
							vkey = VK_F1 + w;
						}
					}
				}
				else if (stricmp(key+i, (unsigned char*) "space") == 0)
					vkey = VK_SPACE;
				else if (stricmp(key+i, (unsigned char*) "esc") == 0 || stricmp(key+i, (unsigned char*) "escape") == 0)
					vkey = VK_ESCAPE;
				else if (stricmp(key+i, (unsigned char*) "pgup") == 0 || stricmp(key+i, (unsigned char*) "pageup") == 0)
					vkey = VK_PRIOR;
				else if (stricmp(key+i, (unsigned char*) "pgdn") == 0 || stricmp(key+i, (unsigned char*) "pagedn") == 0 || stricmp(key+i, (unsigned char*) "pagedown") == 0)
					vkey = VK_NEXT;
				else if (stricmp(key+i, (unsigned char*) "end") == 0)
					vkey = VK_END;
				else if (stricmp(key+i, (unsigned char*) "home") == 0)
					vkey = VK_HOME;
				else if (stricmp(key+i, (unsigned char*) "left") == 0)
					vkey = VK_LEFT;
				else if (stricmp(key+i, (unsigned char*) "right") == 0)
					vkey = VK_RIGHT;
				else if (stricmp(key+i, (unsigned char*) "down") == 0)
					vkey = VK_DOWN;
				else if (stricmp(key+i, (unsigned char*) "up") == 0)
					vkey = VK_UP;
				else if (stricmp(key+i, (unsigned char*) "print") == 0 || stricmp(key+i, (unsigned char*) "printscreen") == 0)
					vkey = VK_SNAPSHOT;
				else if (stricmp(key+i, (unsigned char*) "ins") == 0 || stricmp(key+i, (unsigned char*) "insert") == 0)
					vkey = VK_INSERT;
				else if (stricmp(key+i, (unsigned char*) "del") == 0 || stricmp(key+i, (unsigned char*) "delete") == 0)
					vkey = VK_DELETE;
				else if (stricmp(key+i, (unsigned char*) "help") == 0)
					vkey = VK_HELP;
				else if (stricmp(key+i, (unsigned char*) "pause") == 0)
					vkey = VK_PAUSE;
				else if (stricmp(key+i, (unsigned char*) "caps") == 0 || stricmp(key+i, (unsigned char*) "capslock") == 0)
					vkey = VK_CAPITAL;
				else if (stricmp(key+i, (unsigned char*) "tab") == 0)
					vkey = VK_TAB;
				else if (stricmp(key+i, (unsigned char*) "scroll") == 0 || stricmp(key+i, (unsigned char*) "scrolllock") == 0)
					vkey = VK_SCROLL;
				else if (stricmp(key+i, (unsigned char*) "back") == 0 || stricmp(key+i, (unsigned char*) "backspace") == 0)
					vkey = VK_BACK;
				else if (stricmp(key+i, (unsigned char*) "clear") == 0 || stricmp(key+i, (unsigned char*) "cls") == 0 || stricmp(key+i, (unsigned char*) "clearscreen") == 0)
					vkey = VK_CLEAR;
				else if (stricmp(key+i, (unsigned char*) "enter") == 0 || stricmp(key+i, (unsigned char*) "resturn") == 0)
					vkey = VK_RETURN;
				else if (stricmp(key+i, (unsigned char*) "shift") == 0)
					vkey = VK_SHIFT;
				else if (stricmp(key+i, (unsigned char*) "control") == 0 || stricmp(key+i, (unsigned char*) "ctrl") == 0)
					vkey = VK_MENU;
				if (vkey) {
					if (RegisterHotKey(ghWnd, numShortcuts, modifiers, vkey)) {
						shortcuts[numShortcuts].flags = modifiers;
						shortcuts[numShortcuts].vkey = vkey;
						shortcuts[numShortcuts++].command = command;
						LeftJustify(command);
						continue;
					}
				}
			}
			if (command)
				free(command);
			continue;
		}
		break;
	}
}
