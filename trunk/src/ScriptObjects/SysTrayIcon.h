#ifndef SCRIPT_TRAY_ICON_H
#define SCRIPT_TRAY_ICON_H

#include "../ScriptValue.h"
#include "../Device.h"
#include "../unicode.h"


void SysTrayIcon(ScriptValue &s, ScriptValue *args);
void SysTrayIconNeedRedraw(ScriptValue &s, ScriptValue *args);
void SysTrayIconFree(ScriptValue &s, ScriptValue *args);
void NeedRedrawSysTrayIcons(ScriptValue &s, ScriptValue *args);

void UninitIcons();

extern unsigned int SysTrayIconType;


class SysTrayIconDevice : public Device {
	HICON hIcon;
	wchar_t *wid;
public:
	int iid;
	int funcId;
	// Function object.
	ObjectValue *obj;

	// Script object to control device.
	ScriptValue objectValue;

	SysTrayIconDevice(unsigned char *id, int iid, int funcId, ObjectValue *obj) : Device(16, 16, 32, CONNECTION_SYSTRAY_ICON, SYSTRAY_ICON, (char*) id) {
		this->funcId = funcId;

		if (obj) obj->AddRef();
		this->obj = obj;

		this->iid = iid;
		wid = UTF8toUTF16Alloc((unsigned char *) id);
		int len = (int)wcslen(wid);
		if (len >= 127) len = 127;
		wid[len] = 0;
		this->sendImage = 1;
		hIcon = 0;
		CreateNullValue(objectValue);
	}

	HICON MakeIcon();

	int UpdateImage();
	void RemoveTrayIcon();

	inline ~SysTrayIconDevice() {
		if (objectValue.objectVal) {
			objectValue.objectVal->values[0].intVal = 0;
			objectValue.Release();
		}
		if (obj) obj->Release();
		RemoveTrayIcon();
		free(wid);
	}
};

void UpdateIcons();
void HandleIconMessage(SysTrayIconDevice *dev, LPARAM lParam);

#endif
