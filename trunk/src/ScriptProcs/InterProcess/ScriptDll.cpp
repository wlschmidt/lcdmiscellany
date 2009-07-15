#include "../../global.h"
#include "ScriptDll.h"
#include "DllTypes.h"
#include "../../SymbolTable.h"
#include "../../unicode.h"
#include "../../Config.h"
#include "../../vm.h"

int ConvertValue(ScriptValue &in, ScriptVal &out) {
	memset(&out, 0, sizeof(out));
	out.type = in.type;
	if (in.type == SCRIPT_INT) {
		out.intValue = in.intVal;
	}
	else if (in.type == SCRIPT_DOUBLE) {
		out.doubleValue = in.doubleVal;
	}
	else if (in.type == SCRIPT_LIST) {
		out.listValue.vals = (ScriptVal*) malloc(sizeof(ScriptVal) * (1+in.listVal->numVals));
		if (!out.listValue.vals) {
			out.type = SCRIPT_NULL_DLL;
			return 0;
		}
		out.flags = FLAG_NEED_FREE_INTERNAL;
		out.listValue.numVals = in.listVal->numVals;
		for (int i=0; i<in.listVal->numVals; i++) {
			ConvertValue(in.listVal->vals[i], out.listValue.vals[i]);
		}
	}
	else if (in.type == SCRIPT_DICT) {
		out.dictValue.entries = (ScriptDictEntry*) malloc(sizeof(ScriptDictEntry) * (1+in.dictVal->numEntries));
		if (!out.dictValue.entries) {
			out.type = SCRIPT_NULL_DLL;
			return 0;
		}
		out.flags = FLAG_NEED_FREE_INTERNAL;
		out.dictValue.numEntries = in.dictVal->numEntries;
		for (int i=0; i<in.dictVal->numEntries; i++) {
			ConvertValue(in.dictVal->entries[i].key, out.dictValue.entries[i].key);
			ConvertValue(in.dictVal->entries[i].val, out.dictValue.entries[i].value);
		}
	}
	else if (in.type == SCRIPT_STRING) {
		out.stringValue.len = in.stringVal->len;
		out.stringValue.value = in.stringVal->value;
	}
	else {
		out.type = SCRIPT_NULL_DLL;
	}
	return 1;
}

int ConvertValue(ScriptVal &in, ScriptValue &out, FreeProc freeProc) {
	if (in.type == SCRIPT_NULL_DLL) {
		CreateNullValue(out);
	}
	else if (in.type == SCRIPT_INT) {
		CreateIntValue(out, in.intValue);
	}
	else if (in.type == SCRIPT_DOUBLE) {
		CreateDoubleValue(out, in.doubleValue);
	}
	else if (in.type == SCRIPT_LIST) {
		if (!CreateListValue(out, in.listValue.numVals|1)) {
			out.type = -1;
			return 0;
		}
		for (int i=0; i<in.listValue.numVals; i++) {
			ScriptValue temp;
			ConvertValue(in.listValue.vals[i], temp, freeProc);
			out.listVal->PushBack(temp);
		}
		if (in.flags & FLAG_NEED_FREE_DLL) freeProc(in.stringValue.value);
	}
	else if (in.type == SCRIPT_DICT) {
		if (!CreateDictValue(out, in.dictValue.numEntries|1)) {
			out.type = -1;
			return 0;
		}
		for (int i=0; i<in.dictValue.numEntries; i++) {
			ScriptValue key, value;
			ConvertValue(in.dictValue.entries[i].key, key, freeProc);
			ConvertValue(in.dictValue.entries[i].value, value, freeProc);
			out.dictVal->Add(key, value);
		}
		if (in.flags & FLAG_NEED_FREE_DLL) freeProc(in.dictValue.entries);
	}
	else if (in.type == SCRIPT_STRING || in.type == SCRIPT_WIDE_STRING || in.type == SCRIPT_ASCII_STRING) {
		if (in.type == SCRIPT_STRING) {
			CreateStringValue(out, in.stringValue.value, in.stringValue.len);
		}
		else {
			unsigned char *string;
			if (in.type == SCRIPT_ASCII_STRING) {
				string = ASCIItoUTF8Alloc(in.asciiStringValue.value);
			}
			else if (in.type == SCRIPT_WIDE_STRING) {
				string = UTF16toUTF8Alloc(in.wideStringValue.value);
			}
			if (string) {
				CreateStringValue(out, string);
				free(string);
			}
			else {
				CreateStringValue(out);
			}
		}
		if (in.flags & FLAG_NEED_FREE_DLL) freeProc(in.stringValue.value);
	}
	return 1;
}

