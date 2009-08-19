#include "global.h"
#include "Config.h"
#include "Unicode.h"
#include "stringUtil.h"
#include <stdarg.h>
#include "util.h"
#include "malloc.h"
#include <commctrl.h>

//#define filename "LCDMisc.ini"


Config config;

App *Config::apps = 0;
int Config::numApps = 0;

void DisplayErrorLine(unsigned char *line, unsigned char *pos) {
	if (!line) {
		return;
	}
	unsigned char *t = line;
	while (*t != '\r' && *t != 0 && *t != '\n') {
		errorPrintf(2, "%c", *t);
		t++;
	}
	t = line;
	errorPrintf(2, "\r\n");
	while (t < pos) {
		if (*t == '\t')
			errorPrintf(2, "\t");
		else
			errorPrintf(2, " ");
		t++;
	}
	errorPrintf(2, "^\r\n");
}


int GetConfFile(wchar_t *s, wchar_t *ext) {
	//MessageBoxA(0, "here", "HERE", MB_OK);
	wsprintf(s, L"LCDMisc%s", ext);
	wchar_t *o = GetFile(s);
	if (!o || wcslen(o) > MAX_PATH) {
		free(o);
		return 0;
	}
	wcscpy(s, o);
	free(o);
	return 1;
}
HWND hWndDebug = 0;

WNDPROC wndProcDebugOld = 0;

LRESULT CALLBACK WndProcDebug(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT out = 0;
	if (uMsg == WM_CLOSE || uMsg == WM_DESTROY) {
		if (wndProcDebugOld) {
			SetWindowLongPtr(hWndDebug, GWLP_WNDPROC, (__int3264)(LONG_PTR)wndProcDebugOld);
			out = CallWindowProc(wndProcDebugOld, hWnd, uMsg, wParam, lParam);
		}
		hWndDebug = 0;
		wndProcDebugOld = 0;
		return out;
	}
	return CallWindowProc(wndProcDebugOld, hWnd, uMsg, wParam, lParam);
}

void CreateDebugWindow() {
	static BOOL inited = 0;
	if (!inited) {
		InitCommonControls();
		inited = 1;
	}
	if (!hWndDebug && !wndProcDebugOld) {
		hWndDebug = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"LDC Miscellany Debug Window.\r\n",
					  ES_MULTILINE | WS_OVERLAPPEDWINDOW,
					  CW_USEDEFAULT, CW_USEDEFAULT,
					  CW_USEDEFAULT, CW_USEDEFAULT,
					  0,0, ghInst, 0);
		ShowWindow(hWndDebug, 1);

		wndProcDebugOld = (WNDPROC)(LONG_PTR)SetWindowLongPtr(hWndDebug, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProcDebug);
	}
}



void verrorPrintf(int mode, char *string, va_list &arg) {
	wchar_t file[MAX_PATH*2];
	if (!(mode &4)) {
		if (GetConfFile(file, L".log")) {
			FILE *outf = _wfopen(file, L"ab");
			if (outf) {
				vfprintf (outf, string, arg);
				fflush(outf);
				fclose(outf);
			}
		}
	}
	mode &= 3;
	if (mode == 2) {
		static int tested = -1;
		if (tested < 0) {
			tested = (config.GetConfigInt((unsigned char*)"Script", (unsigned char*) "ScriptDebug", 0) & 2)>>1;
		}
		mode = tested;
	}
	if (mode) {
		CreateDebugWindow();
		LRESULT l = SendMessage(hWndDebug, WM_GETTEXTLENGTH, 0, 0);
		SendMessage(hWndDebug, EM_SETSEL, l, l);
		char buffer[10000];
		_vsnprintf(buffer, 10000, string, arg);
		buffer[sizeof(buffer)-1] = 0;
		wchar_t *temp = UTF8toUTF16Alloc((unsigned char*)buffer, 0);
		if (temp) {
			SendMessage(hWndDebug, EM_REPLACESEL, 0, (LPARAM)temp);
			free(temp);
		}
	}
}

void __cdecl errorPrintf(int mode, char *string, ...) {
	va_list arg;

	va_start (arg, string);
	verrorPrintf (mode, string, arg);
	va_end (arg);
}

void App::Cleanup() {
	int i;
	for (i=0; i<numPairs; i++) {
		pairs[i].key->Release();
		pairs[i].value->Release();
	}
	free(pairs);
	for (i=0; i<numPairsStatic; i++) {
		pairsStatic[i].key->Release();
		pairsStatic[i].value->Release();
	}
	free(pairsStatic);
	free(name);
}

