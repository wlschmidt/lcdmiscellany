#include "unicode.h"
#include "malloc.h"
#include "stringUtil.h"

unsigned long NextUnicodeChar(unsigned char *in, int *len) {
	if (in[0]<0x80) {
		*len = 1;
		return in[0];
	}
	else if (in[0]<0xE0) {
		if (in[0]>=0xC0 &&
			(in[1]&0xC0)== 0x80) {

				*len = 2;
				return
					((((unsigned int)in[0])&0x1F)<<6) |
					(((unsigned int)in[1])&0x3F);
		}
	}
	else if (in[0]<0xF0) {
		// Shouldn't happen.
		if ((in[1]&0xC0) == 0x80 && 
			(in[2]&0xC0) == 0x80) {
				*len = 3;
				return
					((((unsigned int)in[0])&0x0F)<<12) |
					((((unsigned int)in[1])&0x3F)<<6) |
					(((unsigned int)in[2])&0x3F);
		}
	}
	else if (in[0]<0xF8) {
		// Shouldn't happen.
		if ((in[1]&0xC0)== 0x80 && 
			(in[2]&0xC0)== 0x80 && 
			(in[3]&0xC0)== 0x80) {

				*len = 4;
				return
					((((unsigned int)in[0])&0x07)<<18) |
					((((unsigned int)in[1])&0x3F)<<12) |
					((((unsigned int)in[2])&0x3F)<<6) |
					(((unsigned int)in[3])&0x3F);
		}
	}
	*len = 1;
	return '?';
}


int UTF16toASCII(unsigned char *out, const wchar_t *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0x100) {
			out[outpos ++] = (char)in[inpos++];
			if (len == -1 && !in[inpos-1]) break;
		}
		else if (in[inpos]<0xD800 || in[inpos] > 0xE000) {
			inpos++;
			out[outpos ++] = '?';
		}
		else if (in[inpos]<0xDC00) {
			inpos+=2;
			out[outpos ++] = '?';
		}
		else {
			// Not even a valid unicode char.
			out[outpos++] = '?';
			inpos++;
		}
	}
	return outpos;
}

int UTF8toUTF32(unsigned int *out, const unsigned char *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0x80) {
			out[outpos] = in[inpos];
			if (in[inpos] == 0 && len == -1)
				break;
			inpos++;
			outpos++;
		}
		else if (in[inpos]<0xE0) {
			// Shouldn't happen.
			if (in[inpos]<0xC0 ||
				inpos+1 >= len || 
				(in[inpos+1]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] =
				((((unsigned int)in[inpos])&0x1F)<<6) |
				(((unsigned int)in[inpos+1])&0x3F);
			inpos+=2;
		}
		else if (in[inpos]<0xF0) {
			// Shouldn't happen.
			if (inpos+2 >= len || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] =
				((((unsigned int)in[inpos])&0x0F)<<12) |
				((((unsigned int)in[inpos+1])&0x3F)<<6) |
				(((unsigned int)in[inpos+2])&0x3F);
			inpos+=3;
		}
		else if (in[inpos]<0xF8) {
			// Shouldn't happen.
			if (inpos+3 >= len || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80 || 
				(in[inpos+3]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] =
				((((unsigned int)in[inpos])&0x07)<<18) |
				((((unsigned int)in[inpos+1])&0x3F)<<12) |
				((((unsigned int)in[inpos+2])&0x3F)<<6) |
				(((unsigned int)in[inpos+3])&0x3F);
			inpos+=4;
		}
		else {
			out[outpos++] = '?';
			inpos++;
		}
	}
	return outpos;
}


