#include "ScriptStringParser.h"
#include "../../malloc.h"
#include "../../unicode.h"
#include "../../stringUtil.h"
#include "../../config.h"
#include <ctype.h>

void ScriptToUpper(ScriptValue &s, ScriptValue *args) {
	int len = args[0].stringVal->len;
	wchar_t *w = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
	if (w) {
		for (int i=0; i<len; i++) {
			w[i] = towupper(w[i]);
		}
		CreateStringValue(s, w, len);
		free(w);
	}
}

void ScriptToLower(ScriptValue &s, ScriptValue *args) {
	int len = args[0].stringVal->len;
	wchar_t *w = UTF8toUTF16Alloc(args[0].stringVal->value, &len);
	if (w) {
		for (int i=0; i<len; i++) {
			w[i] = towlower(w[i]);
		}
		CreateStringValue(s, w, len);
		free(w);
	}
}

void ParseBinaryInt(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	unsigned char *str = args[0].stringVal->value;
	__int64 len = args[2].intVal;
	if (args[1].intVal == args[0].stringVal->len) return;
	int start = args[1].i32;
	if (start+len > args[0].stringVal->len) {
		len = args[0].stringVal->len - start;
	}
	if (len <= 0) return;
	__int64 val = 0;
	if (args[3].intVal == 0) {
		val = (__int64)(((char*)str)[start+--len]);
	}
	for (int i = (int)len-1; i>=0; i--) {
		val = (val << 8) | str[start+i];
	}
	CreateIntValue(s, val);
}

void ParseBinaryInts(ScriptValue &s, ScriptValue *args) {
	if (args[4].i32 <= 0 || args[2].intVal<0 || args[2].intVal > 10000) return;
	if (!CreateListValue(s, args[4].i32)) return;
	int pos = args[1].i32;
	int len = args[2].i32;
	int unSigned = args[3].i32;
	int count = args[4].i32;
	unsigned char *str = args[0].stringVal->value;
	int strLen = args[0].stringVal->len;
	for (int i=0; i<count; i++) {
		__int64 val = 0;
		int p = len-1;
		if (!unSigned && pos+p >= 0 && pos+p < strLen) {
			val = (__int64)(char)str[pos+p];
			p--;
		}
		while (pos+p >= strLen && p>=0) p--;
		while (p >= 0 && pos+p>=0) {
			val = (val<<8) + str[pos+p];
			p--;
		}
		while (p>=0) val <<=8;
		pos += len;
		s.listVal->PushBack(val);
	}
}

void ParseBinaryIntReverse(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	unsigned char *str = args[0].stringVal->value;
	__int64 len = args[2].intVal;
	if (args[1].intVal >= args[0].stringVal->len) return;
	int start = args[1].i32;
	if (start+len > args[0].stringVal->len) len = args[0].stringVal->len - start;
	if (len <= 0) return;
	s.type = SCRIPT_INT;
	if (args[3].intVal == 0) {
		s.intVal = (__int64)(((char*)str)[start++]);
		len--;
	}
	for (int i = 0; i<len; i++) {
		s.intVal = (s.intVal << 8) | str[start+i];
	}
}

//void ParseBinartDouble(ScriptValue &s, ScriptValue *args);

void ParseBinaryFloat(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal < 0) return;
	if (args[2].intVal == 4) {
		if (args[1].intVal > args[0].stringVal->len + 4) return;
		s.type = SCRIPT_DOUBLE;
		int t = *(int*)&args[0].stringVal->value[args[1].intVal];
		s.doubleVal = *((float*)&t);
		return;
	}
	if (args[2].intVal == 8) {
		if (args[1].intVal > args[0].stringVal->len + 8) return;
		s.type = SCRIPT_DOUBLE;
		__int64 t = *(__int64*)&args[0].stringVal->value[args[1].intVal];
		s.doubleVal = *((double*)&t);
		return;
	}
}

void ParseBinaryStringUTF8(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	int len;
	if (args[2].intVal + args[1].intVal > args[0].stringVal->len) {
		len = (int) (args[0].stringVal->len - args[1].intVal);
	}
	else {
		len = (int) args[2].intVal;
	}
	if (len <= 0) return;
	unsigned char *str = args[0].stringVal->value+args[1].intVal;
	for (int i=0; i<len;i++) {
		if (str[i] == 0) {
			len = i;
			break;
		}
	}
	CreateStringValue(s, str, len);
}

