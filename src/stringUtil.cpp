#include <stdio.h>
#include "stringUtil.h"
#include "malloc.h"
#include "unicode.h"
#include <ctype.h>

int Unescape(unsigned char *s, int len) {
	int i;
	int j;
	for (j=0, i=0; i<len; i++,j++) {
		if (s[i] == '|') {
			i++;
			if (s[i] == 'n')
				s[j] = '\n';
			else if (s[i] == 'r')
				s[j] = '\r';
			else if (s[i] == 't')
				s[j] = '\t';
			else if (LCASE(s[i]) == 'x' && i != len-1 && IsHex(s[i+1])) {
				unsigned int L = 0;
				int count = 0;
				while (i < len-1 && IsHex(s[i+1]) && count < 8) {
					i++;
					if (IsNumber(s[i])) L = L*16 + s[i]-'0';
					else L =  L*16 + LCASE(s[i])-'a' + 10;
					count ++;
				}
				if (L == 0) {
					s[j] = '\0';
				}
				else j += UTF32toUTF8(s+j, &L, 1)-1;
			}
			else if (IsNumber(s[i])) {
				unsigned int L = 0;
				int count = 0;
				i--;
				while (i < len - 1 && IsNumber(s[i+1]) && count < 3) {
					i++;
					L = L*10 + s[i]-'0';
					count ++;
				}
				if (L == 0) {
					s[j] = '\0';
				}
				else j += UTF32toUTF8(s+j, &L, 1)-1;
			}
			else
				s[j] = s[i];
		}
		else
			s[j] = s[i];
	}
	return j;
}

// Deliberately doesn't handle base 8
__int64 strtoi64(const char *start, char **end, int base) {
	const char *s = start;
	while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
		s++;
	}

	int sign = 1;
	if (s[0] == '-') {
		sign = -1;
		s++;
	}

	if (!base) {
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
			base = 16;
			s+=2;
		}
		else base = 10;
	}
	else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s+=2;

	__int64 out = 0;
	const char *test = s;
	if (base == 10) {
		unsigned char c;
		while ((c = (*s - '0')) <= 9) {
			out = out * 10 + c;
			s++;
		}
	}
	else if (base == 16) {
		while (1) {
			unsigned char c = *s;
			if (c <= '9' && c >= '0') c -= '0';
			else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
			else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
			else break;
			out = out * 16 + c;
			s++;
		}
	}
	out *= sign;
	if (end) {
		if (test != s) *end = (char*)s;
		else *end = (char*)start;
	}
	return out;
}


int utf8stricmp(const unsigned char *s1, const unsigned char* s2) {
	wchar_t *w1 = UTF8toUTF16Alloc(s1);
	wchar_t *w2 = UTF8toUTF16Alloc(s2);
	int out = 0;
	if (w1 && w2) {
		out = wcsicmp(w1, w2);
	}
	free(w1);
	free(w2);
	return out;
}

int localwcsicmp(wchar_t *s1, wchar_t *s2) {
	int w = 0;
	while (LCASE(s1[w]) == LCASE(s2[w])) {
		if (!s1[w]) {
			if (!s2[w]) return 0;
			return -1;
		}
		w++;
	}
	if (!s2[w]) {
		return 1;
	}
	if (s1[w] == '.') return -1;
	if (s2[w] == '.') return 1;
	return wcsicmp(s1+w, s2+w);
}


wchar_t *wcsistr(wchar_t *s1, wchar_t *s2) {
	while (*s1) {
		int w = 0;
		while (LCASE(s1[w]) == LCASE(s2[w])) {
			if (!s2[w]) break;
			w++;
		}
		if (!s2[w]) return s1;
		s1++;
	}
	return 0;
}

int substricmp(const char *s1, const char *s2) {
	while (*s2 && LCASE(*s1) == LCASE(*s2)) {
		s1++;
		s2++;
	}
	if (*s2 == 0) return 0;
	// To order consistently.
	return stricmp(s1, s2);
}

int substrcmp(const char *s1, const char *s2) {
	while (*s2 && *s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s2 == 0) return 0;
	// To order consistently.
	return strcmp(s1, s2);
}


// Gives a bit more control than sprintf.  Use it to guarantee I
// only have 3 digits and a decimal.
void DisplayDouble(double d, char *c, double m, int digits) {
	while (m > d && m > 9) m/= 10;
	while (digits>0 || m >= 0.9) {
		int i = (int)(d/m);
		d -= i*m;
		if (i>9) i = 9;
		if (d<0) d = 0;
		(c[0]) = i + '0';
		c++;
		if (digits>1 && m <1.2 && m > 0.8) {
			c[0] = '.';
			c++;
		}
		m/=10;
		digits--;
	}
	c[0] = 0;
}
void FormatDataSize (char *out, double d, int min, int extra, int extension) {
	const static char prefixes[] = "BKMGTPEZY";
	// 4 numbers (including .), 2 characters, null, extensionSize, 3 slack.
	if (d<0) d = 0;
	int p = 0;
	while ((d>990 || min>0) && prefixes[p+1]) {
		p++;
		d/=(double)1024;
		min --;
	}

	out[0] = '?';
	out[1] = '0';
	int digits=3+extra;
	double m = 100;
	if (!p) {
		digits = 0;
	}
	DisplayDouble(d, out, m, digits);
	out += strlen(out);
	if (extension) {
		if (p || extension > 2) {
			if (!(extension & 1))
				*(out++) = ' ';
			*(out++) = prefixes[p];
		}
	}
	out[0] = 0;
}

char *localstristr(char *s, char *s2) {
	while (*s) {
		int i=0;
		while (LCASE(s[i]) == LCASE(s2[i]) && s2[i])
			i++;
		if (!s2[i]) return s;
		s++;
	}
	return 0;
}

int FormatDuration(char *s, int secs) {
	if (secs < 0) {
		return sprintf(s, "?:??");
	}
	if (secs > 3600) {
		return sprintf(s, "%i:%02i:%02i", secs/3600, (secs/60)%60, secs%60);
	}
	return sprintf(s, "%i:%02i", secs/60, secs%60);
}

void sort(unsigned char **data, unsigned int len, int offset) {
	unsigned int counts[2][256];
	unsigned int i, p, start;
	memset(counts[0], 0, sizeof(counts[0]));
	for (i=0; i<len; i++) {
		counts[0][data[i][offset]]++;
	}
	counts[1][0] = counts[0][0];
	for (i=1; i<256; i++) {
		counts[1][i] = (counts[0][i] += counts[0][i-1]);
	}
	p = 0;
	start = 0;
	i = 0;
	while (1) {
	if (counts[1][p] <= i) {
		if (p && start + 1 < counts[0][p]) {
			sort(data+counts[0][p-1], counts[0][p]-counts[0][p-1], offset+1);
		}
		if (p == 255) break;
			start = i = counts[0][p++];
		}
		else {
			unsigned char key = data[i][offset];
			if (key == p) i++;
			else {
				unsigned char *temp = data[--counts[1][key]];
				data[counts[1][key]] = data[i];
				data[i] = temp;
			}
		}
	}
}

void *memdup (const void *mem, size_t size) {
	void *out = malloc(size);
	if (!out) return 0;
	memcpy(out, mem, size);
	return out;
}

void sort(unsigned char **data, unsigned int len, int offset = 0);

