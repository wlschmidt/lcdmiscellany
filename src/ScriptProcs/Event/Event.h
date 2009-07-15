#ifndef EVENT_H
#define EVENT_H

#include "../../vm.h"
#include "../../ScriptValue.h"

void SetEventHandler(ScriptValue &s, ScriptValue *args);
void CleanupEvents();
int InitEvents();
//int TriggerEvent(ScriptValue &s, ScriptMode mode = CAN_WAIT);
//int TriggerEvent(char *s, int val, ScriptMode mode = CAN_WAIT);
//int TriggerEvent(char *s, ScriptValue &v1, ScriptValue &v2, ScriptMode mode = CAN_WAIT);

struct EventParams {
	ScriptValue args[2];
	int id;
};

struct FullEventParams {
	ScriptValue args;
	int id;
};

int GetEventId(unsigned char *string, int make = 1);
inline int GetEventId(char *string) {
	return GetEventId((unsigned char*)string);
}

int TriggerEvent(int id, ScriptMode mode = CAN_WAIT, ScriptValue *v1=0, ScriptValue *v2=0, ScriptValue *v3=0);


inline int TriggerEvent(int id, ScriptValue *v1, ScriptValue *v2=0, ScriptValue *v3=0) {
	return TriggerEvent(id, CAN_WAIT, v1, v2, v3);
}

/*
int TriggerEvent(int id, ScriptValue &v1, ScriptValue &v2, ScriptMode mode = CAN_WAIT);
int TriggerEvent(int id, ScriptValue &v1, ScriptValue &v2, ScriptValue &v3, ScriptMode mode = CAN_WAIT);
int TriggerEvent(int id, ScriptMode mode = CAN_WAIT);
int TriggerEvent(int id, ScriptValue &v1, ScriptMode mode = CAN_WAIT);
//*/
inline int TriggerEvent(int id, int val, ScriptMode mode = CAN_WAIT) {
	ScriptValue sv;
	CreateIntValue(sv, val);
	return TriggerEvent(id, mode, &sv);
}

inline int TriggerEvent(int id, __int64 v1, __int64 v2, ScriptMode mode = CAN_WAIT) {
	ScriptValue sv1, sv2;
	CreateIntValue(sv1, v1);
	CreateIntValue(sv2, v2);
	return TriggerEvent(id, mode, &sv1, &sv2);
}

int TriggerEventFull(int id, ScriptValue &list, ScriptMode mode = CAN_WAIT);

int EventExists(int id);

void PostEvent(ScriptValue &s, ScriptValue *args);

// Internal event types.  Initialized by InitEvents().
extern int clipboardEvent;
extern int counterUpdateEvent;
extern int screenSaverEvent;
extern int monitorPowerEvent;
extern int foregroundWindowChangeEvent;
extern int keyDownEvent;
extern int keyUpEvent;
extern int drawEvent;
extern int g15ButtonDownEvent;
extern int g15ButtonUpEvent;
extern int execEvent;
extern int mediaEvent;
extern int userEvent;
extern int audioChangeEvent;
extern int quitEvent;

#endif