int App::SetVal(StringValue* key, StringValue* val, int type) {
	if (type != 2) {
		//int i = 0;
		//int res = GetValStatic(key);
		//if (res < 0) {
		if (srealloc(pairsStatic, (numPairsStatic+1) * sizeof(KeyValPair))) {
			memset(pairsStatic + numPairsStatic, 0, sizeof(KeyValPair));
			key->AddRef();
			val->AddRef();
			pairsStatic[numPairsStatic].key = key;
			pairsStatic[numPairsStatic].value = val;
			numPairsStatic++;
			return 1;
		}
		return 0;
	}
	else {
		int res = GetValDynamic(key->value, key->len);
		if (res < 0) {
			if (srealloc(pairs, (numPairs+1) * sizeof(KeyValPair))) {
				memset(pairs + numPairs, 0, sizeof(KeyValPair));
				key->AddRef();
				val->AddRef();
				pairs[numPairs].key = key;
				pairs[numPairs].value = val;
				numPairs++;
				return 1;
			}
			return 0;
		}
		else {
			pairs[res].value->Release();
			pairs[res].value = val;
			val->AddRef();
			return 1;
		}
	}
}

void App::SetVal(unsigned char* key, unsigned char* val, int type) {
	ScriptValue sv;
	if (!CreateStringValue(sv, val)) return;
	StringValue *newVal = sv.stringVal;
	if (type != 2) {
		//int i = 0;
		//int res = GetValStatic(key);
		//if (res < 0) {
		if (srealloc(pairsStatic, (numPairsStatic+1) * sizeof(KeyValPair)) && CreateStringValue(sv, key)) {
			memset(pairsStatic + numPairsStatic, 0, sizeof(KeyValPair));
			pairsStatic[numPairsStatic].key = sv.stringVal;
			pairsStatic[numPairsStatic].value = newVal;
			numPairsStatic++;
			return;
		}
		newVal->Release();
		return;
		//}
		//else {
		//	free(pairsStatic[res].value);
		//	pairsStatic[res].value = newVal;
		//}
	}
	else {
		int len = strlen(key);
		int res = GetValDynamic(key, len);
		if (res < 0) {
			if (srealloc(pairs, (numPairs+1) * sizeof(KeyValPair)) && CreateStringValue(sv, key, len)) {
				memset(pairs + numPairs, 0, sizeof(KeyValPair));
				pairs[numPairs].key = sv.stringVal;
				pairs[numPairs].value = newVal;
				numPairs++;
				return;
			}
			newVal->Release();
			return;
		}
		else {
			pairs[res].value->Release();
			pairs[res].value = newVal;
		}
	}
}

StringValue* App::GetVal(StringValue *key, int type, int index) {
	int res;
	if (!(type & 2)) {
		if ((res = GetValStatic(key->value, key->len, index)) >= 0) return pairsStatic[res].value;
	}
	if (index) return 0;
	if (!(type & 1)) {
		if ((res = GetValDynamic(key->value, key->len)) >= 0) return pairs[res].value;
	}
	return 0;
}

unsigned char* App::GetVal(unsigned char *key, int type, int index) {
	int res;
	if (!(type & 2)) {
		if ((res = GetValStatic(key, strlen(key), index)) >= 0) return pairsStatic[res].value->value;
	}
	if (index) return 0;
	if (!(type & 1)) {
		if ((res = GetValDynamic(key, strlen(key))) >= 0) return pairs[res].value->value;
	}
	return 0;
}

int App::GetValStatic(unsigned char *key, int len, int index) {
	int w = 0;
	for (int i=0; i<numPairsStatic; i++) {
		if (pairsStatic[i].key->len == len && strncmp(key, pairsStatic[i].key->value, len) == 0) {
			if (w == index)
				return i;
			w++;
		}
	}
	return -1;
}

int App::GetValDynamic(unsigned char *key, int len) {
	for (int i=0; i<numPairs; i++) {
		if (pairs[i].key->len == len && strncmp(key, pairs[i].key->value, len) == 0) return i;
	}
	return -1;
}

void Config::Cleanup() {
	for (int i=0; i<numApps; i++) {
		apps[i].Cleanup();
	}
	free(apps);
	apps = 0;
	numApps = 0;
}

App *Config::GetApp(unsigned char* app) {
	for (int i=0; i<numApps; i++) {
		if (stricmp(apps[i].name, app)==0) return &apps[i];
	}
	if (!srealloc(apps, sizeof(App)*(numApps+1))) return 0;
	memset(&apps[numApps], 0, sizeof(App));
	apps[numApps].name = strdup(app);
	if (!apps[numApps].name) return 0;
	return &apps[numApps++];
}

