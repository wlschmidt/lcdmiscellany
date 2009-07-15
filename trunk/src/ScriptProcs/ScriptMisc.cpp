#define DIRECT3D_VERSION 0x0400
#include "ScriptMisc.h"
#include "..\\global.h"
#include "..\\vm.h"
//#include "..\\stringUtil.h"
#include "..\\SymbolTable.h"
#include "..\\unicode.h"
#include <Psapi.h>
#include "..\\util.h"
//#include <PowrProf.h>

#include "../Config.h"


#include <ddraw.h>
#include <float.h>

IDirectDraw* ddraw = 0;
void CleanupDdraw() {
	if (ddraw) {
		ddraw->Release();
		ddraw = 0;
	}
}

void CleanupDdraw(ScriptValue &s, ScriptValue *args) {
	CleanupDdraw();
}

void GetVram(ScriptValue &s, ScriptValue *args) {
	if (!ddraw) {
		if (!SUCCEEDED(DirectDrawCreate((GUID*)DDCREATE_HARDWAREONLY, &ddraw, 0)) || !ddraw) {
			if (!SUCCEEDED(DirectDrawCreate(0, &ddraw, 0)) || !ddraw) {
				return;
			}
		}
	}
	DDCAPS ddcaps;
	memset(&ddcaps, 0, sizeof(ddcaps));
	ddcaps.dwSize = sizeof(ddcaps);
	if (SUCCEEDED(ddraw->GetCaps(&ddcaps, 0)) && CreateListValue(s, 2)) {
		ScriptValue sv;
		CreateIntValue(sv, ddcaps.dwVidMemTotal);
		s.listVal->PushBack(sv);
		CreateIntValue(sv, ddcaps.dwVidMemTotal-ddcaps.dwVidMemFree);
		s.listVal->PushBack(sv);
	}
}


void Suspend(ScriptValue &s, ScriptValue *args) {
	getShutdownPriv();
	CreateIntValue(s, SetSystemPowerState(args[0].i32, args[0].i32));
}

void Shutdown(ScriptValue &s, ScriptValue *args) {
	getShutdownPriv();
	/*wchar_t *name = 0;
	if (args[3].stringVal->len) {
		name = UTF8toUTF16Alloc(args[3].stringVal->value);
	}//*/
	CreateIntValue(s, InitiateSystemShutdownW(0, 0, args[0].i32, args[1].i32, args[2].i32));
	//free(name);
}

void AbortShutdown(ScriptValue &s, ScriptValue *args) {
	getShutdownPriv();
	/*wchar_t *name = 0;
	if (args[0].stringVal->len) {
		name = UTF8toUTF16Alloc(args[0].stringVal->value);
	}//*/
	CreateIntValue(s, AbortSystemShutdownW(0));
	//free(name);
}

void LockSystem(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, LockWorkStation());
}

void Quit(ScriptValue &s, ScriptValue *args) {
	Quit();
}

/*
#pragma pack(push, 1)
struct SpeedFanData {
	unsigned short version;
	unsigned short flags;
	int MemSize;
	HANDLE THandle;
	unsigned short NumTemps;
	unsigned short NumFans;
	unsigned short NumVolts;
	int temps[32];
	int fans[32];
	int volts[32];
};
#pragma pack(pop)


void GetSpeedFanData(ScriptValue &s, ScriptValue *args) {
	SpeedFanData *sfd;
	HANDLE hSData;

	hSData=OpenFileMappingA(FILE_MAP_READ, FALSE, "SFSharedMemory_ALM");
	if (hSData==0)
		return;

	sfd = (SpeedFanData *)MapViewOfFile(hSData, FILE_MAP_READ, 0, 0, 0);
	if (sfd == 0) {
		CloseHandle(hSData);
		return;
	}

	if (sfd->version == 1 && sfd->MemSize == 402 && CreateDictValue(s, 3)) {
		int i;
		char *names[3] = {"temps", "fans", "volts"};
		for (i=0; i<3; i++) {
			ScriptValue sv;
			ScriptValue sv2;
			if (!CreateListValue(sv, 32)) {
				break;
			}
			CreateStringValue(sv2, (unsigned char*)names[i]);
			if (!s.dictVal->Add(sv2, sv)) {
				sv2.Release();
				sv.Release();
				break;
			}
			if (i==0) {
				for (int j=0; j<sfd->NumTemps; j++) {
					CreateDoubleValue(sv2, sfd->temps[j]/100.0);
					sv.listVal->PushBack(sv2);
				}
			}
			else if (i==1) {
				for (int j=0; j<sfd->NumFans; j++) {
					CreateIntValue(sv2, sfd->fans[j]);
					sv.listVal->PushBack(sv2);
				}
			}
			else if (i==2) {
				for (int j=0; j<sfd->NumVolts; j++) {
					CreateDoubleValue(sv2, sfd->volts[j]/100.0);
					sv.listVal->PushBack(sv2);
				}
			}
		}
		if (i<3) {
			s.Release();
			CreateNullValue(s);
		}
	}

	UnmapViewOfFile(sfd);
	CloseHandle(hSData);
}
//*/
void FormatValue(ScriptValue &s, ScriptValue *args) {
	double d = args[0].doubleVal;
	if (_finite (d)) {
		char temp2[100];
		int i = args[1].i32;
		int i2 = args[2].i32;
		char temp[100];
		if (i > 20) i = 20;
		else if (i < 0) i = 0;
		if (i2 > 20) i2 = 20;
		else if (i2 < 0) i2 = 0;
		sprintf(temp, "%%%i.%ilf", i, i2);
		sprintf(temp2, temp, d);
		CreateStringValue(s, (unsigned char*) temp2);
	}
	else {
		if (_isnan(d))
			CreateStringValue(s, (unsigned char*) "NaN");
		else if (d < 0)
			CreateStringValue(s, (unsigned char*) "-Inf");
		else
			CreateStringValue(s, (unsigned char*) "Inf");
	}
}

