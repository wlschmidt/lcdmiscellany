#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string.h>

int localwcsicmp(wchar_t *s1, wchar_t*s2);

#ifdef strcpy
#undef strcpy
#endif

#ifdef sprintf
#undef sprintf
#endif

//#define wcslen wcslen2
//#define strlen strlen2
/*#define wcscpy wcscpy2
#define strcpy strcpy2
#define wcschr wcschr2
#define strchr strchr2
//*/


#define IsLetter(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define IsNumber(c) ((c)>='0' && (c)<='9')
#define IsHex(c) (IsNumber(c) || ((c)>='A' && (c)<='F') || ((c)>='a' && (c)<='f'))
#define IsWhitespace(c) ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r')

inline int IsPunctuation(char c) {
	return ('.'== c || c==';' || c=='\"' || c=='>' ||
		','== c || c==':' || c=='\'' || c=='<' ||
		'['== c || c==']' || c=='{' || c=='}' ||
		'='== c || c=='-' || c=='(' || c==')');
}


#define KEY_SHIFT 1
#define KEY_CONTROL 2
#define KEY_ALT 4
#define KEY_WIN 8
#define KEY_IGNORE 16
#define KEY_CHARACTER 32

// Remove leading and trailing spaces, make sure zeroed out to
// at least minLen characters.

template<class T>
void LeftJustify(T *s, int minLen=0) {
	int i=0;
	int j=0;
	// remove leading spaces.
	while (s[i] && (s[i] == ' ' || s[i] == '\t')) i++;
	while (s[i]) {
		s[j++] = s[i++];
	}
	// 0 it out to minLen.  minLen can be 0.
	s[j++] = 0;
	while (j<minLen) {
		s[j++] = 0;
	}
	// remove trailing spaces.
	j--;
	while (j && (s[j]== ' ' ||  s[j] == 0 || s[j] == '\t')) {
		s[j] = 0;
		j--;
	}
}


inline char LCASE(char c) {
	if (c > 'Z' || c<'A') return c;
	return c - 'A'+'a';
}
inline char UCASE(char c) {
	if (c > 'z' || c<'a') return c;
	return c + 'A'-'a';
}

int FormatDuration(char *s, int secs);

int substricmp(const char *s1, const char *s2);
int substrcmp(const char *s1, const char *s2);
inline int stricmp(const unsigned char *s1, const unsigned char *s2) {
	return stricmp((char*)s1, (char*)s2);
}
inline int strcmp(const unsigned char *s1, const unsigned char *s2) {
	return strcmp((char*)s1, (char*)s2);
}

int utf8stricmp(const unsigned char *s1, const unsigned char *s2);

wchar_t *wcsistr(wchar_t *s1, wchar_t *s2);

void FormatDataSize (char *out, double d, int min=0, int extra=0, int extension=0);
__forceinline void FormatBytes (char *s, double b, int min=2, int extra = 0, int extension = 0) {
	FormatDataSize(s, b, min, extra, extension);
	//size_t i = strlen(s);
	//if (s[i-2] == 'B') s[i-1] = 0;
}
/*
__forceinline void FormatBytesPerSecond (char *s, double b, int min=1) {
	FormatDataSize(s, b, "/s", 2, min);
}
//*/

char *localstristr(char *s, char *s2);

#define UCASE(x) ((x)>='a' && (x) <= 'z' ? (x)-'a'+'A' : (x))
#define LCASE(x) ((x)>='A' && (x) <= 'Z' ? (x)-'A'+'a' : (x))
#define IsLowercase(x) ((x)>='a' && (x)<='z')
#define IsUppercase(x) ((x)>='A' && (x)<='Z')

template<class T>
inline int StrLen(T *s) {
	int i=0;
	for (; s[i]; i++);
	return i;
}

