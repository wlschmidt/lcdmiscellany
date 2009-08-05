#include "ScriptFont.h"
#include "ScriptObjects.h"
#include "..\malloc.h"
#include "..\vm.h"
#include "..\SymbolTable.h"
#include "..\Script.h"
#include "..\util.h"
#include "ScriptImage.h"
#include "File.h"
#include "PerfCounter.h"
#include "Socket.h"
#include "MediaFile.h"
#include "MiniObjects.h"
#include "SysTrayIcon.h"

#include "WMI.h"

ObjectType *types = 0;
int numTypes = 0;

void ObjectType::Cleanup() {
	//name->Release();
	free(functs);
	free(values);
}

int ObjectType::FindValue(StringValue *s) {
	int i = VM.StringTable.Find(s);
	if (i<0) return -1;
	return FindValue(i);
}


int ObjectType::AddValue(unsigned char *name, int len) {
	int id = VM.StringTable.Add(name, len);
	if (id < 0) return 0;
	int i;
	for (i=0; i<numValues; i++) {
		if (values[i].stringID == id) return 0;
	}
	if (!srealloc(values, (numValues+1) * sizeof(ObjectTypeValue))) return 0;
	while (i && values[i-1].stringID > id) {
		values[i] = values[i-1];
		i--;
	}
	values[i].stringID = id;
	numValues++;
	return 1;
}

int ObjectType::AddFunction(unsigned char *name, int len, int id) {
	int nameId = VM.StringTable.Add(name, len);
	if (nameId < 0) return 0;
	int i;
	for (i=0; i<numFuncts; i++) {
		if (functs[i].stringID == nameId) return 0;
	}
	if (!srealloc(functs, (numFuncts+1) * sizeof(ObjectTypeFunction))) return 0;
	while (i && functs[i-1].stringID > nameId) {
		functs[i] = functs[i-1];
		i--;
	}
	functs[i].functionID = id;
	functs[i].stringID = nameId;
	numFuncts++;
	return 1;
}

int ObjectType::AddFunction(int stringID, int functionID) {
	if (!srealloc(functs, (numFuncts+1) * sizeof(ObjectTypeFunction))) return 0;
	int i = numFuncts;
	while (i && functs[i-1].stringID > stringID) {
		functs[i] = functs[i-1];
		i--;
	}
	functs[i].functionID = functionID;
	functs[i].stringID = stringID;
	numFuncts++;
	return 1;
}

#pragma pack(push, 1)
struct ProcDescription {
	char *name;
	ScriptProc *proc;
	FunctionType type;
};
#pragma pack(pop)

int CreateObjectType(char *name, const char **vals, int numVals, const ProcDescription *create, int numCreate, const ProcDescription *script, int numScript, const ProcDescription *dest = 0) {
	ObjectType *type = CreateObjectType((unsigned char*)name, 0);
	if (!type) return 0;
	int i;
	for (i=0; i<numVals; i++) {
		if (!type->AddValue((unsigned char*)vals[i], (int)strlen(vals[i]))) return 0;
	}
	for (i=0; i<numCreate; i++) {
		if (!RegisterProc((unsigned char*)create[i].name, create[i].proc, create[i].type)) return 0;
	}
	for (i=0; i<numScript; i++) {
		char *temp = (char*)malloc(strlen(script[i].name) + strlen(name) + 3);
		if (!temp) return 0;
		sprintf(temp, "%s::%s", name, script[i].name);
		if (VM.StringTable.Add((unsigned char*)temp) < 0) return 0;
		if (!RegisterProc((unsigned char*)temp, script[i].proc, script[i].type)) return 0;
		free(temp);
		VM.Functions.table[VM.Functions.numValues-1].data.needThis = 1;
		if (!type->AddFunction((unsigned char*)script[i].name, (int)strlen(script[i].name), VM.Functions.numValues-1)) return 0;
	}
	if (dest) {
		if (!RegisterProc((unsigned char*)dest[0].name, dest[0].proc, dest[0].type)) return 0;
		if (VM.StringTable.Add((unsigned char*)dest[0].name) < 0) return 0;
		VM.Functions.table[VM.Functions.numValues-1].data.needThis = 1;
		type->CleanupFunction = VM.Functions.numValues-1;
	}
	return 1;
}

