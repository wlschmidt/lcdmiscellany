#include "RegExp.h"
#include "../../malloc.h"
#include "../../stringUtil.h"
#include "../../Unicode.h"
/*
struct RegTerm;

struct Transition {
	RegTerm *term;
};

struct RegTerm {
	RegTerm *prev[2];
	unsigned char flag;
	unsigned char min;
	unsigned char max;
};

#define REGEXP_MINIMAL 1
#define REGEXP_STAR 2
#define REGEXP_PLUS 4
#define REGEXP_QUESTION 8
#define REGEXP_PRE_REQ 16
#define REGEXP_POST_REQ 32
#define REGEXP_OR 64
#define REGEXP_MERGE 128
/*
int AddTerms(RegTerm *&terms, int &maxLen) {
	if (!srealloc(&terms, sizeof(RegTerm) * 2 * maxLen)) {
		return 0;
	}
	memset(terms + maxLen, 0, maxLen*sizeof(RegTerm));
	maxLen *= 2;
	return 1;
}

int CreateTermList(unsigned char* &regExp, int &regExpLen, RegTerm *&terms, int &numTerms, int &maxLen);

int MakeClassCode(unsigned char* &regExp, int &regExpLen, RegTerm *&terms, int &numTerms, int &maxLen, unsigned char *end, int include) {
	unsigned char map[256];
	memset(map, include^1, sizeof(map));
	while (regExp<end) {
		if (regExp[0] == '\\') {
			if (regExp + 1 == end) return 0;
			if (UCASE(regExp[1]) == regExp[1]) {
				int negate = 0;
				unsigned char map2[256];
				memset(map2, 1, 256);
				switch (regExp[1]) {
					case 'W':
						map2['_'] = 0;
					case 'A':
						memset(&map2['0'], 0, 10);
					case 'C':
						memset(&map2['a'], 0, 26);
						memset(&map2['A'], 0, 26);
						negate = 1;
						break;
					case 'D':
						memset(&map2['0'], 0, 10);
						negate = 1;
						break;
					case 'H':
						memset(&map2['0'], 0, 10);
						memset(&map2['a'], 0, 6);
						memset(&map2['A'], 0, 6);
						negate = 1;
						break;
					default:
						map[regExp[1]] = include;
						break;
				}
				if (negate) {
					for (int i=0; i<256; i++) {
						if (map2[i]) map[i] = include;
					}
				}
			}
			else {
				switch (regExp[1]) {
					case 'w':
						map['_'] = include;
					case 'a':
						memset(&map['0'], include, 10);
					case 'c':
						memset(&map['a'], include, 26);
						memset(&map['A'], include, 26);
						break;
					case 'd':
						memset(&map['0'], include, 10);
						break;
					case '\\':
						map['\\'] = include;
						break;
					case 'h':
						memset(&map['0'], include, 10);
						memset(&map['a'], include, 6);
						memset(&map['A'], include, 6);
						break;
					case 'r':
						map['\r'] = include;
						break;
					case 'n':
						map['\n'] = include;
						break;
					case ' ':
						map[' '] = include;
						break;
					case 's':
						map['\r'] = include;
						map['\n'] = include;
						map[' '] = include;
					case 't':
						map['\t'] = include;
						break;
					case 'x':
						if (regExp + 3 >= end) return 0;
						{
							unsigned char c;
							if (regExp[2] >= '0' && regExp[2] <= '9')
								c = regExp[2];
							else if ((regExp[2] >= 'A' && regExp[2] <= 'F') ||
								(regExp[2] >= 'a' && regExp[2] <= 'f')) {
								c = UCASE(regExp[2])-'A'+0xA;
							}
							else return 0;
							if (regExp[3] >= '0' && regExp[3] <= '9')
								c = regExp[3];
							else if ((regExp[3] >= 'A' && regExp[3] <= 'F') ||
								(regExp[3] >= 'a' && regExp[3] <= 'f')) {
								c = c*0x10 + UCASE(regExp[3])-'A'+0xA;
							}
							else return 0;
							map[c] = include;
							regExp+=2;
							regExpLen-=2;
							break;
						}
					default:
						map[regExp[1]] = include;
						break;
				}
			}
			regExp+=2;
			regExpLen-=2;
		}
		else if (regExp + 2 < end && regExp[1] == '-') {
			unsigned int start = regExp[0];
			unsigned int end = regExp[2];
			if (start > end) {
				unsigned int start = regExp[2];
				unsigned int end = regExp[0];
			}
			while(start <= end) {
				map[start] = include;
				start++;
			}
			regExp+=3;
			regExpLen-=3;
		}
		else {
			map[regExp[0]] = include;
			regExp++;
			regExpLen--;
		}
	}
	unsigned int start;
	int prev = 0;
	for (start = 0; start <256; start++) {
		if (map[start]) {
			int end = start;
			while (end+1 < 256 && map[end+1]) end++;
			if (maxLen < numTerms+2 && !AddTerms(terms, maxLen)) return 0;

			terms[numTerms].min = (unsigned char) start;
			terms[numTerms].max = (unsigned char) end;

			if (!prev) {
				prev = numTerms;
				numTerms++;
			}
			else {
				numTerms++;
				terms[numTerms].max = 0xFF;
				terms[numTerms].flag = REGEXP_MERGE;

				terms[prev].prev[0] = &terms[numTerms];
				terms[numTerms-1].prev[0] = &terms[numTerms];
				prev = numTerms;

				terms[numTerms+1].max = 0xFF;
				terms[numTerms+1].prev[0] = &terms[end1];
				terms[numTerms+1].prev[1] = &terms[numTerms-1];
				terms[numTerms+1].flag = REGEXP_OR;
				numTerms+=2;
			}
		}
	}
	return prev;
}

int Pop(unsigned char *&regExp, int &regExpLen, RegTerm *&terms, int &numTerms, int &maxLen) {
	int first;
	if (!regExpLen || maxLen < numTerms && !AddTerms(terms, maxLen)) return 0;
	switch (regExp[0]) {
		case '.':
			terms[numTerms].max = 0xFF;
			first = numTerms;
			numTerms++;
			break;
		case '*':
		case '+':
		case '{':
		case '}':
		case '?':
		case ')':
			return 0;
		case '[':
			{
				regExp++;
				regExpLen--;
				if (!regExpLen) return 0;
				int include = 1;
				if (regExp[0] == '^') {
					include = 0;
					regExp++;
					regExpLen--;
				}
				unsigned char *e = regExp+1;
				while (e < regExp+regExpLen) {
					if (*e == '\\') e++;
					else if (*e == ']') break;
					e++;
				}
				if (e >= regExp+regExpLen) return 0;
				first = MakeClassCode(regExp, regExpLen, terms, numTerms, maxLen, e, include);
				if (!first) return 0;
			}
			break;
		case '\\':
			if (regExpLen < 2) return 0;
			if (regExp[1] == 'x') {
				if (regExpLen < 4) return 0;
				first = MakeClassCode(regExp, regExpLen, terms, numTerms, maxLen, regExp+4, 1);
			}
			else
				first = MakeClassCode(regExp, regExpLen, terms, numTerms, maxLen, regExp+2, 1);
			if (!first) return 0;
			break;
		case '(':
			first = CreateTermList(++regExp, --regExpLen, terms, numTerms, maxLen);
			if (!first) return 0;
			if (!regExpLen || regExp[0] != ')') return 0;
			break;
		default:
			terms[numTerms].max = 
				terms[numTerms].min = regExp[0];
			first = numTerms;
			numTerms++;
			break;
	}
	regExp++;
	regExpLen--;
	while (regExpLen) {
		if (maxLen < numTerms && !AddTerms(terms, maxLen)) return 0;
		int min = 0;
		if (regExpLen > 1 && regExp[1] == '?') min = 1;
		switch (regExp[0]) {
			case '*':
				terms[numTerms].max = 0xFF;
				terms[numTerms].prev[1] = &terms[numTerms-1];
				terms[numTerms].flag = REGEXP_STAR;
				terms[first].prev[0] = &terms[first];
				first = numTerms;
				break;
			case '+':
				if (
			case '{':
			case '}':
			case '?':
				return 0;
			default:
				return first;
		}
		if (min)
			terms[numTerms].flag = REGEXP_NON_GREEDY;
		numTerms++;
		regExp+=min+1;
		regExpLen-=min+1;
	}
	return first;
}

int CreateTermList(unsigned char *&regExp, int &regExpLen, RegTerm *&terms, int &numTerms, int &maxLen) {
	int prev = Pop(regExp, regExpLen, terms, numTerms, maxLen);
	if (!prev) return 0;
	while(regExpLen) {
		if (regExp[0] == ')' || regExp[0] == '}') return prev;

		int end1 = numTerms-1;
		if (regExp[0] == '|') {
			regExp++;
			regExpLen--;
			int prev2 = CreateTermList(regExp, regExpLen, terms, numTerms, maxLen);
			if (!prev2) return 0;
			if (maxLen < numTerms-2 && !AddTerms(terms, maxLen)) return 0;

			terms[numTerms].max = 0xFF;
			terms[numTerms].flag = REGEXP_MERGE;

			terms[prev].prev[0] = &terms[numTerms];
			terms[numTerms-1].prev[0] = &terms[numTerms];
			prev = numTerms;

			terms[numTerms+1].max = 0xFF;
			terms[numTerms+1].prev[0] = &terms[end1];
			terms[numTerms+1].prev[1] = &terms[numTerms-1];
			terms[numTerms+1].flag = REGEXP_OR;
			numTerms+=2;
		}
		else {
			int prev2 = Pop(regExp, regExpLen, terms, numTerms, maxLen);
			if (!prev2) return 0;
			terms[prev2].prev[0] = &terms[end1];
		}
	}
	return prev;
}

RegTerm *CreateTerms(unsigned char *regExp, int regExpLen) {
	int numTerms = 1;
	int maxLen = regExpLen+ 50;
	RegTerm *terms = (RegTerm *) calloc(maxLen, sizeof(RegTerm));
	if (!terms) return 0;
	memset(&terms[0], 0, sizeof(terms[0]));
	terms[0].max = 255;
	terms[0].flag = REGEXP_STAR | REGEXP_MINIMAL;
	terms[0].prev[0] = terms;
	int regExpLen2 = regExpLen;
	unsigned char *regExp2 = regExp;
	int i =CreateTermList(regExp2, regExpLen2, terms, numTerms, maxLen);
	return 0;
}//*/