int UTF32toUTF8(unsigned char *out, const unsigned int *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0x80) {
			out[outpos] = in[inpos];
			if (in[inpos] == 0 && len == -1)
				break;
			outpos++;
			inpos++;
		}
		else if (in[inpos]<0x800) {
			out[outpos] =0xC0 | (in[inpos] >> 6);
			out[outpos+1] =0x80 | (in[inpos] & 0x3F);
			outpos+=2;
			inpos++;
		}
		else if (in[inpos]<0x10000) {
			out[outpos] =0xE0 | (in[inpos] >> 12);
			out[outpos+1] =0x80 | ((in[inpos]>>6) & 0x3F);
			out[outpos+2] =0x80 | (in[inpos] & 0x3F);
			outpos+=3;
			inpos++;
		}
		else if (in[inpos]<0x200000) {
			out[outpos] =0xF0 | (in[inpos] >> 18);
			out[outpos+1] =0x80 | ((in[inpos]>>12) & 0x3F);
			out[outpos+2] =0x80 | ((in[inpos]>>6) & 0x3F);
			out[outpos+3] =0x80 | (in[inpos] & 0x3F);
			outpos+=4;
			inpos++;
		}
		else {
			out[outpos++] = '?';
			inpos++;
		}
	}
	return outpos;
}




int UTF8toUTF16(wchar_t *out, const unsigned char *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0x80) {
			out[outpos] = in[inpos];
			if (in[inpos] == 0 && len == -1)
				break;
			outpos++;
			inpos++;
		}
		else if (in[inpos]<0xE0) {
			// Shouldn't happen.
			if (in[inpos]<0xC0 ||
				(len!=-1 && inpos+1 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] =
				((((wchar_t)in[inpos])&0x1F)<<6) |
				(((wchar_t)in[inpos+1])&0x3F);
			inpos+=2;
		}
		else if (in[inpos]<0xF0) {
			// Shouldn't happen.
			if ((len!=-1 && inpos+2 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] =
				((((wchar_t)in[inpos])&0x0F)<<12) |
				((((wchar_t)in[inpos+1])&0x3F)<<6) |
				(((wchar_t)in[inpos+2])&0x3F);
			inpos+=3;
		}
		else if (in[inpos]<0xF8) {
			// Shouldn't happen.
			if ((len!=-1 && inpos+3 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80 || 
				(in[inpos+3]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			unsigned int out32 =
				((((unsigned int)in[inpos])&0x07)<<18) |
				((((unsigned int)in[inpos+1])&0x3F)<<12) |
				((((unsigned int)in[inpos+2])&0x3F)<<6) |
				(((unsigned int)in[inpos+3])&0x3F);
			out[outpos] = 0xD800 + (out32 >> 10) - 1;
			out[outpos+1] = 0xDC00 + (out32 & ~0xFC00);
			outpos +=2;
			inpos+=4;
		}
		else {
			out[outpos++] = '?';
			inpos++;
		}
	}
	out[outpos]=0;
	return outpos;
}


int UTF16toUTF8(unsigned char *out, const wchar_t *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0xD800 || in[inpos] > 0xE000) {
			if (in[inpos]<0x80) {
				out[outpos] = (unsigned char) in[inpos];
				if (in[inpos] == 0 && len == -1)
					break;
				outpos++;
				inpos++;
			}
			else if (in[inpos]<0x800) {
				out[outpos] =0xC0 | (in[inpos] >> 6);
				out[outpos+1] =0x80 | (in[inpos] & 0x3F);
				outpos+=2;
				inpos++;
			}
			else  {
				out[outpos] =0xE0 | (in[inpos] >> 12);
				out[outpos+1] =0x80 | ((in[inpos]>>6) & 0x3F);
				out[outpos+2] =0x80 | (in[inpos] & 0x3F);
				outpos+=3;
				inpos++;
			}
		}
		else if (in[inpos]<0xDC00) {
			if (inpos + 1 == len || (in[inpos+1] & 0xFC00) != 0xDC00) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			unsigned int out32 = (((unsigned int)in[inpos] & ~0xFC00) << 10) + 0x10000 + ((unsigned int)in[inpos+1] & ~0xFC00);
			inpos += 2;
			out[outpos] =0xF0 | (out32 >> 18);
			out[outpos+1] =0x80 | ((out32>>12) & 0x3F);
			out[outpos+2] =0x80 | ((out32>>6) & 0x3F);
			out[outpos+3] =0x80 | (out32 & 0x3F);
			outpos+=4;
		}
		else {
			out[outpos++] = '?';
			inpos++;
		}
	}
	return outpos;
}

wchar_t * UTF8toUTF16Alloc(const unsigned char *in, int *slen) {
	if (!in) return 0;
	int len = -1;
	if (slen) len = *slen;
	if (len < 0)
		len = (int)strlen(in);
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t)*(1+len));
	if (!out) return 0;
	int L = UTF8toUTF16(out, in, -1);
	if (L < len)
		srealloc(out, sizeof(wchar_t)*(L+1));
	out[L] = 0;
	if (slen) *slen = L;
	return out;
}
unsigned char * UTF16toUTF8Alloc(const wchar_t *in, int *slen) {
	if (!in) return 0;
	int len = -1;
	// Note:  for minor optimization only, not to ignore terminal null.
	if (slen)
		len = *slen;
	if (len < 0)
		len = (int)wcslen(in);
	unsigned char *out = (unsigned char*) malloc(sizeof(unsigned char)*(1+3*len));
	if (!out) return 0;
	int L = UTF16toUTF8(out, in, len);
	if (L < len)
		srealloc(out, sizeof(unsigned char)*(L+1));
	out[L] = 0;
	if (slen) * slen = L;
	return out;
}

