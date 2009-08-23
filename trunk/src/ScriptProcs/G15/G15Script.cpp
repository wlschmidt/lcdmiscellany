#include "../../global.h"
#include "G15Script.h"
#include "../../G15Util/G15Hid.h"
#include "../../sdk/v3.01/lglcd.h"
#include "../../timer.h"

void G15SetBacklightColor(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (G15sSetBacklightColor(args[0].i32, args[1])));
}

void G15SetMLights(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (G15sSetMLights(args[0].i32, args[1])));
}

void G15SetLCDLight(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (G15sSetLCDLight(args[0].i32, args[1])));
}

void G15SetLight(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (G15sSetLight(args[0].i32, args[1])));
}

void G15SetContrast(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (G15sSetContrast(args[0].i32, args[1])));
}

void G15GetState(ScriptValue &s, ScriptValue *args) {
	ScriptValue sv4;
	G15State* state = GetG15State(args[0], sv4);
	if (state && CreateListValue(s, 4)) {
		ScriptValue sv;
		if (state->light > 2) state->light = 2;
		CreateIntValue(sv, state->light);
		s.listVal->PushBack(sv);
		if (state->LCDLight > 2) state->light = 2;
		CreateIntValue(sv, state->LCDLight);
		s.listVal->PushBack(sv);
		CreateIntValue(sv, (15&state->MLights)^15);
		s.listVal->PushBack(sv);
		sv4.AddRef();
		s.listVal->PushBack(sv4);
	}
}

extern int gButtons;

extern int connection;

void G15GetButtonsState(ScriptValue &s, ScriptValue *args) {
	int index = GetG15Index(args);
	if (index == 0 && numDevices) {
		unsigned int buttons=0;
		for (int i=0; i<numDevices; i++) {
			buttons |= devices[i]->gKeyState;
		}
		CreateIntValue(s, buttons);
	}
	else if (index>0) {
		CreateIntValue(s, devices[index-1]->gKeyState);
	}
}

void G15SetPriority(ScriptValue &s, ScriptValue *args) {
	int index = GetG15Index(&args[2]);
	int priority = 255 & args[0].i32;
	__int64 timer = args[1].intVal;
	if (timer > 0) {
		timer += time64i();
	}
	else {
		// 0 internally means forever.  Externally -1 means forever, so swap two cases.
		timer = -!timer;
	}
	int count = 0;
	for (int i=0; i<numDevices; i++) {
		if ((index == 0 || i == index-1) && (devices[i]->cType == CONNECTION_SDK301 || devices[i]->cType == CONNECTION_SDK103)) {
			count++;
			int oldPriority = devices[i]->priority;
			devices[i]->priority = priority;
			devices[i]->priorityTimer = timer;
			// If we aren't drawing now and don't have a redraw queued, just send the image again
			// with the updated priority.  Otherwise, will be sent with the updated priority soon, anyways.
			if (oldPriority != priority && !devices[i]->needRedraw && !devices[i]->sendImage) {
				devices[i]->UpdateImage();
			}
		}
	}
	CreateIntValue(s, count);
}

/*
void G15EnableHID(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, G15EnableHID(args[0].i32, args[1].i32));
}

void G15DisableHID(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, G15DisableHID());
}

void G15GetHIDs(ScriptValue &s, ScriptValue *args) {
}
//*/