void FormatSize(ScriptValue &s, ScriptValue *args) {
	double d = args[0].doubleVal;
	int min = args[1].i32;
	int extra = args[2].i32;
	char temp[100];
	if ((unsigned int)args[3].i32 > 4) args[3].i32 = 2;
	else if (args[3].i32 > 2) {
		args[3].i32 = 7 - args[3].i32;
	}
	else {
		args[3].i32 = 2 - args[3].i32;
	}
	if (min < 0 || min > 20 || extra < 0 || extra > 20) {
		temp[0] = 0;
	}
	else
		FormatDataSize (temp, d, min, extra, args[3].i32);
	CreateStringValue(s, (unsigned char*)temp);
}

void GetMemoryStatus(ScriptValue &s, ScriptValue *args) {
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);
	if (GlobalMemoryStatusEx(&mem) && CreateListValue(s, 6)) {
		CreateIntValue(s.listVal->vals[0], mem.ullTotalPhys);
		CreateIntValue(s.listVal->vals[1], mem.ullTotalPhys-mem.ullAvailPhys);
		CreateIntValue(s.listVal->vals[2], mem.ullTotalPageFile);
		CreateIntValue(s.listVal->vals[3], mem.ullTotalPageFile-mem.ullAvailPageFile);
		CreateIntValue(s.listVal->vals[4], mem.ullTotalVirtual);
		CreateIntValue(s.listVal->vals[5], mem.ullTotalVirtual-mem.ullAvailVirtual);
		s.listVal->numVals=6;
	}
}

void GetUsedMemory(ScriptValue &s, ScriptValue *args) {
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);
	GlobalMemoryStatusEx(&mem);
	CreateIntValue(s, mem.ullTotalPhys-mem.ullAvailPhys);
}

void GetTotalMemory(ScriptValue &s, ScriptValue *args) {
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);
	GlobalMemoryStatusEx(&mem);
	CreateIntValue(s, mem.ullTotalPhys);
}

void GetDiskSpace(ScriptValue &s, ScriptValue *args) {
	ULARGE_INTEGER avail, total, free;
	if (GetDiskFreeSpaceExA((char*)args->stringVal->value, &avail, &total, &free) && CreateListValue(s, 2)) {
		ScriptValue sv;
		CreateIntValue(sv, ((__int64*) &free)[0]);
		s.listVal->PushBack(sv);
		CreateIntValue(sv, ((__int64*) &total)[0]);
		s.listVal->PushBack(sv);
	}
}

void GetDiskSpaceFree(ScriptValue &s, ScriptValue *args) {
	ULARGE_INTEGER avail, total, free;
	if (GetDiskFreeSpaceExA((char*)args->stringVal->value, &avail, &total, &free)) {
		CreateIntValue(s, ((__int64*) &free)[0]);
	}
}

void GetDiskSpaceTotal(ScriptValue &s, ScriptValue *args) {
	ULARGE_INTEGER avail, total, free;
	if (GetDiskFreeSpaceExA((char*)args->stringVal->value, &avail, &total, &free)) {
		CreateIntValue(s, ((__int64*) &total)[0]);
	}
}