int ASCIItoUTF8(unsigned char *out, const char *in, int len) {
	unsigned char *start = out;
	while (len && (*in || len > 0)) {
		if (((unsigned char*)in)[0] < 0x80) {
			out++[0] = in++[0];
		}
		else {
			out++[0] = 0xC2 + (((unsigned char*)in)[0]>=0xC0);
			out++[0] = 0x80 + (((unsigned char*)in++)[0]&0x3F);
		}
	}
	out[0] = 0;
	return (int)(out - start);
}

int UTF8toASCII(char *out, const unsigned char *in, int len) {
	int inpos = 0;
	int outpos = 0;
	while (inpos < len || len == -1) {
		if (in[inpos]<0x80) {
			out[outpos++] = in[inpos];
			if (in[inpos] == 0 && len == -1)
				break;
			inpos++;
		}
		else if (in[inpos]<0xE0) {
			// Shouldn't happen.
			if (in[inpos]<0xC0 ||
				(len!=-1 && inpos+1 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			int outc = 
				((((wchar_t)in[inpos])&0x1F)<<6) |
				(((wchar_t)in[inpos+1])&0x3F);
			if (outc < 256)
				out[outpos] = ((char*)&outc)[0];
			else
				out[outpos] = '?';
			outpos++;
			inpos+=2;
		}
		else if (in[inpos]<0xF0) {
			// Shouldn't happen.
			if ((len!=-1 && inpos+2 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] = '?';
			inpos+=3;
		}
		else if (in[inpos]<0xF8) {
			// Shouldn't happen.
			if ((len!=-1 && inpos+3 >= len) || 
				(in[inpos+1]&0xC0)!= 0x80 || 
				(in[inpos+2]&0xC0)!= 0x80 || 
				(in[inpos+3]&0xC0)!= 0x80) {
				out[outpos++] = '?';
				inpos++;
				continue;
			}
			out[outpos++] = '?';
			inpos+=4;
		}
		else {
			out[outpos++] = '?';
			inpos++;
		}
	}
	return outpos;
}

char * UTF8toASCIIAlloc(const unsigned char *in) {
	if (!in) return 0;
	char *out = (char*)strdup(in);
	if (!out) return 0;
	int L = UTF8toASCII(out, in, -1);
	srealloc(out, sizeof(char)*(L+1));
	return out;
}

unsigned char * ASCIItoUTF8Alloc(const char *in) {
	if (!in) return 0;
	unsigned char *out = (unsigned char*)malloc(sizeof(unsigned char) * (2*strlen(in)+1));
	if (!out) return 0;
	int L = ASCIItoUTF8(out, in, -1);
	srealloc(out, sizeof(unsigned char)*(L+1));
	return out;
}

BSTR UTF8toBSTR(unsigned char *in, const int *slen) {
	if (!in) return 0;
	int len = -1;
	if (slen) len = *slen;
	if (len < 0)
		len = (int)strlen(in);
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t)*(1+len+4));
	if (!out) return 0;
	int L = UTF8toUTF16(out+2, in, -1);
	if (L < len)
		srealloc(out, 4 + sizeof(wchar_t)*(L+1));
	out[2+L] = 0;
	((unsigned int*)out)[0] = len;
	return (BSTR)(out+2);
}

void freeBSTR(BSTR b) {
	free(((char*)b)-4);
}
