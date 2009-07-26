#ifndef PLUGIN_H
#define PLUGIN_H

// Currently exists separately from dlls that support scripted functions.
// May merge in the future.
#include <windows.h>

// Current version of all plugin types.  Don't plan on adding more, but you never know.
#define SCRIPT_PLUGIN_VERSION  1
#define LCD_PLUGIN_VERSION     1


/******************************
 * Functions for LCD plugins. *
 ******************************/

struct LCDInfo;

// Return 0 on fail.  Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.  Not required.
typedef int (CALLBACK * lcdStart)(LCDInfo *info);

// Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.  Not required.
typedef void (CALLBACK * lcdStop)(LCDInfo *info);

// Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.
// Final function called on info.  Not required.
typedef void (CALLBACK * lcdDestroy)(LCDInfo *info);

// Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.  Required for each LCD.
typedef void (CALLBACK * lcdUpdate)(LCDInfo *info, unsigned char *bitmap, int width, int height);

// Plugin returns a pointed to a populated data structure in response to lcdEnum.
struct LCDInfo {
	// Version of the interface the plugin supports.
	int version;

	int width;
	int height;
	// Should be 1, 8, 24, or 32.
	int bpp;

	// At the moment, mostly not used...
	int refreshRate;

	lcdStart Start;
	lcdStop Stop;
	lcdUpdate Update;
	lcdDestroy Destroy;
};

// Callback functions passed to LCD plugins.  Thread safe,
// so if needed, dll can start its own monitoring thread.

// When called, will stop all lcds the dll is managing, enumerate
// them again, and then start up all returned devices.  Note that
// Other events may be queued, so may not happen before the next
// draw event.  If called when enumerating devices, enumeration will
// continue and then restart later.
typedef void (CALLBACK * lcdDeviceChange)(int id);

// Triggers event, with the specified string as a paramter.  Strings are assumed to be in UTF8.
// If need more than one parameter, can use a delimited list, and scripts can split it themselves.
// If no parameter is needed, param can be null.
typedef void (CALLBACK * lcdTriggerEvent)(int id, unsigned char *eventName, unsigned char *param);

struct LCDCallbacks {
	// For simplicity, same as LCDInfo version (LCD_PLUGIN_VERSION).
	int version;

	// Id assigned to plugin for use in callback functions.
	int id;

	lcdDeviceChange DeviceChange;
	lcdTriggerEvent TriggerEvent;
};


/* Exported functions:
 */

// This is called first.
// Return 0 on failure, no other functions will be called.
// Not required, but if present, must return 1 when called or
// no other calls will ever occur.

// Callbacks contains all callback functions.  Callback structure
// will not be modified, so no need to make your own copy.
typedef int (CALLBACK * lcdInit)(const LCDCallbacks *callbacks);

// Called when done.  No other commands will be called unless lcdInit()
// is called first.
// Not required.
typedef void (CALLBACK * lcdUninit)();

// Required.  Plugin must allocate and populate info itself.
// Returns 0 on fail.
// Return value must stay valid until its destroy function is called on itself.
// index is a 0-based index.  It will be be called for each successive integer until it
// returns 0.  All devices will be destroyed before enumerating devices again.
typedef LCDInfo * (CALLBACK * lcdEnum)(int index);


#endif
