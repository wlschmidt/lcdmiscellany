#ifndef SCRIPTIP_H
#define SCRIPTIP_H
#include "../../ScriptValue.h"
void CleanupIP();
void GetLocalIP(ScriptValue &s, ScriptValue *args);
void GetRemoteIP(ScriptValue &s, ScriptValue *args);
void GetRemoteDNS(ScriptValue &s, ScriptValue *args);
void GetLocalDNS(ScriptValue &s, ScriptValue *args);
void CheckRIP();
#endif
