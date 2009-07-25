// Currently exists separately from dlls that support scripted functions.
// May merge in the future.
#include <windows.h>

// Dlls with functions called by scripts - currently not required,
// as functions have to be declared in script files, anyways.
#define PLUGIN_TYPE_SCRIPT   0x01

// Plugins that support some method of displaying bitmaps
// (Presumably, but not necessarily, on a secondary LCD display).
#define PLUGIN_TYPE_LCD      0x02

// Current version of all types
#define SCRIPT_PLUGIN_VERSION  1
#define LCD_PLUGIN_VERSION     1


// All plugins (Except plugins called from scripts) must export this function.
typedef unsigned int (CALLBACK * lcdmGetPluginType)();


/******************************
 * Functions for LCD plugins. *
 ******************************/

struct LCDInfo;

// Return 0 on fail.  Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.
typedef int (CALLBACK * lcdStart)(LCDInfo *info);

// Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.
typedef void (CALLBACK * lcdStop)(LCDInfo *info);

// Function specified by LCDInfo structure.
// Info contains what lcdEnum returned.
typedef void (CALLBACK * lcdUpdate)(LCDInfo *info, unsigned char *bitmap, int width, int height);

// Empty data structure passed to the plugin by lcdEnum().
// Plugin should populate and return.
struct LCDInfo {
	// Version of the interface the plugin supports.
	int version;

	int width;
	int height;
	// Should be 1, 8, 24, or 32.
	int bpp;

	// At the moment, mostly not used...
	int refreshRate;

	lcdStart *Start;
	lcdStop *Stop;
	lcdUpdate *Update;
};

// Callback functions passed to LCD plugins.  Thread safe,
// so if needed, dll can start its own monitoring thread.

// When called, will stop all lcds the dll is managing, enumerate
// them again, and then start up all returned devices.  Note that
// Other events may be queued, so may not happen before the next
// draw event.
typedef void (CALLBACK * lcdDeviceChange)(int id);

// Triggers event, with the specified string as a paramter.  Strings are assumed to be in UTF8.
// If need more than one parameter, can use a delimited list, and scripts can split it themselves.
// If no parameter is needed, param can be null.
typedef void (CALLBACK * lcdTriggerEvent)(int id, unsigned char *eventName, unsigned char *param);

struct LCDCallbacks {
	// For simplicity, same as LCDInfo version.
	int version;

	lcdDeviceChange * DeviceChange;
	lcdTriggerEvent * TriggerEvent;
};


/* Expored functions:
 */

// After lcdmGetPluginType, this is called to do whatever needs to be done.
// Return 0 on failure, no other functions will be called.
// Not required.
typedef int (CALLBACK * lcdInit)();

// Called when done.  No other commands will be called unless lcdInit()
// is called first.
// Not required.
typedef void (CALLBACK * lcdShutdown)();

// Required.  Plugin must allocate and populate info itself.
// callbacks is already populated.  Script that uses callbacks
// should make its own copy of the structure.
typedef unsigned int (CALLBACK * lcdEnum)(LCDInfo **info, const LCDCallbacks *callbacks);
