#ifndef NETWORK_H
#define NETWORK_H
#include "global.h"
#include <Winsock2.h>
#include <Ws2tcpip.h>

typedef int (WSAAPI *TGetAddrInfoW) (
  PCWSTR pNodeName,
  PCWSTR pServiceName,
  const ADDRINFOW* pHints,
  PADDRINFOW* ppResult
);

typedef int (WSAAPI *TGetNameInfoW) (
  __in   const SOCKADDR *pSockaddr,
  __in   socklen_t SockaddrLength,
  __out  PWCHAR pNodeBuffer,
  __in   DWORD NodeBufferSize,
  __out  PWCHAR pServiceBuffer,
  __in   DWORD ServiceBufferSize,
  __in   INT Flags
);

typedef void (WSAAPI *TFreeAddrInfoW) (IN  PADDRINFOW      pAddrInfo);

extern TGetAddrInfoW pGetAddrInfoW;
extern TFreeAddrInfoW pFreeAddrInfoW;
extern TGetNameInfoW pGetNameInfoW;


//Note:  Must cleanup addrinfo.
typedef void NetProc (addrinfo *, __int64 id);

struct ConnectCallback;

typedef void FailProc (__int64 id);
typedef int InitProc (ConnectCallback*, __int64 id);

int InitSockets();
void CleanupSockets();

struct ConnectCallback {
	__int64 id;
	addrinfo *firstAddr;
	addrinfo *currentAddr;
	FailProc *failProc;
	InitProc * initProc;
	HWND hWnd;
	// Same as SOCKET.  Just gets headers out of the way
	UINT_PTR sock;
	// IPPROTO_TCP or IPPROTO_UDP.
	unsigned int proto;
	int index;
	unsigned int message;
};
void CleanupCallback(ConnectCallback *callback);
void TryNextSocket(ConnectCallback *callback);
void TrySocket(ConnectCallback *callback);

// Creates socket, binds if ip and/or port is specified.  Returns null on failure.  Won't look up DNS.
ConnectCallback * CreateUDPCallback(char *ip, unsigned short port, UINT message, HWND hWnd, __int64 id);

struct DNSData {
	__int64 id;
	addrinfo *firstAddr;
	NetProc *netProc;
	FailProc *failProc;
	InitProc * initProc;
	char *dns;
	HANDLE hThread;
	HWND hWnd;
	unsigned int message;
	int index;
	unsigned int type;
	unsigned short port;
};

int LookupDNSThreadConnectCallback(char *dns, unsigned short port, int type, FailProc *failProc, InitProc *initProc, unsigned int message, HWND hWnd, __int64 id);

int LookupDNSThread(char *dns, unsigned short port, int type, NetProc *netProc, HWND hWnd, __int64 id, FailProc *failProc, InitProc *initProc, unsigned int message);
//int LookupDNSThread(char *dns, unsigned short port, int type, NetProc *netProc, HWND hWnd, __int64 id);
void CleanupDNS(DNSData *data);
void CleanupAllDNS();

struct NetworkName {
	char *ip;
	int family;
	char dns[4];
	inline void Cleanup() {
		free(this);
	}
};


struct NetworkNames {
	NetworkName **locals;
	NetworkName *remote;
	int numLocals;
	inline void Cleanup() {
		remote->Cleanup();
		for (int i=0; i<numLocals; i++) locals[i]->Cleanup();
		free(locals);
		free(this);
	}
};

void GetNamesAsync(int message, HWND hWnd, unsigned int remoteip, LPARAM lParam = 0);

#endif