/*void GetVolume(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, soundControls.val);
}
/*
void SetMenuOrder(ScriptValue &s, ScriptValue * args) {
	if (args->type == SCRIPT_LIST) {
		int pos = 0;
		for (int j=0; j<args->listVal->numVals; j++) {
			ScriptValue sv;
			args->listVal->vals[j].AddRef();
			CoerceString(args->listVal->vals[j], sv);
			int i;
			for (i=0; i<sv.stringVal->len; i++) {
				if (sv.stringVal->value == 0) break;
			}
			if (i < sv.stringVal->len) {
				sv.Release();
				continue;
			}
			for (i=pos; i<appManager.numApps; i++) {
				if (strcmp((unsigned char*)root.buttons[i].obj->name, sv.stringVal->value) == 0) {
					Button b = root.buttons[pos];
					root.buttons[pos] = root.buttons[i];
					root.buttons[i] = b;
					pos ++;
					break;
				}
			}
			if (i == appManager.numApps) {
				for (i = 0; i<appManager.numApps; i++) {
					if (strcmp((unsigned char*)root.buttons[i].obj->name, sv.stringVal->value) == 0) {
						break;
					}
				}
				if (i == appManager.numApps) {
					ScriptedView *app = new ScriptedView(sv.stringVal->value);
					if (app) {
						if (app->displayProc == -1) {
							errorPrintf("Failed to initialize view: %s.\r\n", sv.stringVal->value);
							delete app;
							app = 0;
						}
						if (app) {
							if (app->imageLoaded) {
								root.AddButton(app, (BitImage*)app->image.stringVal->value, LCDM_FOCUS);
							}
							else
								root.AddButton(app, image_eye, LCDM_FOCUS);
							appManager.AddApp(app);
							Button b = root.buttons[pos];
							root.buttons[pos] = root.buttons[i];
							root.buttons[i] = b;
							pos ++;
						}
					}
				}
			}
			sv.Release();
		}
		if (pos) {
			root.numButtons = pos;
			root.buttons[0].Focus();
			CreateIntValue(s, pos);
		}
	}
}
//*/
void FreeResult(void *data, int i) {
	free(data);
}

void StartScreenSaver(ScriptValue &s, ScriptValue *args) {
	HWND h = GetDesktopWindow();
	if (h) {
		SendMessage(h, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
	}
}

void MontorPower(ScriptValue &s, ScriptValue *args) {
	SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM) 2);
}

void list(ScriptValue &s, ScriptValue * args) {
	args[0].AddRef();
	s = args[0];
	return;
}

void _null(ScriptValue &s, ScriptValue * args) {
	return;
}


struct FrapsData {
	DWORD SizeOfStruct;
	DWORD CurrentFPS;
	DWORD TotalFrames;
	DWORD TimeOfLastFrame;
	char GameName[32];
	int unknown[4];
	int width;
	int height;
	int unknown2;
};

typedef FrapsData * WINAPI  FrapsSharedDataProc();
FrapsSharedDataProc *FrapsSharedData = 0;
HMODULE hFraps = 0;
//sFraps *(WINAPI *FrapSharedData) ();

void UnloadFraps(ScriptValue &s, ScriptValue *args) {
	if (hFraps) {
		FreeLibrary(hFraps);
		hFraps = 0;
		FrapsSharedData = 0;
	}
}

//void LoadFraps(ScriptValue &s, ScriptValue *args) {
	/*if (!hFraps) {
		wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value);
		if (path) {
			hFraps = LoadLibraryW(path);
			free(path);
		}
	}
	if (hFraps) {
		CreateIntValue(s, 1);
		FrapsSharedData = (FrapsSharedDataProc*) GetProfcAddress(hFraps, "FrapsSharedData");
		FrapsData *fsd = FrapsSharedData();
		fsd = fsd;
	}//*/
//}