struct Mark {
	unsigned char *start;
	unsigned char *end;
};

static bool forceEnd;

static int MatchChar(unsigned int stringChar, unsigned char *query, int &queryEnd) {
	unsigned int c2;
	int c2len;
	switch(*query) {
		case '\\':
			c2 = NextUnicodeChar(query+1, &c2len);
			queryEnd = 1+c2len;
			switch (c2) {
				case 'n':
					if (stringChar == '\n') return 1;
					if (stringChar == '\r') return 1;
					return 0;
				case 'x':
					if ((stringChar>='A' && stringChar<='F') ||(stringChar>='a' && stringChar<='f')) return 1;
					return (stringChar == '\n');
				case 'd':
					return (stringChar>='0' && stringChar<='9');
				case 'a':
					return (stringChar>='A' && stringChar<='Z') || (stringChar>='a' && stringChar<='z');
				case 'u':
					return (stringChar>='A' && stringChar<='Z');
				case 'l':
					return (stringChar>='a' && stringChar<='z');
				case 'w':
					return (stringChar == ' ' || stringChar == '\t');
				case 'W':
					return (stringChar == ' ' || stringChar == '\t' || stringChar == '\r' || stringChar == '\n');
				case 'm':
					return (stringChar>='A' && stringChar<='Z') || (stringChar>='a' && stringChar<='z') || (stringChar>='0' && stringChar<='9');
				default:
					return stringChar==c2;
			}
		default:
			c2 = NextUnicodeChar(query, &c2len);
			return c2 == stringChar;
		case '.':
			queryEnd = 1;
			return (stringChar != '\n' && stringChar != '\r' && stringChar);
		case '#':
			return (stringChar>='0' && stringChar<='9');
		case '[':
			{
				int p = 1;
				int neg = (query[1] == '^');
				p += neg;
				int hit = neg;
				int flip = !hit;
				if (query[p] == ']') {
					if (stringChar == ']') {
						hit = flip;
					}
					p++;
				}
				if (query[p] == '-') {
					if (stringChar == '-') {
						hit = flip;
					}
					p++;
				}
				while (query[p] != ']') {
					c2 = NextUnicodeChar(query+p, &c2len);
					if (!c2) {
						queryEnd = 0;
						return 0;
					}
					if (c2 == '\\' || c2 == '.') {
						if (MatchChar(stringChar, query+p, c2len)) {
							hit = flip;
						}
						if (!c2len) {
							queryEnd = 0;
							return 0;
						}
						p += c2len;
					}
					else if (c2 == '-') {
						queryEnd = 0;
						return 0;
					}
					else {
						p+= c2len;
						if (query[p] == '-') {
							p+=1;
							unsigned int c3 = NextUnicodeChar(query+p, &c2len);
							if (!c3) {
								queryEnd = 0;
								return 0;
							}
							if (c3 < c2) {
								unsigned int c = c2;
								c2 = c3;
								c3 = c;
							}
							if (c2 <= stringChar && stringChar<=c3) {
								hit = flip;
							}
							p+=c2len;
						}
						else {
							if (c2 == stringChar) {
								hit = flip;
							}
						}
					}
				}
				queryEnd = p+1;
				return hit;
			}
	}
}

