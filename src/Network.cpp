#include <stdio.h>              // sprintf()
#include <stdlib.h>             // calloc(), strtoul()
//#include <malloc.h>             // calloc()
//#include <string.h>             // strlen(), strcmp(), strstr()

#undef __GOT_SECURE_LIB__
#include "stringUtil.h"


#include "Network.h"
#include "malloc.h"
#include "Unicode.h"
//HANDLE hMutex = CreateMutex(0,0,0);

TGetAddrInfoW pGetAddrInfoW = 0;
TFreeAddrInfoW pFreeAddrInfoW = 0;
TGetNameInfoW pGetNameInfoW = 0;

int InitSockets() {
	WSAData junk;
	if (WSAStartup(0x202, &junk))
		return 0;
	HMODULE hMod = GetModuleHandleA("Ws2_32.dll");
	if (hMod) {
		pGetAddrInfoW  = (TGetAddrInfoW) GetProcAddress(hMod, "GetAddrInfoW");
		pFreeAddrInfoW = (TFreeAddrInfoW)GetProcAddress(hMod, "FreeAddrInfoW");
		pGetNameInfoW = (TGetNameInfoW)GetProcAddress(hMod, "GetNameInfoW");
		if (!pGetAddrInfoW || !pFreeAddrInfoW || !pGetNameInfoW) {
			pGetNameInfoW = 0;
			pGetAddrInfoW  = 0;
			// Simplifies things slightly to not have to check before freeing.
			pFreeAddrInfoW = (TFreeAddrInfoW)freeaddrinfo;
		}
	}
	return 1;
}
void CleanupSockets() {
	WSACleanup();
}


DNSData **dnsData = 0;
int numDNSData = 0;
ConnectCallback **connectCallbacks = 0;
int numConnectCallbacks = 0;
unsigned long WINAPI LookupDNSThreadProc(void *r) {
	DNSData *data = (DNSData *) r;
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	if (data->type == IPPROTO_TCP) {
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
	}
	else {
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
	}
	//hints.ai_flags = AI_CANONNAME;
	wchar_t *temp;
	int iport = (unsigned int) data->port;
	if (pGetAddrInfoW && (temp = (wchar_t*) alloca(sizeof(wchar_t)*(1+strlen(data->dns))))) {
		wchar_t port[10];
		_itow(iport, port, 10);
		UTF8toUTF16(temp, data->dns);
		if (pGetAddrInfoW(temp, port, (ADDRINFOW*)&hints, (ADDRINFOW**)&data->firstAddr)) {
			data->firstAddr = 0;
		}
	}
	else {
		char port[10];
		itoa(iport, port, 10);
		if (getaddrinfo(data->dns, port, &hints, &data->firstAddr)) {
			data->firstAddr = 0;
		}
	}
	//ReleaseMutex(hMutex);
	if (!quitting)
		PostMessage(data->hWnd, WMU_DNS_DONE, (WPARAM)data, 0);
	return 0;
}

void CleanupDNS(DNSData *data) {
	if (data->hThread) {
		WaitForSingleObject(data->hThread, INFINITE);
		CloseHandle(data->hThread);
	}
	if (data->dns) free(data->dns);
	if (data->firstAddr) freeaddrinfo(data->firstAddr);
	for (int i=data->index + 1; i<numDNSData; i++) {
		dnsData[i]->index = i-1;
		dnsData[i-1] = dnsData[i];
	}
	numDNSData--;
	srealloc(dnsData, sizeof(DNSData*) * numDNSData);
	free(data);
}

void CleanupCallback(ConnectCallback *callback) {
	if (callback->firstAddr) freeaddrinfo(callback->firstAddr);
	if (callback->sock != INVALID_SOCKET) closesocket(callback->sock);
	for (int i=callback->index + 1; i<numConnectCallbacks; i++) {
		connectCallbacks[i]->index = i-1;
		connectCallbacks[i-1] = connectCallbacks[i];
	}
	numConnectCallbacks--;
	srealloc(connectCallbacks, sizeof(ConnectCallback*) * numConnectCallbacks);
	free(callback);
}

void CleanupAllDNS() {
	while(numDNSData) {
		CleanupDNS(dnsData[numDNSData-1]);
	}
	//while(numConnectCallbacks) {
	//	CleanupCallback(connectCallbacks[numConnectCallbacks-1]);
	//}
}

