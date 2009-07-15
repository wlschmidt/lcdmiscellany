#include <stdlib.h>
#include <stdio.h>
#include "BencodeScript.h"
#include "../../stringUtil.h"
#include "../../unicode.h"

int GetIntLength(__int64 i) {
	int len=1;
	if (i<0) {
		len++;
		i=-i;
	}
	while (i/=10) len++;
	return len;
}

int GetLength(ScriptValue *sv, int strict) {
	if (sv->type == SCRIPT_NULL ||
		sv->type == SCRIPT_OBJECT) {
		if (!strict) return 1;
		return 2;
	}
	else if (sv->type == SCRIPT_INT) {
		return 2 + GetIntLength(sv->intVal);
	}
	else if (sv->type == SCRIPT_STRING) {
		return 1 + sv->stringVal->len + GetIntLength(sv->stringVal->len);
	}
	else if (sv->type == SCRIPT_LIST) {
		int len = 2;
		for (int i=0; i<sv->listVal->numVals; i++) {
			len += GetLength(sv->listVal->vals+i, strict);
		}
		return len;
	}
	else if (sv->type == SCRIPT_DICT) {
		int len = 2;
		for (int i=0; i<sv->dictVal->numEntries; i++) {
			len += GetLength(&sv->dictVal->entries[i].key, strict);
			len += GetLength(&sv->dictVal->entries[i].val, strict);
		}
		return len;
	}
	else if (sv->type == SCRIPT_DOUBLE) {
		if (strict) {
			char temp[30];
			_gcvt(sv->doubleVal, 14, temp);
			int len = (int)strlen(temp);
			return 1 + len + GetIntLength(len);
		}
		return 1+8;
	}
	else
		return 0;
}

void BencodeData(StringValue *s, ScriptValue *sv, int strict, int *pos) {
	if (sv->type == SCRIPT_NULL ||
		sv->type == SCRIPT_OBJECT) {
			if (!strict) {
				s->value[pos[0]] = 'n';
				pos[0]++;
			}
			else {
				s->value[pos[0]] = '0';
				s->value[pos[0]+1] = ':';
				pos[0]+=2;
			}
	}
	else if (sv->type == SCRIPT_INT) {
		s->value[pos[0]++] = 'i';
		_i64toa(sv->intVal, (char*)s->value+pos[0], 10);
		pos[0] += (int)strlen(s->value+pos[0]);
		s->value[pos[0]++] = 'e';
	}
	else if (sv->type == SCRIPT_STRING) {
		itoa(sv->stringVal->len, (char*)s->value+pos[0], 10);
		pos[0] += (int)strlen(s->value+pos[0]);
		s->value[pos[0]++] = ':';
		memcpy(s->value+pos[0], sv->stringVal->value, sv->stringVal->len);
		pos[0] += sv->stringVal->len;
	}
	else if (sv->type == SCRIPT_LIST) {
		s->value[pos[0]++] = 'l';
		for (int i=0; i<sv->listVal->numVals; i++) {
			BencodeData(s, sv->listVal->vals+i, strict, pos);
		}
		s->value[pos[0]++] = 'e';
	}
	else if (sv->type == SCRIPT_DICT) {
		s->value[pos[0]++] = 'd';
		//sv->dictVal->Sort();
		for (int i=0; i<sv->dictVal->numEntries; i++) {
			BencodeData(s, &sv->dictVal->entries[i].key, strict, pos);
			BencodeData(s, &sv->dictVal->entries[i].val, strict, pos);
		}
		s->value[pos[0]++] = 'e';
	}
	else if (sv->type == SCRIPT_DOUBLE) {
		if (strict) {
			char temp[30];
			_gcvt(sv->doubleVal, 14, temp);
			sprintf((char*)s->value + pos[0], "%i:%s", strlen(temp), temp);
			pos[0] += (int) strlen(s->value+pos[0]);
		}
		else {
			s->value[pos[0]++] = 'f';
			memcpy(s->value+pos[0], &sv->doubleVal, 8);
			pos[0]+=8;
		}
	}
}

void Bencode(ScriptValue *sv, int strict, ScriptValue &sv2) {
	int len = GetLength(sv, strict);
	if (AllocateStringValue(sv2, len)) {
		int pos = 0;
		BencodeData(sv2.stringVal, sv, strict, &pos);
	}
}

