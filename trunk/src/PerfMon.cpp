#include "PerfMon.h"
#include "global.h"
//#include <Pdh.h>
//#include <stdlib.h>
//#include "stringUtil.h"
#include <Iphlpapi.h>
#include <float.h>
//#include "Unicode.h"
#include "malloc.h"
#include "Unicode.h"
#include "Config.h"

typedef unsigned long __stdcall NtQuerySystemInformationCall(int infoClass, void *sysIno, ULONG infoSize, PULONG length);

NtQuerySystemInformationCall *NtQuerySystemInformation = 0;

PerfMon::PerfMon() {
	memset(this, 0, sizeof(*this));
	HMODULE h = GetModuleHandleA("Ntdll.dll");
	if (h) {
		NtQuerySystemInformation = (NtQuerySystemInformationCall*) GetProcAddress(h, "NtQuerySystemInformation");
	}
};

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

__forceinline void Counter::CleanOld() {
	if (data) {
		int i = sizeof(Data);
		int j = i;
		data->lastValue = 0;
		Value *dest = data->firstValue;
		Value *src = data->firstValue;
		int newUsed = sizeof(Data);
		while (src) {
			if (src->updated) {
				data->lastValue = dest;
				if (dest==src) {
					if (src->next) {
						newUsed += (int) ((char*)dest->next - (char*)dest);
					}
					else {
						newUsed = data->usedSize;
					}
					dest->updated = 0;
					dest = src->next;
					src = src->next;
				}
				else {
					int len;
					if (src->next) {
						len = (int) ((char*)src->next - (char*)src);
					}
					else {
						len = (int) (data->usedSize - ((char*)src-(char*)data));
					}
					newUsed += len;
					Value *temp = src;
					src = src->next;
					memcpy(dest, temp, len);
					dest->updated = 0;
					dest->next = (Value*)&((char*)dest)[len];
					dest = dest->next;
					//*/
				}
			}
			else {
				src = src->next;
			}
		}
		data->usedSize = newUsed;
		if (!data->lastValue)
			data->firstValue = 0;
		else
			data->lastValue->next = 0;
		int q=0;
	}
}

Value *Counter::FindOldValue(char *id) {
	if (data) {
		Value *v = data->firstValue;
		while (v) {
			if (!v->updated &&
				strcmp(v->id, id) == 0) {
				return v;
			}
			v = v->next;
		}
	}
	return 0;
}

Value *Counter::AddValue(char *id, const __int64 &value) {
	int w = (int)((strlen(id) +4)&~3);
	int w2 = sizeof(Value)-4;
	int addedSize = (int)((strlen(id))&~3) + sizeof(Value);
	// If we have way too much space, trigger a reallocation.
	if (data && data->size > 2*(addedSize+data->usedSize)) {
		data->size = data->usedSize;
	}
	if (!data || data->usedSize + addedSize > data->size) {
		int newSize;
		if (!data) {
			newSize = addedSize + sizeof(Data);
			addedSize = newSize;
		}
		else {
			newSize = data->size;
			do {
				newSize += newSize>>1;
			}
			while (newSize < data->size + addedSize);
		}
		char *newData = (char*) realloc(data, newSize);
		if (!newData) return 0;
		if (data) {
			data = (Data*) newData;
			data->size = newSize;
			int i = sizeof(Data);
			Value ** v = &data->firstValue;
			while (i < data->usedSize) {
				data->lastValue = (Value*) &((char*)data)[i];
				*v = (Value*) &((char*)data)[i];
				(*v)->next = 0;
				int j = 0;
				while ((*v)->id[j]) j++;
				i += (j&~3)+sizeof(Value);
				v = &(*v)->next;
			}
			*v = 0;
		}
		else {
			data = (Data*) newData;
			data->size = newSize;
			data->usedSize = sizeof(Data);
			data->lastValue = 0;
			//data->firstValue = 0;
			addedSize -= sizeof(Data);
		}
	}
	Value *v = (Value*)&(((char*)data)[data->usedSize]);
	if (data->lastValue)
		data->lastValue->next = v;
	else {
		data->firstValue = v;
	}
	data->lastValue = v;
	v->next = 0;
	v->consumerData = -1;
	//int endpt = data->size - (data->usedSize - (sizeof(Data) - sizeof(Value)) - data->numValues*data->numValues);
	//Value *v = &data->values[data->numValues];
	v->doubleValue = 0;
	v->oldValue = value;
	//v->updated = 1;
	int i = 0;
	/*
	while (id[i]) {
		v->id[i] = id[i];
		i++;
	}//*/
	strcpy(v->id, id);
	//endpt -= 1 + strlen(id);
	//v->id = &((char*)data)[endpt];
	//strcpy(v->id, id);
	v->updated = 1;
	
	data->usedSize += addedSize;
	//data->numValues++;
	return v;
}

