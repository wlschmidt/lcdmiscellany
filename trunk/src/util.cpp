#include "util.h"
#include "unicode.h"
#include "global.h"
#include "stringUtil.h"
#include "malloc.h"
#include <time.h>

unsigned __int64 MT[312];
unsigned __int64 y;
int X = 313;

FILETIME uselessOffset;
void UtilInit() {
	SYSTEMTIME base;
	memset(&base, 0, sizeof(base));
	base.wYear = 1970;
	base.wMonth = 1;
	base.wDay = 1;
	SystemTimeToFileTime(&base, &uselessOffset);
}

double FileTimeToUsefulTime(FILETIME *t) {
	return (double) ((*(__int64*)t) - *(__int64*)&uselessOffset);
}

inline static void Seed() {
	MT[0] = _time64(0);
	for (int i=1; i<312; i++) {
		MT[i] = 69069 * MT[i-1]+1;
	}
}

unsigned __int64 GetNumber() {
	if (X >= 312) {
		if (X == 312) {
			for (int i=0; i<312; i ++) {
				y = (MT[i]&((__int64)1<<63)) + (MT[(i+31)%312]&~((__int64)1<<63));
				if (MT[0] & 1) {
					MT[i] = MT[(i+156)%312] ^ (y>>1);
				}
				else {
					MT[i] = MT[(i+156)%312] ^ (y>>1) ^ (__int64)0xB5026F5AA96619E9;
				}
			}
		}
		else {
			Seed();
		}
		X = 0;
	}
	y = MT[X++];
	y = y ^ (y>>29);
	y = y ^ ((y<<17) & (__int64)0xD66B5EF5B4DA0000);
	y = y ^ ((y<<37) & (__int64)0xFDED6BE000000000);
	y = y ^ (y>>41);
	return y;
}


int ComCount = 0;

int InitCom() {
	if (!(ComCount++)) {
		HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		return (hr == S_OK || hr == S_FALSE);
	}
	return 1;
}

void KillCom() {
	if (ComCount) {
		ComCount = 0;
		CoUninitialize();
	}
}

void UninitCom() {
	if (0 == --ComCount) {
		CoUninitialize();
	}
}

int getShutdownPriv() {
	return getPriv(SE_SHUTDOWN_NAME);
}


int getDebugPriv() {
	return getPriv(SE_DEBUG_NAME);
}


int getPriv(wchar_t * name) {
	HANDLE hToken;
	LUID seValue;
	TOKEN_PRIVILEGES tkp;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &seValue) ||
		!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return 0;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = seValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	int res = AdjustTokenPrivileges(hToken, 0, &tkp, sizeof(tkp), NULL, NULL);

	CloseHandle(hToken);
	return res;
}

unsigned __int64 GetTickCountNoOverflow() {
	static unsigned __int64 lastTime = 0;
	static unsigned __int64 baseTime = 0;
	unsigned __int64 t = baseTime + GetTickCount();
	if (t < lastTime) {
		baseTime += 0x100000000;
		t += 0x100000000;
	}
	return lastTime = t;
}

unsigned __int64 GetTimeAccurate() {
	static double f = 0;
	if (f) {
		unsigned __int64 v;
		if (QueryPerformanceCounter((LARGE_INTEGER*) &v)) {
			return (unsigned __int64)(v*f);
		}
	}
	else {
		unsigned __int64 freq = 0;
		if (QueryPerformanceFrequency((LARGE_INTEGER*)&freq) && freq) {
			f = 1000/(double)freq;
			return GetTimeAccurate();
		}
	}
	return GetTickCountNoOverflow();
}

int GetProcessPriority(int id, int &priority) {
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, 0, id);
	if (h == 0) {
		getDebugPriv();
		h = OpenProcess(PROCESS_QUERY_INFORMATION, 0, id);
	}
	if (!h) return 0;
	priority = GetPriorityClass(h);
	CloseHandle(h);
	return priority != 0;
}

int SetProcessPriority(int id, int priority) {
	HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, 0, id);
	if (h == 0) {
		getDebugPriv();
		h = OpenProcess(PROCESS_SET_INFORMATION, 0, id);
	}
	if (!h) return 0;
	int out = SetPriorityClass(h, priority);
	CloseHandle(h);
	return out != 0;
}


