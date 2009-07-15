#include "../global.h"
#include <wchar.h>
#include "File.h"
#include "ScriptObjects.h"
#include "../util.h"
#include "../unicode.h"
#include "../malloc.h"

#ifdef GOAT

int FileType;

void ResolvePath(ScriptValue &s, ScriptValue *args) {
	int t=0;
	while (t < args[0].stringVal->len && iswspace(args[0].stringVal->value[t])) t++;
	int e = args[0].stringVal->len-1;
	while (t <= e && iswspace(args[0].stringVal->value[e])) e--;

	int len = 1+e-t;
	wchar_t *utf;
	if (args[0].stringVal->len) {
		utf = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
	}
	else {
		utf = wcsdup(L".");
	}
	if (!utf) return;
	int r = GetFullPathName(utf, 0, 0, 0);
	wchar_t *temp;
	if (r <= 0 || !(temp = (wchar_t*) malloc(sizeof(wchar_t*)*r))) {
		free(utf);
		return;
	}
	int r2 = GetFullPathName(utf, r, temp, 0);
	free(utf);
	if (r2 >= r) {
		free(temp);
		return;
	}
	r2 = GetLongPathName(temp, 0, 0);
	if (r2 > 0 && (utf = (wchar_t*)malloc(r2*sizeof(wchar_t)))) {
		if ((r = GetLongPathName(temp, utf, r2)) >0 && r<r2) {
			free(temp);
			temp = utf;
		}
		else {
			free(utf);
		}
	}
	if (!CreateStringValue(s, temp, (int)wcslen(temp))) {
		CreateNullValue(s);
	}
	free(temp);
	
	/*
	j=0;
	wchar_t *t;
	if (t = wcschr(utf, ':')) {
		i = j = utf-t+1;
	}
	else if (utf[0] == '\\' && utf[1] == '\\') {
		j=i=2;
	}
	int s = i;
	for (; j<len; j++) {
		if (utf[j] == '\\') {
			if (utf[j+1] == '\\')
				continue;
			if (j+1 < len && utf[j+1] == '.') {
				if (j+2 == len || utf[j+2] == '\\') {
					j+=2;
					continue;
				}
				if (j+2 < len && utf[j+2] == '.') {
					if (j+3 == len || utf[j+3] == '\\') {
						int t = i;
						while (t && utf[t] != '\\' && utf[t-1] != ':') t--;
						if (i && utf[i-1] == '.') {
							if (i == 1 || utf[i-2] == '\\') {
								i--;
								continue;
							}
							if (utf[i-2] == '.' && utf[i-3] == '\\' && i >=4) {
								int t = i-4;
								while (t > 0 && utf[t] != '\\') t--;
								if (utf[t] == '\\') {
									i = t+1;
									continue;
								}
							}
						}
					}
				}
			}
		}
		utf[i++] = utf[j];
	}
	utf[i] = 0;
	len = i;
	free(utf);
	/*
	wchar_t path2[2*MAX_PATH+2];
	int l3;
	l3 = GetModuleFileNameW(0, path2, 2*MAX_PATH+2);
	while (l3>0 && path2[l3-1] != '\\') l3 --;
	while (l3-1>0 && path2[l3-2] == '\\') l3 --;
	path2[l3] = 0;
	l3 = (int)wcslen(path2);
	wchar_t *name3 = (wchar_t*) malloc(sizeof(wchar_t) *(l + l3 + 10));
	if (!name3) {
		free(name2);
		return -2;
	}
	wsprintf(name3, L"%s%s\\%s", path2, L"Override", name2);
	FILE *f = _wfopen(name3, L"rb");
	if (!f) {
		wsprintf(name3, L"%s%s\\%s", path2, L"Include", name2);
	}
	else fclose(f);
	/*
	if (args[0].stringVal->len >= 1 && args[0].stringVal->value[1] == ':') {
		s = args[0];
		s.stringVal->AddRef();
		return;
	}
	ResolvePath(
	//*/
}

#define SIZE 0
#define TYPE 1