void ParseBinaryStringASCII(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	int len;
	if (args[2].intVal + args[1].intVal > args[0].stringVal->len) {
		len = (int) (args[0].stringVal->len - args[1].intVal);
	}
	else {
		len = (int) args[2].intVal;
	}
	if (len <= 0) return;
	char *str = (char*)(args[0].stringVal->value+args[1].intVal);
	for (int i=0; i<len;i++) {
		if (str[i] == 0) {
			len = i;
			break;
		}
	}
	unsigned char *out = (unsigned char*) malloc((len*2+1) * sizeof(unsigned char));
	if (out) {
		len = ASCIItoUTF8(out, str, len);
		CreateStringValue(s, out, len);
		free(out);
	}
}

void ParseBinaryStringUTF16(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	int len;
	if (args[2].intVal + args[1].intVal > args[0].stringVal->len) {
		len = (int) (args[0].stringVal->len - args[1].intVal);
	}
	else {
		len = (int) args[2].intVal;
	}
	if (len <= 0) return;
	wchar_t *str = (wchar_t*)(args[0].stringVal->value+args[1].intVal);
	len /= 2;
	for (int i=0; i<len;i++) {
		if (str[i] == 0) {
			len = i;
			break;
		}
	}
	unsigned char *out = UTF16toUTF8Alloc(str, &len);
	if (out) {
		CreateStringValue(s, out, len);
		free(out);
	}
}

void ParseBinaryBinary(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal > args[0].stringVal->len || args[2].intVal <= 0 || args[1].intVal < 0) return;
	int len;
	if (args[2].intVal + args[1].intVal > args[0].stringVal->len) {
		len = (int) (args[0].stringVal->len - args[1].intVal);
	}
	else {
		len = (int) args[2].intVal;
	}
	if (len <= 0) return;
	unsigned char *str = args[0].stringVal->value+args[1].intVal;
	CreateStringValue(s, str, len);
}


void UTF8toASCII(ScriptValue &s, ScriptValue *args) {
	if (!AllocateStringValue(s, args[0].stringVal->len)) return;
	int len = UTF8toASCII((char*)s.stringVal->value, args[0].stringVal->value, args[0].stringVal->len);
	if (!ResizeStringValue(s, len)) {
		s.stringVal->Release();
		CreateNullValue(s);
	}
}

void UTF8toUTF16(ScriptValue &s, ScriptValue *args) {
	if (!AllocateStringValue(s, 2+args[0].stringVal->len*2)) return;
	UTF8toUTF16((wchar_t*)s.stringVal->value, args[0].stringVal->value, args[0].stringVal->len);
	int len = (int)wcslen((wchar_t*)s.stringVal->value);
	if (!ResizeStringValue(s, len*2+1)) {
		s.stringVal->Release();
		CreateNullValue(s);
	}
	// This gives me two terminating nulls.
	s.stringVal->len = len*2;
	s.stringVal->value[len*2] = 0;
}

void UTF16toUTF8(ScriptValue &s, ScriptValue *args) {
	if (!AllocateStringValue(s, args[0].stringVal->len*2)) return;
	int len = UTF16toUTF8(s.stringVal->value, (wchar_t*)args[0].stringVal->value, (args[0].stringVal->len+1)/2);
	if (!ResizeStringValue(s, len)) {
		s.stringVal->Release();
		CreateNullValue(s);
	}
}

void ASCIItoUTF8(ScriptValue &s, ScriptValue *args) {
	if (!AllocateStringValue(s, 2*args[0].stringVal->len)) return;
	int len = ASCIItoUTF8(s.stringVal->value, (char*)args[0].stringVal->value, args[0].stringVal->len);
	if (!ResizeStringValue(s, len)) {
		s.stringVal->Release();
		CreateNullValue(s);
	}
}

void SaveString(ScriptValue &s, ScriptValue *args) {
	CreateIntValue(s, config.SaveConfigString(args[0].stringVal->value, args[1].stringVal, args[2].stringVal, args[3].i32));
}

void GetString(ScriptValue &s, ScriptValue *args) {
	StringValue *str = config.GetConfigString(args[0].stringVal->value, args[1].stringVal, args[2].stringVal);
	if (str) {
		str->AddRef();
		CreateStringValue(s, str);
	}
}

