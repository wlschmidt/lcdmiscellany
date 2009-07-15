#include "SysTrayIcon.h"
#include "../VM.h"
#include "../ScriptObjects/ScriptObjects.h"

unsigned int SysTrayIconType;
SysTrayIconDevice *CreateSysTrayIconDevice(char *name, ScriptValue &funcId, ObjectValue *obj);

HICON SysTrayIconDevice::MakeIcon() {
	Color4 xor[256];
	Color4 and[256];
	for (int i=0; i<256; i++) {
		and[i].a = and[i].b = and[i].g = and[i].r = image[i].a ^ 0xFF;
		xor[i] = image[i];
	}
	return CreateIcon(ghInst, 16, 16, 1, 32, (BYTE*)and, (BYTE*) xor);
}

int SysTrayIconDevice::UpdateImage() {
	HICON hIcon2 = MakeIcon();
	if (!hIcon2) return 0;

	NOTIFYICONDATA nData;
	memset(&nData, 0, sizeof(NOTIFYICONDATA));
	
	nData.cbSize = sizeof(NOTIFYICONDATA);
	nData.hWnd = ghWnd;
	nData.uID = iid;
	nData.hIcon = hIcon2;
	nData.uCallbackMessage = WMA_ICON_MESSAGE;

	wcscpy(nData.szTip, wid);
	nData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	if (!hIcon) {
		if(Shell_NotifyIcon(NIM_ADD, &nData) == 0) {
			DestroyIcon(hIcon2);
			return 0;
		}
	}
	else {
		if(Shell_NotifyIcon(NIM_MODIFY, &nData) == 0) {
			DestroyIcon(hIcon2);
			return 0;
		}
		DestroyIcon(hIcon);
	}
	hIcon = hIcon2;
	return 1;
}

void SysTrayIconDevice::RemoveTrayIcon() {
	if (hIcon) {
		NOTIFYICONDATA nData;

		nData.cbSize = sizeof(nData);
		nData.hWnd = ghWnd;
		nData.uID = iid;
		
		Shell_NotifyIcon(NIM_DELETE, &nData);
		DestroyIcon(hIcon);
	}
	hIcon = 0;
}

SysTrayIconDevice *CreateSysTrayIconDevice(unsigned char *name, ScriptValue &funcId, ObjectValue *obj) {
	int q = IDI_FROG+1;
	int delta = 1;
	while (delta) {
		delta = 0;
		for (int i=0; i<numDevices; i++) {
			if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
				if (((SysTrayIconDevice*)devices[i])->iid == q) {
					q++;
					delta = 1;
				}
			}
		}
	}

	int f = -1;
	if (obj) {
		int v = VM.StringTable.Find(funcId.stringVal);
		if (v >= 0) {
			f = types[obj->type].FindFunction(v);
		}
	}
	else {
		TableEntry<Function> *func = VM.Functions.Find(funcId.stringVal);
		if (func) f = func->index;
	}
	if (f == -1) return 0;

	SysTrayIconDevice *device = new SysTrayIconDevice(name, q, f, obj);
	AddDevice(device);
	return device;
}

void UpdateIcons() {
	for (int i = 0; i<numDevices; i++) {
		if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
			SysTrayIconDevice* dev = (SysTrayIconDevice*)devices[i];
			dev->RemoveTrayIcon();
			dev->UpdateImage();
		}
	}
}

void SysTrayIcon(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = 0;
	if (args[2].type == SCRIPT_OBJECT) {
		obj = args[2].objectVal;
	}
	SysTrayIconDevice *dev = CreateSysTrayIconDevice(args[0].stringVal->value, args[1], obj);
	if (dev) {
		if (CreateObjectValue(s, SysTrayIconType)) {
			CreateIntValue(s.objectVal->values[0], (__int64)dev);
			dev->objectValue = s;
			s.AddRef();
		}
		else {
			DeleteDevice(dev);
		}
	}
	return;
}

void SysTrayIconNeedRedraw(ScriptValue &s, ScriptValue *args) {
	SysTrayIconDevice *dev = (SysTrayIconDevice*) s.objectVal->values[0].intVal;
	if (dev) {
		if (!dev->needRedraw) {
			dev->needRedraw = 1;
			PostMessage(ghWnd, WM_APP, 2, 2);
		}
	}
	CreateNullValue(s);
	return;
}

// Used both for cleanup and as Destroy function.
void SysTrayIconFree(ScriptValue &s, ScriptValue *args) {
	SysTrayIconDevice *dev = (SysTrayIconDevice*) s.objectVal->values[0].intVal;
	if (dev) {
		DeleteDevice(dev);
	}
	CreateNullValue(s);
	// Not needed, done by deleting device.
	// s.objectVal->values[0].intVal = 0;
}

void NeedRedrawSysTrayIcons(ScriptValue &s, ScriptValue *args) {
	int postMessage = 0;
	for (int i=0; i<numDevices; i++) {
		if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
			postMessage |= !devices[i]->needRedraw;
			devices[i]->needRedraw = 1;
		}
	}
	if (postMessage)
		PostMessage(ghWnd, WM_APP, 2, 2);
}

void UninitIcons() {
	for (int i=0; i<numDevices;) {
		if (devices[i]->cType == CONNECTION_SYSTRAY_ICON) {
			DeleteDevice(devices[i]);
		}
		else i++;
	}
}

void HandleIconMessage(SysTrayIconDevice *dev, LPARAM lParam) {
	ScriptValue list, string;
	if (CreateListValue(list, 4)) {
		CreateStringValue(string, "WindowsMessage");
		list.listVal->PushBack(string);

		CreateNullValue(string);
		list.listVal->PushBack(string);

		dev->objectValue.AddRef();
		list.listVal->PushBack(dev->objectValue);

		list.listVal->PushBack(lParam);

		if (dev->obj) dev->obj->AddRef();
		RunFunction(dev->funcId, list, CAN_WAIT, dev->obj);
	}
}

