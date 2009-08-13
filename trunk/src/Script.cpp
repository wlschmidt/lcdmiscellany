#include "global.h"
#include "Script.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "TempCode.h"
#include "GenCode.h"
#include "util.h"
#include "Config.h"
#include "vm.h"
#include "ScriptProcs\\ScriptProcs.h"
#include "ScriptObjects\\ScriptObjects.h"
#include "ScriptObjects\\WMI.h"
#include "unicode.h"
#include "ScriptValue.h"

struct FileInfo {
	wchar_t *name;
	int loaded;
	int dep;
};
int numFiles = 0;
FileInfo *files = 0;

int AddFile(wchar_t *fileName, wchar_t *path, int &index) {
	index = 0;
	if (!fileName || !fileName[0]) return -2;
	wchar_t *name2 = wcsdup(fileName);
	if (!name2) return -2;

	int l = (int)wcslen(name2);
	/*int pos = 0;
	while (fileName[pos]) {
		if (fileName[pos] == '/') fileName[pos] = '\\'
		l++;
		pos++;
	}//*/

	if (fileName[0] == '<') {
		if (fileName[l-1] != '>') {
			return -2;
		}
		int l2 = 0;
		if (path) l2 = (int)wcslen(path);
		memcpy(name2, fileName+1, l * 2 - 4);
		name2[l-2] = 0;

		wchar_t *name = (wchar_t*) malloc(sizeof(wchar_t) * (l + 30));
		if (name) {
			wsprintf(name, L"Override\\%s", name2);
			if (GetFileAttributes(name) == INVALID_FILE_ATTRIBUTES) {
				wsprintf(name, L"Include\\%s", name2);
			}
			free(name2);
			name2=name;
			l = (int)wcslen(name2);
		}
		//wchar_t *test = ResolvePath(path, name2);
	}
	else {
		if (fileName[0] == '\"') {
			if (l<=3 || fileName[l-1] != '\"') {
				free(name2);
				return -2;
			}
			l-=2;
			fileName++;
			memcpy(name2, fileName, l * 2);
			name2[l] = 0;
		}
		if (name2[1] != ':' && name2[0] != '\\') {
			wchar_t *w = wcsrchr(path, '\\');
			int l2 = 1+(int)(w - path);
			if (!srealloc(name2, 2*(l2 + l + 2))) {
				free(name2);
				return -2;
			}
			memmove(name2 + l2, name2, 2*l+2);
			memcpy(name2, path, 2*l2);
			l+=l2+2;
		}
	}
	int q = GetFullPathName(name2, 0, 0, 0);
	wchar_t *nnew;
	if (q<=0 || !(nnew = (wchar_t*) malloc(q*sizeof(wchar_t)))) {
		free(name2);
		return -2;
	}
	int q2 = GetFullPathName(name2, q, nnew, 0);
	free(name2);
	if (q2 <= 0 || q2 >= q) {
		free(nnew);
		return -2;
	}
	name2 = nnew;
	q2 = GetLongPathName(nnew, 0, 0);
	if (q2 > 0 && (nnew = (wchar_t*)malloc(q2*sizeof(wchar_t)))) {
		if ((q = GetLongPathName(name2, nnew, q2)) >0 && q<q2) {
			free(name2);
			name2 = nnew;
		}
		else {
			free(nnew);
		}
	}
	/*
	//name2 = ResolvePathSteal(name2);
	if (name2[2] == '?') {
		l = (int) wcslen(name2);
		if (l < MAX_PATH+4)
			memmove(name2, name2 + 4, 2*(l - 3));
	}
	int i = GetLongPathName(name2, 0, 0);
	if (i <= 0 || !(fileName = (wchar_t*) malloc(i*2+2))) {
		free(name2);
		return -2;
	}
	int w = i;
	i = GetLongPathName(name2, fileName, w);
	free(name2);
	if (i+1 != w) {
		free(fileName);
		return -2;
	}
//*/
	int i;
	for (i=0; i<numFiles; i++) {
		if (!wcsicmp(files[i].name, name2)) break;
	}
	if (i != numFiles) {
		free(name2);
		index = i;
		return files[i].loaded;
	}
	if (srealloc(files, sizeof(FileInfo)*(numFiles+1))) {
		index = numFiles;
		files[numFiles].name = name2;
		files[numFiles++].loaded = -1;
		return -1;
	}
	free(name2);
	return -2;
}

