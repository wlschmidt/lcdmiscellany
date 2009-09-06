#include "../global.h"
#include "MiniObjects.h"
#include "ScriptObjects.h"
#include <winioctl.h>
#include "../util.h"
#include "../unicode.h"
#include "../malloc.h"

#include "../sdk/nvapi.h"

#include "../G15Util/G15Hid.h"

#ifdef X64
#pragma comment(lib, "sdk\\nvapi64.lib")
#else
#pragma comment(lib, "sdk\\nvapi.lib")
#endif

int InitNvApi() {
	static int initialized = 0;
	if (initialized) return initialized == 1;
	int res = (NVAPI_OK == NvAPI_Initialize());
	initialized = 2*(res)-1;
	return res;
}

int NvGPUThermalSettingsType;
void NvAPI_GPU_GetThermalSettings(ScriptValue &s, ScriptValue *args) {
	if (InitNvApi()) {
		NvPhysicalGpuHandle GPUs[NVAPI_MAX_PHYSICAL_GPUS*4];
		NvU32 numGPUs = 0;
		if (NVAPI_OK == NvAPI_EnumPhysicalGPUs(GPUs, &numGPUs) && numGPUs > 0 && numGPUs <= NVAPI_MAX_PHYSICAL_GPUS*4) {
			if (CreateListValue(s, numGPUs)) {
				for (NvU32 i=0; i<numGPUs; i++) {
					NV_GPU_THERMAL_SETTINGS temps;
					temps.version = NV_GPU_THERMAL_SETTINGS_VER;
					int happy = 0;
					if (NvAPI_GPU_GetThermalSettings(GPUs[i], NVAPI_THERMAL_TARGET_ALL, &temps) == NVAPI_OK && temps.count) {
						ScriptValue list;
						if (CreateListValue(list, temps.count) && s.listVal->PushBack(list)) {
							happy = 1;
							for (NvU32 j=0; j<temps.count; j++) {
								ScriptValue obj;
								if (CreateObjectValue(obj, NvGPUThermalSettingsType)) {
									CreateIntValue(obj.objectVal->values[0], temps.sensor[j].controller);
									// int needed because they're signed, despite being NvU32s.  Brilliant!
									CreateIntValue(obj.objectVal->values[1], (int)temps.sensor[j].defaultMinTemp);
									CreateIntValue(obj.objectVal->values[2], (int)temps.sensor[j].defaultMaxTemp);
									CreateIntValue(obj.objectVal->values[3], (int)temps.sensor[j].currentTemp);
									CreateIntValue(obj.objectVal->values[4], temps.sensor[j].target);
								}
								list.listVal->PushBack(obj);
							}
						}
					}
					if (!happy) s.listVal->PushBack();
				}
			}
		}
	}
}

void NvAPI_GPU_GetTachReading(ScriptValue &s, ScriptValue *args) {
	if (InitNvApi()) {
		NvPhysicalGpuHandle GPUs[NVAPI_MAX_PHYSICAL_GPUS*4];
		NvU32 numGPUs = 0;
		if (NVAPI_OK == NvAPI_EnumPhysicalGPUs(GPUs, &numGPUs) && numGPUs > 0 && numGPUs <= NVAPI_MAX_PHYSICAL_GPUS*4) {
			if (CreateListValue(s, numGPUs)) {
				for (NvU32 i=0; i<numGPUs; i++) {
					NvU32 value;
					int happy = 0;
					if (NvAPI_GPU_GetTachReading(GPUs[i], &value) == NVAPI_OK) {
						if (s.listVal->PushBack(value)) {
							happy = 1;
						}
					}
					if (!happy) s.listVal->PushBack();
				}
			}
		}
	}
}

typedef BOOL (__cdecl* fNvCplGetThermalSettings)( IN UINT nWindowsMonitorNumber, OUT DWORD* pdwCoreTemp, OUT DWORD* pdwAmbientTemp, OUT DWORD* pdwUpperLimit );

int NvThermalSettingsType;
void MyNvCplGetThermalSettings(ScriptValue &s, ScriptValue *args) {
	static fNvCplGetThermalSettings NvCplGetThermalSettings = 0;
	if (!NvCplGetThermalSettings) {
		HMODULE nvDll = LoadLibraryA("nvcpl.dll");
		if (nvDll) {
			NvCplGetThermalSettings = (fNvCplGetThermalSettings)GetProcAddress(nvDll, "NvCplGetThermalSettings");
			if (!NvCplGetThermalSettings) FreeLibrary(nvDll);
		}
		if (!NvCplGetThermalSettings) {
			NvCplGetThermalSettings = (fNvCplGetThermalSettings)-1;
		}
	}
	if (NvCplGetThermalSettings == (fNvCplGetThermalSettings)-1) return;
	DWORD v[3];
	if (NvCplGetThermalSettings(args[0].i32, v, v+1, v+2) && CreateObjectValue(s, NvThermalSettingsType)) {
		CreateIntValue(s.objectVal->values[0], v[0]);
		CreateIntValue(s.objectVal->values[1], v[1]);
		CreateIntValue(s.objectVal->values[2], v[2]);
	}
}



