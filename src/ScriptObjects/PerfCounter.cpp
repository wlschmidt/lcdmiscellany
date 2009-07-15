#include "PerfCounter.h"
#include "..\\perfMon.h"
#include "..\\malloc.h"
#include "ScriptObjects.h"
unsigned int PerfCounterType;

static void MakeListFromValue(ScriptValue &s, Value *v) {
	CreateNullValue(s);
	if (!v) return;
	int count = 0;
	Value *t = v;
	while (t) {
		count ++;
		t = t->next;
	}
	if (CreateListValue(s, 2*count)) {
		ScriptValue sv, sv2;
		for (int i =0; i<count; i++) {
			CreateDoubleValue(sv2, v->doubleValue);
			//*
			if (!CreateStringValue(sv, (unsigned char*)v->id)|| !s.listVal->PushBack(sv) || !s.listVal->PushBack(sv2)) {
				s.Release();
				CreateNullValue(sv);
				return;
			}/*/
			free(malloc(2235));
			CreateStringValue(sv, (unsigned char*)v->id);
			free(malloc(2235));
			s.listVal->PushBack(sv);
			free(malloc(2235));
			s.listVal->PushBack(sv2);
			free(malloc(2235));
			//*/
			v = v->next;
		}
	}
}

static void MakeDictFromValue(ScriptValue &s, Value *v) {
	CreateNullValue(s);
	if (!v) {
		return;
	}
	int count = 0;
	Value *t = v;
	while (t) {
		count ++;
		t = t->next;
	}
	if (CreateDictValue(s, count)) {
		ScriptValue sv, sv2;
		for (int i =0; i<count; i++) {
			CreateDoubleValue(sv2, v->doubleValue);
			if (!CreateStringValue(sv, (unsigned char*)v->id) || !s.dictVal->Add(sv, sv2)) {
				s.Release();
				CreateNullValue(s);
				return;
			}
			v = v->next;
		}
	}
}

void PerformanceCounter(ScriptValue &s, ScriptValue *args) {
	CreateNullValue(s);
	int id = perfMon.AddCounter(args[0].stringVal, args[1].stringVal, args[2].stringVal, args[3].i32);
	if (id >= 0) {
		if (CreateObjectValue(s, PerfCounterType)) {
			CreateIntValue(s.objectVal->values[0], id);
			CreateIntValue(s.objectVal->values[1], -1);
		}
		else {
			perfMon.RemoveCounter(id);
		}
	}
}

void PerformanceCounterGetParameters(ScriptValue &s, ScriptValue *args) {
	StringValue *sv[3];
	short singleton;
	int autoUpdate = perfMon.GetParams(s.objectVal->values[0].i32, sv[0], sv[1], sv[2], singleton);
	if (autoUpdate >= 0 && CreateListValue(s, 4)) {
		ScriptValue v;
		for (int i=0; i<3; i++) {
			sv[i]->AddRef();
			CreateStringValue(v, sv[i]);
			s.listVal->PushBack(v);
		}
		CreateIntValue(v, singleton);
		s.listVal->PushBack(v);
	}
	else {
		CreateNullValue(s);
	}
}

void PerformanceCounterIsUpdated(ScriptValue &s, ScriptValue *args) {
	__int64 i;
	perfMon.GetVal(s.objectVal->values[0].i32, i);
	CreateIntValue(s, s.objectVal->values[1].intVal!=i);
}

void PerformanceCounterSetAutoUpdate(ScriptValue &s, ScriptValue *args) {
	int r = perfMon.SetUpdate(s.objectVal->values[0].i32, args[0].i32);
	if (r >= 0) {
		CreateIntValue(s, r);
	}
	else
		CreateNullValue(s);
}

void PerformanceCounterUpdate(ScriptValue &s, ScriptValue *args) {
	int r = perfMon.UpdateSingle(s.objectVal->values[0].i32);
	if (r>=0) {
		CreateIntValue(s, 1);
	}
	else
		CreateNullValue(s);
}

void PerformanceCounterGetValueList(ScriptValue &s, ScriptValue *args) {
	int type = perfMon.GetCounterType(s.objectVal->values[0].i32);
	int counter = s.objectVal->values[0].i32;
	if (type < 0) {
		CreateNullValue(s);
	}
	else if (type == 0) {
		CreateDoubleValue(s, perfMon.GetVal(s.objectVal->values[0].i32, s.objectVal->values[1].intVal));
	}
	else {
		MakeListFromValue(s, perfMon.GetVals(s.objectVal->values[0].i32, s.objectVal->values[1].intVal));
	}
}

void PerformanceCounterGetValue(ScriptValue &s, ScriptValue *args) {
	int type = perfMon.GetCounterType(s.objectVal->values[0].i32);
	if (type < 0) {
		CreateNullValue(s);
	}
	else if (type == 0) {
		CreateDoubleValue(s, perfMon.GetVal(s.objectVal->values[0].i32, s.objectVal->values[1].intVal));
	}
	else {
		MakeDictFromValue(s, perfMon.GetVals(s.objectVal->values[0].i32, s.objectVal->values[1].intVal));
	}
}

void PerformanceCounterFree(ScriptValue &s, ScriptValue *args) {
	perfMon.RemoveCounter(s.objectVal->values[0].i32);
}