int PerfMon::UpdateSingle (int id) {
	int i = GetCounter(id);
	if (i < 0 || !counters[i].singleton) return i;
	int temp = numCounters;
	unsigned char update = counters[i].normalUpdate;
	counters[i].normalUpdate = 1;
	Counter temp2 = counters[0];
	counters[0] = counters[i];
	counters[i] = temp2;
	numCounters = 1;
	Update(0);
	numCounters = temp;
	counters[i].normalUpdate = update;

	return i;
}
/*
Data *PerfMon::GetValsOnce (char *objectType, char*object, char *counter) {
	int temp = numCounters;
	counters[numCounters] = counters[0];
	numCounters = 0;
	int id = AddCounter(objectType, object, counter);
	Data *out = 0;
	if (id>=0) {
		Update(0);
		out = counters[0].data;
		counters[0].data = 0;
		RemoveCounter(id);
	}
	numCounters = temp;
	counters[0] = counters[numCounters];

	return out;
}
/*
int PerfMon::GetInt(int id) {
	PDH_FMT_COUNTERVALUE val;
	PdhGetFormattedCounterValue(counters[id].hCounter, PDH_FMT_LONG, 0, &val);
	return val.longValue;
}

double PerfMon::GetDouble(int id) {
	PDH_FMT_COUNTERVALUE val;
	PdhGetFormattedCounterValue(counters[id].hCounter, PDH_FMT_DOUBLE, 0, &val);
	return val.doubleValue;
}

CounterData *PerfMon::GetVals(int id, int type, CounterData *data) {
	unsigned long s = 0;
	if (data) s = data->size;
	unsigned long c;

	int w = PdhGetFormattedCounterArray(counters[id].hCounter, type, &s, &c, data->data);
	if (w == ERROR_SUCCESS) {
		if (data) {
			data->items = c;
		}
		return data;
	}
	else if (w != PDH_MORE_DATA) {
		free(data);
		return 0;
	}

	data = (CounterData*) realloc(data, s + 2*sizeof(int));
	unsigned long s2 = s;

	if (!data || 
		PdhGetFormattedCounterArray(counters[id].hCounter, type, &s2, &c, data->data) != ERROR_SUCCESS) {

		free(data);
		return 0;
	}
	data->size = s;
	data->items = c;
	return data;
}
*/

int PerfMon::AddCounter(StringValue *objectType, StringValue *object, StringValue *counter, short singleton) {
	int i = 0;
	for (; i<numCounters; i++) {
		if (!counters[i].singleton &&
			scriptstricmp(objectType, counters[i].objectType.s) == 0 &&
			scriptstricmp(counter, counters[i].counter.s) == 0 &&
			scriptstricmp(object, counters[i].object) == 0) {
			break;
		}
	}
	if (i<numCounters) {
		counters[i].refCount++;
		return counters[i].id;
	}
	int id = 0;
	if (numCounters % 8 == 0) {
		if (!srealloc(counters, sizeof(Counter)*(numCounters+9))) return -1;
	}
	while (GetCounter(id)>=0) id++;
	counters[i].id = id;
	counters[i].refCount = 1;
	objectType->AddRef();
	counters[i].objectType.s = objectType;
	counters[i].objectType.id = -1;
	object->AddRef();
	counters[i].object = object;
	counter->AddRef();
	counters[i].counter.s = counter;
	counters[i].counter.id = -1;
	counters[i].data = 0;
	counters[i].singleton = (singleton != 0);
	counters[i].normalUpdate = 1;
	numCounters++;
	return id;
}

char *GetString(char *s, char *s2) {
	while (s[0]) {
		char *s3 = s2;
		while (*s == *s3) {
			if (!*s) {
				while (s[-1]) s--;
				s--;
				while (s[-1]) s--;
				return s;
			}
			s3++;
			s++;
		}
		while (*s) s++;
		s++;
	}
	return 0;
}