/*
struct SMARTData {
   DWORD         cBufferSize;
   DRIVERSTATUS  DriverStatus;
   USHORT  wGenConfig;
   USHORT  wNumCyls;
   USHORT  wReserved;
   USHORT  wNumHeads;
   USHORT  wBytesPerTrack;
   USHORT  wBytesPerSector;
   USHORT  wSectorsPerTrack;
   USHORT  wVendorUnique[3];
   CHAR    sSerialNumber[20];
   USHORT  wBufferType;
   USHORT  wBufferSize;
   USHORT  wECCSize;
   CHAR    sFirmwareRev[8];
   CHAR    sModelNumber[40];
   USHORT  wMoreVendorUnique;
   USHORT  wDoubleWordIO;
   USHORT  wCapabilities;
   USHORT  wReserved1;
   USHORT  wPIOTiming;
   USHORT  wDMATiming;
   USHORT  wBS;
   USHORT  wNumCurrentCyls;
   USHORT  wNumCurrentHeads;
   USHORT  wNumCurrentSectorsPerTrack;
   ULONG   ulCurrentSectorCapacity;
   USHORT  wMultSectorStuff;
   ULONG   ulTotalAddressableSectors;
   USHORT  wSingleWordDMA;
   USHORT  wMultiWordDMA;
   BYTE    bReserved[128];
   BYTE junk[256];
};

typedef struct _SRB_IO_CONTROL
{
ULONG HeaderLength;
UCHAR Signature[8];
ULONG Timeout;
ULONG ControlCode;
ULONG ReturnCode;
ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;
//*/

#define IDE_ATAPI_IDENTIFY 0xA1 // Returns ID sector for ATAPI.
#define IDE_ATA_IDENTIFY 0xEC // Returns ID sector for ATA.

#pragma pack(push, 1)
struct SMART_VALUE {
	unsigned int id:8, flags:16, current:8;
	unsigned __int64 worst:8;
	// Could be longer...No clue
	unsigned __int64 raw:40;
	__int64 reserved:16;
};
#pragma pack(pop)

struct SMART_OUT {
	ULONG  cBufferSize;
	DRIVERSTATUS  DriverStatus;
	unsigned short count;
	SMART_VALUE value[43];
};
int SmartAttributeType;

void GetDiskSmartInfo(ScriptValue &s, ScriptValue *args) {
	char temp[600];
	if (args->stringVal->len > 500) return;
	char *name = temp;
	if (args->stringVal->value[0] == '\\') {
		name = (char*)args->stringVal->value;
	}
	else if (args->stringVal->value[0]>= '0' && args->stringVal->value[0] <= '9') {
		sprintf(temp, "\\\\.\\PhysicalDrive%s", args->stringVal->value);
	}
	else {
		sprintf(temp, "\\\\.\\%s", args->stringVal->value);
	}
	size_t test = strlen(name);
	int drive = -1;
	// Remove '\' in "X:\"
	if (test == 7 && name[6] == '\\') name[6] = 0;
	else if (test >= 18) {
		char t = name[17];
		name[17] = 0;
		if (!strcmp(name, "\\\\.\\PhysicalDrive")) {
			name[17] = t;
			drive = strtol(name+17, 0, 10);
		}
	}
	HANDLE hDevice = CreateFileA(name,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE, 
								0,
								OPEN_EXISTING,
								0,
								0);
	if (hDevice != INVALID_HANDLE_VALUE) {
		GETVERSIONINPARAMS params;
		unsigned long size=0;
		int version = DeviceIoControl(hDevice, SMART_GET_VERSION, 0, 0, &params, sizeof(params), &size, 0);
		if (version && params.fCapabilities & 4) {
			if (drive == -1) {
				STORAGE_DEVICE_NUMBER num;
				DWORD out;
				drive = 0;
				int res = DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, 0, 0, &num, sizeof(num), &out, 0);
				if (res) drive = num.PartitionNumber;
			}
			SENDCMDINPARAMS in;
			memset(&in, 0, sizeof(in));
			in.cBufferSize = 512;
			in.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;
			in.irDriveRegs.bSectorCountReg = 1;
			in.irDriveRegs.bSectorNumberReg = 1;
			in.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
			in.irDriveRegs.bCylHighReg = SMART_CYL_HI;
			in.irDriveRegs.bDriveHeadReg = 0xA0 | ((drive&1)<<4);
			in.irDriveRegs.bCommandReg = SMART_CMD;
			in.bDriveNumber = drive;
			SMART_OUT read;
			memset(&read, 0, sizeof(read));
			version = DeviceIoControl(hDevice,
										SMART_RCV_DRIVE_DATA,
										&in, sizeof(in),
										&read,
										sizeof(read),
										&size, 0);
			if (version && /*read.count &&*/ CreateListValue(s, 256)) {
				ScriptValue sv, *sv2;
				CreateNullValue(sv);
				s.listVal->Insert(sv, 255);
				//if (read.count > 40) read.count = 40;
				for (int i=0; 2+(i+1)*12 < 512; i++) {
					unsigned char id = read.value[i].id;
					if (id == 0 || s.listVal->vals[id].type != SCRIPT_NULL) break;
					if (CreateObjectValue(s.listVal->vals[id], SmartAttributeType)) {
						sv2 = s.listVal->vals[id].objectVal->values;
						CreateIntValue(sv2[0], read.value[i].raw);
						CreateIntValue(sv2[1], read.value[i].current);
						CreateIntValue(sv2[2], read.value[i].worst);
						CreateIntValue(sv2[3], read.value[i].flags);
					}
				}
			}
		}
		CloseHandle(hDevice);
	}
}

				/*if ((test2 = QueryDosDeviceA(name+4, temp2, sizeof(temp2))) && test2 >= 23) {
					char t = temp2[22];
					temp2[22] = 0;
					if (!stricmp(temp2, "\\Device\\HarddiskVolume")) {
						temp2[22] = t;
						char *e;
						drive = strtol(temp2+22, &e, 10);
						if (e != temp2+22) {
							sprintf(temp, "\\\\.\\PhysicalDrive%i", drive);
							name = temp;
							test = strlen(name);
						}
					}
				}//*/