ArgLists argLists;

int ArgLists::AddList(int id, const unsigned char*args) {
	if (numArgLists <= id) {
		if (!srealloc(argLists, (id+5)*sizeof(unsigned short)))
			return 0;
		for (int i = id+4; i>=numArgLists; i--) {
			argLists[i] = 0;
		}
		numArgLists = id+5;
	}
	int i = -2;
	argLists[id] = listLen;
	do {
		if (listLen%16 == 0 && !srealloc(list, sizeof(ArgListElement) * (listLen+16))) {
			return 0;
		}
		i += 2;
		list[listLen].type = args[i];
		list[listLen].count = args[i+1];
		listLen ++;
	}
	while (args[i+1]);
	return 1;
}

void ArgLists::Cleanup() {
	free(list);
	free(argLists);
}

int RegisterProc(unsigned char *name, ScriptProc *proc, FunctionType type) {
	int index = VM.StringTable.Add(name);
	if (index>=0) {
		TableEntry<Function> *e = VM.Functions.Find(VM.StringTable.table[index].stringVal);
		if (e) {
			return 0;
		}
		Function *f = &VM.Functions.Add(VM.StringTable.table[index].stringVal)->data;
		if (f) {
			VM.StringTable.table[index].stringVal->AddRef();
			f->address = proc;
			f->type = type;
		}
		return 1;
	}
	return 0;
}