void Config::Reload() {
	Cleanup();
	wchar_t fileName[MAX_PATH + 20];
	FileLineReader* in;
	unsigned char* line = 0;
	for (int i=1; i<3; i++) {
		App* app = GetApp((unsigned char*)"General");
		wchar_t *ext = L".ini";
		if (i == 2) ext = L".cfg";
		if (GetConfFile(fileName, ext) && (in = OpenLineReader(fileName, 1, SINGLE_LINE, "#%"))) {
			while (in->NextLine(&line) >= 0) {
				LeftJustify(line);
				if (line[0] == '[') {
					line ++;
					while (*line == ' ' || *line == '\t') line++;
					unsigned char *end = strchr(line, ']');
					if (end) *end = 0;
					LeftJustify(line);
					app = GetApp(line);
				}
				else if (app) {
					unsigned char *val = strchr(line, '=');
					if (val) {
						*val = 0;
						val ++;
						LeftJustify(val);
						LeftJustify(line);
						int len = Unescape(line, strlen(line));
						int len2 = Unescape(val, strlen(val));
						ScriptValue sv, sv2;
						if (CreateStringValue(sv, line, len)) {
							if (CreateStringValue(sv2, val, len2)) {
								app->SetVal(sv.stringVal, sv2.stringVal, i);
								sv2.stringVal->Release();
							}
							sv.stringVal->Release();
						}
					}
				}
			}
			delete in;
		}
	}
}

unsigned char *Config::FindKey(unsigned char *appName, unsigned char *key, int type, int index) {
	App *app = GetApp(appName);
	if (!app) return 0;
	return app->GetVal(key, type, index);
}

StringValue *Config::FindKey(unsigned char *appName, StringValue *key, int type, int index) {
	App *app = GetApp(appName);
	if (!app) return 0;
	return app->GetVal(key, type);
}

void Config::SetDynamicAppValue(unsigned char *appName, unsigned char *key, unsigned char* value) {
	App *app = GetApp(appName);
	if (!app) return;
	app->SetVal(key, value, 2);
}

int Config::SetDynamicAppValue(unsigned char *appName, StringValue *key, StringValue *value) {
	App *app = GetApp(appName);
	if (!app) return 0;
	return app->SetVal(key, value, 2);
}

wchar_t *Config::GetConfigString (unsigned char *appName, unsigned char*key, wchar_t *defaultVal, int type, int index) {
	if (!apps) Reload();
	unsigned char *val = FindKey(appName, key, type, index);
	if (val) {
		wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t)*(1+strlen(val)));
		int len = UTF8toUTF16(out, val, -1);
		srealloc(out, sizeof(wchar_t) * (len+1));
		return out;
	}
	return wcsdup(defaultVal);
}

StringValue *Config::GetConfigString(unsigned char *appName, StringValue *key, StringValue *defaultVal, int type, int index) {
	if (!apps) Reload();
	StringValue *out = FindKey(appName, key, type, index);
	if (out) return out;
	SetDynamicAppValue(appName, key, defaultVal);
	return defaultVal;
}

unsigned char *Config::GetConfigStringAndKey (unsigned char *appName, unsigned char* &key, int index) {
	if (!apps) Reload();
	App *app = GetApp(appName);
	if (app) {
		if (index >= 0 && index < app->numPairsStatic) {
			unsigned char* val = app->pairsStatic[index].value->value;
			if (val && (val = strdup(val))) {
				key = app->pairsStatic[index].key->value;
				return val;
			}
		}
	}
	key = 0;
	return 0;
}

unsigned char *Config::GetConfigString (unsigned char *appName, unsigned char*key, unsigned char *defaultVal, int type, int index) {
	if (!apps) Reload();
	unsigned char *val = FindKey(appName, key, type, index);
	if (val) return strdup(val);
	if (defaultVal)	return strdup(defaultVal);
	return 0;
}

int Config::GetConfigInt (unsigned char *appName, unsigned char*key, int defaultVal, int type, int index) {
	if (!apps) Reload();
	unsigned char *val = FindKey(appName, key, type, index);
	if (!val) return defaultVal;
	return (int)strtoi64((char*)val, 0, 0);
}

__int64 Config::GetConfigInt64 (unsigned char *appName, unsigned char*key, __int64 defaultVal, int type, int index) {
	if (!apps) Reload();
	unsigned char *val = FindKey(appName, key, type, index);
	if (!val) return defaultVal;
	return strtoi64((char*)val, 0, 0);
}

