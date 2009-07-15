#include "ScriptValue.h"
//#include <conio.h>
//#include <Pdh.h>
//#include <stdlib.h>
//#include <stdio.h>

#ifndef PERFMON_H
#define PERFMON_H
/*
struct Counter {
	HCOUNTER hCounter;
	//PDH_FMT_COUNTERVALUE val;
	int type;
	char *name;
};

struct CounterData {
	int size;
	int items;
	PDH_FMT_COUNTERVALUE_ITEM data[1];
};
//*/

struct Name {
	StringValue *s;
	unsigned int id;
};

struct Value {
	union {
		double doubleValue;
		__int64 intValue;
	};
	__int64 oldValue;
	__int64 oldDenominator;
	Value *next;
	int updated;
	int consumerData;
	char id[4];
};

struct Data {
	int size;
	int usedSize;
	int numValues;
	Value *lastValue;
	Value *firstValue;
};

struct Counter {
	Value *AddValue(char *id, const __int64 &value);
	Value *FindOldValue(char *id);
	void CleanOld();
	void FullClean();

	__int64 freqCounter;
	Name objectType;
	Name counter;

	int refCount;
	unsigned int id;
	StringValue *object;
	Data *data;
	//int read;
	unsigned char singleton;
	unsigned char normalUpdate;
	char objectTypeString[12];
};


class PerfMon {
	int numCounters;
	Counter *counters;
	void GetIDs();
	//__int64 lastUp;
	//__int64 lastDown;
	//__int64 Up;
	//__int64 Down;

	//int lastTime;
	int GetCounter(int id);

private:
	void UpdateNetwork();

	//Data * GetVals(int id, int format);
public:
	int UpdateSingle (int id);
	int GetCounterType(int id);
	int SetUpdate(int id, int update);
	Counter Up;
	Counter Down;
	double upTotal;
	double downTotal;
	//int updated;

	PerfMon();

	/*inline double GetSent() {
		return sent.val.doubleValue;
	}
	inline double GetRec() {
		return rec.val.doubleValue;
	}
	inline double GetCpu() {
		return cpu.val.doubleValue;
	}//*/
	double GetVal(unsigned int id, __int64 &counter);
	Value* GetVals(unsigned int id, __int64 &counter);
	Data* GetValsOnce(char *objectType, char*object, char *counter);
	/*
	int GetInt(int id);
	inline CounterData * GetIntVals(int id, CounterData *data) {
		return GetVals(id, PDH_FMT_LONG, data);
	}
	double GetDouble(int id);
	inline CounterData * GetDoubleVals(int id, CounterData *data) {
		return GetVals(id, PDH_FMT_DOUBLE, data);
	}*/
	void __fastcall Update(int bandwidth = 1);
	void Cleanup();
	int AddCounter(StringValue *objectType, StringValue *object, StringValue *counter, short singleton);
	int GetParams(int id, StringValue *&objectType, StringValue *&object, StringValue *&counter, short &singleton);
	void RemoveCounter(unsigned int id);

	inline ~PerfMon() {
		Cleanup();
	};
};

extern PerfMon perfMon;
#endif
