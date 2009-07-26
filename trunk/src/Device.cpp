#include "global.h"
#include "Device.h"
#include <time.h>
#include <stdio.h>
#include "sdk/v3.01/lgLcd.h"
#include "sdk/v1.03/lgLcd.h"
#include "StringUtil.h"

Device::Device(int width, int height, int bpp, ConnectionType cType, HID_DEVICE_TYPE hidType, char *id) : Screen(width, height, bpp) {
	this->bpp = bpp;
	this->cType = cType;
	this->hidType = hidType;
	gKeyState = 0;
	needRedraw = 1;
	sendImage = 0;
	ScriptValue string;
	g15 = 0;
	CreateStringValue(string, id);
	this->id = string.stringVal;
}

Device::~Device() {
	id->Release();
}

Device **devices = 0;
int numDevices = 0;

ConnectionType sdkType = CONNECTION_NULL;
int connection = LGLCD_INVALID_CONNECTION;

class SDKDevice : public Device {
public:
	int deviceID;
	SDKDevice (int deviceID, int width, int height, int bpp, ConnectionType cType, HID_DEVICE_TYPE hidType, char* idString) :
		Device(width, height, bpp, cType, hidType, idString) {
			this->deviceID = deviceID;
	}

	~SDKDevice() {
		if (cType == CONNECTION_SDK301) {
			lgLcdClose(deviceID);
		}
		else if (cType == CONNECTION_SDK103) {
			lgLcdClose(deviceID);
		}
	}
	int UpdateImage() {
		lgLcdBitmap bmp;
		HRESULT res;
		if (width == 160 && height == 43 && bpp == 1) {
			bmp.hdr.Format = LGLCD_BMP_FORMAT_160x43x1;
			for (int p = 43*160-1; p>=0; p--) {
				bmp.bmp_mono.pixels[p] = (((short)image[p].r + (short)image[p].g + (short)image[p].b)<128*3-1)<<7;
			}
			if (sdkType == CONNECTION_SDK301) {
				res = lgLcdUpdateBitmap(deviceID, &bmp.hdr, LGLCD_PRIORITY_NORMAL);
			}
			else if (sdkType == CONNECTION_SDK103) {
				res = loLcdUpdateBitmap(deviceID, (loLcdBitmapHeader*)&bmp.hdr, LGLCD_PRIORITY_NORMAL);
			}
			if (res != ERROR_SUCCESS) {
				return -1;
			}
			return 1;
		}
		else if (width == 320 && height == 240 && bpp == 32) {
			memcpy(bmp.bmp_qvga32.pixels, image, sizeof(bmp.bmp_qvga32.pixels));
			bmp.hdr.Format = LGLCD_BMP_FORMAT_QVGAx32;
			res = lgLcdUpdateBitmap(deviceID, &bmp.hdr, LGLCD_PRIORITY_NORMAL);
			if (res != ERROR_SUCCESS) {
				return -1;
			}
			return 1;
		}
		return 0;
	}
};

DWORD WINAPI OnNotificationCB(IN int connection, IN const PVOID pContext, IN DWORD notificationCode, IN DWORD notifyParm1, IN DWORD notifyParm2, IN DWORD notifyParm3, IN DWORD notifyParm4) {
	if (notificationCode == LGLCD_NOTIFICATION_CLOSE_CONNECTION || notificationCode == LGLCD_NOTIFICATION_DEVICE_ARRIVAL || notificationCode == LGLCD_NOTIFICATION_DEVICE_REMOVAL) {
		PostMessage(ghWnd, WM_DEVICECHANGE, 0x0007, 0);
	}
	return 0;
}

struct DualCtx {
	union {
		lgLcdConnectContextExW ctxEx;
		loLcdConnectContextW ctx;
		struct {
			// "Friendly name" display in the listing
			LPCWSTR appFriendlyName;
			// isPersistent determines whether this connection persists in the list
			BOOL isPersistent;
			// isAutostartable determines whether the client can be started by
			// LCDMon
			BOOL isAutostartable;
			lgLcdConfigureContext onConfigure;
			// --> Connection handle
			int connection;
			// New additions added in 1.03 revision
			DWORD dwAppletCapabilitiesSupported;    // Or'd combination of LGLCD_APPLET_CAP_... defines
			DWORD dwReserved1;
			lgLcdNotificationContext onNotify;
		};
	};
};