int __cdecl FindDataCmp(const void *v1, const void *v2) {
	WIN32_FIND_DATAW *f1 = (WIN32_FIND_DATAW *)v1,
					 *f2 = (WIN32_FIND_DATAW *)v2;
	if ((f1->dwFileAttributes ^ f2->dwFileAttributes)&FILE_ATTRIBUTE_DIRECTORY) {
		return 1-((f1->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) *2 / FILE_ATTRIBUTE_DIRECTORY);
	}
	wchar_t *s1 = f1->cFileName, *s2 = f2->cFileName;
	wchar_t *p1, *p2;
	while (1) {
		p1 = wcschr(s1, '.'); 
		p2 = wcschr(s2, '.');
		if (!p1) {
			if (!p2) break;
			int len = (int)(p2-s2);
			int cmp = wcsnicmp(s1, s2, len);
			if (cmp) return cmp;
			return  1 - 2*!s1[len];
		}
		if (!p2) {
			int len = (int)(p1-s1);
			int cmp = wcsnicmp(s1, s2, len);
			if (cmp) return cmp;
			return  -1 + 2*!s2[len];
		}
		int len = (int)(p1-s1);
		if (p2-s2 < len) len = (int)(p2-s2);
		int cmp = wcsnicmp(s1, s2, len);
		if (cmp) return cmp;
		if (s1[len] == s2[len]) {
			s1 += len+1;
			s2 += len+1;
			continue;
		}
		if (!s1[len]) return -1;
		if (!s2[len]) return 1;
		if (s1[len] == '.') return -1;
		return 1;
	}
	return wcsicmp(s1, s2);
	//wchar_t *s1 = ((WIN32_FIND_DATAW *)f1)->cFileName, *s2 = ((WIN32_FIND_DATAW *)f2)->cFileName;
	//wchar_t *p1, p2;
	//return wcsicmp(((WIN32_FIND_DATAW *)f1)->cFileName, ((WIN32_FIND_DATAW *)f2)->cFileName);
}

