#include "../Global.h"
#include "plugin.h"
#include "plugins.h"

// Dlls with functions called by scripts - currently not used,
// as functions have to be declared in script files, it's all handled elsewhere.
#define PLUGIN_TYPE_SCRIPT   0x01

// Plugins that support some method of displaying bitmaps
// (Presumably, but not necessarily, on a secondary LCD display).
#define PLUGIN_TYPE_LCD      0x02


class Plugin {
protected:
	HMODULE hMod;
public:
	unsigned int type;

	struct {
		lcdInit Init;
		lcdUninit Uninit;
		lcdEnum Enum;
		LCDCallbacks callbacks;
	} lcd;

	Plugin(HMODULE module);

	~Plugin();
};

Plugin **plugins = 0;
int numPlugins = 0;

void InitPlugin(wchar_t *path, wchar_t *name) {
	HMODULE hMod = LoadLibraryExW(path, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (hMod) {
		Plugin *plugin = new Plugin(hMod);
		if (plugin->type) {
			plugins = (Plugin**) realloc(plugins, sizeof(Plugin*) * (numPlugins+1));
			plugins[numPlugins++] = plugin;
		}
		else {
			delete plugin;
		}
	}
}

void InitPlugins() {
	WIN32_FIND_DATAW data;
	wchar_t path[MAX_PATH*2];
	if (!GetCurrentDirectory(MAX_PATH, path)) return;
	wcscat(path, L"\\dll\\");
	wchar_t *ext = wcschr(path, 0);
	wcscpy(ext, L"*.dll");
	HANDLE hFind = FindFirstFileW(path, &data);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		wcscpy(ext, data.cFileName);
		InitPlugin(path, data.cFileName);
	}
	while (FindNextFile(hFind, &data));
	FindClose(hFind);
}

void UninitPlugins() {
	for (int i=0; i<numPlugins; i++) {
		delete plugins[i];
	}
	free(plugins);
}

void CALLBACK LcdDeviceChange(int id) {
}

void CALLBACK LcdTriggerEvent(int id, unsigned char *eventName, unsigned char *param) {
}

// Triggers event, with the specified string as a paramter.  Strings are assumed to be in UTF8.
// If need more than one parameter, can use a delimited list, and scripts can split it themselves.
// If no parameter is needed, param can be null.
typedef void (CALLBACK * lcdTriggerEvent)(int id, unsigned char *eventName, unsigned char *param);


Plugin::Plugin(HMODULE module) {
	hMod = module;
	type = 0;

	lcd.callbacks.version = LCD_PLUGIN_VERSION;
	lcd.callbacks.DeviceChange = LcdDeviceChange;
	lcd.callbacks.TriggerEvent = LcdTriggerEvent;

	lcd.Init = (lcdInit) GetProcAddress(module, "lcdInit");
	lcd.Uninit = (lcdUninit) GetProcAddress(module, "lcdUninit");
	lcd.Enum = (lcdEnum) GetProcAddress(module, "lcdEnum");
	if (lcd.Enum) {
		type |= PLUGIN_TYPE_LCD;
	}

}

Plugin::~Plugin() {
	FreeLibrary(hMod);
}