void Bencode(ScriptValue &s, ScriptValue *args) {
	if (args[0].type != SCRIPT_LIST) return;
	ScriptValue *sv;
	ScriptValue sv2;
	if (args[0].listVal->numVals == 0) {
		CreateNullValue(sv2);
		sv = &sv2;
	}
	else sv = &args[0].listVal->vals[0];
	Bencode(sv, 1, s);
}

void BencodeExact(ScriptValue &s, ScriptValue *args) {
	if (args[0].type != SCRIPT_LIST) return;
	ScriptValue *sv;
	ScriptValue sv2;
	if (args[0].listVal->numVals == 0) {
		CreateNullValue(sv2);
		sv = &sv2;
	}
	else sv = &args[0].listVal->vals[0];
	Bencode(sv, 0, s);
}

int ReadInt(unsigned char *s, unsigned char *end, __int64 &val) {
	val = 0;
	if (!end || s >= end) return 0;
	int sign = 1;
	if (*s == '-') {
		sign = -1;
		s++;
	}
	if (*s <= '0' || *s > '9') return 0;
	while (s < end && *s >= '0' && *s <= '9') {
		val = 10 * val + *s-'0';
		s++;
	}
	val *= sign;
	return (s==end);
}

int BeDecode(ScriptValue &sv, StringValue *s, int *pos) {
	if (s->len < pos[0]) return 0;
	if (s->value[pos[0]] == 'i') {
		pos[0] ++;
		unsigned char *end = strchr(s->value + pos[0], 'e');
		__int64 val;
		if (!ReadInt(s->value + pos[0], end, val)) return 0;
		CreateIntValue(sv, val);
		pos[0] = 1+(int)(end-s->value);
	}
	else if (s->value[pos[0]] == 'l') {
		pos[0] ++;
		CreateListValue(sv, 3);
		while (s->len > pos[0]) {
			if (s->value[pos[0]] == 'e') {
				pos[0] ++;
				return 1;
			}
			ScriptValue sv2;
			if (!BeDecode(sv2, s, pos) || !sv.listVal->PushBack(sv2)) {
				sv.Release();
				return 0;
			}
		}
		sv.Release();
		return 0;
	}
	else if (s->value[pos[0]] == 'd') {
		pos[0] ++;
		CreateDictValue(sv, 3);
		while (s->len > pos[0]) {
			if (s->value[pos[0]] == 'e') {
				pos[0] ++;
				return 1;
			}
			ScriptValue sv2, sv3;
			if (!BeDecode(sv2, s, pos)) {
				sv.Release();
				return 0;
			}
			if (sv2.type != SCRIPT_STRING || !BeDecode(sv3, s, pos)) {
				sv2.Release();
				sv.Release();
				return 0;
			}
			if (!sv.dictVal->Add(sv, sv2)) {
				sv.Release();
				return 0;
			}
		}
		sv.Release();
		return 0;
	}
	else if (s->value[pos[0]] == 'n') {
		CreateNullValue(sv);
		pos[0]++;
	}
	else if (s->value[pos[0]] == 'f') {
		pos[0]++;
		if (pos[0] + 8 > s->len) return 0;
		double d;
		memcpy(&d, s->value+pos[0], 8);
		pos[0] += 8;
		CreateDoubleValue(sv, d);
		return 1;
	}
	else {
		unsigned char *end = strchr(s->value + pos[0], ':');
		__int64 val;
		if (!ReadInt(s->value + pos[0], end, val) || val < 0) return 0;
		pos[0] = 1+(int)(end-s->value);
		if (pos[0] + val > s->len) return 0;
		if (!CreateStringValue(sv, s->value+pos[0], (int) val)) return 0;
		pos[0] += (int) val;
	}
	return 1;
}

void Bedecode(ScriptValue &s, ScriptValue *args) {
	int pos = 0;
	if (!BeDecode(s, args[0].stringVal, &pos)) {
		CreateNullValue(s);
	}
}