void FreePassedValues(ScriptVal &v) {
	if (v.type == SCRIPT_LIST) {
		for (int i=0; i<v.listValue.numVals; i++) {
			FreePassedValues(v.listValue.vals[i]);
		}
		if (v.flags & FLAG_NEED_FREE_INTERNAL) free(v.listValue.vals);
	}
	else if (v.type == SCRIPT_DICT) {
		for (int i=0; i<v.dictValue.numEntries; i++) {
			FreePassedValues(v.dictValue.entries[i].key);
			FreePassedValues(v.dictValue.entries[i].value);
		}
		if (v.flags & FLAG_NEED_FREE_INTERNAL) free(v.dictValue.entries);
	}
	else if (v.type == SCRIPT_STRING || v.type == SCRIPT_WIDE_STRING || v.type == SCRIPT_ASCII_STRING) {
		if (v.flags & FLAG_NEED_FREE_INTERNAL) free(v.stringValue.value);
	}
}

struct Dll {
	DllInfo info;

	unsigned char *alias;
	unsigned char *name;
	char *init;
	HMODULE hModule;

	void Cleanup() {
		free(alias);
		free(name);
		free(init);
		if (hModule) {
			FreeLibrary(hModule);
		}
	}
};

struct DllFunction {
	DllProc proc;
	int dll;
	char *name;
	unsigned char *args;
	unsigned char returnType;


	void Cleanup() {
		free(args);
		free(name);
	}
};

int numDlls=0;
Dll *dlls=0;


int numDllFuncts=0;
DllFunction *dllFuncts=0;

int FindDll(unsigned char *alias) {
	for (int i = 0; i<numDlls; i++) {
		if (utf8stricmp(alias, dlls[i].alias) == 0) return i;
	}
	return -1;
}

int CreateDll(unsigned char *alias, unsigned char *dll, unsigned char *init) {
	int id = FindDll(alias);
	if (id >= 0) {
		return  0 != utf8stricmp(dll, dlls[id].name);
	}
	if (!srealloc(dlls, sizeof(Dll) * (numDlls+1))) return 1;
	memset(&dlls[numDlls], 0, sizeof(Dll));
	if (!(dlls[numDlls].name = strdup(dll)) ||
		!(dlls[numDlls].alias = strdup(alias)) ||
		(init && !(dlls[numDlls].init = UTF8toASCIIAlloc(init)))) {
			dlls[numDlls].Cleanup();
			return 2;
	}
	numDlls++;
	return 0;
}

int CreateDllFunction(unsigned char *function, unsigned char *dllAlias, unsigned char returnType, unsigned char *argTypes) {
	int id = FindDll(dllAlias);
	if (id < 0) return -1;
	/*for (int i = 0; i<numDllFuncts; i++) {
		if (utf8stricmp(function, dllFuncts[i].name) == 0) {
			return i;
		}
	}//*/
	if (!srealloc(dllFuncts, sizeof(DllFunction) * (numDllFuncts+1))) return -2;
	memset(&dllFuncts[numDllFuncts], 0, sizeof(DllFunction));
	dllFuncts[numDllFuncts].dll = id;
	if ((argTypes && !(dllFuncts[numDllFuncts].args = strdup(argTypes))) ||
		!(dllFuncts[numDllFuncts].name = UTF8toASCIIAlloc(function))) {
			dllFuncts[numDllFuncts].Cleanup();
			return -2;
	}
	dllFuncts[numDllFuncts].returnType = returnType;
	return numDllFuncts++;
}

void CleanupDlls() {
	int i;
	for (i=0; i<numDllFuncts; i++) {
		dllFuncts[i].Cleanup();
	}
	free(dllFuncts);
	dllFuncts = 0;
	numDllFuncts = 0;

	for (i=0; i<numDlls; i++) {
		dlls[i].Cleanup();
	}
	free(dlls);
	dlls = 0;
	numDlls = 0;
}

void UnloadDll(ScriptValue &out, ScriptValue *args) {
	int id = FindDll(args[0].stringVal->value);
	out.type = SCRIPT_INT;
	if (id < 0) {
		out.intVal = -1;
		return;
	}
	if (dlls[id].hModule == 0) return;
	FreeLibrary(dlls[id].hModule);
	dlls[id].hModule = 0;
	for (int i=0; i<numDllFuncts; i++) {
		if (dllFuncts[i].dll == id) {
			dllFuncts[i].proc = 0;
		}
	}
	out.intVal = 1;
	return;
}

#ifndef X64
typedef float FloatType;
typedef int IntType;
#else
typedef double FloatType;
typedef __int64 IntType;
#endif