void GetFrapsData(ScriptValue &s, ScriptValue *args) {
	if (!hFraps) {
		//hFraps = GetModuleHandleW(L"fraps.dll");
		//return;
		HWND hWnd;
		hWnd = GetForegroundWindow();
		if (!hWnd) return;
		DWORD id = -1, temp=0;
		/*{
			DWORD ids[10000];
			if (!EnumProcesses(ids, sizeof(DWORD)*10000, &temp)) {
				return;
			}
			for (int i=0; i<temp; i++) {
				GetProcessImageFileName
			}
			temp/=sizeof(DWORD);
		}//*/
		if (GetWindowThreadProcessId(hWnd, &id)) {
			HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, id);
			HMODULE modules[10000];
			int res = EnumProcessModules(h, modules, 10000, &temp);
			if (res) {
				temp/=sizeof(HMODULE);
				unsigned int i;
				wchar_t name[1500];
				for (i=0; i<temp; i++) {
					res = GetModuleFileNameExW(h, modules[i], name, 1500);
					if (res) {
						wchar_t *fname = 0;
						for (int j=0; name[j]; j++) {
							if (name[j] == '\\') fname = name+j+1;
						}
#ifdef X64
						if (wcsicmp(fname, L"fraps64.dll") == 0) {
#else
						if (wcsicmp(fname, L"fraps.dll") == 0) {
#endif
							hFraps = LoadLibraryW(name);
							if (hFraps) {
								FrapsSharedData = (FrapsSharedDataProc*) GetProcAddress(hFraps, "FrapsSharedData");
								if (FrapsSharedData) {
									break;
								}
								FreeLibrary(hFraps);
								hFraps = 0;
							}
						}
					}
				}
			}
			CloseHandle(h);
			if (!hFraps) return;
		}
/*
#ifdef X64
		hFraps = GetModuleHandleA("fraps64.dll");
#else
		hFraps = GetModuleHandleA("fraps.dll");
#endif
		/*
		if (hFraps) {
			wchar_t test[MAX_PATH*2];
			if (!GetModuleFileName(hFraps, test, sizeof(test)/sizeof(wchar_t))) {
				hFraps = 0;
				return;
			}
			hFraps = LoadLibraryW(test);
		}
		if (!hFraps) {
			return;
		}
		//*/
	}
	if (!FrapsSharedData) {
		FrapsSharedData = (FrapsSharedDataProc*) GetProcAddress(hFraps, "FrapsSharedData");
		if (!FrapsSharedData) return;
	}
	FrapsData *fsd = FrapsSharedData();
	if (fsd && fsd->SizeOfStruct >= 48) {
		ScriptValue sv, sv2;
		if (CreateDictValue(s, 8)) {
			CREATE_STATIC_STRING(sv, "fps");
			//CreateStringValue(sv, (unsigned char*)"fps");
			CreateIntValue(sv2, fsd->CurrentFPS);
			s.dictVal->Add(sv, sv2);

			CREATE_STATIC_STRING(sv, "frame");
			CreateIntValue(sv2, fsd->TotalFrames);
			s.dictVal->Add(sv, sv2);

			CREATE_STATIC_STRING(sv, "lastframe");
			CreateIntValue(sv2, fsd->TimeOfLastFrame);
			s.dictVal->Add(sv, sv2);

			if (fsd->SizeOfStruct >= sizeof(*fsd)) {
				CREATE_STATIC_STRING(sv, "width");
				CreateIntValue(sv2, fsd->width);
				s.dictVal->Add(sv, sv2);

				CREATE_STATIC_STRING(sv, "height");
				CreateIntValue(sv2, fsd->height);
				s.dictVal->Add(sv, sv2);

				CREATE_STATIC_STRING(sv, "renderer");
				CreateIntValue(sv2, fsd->unknown[0]);
				s.dictVal->Add(sv, sv2);

				CREATE_STATIC_STRING(sv, "unknown2");
				CreateIntValue(sv2, fsd->unknown2);
				s.dictVal->Add(sv, sv2);
			}

			CREATE_STATIC_STRING(sv, "name");
			int len = 0;
			while (fsd->GameName[len] && len < 31) len++;
			unsigned char name[100];
			ASCIItoUTF8(name, fsd->GameName);
			CreateStringValue(sv2, name);
			s.dictVal->Add(sv, sv2);
		}
	}
}

void GetVersion(ScriptValue &s, ScriptValue *args) {
	if (CreateListValue(s, 5)) {
		ScriptValue sv;
		char v[5] = VERSION;
		for (int i=0; i<5; i++) {
			CreateIntValue(sv, v[i]);
			s.listVal->PushBack(sv);
		}
	}
}

void GetVersionString(ScriptValue &s, ScriptValue *args) {
	if (!args[0].intVal)
		CreateStringValue(s, (unsigned char*)APP_NAME);
	else if (args[0].intVal == 1)
		CreateStringValue(s, (unsigned char*)VERSION_STRING_LONG);
	else if (args[0].intVal == 2)
		CreateStringValue(s, (unsigned char*)VERSION_STRING_SHORT);
}

void WriteLog(ScriptValue &s, ScriptValue *args) {
	errorPrintf(args[1].i32, "%s", args[0].stringVal->value);
}

void WriteLogLn(ScriptValue &s, ScriptValue *args) {
	errorPrintf(args[1].i32, "%s\r\n", args[0].stringVal->value);
}

