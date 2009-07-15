#ifndef SCRIPT_SOCKET_H
#define SCRIPT_SOCKET_H

#include "../ScriptValue.h"
#include "../vm.h"
int IPAddrWait(ScriptValue *args, Stack *stack);
void IPAddrGetIP(ScriptValue &s, ScriptValue *args);
void IPAddr(ScriptValue &s, ScriptValue *args);
int IPAddrPingWait(ScriptValue *args, Stack *stack);
int IPAddrGetDNSWait(ScriptValue *args, Stack *stack);

void CleanupSocket(ScriptValue &s, ScriptValue *args);

//void HandlePingSocketMessage(WPARAM wParam, LPARAM lParam);
void CleanupPings();
extern unsigned int AddrType;

int ConnectWait(ScriptValue *args, Stack *stack);
extern unsigned int SocketType;

void SocketSend(ScriptValue &s, ScriptValue *args);
void SocketStatus(ScriptValue &s, ScriptValue *args);
void SocketRead(ScriptValue &s, ScriptValue *args);
void SocketStartTLS(ScriptValue &s, ScriptValue *args);

void SocketReadFrom(ScriptValue &s, ScriptValue *args);
void SocketSocket(ScriptValue &s, ScriptValue *args);

void HandleScriptedSocketMessage(WPARAM wParam, LPARAM lParam);

#endif