int RegisterObjectTypes() {
	int count = 0;
	int res = 1;
	// Value order matters...have to be careful.
	{
		const char *vals[] = {"coreTemp", "ambientTemp", "tempLimit"};
		const ProcDescription create[] = {{"NvThermalSettings", MyNvCplGetThermalSettings, C_int}
		};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("NvThermalSettings", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0);
		NvThermalSettingsType = count++;
	}

	res &= RegisterProc((unsigned char*)"NvAPI_GPU_GetTachReading", NvAPI_GPU_GetTachReading, C_noArgs);

	// Value order matters...have to be careful.
	{
		const char *vals[] = {"controller", "defaultMinTemp", "defaultMaxTemp", "currentTemp", "target"};
		const ProcDescription create[] = {{"NvAPI_GPU_GetThermalSettings", NvAPI_GPU_GetThermalSettings, C_noArgs}
		};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("NvGPUThermalSettings", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0);
		NvGPUThermalSettingsType = count++;
	}

	// Value order matters...have to be careful.
	{
		const char *vals[] = {"name", "bytes", "attributes", "created", "modified"};
		const ProcDescription create[] = {{"FileInfo", GetFileInfo, C_string2ints},
									{"DriveList", GetDriveList, C_noArgs},
		};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("GetFiles", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0);
		FileInfoType = count++;
	}
	{
		const char *vals[] = {"volumeName", "volumeSerialNumber", "maximumComponentLength", "fileSystemFlags", "fileSystem", "driveType"};
		const ProcDescription create[] = {{"VolumeInformation", ScriptGetVolumeInformation, C_string},
		};
		res &= CreateObjectType("VolumeInformation", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0);
		VolumeInfoType = count++;
	}

	{
		const char *vals[] = {"value", "normalized", "worst", "flags"};
		const ProcDescription create[] = {{"GetDiskSmartInfo", GetDiskSmartInfo, C_string}
		};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("SmartAttribute", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0);
		SmartAttributeType = count++;
	}
	{
		const char *vals[] = {"&1","&2","&3","&4","&5"};
		const ProcDescription create[] = {{"FileReader", OpenFileReader, C_stringint}};
		const ProcDescription script[] = {{"Read", FileReaderRead, C_int}};
		const ProcDescription dest[] = {{"FileReader::~FileReader", CleanupFileReader, C_noArgs}};
		res &= CreateObjectType("FileReader", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		FileReaderType = count++;
	}
	{
		const char *vals[] = {"&1","&2","&3","&4","&5"};
		const ProcDescription create[] = {{"FileWriter", OpenFileWriter, C_stringint}};
		const ProcDescription script[] = {{"Write", FileWriterWrite, C_string}};
		const ProcDescription dest[] = {{"FileWriter::~FileWriter", CleanupFileWriter, C_noArgs}};
		res &= CreateObjectType("FileWriter", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		FileWriterType = count++;
	}
	/*{
		char *vals[] = {"&1","&2","&3", "&4"};
		const ProcDescription create[] = {{"ListFiles", ListFiles, C_stringint},
									{"ResolvePath", ListFiles, C_string}};
		const ProcDescription script[] = {{"Size", FileSize, C_noArgs},
									{"Name", FileName, C_noArgs},
									{"Path", FilePath, C_noArgs},
									{"IsDirectory", FileIsDirectory, C_noArgs}};
		res &= CreateObjectType("File", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]));
		FileType = count++;
	}//*/

	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"LoadImage", MyLoadImage, C_stringdoubleint},
									{"LoadMemoryImage", LoadMemoryImage, C_stringdoubleint},
									{"ClearImage", ClearImage, C_obj6ints},
									{"InvertImage", InvertImage, C_obj6ints},
									{"IntersectImage", IntersectImage, C_obj6ints},
									{"DrawImage", DrawImage, C_obj6ints},
									{"DrawTransformedImage", DrawTransformedImage, C_obj16doubles},
									{"DrawRotatedScaledImage", DrawRotatedScaledImage, C_obj5doubles},
									{"GetClipboardData", ScriptGetClipboardData, C_2ints},
									// Only here because GetClipboardData() is.
									{"SetClipboardData", ScriptSetClipboardData, C_direct}
		};
		const ProcDescription script[] = {{"Size", ImageSize, C_noArgs}};
		res &= CreateObjectType("Image", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]));
		ImageType = count++;
	}

	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"LoadImage32", LoadImage32, C_string},
									{"LoadMemoryImage32", LoadMemoryImage32, C_string},
		};
		const ProcDescription script[] = {{"Size", Image32Size, C_noArgs},
									{"Zoom", Image32Zoom, C_double},
									{"ToImage", Image32ToBitImage, C_doubleint},
									{"SaveBMP", Image32SaveBMP, C_string},
		};
		res &= CreateObjectType("Image32", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]));
		Image32Type = count++;
	}

	{
		const char *vals[] = {"&1", "&2"};
		const ProcDescription create[] = {{"PerformanceCounter", PerformanceCounter, C_3stringsint},
									{"GetUpstream", GetUpstream, C_noArgs},
									{"GetDownstream", GetDownstream, C_noArgs},
									{"GetAllUp", GetAllUp, C_noArgs},
									{"GetAllDown", GetAllDown, C_noArgs},
									};
		const ProcDescription script[] = {{"GetParameters", PerformanceCounterGetParameters, C_noArgs},
									{"SetAutoUpdate", PerformanceCounterSetAutoUpdate, C_int},
									{"Update", PerformanceCounterUpdate, C_noArgs},
									{"IsUpdated", PerformanceCounterIsUpdated, C_noArgs},
									{"GetValue", PerformanceCounterGetValue, C_noArgs},
									{"GetValueList", PerformanceCounterGetValueList, C_noArgs},
		};
		const ProcDescription dest[] = {{"PerformanceCounter::~PerformanceCounter", PerformanceCounterFree, C_noArgs}};
		res &= CreateObjectType("PerformanceCounter", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		PerfCounterType = count++;
	}

	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"SysTrayIcon", SysTrayIcon, C_2stringsobj},
										  {"NeedRedrawIcons", NeedRedrawSysTrayIcons, C_noArgs},
									};
		const ProcDescription script[] = {{"NeedRedraw", SysTrayIconNeedRedraw, C_noArgs},
										  {"Destroy", SysTrayIconFree, C_noArgs},
		};
		const ProcDescription dest[] = {{"SysTrayIcon::~SysTrayIcon", SysTrayIconFree, C_noArgs}};

		res &= CreateObjectType("SysTrayIcon", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		SysTrayIconType = count++;
	}