int KillProcess(int id) {
	HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, id);
	if (h == 0) {
		getDebugPriv();
		h = OpenProcess(PROCESS_TERMINATE, 0, id);
	}
	if (!h) return 0;
	int out = TerminateProcess(h, -1);
	CloseHandle(h);
	return out != 0;
}

void GenericDisplay (char *& start, char *&out, int val) {
	int mode = 0;
	if (LCASE(start[0]) ==  LCASE(start[1])) {
		mode = 1 + (start[1] != LCASE(start[1]));
		start += 2;
	}
	else start++;
	if (val >= 10 || mode == 2) {
		sprintf(out, "%02i", val);
		out += strlen(out);
		return;
	}
	if (mode == 1) {
		out++[0] = ' ';
	}
	sprintf(out++, "%i", val);
}

void DecodeChar(int &message, int &scancode) {
	int out = 0;
	if (message == VK_NUMLOCK || message == VK_CAPITAL || message == VK_SCROLL) {
		// not quite sure why this is needed...But it is
		//GetKeyboardState(s);
		//s[message] ^= 1;
		//SetKeyboardState(s);
		out |= KEY_IGNORE;
	}
	else if (message == VK_RWIN || message == VK_LWIN) {
		out |= KEY_IGNORE;
	}
	else {
		if (GetAsyncKeyState(VK_CONTROL)&0x8000)
			out = KEY_CONTROL;
		if (GetAsyncKeyState(VK_SHIFT)&0x8000)
			out |= KEY_SHIFT;
		if (GetAsyncKeyState(VK_MENU)&0x8000)
			out |= KEY_ALT;
		if ((GetAsyncKeyState(VK_RWIN) | GetAsyncKeyState(VK_LWIN))&0x8000)
			out |= KEY_WIN;
		unsigned char s[256];
		//if ((message >= 'A' && message <= 'Z') ||
		//(message >= '0' && message <= '9')) {
		wchar_t key[2];
		memset(s, 0, 256);
		//int test = GetAsyncKeyState(VK_CAPITAL);
		//GetKeyboardState(s);
		s[VK_SHIFT] = (GetKeyState(VK_SHIFT)>>8)&0x80;
		s[VK_CAPITAL] = (char) GetKeyState(VK_CAPITAL);
		//s[VK_TAB] = (GetKeyState(VK_TAB)>>8)&0x80;
		s[VK_NUMLOCK] = (char) GetKeyState(VK_TAB);
		s[VK_SCROLL] = (char) GetKeyState(VK_SCROLL);
		char temp = ToUnicode(message, scancode, s, key, 2, 0);
		if (temp) {
			out |= KEY_CHARACTER;
			message = key[0];
		}
	}
	scancode = out;
}
wchar_t *GetFile(wchar_t *name) {
	if (!name || wcslen(name)>MAX_PATH) return 0;
	wchar_t s[MAX_PATH*2+3];
	if (name[0] != '\\' && !wcschr(name, ':') && name[0] !='.') {
		int w = UCASE(name[0]) != 'O' || UCASE(name[1]) != 'V' || UCASE(name[2]) != 'E' || UCASE(name[4]) != 'R';
		int w2 = UCASE(name[5]) != 'R' || UCASE(name[6]) != 'I';
		int w3 = UCASE(name[7]) != 'D' || UCASE(name[8]) != 'E';
		if (UCASE(name[0]) != 'O' || UCASE(name[1]) != 'V' || UCASE(name[2]) != 'E' || UCASE(name[3]) != 'R' ||
			UCASE(name[4]) != 'R' || UCASE(name[5]) != 'I' || UCASE(name[6]) != 'D' || UCASE(name[7]) != 'E' || (UCASE(name[8]) != '\\' && UCASE(name[8]) != '/')) {
			wsprintf(s, L"Override\\%s", name);
			wchar_t *o = GetFile(s);
			if (o) {
				if (GetFileAttributesW(o) != INVALID_FILE_ATTRIBUTES)
					return o;
				free(o);
			}
		}
	}
	int temp = GetModuleFileNameW(0, s, MAX_PATH+2);
	if (temp >= MAX_PATH+2 || temp == 0) return 0;
	wchar_t *p = 0;
	wchar_t *s2 = s;
	while (*s2) {
		if (*s2 == '\\') p = s2;
		s2++;
	}
	if (!p || *p != '\\') return 0;
	p++;
	wcscpy(p, name);
	return wcsdup(s);
}