void Config::Save() {
	wchar_t fileName[MAX_PATH + 20];
	FILE *out;
	if (!GetConfFile(fileName, L".cfg") || !(out = _wfopen(fileName, L"wb"))) return;
	unsigned char header[3] = {0xEF, 0xBB, 0xBF};
	fwrite(header, 1, 3, out);
	for (int i=0; i<numApps; i++) {
		if (apps[i].pairs) {
			if (i)
				fprintf(out, "[%s]\n\n", apps[i].name);
			for (int j=0; j<apps[i].numPairs; j++) {
				int k;
				StringValue *s = apps[i].pairs[j].key;
				for (k = 0; k < s->len; k++) {
					unsigned char c = s->value[k];
					if (c >= 20 && ((c !=' ' && c !='\t') || (k && k<s->len-1))) fputc(c, out);
					else {
						fputc('|', out);
						if (s->value[k+1] >= '0' && s->value[k+1] <= '9') {
							fputc(0, out);
							fputc(k/10, out);
							fputc(k%10, out);
						}
						else {
							if (k >= 10)
								fputc(k/10, out);
							fputc(k%10, out);
						}
					}
				}
				fprintf(out, " = ");
				s = apps[i].pairs[j].value;
				for (k = 0; k < s->len; k++) {
					unsigned char c = s->value[k];
					if (c >= 20) fputc(c, out);
					else {
						if (s->value[k+1] >= '0' && s->value[k+1] <= '9') {
							fputc(0, out);
							fputc(k/10, out);
							fputc(k%10, out);
						}
						else {
							if (k >= 10)
								fputc(k/10, out);
							fputc(k%10, out);
						}
					}
				}
				fputc('\n', out);
				//fprintf(out, "%s = %s\n", apps[i].pairs[j].key, apps[i].pairs[j].value);
			}
			fprintf(out, "\n");
		}
	}
	fclose(out);
}

int Config::SaveConfigString(unsigned char *appName, StringValue *key, StringValue *val, int saveNow) {
	if (!apps) Reload();
	int res = SetDynamicAppValue(appName, key, val);
	if (saveNow)
		Save();
	return res;
}

void Config::SaveConfigString(unsigned char *appName, unsigned char *key, unsigned char *val, int saveNow) {
	if (!apps) Reload();
	SetDynamicAppValue(appName, key, val);
	if (saveNow)
		Save();
}

void Config::SaveConfigInt(unsigned char *appName, unsigned char *key, int val, int saveNow) {
	if (!apps) Reload();
	char temp[100];
	_i64toa(val, temp, 10);
	SetDynamicAppValue(appName, key, (unsigned char*) temp);
	if (saveNow)
		Save();
}

void Config::SaveConfigInt64(unsigned char *appName, unsigned char *key, __int64 val, int saveNow) {
	if (!apps) Reload();
	char temp[100];
	_i64toa(val, temp, 10);
	SetDynamicAppValue(appName, key, (unsigned char*) temp);
	if (saveNow)
		Save();
}

/*
char* GetConfigString(char *appName, char *keyName, char *defaultVal) {
	if (!confFile[0]) GetConfFile();
	char temp[2001];
	int len = GetPrivateProfileString(appName, keyName, defaultVal, temp, 2000, confFile);
	// only an issue when more than 10000 chars, but can't hurt.
	temp[len+1] = 0;
	char *out = (char*) malloc((strlen(temp)+1)*sizeof(char));
	if (!out) return 0;
	strcpy(out, temp);
	return out;
}

int GetConfigInt(char *appName, char *keyName, int defaultVal) {
	if (!confFile[0]) GetConfFile();
	char temp[2001], temp2[20];
	sprintf(temp2, "%i%", defaultVal);
	int len = GetPrivateProfileString(appName, keyName, temp2, temp, 2000, confFile);
	// only an issue when more than 10000 chars, but can't hurt.
	temp[len+1] = 0;
	char *junk;
	return strtol(temp, &junk, 0);
}

void SaveConfigString(char *appName, char *keyName, char *val) {
	if (!confFile[0]) GetConfFile();
	WritePrivateProfileString(appName, keyName, val, confFile);
}

void SaveConfigInt(char *appName, char *keyName, int val) {
	if (!confFile[0]) GetConfFile();
	char temp[20];
	sprintf(temp, "%i", val);
	WritePrivateProfileString(appName, keyName, temp, confFile);
}
//*/