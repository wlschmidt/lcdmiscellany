#ifndef SCRIPTWINDOWMANAGEMENT_H
#define SCRIPTWINDOWMANAGEMENT_H
#include "../../ScriptValue.h"

void GetForegroundWindow(ScriptValue &s, ScriptValue *args);
void MyGetWindowText(ScriptValue &s, ScriptValue *args);
void MyGetWindowModuleFileName(ScriptValue &s, ScriptValue *args);
void CloseWindow(ScriptValue &s, ScriptValue *args);
void GetWindowProcessId(ScriptValue &s, ScriptValue *args);
void KillProcess(ScriptValue &s, ScriptValue *args);
void ScriptGetWindowRect(ScriptValue &s, ScriptValue *args);
void GetWindowClientRect(ScriptValue &s, ScriptValue *args);

void ScriptPostMessage(ScriptValue &s, ScriptValue *args);
void ScriptSendMessage(ScriptValue &s, ScriptValue *args);

void ScriptIsWindow(ScriptValue &s, ScriptValue *args);
void ScriptFindWindow(ScriptValue &s, ScriptValue *args);
void ScriptEnumWindows(ScriptValue &s, ScriptValue *args);

void ScriptSetProcessPriority(ScriptValue &s, ScriptValue *args);
void ScriptGetProcessPriority(ScriptValue &s, ScriptValue *args);

void MyGetProcessFileName(ScriptValue &s, ScriptValue *args);

void ScriptEnumChildWindows(ScriptValue &s, ScriptValue *args);

void GetScriptWindow(ScriptValue &s, ScriptValue *args);

#endif