/*	{
		char *vals[] = {"&1"};
		const ProcDescription create[] = {{"LoadColorImage", LoadColorImage, C_stringdouble},
									{"LoadMemoryColorImage", LoadMemoryColorImage, C_stringdouble},
									{"ClearImage", ClearImage, C_obj6ints},
									{"InvertImage", InvertImage, C_obj6ints},
									{"DrawImage", DrawImage, C_obj6ints}};
		const ProcDescription script[] = {{"Size", ColorImageSize, C_noArgs}};
		CreateObjectType("ColorImage", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]));
		ColorImageType = count++;
	}
	//*/
	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"Font", Font, C_string5ints},
										  {"UseFont", UseFont, C_obj},
										  {"GetFontHeight", GetFontHeight, C_obj}};
		//const ProcDescription script[] = {{"GetVals", GetFontVals, C_noArgs}};
		const ProcDescription dest[] = {{"Font::~Font", DeleteFont, C_noArgs}};
		res &= CreateObjectType("Font", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0, dest);
		FontType = count++;
	}
	/*{
		char *vals[] = {"&1"};
		const ProcDescription create[] = {{"WMIMonitor", WMIMonitor, C_2strings}};
		const ProcDescription script[] = {{"GetVals", WMIGetVals, C_noArgs}};
		const ProcDescription dest[] = {{"WMIMonitor::~WMIMonitor", DeleteWMIMonitor, C_noArgs}};
		res &= CreateObjectType("WMIMonitor", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		WMIValueType = count++;
	}//*/
	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"ConnectWait", (ScriptProc*)ConnectWait, C_directwait},
										  {"Socket", SocketSocket, C_direct},
		};
		const ProcDescription script[] = {{"Send", SocketSend, C_string},
										  {"GetStatus", SocketStatus, C_noArgs},
										  {"Read", SocketRead, C_int},
										  {"StartTLS", SocketStartTLS, C_noArgs},
										  {"ReadFrom", SocketReadFrom, C_noArgs},
		};
		const ProcDescription dest[] = {{"Socket::~Socket", CleanupSocket, C_noArgs}};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("Socket", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		SocketType = count++;
	}
	{
		const char *vals[] = {"&1", "&2", "&3", "&4"};
		const ProcDescription create[] = {{"IPAddr", IPAddr, C_stringint},
										  {"IPAddrWait", (ScriptProc*)IPAddrWait, C_directwait},
		};
		const ProcDescription script[] = {{"GetString", IPAddrGetIP, C_noArgs},
										  {"PingWait", (ScriptProc*)IPAddrPingWait, C_directwait},
										  {"GetDNSWait", (ScriptProc*)IPAddrGetDNSWait, C_directwait},
		};
		//const ProcDescription dest[] = {{"IPAddr::~IPAddr", FreeIPAddr, C_noArgs}};
		res &= CreateObjectType("IPAddr", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]));
		AddrType = count++;
	}
