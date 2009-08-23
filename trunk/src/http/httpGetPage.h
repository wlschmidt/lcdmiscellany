#ifndef HTTP_GET_PAGE_H
#define HTTP_GET_PAGE_H

#include "../Network.h"

//void CheckIdleHttpSockets(__int64 time, __int64 timeFix);

enum TranferProtocol {PROTOCOL_HTTP, PROTOCOL_HTTPS, PROTOCOL_UNKNOWN};
enum HttpStatus {STATUS_GETTING_IP, STATUS_CONNECTING, STATUS_READING_HEADER, STATUS_READING_FILE, STATUS_FAILED, STATUS_SUCCESS, STATUS_CONFUSED, STATUS_NO_LENGTH};

// Should be more than enough.
#define MAX_FILE_SIZE (50*1024*1024)

struct HTTPFile {
	int len;
	char *data;
	inline void Cleanup() {
		free(data);
		free(this);
	}
};

struct BufferedHttpSocket {
	__int64 expected;
	HTTPFile *result;
	ConnectCallback * connectCallback;
	//addrinfo *firstAddr;
	//addrinfo *currentAddr;
	char *type;
	char *outBuffer;
	char *dns;
	char *path;
	char *buffer;
	LPARAM lParam;
	int startTime;
	int timeoutTime;
	HWND hWnd;
	int completionMessage;
	int expectedLength;
	int bufferLen;
	int outBufferLen;
	unsigned int protocol;
	//SOCKET sock;
	int status;
	unsigned short port;
	short gzip;
	int id;
	int httpMajor;
	int httpMinor;
	int httpCode;
	int close;
	int chunked;

	char *userName;
	char *pass;

	char *url;

	void SendRequest(char *location);
	void SendData();
	void TryNext();
	void Try();
	void Cleanup();
	// Disconnects and clears buffers.  Keeps any recieved data.
	void Hangup();
	void ReadData();
	void MakeResult(int size);
	void CloseConnection(int error);
	//void TryConnect();
};

BufferedHttpSocket *HTTPGet(char *url, HWND hWnd, int msg, LPARAM lParam=0);
void CleanupHttpSocket(int id);
void CleanupHttpSockets();
//void StartHttpConnection(int id);
BufferedHttpSocket *GetBufferedHttpSocket(int id);


//int SpawnGetPageThread(char *url, HWND hWnd, int message);
void HandleHttpSocketMessage(WPARAM socket, LPARAM lParam);

int ParseHTMLHeader(int &argc, char ** &argv, char *s, int size);


// If it's a path (Which can have '/' in it), then path should be 1
// do not escape forward slashes.  Think I escape one or two more characters
// than strictly necessary ('*' in particular), but who cares.
// All characters before start are copied directly.
char *HTTPEscape(const char *s, const int len, int path = 0, int start=0);
char *HTTPUnescape(const char *s, int *len);

char *GetDNSAndPath(const char *url, char *&path, unsigned short &port, unsigned int &protocol, char *&userName, char *&pass, int host = 0);


#endif