typedef size_t __stdcall  Args0();
typedef size_t __stdcall  Args1(size_t);
typedef size_t __stdcall  Args2(size_t, size_t);
typedef size_t __stdcall  Args3(size_t, size_t, size_t);
typedef size_t __stdcall  Args4(size_t, size_t, size_t, size_t);
typedef size_t __stdcall  Args5(size_t, size_t, size_t, size_t, size_t);
typedef size_t __stdcall  Args6(size_t, size_t, size_t, size_t, size_t, size_t);
typedef size_t __stdcall  Args7(size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef size_t __stdcall  Args8(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef size_t __stdcall  Args9(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef size_t __stdcall Args10(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);

typedef double __stdcall  fArgs0();
typedef double __stdcall  fArgs1(size_t);
typedef double __stdcall  fArgs2(size_t, size_t);
typedef double __stdcall  fArgs3(size_t, size_t, size_t);
typedef double __stdcall  fArgs4(size_t, size_t, size_t, size_t);
typedef double __stdcall  fArgs5(size_t, size_t, size_t, size_t, size_t);
typedef double __stdcall  fArgs6(size_t, size_t, size_t, size_t, size_t, size_t);
typedef double __stdcall  fArgs7(size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef double __stdcall  fArgs8(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef double __stdcall  fArgs9(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);
typedef double __stdcall fArgs10(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);

void SimpleCall(ScriptValue &out, ScriptValue &in, DllFunction *funct) {
	size_t params[10];
	ScriptValue cleanup[10];
	int numCleanup = 0;
	void *cleanup2[10];
	int numCleanup2 = 0;
	int numArgs = 0;
	memset(&params, 0, sizeof(params));
	int i;
	for (i=0; funct->args[i]; i++) {
		ScriptValue temp;
		if (in.type != SCRIPT_LIST || in.listVal->numVals < i) {
			continue;
			/*if (funct->args[i] == SCRIPT_STRING || funct->args[i] == SCRIPT_ASCII_STRING || funct->args[i] == SCRIPT_WIDE_STRING) {
				params[i] = (size_t)L"";
			}//*/
		}
		else if (funct->args[i] == SCRIPT_INT) {
			CoerceIntNoRelease(in.listVal->vals[i], temp);
			params[i] = (size_t)temp.intVal;
		}
		else if (funct->args[i] == SCRIPT_DOUBLE) {
			FloatType f;
			CoerceDoubleNoRelease(in.listVal->vals[i], temp);
			f = (FloatType) temp.doubleVal;
			params[i] = *(size_t*)&temp.intVal;
		}
		else {
			in.listVal->vals[i].AddRef();
			CoerceString(in.listVal->vals[i], temp);
			if (funct->args[i] == SCRIPT_STRING) {
				cleanup[numCleanup++] = temp;
				params[i] = (size_t) temp.stringVal->value;
			}
			else {
				if (funct->args[i] == SCRIPT_ASCII_STRING) {
					char *str = UTF8toASCIIAlloc(temp.stringVal->value);
					if (str) cleanup2[numCleanup2++] = str;
					params[i] = (size_t) str;
				}
				else if (funct->args[i] == SCRIPT_WIDE_STRING) {
					int len = temp.stringVal->len;
					wchar_t *str = UTF8toUTF16Alloc(temp.stringVal->value, &len);
					if (str) cleanup2[numCleanup2++] = str;
					params[i] = (size_t) str;
				}
			}
		}
	}
	numArgs = i;
	// Ugly, but compiles in both 32- and 64-bit mode.
	if (funct->returnType != SCRIPT_DOUBLE) {
		size_t res;
		if (numArgs == 10) {
			res = ((Args10*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9]);
		}
		else if (numArgs == 9) {
			res = ((Args9*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8]);
		}
		else if (numArgs == 8) {
			res = ((Args8*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
		}
		else if (numArgs == 7) {
			res = ((Args7*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
		}
		else if (numArgs == 6) {
			res = ((Args6*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5]);
		}
		else if (numArgs == 5) {
			res = ((Args5*)funct->proc)(params[0], params[1], params[2], params[3], params[4]);
		}
		else if (numArgs == 4) {
			res = ((Args4*)funct->proc)(params[0], params[1], params[2], params[3]);
		}
		else if (numArgs == 3) {
			res = ((Args3*)funct->proc)(params[0], params[1], params[2]);
		}
		else if (numArgs == 2) {
			res = ((Args2*)funct->proc)(params[0], params[1]);
		}
		else if (numArgs == 1) {
			res = ((Args1*)funct->proc)(params[0]);
		}
		else {
			res = ((Args0*)funct->proc)();
		}
		if (funct->returnType == SCRIPT_INT) {
			CreateIntValue(out, (IntType)res);
		}
		else if (funct->returnType == SCRIPT_NULL || !res) {
			CreateNullValue(out);
		}
		else {
			if (funct->returnType == SCRIPT_STRING) {
				CreateStringValue(out, (unsigned char*)res);
			}
			else if (funct->returnType == SCRIPT_WIDE_STRING) {
				CreateStringValue(out, (char*)res);
			}
			else if (funct->returnType == SCRIPT_ASCII_STRING) {
				CreateStringValue(out, (wchar_t*)res);
			}
		}
	}
	/* Floats are returned on the stack.
	 */
	else {
		double res;
		if (numArgs == 10) {
			res = ((fArgs10*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9]);
		}
		else if (numArgs == 9) {
			res = ((fArgs9*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8]);
		}
		else if (numArgs == 8) {
			res = ((fArgs8*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
		}
		else if (numArgs == 7) {
			res = ((fArgs7*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
		}
		else if (numArgs == 6) {
			res = ((fArgs6*)funct->proc)(params[0], params[1], params[2], params[3], params[4], params[5]);
		}
		else if (numArgs == 5) {
			res = ((fArgs5*)funct->proc)(params[0], params[1], params[2], params[3], params[4]);
		}
		else if (numArgs == 4) {
			res = ((fArgs4*)funct->proc)(params[0], params[1], params[2], params[3]);
		}
		else if (numArgs == 3) {
			res = ((fArgs3*)funct->proc)(params[0], params[1], params[2]);
		}
		else if (numArgs == 2) {
			res = ((fArgs2*)funct->proc)(params[0], params[1]);
		}
		else if (numArgs == 1) {
			res = ((fArgs1*)funct->proc)(params[0]);
		}
		else {
			res = ((fArgs0*)funct->proc)();
		}
		CreateDoubleValue(out, res);
	}


	for (i=0; i<numCleanup; i++) {
		cleanup[i].Release();
	}
	for (i=0; i<numCleanup2; i++) {
		free(cleanup2[i]);
	}
}

void ScriptCall(ScriptValue &out, ScriptValue &in, DllFunction *funct, Dll * dll) {
	ScriptVal v;
	memset(&v, 0, sizeof(v));
	v.type = SCRIPT_NULL_DLL;
	if (funct->args) {
		v.type = SCRIPT_LIST;
		v.listValue.numVals = (int)strlen(funct->args);
		v.listValue.vals = (ScriptVal*) calloc(v.listValue.numVals, sizeof(ScriptVal));
		if (v.listValue.vals) {
			v.flags = FLAG_NEED_FREE_INTERNAL;
			ScriptValue temp;
			for (int i=0; i<v.listValue.numVals; i++) {
				if (!funct->args[i]) break;
				if (funct->args[i] == SCRIPT_INT) {
					v.listValue.vals[i].type = SCRIPT_INT;
					if (in.listVal->numVals <= i) {
						v.listValue.vals[i].intValue = 0;
					}
					else {
						CoerceIntNoRelease(in.listVal->vals[i],  temp);
						v.listValue.vals[i].intValue = temp.intVal;
					}
				}
				else if (funct->args[i] == SCRIPT_DOUBLE) {
					v.listValue.vals[i].type = SCRIPT_DOUBLE;
					if (in.listVal->numVals <= i) {
						v.listValue.vals[i].doubleValue = 0;
					}
					else {
						CoerceDoubleNoRelease(in.listVal->vals[i],  temp);
						v.listValue.vals[i].doubleValue = temp.doubleVal;
					}
				}
				else {
					unsigned char *string;
					unsigned char needFree = 0;
					int len = 0;
					if (in.listVal->numVals <= i) {
						string = (unsigned char*)"";
					}
					else if (in.listVal->vals[i].type == SCRIPT_STRING) {
						string = in.listVal->vals[i].stringVal->value;
						len = in.listVal->vals[i].stringVal->len;
					}
					else {
						in.listVal->vals[i].AddRef();
						CoerceString(in.listVal->vals[i], temp);
						string = strdup(temp.stringVal->value);
						if (string) {
							needFree = FLAG_NEED_FREE_INTERNAL;
							len = temp.stringVal->len;
						}
						else string = (unsigned char*)"";
						temp.Release();
					}
					v.listValue.vals[i].type = funct->args[i];
					if (funct->args[i] == SCRIPT_STRING) {
						v.listValue.vals[i].stringValue.value = string;
						v.listValue.vals[i].stringValue.len = len;
						v.listValue.vals[i].flags = needFree;
					}
					else if (funct->args[i] == SCRIPT_ASCII_STRING) {
						if (needFree) {
							len = UTF8toASCII((char*)string, string);
						}
						else {
							len = 0;
							for (; string[len]; len++) {
								if (string[len] >= 0x80) break;
							}
							if (string[len]) {
								string = (unsigned char*)UTF8toASCIIAlloc(string);
								if (!string) {
									string = (unsigned char*)"";
									len = 0;
								}
								else {
									len = (int)strlen(string);
									needFree = FLAG_NEED_FREE_INTERNAL;
								}
							}
						}
						v.listValue.vals[i].asciiStringValue.value = (char*)string;
						v.listValue.vals[i].asciiStringValue.len = len;
						v.listValue.vals[i].flags = needFree;
					}
					else if (funct->args[i] == SCRIPT_WIDE_STRING) {
						int len = -1;
						wchar_t *out = UTF8toUTF16Alloc(string);
						if (needFree) free(string);
						if (!out) {
							out = L"";
							len = 0;
							needFree = 0;
						}
						else {
							len = (int) wcslen(out);
							needFree = FLAG_NEED_FREE_INTERNAL;
						}
						v.listValue.vals[i].wideStringValue.value = out;
						v.listValue.vals[i].wideStringValue.len = len;
						v.listValue.vals[i].flags = needFree;
					}
				}
			}
		}
	}
	else {
		ConvertValue(in, v);
	}
	if (v.type == SCRIPT_NULL_DLL || (v.type == SCRIPT_LIST && v.listValue.vals)) {
		ScriptVal out2;
		memset(&out2, 0, sizeof(out2));
		out2.type = SCRIPT_NULL;
		funct->proc(&v, &out2);
		ConvertValue(out2, out, dll->info.free);
	}
	FreePassedValues(v);
}

void CallDll(ScriptValue &out, ScriptValue &in, int id) {
	CreateNullValue(out);
	if (in.type == SCRIPT_LIST || in.type == -1) {
		DllFunction *funct = dllFuncts+id;
		Dll * dll = dlls+funct->dll;
		if (funct->proc == 0) {
			if (dll->hModule == 0) {
				wchar_t *name = UTF8toUTF16Alloc(dll->name);
				if (name) {
					dll->hModule = LoadLibraryExW(name, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
					if (!dll->hModule) {
						wchar_t *name2 = (wchar_t*)malloc((wcslen(name)+10)*sizeof(wchar_t));
						if (name2) {
							wsprintf(name2, L"dll\\%s", name);
							dll->hModule = LoadLibraryExW(name2, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
							if (!dll->hModule) {
								wsprintf(name2, L"dll\\%s.dll", name);
								dll->hModule = LoadLibraryExW(name2, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
							}
							if (!dll->hModule) {
								wsprintf(name2, L"%s.dll", name);
								dll->hModule = LoadLibraryExW(name2, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
							}
							free(name2);
						}
					}
					free(name);
				}
				if (!dll->hModule) {
					//errorPrintf("Couldn't load dll: %s\r\n", dll->name);
				}
				else if (dll->init) {
					InitDll init = (InitDll) GetProcAddress(dll->hModule, dll->init);
					if (!init) {
						FreeLibrary(dll->hModule);
						dll->hModule = 0;
						//errorPrintf("Couldn't find initialization function: %s\r\n", dll->name);
					}
					else {
						AppInfo info;
						unsigned char v[5] = VERSION;
						memcpy(info.version, v, sizeof(info.version));
						info.size = sizeof(AppInfo);
						info.maxDllVersion = 1;

						memset(&dll->info, 0, sizeof(DllInfo));
						dll->info.size = sizeof(DllInfo);
						init(&info, &dll->info);
						if (dll->info.dllVersion != 1) {
							FreeLibrary(dll->hModule);
							dll->hModule = 0;
							//errorPrintf("Initialization failed: %s\r\n", dll->name);
						}
					}
				}
			}
			if (dll->hModule) {
				funct->proc = (DllProc) GetProcAddress(dll->hModule, funct->name);
				//if (!funct->proc)
				//	errorPrintf("Cant find %s in %s\r\n", funct->name, dll->name);
			}
		}
		if (funct->proc) {
			if (funct->returnType) {
				SimpleCall(out, in, funct);
			}
			else {
				ScriptCall(out, in, funct, dll);
			}
		}
		else {
			errorPrintf(2, "Unable to find %s in %s\r\n", funct->name, dll->name);
		}
	}
	in.Release();
}