int RegisterProcs() {
	if (-1 == (VM.StringTable.Add((unsigned char*)"") |
			   VM.StringTable.Add((unsigned char*)"null") |
			   VM.StringTable.Add((unsigned char*)"int") |
			   VM.StringTable.Add((unsigned char*)"float") |
			   VM.StringTable.Add((unsigned char*)"string") |
			   VM.StringTable.Add((unsigned char*)"list") |
			   VM.StringTable.Add((unsigned char*)"dict")))
			   return 0;
	VM.StringTable.table[1].stringVal->AddRef();
	if (!VM.Globals.Add(VM.StringTable.table[1].stringVal)) return 0;
	static const unsigned char lists[][9] = {
		{C_noArgs, 0, 0},
		// Not needed.
		//{C_noargsDraw, 0, 0},

		{C_4intsDraw, SCRIPT_INT, 4, 0, 0},
		{C_2intsDraw,SCRIPT_INT, 2, 0, 0},
		{C_intDraw,SCRIPT_INT, 1, 0, 0},
		{C_string4intsDraw,SCRIPT_STRING, 1, SCRIPT_INT, 4, 0, 0},

		{C_2doubles,SCRIPT_DOUBLE, 2, 0, 0},
		{C_double,SCRIPT_DOUBLE, 1, 0, 0},

		{C_3ints,SCRIPT_INT, 3, 0, 0},
		{C_4ints,SCRIPT_INT, 4, 0, 0},
		{C_5ints,SCRIPT_INT, 5, 0, 0},
		{C_2ints,SCRIPT_INT, 2, 0, 0},
		{C_int,SCRIPT_INT, 1, 0, 0},
		{C_string4ints,SCRIPT_STRING, 1, SCRIPT_INT, 4, 0, 0},
		{C_string5ints,SCRIPT_STRING, 1, SCRIPT_INT, 5, 0, 0},
		{C_stringintstring,SCRIPT_STRING, 1, SCRIPT_INT, 1, SCRIPT_STRING, 1, 0, 0},

		{C_2ints4strings, SCRIPT_INT, 2, SCRIPT_STRING, 4, 0, 0},

		{C_obj16doubles,SCRIPT_OBJECT, 1, SCRIPT_DOUBLE, 16, 0, 0},
		{C_obj5doubles,SCRIPT_OBJECT, 1, SCRIPT_DOUBLE, 5, 0, 0},
		{C_obj6ints,SCRIPT_OBJECT, 1, SCRIPT_INT, 6, 0, 0},
		{C_string3ints,SCRIPT_STRING, 1, SCRIPT_INT, 3, 0, 0},
		{C_string2ints,SCRIPT_STRING, 1, SCRIPT_INT, 2, 0, 0},
		{C_string2intsobj,SCRIPT_STRING, 1, SCRIPT_INT, 2, SCRIPT_OBJECT, 1, 0, 0},
		{C_2strings2ints,SCRIPT_STRING, 2, SCRIPT_INT, 2, 0, 0},
		{C_3strings3ints,SCRIPT_STRING, 3, SCRIPT_INT, 3, 0, 0},
		{C_2strings3ints,SCRIPT_STRING, 2, SCRIPT_INT, 3, 0, 0},
		{C_stringint,SCRIPT_STRING, 1, SCRIPT_INT, 1, 0, 0},
		{C_2strings,SCRIPT_STRING, 2, 0, 0},
		{C_2objs, SCRIPT_OBJECT, 2, 0, 0},
		{C_obj, SCRIPT_OBJECT, 1, 0, 0},
		{C_string2objs, SCRIPT_STRING, 1, SCRIPT_OBJECT, 2, 0, 0},
		{C_2stringsobj,SCRIPT_STRING, 2,SCRIPT_OBJECT,1, 0, 0},
		{C_2strings2objs,SCRIPT_STRING, 2,SCRIPT_OBJECT,2, 0, 0},
		{C_2stringsint,SCRIPT_STRING, 2,SCRIPT_INT,1, 0, 0},
		{C_2stringsintstring,SCRIPT_STRING, 2,SCRIPT_INT,1, SCRIPT_STRING,1,0, 0},

		{C_stringintobj, SCRIPT_STRING,1,SCRIPT_INT,1,SCRIPT_OBJECT,1,0,0},

		{C_stringobj,SCRIPT_STRING, 1, SCRIPT_OBJECT, 1, 0, 0},

		{C_intstring,SCRIPT_INT, 1,SCRIPT_STRING, 1, 0, 0},
		{C_3strings,SCRIPT_STRING, 3, 0, 0},
		{C_3stringsint,SCRIPT_STRING, 3, SCRIPT_INT, 1, 0, 0},
		{C_string,SCRIPT_STRING, 1, 0, 0},
		{C_double2ints,SCRIPT_DOUBLE, 1, SCRIPT_INT, 2, 0, 0},
		{C_double3ints,SCRIPT_DOUBLE, 1, SCRIPT_INT, 3, 0, 0},
		{C_stringdouble,SCRIPT_STRING, 1, SCRIPT_DOUBLE, 0, 0},
		{C_stringdoubleint,SCRIPT_STRING, 1, SCRIPT_DOUBLE, 1, SCRIPT_INT, 1, 0, 0},
		{C_doubleint, SCRIPT_DOUBLE, 1, SCRIPT_INT, 1, 0, 0},
	};

	for (int i=0; i<sizeof(lists)/sizeof(lists[0]); i++) {
		if (!argLists.AddList(lists[i][0], lists[i]+1)) return 0;
	}
	/*
	argLists[C_4intsDraw].Set(SCRIPT_INT, 4);
	argLists[C_2intsDraw].Set(SCRIPT_INT, 2);
	argLists[C_intDraw].Set(SCRIPT_INT, 1);
	argLists[C_string4intsDraw].Set(SCRIPT_STRING, 1, SCRIPT_INT, 4);

	argLists[C_2doubles].Set(SCRIPT_DOUBLE, 2);
	argLists[C_double].Set(SCRIPT_DOUBLE, 1);

	argLists[C_4ints].Set(SCRIPT_INT, 4);
	argLists[C_2ints].Set(SCRIPT_INT, 2);
	argLists[C_int].Set(SCRIPT_INT, 1);
	argLists[C_string4ints].Set(SCRIPT_STRING, 1, SCRIPT_INT, 4);
	argLists[C_stringintstring].Set(SCRIPT_STRING, 1, SCRIPT_INT, 1, SCRIPT_STRING, 1);

	argLists[C_obj6ints].Set(SCRIPT_OBJECT, 1, SCRIPT_INT, 6);
	argLists[C_string3ints].Set(SCRIPT_STRING, 1, SCRIPT_INT, 3);
	argLists[C_string2ints].Set(SCRIPT_STRING, 1, SCRIPT_INT, 2);
	argLists[C_stringint].Set(SCRIPT_STRING, 1, SCRIPT_INT, 1);
	argLists[C_2strings].Set(SCRIPT_STRING, 2);
	argLists[C_3strings].Set(SCRIPT_STRING, 3);
	argLists[C_string].Set(SCRIPT_STRING, 1);
	argLists[C_double2ints].Set(SCRIPT_DOUBLE, 1, SCRIPT_INT, 2);
	argLists[C_stringdouble].Set(SCRIPT_STRING, 1, SCRIPT_DOUBLE);
	argLists[C_stringdoubleint].Set(SCRIPT_STRING, 1, SCRIPT_DOUBLE, 1, SCRIPT_INT, 1);
	/*C_7ints,
	C_char3ints,
	C_2ints,
	C_noArgs,
	C_charint64,
	C_3chars,
	C_int,
	C_char,
	C_double2int,
	C_charDouble,
	*/

	int res = RegisterProcs2();
	return res;
}




