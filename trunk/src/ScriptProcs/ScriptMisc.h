#ifndef SCRIPTMISC_H
#define SCRIPTMISC_H
#include "..\\ScriptValue.h"

void CleanupDdraw();
void CleanupDdraw(ScriptValue &s, ScriptValue *args);

void GetVram(ScriptValue &s, ScriptValue *args);

void Suspend(ScriptValue &s, ScriptValue *args);
void Shutdown(ScriptValue &s, ScriptValue *args);
void AbortShutdown(ScriptValue &s, ScriptValue *args);
void LockSystem(ScriptValue &s, ScriptValue *args);
//void GetSpeedFanData(ScriptValue &s, ScriptValue *args);
void FormatValue(ScriptValue &s, ScriptValue *args);
void FormatSize(ScriptValue &s, ScriptValue *args);
void GetUsedMemory(ScriptValue &s, ScriptValue *args);
void GetTotalMemory(ScriptValue &s, ScriptValue *args);
void GetMemoryStatus(ScriptValue &s, ScriptValue *args);

void GetDiskSpaceFree(ScriptValue &s, ScriptValue *args);
void GetDiskSpaceTotal(ScriptValue &s, ScriptValue *args);
void GetDiskSpace(ScriptValue &s, ScriptValue *args);

//void GetVolume(ScriptValue &s, ScriptValue *args);
//void SetMenuOrder(ScriptValue &s, ScriptValue * args);
//void Alert(ScriptValue &s, ScriptValue *args);
void StartScreenSaver(ScriptValue &s, ScriptValue *args);
void list(ScriptValue &s, ScriptValue *args);
void _null(ScriptValue &s, ScriptValue *args);
void MontorPower(ScriptValue &s, ScriptValue *args);

void UnloadFraps(ScriptValue &s, ScriptValue *args);
void GetFrapsData(ScriptValue &s, ScriptValue *args);

void Quit(ScriptValue &s, ScriptValue *args);
void GetVersion(ScriptValue &s, ScriptValue *args);
void GetVersionString(ScriptValue &s, ScriptValue *args);

void WriteLog(ScriptValue &s, ScriptValue *args);
void WriteLogLn(ScriptValue &s, ScriptValue *args);
#endif
