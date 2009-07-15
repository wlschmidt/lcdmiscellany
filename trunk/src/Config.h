#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include "ScriptValue.h"

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

void __cdecl errorPrintf(int mode, char *string, ...);

void verrorPrintf(int mode, char *string, va_list &arg);

int GetConfFile(wchar_t *s, wchar_t *ext);

void DisplayErrorLine(unsigned char *line, unsigned char *pos);

struct KeyValPair {
	StringValue *key;
	StringValue *value;
};

struct App {
	unsigned char *name;
	KeyValPair *pairsStatic;
	int numPairsStatic;
	KeyValPair *pairs;
	int numPairs;
	void Cleanup();
	void SetVal(unsigned char* key, unsigned char* val, int type);
	unsigned char* GetVal(unsigned char *key, int type=0, int index = 0);
	int GetValStatic(unsigned char *key, int len, int index);
	int GetValDynamic(unsigned char *key, int len);

	int SetVal(StringValue *key, StringValue* val, int type);
	StringValue* GetVal(StringValue *key, int type=0, int index = 0);
};

// Rather crude.  I keep the entire text file in memory, along with indices of start positions.
class Config {
	// static to make sure they get initialized before other constructors try to use
	// this class.  could prolly do this in the constructor without issue.
	static App * apps;
	static int numApps;

	void Cleanup();
	App *GetApp(unsigned char *app);
	unsigned char *FindKey(unsigned char *app, unsigned char *key, int type, int index);
	StringValue *FindKey(unsigned char *app, StringValue *key, int type, int index);
	void SetDynamicAppValue(unsigned char *app, unsigned char *key, unsigned char* value);
public:
	void Save();
	//inline Config() {}
	inline ~Config() {
		Cleanup();
	}
	void Reload();

	unsigned char *GetConfigString (unsigned char *appName, unsigned char*key, unsigned char *defaultVal, int type=0, int index = 0);
	unsigned char *GetConfigStringAndKey (unsigned char *appName, unsigned char* &key, int index);
	wchar_t *GetConfigString (unsigned char *appName, unsigned char*key, wchar_t *defaultVal, int type=0, int index = 0);
	int GetConfigInt(unsigned char *appName, unsigned char *key, int defaultVal, int type=0, int index = 0);
	__int64 GetConfigInt64(unsigned char *appName, unsigned char *key, __int64 defaultVal, int type=0, int index = 0);

	void SaveConfigString(unsigned char *appName, unsigned char *key, unsigned char *val, int saveNow = 0);
	void SaveConfigInt(unsigned char *appName, unsigned char *key, int val, int saveNow = 0);
	void SaveConfigInt64(unsigned char *appName, unsigned char *key, __int64 val, int saveNow = 0);

	int SetDynamicAppValue(unsigned char *appName, StringValue *key, StringValue *value);
	int SaveConfigString(unsigned char *appName, StringValue *key, StringValue *val, int saveNow = 0);
	StringValue *GetConfigString(unsigned char *appName, StringValue *key, StringValue *defaultVal, int type=0, int index=0);
};

extern Config config;



#endif