void PerfMon::GetIDs() {
	//*
	int i;
	unsigned long type;
	char *str;
	/*
	for (i=0; i<numCounters; i++) {
		if (counters[i].objectType.id < 0) {
			if (!s.data) {
				HKEY h;
				if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009", &h)) {
					if (ERROR_SUCCESS == RegQueryValueEx(h, "Counters", 0, &type, (unsigned char*)s.data, &s.size)) {
						s.data = (char*) malloc(s.size);
						if (s.data &&
							ERROR_SUCCESS != RegQueryValueEx(h, "Counters", 0, &type, (unsigned char*)s.data, &s.size) ||
							type != REG_MULTI_SZ) {
								s.Clear();
						}
					}
					RegCloseKey(h);
				}
			}
		}

		if (s.data && (str = s.GetString(counters[i].objectType.s))) {
			counters[i].objectType.id = ParseInt(str);
		}
	}//*/
	int loadedData = 0;

	unsigned long dataSize = 500;
	char *data = (char*) malloc(dataSize);

	for (i=0; i<numCounters; i++) {
		if ((int)counters[i].objectType.id < 0 || (int)counters[i].counter.id < 0) {
			if (!loadedData) {
				DWORD size = 0;
				int w = RegQueryValueExA(HKEY_PERFORMANCE_TEXT, "Counter", 0, &type, 0, &size);
				if (w != ERROR_SUCCESS || type != REG_MULTI_SZ) continue;
				if (dataSize < size) {
					void *temp = realloc(data, size + 100);
					if (!temp) continue;
					dataSize = size + 100;
					data = (char*) temp;
				}
				w = RegQueryValueExA(HKEY_PERFORMANCE_TEXT, "Counter", 0, &type, (BYTE*)data, &size);
				if (w != ERROR_SUCCESS) continue;
				data[size] = 0;
				loadedData = 1;
			}

			char *junk;
			if ((int)counters[i].objectType.id < 0  && (str = GetString(data, (char*)counters[i].objectType.s->value))) {
				int w = (int)strlen(str);
				if (w <= sizeof(counters[i].objectTypeString)) {
					strcpy(counters[i].objectTypeString, str);
					counters[i].objectType.id = strtoul(str, &junk, 0);
				}
			}
			if ((int)counters[i].counter.id < 0  && (str = GetString(data, (char*)counters[i].counter.s->value))) {
				counters[i].counter.id = strtoul(str, &junk, 0);
			}
		}
	}
}

char *total = "_Total";

inline void PerfMon::UpdateNetwork() {
	int temp = GetTickCount();
	double div = (temp-(int)Down.freqCounter)/(double)1000;
	static unsigned long ifsize = 0;
	MIB_IFTABLE *ifTable = (MIB_IFTABLE*) malloc(2*ifsize);
	Down.freqCounter = temp;
	upTotal = downTotal = 0;
	if (ifTable || !ifsize) {
		HRESULT hr = GetIfTable(ifTable, &ifsize, 0);
		while (hr == ERROR_INSUFFICIENT_BUFFER && srealloc(ifTable, 2*ifsize)) {
			hr = GetIfTable(ifTable, &ifsize, 0);
		}
		double *vals = (double*)(((char*)ifTable) + ((ifsize+8)&~7));
		if (hr == S_OK) {
			static bool listed = 0;
			if (!listed) {
				errorPrintf(0, "\r\nNetwork devices:\r\n");
			}
			for (unsigned int i=0; i<ifTable->dwNumEntries; i++) {
				/*
					||
					ifTable->table[i].dwType == IF_TYPE_IEEE1394 || 
					ifTable->table[i].dwType == IF_TYPE_TUNNEL) continue;
					//*/
				Value *v = 0, *v2 = 0;
				char *name = (char*)ifTable->table[i].bDescr;
				if (ifTable->table[i].dwDescrLen >= MAXLEN_IFDESCR)
					ifTable->table[i].dwDescrLen = MAXLEN_IFDESCR-1;
				else if (ifTable->table[i].dwDescrLen < 0) {
					ifTable->table[i].dwDescrLen = 0;
				}
				ifTable->table[i].bDescr[ifTable->table[i].dwDescrLen] = 0;
				/*if (ifTable->table[i].dwType != IF_TYPE_ETHERNET_CSMACD) {
					if (!listed) {
						errorPrintf(0, "\t%s (Type %i, Removed from list)\r\n", name, ifTable->table[i].dwType);
					}
					continue;
				}
				if (ifTable->table[i].dwDescrLen > 19) {
					if (!stricmp(&name[ifTable->table[i].dwDescrLen-19], "cFosSpeed Miniport")) {
						if (!listed) {
							errorPrintf(0, "\t%s (Removed from list)\r\n", name);
						}
						continue;
					}
					if (ifTable->table[i].dwDescrLen > 26) {
						char test[21];
						memcpy(test, &name[ifTable->table[i].dwDescrLen-26], 20);
						test[20] = 0;
						if (!stricmp(test, "QoS Packet Scheduler")) {
							if (!listed) {
								errorPrintf(0, "\t%s (Removed from list)\r\n", name);
							}
							continue;
						}
					}
				}//*/
				if (!listed) {
					errorPrintf(0, "\t%s\r\n", name);
				}
				//if (strstr(name, "QoS Packet Scheduler"))
				//if (!name || name[0] == 0) name = "Junk";
				if (Down.data) {
					v = Down.data->firstValue;
					while (v) {
						if (!v->updated &&
							strcmp(v->id, name) == 0) {
							break;
						}
						v = v->next;
					}
				}
				if (!v) {
					v = Down.AddValue((char*)ifTable->table[i].bDescr, ifTable->table[i].dwInOctets);
				}
				if (!v) continue;
				if (Up.data) {
					v2 = Up.data->firstValue;
					while (v2) {
						if (stricmp(v2->id, name) == 0 &&
							!v2->updated) {
							break;
						}
						v2 = v2->next;
					}
				}
				if (!v2) {
					v2 = Up.AddValue((char*)ifTable->table[i].bDescr, ifTable->table[i].dwOutOctets);
				}
				if (!v2) continue;
				if (div) {
					v->doubleValue = (ifTable->table[i].dwInOctets - (unsigned int)v->oldValue)/div;
					v->oldValue = ifTable->table[i].dwInOctets;
					v2->doubleValue = (ifTable->table[i].dwOutOctets - (unsigned int)v2->oldValue)/div;
					v2->oldValue = ifTable->table[i].dwOutOctets;
					vals[i*2] = v->doubleValue;
					vals[i*2+1] = v2->doubleValue;
					if (ifTable->table[i].dwType == IF_TYPE_ETHERNET_CSMACD && ifTable->table[i].dwPhysAddrLen == 6) {
						int foundMatch = 0;
						for (unsigned int j=0; j<i; j++) {
							if (ifTable->table[j].dwType == IF_TYPE_ETHERNET_CSMACD && ifTable->table[j].dwPhysAddrLen == 6) {
								if (!memcmp(ifTable->table[j].bPhysAddr, ifTable->table[i].bPhysAddr, 6) &&
									vals[i*2] == vals[j*2] && vals[i*2+1] == vals[j*2+1]) {
										foundMatch = 1;
								}
							}
						}
						if (!foundMatch) {
							downTotal += v->doubleValue;
							upTotal += v2->doubleValue;
						}
					}
				}
				v->updated = 1;
				v2->updated = 1;
			}
			if (!listed) {
				listed = 1;
				errorPrintf(0, "\r\n");
			}
		}
		free (ifTable);
	}
	Down.CleanOld();
	Up.CleanOld();
}


