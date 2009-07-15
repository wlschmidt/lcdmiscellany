#ifndef UNICODE_H
#define UNICODE_H

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

int UTF8toUTF32(unsigned int *out, const unsigned char *in, int len = -1);
inline int UTF8toUTF32(unsigned int *out, const char *in, int len = -1) {return UTF8toUTF32(out, (const unsigned char *)in, len);}


int UTF32toUTF8(unsigned char *out, const unsigned int *in, int len = -1);
inline int UTF32toUTF8(char *out, const unsigned int *in, int len = -1) {return UTF32toUTF8((unsigned char*) out, in, len);}

int UTF8toUTF16(wchar_t *out, const unsigned char *in, int len = -1);
inline int UTF8toUTF16(wchar_t *out, const char *in, int len = -1) {return UTF8toUTF16(out, (const unsigned char*) in, len);}

int UTF16toUTF8(unsigned char *out, const wchar_t *in, int len = -1);
inline int UTF16toUTF8(char *out, const wchar_t *in, int len = -1) {return UTF16toUTF8((unsigned char*) out, in, len);}

int UTF16toASCII(unsigned char *out, const wchar_t *in, int len = -1);
inline int UTF16toASCII(char *out, const wchar_t *in, int len = -1) {return UTF16toASCII((unsigned char *)out, in, len);}

int ASCIItoUTF8(unsigned char *out, const char *in, int len = -1);
// note:  In and out can be the same.
int UTF8toASCII(char *out, const unsigned char *in, int len = -1);

wchar_t * UTF8toUTF16Alloc(const unsigned char *in, int *slen = 0);

// slen in = overestimate of string len.  Out is final length of string.
// len is exact length of string.  Overrides slen.
unsigned char * UTF16toUTF8Alloc(const wchar_t *in, int *slen = 0);
char * UTF8toASCIIAlloc(const unsigned char *in);
unsigned char * ASCIItoUTF8Alloc(const char *in);

unsigned long NextUnicodeChar(unsigned char *s, int *len);

typedef wchar_t * BSTR;

BSTR UTF8toBSTR(unsigned char *in, const int *slen = 0);
void freeBSTR(BSTR b);

#endif