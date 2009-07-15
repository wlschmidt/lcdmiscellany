#include "ScriptProcessManagement.h"
#include "../../global.h"
#include <shellapi.h>
#include "../../util.h"
#include "../../stringUtil.h"
#include "../../unicode.h"

void Run(ScriptValue &s, ScriptValue* args) {
	wchar_t *path = ResolvePath(args[0].stringVal->value);
	wchar_t *dir = 0;
	if (path && (!args[3].stringVal->len || (dir = ResolvePath(args[3].stringVal->value)))) {
		wchar_t *params = UTF8toUTF16Alloc(args[1].stringVal->value);
		if (params) {
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			memset(&si, 0, sizeof(si));
			si.cb = sizeof(si);
			wchar_t *params2 = 0;
			if (params[0]) params2 = params;
			if (args[2].intVal == 2) {
				HINSTANCE res = ShellExecuteW(ghWnd, 0, path, params, dir, SW_SHOW);
				if ((size_t)res > 32) {
					CreateIntValue(s, 1);
				}
			}
			else if (CreateProcessW(path, params, 0, 0, 0, CREATE_DEFAULT_ERROR_MODE, 0, dir, &si, &pi)/* && CreateListValue(s, 4)//*/) {
				if (args[2].intVal) {
					WaitForInputIdle(pi.hProcess, INFINITE);
				}
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				CreateIntValue(s, pi.dwProcessId);
				/*s.listVal->PushBack((__int64)pi.hProcess);
				s.listVal->PushBack(pi.dwThreadId);
				s.listVal->PushBack((__int64)pi.hTread);
				//*/
			}

			//ShellExecuteW(ghWnd, 0, path, params, 0, SW_SHOWNORMAL);CreateProcessW(
			free(params);
		}
	}
	free(path);
	free(dir);
}

void RunSimple(ScriptValue &s, ScriptValue* args) {
	wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value);
	if (path) {
		wchar_t *params = UTF8toUTF16Alloc(args[1].stringVal->value);
		if (params) {
			LeftJustify(path);

			//ShellExecuteW(ghWnd, 0, path, params, 0, SW_SHOWNORMAL);
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			memset(&si, 0, sizeof(si));
			si.cb = sizeof(si);

			if (CreateProcessW(path, params, 0, 0, 0, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi)) {
				if (args[2].intVal) {
					WaitForInputIdle(pi.hProcess, INFINITE);
				}
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				CreateIntValue(s, pi.dwProcessId);
			}
			free(params);
		}
		free(path);
	}
}