void __fastcall PerfMon::Update(int bandwidth) {
	//updated = 1;
	/*__int64 temp, f;
	QueryPerformanceCounter((LARGE_INTEGER*)&temp);
	QueryPerformanceFrequency((LARGE_INTEGER*)&f);
	double div = (temp-Down.freqCounter)/(double)f;
	//*/
	if (bandwidth) {
		UpdateNetwork();
	}
	if (!numCounters) {
		return;
	}
	//int t = GetTickCount();
	// too soon.
	//if (gTick - lastTime < 900) {
	//	return;
	//}
	//lastTime = gTick;

	//static int test = 60*60*6;

	//if (1||test < 0) test = 0;
	//while(test>=0) {test--;

	int i, j;
	char queryString[8000];
	char *q = queryString;
	{
		ULONG haveCpuData = 0;
		_SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION info[16];
		__int64 cpuTime;
		for (i=0; i<numCounters; i++) {
			//counters[i].read = 0;
			/*if (counters[i].data) {
				Value *v = counters[i].data->firstValue;
				while (v) {
					v->updated = 0;
					v = v->next;
				}
			}*/
			if ((int)counters[i].objectType.id < 0 || (int)counters[i].counter.id <0) {
				GetIDs();
				if ((int)counters[i].objectType.id < 0 || (int)counters[i].counter.id <0) continue;
			}

			if (counters[i].normalUpdate == 0) continue;
			if (counters[i].counter.id == 6 && counters[i].objectType.id == 238 && NtQuerySystemInformation) {
				if (!haveCpuData) {
					if (0 == NtQuerySystemInformation(8, info, sizeof(info), &haveCpuData) && haveCpuData % sizeof(info[0]) == 0 && haveCpuData > 0 && haveCpuData < sizeof(info)) {
						GetSystemTimeAsFileTime((LPFILETIME)&cpuTime);
					}
					else {
						NtQuerySystemInformation = 0;
					}
				}
				if (haveCpuData) {
					ULONG count = haveCpuData/sizeof(info[0]);
					int id = -1;
					int found = 0;
					__int64 sum = 0;
					for (unsigned int j = 0; j<count; j++) {
						char temp[5];
						itoa(j, temp, 10);
						__int64 s = *(__int64*)&info[j].IdleTime;
						sum += s;
						//double u = (s + info[i].UserTime)/(double)s;
						if (counters[i].object->value[0] != '*') {
							if (strcmp((char*)counters[i].object->value, temp)) continue;
						}
						Value *v = counters[i].FindOldValue(temp);
						if (!v) {
							v = counters[i].AddValue(temp, s);
							if (!v) continue;
						}
						else {
							v->updated = 1;
							if (cpuTime != v->oldDenominator) {
								v->doubleValue = 100*(1-(s - v->oldValue)/(double)(cpuTime-v->oldDenominator));
								if (v->doubleValue < 0) v->doubleValue = 0;
								else if (v->doubleValue >= 100) v->doubleValue = 100;
								v->oldValue = s;
							}
						}
						v->oldDenominator = cpuTime;
						found = 1;
					}
					if (counters[i].object->value[0] == '*' || !stricmp((char*)counters[i].object->value, "_Total")) {
						Value *v = counters[i].FindOldValue("_Total");
						if (!v) {
							v = counters[i].AddValue("_Total", sum);
							if (v) {
								found = 1;
								v->oldDenominator = cpuTime;
							}
						}
						else {
							v->updated = 1;
							if (cpuTime != v->oldDenominator) {
								v->doubleValue = 100*(count-(sum - v->oldValue)/(double)(cpuTime-v->oldDenominator))/count;
								if (v->doubleValue < 0) v->doubleValue = 0;
								else if (v->doubleValue >= 100) v->doubleValue = 100;
								v->oldValue = sum;
								found = 1;
								v->oldDenominator = cpuTime;
							}
						}
					}
					if (found) continue;
				}
			}
			for (j=i-1; j>=0; j--) {
				if (counters[i].objectType.id == counters[j].objectType.id && !counters[i].normalUpdate) break;
			}
			if (j>=0) continue;
			char *c = counters[i].objectTypeString;
			while (*c) {
				q++[0] = c++[0];
			}
			q++[0] = ' ';
		}
	}

	static unsigned long lastDataSize = 0;
	unsigned long dataSize = lastDataSize + 2000;
	unsigned char *data;

	if (q != queryString && (data = (unsigned char*) malloc(dataSize))) {
		q[-1] = 0;
		int w = ERROR_MORE_DATA;

		wchar_t *q = UTF8toUTF16Alloc((unsigned char*)queryString);
		if (q) {
			while (1) {
				w = RegQueryValueExW(HKEY_PERFORMANCE_DATA, q, 0, 0, data, &dataSize);
				if (w != ERROR_MORE_DATA || dataSize + 2000 < 2000) break;
				void *temp = realloc(data, dataSize += 2000);
				if (!temp) {
					w = ERROR_MORE_DATA;
					break;
				}
				data = (unsigned char*) temp;
			}
			free(q);
		}
		//RegCloseKey(HKEY_PERFORMANCE_DATA);

		PERF_DATA_BLOCK *header = (PERF_DATA_BLOCK *) data;
		if (w != ERROR_SUCCESS || dataSize < sizeof(PERF_DATA_BLOCK) ||
			header->HeaderLength < 0 || header->HeaderLength > dataSize) {
				lastDataSize = 0;
		}
		else {
			lastDataSize = dataSize;

			int pos = header->HeaderLength;

			for (unsigned int k=0; k<header->NumObjectTypes; k++) {
				PERF_OBJECT_TYPE *type = (PERF_OBJECT_TYPE*) (data+pos);
				//if (pos + sizeof(PERF_OBJECT_TYPE) > dataSize ||
				//	((pos + type->TotalByteLength > dataSize) |
				//	 (type->HeaderLength < sizeof(PERF_OBJECT_TYPE)) |
				//	 (type->DefinitionLength < type->HeaderLength) |
				//	 (type->DefinitionLength > type->TotalByteLength))) break;


				if (type->NumInstances) {
					for (i=0; i<numCounters; i++) {
						if (!counters[i].normalUpdate) continue;
						PERF_COUNTER_DEFINITION* counter = 0;
						if (counters[i].objectType.id == type->ObjectNameTitleIndex) {
							int pos2 = pos + type->HeaderLength;
							for (unsigned int j = 0; j <type->NumCounters; j++) {
								PERF_COUNTER_DEFINITION* def = (PERF_COUNTER_DEFINITION*) &data[pos2];
								//if (pos2 + sizeof(PERF_COUNTER_DEFINITION) > pos + type->TotalByteLength ||
								//	pos2 + def->ByteLength > pos + type->TotalByteLength ||
								//	def->ByteLength < 0 ||
								//	def->CounterOffset < 0 || def->CounterSize < 0)
								//	break;
								if (def->CounterNameTitleIndex == counters[i].counter.id) {
									counter = def;
									break;
								}
								pos2 += def->ByteLength;
							}
						}
						if (counter) {
							__int64 perfCounter;
							int counterType = counter->CounterType;
							int size = counterType&0x300;
							int dataType = counterType&0xC00;
							int dataFormat = counterType&0xF0000;
							int timerType =  counterType&0x300000;
							int extraCalc =  counterType&0x2C00000;
							int display =  counterType&0x70000000;
							__int64 freq = 1;

							//if (dataFormat == PERF_COUNTER_RATE) {
								if (timerType == PERF_TIMER_TICK) {
									perfCounter = *(__int64*)&header->PerfTime;
									freq = *(__int64*)&header->PerfFreq;
								}
								else if (timerType == PERF_OBJECT_TIMER) {
									perfCounter = *(__int64*)&type->PerfTime;
									freq = *(__int64*)&type->PerfFreq;
								}
								else /*if (timerType == PERF_TIMER_100NS)//*/ {
									perfCounter = *(__int64*)&header->PerfTime100nSec;
								}
								int t1 = PERF_COUNTER_100NS_QUEUELEN_TYPE;
								t1=t1;
							//}

							/*if (counter->CounterType == PERF_100NSEC_TIMER ||
								counter->CounterType == PERF_100NSEC_TIMER_INV) {
								perfCounter = *(__int64*)&header->PerfTime100nSec;
							}//*/
							int c=type->NumInstances;
							int pos2 = pos + type->DefinitionLength;
							while (c) {
								PERF_INSTANCE_DEFINITION *inst = 0;
								char *name = 0;
								if (type->NumInstances != PERF_NO_INSTANCES) {
									inst = (PERF_INSTANCE_DEFINITION *) &data[pos2];
									/*
									if (pos2 + sizeof(PERF_INSTANCE_DEFINITION) > dataSize ||
										inst->ByteLength < 0 || pos2 + inst->ByteLength > dataSize ||
										(inst->NameLength &&
										 (inst->NameOffset < 0 || inst->NameOffset < 0 ||
										  inst->ByteLength < inst->NameOffset + inst->NameLength)))
										break;
										//*/
									wchar_t * name2;
									if (inst->NameLength > 2 && ((wchar_t*)&data[pos2 + inst->NameOffset])[0]) {
										name2 = (wchar_t*)&data[pos2 + inst->NameOffset];
										name2[inst->NameLength/2-1] = 0;
										/*
										//if (type->CodePage == 0) {
										if (name[1] == 0) {
											name [inst->NameLength-2] = 0;
											int j = -1;
											do {
												j++;
												if (((wchar_t*)name)[j] > 127) {
													name[j] = '?';
												}
												else {
													name[j] = (char) ((wchar_t*)name)[j];
												}
											}
											while (((wchar_t*)name)[j]);
										}//*/
										name = (char*)UTF16toUTF8Alloc(name2);
									}
									else {
										name = "";
									}
									if (!((name[0] >= '0' && name[0] <= '9') ||
										  (name[0] >= 'a' && name[0] <= 'z') ||
										  (name[0] >= 'A' && name[0] <= 'Z') ||
										  name[0] == '_')) {
											  name=name;
									}

									pos2 += inst->ByteLength;
								}
								else {
									name = "";
								}

								PERF_COUNTER_BLOCK * block = (PERF_COUNTER_BLOCK *) &data[pos2];
								/*
								if (pos2 + sizeof(PERF_COUNTER_BLOCK) > dataSize ||
									block->ByteLength < 0 || pos2 + block->ByteLength > dataSize ||
									block->ByteLength < counter->CounterOffset + counter->CounterSize)
									break;
									//*/

								if ((counters[i].object->len == 1 && counters[i].object->value[0] == '*') ||
									stricmp((char*)counters[i].object->value, name) == 0) {
									if (counter->CounterOffset + counter->CounterSize <= block->ByteLength) {
										__int64 value;
										int happy = 0;
										if (counter->CounterSize == 4) {
											value = ((int*)&data[pos2 + counter->CounterOffset])[0];
											happy = 1;
										}
										else if (counter->CounterSize == 8) {
											value = ((__int64*)&data[pos2 + counter->CounterOffset])[0];
											happy = 1;
										}
										if (happy) {
											Value *v = counters[i].FindOldValue(name);
											if (!v) {
												v = counters[i].AddValue(name, value);
												if (!v) happy = 0;
											}
											else {
												v->updated = 1;
												happy = 2;
											}
											if (happy) {
												__int64 denom = 0;
												PERF_COUNTER_DEFINITION* counter2 = 0;
												double d = v->doubleValue;
												if (dataType == PERF_TYPE_COUNTER &&
													dataFormat == PERF_COUNTER_FRACTION) {
													counter2 = (PERF_COUNTER_DEFINITION*)&((char*)counter)[counter->ByteLength];
													if ((counter2->CounterType & 0x00000C00) == 0x00000400 &&
														(counter2->CounterType & 0x00070000) == 0x00030000) {
														if ((counter2->CounterType & 0x300) == 0) {
															denom = *(int*)&((char*)block)[counter2->CounterOffset];
														}
														else if ((counter2->CounterType & 0x300) == 0x100) {
															denom = *(__int64*)&((char*)block)[counter2->CounterOffset];
														}
														else {
															counter2 = 0;
														}
														if (denom == 0) denom = 1;
													}
													else
														counter2 = 0;
												}
												/*if (counter->DefaultScale) {
													int c = counter->DefaultScale;
													while (c<0) {
														c++;
														value /= 10;
													}
													while (c>0) {
														c--;
														value *= 10;
													}
												}//*/
												if (counter->CounterType == PERF_RAW_FRACTION ||
													counter->CounterType == PERF_LARGE_RAW_FRACTION) {
													if (counter2) {
														d = 100*(double)value / (double)denom;
													}
												}
												else if (counter->CounterType == PERF_COUNTER_RAWCOUNT ||
													counter->CounterType == PERF_COUNTER_LARGE_RAWCOUNT) {
													d = (double)value;
													//v->intValue = value;
												}
												else if (counter->CounterType ==  PERF_ELAPSED_TIME) {
													// divide by secs/100 ns
													d = (perfCounter-value)/10000000.0;
												}
												else if (happy == 2) {
													if (perfCounter != counters[i].freqCounter) {
														double diff;
														double div = (double)(perfCounter - counters[i].freqCounter);
														if (counter->CounterSize == 4) {
														/* if ((counter->CounterType & 0x300) == 0) { */
															diff = (int) value - (int)v->oldValue;
														}
														else /*if ((counter->CounterType & 0x300) == 0x100)*/ {
															diff = (double)(value - v->oldValue);
														}
														if (counter->CounterType == PERF_COUNTER_COUNTER ||
															counter->CounterType == PERF_SAMPLE_COUNTER ||
															counter->CounterType == PERF_COUNTER_BULK_COUNT) {
															d = diff/(div/(double)*(__int64*)&header->PerfFreq);
														}
														else if (counter->CounterType == PERF_100NSEC_TIMER ||
																 counter->CounterType == PERF_COUNTER_TIMER ||
																 counter->CounterType == PERF_OBJ_TIME_TIMER ||
																 counter->CounterType == PERF_PRECISION_SYSTEM_TIMER ||
																 counter->CounterType == PERF_PRECISION_100NS_TIMER ||
																 counter->CounterType == PERF_PRECISION_OBJECT_TIMER ||
																 counter->CounterType == PERF_SAMPLE_FRACTION) {
															d = 100*diff/div;
															if (d<0) d = 0;
															//else if (d > 100) d = 100;
														}
														else if (counter->CounterType == PERF_AVERAGE_BULK ||
																 counter->CounterType == PERF_COUNTER_QUEUELEN_TYPE ||
																 counter->CounterType == PERF_COUNTER_100NS_QUEUELEN_TYPE ||
																 counter->CounterType == PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE ||
																 counter->CounterType == PERF_COUNTER_LARGE_QUEUELEN_TYPE) {
															d = diff/div;
															//else if (d > 100) d = 100;
														}
														else if (counter->CounterType == PERF_AVERAGE_TIMER) {
															d = diff/div/((double)*(__int64*)&header->PerfFreq);
															//else if (d > 100) d = 100;
														}
														else if (counter->CounterType == PERF_100NSEC_TIMER_INV) {
															d = 100*(1-diff/div);
															if (d<0) d = 0;
															//else if (d > 100) d = 100;
														}
													}
													v->oldValue = value;
												}
												if (_finite(d))
													v->doubleValue = d;
											}
										}
									}
								}
								if (name[0]) free(name);
								if (type->NumInstances == PERF_NO_INSTANCES) {
									c = 0;
								}
								else {
									c--;
								}
								pos2 += block->ByteLength;
							}
							counters[i].freqCounter = perfCounter;
						}
					}
				}
				pos += type->TotalByteLength;
			}
		}
		free(data);
	}

	for (i=0; i<numCounters; i++) {
		if (counters[i].normalUpdate)
			counters[i].CleanOld();
	}

	//}

	//lastTime = t;
	//updated = 1;
	//*/
	return;
	/*
	if (lastTime) {
		int i;
		for (i=sizeof(singleCounters)/sizeof(singleCounters[0])-1; i>=0; i--) {
			PdhGetFormattedCounterValue(singleCounters[i].hCounter, PDH_FMT_DOUBLE, 0, &singleCounters[i].val);
		}

		unsigned long s = 0;
		unsigned long c;

		for(i=sizeof(multiCounters)/sizeof(multiCounters[0])-1; i>=0; i--) {
			unsigned long s2 = 0;
			int w = PdhGetFormattedCounterArray(multiCounters[i].hCounter, PDH_FMT_DOUBLE, &s2, &c, 0);
			if (w == PDH_MORE_DATA) {
				if (s2 > s) s = s2;
			}
			multiCounters[i].val.doubleValue = 0;
		}
		PDH_FMT_COUNTERVALUE_ITEM *data = (PDH_FMT_COUNTERVALUE_ITEM*) malloc(s);
		if (data) {
			for(i=sizeof(multiCounters)/sizeof(multiCounters[0])-1; i>=0; i--) {
				unsigned long s2 = s;
				int w = PdhGetFormattedCounterArray(multiCounters[i].hCounter, PDH_FMT_DOUBLE, &s2, &c, data);
				if (w == ERROR_SUCCESS) {
					multiCounters[i].val.doubleValue = 0;
					for (int j=c-1; j>=0; j--) {
						if (stricmp(data[j].szName, "MS TCP Loopback interface"))
							multiCounters[i].val.doubleValue += data[j].FmtValue.doubleValue;
					}
				}
			}
			free(data);
		}

		//PdhGetFormattedCounterValue(net.hCounter, PDH_FMT_LONG, 0, &net.val);
	}
	*/
}

