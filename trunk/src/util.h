#ifndef UTIL_H
#define UTIL_H
//#include "global.h"
//#include "stringUtil.h"
extern "C" {
	#include "FileReader.h"
}
#include <stdio.h>
// Lotta random util functions...Most not used a whole lot, but might use some elsewhere eventually.

// Currently don't record state, as it has to be called once per thread.  Not using it in multiple threads,
// but could change that later.

int InitCom();
void UninitCom();
void KillCom();
void UtilInit();

typedef struct _FILETIME FILETIME;
double FileTimeToUsefulTime(FILETIME *t);

int getPriv(wchar_t *name);

int getShutdownPriv();
int getDebugPriv();

int KillProcess(int id);
int SetProcessPriority(int id, int priority);
int GetProcessPriority(int id, int &priority);

unsigned __int64 GetTickCountNoOverflow();
unsigned __int64 GetTimeAccurate();

void DecodeChar(int &message, int &scancode);

wchar_t *GetFile(wchar_t *name);

wchar_t *ResolvePathRel(unsigned char *base);
wchar_t *ResolvePathRelSteal(wchar_t *base);

#ifndef min
#define min(x,y) (((x) <= (y)) ? (x) : (y))
#define max(x,y) (((x) <= (y)) ? (y) : (x))
#endif

#define DOUBLE_BREAK	0
#define SINGLE_LINE		1
#define WHOLE_FILE		2

#define ASCII	0
#define UTF8	1
#define UTF16	2
#define UTF16R	3
#define EUC_JP	51932
#define JIS		50222

unsigned __int64 GetNumber();

// Note:  In practice, the buffering only makes a big difference in debug builds.
class FileLineReader {
private:
public:
	unsigned char *comments;
	unsigned char *buffer;
	wchar_t *utf16;
	int used;
	int bufferSize;
	int lineStart;
	int maxLineLen;
	int initialBufferSize;
	// Note:  -1 indicates still reading data, so buffer is full up to the end.
	// Otherwise, has last valid value.
	int bufferEnd;
	FileReader *file;
	int ignoreEmptyLines;
	int fileType;
	int outputType;
	int lb;

	// Note:  the largest the buffer will ever be is maxLineLen+initialBufferSize+1.
	// Any lines that are too long will be truncated.
	FileLineReader(FileReader* f, int ignoreEmptyLines, int type, int initialBufferSize = 50000, int maxLineLen = 1000000, const unsigned char *commentChars = 0);

	// Returns null object on fail.  Just makes failure simpler
	// and keeps it working like fopen, though needs to be deleted
	// instead of closed.
	friend FileLineReader* OpenLineReader(wchar_t *fileName, int ignoreEmptyLines, int type, const unsigned char *commentChars = 0, int maxLineLen = 1000000);

	// Returns a pointer to the next line (line ends in null and with line breaks,
	// a carriage return preceeding a linebreak is ignored.
	// Empty strings can be returned if ignoreEmptyLines is 0.
	// When the end is reached, -1 is returned and *line is set to null.
	// Calling it again will invalidate the last string returned.
	int NextLine(unsigned char **line);
	int NextLine(wchar_t **line, int keep);
	~FileLineReader();
};

FileLineReader* OpenLineReader(unsigned char *fileName, int ignoreEmptyLines, int type, const char *commentChars, int maxLineLen = 1000000);

inline FileLineReader* OpenLineReader(wchar_t *fileName, int ignoreEmptyLines, int type, const char *commentChars, int maxLineLen = 1000000) {
	return OpenLineReader(fileName, ignoreEmptyLines, type, (unsigned char*) commentChars, maxLineLen);
}

/*
inline unsigned char *Buffer(wchar_t *fileName) {
	FileLineReader *in = OpenLineReader(fileName, 0, WHOLE_FILE);
	if (!in) return 0;
	unsigned char *line;
	int len = in->NextLine(&line);
	unsigned char *out = 0;
	if (len) {
		out = (unsigned char*) memdup(line, sizeof(unsigned char) * (len+1));
	}
	delete in;
	return out;
}//*/
int GetLine(FILE* in, char *&line, int &lineLen);

int getLine(FILE* in, char *line);

void Delete(const char *fileName);

// Resolves a path.  Converts UTF-8 to unicode for more compact storage.
// Adds \\?\ if necessary for local paths.  Does nothing to urls.
wchar_t *ResolvePath(unsigned char *base);
wchar_t *ResolvePath(wchar_t *base);
// Destroys base if modified, returns it otherwise.
wchar_t *ResolvePathSteal(wchar_t *base);

// Adds add to end of base, if add is not an absolute path.
// works for URLs and paths.
wchar_t *ResolvePath(unsigned char *base, unsigned char* add);
wchar_t *ResolvePath(wchar_t *base, wchar_t* add);

extern const wchar_t windows1252Map[128];

#endif