void MyStrstr(ScriptValue &s, ScriptValue *args) {
	if ((unsigned __int64)args[2].intVal >= (unsigned int)args[0].stringVal->len) return;
	int start = args[2].i32;
	int end = args[0].stringVal->len - args[1].stringVal->len;
	if (args[3].intVal) {
		if (args[3].intVal < args[1].stringVal->len)
			return;
		int d = (int)args[3].intVal - args[1].stringVal->len;
		if (d < end)
			end = d;
	}
	int l = args[1].stringVal->len;
	for (int i=start; i<=end; i++) {
		int j;
		for (j=0; j<l; j++) {
			if (args[0].stringVal->value[i+j] != args[1].stringVal->value[j]) break;
		}
		if (j == l) {
			CreateIntValue(s, i);
			return;
		}
	}
}

void MyStristr(ScriptValue &s, ScriptValue *args) {
	if ((unsigned __int64)args[2].intVal >= (unsigned int)args[0].stringVal->len) return;
	int start = args[2].i32;
	int end = args[0].stringVal->len - args[1].stringVal->len;
	if (args[3].intVal) {
		if (args[3].intVal < args[1].stringVal->len)
			return;
		int d = (int)args[3].intVal - args[1].stringVal->len;
		if (d < end)
			end = d;
	}
	int l = args[1].stringVal->len;
	int l2 = args[0].stringVal->len;
	for (int i=start; i<=end;) {
		int p1=i, p2=0;
		while(1) {
			int len;
			unsigned long c1 = NextUnicodeChar(args[0].stringVal->value + p1, &len);
			p1 += len;
			unsigned long c2 = NextUnicodeChar(args[1].stringVal->value + p2, &len);
			p2 += len;

			if (towupper((wchar_t)c1) != towupper((wchar_t)c2)) {
				p2 = 0;
				break;
			}
			if (p2 == l) break;
			if (p1 == l2) break;
		}
		if (p2 == l) {
			CreateIntValue(s, i);
			return;
		}
		int len;
		unsigned long c1 = NextUnicodeChar(args[0].stringVal->value + i, &len);
		i += len;
	}
}

void strsplit(ScriptValue &s, ScriptValue *args) {
	if ((unsigned __int64)args[2].intVal >= (unsigned int)args[0].stringVal->len || args[1].stringVal->len == 0) return;
	int start = args[2].i32;
	int end = args[0].stringVal->len - args[1].stringVal->len;
	if (args[3].intVal) {
		if (args[3].intVal < args[1].stringVal->len)
			return;
		int d = (int)args[3].intVal - args[1].stringVal->len;
		if (d < end)
			end = d;
	}
	int l = args[1].stringVal->len;
	if (start>end || !CreateListValue(s, 2)) return;
	ScriptValue sv;
	for (int i=start; i<=end; i++) {
		int j;
		for (j=0; j<l; j++) {
			if (args[0].stringVal->value[i+j] != args[1].stringVal->value[j]) break;
		}
		if (j == l) {
			int len = i-start;
			if (len || args[4].i32) {
				if (CreateStringValue(sv, args[0].stringVal->value+start, len)) {
					if (!s.listVal->PushBack(sv)) {
						sv.stringVal->Release();
					}
				}
			}
			start = i+j;
			i = start - 1;
		}
	}
	int len = end + l - start;
	if (len || args[4].i32) {
		if (CreateStringValue(sv, args[0].stringVal->value+start, len)) {
			if (!s.listVal->PushBack(sv)) {
				sv.stringVal->Release();
			}
		}
	}
}

void GetChar(ScriptValue &s, ScriptValue *args) {
	if (args[1].intVal < 0 || args[0].stringVal->len <= args[1].intVal) return;
	unsigned int pos = args[1].i32;
	int len = args[0].stringVal->len-(int)pos;
	unsigned long c = NextUnicodeChar(args[0].stringVal->value+pos, &len);
	if (!len) return;
	ScriptValue sv;
	if (!CreateStringValue(sv, args[0].stringVal->value+pos, len)) {
		return;
	}
	if (!CreateListValue(s, 2)) {
		sv.stringVal->Release();
		return;
	}
	s.listVal->PushBack(sv);
	CreateIntValue(sv, len);
	s.listVal->PushBack(sv);
}