/*
int numScriptCounters = 0;
struct ScriptCounter {
	int id;
	int type;
};
ScriptCounter *scriptCounters = 0;
/*
void MakeListFromValueNames(ScriptValue &s, Value *v) {
	if (!v) return;
	int count = 0;
	Value *t = v;
	while (t) {
		count ++;
		t = t->next;
	}
	if (CreateListValue(s, count)) {
		ScriptValue sv;
		for (int i =0; i<count; i++) {
			if (!CreateStringValue(sv, (unsigned char*)v->id) || !s.listVal->PushBack(sv)) {
				s.Release();
				CreateNullValue(s);
				return;
			}
			v = v->next;
		}
	}
}//*/
/*
void MakeListFromValue(ScriptValue &s, Value *v) {
	if (!v) return;
	int count = 0;
	Value *t = v;
	while (t) {
		count ++;
		t = t->next;
	}
	if (CreateListValue(s, 2*count)) {
		ScriptValue sv, sv2;
		for (int i =0; i<count; i++) {
			CreateDoubleValue(sv2, v->doubleValue);
			if (!CreateStringValue(sv, (unsigned char*)v->id)|| !s.listVal->PushBack(sv) || !s.listVal->PushBack(sv2)) {
				s.Release();
				CreateNullValue(sv);
				return;
			}
			v = v->next;
		}
	}
}

void StartCounterListen(ScriptValue &s, ScriptValue *args) {
	unsigned char *c = args[0].stringVal->value;
	unsigned char *c2 = args[1].stringVal->value;
	unsigned char *c3 = args[2].stringVal->value;
	if (!c[0] || !c3[0]) return;
	int i;
	if (!c || !c2 || !c3) return;
	for (i = 0; i<numScriptCounters; i++) {
		if (scriptCounters[i].id == -1) break;
	}
	if (i == numScriptCounters) {
		if (!srealloc(&scriptCounters, sizeof(ScriptCounter) * (numScriptCounters+1))) {
			CreateIntValue(s, -1);
			return;
		}
		numScriptCounters++;
	}
	scriptCounters[i].id = perfMon.AddCounter((char*)c, (char*)c2, (char*)c3);
	scriptCounters[i].type = (c2[0]=='*' && c2[1] == 0);
	if (scriptCounters[i].id == -1) {
		return;
	}
	CreateIntValue(s, i);
	if (i==numScriptCounters) numScriptCounters ++;
}

void StopCounterListen(ScriptValue &s, ScriptValue *args) {
	int id = args->i32;
	if (scriptCounters[id].id == -1) {
		CreateIntValue(s, 0);
		return;
	}
	perfMon.RemoveCounter(scriptCounters[id].id);
	scriptCounters[id].id = -1;
	CreateIntValue(s, 1);
	return;
}

void GetCounterValue(ScriptValue &s, ScriptValue *args) {
	int id = args->i32;
	if (id < 0 || id >= numScriptCounters || scriptCounters[id].id == -1) {
	}
	else if (scriptCounters[id].type == 0) {
		CreateDoubleValue(s, perfMon.GetVal(scriptCounters[id].id));
	}
	else {
		MakeDictFromValue(s, perfMon.GetVals(scriptCounters[id].id));
	}
}

void GetCounterValueList(ScriptValue &s, ScriptValue *args) {
	int id = args->i32;
	if (id < 0 || id >= numScriptCounters || scriptCounters[id].id == -1) {
	}
	else if (scriptCounters[id].type == 0) {
		CreateDoubleValue(s, perfMon.GetVal(scriptCounters[id].id));
	}
	else {
		MakeListFromValue(s, perfMon.GetVals(scriptCounters[id].id));
	}
}
/*
void GetCounterNameList(ScriptValue &s, ScriptValue *args) {
	int id = args->i32;
	if (id < 0 || id >= numScriptCounters || scriptCounters[id].id == -1) {
	}
	else if (scriptCounters[id].type == 0) {
		CreateDoubleValue(s, perfMon.GetVal(scriptCounters[id].id));
	}
	else {
		MakeListFromValueNames(s, perfMon.GetVals(scriptCounters[id].id));
	}
}//*/
/*

void CleanupCounters() {
	ScriptValue sv;
	for (int i =0; i<numScriptCounters; i++) {
		ScriptValue s;
		CreateDoubleValue(sv, i);
		StopCounterListen(s, &sv);
	}
	free(scriptCounters);
	scriptCounters = 0;
	numScriptCounters = 0;
}
//*/


void GetAllDown(ScriptValue &s, ScriptValue *args) {
	if (perfMon.Up.data)
		MakeDictFromValue(s, perfMon.Down.data->firstValue);
}

void GetAllUp(ScriptValue &s, ScriptValue *args) {
	if (perfMon.Down.data)
		MakeDictFromValue(s, perfMon.Up.data->firstValue);
}

void GetUpstream(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, perfMon.upTotal);
}

void GetDownstream(ScriptValue &s, ScriptValue *args) {
	CreateDoubleValue(s, perfMon.downTotal);
}

void UpdateCounter(ScriptValue &s, ScriptValue *args) {
	perfMon.Update();
}