int LookupDNSThread(char *dns, unsigned short port, int type, NetProc *netProc, HWND hWnd, __int64 id, FailProc *failProc, InitProc *initProc, unsigned int message) {
	char *dns2 = strdup(dns);
	DNSData *data = (DNSData*) calloc(sizeof(DNSData), 1);
	if (!data || !dns2 || !srealloc(dnsData, sizeof(DNSData*) * (numDNSData+1))) {
		free(dns2);
		free(data);
		return 0;
	}

	dnsData[numDNSData] = data;
	data->failProc = failProc;
	data->initProc = initProc;
	data->message = message;
	data->netProc = netProc;
	data->dns = dns2;
	data->hWnd = hWnd;
	data->index = numDNSData;
	data->id = id;
	data->port = port;
	data->type = type;
	numDNSData ++;
	int i = 0;
	// If I have a number IPv4 IP, no need to spawn a thread.
	//unsigned long ip = inet_addr((char*)dns);
	//if (ip == INADDR_NONE) {
		data->hThread = CreateThread(0, 0x1000, LookupDNSThreadProc, data, 0, 0);
		if (!data->hThread) {
			CleanupDNS(data);
			return 0;
		}
	//}
	//else {
	//	LookupDNSThreadProc(data);
	//}
	return 1;
}


void TrySocket(ConnectCallback *callback) {
	if (callback->sock != INVALID_SOCKET) {
		closesocket(callback->sock);
		callback->sock = INVALID_SOCKET;
	}
	while (callback->currentAddr) {
		callback->sock = socket(callback->currentAddr->ai_family, callback->currentAddr->ai_socktype, callback->currentAddr->ai_protocol);
		if (callback->sock != INVALID_SOCKET) {
			if (WSAAsyncSelect(callback->sock, callback->hWnd, callback->message, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE) == 0) {
				if (connect(callback->sock, callback->currentAddr->ai_addr, (int)callback->currentAddr->ai_addrlen) == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
					return;
				}
			}
			closesocket(callback->sock);
			callback->sock = INVALID_SOCKET;
		}
		callback->currentAddr = callback->currentAddr->ai_next;
	}
	// no addresses left to try.
	//status = STATUS_FAILED;
	//PostMessage(hWnd, completionMessage, id, 0);
	callback->failProc(callback->id);
	CleanupCallback(callback);
}

void TryNextSocket(ConnectCallback *callback) {
	callback->currentAddr = callback->currentAddr->ai_next;
	TrySocket(callback);
}

ConnectCallback *AllocCallback(UINT message, HWND hWnd, __int64 id, unsigned int protocol) {
	ConnectCallback *callback = (ConnectCallback *) calloc(sizeof(ConnectCallback),1);
	if (!callback || !srealloc(connectCallbacks, sizeof(ConnectCallback*)*(numConnectCallbacks+1))) {
		free(callback);
		return 0;
	}
	callback->proto = protocol;
	callback->sock = INVALID_SOCKET;
	callback->hWnd = hWnd;
	callback->message = message;
	callback->index = numConnectCallbacks;
	connectCallbacks[numConnectCallbacks++] = callback;
	return callback;
}

void CreateCallback (addrinfo *addr, __int64 id) {
	int i;
	for (i=0; i<numDNSData; i++) {
		if (dnsData[i]->firstAddr == addr) break;
	}
	// shouldn't happen.
	if (i >= numDNSData) {
		if (addr) freeaddrinfo(addr);
		return;
	}
	if (!addr) {
		dnsData[i]->failProc(id);
		return;
	}

	ConnectCallback *callback = AllocCallback(dnsData[i]->message, dnsData[i]->hWnd, id, IPPROTO_TCP);
	if (!callback) {
		freeaddrinfo(addr);
		dnsData[i]->failProc(id);
		return;
	}

	callback->currentAddr = callback->firstAddr = addr;
	callback->id = id;
	callback->failProc = dnsData[i]->failProc;
	callback->initProc = dnsData[i]->initProc;
	dnsData[i]->firstAddr = 0;

	if (callback->initProc(callback, id))
		TrySocket(callback);
	else {
		CleanupCallback(callback);
		callback->failProc(id);
	}
}

ConnectCallback * CreateUDPCallback(char *ip, unsigned short port, UINT message, HWND hWnd, __int64 id) {
	ConnectCallback *callback = AllocCallback(message, hWnd, id, IPPROTO_UDP);
	if (!callback) return 0;
	callback->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (callback->sock == INVALID_SOCKET) {
		CleanupCallback(callback);
		return 0;
	}

	if (ip) {
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(ip);
		addr.sin_port = htons(port);
		if (bind(callback->sock, (sockaddr*)&addr, sizeof(addr))) {
			CleanupCallback(callback);
			return 0;
		}
	}

	if (WSAAsyncSelect(callback->sock, callback->hWnd, callback->message, FD_CLOSE | FD_READ | FD_WRITE)) {
		CleanupCallback(callback);
		return 0;
	}

	return callback;
}

