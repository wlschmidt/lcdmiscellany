#include "ScriptEmail.h"
#include "../../global.h"

int unreadEmailCount = 0;
HKEY emailKey = 0;
HANDLE emailEvent = 0;

void CleanupEmail() {
	if (emailKey) {
		RegCloseKey(emailKey);
		emailKey = 0;
	}
	if (emailEvent) {
		CloseHandle(emailEvent);
		emailEvent = 0;
	}
}

void GetEmailCount(ScriptValue &s, ScriptValue *args) {
	if (emailKey && emailEvent) {
		int test = WaitForSingleObject(emailEvent, 0);
		if (test == WAIT_OBJECT_0) {
			unreadEmailCount = 0;
			wchar_t subkey[2000];
			// Don't tust Microsoft.
			subkey[1999] = 0;
			DWORD size;
			for (int i = 0; ERROR_SUCCESS == RegEnumKeyExW(emailKey, i, subkey, &(size=1999),0,0,0,0); i++) {
				HKEY key2;
				if (ERROR_SUCCESS == RegOpenKeyExW(emailKey, subkey, 0, KEY_READ, &key2)) {
					__int64 inc = 0;
					DWORD junk = 8;
					DWORD type;
					if (ERROR_SUCCESS == RegQueryValueExW(key2, L"MessageCount", 0, &type, (BYTE*)&inc, &junk) &&
						(type == REG_DWORD || type == REG_QWORD)) {
						unreadEmailCount+=(int)inc;
					}
					RegCloseKey(key2);
				}
			}
			if (ERROR_SUCCESS != RegNotifyChangeKeyValue(emailKey, 1, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, emailEvent, 1)) {
				RegCloseKey(emailKey);
				emailKey = 0;
				CloseHandle(emailEvent);
				emailEvent = 0;
			}
			CreateIntValue(s, unreadEmailCount);
			return;
		}
		if (test == WAIT_TIMEOUT) {
			CreateIntValue(s, unreadEmailCount);
			return;
		}
		RegCloseKey(emailKey);
		emailKey = 0;
		CloseHandle(emailEvent);
		emailEvent = 0;
	}
	if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail", 0, KEY_READ, &emailKey)) {
		emailKey = 0;
		return;
	}
	emailEvent = CreateEvent(0, 0, 0, 0);
	if (!emailEvent) {
		RegCloseKey(emailKey);
		emailKey = 0;
		return;
	}
	if (ERROR_SUCCESS != RegNotifyChangeKeyValue(emailKey, 1, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, emailEvent, 1)) {
		RegCloseKey(emailKey);
		emailKey = 0;
		CloseHandle(emailEvent);
		emailEvent = 0;
		return;
	}
	SetEvent(emailEvent);
	return;
}

