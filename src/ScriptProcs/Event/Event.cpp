#include "../../Global.h"
#include "Event.h"
#include "../../ScriptObjects/ScriptObjects.h"

struct Event {
	ScriptValue param;
	ObjectValue *object;
	int id;
};

SymbolTable<Event> Events;

int GetEventId(unsigned char *string, int make) {
	TableEntry<Event> *entry = Events.Find(string);
	if (entry) {
		return entry->index;
	}
	if (!make) return -1;
	int i = VM.StringTable.Add((unsigned char*) string);
	if (i < 0) return -1;
	for (int j=0; j<VM.numStacks; j++) {
		VM.stacks[j]->strings = VM.StringTable.table;
	}
	VM.StringTable.table[i].AddRef();
	entry = Events.Add(VM.StringTable.table[i].stringVal);
	if (entry) {
		CreateNullValue(entry->data.param);
		entry->data.object = 0;
		entry->data.id = -1;
		return entry->index;
	}
	return -1;
}

void SetEventHandler(ScriptValue &s, ScriptValue *args) {
	if (args[0].stringVal->len == 0) return;
	TableEntry<Event> *entry = Events.Find(args[0].stringVal);
	if (!entry) {
		GetEventId(args[0].stringVal->value, 1);
		entry = Events.Find(args[0].stringVal);
	}
	CreateIntValue(s, 0);
	int res;
	if (!entry) {
		res = 2;
		args[0].stringVal->AddRef();
		entry = Events.Add(args[0].stringVal);
		if (!entry) return;
	}
	else {
		if (entry->data.object) entry->data.object->Release();
		entry->data.param.Release();
		CreateNullValue(entry->data.param);
		res = 1;
	}
	entry->data.object = 0;
	entry->data.id = -1;
	if (args[0].stringVal->len) {
		if (args[2].type == SCRIPT_OBJECT) {
			int v = VM.StringTable.Find(args[1].stringVal);
			if (v >= 0) {
				int f = types[args[2].objectVal->type].FindFunction(v);
				if (f >= 0) {
					entry->data.id = types[args[2].objectVal->type].functs[f].functionID;
					entry->data.object = args[2].objectVal;
					args[2].objectVal->AddRef();
					entry->data.param = args[3];
					args[3].AddRef();
					s.intVal = res;
				}
			}
		}
		else if (args[2].type == SCRIPT_NULL || ((args[2].type == SCRIPT_INT) && args[2].intVal == 0)) {
			if (args[1].stringVal->value[0]) {
				TableEntry<Function> *func = VM.Functions.Find(args[1].stringVal);
				if (func) {
					entry->data.id = func->index;
					s.intVal = res;
					entry->data.param = args[3];
					args[3].AddRef();
				}
			}
		}
	}
	/*
	if (entry->data.id == -1) {
		if (entry->index == clipboardEvent && nextClipboardListener) {
			ChangeClipboardChain(ghWnd, nextClipboardListener);
			nextClipboardListener  = 0;
		}
	}
	else {
		if (entry->index == clipboardEvent && !nextClipboardListener) {
			nextClipboardListener = SetClipboardViewer(ghWnd);
			if (!nextClipboardListener) {
				int w = GetLastError();
				if (!w) nextClipboardListener = INVALID_HANDLE_VALUE;
			}
			nextClipboardListener = nextClipboardListener;
			char junk [10000];
			GetWindowTextA(nextClipboardListener, junk, sizeof(junk));
			w=w;
		}
	}//*/
}

void CleanupEvents() {
	for (int i=0; i<Events.numValues; i++) {
		if (Events.table[i].data.object)
			Events.table[i].data.object->Release();
	}
	Events.Cleanup();
}

int EventExists(int id) {
	if (id < 0 || Events.table[id].data.id < 0) return 0;
	return 1;
}

int TriggerEvent(int id, ScriptMode mode, ScriptValue *v1, ScriptValue *v2, ScriptValue *v3) {
	ScriptValue sv, sv2;
	if (id < 0 || Events.table[id].data.id < 0 || !CreateListValue(sv, 4)) {
		if (v1) {
			v1->Release();
			if (v2) {
				v2->Release();
				if (v3) v3->Release();
			}
		}
		return 0;
	}
	CreateStringValue(sv2, Events.table[id].sv);
	Events.table[id].sv->AddRef();
	sv.listVal->PushBack(sv2);

	Events.table[id].data.param.AddRef();
	sv.listVal->PushBack(Events.table[id].data.param);

	if (v1) {
		sv.listVal->PushBack(*v1);
		if (v2) {
			sv.listVal->PushBack(*v2);
			if (v3) sv.listVal->PushBack(*v3);
		}
	}
	if (Events.table[id].data.object) Events.table[id].data.object->AddRef();
	return RunFunction(Events.table[id].data.id, sv, mode, Events.table[id].data.object);
}


