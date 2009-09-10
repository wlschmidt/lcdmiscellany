#include "MutableString.h"
#include "../global.h"
#include "../unicode.h"
#include "ScriptObjects.h"

unsigned int MutableStringType;
unsigned int FileMappingType;
unsigned int MutableImageType;

void MyOpenFileMapping(ScriptValue &s, ScriptValue *args) {
	wchar_t *name =  UTF8toUTF16Alloc(args->stringVal->value);
	if (name) {
		HANDLE hShared = OpenFileMappingW(FILE_MAP_ALL_ACCESS, 0, name);
		if (hShared) {
			if (CreateObjectValue(s, FileMappingType)) {
				CreateIntValue(s.objectVal->values[0], (__int64)hShared);
			}
			else {
				CloseHandle(hShared);
			}
		}
	}
	free(name);
}

void MyFreeFileMapping(ScriptValue &s, ScriptValue *args) {
	if (s.objectVal->values[0].intVal) {
		CloseHandle((HANDLE)s.objectVal->values[0].intVal);
	}
}

void FileMappingMapViewOfFile(ScriptValue &s, ScriptValue *args) {
	__int64 offset = args[0].intVal;
	size_t size = (size_t) args[1].intVal;
	void *data = MapViewOfFile((HANDLE)s.objectVal->values[0].intVal, FILE_MAP_ALL_ACCESS, (DWORD)(offset>>32), (DWORD)offset, size);
	if (data) {
		ScriptValue fileMapping = s;
		if (CreateObjectValue(s, MutableStringType)) {
			CreateIntValue(s.objectVal->values[0], (__int64)data);
			CreateIntValue(s.objectVal->values[1], size);
			// 2 is reserved.  May allow creating read only things in the future.

			// Makes sure file mapping object is freed after this is.
			// Not actually necessary, but...
			s.objectVal->values[3] = fileMapping;
			fileMapping.AddRef();
			return;
		}
		UnmapViewOfFile(data);
	}
	CreateNullValue(s);
	//512*512*4
}

void FreeMutableString(ScriptValue &s, ScriptValue *args) {
	if (s.objectVal->values[3].type == SCRIPT_OBJECT && s.objectVal->values[3].objectVal->type == FileMappingType) {
		UnmapViewOfFile((void*)s.objectVal->values[0].intVal);
	}
}

void MutableStringReadInt(ScriptValue &s, ScriptValue *args) {
	int slen = (int) s.objectVal->values[1].intVal;
	unsigned char *str = (unsigned char*) s.objectVal->values[0].intVal;
	CreateNullValue(s);

	__int64 start = args[0].intVal;
	__int64 len = args[1].intVal;
	if (start >= slen || args[1].intVal <= 0 || start < 0) return;

	if (start+len > slen) {
		len = slen - start;
	}
	if (len <= 0) return;
	__int64 val = 0;
	if (args[2].intVal == 0) {
		val = (__int64)(((char*)str)[start+--len]);
	}
	for (int i = (int)len-1; i>=0; i--) {
		val = (val << 8) | str[start+i];
	}
	CreateIntValue(s, val);
}

void MutableStringReadInts(ScriptValue &s, ScriptValue *args) {
	int slen = (int)s.objectVal->values[1].intVal;
	unsigned char *str = (unsigned char*) s.objectVal->values[0].intVal;
	CreateNullValue(s);

	int pos = args[0].i32;
	int len = args[1].i32;
	int unSigned = args[2].i32;
	int count = args[3].i32;

	if (count <= 0 || len < 0 || len > 10000 || count > 100000) return;
	if (!CreateListValue(s, count)) return;
	for (int i=0; i<count; i++) {
		__int64 val = 0;
		int p = len-1;
		if (!unSigned && pos+p >= 0 && pos+p < slen) {
			val = (__int64)(char)str[pos+p];
			p--;
		}
		while (pos+p >= slen && p>=0) p--;
		while (p >= 0 && pos+p>=0) {
			val = (val<<8) + str[pos+p];
			p--;
		}
		while (p>=0) val <<=8;
		pos += len;
		s.listVal->PushBack(val);
	}
}

void MutableStringWriteInt(ScriptValue &s, ScriptValue *args) {
	__int64 val = args[0].intVal;
	__int64 offset = args[1].intVal;
	__int64 length = args[2].intVal;

	__int64 dlen = s.objectVal->values[1].intVal;
	unsigned char *data = (unsigned char*) s.objectVal->values[0].intVal;
	if (offset >= 0 && length >= 0 && offset < dlen) {
		if (offset + length > dlen) {
			length = dlen - offset;
		}
		while (length) {
			data[offset] = (unsigned char) val;
			val >>= 8;
			length --;
			offset ++;
		}
		CreateIntValue(s, 1);
	}
	else {
		CreateIntValue(s, 0);
	}
}

void MutableStringLoadImage(ScriptValue &s, ScriptValue *args) {
	int w = args[0].i32;
	int h = args[1].i32;
	int bpp = args[2].i32;
	int startOffset = args[3].i32;
	int memWidth = args[4].i32;

	__int64 len = s.objectVal->values[1].intVal;
	unsigned char *data = (unsigned char*) s.objectVal->values[0].intVal;

	ScriptValue sv = s;

	CreateNullValue(s);
	if (bpp != 8 && bpp != 24 && bpp != 32) return;

	if (startOffset < 0 || w < 0 || h < 0 || memWidth < 0) return;
	if (!memWidth) memWidth = w * bpp / 8;

	__int64 tm = memWidth * (__int64)h;

	if (tm != (int)(memWidth * h) || tm+startOffset > len) return;

	data += startOffset;
	if (CreateObjectValue(s, MutableImageType)) {
		CreateIntValue(s.objectVal->values[0], (__int64) data);
		CreateIntValue(s.objectVal->values[1], w);
		CreateIntValue(s.objectVal->values[2], h);
		CreateIntValue(s.objectVal->values[3], bpp);
		CreateIntValue(s.objectVal->values[4], memWidth);
		s.objectVal->values[5] = sv;
		sv.AddRef();
	}
}

void MutableImageSize(ScriptValue &s, ScriptValue *args) {
	ObjectValue *obj = s.objectVal;
	if (CreateListValue(s, 3)) {
		s.listVal->PushBack(obj->values[1]);
		s.listVal->PushBack(obj->values[2]);
		s.listVal->PushBack(obj->values[3].i32/8);
	}
}