FileLineReader::FileLineReader(FileReader * file, int ignoreEmptyLines, int otype, int initialBufferSize, int maxLineLen, const unsigned char *comments) {
	utf16 = 0;
	used = 0;
	lb = 0;
	if (!comments) this->comments = 0;
	else this->comments = (unsigned char*)strdup((char*)comments);
	outputType = otype;
	this->initialBufferSize = initialBufferSize;
	bufferSize = initialBufferSize;
	if (bufferSize<1000) bufferSize = 1000;
	this->maxLineLen = maxLineLen;
	if (maxLineLen < bufferSize) maxLineLen = bufferSize;
	// +1 is for the terminating null.
	buffer = (unsigned char*) malloc(sizeof(unsigned char) * (bufferSize+1));
	//lineStart = bufferSize;
	this->file = file;
	bufferEnd = -1;
	this->ignoreEmptyLines = ignoreEmptyLines;
	int i = (int) readerRead(buffer, 1, 4, file);
	//if (bufferSize != i) bufferEnd = i;
	bufferEnd = 0;
	lineStart = -1;
	// UTF8 (treat just as ASCII, since I don't really want ASCII)
	if (i >= 3 &&
		buffer[0] == 0xEF &&
		buffer[1] == 0xBB &&
		buffer[2] == 0xBF) {
			readerMiniseek(file, 3, SEEK_SET);
			fileType = UTF8;
	}
	else if (i >= 2 &&
			 buffer[0] == 0xFF &&
			 buffer[1] == 0xFE) {
		readerMiniseek(file, 2, SEEK_SET);
		fileType = UTF16;
	}
	else if (i >= 2 &&
			 buffer[0] == 0xFE &&
			 buffer[1] == 0xFF) {
		readerMiniseek(file, 2, SEEK_SET);
		fileType = UTF16R;
	}
	else {
		readerMiniseek(file, 0, SEEK_SET);
		if ((i >= 2 && buffer[1] == 0) ||
				 (i >= 4 && buffer[3] == 0)) {
			fileType = UTF16;
		}
		else if ((i >= 2 && buffer[0] == 0) ||
				 (i >= 4 && buffer[2] == 0)) {
			fileType = UTF16R;
		}
		else {
			fileType = ASCII;
			//fileType = UTF8;
		}
	}
	/*
	else {
		int i = 0;
		unsigned char* buf  = (unsigned char*) buffer;
		for (i=0; i<bufferSize-3; i++) {
			if (buf[i+1] >= 0xA1 && buf[i+1] < 0xfe) {
				if ((buf[i] >= 0xA1 && buf[i] < 0xfe) ||
					buf[i] == 0x8E ||
					(buf[i] == 0x8F && buf[i+2] >= 0xA1 && buf[i+2] <= 0xFE)) {
					fileType = EUC_JP;
				}
			}
			/*else if (buf[i+1] >= 0x40 && buf[i+1] < 0x7E) {
				if ((buf[i] >= 0xA1 && buf[i] < 0xfe) ||
					buf[i] == 0x8E ||
					(buf[i] == 0x8F && buf[i+2] >= 0xA1 && buf[i+2] <= 0xFE)) {
					fileType = EUC_JP;
				}
			}*/
	//	}
	//}
}

FileLineReader* OpenLineReader(wchar_t *fileName, int ignoreEmptyLines, int type, const unsigned char *comments, int maxLineLen) {
	if (!fileName) return 0;
	FileReader *f = wreaderOpen(fileName);
	if (!f) return 0;
	return new FileLineReader(f, ignoreEmptyLines, type, 200020, maxLineLen, comments);
}

const wchar_t windows1252Map[128] = {
	0x20AC,    ' ', 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152,    ' ', 0x017D,    ' ',
	   ' ', 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153,    ' ', 0x017E, 0x0178,
	0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
};