int LookupDNSThreadConnectCallback(char *dns, unsigned short port, int type, FailProc *failProc, InitProc *initProc, unsigned int message, HWND hWnd, __int64 id) {
	if (!LookupDNSThread(dns, port, type, CreateCallback, hWnd, id, failProc, initProc, message)) return 0;
	return 1;
}


struct NameRequest {
	int message;
	HWND hWnd;
	unsigned int remoteip;
	LPARAM lparam;
};

NetworkName *MakeNetworkName(addrinfo *info) {
	int len;
	if (info->ai_family == AF_INET) {
		len = 20;
	}
	else if (info->ai_family == AF_INET6) {
		len = 40;
	}
	else return 0;
	char temp[1000];
	int res;
	if (!(res = getnameinfo(info->ai_addr, (int)info->ai_addrlen, temp, 1000, 0, 0, 0))) {
		len += (int)strlen(temp);
	}
	NetworkName* out = (NetworkName*) malloc(sizeof(NetworkName) + sizeof(char)*(len));
	if (!out) return 0;
	out->ip = out->dns;
	if (!res) {
		strcpy(out->dns, temp);
		out->ip += 1+strlen(out->dns);
	}
	out->family = info->ai_family;
	if (info->ai_family == AF_INET) {
		unsigned char * ip = &((sockaddr_in*)info->ai_addr)->sin_addr.S_un.S_un_b.s_b1;
		sprintf(out->ip, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
	}
	else if (info->ai_family == AF_INET6) {
		unsigned char *c = ((sockaddr_in6*)info->ai_addr)->sin6_addr.u.Byte;
		for (int i=0; i<8; i++) {
			sprintf(out->ip+i*5, "%0X%0X", c[2*i], c[2*i+1]);
			if (i<7) out->ip[i*5] = ':';
		}
	}
	return out;
}

unsigned long WINAPI GetNames(void *req) {
	NameRequest *r = (NameRequest*) req;
	HWND hWnd = r->hWnd;
	int message = r->message;
	LPARAM lParam = r->lparam;
	unsigned int remoteIP = r->remoteip;
	free(r);
	char hostname[1000];
	if (SOCKET_ERROR == gethostname(hostname, 1000)) return 1;
	NetworkNames *names = (NetworkNames*) calloc(1, sizeof(NetworkNames));
	if (!names) return 1;

	//hostent *h = gethostbyname(hostname);
	addrinfo *info = 0;
	char *port = "80";
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_CANONNAME;
	if (remoteIP) {
		char temp[20];
		sprintf(temp, "%u.%u.%u.%u", remoteIP>>24, 0xFF&(remoteIP>>16), 0xFF&(remoteIP>>8), remoteIP&0xFF);
		if (!getaddrinfo(temp, port, &hints, &info)) {
			names->remote = MakeNetworkName(info);
			freeaddrinfo(info);
		}
	}
	if (!getaddrinfo(hostname, port, &hints, &info)) {
		addrinfo *info2 = info;
		while (info2) {
			if (!srealloc(names->locals, sizeof(NetworkName*) * (names->numLocals+1))) {
				continue;
			}
			int len = 0;
			names->locals[names->numLocals] = MakeNetworkName(info2);
			if (names->locals[names->numLocals]) names->numLocals++;
			if (!names->remote) names->remote = MakeNetworkName(info2);
			info2 = info2->ai_next;
		}
		freeaddrinfo(info);
	}
	// Simple way of handling shutdown issues.
	SendMessage(hWnd, message, (WPARAM)&names, lParam);
	if (names) names->Cleanup();
	return 0;
}

void GetNamesAsync(int message, HWND hWnd, unsigned int remoteip, LPARAM lParam) {
	NameRequest *r = (NameRequest*) malloc(sizeof(NameRequest));
	if (!r) return;
	HANDLE h;
	r->lparam = lParam;
	r->message = message;
	r->remoteip = remoteip;
	r->hWnd = hWnd;
	if ((h = CreateThread(0, 0x1000, GetNames, r, 0, 0)) == 0) {
		free(r);
		return;
	}
	CloseHandle(h);
}
