#include "ScriptTime.h"
#include "../../global.h"
#include "../../vm.h"
#include "../../Timer.h"
#include "../../malloc.h"
#include "../../ScriptObjects/ScriptObjects.h"

enum TimerType {
	FAST_TIMER,
	SLOW_TIMER,
	KILLED_TIMER
};

struct FastTimer {
	int id;
	int delay;
	int active;
};

struct ScriptTimer {
	TimerType type:2;
	int function;
	ObjectValue *obj;
	union {
		Timer *slowTimer;
		FastTimer fastTimer;
	};
	void Stop();
	int Start(int forceRestart);
	void Kill();
};

ScriptTimer * timers = 0;
int numTimers = 0;

void RunTimerFunction(int fxn, ObjectValue *obj, int index) {
	if (obj) obj->AddRef();
	ScriptValue sv, sv2;
	if (CreateListValue(sv, 1)) {
		CreateIntValue(sv2, index);
		sv.listVal->PushBack(sv2);
		RunFunction(fxn, sv, CAN_WAIT, obj);
	}
}

void CALLBACK FastTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	int index = (int)(idEvent - 10);
	if (index < 0 || index>=numTimers || timers[index].type != FAST_TIMER) {
		// ??
		KillTimer(hWnd, idEvent);
		return;
	}
	if (timers[index].fastTimer.active) {
		RunTimerFunction(timers[index].function, timers[index].obj, index+1);
	}
}

void SlowTimerProc(void *data) {
	int index = (int)(size_t) data;
	if (index < 0 || index>=numTimers || timers[index].type != SLOW_TIMER) {
		// ??
		return;
	}
	RunTimerFunction(timers[index].function, timers[index].obj, index+1);
	//RunFunction(timers[index].function, index+1, CAN_WAIT);
}

void ScriptTimer::Stop() {
	if (type == SLOW_TIMER) {
		slowTimer->Stop();
	}
	else if (type == FAST_TIMER) {
		//if (fastTimer.active) {
		fastTimer.active = 0;
		KillTimer(ghWnd, fastTimer.id);
		//}
	}
}

void ScriptTimer::Kill() {
	if (type != KILLED_TIMER) {
		Stop();
		if (type == SLOW_TIMER) free(slowTimer);
		type = KILLED_TIMER;
		if (obj) {
			obj->Release();
			obj = 0;
		}
	}
}

int ScriptTimer::Start(int forceRestart) {
	if (type == SLOW_TIMER) {
		if (forceRestart)
			slowTimer->Stop();
		slowTimer->SetMode(RUN_ALWAYS);
		return (slowTimer->timerMode == RUN_ALWAYS);
	}
	else if (type == FAST_TIMER) {
		if (fastTimer.active == 1) KillTimer(ghWnd, fastTimer.id);
		if (SetTimer(ghWnd, fastTimer.id, fastTimer.delay, FastTimerProc)) {
			fastTimer.active = 1;
			return 1;
		}
	}
	return 0;
}


inline int FindTimerSpace() {
	for (int i = 0; i<numTimers; i++) {
		if (timers[i].type == KILLED_TIMER) return i;
	}
	if (!srealloc(timers, sizeof(ScriptTimer) * (numTimers+1))) return -1;
	return numTimers ++;
}

int GetFunction(ScriptValue *s, ScriptValue *o, ObjectValue *&obj) {
	int function;
	if (o->type != SCRIPT_OBJECT) {
		function = FindFunction(s->stringVal->value);
		obj = 0;
	}
	else {
		obj = o->objectVal;
		function = types[obj->type].FindFunction(s->stringVal);
		if (function < 0) return function;
		function = types[obj->type].functs[function].functionID;
		obj->AddRef();
	}
	return function;
}

void CreateFastTimer(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal < 10 || args[1].intVal > 3600000) {
		return;
	}
	ObjectValue *obj;
	int function = GetFunction(&args[0], &args[3], obj);
	if (function < 0) return;

	int index = FindTimerSpace();
	if (index == -1) {
		if (obj) obj->Release();
		return;
	}
	timers[index].type = FAST_TIMER;
	timers[index].obj = obj;
	timers[index].function = function;
	timers[index].fastTimer.active = 0;
	timers[index].fastTimer.id = index+10;
	timers[index].fastTimer.delay = args[1].i32;
	if (!timers[index].Start(1)) {
		timers[index].Kill();
	}
	else {
		CreateIntValue(s, index+1);
		if (args[2].intVal)
			RunTimerFunction(timers[index].function, timers[index].obj, index+1);
	}
}

