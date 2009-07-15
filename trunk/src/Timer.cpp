#include "http/httpGetPage.h"
#include "Timer.h"
#include <time.h>
#include "Config.h"
#include "global.h"
#include "malloc.h"

__int64 Timer::lastClock = time64i();
Timer **Timer::timers = 0;
int Timer::numTimers = 0;

#if (_MSC_VER<1300)
__int64 _time64(__int64 *t) {
	if (t) *t = 0;
	return time((long*)t);
}
tm * _localtime64(const __int64 * _Time) {
	return (localtime((long*)_Time));
}
#endif

//__int64 time64i() {
	/*
	__int64 out;
	GetSystemTimeAsFileTime((FILETIME *)&out);
	__int64 q = time(0);
	return out / (1000*1000*10);
	//*/
//	return time(0);
//}
/*
double time64( ) {
	__int64 out;
	GetSystemTimeAsFileTime((FILETIME *)&out);
	return out / (double) (1000*1000*10);
}
//*/

void Timer::ReInit(void *data, __int64 delay, TimerMode timerMode, SaveMode saveMode) {
	Stop();
	this->data = data;
	this->delay = delay;
	this->saveMode = saveMode;
	if (id && (saveMode == SAVE_BOTH || saveMode == SAVE_DELAY)) {
		config.SaveConfigInt64((unsigned char*)"Timer Delay", id, delay);
		this->delay = config.GetConfigInt64((unsigned char*)"Timer Delay", id, delay);
		if (this->delay <= 1) this->delay = 1;
	}
	//this->delay *= 60;
	if (timerMode != RUN_NEVER) SetMode(timerMode);
}

void Timer::Init(TimedProc *proc, void *data, __int64 delay, TimerMode timerMode, unsigned char *id, SaveMode saveMode) {
	this->proc = proc;
	this->data = data;
	this->delay = delay;
	this->saveMode = saveMode;
	this->timerMode = RUN_NEVER;
	this->id = id;
	index = -1;
	if (id && (saveMode == SAVE_BOTH || saveMode == SAVE_DELAY)) {
		config.SaveConfigInt64((unsigned char*)"Timer Delay", id, delay);
		this->delay = config.GetConfigInt64((unsigned char*)"Timer Delay", id, delay);
		if (this->delay <= 1) this->delay = 1;
	}
	lastRun = lastClock;
	if (!id || (saveMode != SAVE_BOTH && saveMode != SAVE_LAST_RUN)) {
		lastRun -= this->delay;
	}
	else {
		lastRun = config.GetConfigInt64((unsigned char*)"Timer Last Run", id, lastRun - this->delay);
		/*if (lastRun > lastClock - delay || lastRun < lastClock) {
			lastRun = lastClock - delay;
		}
		// Delay is randomly changed by 0 to 4 minutes, to prevent rare jobs from all
		// running at once.  Doesn't work right when time overflows...Which isn't too
		// common, with 64-bit times.
		//*/
		//lastRun +=  (int) ((rand()/(float) RAND_MAX)*240);
		//*/
	}
	if (timerMode != RUN_NEVER) SetMode(timerMode);
}

void Timer::SetMode(TimerMode mode) {
	if (mode == RUN_NEVER) {
		Stop();
		return;
	}
	else if (index == -1) {
		// if can't allocate, fail.  index == -1 and mode == RUN_NEVER are the only
		// indicators this happened.
		if (!srealloc(timers, sizeof(Timer*) * (numTimers+1))) return;
		index = numTimers;
		timers[numTimers++] = this;
		ChangeDelay(delay);
	}
	timerMode = mode;
}

void Timer::Stop() {
	if (index  < 0) return;
	for (int i=index+1; i<numTimers; i++) {
		timers[i-1] = timers[i];
		timers[i-1]->index = i-1;
	}
	index = -1;
	timerMode = RUN_NEVER;
	srealloc(timers, sizeof(Timer*) * (--numTimers));
}

void Timer::ChangeDelay(__int64 newDelay) {
	if (newDelay <= 0) newDelay = 1;
	// Reuse timers for related operations (like cleanup), so bad idea.
	//if (delay != newDelay && (saveMode == SAVE_BOTH || saveMode == SAVE_DELAY)) {
	//	config.SaveConfigInt64((unsigned char*)"Timer Delay", id, newDelay/60);
	//}
	delay = newDelay;
	if (index < 0) return;
	while (index && lastRun + delay < timers[index-1]->lastRun + timers[index-1]->delay) {
		timers[index] = timers[index-1];
		timers[index]->index = index;
		timers[--index] = this;
		//index--;
	}
	while (index<numTimers-1 && lastRun + delay > timers[index+1]->lastRun + timers[index+1]->delay) {
		timers[index] = timers[index+1];
		timers[index]->index = index;
		timers[++index] = this;
	}
}

void Timer::Update() {
	__int64 newTime = time64i();
	__int64 timeFix = 0;
	if (newTime <= lastClock) {
		if (newTime == lastClock) return;
		timeFix = newTime - lastClock - 1;
	}
	else if (newTime > lastClock + 240) {
		timeFix = newTime - lastClock - 1;
	}
	if (timeFix) {
		for (int i=0; i<numTimers; i++) {
			timers[i]->lastRun += timeFix;
		}
	}
	lastClock = newTime;
	if (!timers) return;
	//CheckIdleHttpSockets(newTime, timeFix);
	while (lastClock > timers[0]->lastRun + timers[0]->delay) {
		Timer *t = timers[0];
		t->lastRun = lastClock;
		if (t->timerMode != RUN_ALWAYS) {
			t->Stop();
		}
		else {
			t->ChangeDelay(t->delay);
		}
		t->proc(t->data);
		if (!timers) return;
		//if (t->saveMode == SAVE_LAST_RUN || t->saveMode == SAVE_BOTH) {
		//}
	}
}

void Timer::SaveTime() {
	if (id)
		config.SaveConfigInt64((unsigned char*)"Timer Last Run", id, lastClock, 1);
}