int TriggerEventFull(int id, ScriptValue &list, ScriptMode mode) {
	int len = 2;
	ScriptValue sv, sv2;
	if (list.type == SCRIPT_LIST) len += list.listVal->numVals;
	if (id < 0 || Events.table[id].data.id < 0 || !CreateListValue(sv, len)) {
		list.Release();
		return 0;
	}

	CreateStringValue(sv2, Events.table[id].sv);
	Events.table[id].sv->AddRef();
	sv.listVal->PushBack(sv2);

	Events.table[id].data.param.AddRef();
	sv.listVal->PushBack(Events.table[id].data.param);

	if (list.type == SCRIPT_LIST) {
		for (int i=0; i<list.listVal->numVals; i++) {
			sv.listVal->PushBack(list.listVal->vals[i]);
			list.listVal->vals[i].AddRef();
		}
		list.Release();
	}

	if (Events.table[id].data.object) Events.table[id].data.object->AddRef();
	return RunFunction(Events.table[id].data.id, sv, mode, Events.table[id].data.object);
}

void PostEvent(ScriptValue &s, ScriptValue *args) {
	TableEntry<Event> *entry = Events.Find(args[0].stringVal);
	if (entry && entry->data.id >= 0 && (args[2].type & SCRIPT_LIST)) {
		ScriptValue sv;
		int len = 0;
		if (args[2].type == SCRIPT_LIST) {
			len += args[2].listVal->numVals;
		}
		if (CreateListValue(sv, len)) {
			args[0].AddRef();
			if (args[2].type == SCRIPT_LIST) {
				for (int i=0; i<args[2].listVal->numVals; i++) {
					args[2].listVal->vals[i].AddRef();
					sv.listVal->PushBack(args[2].listVal->vals[i]);
				}
			}

			if (args[1].i32) {
				CreateIntValue(s, TriggerEventFull(entry->index, sv));
			}
			else if (args[1].i32) {
				FullEventParams *params = (FullEventParams*) malloc(sizeof(FullEventParams));
				if (params) {
					params->args = sv;
					params->id = entry->index;
					if (!PostMessage(ghWnd, WMU_TRIGGER_EVENT_FULL, 0, (LPARAM) params)) {
						free(params);
						sv.Release();
					}
					else
						CreateIntValue(s, 2);
				}
				else {
					sv.Release();
				}
			}
		}
	}
}

int clipboardEvent;
int counterUpdateEvent;
int screenSaverEvent;
int monitorPowerEvent;
int foregroundWindowChangeEvent;
int keyDownEvent;
int keyUpEvent;
int drawEvent;
int g15ButtonDownEvent;
int g15ButtonUpEvent;
int execEvent;
int mediaEvent;
int userEvent;
int audioChangeEvent;
int quitEvent;

int InitEvents() {
	struct InternalEventInfo {
		int *id;
		char *string;
	};
	static const InternalEventInfo internalEvents[] = {
		{&clipboardEvent, "ClipboardChange"},
		{&counterUpdateEvent, "CounterUpdate"},
		{&screenSaverEvent, "ScreenSaver"},
		{&monitorPowerEvent, "MonitorOff"},
		{&foregroundWindowChangeEvent, "ForegroundWindowChange"},

		{&keyDownEvent, "KeyDown"},
		{&keyUpEvent, "KeyUp"},
		{&drawEvent, "Draw"},

		{&g15ButtonDownEvent, "G15ButtonDown"},
		{&g15ButtonUpEvent, "G15ButtonUp"},
		{&execEvent, "Exec"},
		{&mediaEvent, "Media"},

		{&userEvent, "WMUser"},

		{&audioChangeEvent, "AudioChange"},
		{&quitEvent, "Quit"},
	};

	for (int i=0; i<sizeof(internalEvents)/sizeof(internalEvents[0]); i++) {
		if (-1 == (*internalEvents[i].id = GetEventId(internalEvents[i].string))) return 0;
	}
	return 1;
}