void strreplace(ScriptValue &s, ScriptValue *args) {
	StringValue *src = args[0].stringVal;
	StringValue *sub = args[1].stringVal;
	StringValue *rep = args[2].stringVal;
	int count = args[3].i32;
	int start = args[4].i32;
	int end = args[5].i32;
	int replaceCount = 0;
	int *replace = 0;
	int replaceSize = 0;
	unsigned char *srcv = src->value;
	unsigned char *subv = sub->value;
	if (end == 0) end = src->len;
	if (!(!sub->len || !src->len || count < 0 || start < 0 || end < 0 || end<start || (end && end < start || start >= src->len))) {
		if (count == 0) count = src->len;
		else if ((unsigned int)end > (unsigned int)src->len) end = src->len;
		end -= sub->len;
		int l = sub->len;
		for (int i=start; i<=end; i++) {
			int j;
			for (j=0; j<l; j++) {
				if (srcv[i+j] != subv[j]) break;
			}
			if (j == l) {
				if (replaceSize == replaceCount) {
					if (!srealloc(replace, sizeof(int) * (replaceSize*2+2))) continue;
					replaceSize = replaceSize*2+2;
				}
				replace[replaceCount++] = i;
				i += sub->len-1;
				count--;
				if (count == 0) break;
			}
		}
	}
	__int64 test = src->len + (rep->len - sub->len) * (__int64) replaceCount;
	if (!replaceCount || test > 1024*1024*120 || !AllocateStringValue(s, (int) test)) {
		src->AddRef();
		free(replace);
		s = args[0];
		return;
	}
	int pos = 0;
	int pos2 = 0;
	for (int i=0; i<replaceCount; i++) {
		memcpy(s.stringVal->value + pos2, srcv + pos, replace[i]-pos);
		pos2 += replace[i]-pos;
		pos = replace[i];

		memcpy(s.stringVal->value + pos2, rep->value, rep->len);
		pos2 += rep->len;
		pos += sub->len;
	}
	free(replace);
	memcpy(s.stringVal->value + pos2, srcv+pos, src->len - pos);
}

void strinsert(ScriptValue &s, ScriptValue *args) {
	StringValue *src = args[0].stringVal;
	StringValue *add = args[1].stringVal;
	int pos = args[2].i32;

	if (pos < 0 || pos > src->len) pos = src->len;
	if (add->len == 0) {
		src->AddRef();
		s = args[0];
	}
	else if (src->len == 0) {
		add->AddRef();
		s = args[1];
	}
	else if (AllocateStringValue(s, add->len + src->len)) {
		memcpy(s.stringVal->value, src->value, pos);
		memcpy(s.stringVal->value+pos, add->value, add->len);
		memcpy(s.stringVal->value+pos+add->len, src->value+pos, src->len-pos);
	}
}

void strswap(ScriptValue &s, ScriptValue *args) {
	StringValue *src = args[0].stringVal;
	StringValue *add = args[1].stringVal;
	int start = args[2].i32;
	int end = args[3].i32;
	if (CreateListValue(s, 2)) {
		int happy = 0;
		if (end < 0 || end > src->len) end = src->len;
		if (start < 0 || start > src->len) start = src->len;
		if (start > end) {
			start = end = 0;
		}

		ScriptValue sv, sv2;
		if (start <= end) {
			if (!add->len && start == end) {
				sv = args[0];
				src->AddRef();
				CreateStringValue(sv2);
				happy = 1;
			}
			else if (!add->len && start == 0 && end == src->len) {
				sv2 = args[0];
				src->AddRef();
				CreateStringValue(sv);
				happy = 1;
			}
			else if (AllocateStringValue(sv, src->len - (end-start) + add->len)) {
				if (!AllocateStringValue(sv2, end-start)) {
					s.Release();
				}
				else {
					memcpy(sv2.stringVal->value, src->value+start, end-start);
					memcpy(sv.stringVal->value, src->value, start);
					memcpy(sv.stringVal->value+start, add->value, add->len);
					memcpy(sv.stringVal->value+start+add->len, src->value+end, src->len - end);
					happy = 1;
				}
			}
		}
		if (!happy) {
			s.Release();
			CreateNullValue(s);
		}
		else {
			s.listVal->PushBack(sv);
			s.listVal->PushBack(sv2);
		}
	}
}

void substring(ScriptValue &s, ScriptValue *args) {
	StringValue *src = args[0].stringVal;
	int start = args[1].i32;
	int end = args[2].i32;
	if (end < 0 || end > src->len) end = src->len;
	if (start < 0 || start > src->len) start = src->len;
	if (start >= end) {
		start = end = 0;
	}

	if (AllocateStringValue(s, end-start)) {
		memcpy(s.stringVal->value, src->value+start, end-start);
	}
}

