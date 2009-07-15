#include "..\ScriptValue.h"

// Not actually an object type.  Fits with other nvidia stuff, though.
void NvAPI_GPU_GetTachReading(ScriptValue &s, ScriptValue *args);

extern int NvGPUThermalSettingsType;
void NvAPI_GPU_GetThermalSettings(ScriptValue &s, ScriptValue *args);

extern int NvThermalSettingsType;
void MyNvCplGetThermalSettings(ScriptValue &s, ScriptValue *args);


void GetDiskSmartInfo(ScriptValue &s, ScriptValue *args);
void GetFileInfo(ScriptValue &s, ScriptValue *args);
void GetDriveList(ScriptValue &s, ScriptValue *args);


extern int SmartAttributeType;
extern int FileInfoType;


extern int VolumeInfoType;
void ScriptGetVolumeInformation(ScriptValue &s, ScriptValue *args);

extern int SystemStateType;
void GetSystemState(ScriptValue &s, ScriptValue *args);

extern int DeviceStateType;
void GetDeviceState(ScriptValue &s, ScriptValue *args);