static int MatchRegExp(unsigned char *string, int stringlen, unsigned char *query, int querylen, Mark *hits, int mark) {
	while (1) {
		if (querylen < 2) {
			if (!querylen) return ((hits[mark].start == 0 || hits[mark].end) && (!forceEnd || !stringlen));
			if (querylen == 1 && query[0] == '$') {
				return stringlen == 0;
			}
		}
		//if (!stringlen) return 0;
		unsigned int c1;//, c2;
		int c1len;//, c2len;
		c1 = NextUnicodeChar(string, &c1len);
		//c2 = NextUnicodeChar(query, &c2len);
		//int queryEnd = c2len;
		switch (query[0]) {
			case '*':
			case '?':
			case '+':
				return 0;
			case '(':
				if (!hits[mark].start) {
					hits[mark].start = string;
					if (!MatchRegExp(string, stringlen, query+1, querylen-1, hits, mark)) {
						hits[mark].start = 0;
						return 0;
					}
					return 1;
				}
				goto simple;
			case ')':
				if (hits[mark].start) {
					hits[mark].end = string;
					if (!MatchRegExp(string, stringlen, query+1, querylen-1, hits, mark+1)) {
						hits[mark].end = 0;
						return 0;
					}
					return 1;
				}
				goto simple;
	simple:
			default:
				{
					int hit = 0;
					int queryEnd = 1;
					hit = MatchChar(c1, query, queryEnd);
					hit &= (stringlen != 0);
					if (!queryEnd) return 0;
					if (query[queryEnd] == '?') {
						if (query[queryEnd+1] == '?') {
							if (MatchRegExp(string, stringlen, query+queryEnd+2, querylen-(queryEnd+2), hits, mark))
								return 1;
							return hit &&
								MatchRegExp(string+c1len, stringlen-c1len, query+queryEnd+2, querylen-(queryEnd+2), hits, mark);
						}
						if (hit && MatchRegExp(string+c1len, stringlen-c1len, query+queryEnd+1, querylen-(queryEnd+1), hits, mark))
							return 1;
						return MatchRegExp(string, stringlen, query+queryEnd+1, querylen-(queryEnd+1), hits, mark);
					}
					else if (query[queryEnd] == '+' || query[queryEnd] == '*' || query[queryEnd] == '{') {
						int min = 0, max = stringlen;
						queryEnd++;
						if (query[queryEnd-1] == '+') {
							if (!hit) return 0;
							string += c1len;
							stringlen -= c1len;
						}
						else if (query[queryEnd-1] == '{') {
							min = 0;
							max = stringlen;
							while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
							unsigned char *p;
							if (query[queryEnd] != ',') {
								__int64 w = strtoi64(query+queryEnd, &p, 0);
								if (w > 1000000 || p == query+queryEnd || w<0) return 0;
								queryEnd = (int)(p - query);
								min = (int)w;
								while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
								if (query[queryEnd] == '}') {
									max = min;
									queryEnd++;
								}
								else if (query[queryEnd++] == ',') {
									while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
									if (query[queryEnd] == '}') {
										queryEnd++;
									}
									else {
										w = strtoi64(query+queryEnd, &p, 0);
										if (w > 1000000 || p == query+queryEnd || w<0) return 0;
										queryEnd = (int)(p - query);
										max = (int)w;
										while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
										if (query[queryEnd++] != '}') return 0;
									}
								}
								else return 0;
							}
							else if (query[queryEnd++] == ',') {
								while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
								if (query[queryEnd] == '}') {
									queryEnd++;
								}
								else {
									__int64 w = strtoi64(query+queryEnd, &p, 0);
									if (w > 1000000 || p == query+queryEnd || min>w) return 0;
									queryEnd = (int)(p - query);
									max = (int)w;
									while (query[queryEnd] == ' ' || query[queryEnd] == '\t') queryEnd++;
									if (query[queryEnd++] != '}') return 0;
								}
							}
							else return 0;
						}
						if (query[queryEnd] != '?') {
							int matches=0, pos = 0;
							int queryEnd2;
							while (pos < stringlen && matches<max) {
								c1 = NextUnicodeChar(string+pos, &c1len);
								if (!c1) return 0;
								if (!MatchChar(c1, query, queryEnd2)) break;
								pos += c1len;
								matches++;
							}
							for (int i=matches; i>=min; i--) {
								int matches2 = 0, pos = 0;
								while (matches2 < i) {
									c1 = NextUnicodeChar(string+pos, &c1len);
									pos += c1len;
									matches2++;
								}
								if (MatchRegExp(string+pos, stringlen-pos, query+queryEnd, querylen-queryEnd, hits, mark))
									return 1;
							}
							return 0;
						}
						else {
							int matches=0, pos = 0;
							queryEnd++;
							int queryEnd2;
							while (pos < stringlen && matches<max) {
								if (matches >= min && MatchRegExp(string+pos, stringlen-pos, query+queryEnd, querylen-queryEnd, hits, mark))
									return 1;
								c1 = NextUnicodeChar(string+pos, &c1len);
								if (!c1) {
									return 0;
								}
								if (!MatchChar(c1, query, queryEnd2)) {
									return 0;
								}
								pos += c1len;
								matches++;
							}
							return MatchRegExp(string+pos, stringlen-pos, query+queryEnd, querylen-queryEnd, hits, mark);
						}
						/*if (query[queryEnd+1] == '?') {
						}
						if (hit) return MatchRegExp(string+c1len, stringlen-c1len, query+queryEnd, querylen-queryEnd, hits, mark);
						//*/
					}
					else {
						if (!hit) return 0;
						string += c1len;
						stringlen -= c1len;
						query += queryEnd;
						querylen -= queryEnd;
					}
					/*else if (query[queryEnd] == '*') {
						if (query[queryEnd+1] == '?') {
							if (MatchRegExp(string, stringlen, query+queryEnd+2, querylen-(queryEnd+2), hits, mark))
								return 1;
							return hit &&
								MatchRegExp(string+c1len, stringlen-c1len, query+queryEnd+2, querylen-(queryEnd+2), hits, mark);
						}
						if (hit && MatchRegExp(string+c1len, stringlen-c1len, query+queryEnd+1, querylen-(queryEnd+1), hits, mark))
							return 1;
						return MatchRegExp(string, stringlen, query+queryEnd+1, querylen-(queryEnd+1), hits, mark);
					}//*/
				}
		}
	}
}

