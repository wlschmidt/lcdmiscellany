#ifndef SCRIPTPERFCOUNTER_H
#define SCRIPTPERFCOUNTER_H
#include "..\\ScriptValue.h"

void PerformanceCounter(ScriptValue &s, ScriptValue *args);
void PerformanceCounterGetParameters(ScriptValue &s, ScriptValue *args);
void PerformanceCounterSetAutoUpdate(ScriptValue &s, ScriptValue *args);
void PerformanceCounterGetValue(ScriptValue &s, ScriptValue *args);
void PerformanceCounterGetValueList(ScriptValue &s, ScriptValue *args);
void PerformanceCounterFree(ScriptValue &s, ScriptValue *args);
void PerformanceCounterUpdate(ScriptValue &s, ScriptValue *args);
void PerformanceCounterIsUpdated(ScriptValue &s, ScriptValue *args);

extern unsigned int PerfCounterType;


void GetUpstream(ScriptValue &s, ScriptValue *args);
void GetDownstream(ScriptValue &s, ScriptValue *args);
void GetAllDown(ScriptValue &s, ScriptValue *args);
void GetAllUp(ScriptValue &s, ScriptValue *args);

#endif