void ListFiles(ScriptValue &s, ScriptValue *args) {
	if (!(args[1].i32&3)) return;
	ScriptValue path;
	ResolvePath(path, args);
	if (path.type != SCRIPT_STRING) return;
	if (path.stringVal->len == 0) {
		path.stringVal->Release();
		CreateNullValue(path);
		return;
	}
	if (path.stringVal->value[path.stringVal->len - 1] != '\\') {
		if (!ResizeStringValue(path, path.stringVal->len +1)) {
			path.stringVal->Release();
			CreateNullValue(path);
			return;
		}
		path.stringVal->value[path.stringVal->len-1] = '\\';
	}
	wchar_t *temp = (wchar_t *)alloca(sizeof(wchar_t)*(path.stringVal->len+11));
	if (temp) {
		int len = UTF8toUTF16(temp, s.stringVal->value, s.stringVal->len);
		if (len) {
			temp[len] = '*';
			temp[len+1] = 0;
			WIN32_FIND_DATA data;
			HANDLE hFind = FindFirstFile(temp, &data);
			if (hFind == INVALID_HANDLE_VALUE) return;
			if (CreateListValue(s, 10)) {
				do {
					if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (!(args[1].i32&2)) continue;
						if (data.cFileName[0] == '.' && !data.cFileName[1]) {
							continue;
						}
					}
					else {
						if (!(args[1].i32&1)) continue;
					}
					ScriptValue sv;
					if (!CreateObjectValue(sv, FileType)) continue;
					if (!CreateStringValue(sv.objectVal->values[0], data.cFileName, (int)wcslen(data.cFileName))) {
						sv.Release();
						continue;
					}
					sv.objectVal->values[1] = path;
					path.AddRef();
					CreateIntValue(sv.objectVal->values[2], data.nFileSizeLow + (((__int64)data.nFileSizeHigh) << 32));
					CreateIntValue(sv.objectVal->values[3], data.dwFileAttributes);
					s.listVal->PushBack(sv);
				}
				while (FindNextFile(hFind, &data));
			}
			FindClose(hFind);
		}
	}
	path.Release();
}

void FileSize(ScriptValue &s, ScriptValue *args) {
	s = args[0].objectVal->values[2];
}

void FileName(ScriptValue &s, ScriptValue *args) {
	s = args[0].objectVal->values[0];
	s.AddRef();
}

void FileIsDirectory(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, (args[0].objectVal->values[3].i32&FILE_ATTRIBUTE_DIRECTORY) != 0);
}

void FilePath(ScriptValue &s, ScriptValue *args) {
	s = args[0].objectVal->values[1];
	s.AddRef();
}
/*
void FileFullPath(ScriptValue &s, ScriptValue *args) {
	ObjectValue *o = s.objectVal;
	if (!AllocateStringValue(s, o->values[0].stringVal->len + o->values[1].stringVal->len)) {
		CreateNullValue(s);
		return;
	}
	memcpy(s.stringVal->value, o->values[1].stringVal->value, o->values[1].stringVal->len);
	memcpy(s.stringVal->value + o->values[1].stringVal->len, o->values[0].stringVal->value, o->values[0].stringVal->len);
}
//*/

#endif

#define READER_CONVERT_TEXT 3
#define READER_UNCOMPRESS 1
#define READER_NORMAL 0

int FileReaderType;
int FileWriterType;

void OpenFileReader(ScriptValue &s, ScriptValue *args) {
	if (!CreateObjectValue(s, FileReaderType)) {
		return;
	}
	ObjectValue *obj = s.objectVal;
	int len = args[0].stringVal->len;
	wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
	if (path) {
		int type = args[1].i32;
		void *f = 0;
		if (type == READER_CONVERT_TEXT) {
			f = OpenLineReader(path, 0, SINGLE_LINE);
		}
		else if (type == READER_UNCOMPRESS) {
			f = wreaderOpen(path);
		}
		else if (type == READER_NORMAL) {
			f = _wfopen(path, L"rb");
		}
		free(path);
		if (f) {
			CreateIntValue(obj->values[0], type);
			CreateIntValue(obj->values[1], (__int64)f);
			obj->values[2] = args[0];
			args[0].stringVal->AddRef();
			return;
		}
	}
	obj->Release();
	CreateNullValue(s);
}

