#include "../Global.h"
#include "../Device.h"
#include "plugin.h"
#include "plugins.h"
#include "../ScriptValue.h"

// Dlls with functions called by scripts - currently not used,
// as functions have to be declared in script files, it's all handled elsewhere.
#define PLUGIN_TYPE_SCRIPT   0x01

// Plugins that support some method of displaying bitmaps
// (Presumably, but not necessarily, on a secondary LCD display).
#define PLUGIN_TYPE_LCD      0x02

class DllLcdDevice;

struct ActiveLcdInfo {
	LcdInfo *info;
	DllLcdDevice *device;
};

class Plugin {
protected:
	HMODULE hMod;
public:
	unsigned int type;

	struct {
		lcdInit Init;
		lcdUninit Uninit;
		lcdEnum Enum;
		LcdCallbacks callbacks;
		int numLcds;
		ActiveLcdInfo *lcds;
	} lcd;

	Plugin(int id, HMODULE module);

	int LcdFindDevice(DllLcdDevice *device);
	void LcdEnum();
	void LcdDestroy(int index);
	void LcdDestroyAll();
	void LcdStopped(DllLcdDevice *device);

	~Plugin();
};


class DllLcdDevice : public Device {
public:
	LcdInfo *info;
	// Needed for cleanup.
	Plugin *plugin;

	DllLcdDevice(Plugin *plugin, LcdInfo *info) : Device(info->width, info->height, info->bpp, CONNECTION_3RD_PARTY, LCD_3RD_PARTY, info->id) {
		this->info = info;
		this->plugin = plugin;
	}

	~DllLcdDevice() {
		if (info->Stop) info->Stop(info);
		plugin->LcdStopped(this);
	}

	// Currently ignore the return value.
	int UpdateImage() {
		if (info->bpp > 8) {
			info->Update(info, (unsigned char*)image, width, height, 32);
		}
		else {
			unsigned char *pixels = (unsigned char*) malloc(width*height);
			int p = 0;
			for (int p = width*height-1; p >= 0; p--) {
				// Division by 3 without the divide.
				pixels[p] = ((((int)image[p].r + (int)image[p].g + (int)image[p].b) + 1) * 43691) >> 17;
			}
			info->Update(info, pixels, width, height, 8);
			free(pixels);
		}
		return 1;
	}
};

Plugin **plugins = 0;
int numPlugins = 0;

void InitPlugin(wchar_t *path, wchar_t *name) {
	HMODULE hMod = LoadLibraryExW(path, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (hMod) {
		static int nextID = 0;

		Plugin *plugin = new Plugin(nextID, hMod);
		// Pretty safe to assume this never wraps around...
		nextID++;

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
	PostMessage(ghWnd, WMA_LCD_DEVICE_CHANGE, id, 0);
}

void CALLBACK LcdTriggerEvent(int id, unsigned char *eventName, unsigned char *param) {
	ScriptValue sv;
	CreateStringValue(sv, param);
	PostMessage(ghWnd, WMA_TRIGGER_EVEN_BY_NAME, (WPARAM)strdup((char*)eventName), (LPARAM)sv.stringVal);
}

// Triggers event, with the specified string as a paramter.  Strings are assumed to be in UTF8.
// If need more than one parameter, can use a delimited list, and scripts can split it themselves.
// If no parameter is needed, param can be null.
typedef void (CALLBACK * lcdTriggerEvent)(int id, unsigned char *eventName, unsigned char *param);

Plugin::Plugin(int id, HMODULE module) {
	hMod = module;
	type = 0;

	memset(&lcd, 0, sizeof(lcd));

	lcd.callbacks.id = id;
	lcd.callbacks.version = LCD_PLUGIN_VERSION;
	lcd.callbacks.DeviceChange = LcdDeviceChange;
	lcd.callbacks.TriggerEvent = LcdTriggerEvent;

	lcd.Init = (lcdInit) GetProcAddress(module, "lcdInit");
	lcd.Uninit = (lcdUninit) GetProcAddress(module, "lcdUninit");
	lcd.Enum = (lcdEnum) GetProcAddress(module, "lcdEnum");
	if (lcd.Enum && (lcd.Init || lcd.Init(&lcd.callbacks))) {
		type |= PLUGIN_TYPE_LCD;
		LcdEnum();
	}
}

Plugin::~Plugin() {
	if (type & PLUGIN_TYPE_LCD) {
		LcdDestroyAll();
		if (lcd.Uninit) lcd.Uninit();
	}
	FreeLibrary(hMod);
}

void Plugin::LcdDestroy(int index) {
	ActiveLcdInfo *info = &lcd.lcds[index];
	if (info->device) {
		delete info->device;
	}
	if (info->info->Destroy) {
		info->info->Destroy(info->info);
	}
	lcd.numLcds--;
	for (int i=index; i<lcd.numLcds; i++) {
		lcd.lcds[i] = lcd.lcds[i+1];
	}
	if (!lcd.numLcds) {
		free(lcd.lcds);
		lcd.lcds = 0;
	}
}

void Plugin::LcdDestroyAll() {
	while (lcd.numLcds) LcdDestroy(lcd.numLcds-1);
}

int Plugin::LcdFindDevice(DllLcdDevice *device) {
	for (int i=0; i<lcd.numLcds; i++) {
		if (device == lcd.lcds[i].device) return i;
	}
	return -1;
}

void Plugin::LcdStopped(DllLcdDevice *device) {
	int i = LcdFindDevice(device);
	if (i >= 0) {
		lcd.lcds[i].device = 0;
	}
}

void Plugin::LcdEnum() {
	if (type & PLUGIN_TYPE_LCD) {
		LcdDestroyAll();
		LcdInfo *info;
		while (info = lcd.Enum()) {
			if (info->bpp < 1 || info->bpp > 32 || !info->Update || info->width < 1 || info->height < 1) {
				if (info->Destroy) info->Destroy(info);
				break;
			}
			lcd.lcds = (ActiveLcdInfo*) realloc(lcd.lcds, sizeof(ActiveLcdInfo) * (lcd.numLcds+1));
			lcd.lcds[lcd.numLcds].device = 0;
			lcd.lcds[lcd.numLcds].info = info;
			if (!info->Start || info->Start(info)) {
				lcd.lcds[lcd.numLcds].device = new DllLcdDevice(this, info);
			}
			lcd.numLcds++;
		}
	}
}

int FindPlugin(int id) {
	for (int i=0; i<numPlugins; i++) {
		if (plugins[i]->lcd.callbacks.id == id) return i;
	}
	return -1;
}

void RefreshPluginDevices(int id) {
	int i = FindPlugin(id);
	if (i<0) return;
	plugins[i]->LcdEnum();
}