template<class T>
inline int Compare(T *s1, T *s2) {
	int i=0;
	for (; s1[i]; i++) {
		if (s1[i] != s2[i]) break;
	}
	if (s1[i] == s2[i]) return 0;
	return (s1[i] > s2[i])*2 - 1;
}

template<class T>
inline int CompareSubstringNoCase(T *s1, T *s2) {
	int i=0;
	for (; s2[i]; i++) {
		if (UCASE(s1[i]) != UCASE(s2[i])) break;
	}
	if (0 == s2[i]) return 0;
	return (UCASE(s1[i]) > UCASE(s2[i]))*2 - 1;
}


template<class T>
inline int CompareSubstring(T *s1, T *s2) {
	int i=0;
	for (; s2[i]; i++) {
		if (s1[i] != s2[i]) break;
	}
	if (0 == s2[i]) return 0;
	return (s1[i] > s2[i])*2 - 1;
}

template<class T>
inline int CompareSubstringReverse(T *s1, T *s2) {
	int i=0;
	for (; s2[i]; i--) {
		if (s1[i] != s2[i]) break;
	}
	if (0 == s2[i]) return 0;
	return (s1[i] > s2[i])*2 - 1;
}

template<class T>
inline int ReverseCompare(T *s1, T *s2) {
	int i=0;
	for (; s1[i]; i--) {
		if (s1[i] != s2[i]) break;
	}
	if (s1[i] == s2[i]) return 0;
	return (s1[i] > s2[i])*2 - 1;
}


void *memdup (const void *mem, size_t size);

#define WHITESPACE(x) ((x)==' ' ||(x)=='\t' ||(x)=='\n' ||(x)=='\r')

char ** parseArgs(char *s, int *argc, char *split);

// Only works on values from 0 to 999
void GenericDisplay (char *& start, char *&out, int val);

__int64 strtoi64(const char *s, char **end, int base);
inline __int64 strtoi64(const unsigned char *s, unsigned char **end, int base) {
	return strtoi64((const char*)s, (char**)end, base);
}
#define strtoul(x,y,z) ((unsigned int)strtoi64(x,y,z));
/*
inline unsigned int wcslen(const wchar_t *s) {
	const wchar_t *s2 = s;
	while (*s2) s2++;
	return (unsigned int)(s2 - s);
}

inline unsigned int strlen(const char *s) {
	const char *s2 = s;
	while (*s2) s2++;
	return (unsigned int)(s2 - s);
}
/*

inline wchar_t *wcscpy(wchar_t *s, const wchar_t *s2) {
	wchar_t *out = s;
	while (*s2) {
		*s = *s2;
		s++;
		s2++;
	}
	*s = 0;
	return out;
}

inline void strcpy(char *s, const char *s2) {
	while (*s2) {
		*s = *s2;
		s++;
		s2++;
	}
	*s = 0;
}

inline wchar_t * wcschr(const wchar_t *s, wchar_t c) {
	while (*s) {
		if (*s == c) return (wchar_t*)s;
		s++;
	}
	if (!c) return (wchar_t*)s;
	return 0;
}

inline char * strchr(const char *s, char c) {
	while (*s) {
		if (*s == c) return (char*)s;
		s++;
	}
	if (!c) return (char*)s;
	return 0;
}
//*/
inline unsigned int strlen(const unsigned char *s) {
	return (unsigned int)strlen((const char*)s);
}

inline void strcpy(unsigned char *s, const unsigned char *s2) {
	strcpy((char*)s, (const char*) s2);
}
inline unsigned char* strchr(const unsigned char *s, unsigned char c) {
	return (unsigned char*)strchr((const char*)s, (char) c);
}


inline unsigned char* strdup(const unsigned char *s) {
	return (unsigned char*)strdup((char*)s);
}

inline int strncmp(const unsigned char *s1, const unsigned char *s2, size_t len) {
	return strncmp((const char*)s1, (const char*)s2, len);
}

int Unescape(unsigned char *s, int len);

#endif