int JSONParseString(ScriptValue &s, unsigned char * &b) {
	while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
	CreateNullValue(s);
	if (b[0] == '"') {
		unsigned char *e = ++b;
		while (*e && *e != '"') {
			if (*e == '\\' && e[1]) e++;
			e++;
		}
		if (AllocateStringValue(s, (int)(e-b))) {
			int p = 0;
			unsigned char *w = s.stringVal->value;
			while(b<e) {
				if (b[0] != '\\') {
					w[p++] = b[0];
					b++;
					continue;
				}
				b++;
				if (b[0] == 'n') {
					w[p] = '\n';
				}
				else if (b[0] == 'r') {
					w[p] = '\r';
				}
				else if (b[0] == 't') {
					w[p] = '\t';
				}
				else if (b[0] == 'f') {
					w[p] = '\f';
				}
				else if (b[0] == 'b') {
					w[p] = '\b';
				}
				else if (b[0] == '\\') {
					w[p] = '\\';
				}
				else if (b[0] == '/') {
					w[p] = '/';
				}
				else if (b[0] == '\"') {
					w[p] = '/';
				}
				else if (b[0] == 'u') {
					b++;
					wchar_t c = 0;
					int i;
					for (i=0; i<4; i++) {
						if (*b>= '0' && *b<='9') {
							c = c*16 + b[0] - '0';
						}
						else if (*b>= 'A' && *b<='F') {
							c = c*16 + b[0] - 'A'+10;
						}
						else if (*b>= 'a' && *b<='f') {
							c = c*16 + b[0] - 'a'+10;
						}
						else break;
						b++;
					}
					p += UTF16toUTF8(w+p, &c, 1);
				}
				else if (b[0]) {
					// Who cares?
					b[0] = w[p];
				}
				else {
					b--;
					break;
				}
				p++;
				b++;
			}
			ResizeStringValue(s, p);
			if (b < e || !*b) return 0;
			b++;
			return 1;
		}
	}
	return 0;
}

int JSONdecodeSub(ScriptValue &s, unsigned char * &b) {
	while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
	CreateNullValue(s);
	if (b[0] == '"') {
		return JSONParseString(s, b);
	}
	else if (b[0] == '[') {
		if (!CreateListValue(s, 4)) return 0;
		b++;
		while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
		if (*b == ']') {
			b++;
			return 1;
		}
		while (1) {
			ScriptValue sv;
			int res = JSONdecodeSub(sv, b);
			int res2;
			if (sv.type != SCRIPT_NULL) {
				res2 = s.listVal->PushBack(sv);
			}
			if (!res || !res2) return 0;
			while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
			if (*b == ']') {
				b++;
				return 1;
			}
			// ???
			if (*b != ',') return 0;
			b++;
		}
	}
	else if (b[0] == '{') {
		if (!CreateDictValue(s, 4)) return 0;
		b++;
		while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
		if (*b == '}') {
			b++;
			return 1;
		}
		while (1) {
			ScriptValue sv, sv2;
			if (!JSONParseString(sv, b)) {
				sv.Release();
				return 0;
			}
			while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
			if (*b != ':') {
				sv.Release();
				return 0;
			}
			b++;
			int res = JSONdecodeSub(sv2, b);
			int res2 = s.dictVal->Add(sv, sv2);
			if (!res || !res2) return 0;
			while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
			if (*b == '}') {
				b++;
				return 1;
			}
			// ???
			if (*b != ',') return 0;
			b++;
		}
	}
	else if ((b[0] >= '0' && b[0] <= '9') || b[0] == '-' || b[0] == '.') {
		unsigned char *t1, *t2;
		__int64 i = strtoi64((char*)b, (char**)&t1, 0);
		double d = strtod((char*)b, (char**)&t2);
		if (t1 >= t2) {
			if (t1) {
				b = t1;
				CreateIntValue(s, i);
				return 1;
			}
		}
		else {
			b = t2;
			CreateDoubleValue(s, d);
			return 1;
		}
	}
	else {
		if (LCASE(b[0]) == 'n' && LCASE(b[1]) == 'u' && LCASE(b[2]) == 'l' && LCASE(b[3]) == 'l') {
			b+=4;
			return 1;
		}
		if (LCASE(b[0]) == 'f' && LCASE(b[1]) == 'a' && LCASE(b[2]) == 'l' && LCASE(b[3]) == 's' && LCASE(b[4]) == 'e') {
			b+=5;
			return 1;
		}
		if (LCASE(b[0]) == 't' && LCASE(b[1]) == 'r' && LCASE(b[2]) == 'u' && LCASE(b[3]) == 'e') {
			CreateIntValue(s, 1);
			b+=4;
			return 1;
		}
	}
	return 0;
}


void JSONdecode(ScriptValue &s, ScriptValue *args) {
	unsigned char *b = args[0].stringVal->value;
	JSONdecodeSub(s, b);
}