/*
void MediaFile(ScriptValue &s, ScriptValue *args);
void MediaFilePlay(ScriptValue &s, ScriptValue *args);
void MediaFilePause(ScriptValue &s, ScriptValue *args);
void MediaFileStop(ScriptValue &s, ScriptValue *args);
void MediaFileInfo(ScriptValue &s, ScriptValue *args);
void MediaFileSet(ScriptValue &s, ScriptValue *args);
void DestroyMediaFile(ScriptValue &s, ScriptValue *args);

void MediaFileAuthor(ScriptValue &s, ScriptValue *args);
void MediaFileTitle(ScriptValue &s, ScriptValue *args);
void MediaFileDescription(ScriptValue &s, ScriptValue *args);
void MediaFileCopyright(ScriptValue &s, ScriptValue *args);
//*/
	{
		const char *vals[] = {"&1"};
		const ProcDescription create[] = {{"MediaFile", MediaFile, C_stringint}};
		const ProcDescription script[] = {
			{"Play", MediaFilePlay, C_noArgs},
			{"Stop", MediaFileStop, C_int},

			{"SetVolume", MediaFileSetVolume, C_int},
			{"SetBalance", MediaFileSetBalance, C_int},

			{"SetPosition", MediaFileSetPosition, C_double},
			{"GetPosition", MediaFileGetPosition, C_noArgs},
			{"GetDuration", MediaFileGetDuration, C_noArgs},

			{"Pause", MediaFilePause, C_noArgs},
			{"GetState", MediaFileGetState, C_noArgs},

			{"GetFFT", MediaFileGetFFT, C_int},

			{"GetAuthor", MediaFileAuthor, C_noArgs},
			{"GetTitle", MediaFileTitle, C_noArgs},
			{"GetDescription", MediaFileDescription, C_noArgs},
			{"GetCopyright", MediaFileCopyright, C_noArgs},
		};
		const ProcDescription dest[] = {{"DestroyMediaFile", DestroyMediaFile, C_noArgs}};
		res &= CreateObjectType("MediaFile", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), script, sizeof(script)/sizeof(script[0]), dest);
		MediaFileType = count++;
	}

	{
		const char *vals[] = {"screenSaverOn", "monitorOff"};
		const ProcDescription create[] = {{"SystemState", GetSystemState, C_noArgs}};
		res &= CreateObjectType("SystemState", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0, 0);
		SystemStateType = count++;
	}

	{
		const char *vals[] = {"name", "type", "width", "height", "bpp", "mkeys", "light", "LCDLight", "LCDColor"};
		const ProcDescription create[] = {{"GetDeviceState", GetDeviceState, C_string}};
		res &= CreateObjectType("DeviceState", vals, sizeof(vals)/sizeof(vals[0]), create, sizeof(create)/sizeof(create[0]), 0, 0, 0);
		DeviceStateType = count++;
	}
	return res && InitFonts();
}

void CleanupObjectTypes() {
	CleanupFonts();
	for (int i=0; i < numTypes; i++) {
		types[i].Cleanup();
	}
	free(types);
}

ObjectType *CreateObjectType(unsigned char *type, int canSubclass) {
	if (!srealloc(types, sizeof(ObjectType) * (numTypes+1))) return 0;
	int id = VM.StringTable.Add(type);
	if (id < 0) return 0;

	int index = 0;
	while (index < numTypes) {
		if (id == types[index].nameID) return 0;
		index ++;
	}
	memset(types+numTypes, 0, sizeof(ObjectType));
	types[numTypes].canSubclass = canSubclass;
	types[numTypes].nameID = id;
	types[numTypes].type = numTypes;
	return &types[numTypes++];
}

ObjectType *FindObjectType(unsigned char *type, int canSubclass) {
	int id = VM.StringTable.Add(type);
	if (id < 0) return 0;
	int index = 0;
	while (index < numTypes) {
		if (id == types[index].nameID) return &types[index];
		index ++;
	}
	return 0;
}

/*
void CallObjectFunction(ScriptValue &obj, int funct, ScriptValue &params) {
	ScriptValue params2;
	if (params.type == SCRIPT_LIST) {
	}
	else {
		CreateListValue(params2, 1);
	}
	RunFunction(types[obj.objectVal->type].functs[funct].functionID, params2, CAN_DO_NOTHING);
}
//*/
int CreateObjectValue(ScriptValue &s, int type) {
	ObjectValue *v = (ObjectValue*) malloc(sizeof(ObjectValue) + sizeof(ScriptValue) * (types[type].numValues-1));
	if (!v) return 0;
	v->type = type;
	v->refCount = 1;
	for (int i=0; i<types[type].numValues; i++) {
		CreateNullValue(v->values[i]);
	}
	memset(&s, 0, sizeof(s));
	s.type = SCRIPT_OBJECT;
	s.objectVal = v;
	return 1;
}


void ObjectValue::Release() {
	if (--refCount == 0) {
		if (types[type].CleanupFunction) {
			// Don't clean up again.
			refCount = 3;
			RunFunction(this, types[type].CleanupFunction, CAN_DO_NOTHING);
		}
		for (int i=types[type].numValues-1; i>=0; i--) {
			values[i].Release();
		}
		nfree(this);
	}
}

int ObjectType::FindFunction(StringValue *s) {
	int v = VM.StringTable.Find(s);
	if (v < 0) return -1;
	return FindFunction(v);
}