DualCtx ctx = {
	L"LCD Miscellany",
	1,
#ifdef NDEBUG
	1,
#else
	0,
#endif
	{0,0},
	LGLCD_INVALID_CONNECTION,
	LGLCD_APPLET_CAP_BW | LGLCD_APPLET_CAP_QVGA,
	0,
	{OnNotificationCB, 0}
};

static void Disconnect() {
	for (int i=numDevices-1; i>= 0; i--) {
		if (devices[i]->cType == CONNECTION_SDK301 || 
			devices[i]->cType == CONNECTION_SDK103) {
				DeleteDevice(i);
		}
	}
	if (connection != LGLCD_INVALID_CONNECTION) {
		if (sdkType == CONNECTION_SDK301) {
			lgLcdDisconnect(connection);
		}
		else if (sdkType == CONNECTION_SDK103) {
			loLcdDisconnect(connection);
		}
		connection = LGLCD_INVALID_CONNECTION;
	}
}

void UninitSDK() {
	Disconnect();
	if (sdkType == CONNECTION_SDK301) {
		lgLcdDeInit();
	}
	else if (sdkType == CONNECTION_SDK103) {
		loLcdDeInit();
	}
	sdkType = CONNECTION_NULL;
}

static int InitSDK() {
	UninitSDK();
	HRESULT res = lgLcdInit();
	if (res == S_OK || res == ERROR_ALREADY_INITIALIZED) {
		sdkType = CONNECTION_SDK301;
		return 1;
	}
	res = loLcdInit();
	if (res == S_OK || res == ERROR_ALREADY_INITIALIZED) {
		sdkType = CONNECTION_SDK103;
		return 1;
	}
	return 0;
}

static int Connect() {
	Disconnect();
	if (sdkType == CONNECTION_NULL && !InitSDK()) return 0;
	HRESULT res;
	connection = LGLCD_INVALID_CONNECTION;
	if (sdkType == CONNECTION_SDK301) {
		res = lgLcdConnectExW(&ctx.ctxEx);
	}
	else if (sdkType == CONNECTION_SDK103) {
		res = loLcdConnect(&ctx.ctx);
	}
	if (res == ERROR_SUCCESS && ctx.connection != LGLCD_INVALID_CONNECTION) {
		connection = ctx.connection;
		return 1;
	}
	if (res == ERROR_SERVICE_NOT_ACTIVE || res == ERROR_ALREADY_EXISTS) {
		UninitSDK();
	}
	return 0;
}

static DWORD __stdcall ButtonsChanged(int device,
							DWORD dwButtons,
							const PVOID pContext) {
	int i;
	for (i=0; i<numDevices; i++) {
		if (devices[i]->cType == CONNECTION_SDK301 || devices[i]->cType == CONNECTION_SDK103) {
			if (((SDKDevice*)devices[i])->deviceID == device) {
				if (devices[i]->width == 320) dwButtons >>= 8;
				break;
			}
		}
	}
	PostMessage(ghWnd, WMU_BUTTONS, dwButtons, i);
	return 1;
}

void DeleteDevice(int i) {
	delete devices[i];
	while (++i < numDevices) {
		devices[i-1] = devices[i];
	}
	numDevices--;
	if (!numDevices) {
		free(devices);
		devices = 0;
	}
}

void DeleteDevice(Device *dev) {
	for (int i=0; i<numDevices; i++) {
		if (dev == devices[i]) {
			DeleteDevice(i);
			return;
		}
	}
}