int FileLineReader::NextLine(unsigned char **line) {
	if (!file || bufferEnd == lineStart) {
		*line = 0;
		return -1;
	}
	int i = lineStart;
	int len;
	do {
		while (1) {
			if (i == bufferEnd) {
				if (i==lineStart) {
					*line = 0;
					return -1;
				}
				break;
			}
			if (i == -1 || i >= used) {
				if (i==-1) {
					i = 0;
					lineStart = 0;
				}
				else if (used >= bufferSize - 20) {
					if (lineStart) {
						memmove(buffer, &buffer[lineStart], (i-lineStart)*sizeof(unsigned char));
						i-=lineStart;
						used -= lineStart;
						lineStart = 0;
					}
					else if (bufferSize < maxLineLen + initialBufferSize) {
						int newBufferSize = bufferSize*2;
						if (newBufferSize > maxLineLen + initialBufferSize)
							newBufferSize = maxLineLen + initialBufferSize;
						// +1 is so I can always add a terminating null.
						if (srealloc(buffer, (newBufferSize+1) * sizeof(unsigned char))) {
							bufferSize = newBufferSize;
						}
					}
					else {
						if (outputType == WHOLE_FILE) {
							bufferEnd = i;
							if (i == bufferSize) i--;
							continue;
						}
						used = i = maxLineLen+3;
					}
				}
				if (fileType == UTF8) {
					int j = (int) readerRead(&buffer[i], 1, bufferSize-used, file);
					if (j < 0) j = 0;
					used += j;
					if (used < bufferSize) {
						bufferEnd = i+j;
						// Exactly at end of file.  initial EOF check will handle properly now.
						if (j == 0) continue;
					}
				}
				else if (fileType == ASCII) {
					int j = (int)readerRead(&buffer[i], 1, (bufferSize-i)/4, file);
					//__int64 test = _ftelli64(file);
					int t;
					int shift = 0;
					if (j < 0) j = 0;
					wchar_t temp;
					unsigned char temp2[10];
					for (t = 0; t<j; t++) {
						if (buffer[i+t] >= 0x80) {
							temp = windows1252Map[buffer[i+t]-0x80];
							shift += UTF16toUTF8(temp2, &temp, 1)-1;
						}
					}
					if (shift) {
						j += shift;
						for (t = j-1-shift; t>=0; t--) {
							if (buffer[i+t] >= 0x80) {
								temp = windows1252Map[buffer[i+t]-0x80];
								int size = UTF16toUTF8(temp2, &temp, 1);
								shift -= size-1;
								memcpy(buffer+i+t+shift, temp2, size*sizeof(unsigned char));

								if (!shift) break;
							}
							else {
								buffer[i+t+shift] = buffer[i+t];
							}
						}
					}
					if (j < (bufferSize - i)/2) {
						bufferEnd = i+j+shift;
						// Exactly at end of file.  initial EOF check will handle properly now.
						if (j == 0) continue;
					}
					j += shift;
					used += j;
				}
				else if (fileType == UTF16 || fileType == UTF16R) {
					while (used < bufferSize) {
						wchar_t temp[25000];
						int want = bufferSize/3;
						if (want < 20) break;
						if (want > 25000) want = 25000;
						int j = (int) readerRead(temp, 2, 25000, file);
						if (j < 0) j = 0;
						if (fileType == UTF16R) {
							for (int k = 0; k<j; k++) {
								wchar_t temp2 = (temp[k]<<8) | (temp[k]>>8);
								temp[k] = temp2;
							}
						}
						int len = UTF16toUTF8(&buffer[used], temp, j);
						used += len;
						if (j < want) {
							bufferEnd = used;
							break;
						}
					}
				}
				/*
				else {
					while (rBufferSize < bufferSize - 100000) {
						wchar_t temp[25000];
						char temp2[25000];
						int j = (int) fread(temp2, 1, 25000, file);
						if (j == 25000) {
							int q = j;
							while (q && temp2[q-1] < 0) {
								q--;
							}
							if (q & 1) {
								j--;
								fseek(file, -1, SEEK_CUR);
							}
						}
						int len2 = MultiByteToWideChar(fileType, MB_PRECOMPOSED, temp2, j, temp, 25000);
						if (len2 <0)
							len2 = 0;
						int len = UTF16toUTF8(&buffer[rBufferSize], temp, len2);
						rBufferSize += len;
						if (j < 24999) {
							bufferEnd = rBufferSize;
							rBufferSize += 120;
							break;
						}
					}
					bufferSize = rBufferSize;
				}
				//*/
			}
			if (outputType == SINGLE_LINE) {
				if (buffer[i] == '\n' || buffer[i] == '\r') {
					break;
				}
			}
			else if (outputType == DOUBLE_BREAK) {
				if ((buffer[i] == '\n' && (buffer[i+1] == '\n' || (buffer[i+1] == '\r' && buffer[i+2] == '\n'))) ||
					(buffer[i] == '\r' && (buffer[i+1] == '\r' || (buffer[i+1] == '\n' && buffer[i+2] == '\r')))) break;
			}
			i++;
		}
		*line = &buffer[lineStart];
		// If it's a file that allow comments, remove leading whitespace, as it's presumably a config
		// file of some sort.
		if (comments) {
			while (i < bufferEnd && (buffer[i] == ' ' || buffer[i] == '\t')) i++;
		}
		if (i == bufferEnd) {
			buffer[i] = 0;
			len = i-lineStart;
			lineStart = i;
		}
		else {
			int j = i;
			if (i>lineStart) {
				if (buffer[i] == '\n' && buffer[i-1] == '\r') j--;
				if (j-lineStart>maxLineLen) j = lineStart + maxLineLen;
			}
			if (i == lineStart && buffer[i] == '\n' && lb == '\r') {
				lb = 0;
				lineStart++;
				len = 0;
				i = lineStart;
				continue;
			}
			lb = buffer[i];
			buffer[j] = 0;
			len = j-lineStart;
			lineStart = ++i;
		}
	}
	while ((ignoreEmptyLines && !len) || (comments && len && 0==strcspn((char*)*line, (char*)comments)));
	return len;
}

