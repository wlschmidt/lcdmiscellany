#ifndef TIMER_H
#define TIMER_H

enum TimerMode {RUN_NEVER=0, RUN_ONCE, RUN_ALWAYS};
enum SaveMode {SAVE_NONE = 0, SAVE_LAST_RUN=1, SAVE_DELAY = 2, SAVE_BOTH = 3};


typedef void TimedProc (void *);

#include <time.h>



#if (_MSC_VER<1300)
	__int64 _time64(__int64 *t);
	tm * _localtime64(const __int64 * _Time);
#endif
//*/

//double time64( );
inline __int64 time64i() {
	return _time64(0);
}


//struct tm *_cdecl _localtime64(const __int64 *t);

struct Timer {
	static __int64 lastClock;
	static Timer **timers;
	static int numTimers;
public:
	void ReInit(void *data, __int64 delay, TimerMode timerMode, SaveMode saveMode);
	__int64 lastRun;
	__int64 delay;
	TimedProc *proc;
	void *data;
	// Timers with a non-null id save the time they were last run.
	unsigned char* id;
	// Time last run
	int index;
	TimerMode timerMode;
	SaveMode saveMode;
	// Note: Delay is in minutes.
	void Init(TimedProc *proc, void *data, __int64 delay, TimerMode timerMode, unsigned char *id, SaveMode saveMode);
	void ChangeDelay(__int64 newDelay);
	void SetMode(TimerMode mode);
	inline void Cleanup() {
		Stop();
	}
	void Stop();
	static void Update();
	// queued op completed successfully.  Save current time as last run time.
	void SaveTime();
};
#endif
