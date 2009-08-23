#pragma once

#include "Images/Image.h"
#include "ScriptValue.h"
#include "Screen.h"

enum UpdateResult {
	UPDATE_HAPPY,
	UPDATE_KILL_ME
};

// Communication method.
enum ConnectionType : unsigned short {
	CONNECTION_NULL,
	CONNECTION_SDK103,
	CONNECTION_SDK301,
	CONNECTION_DIRECT,
	CONNECTION_SYSTRAY_ICON,
	CONNECTION_3RD_PARTY
};

// Display device type.
enum HID_DEVICE_TYPE {
	HID_LCD_G11,
	HID_LCD_G15_V1,
	HID_LCD_G15_V2,
	HID_LCD_Z10,
	HID_LCD_GAME_PANEL,
	HID_LCD_G19,
	SDK_160_43_1 = 64,
	SDK_320_240_32 = 65,
	SYSTRAY_ICON = 128,
	LCD_3RD_PARTY = 256,
};

// Currently only used (in other files) as an optimization.
extern ConnectionType sdkType;

struct G15;
class Device : public Screen {
public:
	// Only used by hids, but got really, really sick of casting them.
	G15 *g15;
	// Exact type, values given to 
	StringValue *id;

	// Only used by sdk devices.
	int priority;
	__int64 priorityTimer;

	HID_DEVICE_TYPE hidType;

	ConnectionType cType;


	// Need to call redraw script.
	char needRedraw;
	// Object itself is active.  0 before I've drawn anything
	// and for direct connections when also connected through LCD Mon.
	char sendImage;

	unsigned int gKeyState;

	Device(int width, int height, int bpp, ConnectionType cType, HID_DEVICE_TYPE hidType, char *id);
	virtual ~Device();

	virtual inline int UpdateImage() {
		return 0;
	};
};

extern Device **devices;
extern int numDevices;

void UninitSDK();
void UpdateSDKDevices();
void DeleteDevice(int i);
void DeleteDevice(Device *dev);

void AddDevice(Device *dev);

int GetG15Index(ScriptValue *kb);