int FileLineReader::NextLine(wchar_t **line, int keep) {
	unsigned char *line8;
	int res = NextLine(&line8);
	if (res <= 0) {
		*line = 0;
		return res;
	}
	if (!srealloc(utf16, sizeof(wchar_t) * (1+strlen(line8)))) {
		*line = 0;
		return 0;
	}
	int len  = UTF8toUTF16(utf16, line8, -1);
	srealloc(utf16, sizeof(wchar_t) * (len+1));
	*line = utf16;
	if (keep) {
		utf16 = 0;
	}
	return len;
}


FileLineReader::~FileLineReader() {
	if (file) readerClose(file);
	free(utf16);
	free(buffer);
	free(comments);
}

wchar_t *GetEnvVars(wchar_t *in) {
	int inLen = (int) (1 + wcslen(in));
	int outLen = 2*inLen+260;
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t) * outLen);
	memset(out, 0, outLen*2);
	if (!out) return 0;
	int outPos = 0;
	int inPos = 0;
	while (1) {
		wchar_t *t1 = wcschr(in+inPos, '%');
		wchar_t *t2 = 0;
		int copyLen = inLen-inPos;
		if (t1) {
			t2 = wcschr(t1+1, '%');
			if (t2)
				copyLen = (int)(t1 - &in[inPos]);
		}
		if (outLen <= outPos + inLen - inPos + 256 && !srealloc(out, sizeof(wchar_t) * (outLen = outPos + inLen - inPos+260))) {
			free(out);
			return 0;
		}
		memcpy(out + outPos, in + inPos, copyLen*sizeof(wchar_t));
		inPos += copyLen;
		outPos += copyLen;
		if (inPos == inLen) break;
		t2[0] = 0;
		int len = GetEnvironmentVariableW(&in[inPos+1], &out[outPos], outLen - outPos);
		t2[0] = '%';
		if (len <=0 || outLen - outPos - 2 <= len) {
			out[outPos++] = in[inPos++];
		}
		else {
			inPos = (int)(t2 + 1 - in);
			outPos += len;
		}
	}
	//out[outPos] = 0;
	return out;
}

wchar_t *ResolvePathRel(unsigned char *base) {
	wchar_t *temp = UTF8toUTF16Alloc(base);
	return ResolvePathRelSteal(temp);
}