void RegExp(ScriptValue &s, ScriptValue *args) {
	if ((unsigned int)args[0].stringVal->len < (unsigned __int64)args[2].intVal) return;
	int len1 = args[0].stringVal->len - args[2].i32;
	unsigned char *s1 = args[0].stringVal->value + args[2].i32;
	unsigned char *s2 = args[1].stringVal->value;
	int len2 = args[1].stringVal->len;
	Mark *hits = (Mark*) calloc(2+args[1].stringVal->len, sizeof(Mark));
	if (hits) {
		int match = 0;
		//forceEnd = 0;
		/*if (len1 && s1[len1-1]=='$' && (len1<2 || s1[len1-2] != '\\')) {
			forceEnd = 1;
			len1 --;
		}//*/

		if (s2[0] == '^') {
			match = MatchRegExp(s1, len1, s2+1, len2-1, hits, 0);
		}
		else {
			for (int i=0; i<len1; i++) {
				/*if (s1[i] == 'n' && s1[i+1] == 'a' &&s1[i+6]=='M' &&s1[i+7]=='e') {
					i=i;
				}//*/
				match = MatchRegExp(s1+i, len1-i, s2, len2, hits, 0);
				if (match) {
					break;
				}
			}
		}
		if (match) {
			int i=0;
			for (i=0; hits[i].start; i++);
			s1 -= args[2].i32;
			if (CreateListValue(s, i+1)) {
				for (i=0; hits[i].start; i++) {
					ScriptValue sv;
					if (CreateListValue(sv, 2)) {
						if (!CreateStringValue(sv.listVal->vals[0], hits[i].start, (int)(hits[i].end-hits[i].start))) {
							sv.listVal->Release();
						}
						else {
							CreateIntValue(sv.listVal->vals[1], hits[i].start-args[0].stringVal->value);
							//CreateIntValue(sv.listVal->vals[2], hits[i].end-hits[i].start);
							sv.listVal->numVals = 2;
							s.listVal->PushBack(sv);
						}
					}
				}
			}
		}
		free(hits);
	}
	hits = hits;
	//return;
	/*
	unsigned char *regExp = args[1].stringVal->value;
	int regExpLen = args[1].stringVal->len;
	if (regExp[0] == '|' || regExp[0] == '*' || regExp[0] == '?' || regExp[0] == '+' || regExp[0] == '{')
		return;
	unsigned char *str = args[0].stringVal->value;
	int len = args[0].stringVal->len;

	RegTerm *terms = CreateTerms(regExp, regExpLen);
	terms = terms;
	//*/
}