int FileInfoType;
void GetFileInfo(ScriptValue &s, ScriptValue *args) {
	wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value);
	WIN32_FIND_DATAW *files = (WIN32_FIND_DATAW*)malloc(sizeof(WIN32_FIND_DATAW) * 128);
	DWORD attribMask = args[1].i32;
	DWORD attribs = args[2].i32;
	if (path && files) {
		int numFiles = 0;
		HANDLE hFind = FindFirstFileW(path, &files[numFiles]);
		if (hFind != INVALID_HANDLE_VALUE) {
			while (1) {
				if (files[numFiles].cFileName[0] != '.' || files[numFiles].cFileName[1] != 0) {
					if (attribs == (files[numFiles].dwFileAttributes & attribMask)) {
						if (numFiles % 128 == 118) {
							if (!srealloc(files, sizeof(WIN32_FIND_DATAW)*(numFiles+138))) break;
						}
						numFiles++;
					}
				}
				if (FindNextFileW(hFind, &files[numFiles]) == 0) break;
			}
			FindClose(hFind);
		}
		qsort(files, numFiles, sizeof(WIN32_FIND_DATAW), FindDataCmp);
		if (CreateListValue(s, numFiles)) {
			for (int i=0; i<numFiles; i++) {
				ScriptValue sv;
				if (CreateObjectValue(sv, FileInfoType)) {
					CreateStringValue(sv.objectVal->values[0], files[i].cFileName);
					CreateIntValue(sv.objectVal->values[1], (1+(__int64)MAXDWORD)*(__int64)files[i].nFileSizeHigh + files[i].nFileSizeLow);
					CreateIntValue(sv.objectVal->values[2], files[i].dwFileAttributes);
					CreateDoubleValue(sv.objectVal->values[3], FileTimeToUsefulTime(&files[i].ftCreationTime));
					CreateDoubleValue(sv.objectVal->values[4], FileTimeToUsefulTime(&files[i].ftLastWriteTime));
					s.listVal->PushBack(sv);
				}
			}
		}
	}
	free(path);
	free(files);
}

void GetDriveList(ScriptValue &s, ScriptValue *args) {
	DWORD test = GetLogicalDrives();
	int count = 0;
	for (int i=0; i<26; i++) {
		if (test & (1<<i)) count++;
	}
	char name[4] = "C:\\";
	if (CreateListValue(s, 1+count)) {
		for (int i=0; i<26; i++) {
			ScriptValue sv;
			name[0] = 'A' + i;
			if ((test & (1<<i)) && CreateStringValue(sv, (unsigned char*)name)) {
				s.listVal->PushBack(sv);
			}
		}
	}
}


int VolumeInfoType;
void ScriptGetVolumeInformation(ScriptValue &s, ScriptValue *args) {
	wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value);
	if (path) {
		wchar_t volumeName[MAX_PATH+1];
		wchar_t fileSystem[MAX_PATH+1];
		DWORD volumeSerialNumber;
		DWORD maximumComponentLength;
		DWORD fileSystemFlags;
		if (GetVolumeInformation(path, volumeName, MAX_PATH+1, &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags, fileSystem, MAX_PATH+1) && CreateObjectValue(s, VolumeInfoType)) {
			CreateStringValue(s.objectVal->values[0], volumeName);
			CreateIntValue(s.objectVal->values[1], volumeSerialNumber);
			CreateIntValue(s.objectVal->values[2], maximumComponentLength);
			CreateIntValue(s.objectVal->values[3], fileSystemFlags);
			CreateStringValue(s.objectVal->values[4], fileSystem);
			CreateIntValue(s.objectVal->values[5], GetDriveType(path));
		}
		free(path);
	}
}



extern int screenSaverOn;
extern int monitorOff;

int SystemStateType;
void GetSystemState(ScriptValue &s, ScriptValue *args) {
	if (CreateObjectValue(s, SystemStateType)) {
		CreateIntValue(s.objectVal->values[0], screenSaverOn);
		CreateIntValue(s.objectVal->values[1], monitorOff);
	}
}

int DeviceStateType;
void GetDeviceState(ScriptValue &s, ScriptValue *args) {
	int index = GetG15Index(&args[0]);
	if (index < 0) return;
	ScriptValue *out = &s;
	if (index == 0) {
		if (!CreateListValue(s, numDevices)) return;
		s.listVal->Resize(numDevices, numDevices);
		out = s.listVal->vals;
	}
	for (int i=0; i<numDevices; i++) {
		if ((!index || index-1 == i) && CreateObjectValue(*out, DeviceStateType)) {
			Device *dev = devices[i];
			ScriptValue *vals = out->objectVal->values;
			out++;
			dev->id->AddRef();
			CreateStringValue(vals[1], dev->id);
			CreateIntValue(vals[0], dev->hidType);
			CreateIntValue(vals[2], dev->width);
			CreateIntValue(vals[3], dev->height);
			CreateIntValue(vals[4], dev->bpp);
			G15State *g15state;
			G19State *g19state;
			if (g15state = GetG15State(dev)) {
				CreateIntValue(vals[5], g15state->MLights);
				CreateIntValue(vals[6], g15state->light);
				CreateIntValue(vals[7], g15state->LCDLight);
			}
			else if (g19state = GetG19State(dev)) {
				CreateIntValue(vals[5], g19state->MLights);
				CreateIntValue(vals[8], g19state->color.val);
			}
		}
	}
}