void AddDevice(Device *dev) {
	devices = (Device**) realloc(devices, sizeof(Device*) * (numDevices+1));
	int i = numDevices;
	while (i) {
		if (devices[i-1]->cType < dev->cType) {
			break;
		}
		if (devices[i-1]->cType == dev->cType &&
			devices[i-1]->bpp > dev->bpp) {
				break;
		}
		devices[i] = devices[i-1];
		i--;
	}
	devices[i] = dev;
	numDevices++;
}

static int EnumSDKDevices() {
	if (connection == LGLCD_INVALID_CONNECTION && !Connect()) return 0;
	HRESULT res;
	char temp[1000];
	int i = 0;
	if (sdkType == CONNECTION_SDK301) {
		int found[2];
		memset(found, 0, sizeof(found));
		while (i<numDevices) {
			if (devices[i]->cType == CONNECTION_SDK301) {
				if (devices[i]->width == 160)
					found[0]++;
				else if (devices[i]->width == 320)
					found[1]++;
			}
			i++;
		}
		if (!found[0]) {
			lgLcdOpenByTypeContext typeCtx = {connection, LGLCD_DEVICE_BW, {ButtonsChanged, 0}, LGLCD_INVALID_DEVICE};
			res = lgLcdOpenByType(&typeCtx);
			if (res == ERROR_SUCCESS) {
				sprintf(temp, "G15 3.01 %i", typeCtx.device);
				AddDevice(new SDKDevice(typeCtx.device, 160, 43, 1, sdkType, SDK_160_43_1, temp));
			}
		}
		if (!found[1]) {
			lgLcdOpenByTypeContext typeCtx = {connection, LGLCD_DEVICE_QVGA, {ButtonsChanged, 0}, LGLCD_INVALID_DEVICE};
			res = lgLcdOpenByType(&typeCtx);
			if (res == ERROR_SUCCESS) {
				sprintf(temp, "G19 3.01 %i", typeCtx.device);
				AddDevice(new SDKDevice(typeCtx.device, 320, 240, 32, sdkType, SDK_320_240_32, temp));
			}
		}
		return 1;
	}
	else if (sdkType == CONNECTION_SDK103) {
		int error = 0;
		while (1) {
			loLcdDeviceDescEx desc;
			res = loLcdEnumerateEx(connection, i, &desc);
			if (res == ERROR_SUCCESS) {
				if (desc.Bpp == 1 && desc.Height == 43 && desc.Width == 160) {
					loLcdOpenContext openctx;
					openctx.connection = connection;
					openctx.index = i;
					openctx.onSoftbuttonsChanged.softbuttonsChangedCallback = ButtonsChanged;
					openctx.device = LGLCD_INVALID_DEVICE;
					res = loLcdOpen(&openctx);
					if (res == ERROR_SUCCESS) {
						sprintf(temp, "G15 1.03 %i", openctx.device);
						AddDevice(new SDKDevice(openctx.device, desc.Width, desc.Height, desc.Bpp, sdkType, SDK_160_43_1, temp));
					}
				}
				i++;
			}
			else if (res == ERROR_NO_MORE_ITEMS) return 1;
			else if (!error) {
				UninitSDK();
				error = 1;
				if (!Connect()) break;
			}
			else break;
		}
	}
	return 0;
}

void UpdateSDKDevices() {
	// If there's a problem, will clean up connection.
	EnumSDKDevices();
	if (sdkType == CONNECTION_NULL) {
		InitSDK();
		EnumSDKDevices();
	}
}

int GetG15Index(ScriptValue *kb) {
	if (!numDevices) return -1;
	if (kb->stringVal->len) {
		int i;
		for (i=0; i<kb->stringVal->len; i++) {
			if (!IsNumber(kb->stringVal->value[i])) break;
		}
		if (i == kb->stringVal->len) {
			int index = strtol((char*)kb->stringVal->value, 0, 10);
			if (index == 0) return 0;
			if ((unsigned int) index > (unsigned int)numDevices) return -1;
			return index;
		}
		else {
			int index = 0;
			while (index < numDevices) {
				if (!scriptstricmp(kb->stringVal, devices[index]->id)) return index+1;
				index ++;
			}
		}
		return -1;
	}
	else return 0;
}

