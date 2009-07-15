#ifndef SCRIPTWAIT_H
#define SCRIPTWAIT_H
#include "../../ScriptValue.h"

struct Stack;

void CleanupWaits();
int Wait(ScriptValue *args, Stack *s);
void Sleep(ScriptValue &s, ScriptValue *args);
int HttpGetWait(ScriptValue *args, Stack *stack);
//int MessageBoxWait(ScriptValue *args, Stack *stack);
void ParseXML(ScriptValue &s, ScriptValue *args);
void FindNextXML(ScriptValue &s, ScriptValue *args);
void PartialParseXML(ScriptValue &s, ScriptValue *args);
void SpawnThread(ScriptValue &s, ScriptValue *args);

#endif