wchar_t *ResolvePathRelSteal(wchar_t *base) {
	LeftJustify(base);
	if (wcschr(base, '%')) {
		wchar_t *w = wcschr(base, ':');
		if (*w == '%' || w <= base+1) {
			wchar_t *out = GetEnvVars(base);
			if (out) {
				free(base);
				base = out;
			}
		}
	}
	return base;
}

wchar_t *ResolvePath(unsigned char *base) {
	wchar_t *temp = UTF8toUTF16Alloc(base);
	return ResolvePathSteal(temp);
}

wchar_t *ResolvePath(wchar_t *base) {
	return ResolvePathSteal(wcsdup(base));
}

wchar_t *ResolvePathSteal(wchar_t *base) {
	if (!base || !base[0] || !base[1]) return 0;
	/*
	int len = 2*strlen(base)+260;
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t) * len);
	if (!out) return 0;
	int i, dest = 0;
	*/
	if (base[0] == '%' || base[0] == '/' || base[0] == '\\' || base[1] == ':') {
		int i = 0;
		for (int j=0; base[j]; j++) {
			if (base[j] == '/') {
				base[j] = '\\';
			}
			if (base[j+1] == '/') {
				base[j+1] = '\\';
			}
			if (base[j] == '\\') {
				if (base[j+1] == '\\')
					continue;
				if (i && base[i-1] == '.') {
					if (i == 1 || base[i-2] == '\\') {
						i--;
						continue;
					}
					if (base[i-2] == '.' && base[i-3] == '\\' && i >=4) {
						int t = i-4;
						while (t > 0 && base[t] != '\\') t--;
						if (base[t] == '\\') {
							i = t+1;
							continue;
						}
					}
				}
			}
			base[i++] = base[j];
		}
		base[i] = 0;
		// Windows path.  Resolve variables and then add prefixes as necessary.
		if (wcschr(base, '%')) {
			wchar_t *out = GetEnvVars(base);
			if (out) {
				free(base);
				base = out;
			}
		}
		if (base[0] == '\\') {
			if (base[1] == '\\' && base[2] != '?') {
				wchar_t *temp = (wchar_t*) malloc(sizeof(wchar_t)*(wcslen(base) + 3));
				if (temp) {
					temp[0] = '\\';
					temp[1] = '\\';
					temp[2] = '?';
					wcscpy(temp+3, base+1);
					free(base);
					/*int len = wcslen(temp);
					if (temp[len-1] != '\\') {
						temp[len] = '\\';
						temp[len+1] = 0;
					}//*/
					return temp;
				}
			}
		}
		else {
			int offset = (base[2] == '\\');
			wchar_t *temp = (wchar_t*) malloc(sizeof(wchar_t)*(wcslen(base) + 6 - offset));
			if (temp) {
				temp[0] = '\\';
				temp[1] = '\\';
				temp[2] = '?';
				temp[3] = '\\';
				temp[4] = base[0];
				temp[5] = ':';
				temp[6] = '\\';
				wcscpy(temp+7, base+2+offset);
				free(base);
				/*int len = wcslen(temp);
				if (temp[len-1] != '\\') {
					temp[len] = '\\';
					temp[len+1] = 0;
				}//*/
				return temp;
			}
		}
	}
	/*int len = wcslen(base);
	if (base[len-1] != '\\') {
		if (!srealloc(base, sizeof(wchar_t) * (len+2))) {
			// fail is better than deleting the wrong files.
			free(base);
			return 0;
		}
		base[len] = '\\';
		base[len+1] = 0;
	}
	//*/
	return base;
}

/*
wchar_t *ResolvePath(char *base) {
	int len = 2*strlen(base)+260;
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t) * len);
	if (!out) return 0;
	int i, dest = 0;
	if (out[0] == '%' || out[0] == '\\' || out[1] == ':') {
		// Windows path.  Resolve variables and then add prefixes as necessary.
	}
	// URL or something else.
	else {
	}
}
*/

//wchar_t *ResolvePath(char *base, char* add);

FileLineReader* OpenLineReader(unsigned char *fileName, int ignoreEmptyLines, int type, const char *commentChars, int maxLineLen) {
	wchar_t *fileName2 = UTF8toUTF16Alloc(fileName);
	FileLineReader* out = OpenLineReader(fileName2, ignoreEmptyLines, type, (unsigned char*) commentChars, maxLineLen);
	free(fileName2);
	return out;
}