void CreateTimer(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal < 1 || args[1].intVal > (1<<30)) {
		return;
	}
	ObjectValue *obj;
	int function = GetFunction(&args[0], &args[3], obj);
	if (function < 0) return;
	int index = FindTimerSpace();
	if (index == -1) {
		if (obj) obj->Release();
		return;
	}
	if (!(timers[index].slowTimer = (Timer*) malloc(sizeof(Timer)))) {
		if (obj) obj->Release();
		timers[index].type = KILLED_TIMER;
		return;
	}
	timers[index].obj = obj;
	timers[index].type = SLOW_TIMER;
	timers[index].function = function;
	timers[index].slowTimer->Init(SlowTimerProc, (void*)(size_t)index, args[1].intVal, RUN_ALWAYS, 0, SAVE_NONE);
	if (timers[index].slowTimer->timerMode != RUN_ALWAYS) {
		timers[index].Kill();
	}
	else {
		CreateIntValue(s, index+1);
		if (args[2].intVal == 0) {
			timers[index].slowTimer->lastRun = time64i();
			timers[index].slowTimer->ChangeDelay(timers[index].slowTimer->delay);
		}
	}
}

void ModifyFastTimer(ScriptValue &s, ScriptValue *args) {
	if (args->intVal == 0) {
		if (args[1].i32 >= 20 && SetTimer(ghWnd, 0, args[1].i32/2, 0)) {
			CreateIntValue(s, 0);
		}
	}
	else if (args->intVal <= 0 || args->intVal > numTimers || args[1].intVal < 10 || args[1].intVal > 3600000) {
	}
	else {
		int index = (int)(args->intVal-1);
		if (timers[index].type != FAST_TIMER) {
		}
		else {
			timers[index].fastTimer.delay = args[1].i32;
			if (timers[index].Start(0))
				CreateIntValue(s, 0);
		}
	}
}

void ModifyTimer(ScriptValue &s, ScriptValue *args) {
	if (args->intVal <= 0 || args->intVal > numTimers || args[1].intVal < 1 || args[1].intVal > (1<<30)) {
	}
	else {
		int index = (int)(args->intVal-1);
		if (timers[index].type != SLOW_TIMER) {
		}
		else {
			timers[index].slowTimer->ChangeDelay(args[1].intVal);
			if (timers[index].Start(0))
				CreateIntValue(s, 0);
		}
	}
}

void StartTimer(ScriptValue &s, ScriptValue *args) {
	if (args->intVal <= 0 || args->intVal > numTimers) {
	}
	else {
		int index = (int)(args->intVal-1);
		if (timers[index].type == KILLED_TIMER) {
		}
		else {
			if (!timers[index].Start(1)) {
			}
			else {
				CreateIntValue(s, 0);
				if (timers[index].type == SLOW_TIMER) {
					timers[index].slowTimer->lastRun = time64i();
					if (args[1].intVal != 0) {
						timers[index].slowTimer->lastRun -= timers[index].slowTimer->delay;
					}
					timers[index].slowTimer->ChangeDelay(timers[index].slowTimer->delay);
				}
				else {
					if (args[1].intVal != 0) {
						RunFunction(timers[index].function, index + 1, CAN_WAIT);
					}
				}
			}
		}
	}
}

void StopTimer(ScriptValue &s, ScriptValue *args) {
	if (args->intVal <= 0 || args->intVal > numTimers) {
	}
	else {
		int index = (int)(args->intVal-1);
		if (timers[index].type == KILLED_TIMER) {
		}
		else {
			timers[index].Stop();
			CreateIntValue(s, 0);
		}
	}
}

void KillTimer(ScriptValue &s, ScriptValue *args) {
	if (args->intVal <= 0 || args->intVal > numTimers) {
	}
	else {
		int index = (int)(args->intVal-1);
		if (timers[index].type == KILLED_TIMER) {
		}
		else {
			timers[index].Kill();
			CreateIntValue(s, 0);
		}
	}
}

void CleanupTimers() {
	for (int i=0; i<numTimers; i++) {
		timers[i].Kill();
	}
	numTimers = 0;
	free(timers);
	timers = 0;
}