int Compile(unsigned char* string, int len, unsigned char *fileName, wchar_t *fileNameW) {
	Token *tokens;
	int numTokens;
	int numErrors = Tokenize(string, len, tokens, numTokens, fileNameW);
	if (numErrors) {
		CleanupTokens(tokens, numTokens);
		return 0;
	}
/*	Token test[100];
	test[0].type = ONE;
	test[1].type = PLUS;
	test[2].type = ONE;
	test[3].type = DOLLAR;
//	TreeNode *t = ParseScript(test, 4);
//*/
	ParseTree *tree = ParseScript(tokens, numTokens);
	if (!tree) {
		return 0;
	}
	TempCode * tempCode = GenTempCode(tree);
	CleanupTree(tree);
	if (!tempCode) {
		return 0;
	}
	if (!GenCode(tempCode, string, len, fileName)) {
		return 0;
	}
	return 1;
}

int LoadScript(int i, int import) {
	// Already tried.
	if (files[i].loaded != -1) return 3;
	unsigned char *temp2 = UTF16toUTF8Alloc(files[i].name);
	if (!import)
		errorPrintf(2, "Compiling: %s\r\n", temp2);
	else
		errorPrintf(2, "Compiling (imported): %s\r\n", temp2);
	if (!temp2) {
		errorPrintf(2, "Error Loading: %s\r\n", temp2);
		errors++;
		return 0;
	}
	int res = 0;
	FileLineReader *reader = OpenLineReader(files[i].name, 0, WHOLE_FILE);
	if (reader) {
		unsigned char *data;
		int len = reader->NextLine(&data);
		files[i].loaded = 2;
		if (data) {
			res = Compile(data, len, temp2, files[i].name);
			files[i].loaded = res;
		}
		delete reader;
	}
	else {
		errorPrintf(2, "\r\nError Loading: %s\r\n\r\n", temp2);
		errors++;
		return 0;
	}
	//	free(temp);
	//}
	free(temp2);
	return res;
}

int LoadScripts() {
	int res = 1;
	for (int i=0; i<numFiles; i++) {
		if (files[i].loaded != -1) continue;
		int res2 = LoadScript(i);
		res &= res2;
	}
	return res;
}

int LoadScript(unsigned char* fileName) {
	wchar_t *temp2 = UTF8toUTF16Alloc(fileName);
	int res = -2;
	if (temp2) {
		wchar_t path2[2*MAX_PATH+2];
		int l3;
		l3 = GetModuleFileNameW(0, path2, 2*MAX_PATH+2);
		int index;
		res = AddFile(temp2, path2, index);
		free(temp2);
	}
	if (res == -2) {
		errorPrintf(2, "Error Loading: %s\r\n", fileName);
		errors++;
		return 0;
	}
	if (res != -1) return 1;
	return LoadScripts();
}

void CheckFxns() {
	int linkErrors = 0;
	for (int i=0; i<VM.Functions.numValues; i++) {
		if (VM.Functions.table[i].data.type == UNDEFINED) {
			if (!linkErrors) {
				linkErrors = 1;
				errorPrintf(2, "\nLink Warnings:\n");
			}
			errorPrintf(2, "Undefined function: %s\n", VM.Functions.table[i].sv->value);
			warnings++;
		}
	}
	if (linkErrors) {
		errorPrintf(2, "\n");
	}
}

void CleanupScripting() {
	int i;
	for (i=0; i<numFiles; i++) {
		free(files[i].name);
	}
	free(files);
	free(code.code);
	CleanupProcs();
	VM.Cleanup();
	CleanupCompiler();
	CleanupObjectTypes();
	argLists.Cleanup();
	//CleanupWMI();
}

void CleanupCompiler() {
	CleanupParser();
	CleanupDefines();
}