/*
int PerfMon::AddCounter(char *counter) {
	int i = 0;
	while(counters[i].name) i++;
	counters[i].name = counter;
	if (hQuery && PdhAddCounter(hQuery, counters[i].name, 0, &counters[i].hCounter) != ERROR_SUCCESS)
		Cleanup();
	lastTime -= 10000;
	numCounters++;
	return i;
}
*/

int PerfMon::GetCounter(int id) {
	for (int i=0; i<numCounters; i++) {
		if (counters[i].id == id) return i;
	}
	return -1;
}

void PerfMon::RemoveCounter(unsigned int id) {
	int i = GetCounter(id);
	if (i>=0) {
		if (0 == --counters[i].refCount) {
			numCounters --;
			counters[i].object->Release();
			counters[i].objectType.s->Release();
			counters[i].counter.s->Release();
			free(counters[i].data);
			counters[i] = counters[numCounters];
		}
	}
}

int PerfMon::SetUpdate(int id, int update) {
	int i = GetCounter(id);
	if (i<0 || !counters[i].singleton) return i;
	int out = counters[i].normalUpdate;
	counters[i].normalUpdate = update;
	return out;
}

int PerfMon::GetParams(int id, StringValue *&objectType, StringValue *&object, StringValue *&counter, short &singleton) {
	int i = GetCounter(id);
	if (i<0) return i;
	objectType = counters[i].objectType.s;
	object = counters[i].object;
	counter = counters[i].counter.s;
	singleton = counters[i].singleton;
	return i;
}