void FileReaderRead(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = s.objectVal;
	CreateNullValue(s);
	int type = obj->values[0].i32;
	void *file = (void*)obj->values[1].intVal;
	unsigned int bytes = 0;
	int readLine = 0;
	if (args[0].intVal == -1 && type == READER_CONVERT_TEXT) {
		bytes = 1024*1024*10;
	}
	if (args[0].intVal>>31) {
		bytes = -1;
		readLine = 1;
	}
	else bytes = args[0].i32;
	int read = 0;

	int usedBytes = 0;
	while(bytes > 0) {
		int buffLen = 0;
		unsigned int sub = 1;
		char buff[8];
		if (type == READER_NORMAL) {
			buffLen = (1 == fread(&buff, 1, 1, (FILE*)file));
		}
		else if (type == READER_UNCOMPRESS) {
			buffLen = (1 == readerRead(&buff, 1, 1, (FileReader*)file));
		}
		else if (type == READER_CONVERT_TEXT) {
			unsigned char *v = (unsigned char*)obj->values[3].intVal;
			int len = obj->values[4].i32;
			if (!v) {
				len = ((FileLineReader*)file)->NextLine(&v);
				if (len < 0) {
					readLine = 0;
					break;
				}
				obj->values[4].intVal = len;
				obj->values[3].intVal = (__int64)v;
			}
			if (!len) {
				obj->values[3].intVal = 0;
				if (readLine) {
					break;
				}
				buff[0] = '\n';
				buffLen = 1;
			}
			else {
				// Not really needed.
				int clen=len;
				unsigned long c = NextUnicodeChar(v, &clen);
				// Shouldn't happen.
				if (clen > len) clen = len;
				obj->values[3].intVal += clen;
				obj->values[4].intVal -= clen;
				memcpy(buff, v, clen);
				buffLen = clen;
			}
		}
		if (!buffLen) {
			if (s.type == SCRIPT_NULL) return;
			break;
		}
		if (s.type == SCRIPT_NULL) {
			if (!AllocateStringValue(s, bytes + buffLen)) return;
		}
		else if (buffLen + usedBytes > s.stringVal->len) {
			if (s.stringVal->len >= 0x30000000 || !ResizeStringValue(s, 2*s.stringVal->len + buffLen)) return;
		}
		memcpy(s.stringVal->value+usedBytes, buff, buffLen);
		usedBytes += buffLen;
		bytes -= sub;
	}
	if (usedBytes) {
		ResizeStringValue(s, usedBytes);
	}
	else if (readLine) {
		CreateStringValue(s);
	}
}

void CleanupFileReader(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = s.objectVal;
	int type = obj->values[0].i32;
	void *file = (void*)obj->values[1].intVal;
	if (type == READER_NORMAL) {
		fclose((FILE*)file);
	}
	else if (type == READER_UNCOMPRESS) {
		readerClose((FileReader*)file);
	}
	else if (type == READER_CONVERT_TEXT) {
		delete (FileLineReader*)file;
	}
}

#define WRITER_RAW		0
#define WRITER_UTF8		1
#define WRITER_ASCII	2

#define WRITER_APPEND	8

void OpenFileWriter(ScriptValue &s, ScriptValue *args) {
	if (!CreateObjectValue(s, FileWriterType)) {
		return;
	}
	ObjectValue *obj = s.objectVal;
	int len = args[0].stringVal->len;
	wchar_t *path = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
	if (path) {
		int type = args[1].i32;
		FILE *f = 0;
		int subtype = type & 0x7;
		int append = type & WRITER_APPEND;
		if (type == WRITER_RAW || type == WRITER_ASCII) {
			if (append) f = _wfopen(path, L"ab");
			else f = _wfopen(path, L"wb");
		}
		else if (type == WRITER_UTF8) {
			if (append && (f = _wfopen(path, L"rb"))) {
				fclose(f);
				f = _wfopen(path, L"ab");
			}
			else {
				f = _wfopen(path, L"wb");
				if (f) {
					const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
					fwrite(bom,1,3,f);
				}
			}
		}
		free(path);
		if (f) {
			CreateIntValue(obj->values[0], type);
			CreateIntValue(obj->values[1], (__int64)f);
			obj->values[2] = args[0];
			args[0].stringVal->AddRef();
			return;
		}
	}
	obj->Release();
	CreateNullValue(s);
}

void FileWriterWrite(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = s.objectVal;
	CreateNullValue(s);
	int type = obj->values[0].i32;
	FILE *file = (FILE*)obj->values[1].intVal;
	int subtype = type & 0x7;
	if (!args[0].stringVal->len) {
		CreateIntValue(s, 1);
		return;
	}
	if (type == WRITER_RAW || type == WRITER_UTF8) {
		if (args[0].stringVal->len == fwrite(args[0].stringVal->value, 1, args[0].stringVal->len, file)) {
			CreateIntValue(s, args[0].stringVal->len);
		}
	}
	else if (type == WRITER_ASCII) {
		int len = args[0].stringVal->len;
		wchar_t *temp = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
		if (temp) {
			int p, j;
			for (p=0; p<len; p++) {
				if (temp[p] < 0x80) {
					if (temp[p] != fputc(temp[p],file)) break;
				}
				else {
					for (j = 0x80; j <= 0xFF; j++) {
						if (windows1252Map[j-0x80] == temp[p]) {
							if (j != fputc(j,file)) {
								free(temp);
								return;
							}
						}
					}
					if (j == 0x100) {
						if ('?' != fputc('?',file)) break;
					}
				}
			}
			free(temp);
			if (p == len) {
				CreateIntValue(s, len);
			}
		}
	}
}

void CleanupFileWriter(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = s.objectVal;
	int type = obj->values[0].i32;
	FILE *file = (FILE*)obj->values[1].intVal;
	if (file) fclose(file);
}