/*
int PerfMon::Init() {
	Cleanup();
	//"Process(*)\\ID Process"
	//"\\Process(*)\\Processor Time"
	//"\\Process(*)\\Private Bytes"
	if (PdhOpenQuery(NULL, NULL, &hQuery) == ERROR_SUCCESS) {
		int i;
		for (i=sizeof(counters)/sizeof(Counter)-1; i>=0; i--) {
			if (counters[i].name)
				if (PdhAddCounter(hQuery, counters[i].name, 0, &counters[i].hCounter) != ERROR_SUCCESS) break;
		}
		if (i<0) return 1;

	}
	Cleanup();
	return 0;
}*/
int PerfMon::GetCounterType(int id) {
	int i = GetCounter(id);
	if (id < 0) return 0;
	return (counters[i].object->len == 1 && counters[i].object->value[0] == '*');
}


double PerfMon::GetVal(unsigned int id, __int64 &updated) {
	int i = GetCounter(id);
	if (i < 0 || counters[i].data == 0)
		return 0;
	updated = counters[i].freqCounter;
	return counters[i].data->firstValue->doubleValue;
}

Value *PerfMon::GetVals(unsigned int id, __int64 &updated) {
	int i = GetCounter(id);
	if (i < 0 || counters[i].data == 0)
		return 0;
	updated = counters[i].freqCounter;
	return counters[i].data->firstValue;
}

void PerfMon::Cleanup() {
	while (numCounters) {
		RemoveCounter(counters[0].id);
	}
	free(Up.data);
	free(Down.data);
	Up.data = 0;
	Down.data = 0;
	RegCloseKey(HKEY_PERFORMANCE_DATA);
	free(counters);
	//lastTime = 0;
}
