#include "../global.h"
#include <time.h>
#include "Socket.h"
#include "ScriptObjects.h"
#include "../Network.h"
#include "../Unicode.h"
#include "../util.h"
#define IPAddr SUPERGOAT
#define IcmpSendEcho2 SUPERGOAT2
#define IcmpCloseHandle SUPERGOAT3
#define IcmpParseReplies SUPERGOAT4
#include <Ipexport.h>
#include <IcmpAPI.h>
#include <winternl.h>
#undef IcmpParseReplies
#undef IcmpCloseHandle
#undef IPAddr
#undef IcmpSendEcho2
unsigned int AddrType;

#include <Ws2tcpip.h>
#include <Wspiapi.h>

#pragma pack(push, 1)
struct ProtocolVersion {
	unsigned char major, minor;
};
struct Random {
	unsigned int gmt_unix_time;
	unsigned char random_bytes[28];
};

struct CipherSuite {
	unsigned char byte[2];
};

struct ClientHello {
	char type;
	char size[3];
	ProtocolVersion client_version;
	Random random;
	unsigned short numSessionIds;
	unsigned char numCipherSuiteBytes;
	CipherSuite cipher_suites[16];
	unsigned char numCompressionMethodBytes;
	unsigned char CompressionMethod;
};
#pragma pack(pop)

void IPAddrNetProc (addrinfo *info, __int64 id) {
	Stack *stack = (Stack*) id;
	ScriptValue sv;
	CreateNullValue(sv);
	if (info) {
		addrinfo *info2 = info;
		int count = 0;
		while (info2) {
			count++;
			info2 = info2->ai_next;
		}
		if (CreateListValue(sv, count)) {
			ScriptValue addr;
			info2 = info;
			while (info2) {
				if ((info->ai_family == AF_INET || info->ai_family == AF_INET6) && CreateObjectValue(addr, AddrType)) {
					if (CreateStringValue(addr.objectVal->values[0], (unsigned char*)info->ai_addr, (int)info->ai_addrlen)) {
						CreateIntValue(addr.objectVal->values[1], info->ai_family);
						CreateIntValue(addr.objectVal->values[2], info->ai_socktype);
						CreateIntValue(addr.objectVal->values[3], info->ai_protocol);
						sv.listVal->PushBack(addr);
					}
					else addr.objectVal->Release();
				}
				info2 = info2->ai_next;
			}
		}
		if (pFreeAddrInfoW)
			pFreeAddrInfoW((ADDRINFOW*)info);
		else
			freeaddrinfo((ADDRINFOA*)info);
	}
	stack->Push(sv);
	RunStack(stack);
}

int IPAddrWait(ScriptValue *args, Stack *stack) {
	if (args[0].type != SCRIPT_LIST || 
		args[0].listVal->numVals < 1) return 0;
	int protocol = IPPROTO_TCP;
	if (args[0].listVal->numVals >= 2) {
		ScriptValue sv;
		CoerceIntNoRelease(sv, args[0].listVal->vals[1]);
		if (sv.intVal) {
			protocol = (int) sv.intVal;
		}
	}
	ScriptValue sv;
	args[0].listVal->vals[0].AddRef();
	CoerceString(args[0].listVal->vals[0], sv);
	int out = 0;
	if (LookupDNSThread((char*)sv.stringVal->value, 0, protocol, IPAddrNetProc, ghWnd, (__int64)stack,0,0,0)) {
		out = 2;
	}
	sv.Release();
	return out;
}

void IPAddr(ScriptValue &s, ScriptValue *args) {
	unsigned long ip = inet_addr((char*)args[0].stringVal->value);
	if (ip != INADDR_NONE && CreateObjectValue(s, AddrType)) {
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.S_un.S_addr = ip;
		addr.sin_family = AF_INET;
		if (CreateStringValue(s.objectVal->values[0], (unsigned char*)&addr, sizeof(addr))) {
			CreateIntValue(s.objectVal->values[1], AF_INET);
			if (!args[1].i32 || args[1].i32 == IPPROTO_TCP) {
				CreateIntValue(s.objectVal->values[2], SOCK_STREAM);
				CreateIntValue(s.objectVal->values[3], IPPROTO_TCP);
			}
			else {
				CreateIntValue(s.objectVal->values[2], SOCK_DGRAM);
				CreateIntValue(s.objectVal->values[3], IPPROTO_UDP);
			}
			return;
		}
		s.objectVal->Release();
		CreateNullValue(s);
	}
}

void IPAddrGetIP(ScriptValue &s, ScriptValue *args) {
	//if (s.type == SCRIPT_OBJECT && s.objectVal->type == AddrType) {
		int family = (int) s.objectVal->values[1].intVal;
		sockaddr *addr = (sockaddr*) s.objectVal->values[0].stringVal->value;
		if (family == AF_INET || family == AF_INET6) {
			char temp[50];
			if (family == AF_INET) {
				unsigned char * ip = &((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b1;
				sprintf(temp, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
			}
			else if (family == AF_INET6) {
				unsigned char *c = ((sockaddr_in6*)addr)->sin6_addr.u.Byte;
				for (int i=0; i<8; i++) {
					sprintf(temp+i*5, "%0X%0X:", c[2*i], c[2*i+1]);
				}
				temp[40] = 0;
			}
			if (CreateStringValue(s, (unsigned char*)temp))
				return;
		}
	//}
	CreateNullValue(s);
}

struct DNSThreadInfo {
	HANDLE hThread;
	Stack *stack;
	ObjectValue *obj;
	ScriptValue result;
};

void DNSResponse(DNSThreadInfo *info) {
	info->obj->Release();
	info->stack->Push(info->result);
	RunStack(info->stack);
	free(info);
};


DWORD WINAPI DNSThreadProc (__in  LPVOID lpParameter) {
	DNSThreadInfo *info = (DNSThreadInfo*) lpParameter;
	sockaddr *addr = (sockaddr*) info->obj->values[0].stringVal->value;
	int family = (int) info->obj->values[1].intVal;
	char host[4000] = "";
	CreateNullValue(info->result);

	int res = 1;
	int size = 0;
	if (family == AF_INET) {
		size = sizeof(sockaddr_in);
		//unsigned char * ip = &((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b1;
		//sprintf(temp, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
	}
	else if (family == AF_INET6) {
		size = sizeof(sockaddr_in6);
		/*unsigned char *c = ((sockaddr_in6*)addr)->sin6_addr.u.Byte;
		for (int i=0; i<8; i++) {
			sprintf(temp+i*5, "%0X%0X:", c[2*i], c[2*i+1]);
		}
		temp[40] = 0;
		//*/
	}
	if (size) {
		if (pGetNameInfoW) {
			wchar_t host2[1000];
			res = pGetNameInfoW(addr, size, host2, sizeof(host2)/sizeof(wchar_t), 0, 0, 0);
			if (!res) {
				UTF16toUTF8(host, host2);
			}
		}
		else {
			res = getnameinfo(addr, size, host, sizeof(host), 0, 0, 0);
		}
	}
	if (!res) {
		CreateStringValue(info->result, host);
	}
	PostMessage(ghWnd, WMU_CALL_PROC, (WPARAM)info, (LPARAM)DNSResponse);
	return 0;
}

int IPAddrGetDNSWait(ScriptValue *args, Stack *stack) {
	DNSThreadInfo *info = (DNSThreadInfo*)malloc(sizeof(DNSThreadInfo));
	if (!info) return 0;
	info->stack = stack;
	info->obj = stack->vals[1].objectVal;
	info->obj->AddRef();
	info->hThread = CreateThread(0, 0, DNSThreadProc, info, 0, 0);
	if (!info->hThread) {
		info->obj->Release();
		free(info);
		return 0;
	}
	CloseHandle(info->hThread);
	return 2;
}

#ifndef X64
	typedef DWORD WINAPI IcmpSendEcho2Type (
	  __in          HANDLE IcmpHandle,
	  __in          HANDLE Event,
	  __in          FARPROC ApcRoutine,
	  __in          PVOID ApcContext,
	  __in          in_addr DestinationAddress,
	  __in          LPVOID RequestData,
	  __in          WORD RequestSize,
	  __in          PIP_OPTION_INFORMATION RequestOptions,
	  __out         LPVOID ReplyBuffer,
	  __in          DWORD ReplySize,
	  __in          DWORD Timeout
	);
#else
	typedef DWORD WINAPI IcmpSendEcho2Type (
	  __in          HANDLE IcmpHandle,
	  __in          HANDLE Event,
	  __in          FARPROC ApcRoutine,
	  __in          PVOID ApcContext,
	  __in          in_addr DestinationAddress,
	  __in          LPVOID RequestData,
	  __in          WORD RequestSize,
	  __in          PIP_OPTION_INFORMATION32 RequestOptions,
	  __out         LPVOID ReplyBuffer,
	  __in          DWORD ReplySize,
	  __in          DWORD Timeout
	);
#endif

typedef DWORD WINAPI IcmpParseRepliesType (
  __in          LPVOID ReplyBuffer,
  __in          DWORD ReplySize
);

typedef HANDLE WINAPI IcmpCreateFileType();

typedef BOOL WINAPI IcmpCloseHandleType(__in HANDLE IcmpHandle);


struct Ping {
	Stack *stack;
	sockaddr *addr;
	ObjectValue *obj;
	char *error;
	int ping;
	DWORD timeout;
	unsigned short seq;
	HANDLE hThread;
	unsigned char ttl;
};

int numPings = 0;
Ping ** pings = 0;

static void RemovePing(int i) {
	if (i >= 0 && i<numPings) {
		if (pings[i]->hThread) {
			WaitForSingleObject(pings[i]->hThread, INFINITE);
			CloseHandle(pings[i]->hThread);
		}
		pings[i]->obj->Release();
		free(pings[i]);
		pings[i] = pings[--numPings];
	}
}

/*void CheckPingTimeouts() {
	if (numPings) {
		__int64 t = GetTimeAccurate();
		for (int i=0; i<numPings; i++) {
			if (t > pings[i].start + pings[i].timeout || t < pings[i].start) {
				ScriptValue s;
				CreateStringValue(s, "timeout");
				pings[i].stack->Push(s);
				RunStack(pings[i].stack);
				RemovePing(i--);
			}
		}
	}
}
//*/

#ifdef GOAT

void HandlePingSocketMessage(WPARAM wParam, LPARAM lParam) {
	for(int i=0; i<numPings; i++) {
		if (pings[i].sock == (SOCKET) wParam) {
			int happy = 0;
			char *error = 0;
			if (!HIWORD(lParam)) {
				char data[10000];
				sockaddr from;
				int fromlen = sizeof(sockaddr);
				int l = recvfrom(pings[i].sock, data, sizeof(data), 0, &from, &fromlen);
				if ((l != SOCKET_ERROR || WSAGetLastError() != WSAEMSGSIZE) &&
					l >= 20) {
						// Not from who we want
						IPHeader *h = (IPHeader*) data;
						if (h->proto != 1) return;
						h->length = (h->length>>8)+(h->length<<8);
						//if (fromlen != sizeof(from) || memcmp(&from, pings[i].addr, sizeof(from))) return;
						if (h->proto == 1) {
							// keep socket around.
							happy = 1;
							if (h->version == 4 && h->length == l && h->header_len >= 5 && h->length >= 4*(int)h->header_len) {
								l -= 4*(int)h->header_len;
								
								if (l < 8) continue;
								ICMPHeader *h2 = (ICMPHeader *) (data + 4*(int)h->header_len);
								if (h2->type == 3 || h2->type == 11) {
									IPHeader *h = (IPHeader*) data;
									if (l == 20 + 8 + 8) {
										if (!memcmp(&pings[i].header, ((char*)h2)+28, 8)) {
											// Timeout or something.
											if (h2->type == 3)
												error = "unreachable";
											else
												error = "ttl exceeded";
										}
									}
								}
								else if (h2->type == 0 && h2->code == 0 && l == 16) {
									if (h2->identifier == pings[i].header.identifier && h2->seq == pings[i].header.seq && h2->start == pings[i].header.start) {
										happy = 2;
									}
								}
								//if (h2->type == ICMP_TTL_EXPIRE || h2->type == ICMP_DEST_UNREACH)
								h2=h2;
							}
						}
				}
			}
			if (happy == 1 && !error) return;
			ScriptValue s;
			if (!happy) {
				CreateNullValue(s);
			}
			else if (error) {
				CreateStringValue(s, error);
			}
			else {
				CreateIntValue(s, GetTimeAccurate() - pings[i].header.start);
			}
			pings[i].stack->Push(s);
			RunStack(pings[i].stack);
			RemovePing(i--);
			return;
		}
	}
}

int IPAddrPingWait(ScriptValue *args, Stack *stack) {
	//errorPrintf("\r\nPing called\r\n");
	if (args[0].type != SCRIPT_LIST) return 0;
	if (numPings >= 1024) return 0;
	int family = (int) stack->vals[1].objectVal->values[1].intVal;
	if (family != AF_INET) return 0;
	sockaddr *addr = (sockaddr*) stack->vals[1].objectVal->values[0].stringVal->value;

	//errorPrintf("IPv4 - %i.%i.%i.%i\r\n", ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b1, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b2, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b3, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b4);

	int t = 564;
	int ttl = 0;
	if (args[0].listVal->numVals > 0) {
		ScriptValue sv;
		CoerceIntNoRelease(sv, args[0].listVal->vals[0]);
		if (sv.intVal > 60000) t = 60000;
		else if (sv.intVal > 0)  t = (int)sv.intVal;
		if (args[0].listVal->numVals > 1) {
			CoerceIntNoRelease(sv, args[0].listVal->vals[1]);
			if (sv.intVal > 0 && sv.intVal < 128) {
				ttl = (int)sv.intVal;
			}
		}
	}
	if (!ttl) ttl = 50;
	if (!srealloc(pings, (numPings+1) * sizeof(Ping))) return 0;
	Ping *ping = pings+numPings;
	memset(ping, 0, sizeof(Ping));
	ping->timeout = t;
	ping->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping->sock == INVALID_SOCKET) return 0;
	if (setsockopt(ping->sock, IPPROTO_IP, IP_TTL, (const char*)&ttl,  sizeof(ttl)) == SOCKET_ERROR) {
		closesocket(ping->sock);
		return 0;
	}
	int id = 0;
	ICMPHeader *header = &ping->header;
	header->type = 8;
	header->code = 0;
	header->checksum = 0;
	header->identifier = (short)GetCurrentProcessId();
	while(1) {
		int i;
		header->seq = (unsigned short) GetNumber();
		for (i=0; i<numPings; i++) {
			if (pings[i].seq == header->seq) break;
		}
		if (i == numPings) break;
	}
	ping->seq = header->seq;
	header->start = ping->start = GetTimeAccurate();
	unsigned int checksum = 0;
	for (int i = 0; i<sizeof(*header)/2; i++) {
		checksum += ((unsigned short*)header)[i];
	}
	checksum = (checksum>>16) + (unsigned short)checksum;
	checksum += (checksum>>16);
	header->checksum = ~(unsigned short) checksum;
	int res;
	res = WSAAsyncSelect(ping->sock, ghWnd, WMU_PING_SOCKET, FD_READ);
	if (res != 0) {
		closesocket(ping->sock);
		return 0;
	}//*/

	res = sendto(ping->sock, (char*) header, sizeof(*header), 0, addr, sizeof(*addr));
	if (res == SOCKET_ERROR || res != sizeof(*header)) {
		closesocket(ping->sock);
		return 0;
	}

	/*char buf[2000];
	int l = 1000;
	recvfrom(ping->sock, buf, 1000, 0, (sockaddr*)(buf+1000), &l);
	__int64 w = GetTimeAccurate();//*/

	ping->obj = stack->vals[1].objectVal;
	ping->obj->AddRef();
	ping->stack = stack;
	ping->addr = addr;

	numPings++;

	//closesocket(ping->sock);
	//int res = sendto(ping->sock, &header, sizeof(header), 0, 
	return 2;
#ifdef GOAT
	//errorPrintf("ttl: %i\r\n", ttl);
	//errorPrintf("timeout: %i\r\n", t);
	static IcmpSendEcho2Type * IcmpSendEcho2 = 0;
	static IcmpCreateFileType * IcmpCreateFile = 0;
	if ((unsigned __int64)IcmpSendEcho2<2) {
		if (!IcmpSendEcho2) {
			HMODULE h = GetModuleHandleA("Iphlpapi.dll");
			if (h) {
				IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
				IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
				IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
				if (IcmpSendEcho2 && IcmpCloseHandle && IcmpParseReplies) {
					IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					/*
					if (IcmpCreateFile) {
						errorPrintf("Using Iphlpapi.dll\r\n");
					}//*/
				}
			}
			if (!IcmpCreateFile) {
				h = LoadLibraryA("Icmp.dll");
				if (h) {
					IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
					IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
					IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
					if (IcmpSendEcho2 && IcmpCloseHandle && IcmpParseReplies) {
						IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					}
					if (!IcmpCreateFile) {
						FreeLibrary(h);
					}
					//else
					//	errorPrintf("Using Icmp.dll\r\n");
				}
			}
		}
		if (!IcmpCreateFile) {
			//errorPrintf("Icmp ping functions not found\r\n");
			IcmpSendEcho2 = (IcmpSendEcho2Type*) 1;
			return 0;
		}
	}

	PingData *data = (PingData*)calloc(1, sizeof(PingData));
	if (data) {
		//errorPrintf("Creating Handle\r\n");
		data->hicmp = IcmpCreateFile();
		if (data->hicmp != INVALID_HANDLE_VALUE) {
			data->stack = stack;
#ifndef X64
			IP_OPTION_INFORMATION info;
#else
			IP_OPTION_INFORMATION info;
#endif
			memset(&info, 0, sizeof(info));
			info.Ttl = ttl;
			//errorPrintf("Starting Asynchronous Ping\r\n");
			int res = IcmpSendEcho2(data->hicmp, 0, (FARPROC)PingCallback, data, *(in_addr*)(addr->sa_data+2), "", 0, &info, &data->reply, sizeof(data->reply)*8+sizeof(data->padding), t);
					SleepEx(564, TRUE);

			if (!res && GetLastError() == ERROR_IO_PENDING) {
				//errorPrintf("Success!  Handle: %08X, Memory: %08X\r\n", data->hicmp, data);
				return 2;
			}
			//errorPrintf("Failed, cleaning up\r\n");
			IcmpCloseHandle(data->hicmp);
		}
		//*
		else {
			//errorPrintf("Failed\r\n");
		}//*/
		free(data);
	}
	return 0;
	/*
	int protocol = IPPROTO_TCP;
	if (args[0].listVal->numVals >= 2) {
		ScriptValue sv;
		CoerceIntNoRelease(sv, args[0].listVal->vals[1]);
		if (sv.intVal) {
			protocol = (int) sv.intVal;
		}
	}
	ScriptValue sv;
	args[0].listVal->vals[0].AddRef();
	CoerceString(args[0].listVal->vals[0], sv);
	int out = 0;
	if (LookupDNSThread((char*)sv.stringVal->value, 0, protocol, IPAddrNetProc, ghWnd, (__int64)stack)) {
		out = 2;
	}
	sv.Release();
	return out;
	//*/
#endif
}
#endif


/*void HandlePingSocketMessage(WPARAM wParam, LPARAM lParam) {
	for(int i=0; i<numPings; i++) {
		if (pings[i].sock == (SOCKET) wParam) {
			int happy = 0;
			char *error = 0;
			if (!HIWORD(lParam)) {
				char data[10000];
				sockaddr from;
				int fromlen = sizeof(sockaddr);
				int l = recvfrom(pings[i].sock, data, sizeof(data), 0, &from, &fromlen);
				if ((l != SOCKET_ERROR || WSAGetLastError() != WSAEMSGSIZE) &&
					l >= 20) {
						// Not from who we want
						IPHeader *h = (IPHeader*) data;
						if (h->proto != 1) return;
						h->length = (h->length>>8)+(h->length<<8);
						//if (fromlen != sizeof(from) || memcmp(&from, pings[i].addr, sizeof(from))) return;
						if (h->proto == 1) {
							// keep socket around.
							happy = 1;
							if (h->version == 4 && h->length == l && h->header_len >= 5 && h->length >= 4*(int)h->header_len) {
								l -= 4*(int)h->header_len;
								
								if (l < 8) continue;
								ICMPHeader *h2 = (ICMPHeader *) (data + 4*(int)h->header_len);
								if (h2->type == 3 || h2->type == 11) {
									IPHeader *h = (IPHeader*) data;
									if (l == 20 + 8 + 8) {
										if (!memcmp(&pings[i].header, ((char*)h2)+28, 8)) {
											// Timeout or something.
											if (h2->type == 3)
												error = "unreachable";
											else
												error = "ttl exceeded";
										}
									}
								}
								else if (h2->type == 0 && h2->code == 0 && l == 16) {
									if (h2->identifier == pings[i].header.identifier && h2->seq == pings[i].header.seq && h2->start == pings[i].header.start) {
										happy = 2;
									}
								}
								//if (h2->type == ICMP_TTL_EXPIRE || h2->type == ICMP_DEST_UNREACH)
								h2=h2;
							}
						}
				}
			}
			if (happy == 1 && !error) return;
			ScriptValue s;
			if (!happy) {
				CreateNullValue(s);
			}
			else if (error) {
				CreateStringValue(s, error);
			}
			else {
				CreateIntValue(s, GetTimeAccurate() - pings[i].header.start);
			}
			pings[i].stack->Push(s);
			RunStack(pings[i].stack);
			RemovePing(i--);
			return;
		}
	}
}
//*/
void PingResponse(Ping *ping) {
	if (!ping->error) {
		ping->stack->PushInt(ping->ping);
	}
	else {
		ScriptValue sv;
		CreateStringValue(sv, (unsigned char*)ping->error);
		ping->stack->Push(sv);
	}
	RunStack(ping->stack);
	for (int i=0; i<numPings; i++) {
		if (ping == pings[i]) {
			RemovePing(i);
			break;
		}
	}
};

DWORD WINAPI PingProc (__in  LPVOID lpParameter) {
	static IcmpSendEcho2Type * IcmpSendEcho2 = 0;
	static IcmpCreateFileType * IcmpCreateFile = 0;
	static IcmpCloseHandleType * IcmpCloseHandle;
	//static IcmpParseRepliesType * IcmpParseReplies;
	if ((unsigned __int64)IcmpSendEcho2<2) {
		if (!IcmpSendEcho2) {
			HMODULE h = GetModuleHandleA("Iphlpapi.dll");
			if (h) {
				IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
				IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
				//IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
				if (IcmpSendEcho2 && IcmpCloseHandle /*&& IcmpParseReplies//*/) {
					IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					/*
					if (IcmpCreateFile) {
						errorPrintf("Using Iphlpapi.dll\r\n");
					}//*/
				}
				if (!IcmpCreateFile) {
					FreeLibrary(h);
				}
			}
			if (!IcmpCreateFile) {
				h = LoadLibraryA("Icmp.dll");
				if (h) {
					IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
					IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
					//IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
					if (IcmpSendEcho2 && IcmpCloseHandle /*&& IcmpParseReplies//*/) {
						IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					}
					if (!IcmpCreateFile) {
						FreeLibrary(h);
					}
					//else
					//	errorPrintf("Using Icmp.dll\r\n");
				}
			}
		}
	}
	Ping *ping = (Ping*) lpParameter;
	ping = ping;
	HANDLE hIcmp = IcmpCreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		// TODO:  Stuff.
	}
	else {
#ifndef X64
		IP_OPTION_INFORMATION info;
		ICMP_ECHO_REPLY replies[10];
#else
		IP_OPTION_INFORMATION32 info;
		ICMP_ECHO_REPLY32 replies[10];
#endif
		memset(&info, 0, sizeof(info));
		info.Ttl = ping->ttl;
		int res = IcmpSendEcho2(hIcmp, 0, 0, 0, *(in_addr*)(ping->addr->sa_data+2), "", 0, &info, replies, sizeof(replies), ping->timeout);
		if (res) {
			ping->ping = replies[0].RoundTripTime;
		}
		else {
			int e = GetLastError();
			char *error;
			switch (e) {
				case IP_BUF_TOO_SMALL:
					error = "buffer too small";
					break;
				case IP_DEST_NET_UNREACHABLE:
					error = "No route to network";
					break;
				case IP_DEST_HOST_UNREACHABLE:
					error = "No route to host";
					break;
				case IP_DEST_PROT_UNREACHABLE:
					error = "Protocol missing";
					break;
				case IP_DEST_PORT_UNREACHABLE:
					error = "Port unreachable";
					break;
				case IP_NO_RESOURCES:
					error = "No IP resourves available";
					break;
				case IP_BAD_OPTION:
					error = "Bad IP option";
					break;
				case IP_HW_ERROR:
					error = "Hardware error";
					break;
				case IP_PACKET_TOO_BIG:
					error = "Packet too big";
					break;
				case IP_REQ_TIMED_OUT:
					error = "Timeout";
					break;
				case IP_BAD_REQ:
					error = "Bad request";
					break;
				case IP_BAD_ROUTE:
					error = "Bad route";
					break;
				case IP_TTL_EXPIRED_TRANSIT:
					error = "TTL expired";
					break;
				case IP_TTL_EXPIRED_REASSEM:
					error = "TTL expired during reassembly";
					break;
				case IP_SOURCE_QUENCH:
					error = "Datagrams discarder";
					break;
				case IP_OPTION_TOO_BIG:
					error = "Options too big";
					break;
				case IP_BAD_DESTINATION:
					error = "Bad destination";
					break;
				case IP_GENERAL_FAILURE:
					error = "General failure";
					break;
				default:
					error = "Unknown error";
					break;
			};
			ping->error = error;
		}
		IcmpCloseHandle(hIcmp);
	}
	PostMessage(ghWnd, WMU_CALL_PROC, (WPARAM)ping, (LPARAM)PingResponse);
	return 0;
}

int IPAddrPingWait(ScriptValue *args, Stack *stack) {
	//errorPrintf("\r\nPing called\r\n");
	if (args[0].type != SCRIPT_LIST) return 0;
	if (numPings >= 1024) return 0;
	int family = (int) stack->vals[1].objectVal->values[1].intVal;
	if (family != AF_INET) return 0;
	sockaddr *addr = (sockaddr*) stack->vals[1].objectVal->values[0].stringVal->value;

	//errorPrintf("IPv4 - %i.%i.%i.%i\r\n", ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b1, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b2, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b3, ((sockaddr_in*)addr)->sin_addr.S_un.S_un_b.s_b4);

	int t = 564;
	int ttl = 0;
	if (args[0].listVal->numVals > 0) {
		ScriptValue sv;
		CoerceIntNoRelease(sv, args[0].listVal->vals[0]);
		if (sv.intVal > 60000) t = 60000;
		else if (sv.intVal > 0)  t = (int)sv.intVal;
		if (args[0].listVal->numVals > 1) {
			CoerceIntNoRelease(sv, args[0].listVal->vals[1]);
			if (sv.intVal > 0 && sv.intVal < 128) {
				ttl = (int)sv.intVal;
			}
		}
	}
	if (ttl <= 0) ttl = 50;
	else if (ttl > 255) ttl = 255;
	if (!srealloc(pings, (numPings+1) * sizeof(Ping *))) return 0;
	Ping *ping = (Ping*) calloc(1, sizeof(Ping));
	if (!ping) return 0;
	pings[numPings] = ping;
	numPings++;

	ping->timeout = t;
	ping->ttl = ttl;
	ping->obj = stack->vals[1].objectVal;
	ping->obj->AddRef();
	ping->stack = stack;
	ping->addr = addr;
	ping->hThread = CreateThread(0, 0, PingProc, ping, 0, 0);
	if (!ping->hThread) {
		RemovePing(numPings);
		return 0;
	}
	return 2;
/*
	ping->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping->sock == INVALID_SOCKET) return 0;
	if (setsockopt(ping->sock, IPPROTO_IP, IP_TTL, (const char*)&ttl,  sizeof(ttl)) == SOCKET_ERROR) {
		closesocket(ping->sock);
		return 0;
	}
	int id = 0;
	ICMPHeader *header = &ping->header;
	header->type = 8;
	header->code = 0;
	header->checksum = 0;
	header->identifier = (short)GetCurrentProcessId();
	while(1) {
		int i;
		header->seq = (unsigned short) GetNumber();
		for (i=0; i<numPings; i++) {
			if (pings[i].seq == header->seq) break;
		}
		if (i == numPings) break;
	}
	ping->seq = header->seq;
	header->start = ping->start = GetTimeAccurate();
	unsigned int checksum = 0;
	for (int i = 0; i<sizeof(*header)/2; i++) {
		checksum += ((unsigned short*)header)[i];
	}
	checksum = (checksum>>16) + (unsigned short)checksum;
	checksum += (checksum>>16);
	header->checksum = ~(unsigned short) checksum;
	int res;
	res = WSAAsyncSelect(ping->sock, ghWnd, WMU_PING_SOCKET, FD_READ);
	if (res != 0) {
		closesocket(ping->sock);
		return 0;
	}//*/
/*
	res = sendto(ping->sock, (char*) header, sizeof(*header), 0, addr, sizeof(*addr));
	if (res == SOCKET_ERROR || res != sizeof(*header)) {
		closesocket(ping->sock);
		return 0;
	}

	/*char buf[2000];
	int l = 1000;
	recvfrom(ping->sock, buf, 1000, 0, (sockaddr*)(buf+1000), &l);
	__int64 w = GetTimeAccurate();//*/
/*
	ping->obj = stack->vals[1].objectVal;
	ping->obj->AddRef();
	ping->stack = stack;
	ping->addr = addr;

	numPings++;

	//closesocket(ping->sock);
	//int res = sendto(ping->sock, &header, sizeof(header), 0, 
	return 2;
	//*/
#ifdef GOAT
	//errorPrintf("ttl: %i\r\n", ttl);
	//errorPrintf("timeout: %i\r\n", t);
	static IcmpSendEcho2Type * IcmpSendEcho2 = 0;
	static IcmpCreateFileType * IcmpCreateFile = 0;
	if ((unsigned __int64)IcmpSendEcho2<2) {
		if (!IcmpSendEcho2) {
			HMODULE h = GetModuleHandleA("Iphlpapi.dll");
			if (h) {
				IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
				IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
				IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
				if (IcmpSendEcho2 && IcmpCloseHandle && IcmpParseReplies) {
					IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					/*
					if (IcmpCreateFile) {
						errorPrintf("Using Iphlpapi.dll\r\n");
					}//*/
				}
			}
			if (!IcmpCreateFile) {
				h = LoadLibraryA("Icmp.dll");
				if (h) {
					IcmpSendEcho2 = (IcmpSendEcho2Type*) GetProcAddress(h, "IcmpSendEcho2");
					IcmpCloseHandle = (IcmpCloseHandleType*) GetProcAddress(h, "IcmpCloseHandle");
					IcmpParseReplies = (IcmpParseRepliesType*) GetProcAddress(h, "IcmpParseReplies");
					if (IcmpSendEcho2 && IcmpCloseHandle && IcmpParseReplies) {
						IcmpCreateFile = (IcmpCreateFileType*) GetProcAddress(h, "IcmpCreateFile");
					}
					if (!IcmpCreateFile) {
						FreeLibrary(h);
					}
					//else
					//	errorPrintf("Using Icmp.dll\r\n");
				}
			}
		}
		if (!IcmpCreateFile) {
			//errorPrintf("Icmp ping functions not found\r\n");
			IcmpSendEcho2 = (IcmpSendEcho2Type*) 1;
			return 0;
		}
	}

	PingData *data = (PingData*)calloc(1, sizeof(PingData));
	if (data) {
		//errorPrintf("Creating Handle\r\n");
		data->hicmp = IcmpCreateFile();
		if (data->hicmp != INVALID_HANDLE_VALUE) {
			data->stack = stack;
#ifndef X64
			IP_OPTION_INFORMATION info;
#else
			IP_OPTION_INFORMATION info;
#endif
			memset(&info, 0, sizeof(info));
			info.Ttl = ttl;
			//errorPrintf("Starting Asynchronous Ping\r\n");
			int res = IcmpSendEcho2(data->hicmp, 0, (FARPROC)PingCallback, data, *(in_addr*)(addr->sa_data+2), "", 0, &info, &data->reply, sizeof(data->reply)*8+sizeof(data->padding), t);
					SleepEx(564, TRUE);

			if (!res && GetLastError() == ERROR_IO_PENDING) {
				//errorPrintf("Success!  Handle: %08X, Memory: %08X\r\n", data->hicmp, data);
				return 2;
			}
			//errorPrintf("Failed, cleaning up\r\n");
			IcmpCloseHandle(data->hicmp);
		}
		//*
		else {
			//errorPrintf("Failed\r\n");
		}//*/
		free(data);
	}
	return 0;
	/*
	int protocol = IPPROTO_TCP;
	if (args[0].listVal->numVals >= 2) {
		ScriptValue sv;
		CoerceIntNoRelease(sv, args[0].listVal->vals[1]);
		if (sv.intVal) {
			protocol = (int) sv.intVal;
		}
	}
	ScriptValue sv;
	args[0].listVal->vals[0].AddRef();
	CoerceString(args[0].listVal->vals[0], sv);
	int out = 0;
	if (LookupDNSThread((char*)sv.stringVal->value, 0, protocol, IPAddrNetProc, ghWnd, (__int64)stack)) {
		out = 2;
	}
	sv.Release();
	return out;
	//*/
#endif
}







struct Buffer {
	char *data;
	int used;
	int size;
};

struct ScriptedSocket {
	Buffer outBuffer;
	Buffer inBuffer;
	ConnectCallback *callback;
	//StringValue *ip;
	Stack *stack;
	Stack *recStack;
	ObjectValue *replyTo;

	int recTimeout;
	int recWant;

	int functId;
	int connected;
	int tls;
	unsigned char tlsText;
	unsigned char tlsEnc1;
	unsigned char tlsEnc2;
};


unsigned int SocketType;

ObjectValue **sockets = 0;
int numSockets;

void ConnectSockFail (__int64 id) {
	ObjectValue *obj = (ObjectValue *)id;
	ScriptedSocket *socket = (ScriptedSocket*) obj->values[0].stringVal;
	//Stack *stack = socket->stack;
	obj->Release();
	//stack->PushNull();
	//RunStack(stack);
}

void HandleTLS(ScriptedSocket *socket) {
}

int SocketReadWait(ScriptValue *args, Stack *stack) {
	if (args[0].type != SCRIPT_LIST) return 0;
	ScriptedSocket *socket = (ScriptedSocket *)stack->vals[1].objectVal->values[0].stringVal;
	if (socket->recStack || socket->functId || (socket->connected && socket->connected != 1)) return 0;
	int want = 0;
	int timeout = 240;
	if (args[0].listVal->numVals > 0) {
		ScriptValue v;
		CoerceIntNoRelease(args[0].listVal->vals[0], v);
		want = v.i32;
		if (args[0].listVal->numVals > 1) {
			CoerceIntNoRelease(args[0].listVal->vals[1], v);
			timeout = v.i32;
		}
	}
	if (want >= 0 && socket->inBuffer.used >= want) {
		stack->PushInt(1);
		return -1;
	}
	socket->recStack = stack;
	socket->recTimeout = timeout;
	socket->recWant = want;
	return 2;
}

void RunSocketFunc(ScriptedSocket *socket, ObjectValue *obj, int msg) {
	ScriptValue sv, sv2;
	CreateListValue(sv, 4);
	CreateNullValue(sv2);
	sv.listVal->PushBack(sv2);
	sv.listVal->PushBack(sv2);
	sv.listVal->PushBack(msg);
	obj->AddRef();
	CreateObjectValue(sv2, obj);
	sv.listVal->PushBack(sv2);
	ObjectValue *obj2 = socket->replyTo;
	if (obj2) {
		obj2->AddRef();
	}
	RunFunction(socket->functId, sv, CAN_WAIT, obj2);
}

void HandleScriptedSocketMessage(WPARAM wParam, LPARAM lParam) {
	SOCKET sock = (SOCKET) wParam;
	ObjectValue *obj = 0;
	ScriptedSocket *socket;
	for (int i=0; i<numSockets; i++) {
		socket = (ScriptedSocket*)sockets[i]->values[0].stringVal->value;
		if (socket->callback && socket->callback->sock == sock) {
			obj = sockets[i];
			break;
		}
	}
	if (!obj) {
		closesocket(sock);
		return;
	}
	int error =  WSAGETSELECTERROR(lParam);
	int msg =  WSAGETSELECTEVENT(lParam);
	/*if (socket->tls && msg != FD_WRITE) {
		socket = socket;
	}//*/
	if (error) {
		if (!socket->connected) {
			// Still have stack.  This will call it.
			obj->Release();
			return;
		}
		msg = FD_CLOSE;
	}
	if (msg == FD_READ) {
		if (socket->callback->proto == IPPROTO_UDP) {
			if (socket->functId > 0) {
				RunSocketFunc(socket, obj, FD_READ);
			}
		}
		else {
			unsigned long len;
			int res = ioctlsocket(sock, FIONREAD, &len);
			if (res == SOCKET_ERROR) {
				msg = FD_CLOSE;
			}
			else if (len == 0) return;
			else {
				int newLen = len + socket->inBuffer.used;
				if (newLen > socket->inBuffer.size) {
					if (!srealloc(socket->inBuffer.data, (newLen)*sizeof(char))) {
						msg = FD_CLOSE;
					}
					else
						socket->inBuffer.size = newLen;
				}
				if (msg != FD_CLOSE) {
					int w = recv(sock, socket->inBuffer.data + socket->inBuffer.used, len, 0);
					if (w != SOCKET_ERROR && w)  {
						socket->inBuffer.used += w;
						if (socket->tls) {
							HandleTLS(socket);
						}
						else if (socket->functId > 0) {
							RunSocketFunc(socket, obj, FD_READ);
						}
						else if (socket->recStack && socket->recWant <= socket->inBuffer.used) {
							Stack *stack = socket->recStack;
							socket->recStack = 0;
							ScriptValue s;
							CreateNullValue(s);
							int len = socket->recWant;
							if (AllocateStringValue(s, len)) {
								memcpy(s.stringVal->value, socket->inBuffer.data, len);
								memmove(socket->inBuffer.data, socket->inBuffer.data + len, socket->inBuffer.used -= len);
								if (socket->inBuffer.size - socket->inBuffer.used >= 1000) {
									socket->inBuffer.size = socket->inBuffer.used;
									srealloc(socket->inBuffer.data, socket->inBuffer.size);
								}
								if (socket->inBuffer.used) {
									PostMessage(ghWnd, WMU_SCRIPTED_SOCKET, socket->callback->sock, FD_READ);
								}
							}
							stack->Push(s);
							RunStack(stack);
						}
						return;
					}
					else {
						int e = WSAGetLastError();
						if (e != WSAEWOULDBLOCK) {
							msg = FD_CLOSE;
						}
					}
				}
			}
		}
	}
	else if (msg == FD_WRITE) {
		if (socket->outBuffer.used) {
			int pos = 0;
			while (socket->outBuffer.used != pos) {
				int res = send(sock, socket->outBuffer.data+pos, socket->outBuffer.used-pos, 0);
				if (res <= 0) {
					int e = WSAGetLastError();
					if (e == WSAEWOULDBLOCK) {
						break;
					}
					lParam = FD_CLOSE;
					break;
				}
				pos += res;
			}
			memmove(socket->outBuffer.data, socket->outBuffer.data + pos, socket->outBuffer.used-=pos);
			if (socket->outBuffer.size > 1000 && pos > 1000) {
				srealloc(socket->outBuffer.data, socket->outBuffer.used);
				socket->outBuffer.size = socket->outBuffer.used;
			}
		}
		else {
			if (socket->functId > 0) {
				RunSocketFunc(socket, obj, FD_WRITE);
			}
		}
	}
	else if (msg == FD_CONNECT) {
		Stack *stack = socket->stack;
		socket->stack = 0;
		ScriptValue sv;
		socket->connected = 1;
		CreateObjectValue(sv, obj);
		stack->Push(sv);
		RunStack(stack);
		return;
	}
	if (msg == FD_CLOSE) {
		CleanupCallback(socket->callback);
		socket->callback = 0;
		socket->connected = -1;
		if (socket->functId > 0) {
			ScriptValue sv, sv2;
			CreateListValue(sv, 4);
			CreateNullValue(sv2);
			sv.listVal->PushBack(sv2);
			if (!error) error = WSAGetLastError();
			if (error) {
				wchar_t *msg = 0;
				FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, 10060, 0, (wchar_t*)&msg, 1, 0);
				if (msg) {
					CreateStringValue(sv2, msg);
					LocalFree(msg);
				}
				//CreateStringValue(
			}
			sv.listVal->PushBack(sv2);

			sv.listVal->PushBack(FD_CLOSE);
			obj->AddRef();
			CreateObjectValue(sv2, obj);
			sv.listVal->PushBack(sv2);
			ObjectValue *obj = socket->replyTo;
			if (obj) {
				obj->AddRef(); 
			}
			RunFunction(socket->functId, sv, CAN_WAIT, obj);
		}
		else if (socket->recStack) {
			Stack *stack = socket->recStack;
			socket->recStack = 0;
			stack->PushInt(0);
			RunStack(stack);
		}
	}
}

void SocketSend(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*)s.objectVal->values[0].stringVal->value;
	CreateNullValue(s);
	if (socket->callback->proto != IPPROTO_UDP) return;
	if (socket->connected <= 0) return;
	if (socket->tls) {
		return;
	}
	int len = args[0].stringVal[0].len;
	unsigned char *data = args[0].stringVal[0].value;
	if (len + socket->outBuffer.used > socket->outBuffer.size) {
		if (!srealloc(socket->outBuffer.data, socket->outBuffer.used + len)) return;
	}
	memcpy(socket->outBuffer.data + socket->outBuffer.used, data, len*sizeof(char));
	socket->outBuffer.used += len;

	if (socket->connected)
		PostMessage(ghWnd, WMU_SCRIPTED_SOCKET, socket->callback->sock, FD_WRITE);
	CreateIntValue(s, 1);
}

int ConnectSockInit (ConnectCallback* callback, __int64 id) {
	ObjectValue *obj = (ObjectValue *)id;
	if (!srealloc(sockets, sizeof(ObjectValue*) * (numSockets+1))) {
		return 0;
	}
	ScriptedSocket *socket = (ScriptedSocket*)obj->values[0].stringVal->value;
	sockets[numSockets++] = obj;
	socket->callback = callback;
	/*
	Stack *stack = (Stack*)obj->values[2].intVal;
	ScriptValue sv;
	CreateObjectValue (sv, obj);
	stack->Push(sv);
	obj->values[2].intVal = callback->sock;
	RunStack(stack);
	id = id;
	//*/
	return 1;
}

int FindSocketFunction(ScriptedSocket *socket, ScriptValue *args) {
	if (args[0].listVal->numVals >= 3 && args[0].listVal->vals[3].type == SCRIPT_STRING) {
		if (args[0].listVal->numVals >= 4 && args[0].listVal->vals[4].type == SCRIPT_OBJECT) {
			ObjectValue *obj = args[0].listVal->vals[4].objectVal;
			int v = VM.StringTable.Find(args[0].listVal->vals[3].stringVal);
			if (v >= 0) {
				int f = types[obj->type].FindFunction(v);
				if (f >= 0) {
					socket->functId = types[obj->type].functs[f].functionID;
					socket->replyTo = obj;
					obj->AddRef();
					return 1;
				}
			}
		}
		else {
			TableEntry<Function> *func = VM.Functions.Find(args[3].stringVal);
			if (func) {
				socket->functId = func->index;
				return 1;
			}
		}
	}
	if (args[0].listVal->vals[3].type == SCRIPT_NULL) return 1;
	return 0;
}

void SocketSocket(ScriptValue &s, ScriptValue *args) {
	if (args[0].type != SCRIPT_LIST || !srealloc(sockets, sizeof(ObjectValue*) * (numSockets+1))) return;

	char *ip = 0;
	unsigned short port = 0;
	if (args[0].listVal->numVals > 0 && args[0].listVal->vals[0].type == SCRIPT_STRING) {
		if (args[0].listVal->numVals > 1) {
			__int64 port64 = CoerceIntNoRelease(args[0].listVal->vals[1]);
			if (port64 < 0 || port64 > 0xFFFF) return;
			port = (unsigned short) port64;
		}
		ip = (char*)args[0].listVal->vals[0].stringVal->value;
	}

	ScriptValue sock;
	if (!CreateObjectValue(sock, SocketType)) return;

	if (!AllocateStringValue(sock.objectVal->values[0], sizeof(ScriptedSocket))) {
		sock.objectVal->Release();
		return;
	}

	ScriptedSocket *socket = (ScriptedSocket*)sock.objectVal->values[0].stringVal->value;
	memset(socket, 0, sizeof(ScriptedSocket));
	FindSocketFunction(socket, args);
	socket->callback = CreateUDPCallback(ip, port, WMU_SCRIPTED_SOCKET, ghWnd, (__int64)sock.objectVal);
	if (!socket->callback) {
		sock.objectVal->Release();
		return;
	}
	sockets[numSockets++] = sock.objectVal;
	s = sock;
	socket->connected = 1;
	//socket->callback = callback;
}


int ConnectWait(ScriptValue *args, Stack *stack) {
	if (args[0].type != SCRIPT_LIST || 
		args[0].listVal->numVals < 2) return 0;

	ScriptValue sv2;
	int protocol;
	unsigned char *dns;
	if (args[0].listVal->vals[0].type == SCRIPT_STRING) {
		protocol = IPPROTO_TCP;
		if (args[0].listVal->numVals > 2) {
			ScriptValue s;
			CoerceIntNoRelease(args[0].listVal->vals[2], s);
			if (s.intVal != 0)
				protocol = s.i32;
		}
		dns = args[0].listVal->vals[0].stringVal->value;
		CreateNullValue(sv2);
	}
	else if (args[0].listVal->vals[0].type == SCRIPT_OBJECT &&
		args[0].listVal->vals[0].objectVal->type == AddrType) {
			sv2 = args[0].listVal->vals[0];
			ObjectValue *obj = sv2.objectVal;
			//family = obj->values[1].i32;
			protocol = obj->values[3].i32;
			IPAddrGetIP(sv2, 0);
			if (sv2.type != SCRIPT_STRING) {
				sv2.objectVal->Release();
				return 0;
			}
			dns = sv2.stringVal->value;
	}
	else return 0;
	if (protocol != IPPROTO_TCP) return 0;

	__int64 port64 = CoerceIntNoRelease(args[0].listVal->vals[1]);
	if (port64 <= 0 || port64 > 0xFFFF) return 0;
	unsigned short port = (unsigned short) port64;

	ScriptValue sv;
	if (!CreateObjectValue(sv, SocketType)) return 0;
	if (!AllocateStringValue(sv.objectVal->values[0], sizeof(ScriptedSocket))) {
		sv.objectVal->Release();
		sv2.Release();
		return 0;
	}
	ScriptedSocket *socket = (ScriptedSocket*) sv.objectVal->values[0].stringVal->value;
	memset(socket, 0, sizeof(ScriptedSocket));
	
	//socket->ip = sv2.stringVal;

	socket->stack = stack;
	FindSocketFunction(socket, args);

	if (!LookupDNSThreadConnectCallback((char*)dns, port, protocol, ConnectSockFail, ConnectSockInit, WMU_SCRIPTED_SOCKET, ghWnd, (__int64)sv.objectVal)) {
		sv.objectVal->Release();
		sv2.Release();
		return 0;
	}
	sv2.Release();
	return 2;

	/*
	if (!CreateStringValue(sv.objectVal->values[0]

	int LookupDNSThreadConnectCallback(char *dns, unsigned short port, int type, FailProc *failProc, InitProc *initProc, unsigned int message, HWND hWnd, __int64 id)

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

	ConnectCallback *callback = (ConnectCallback *) calloc(sizeof(ConnectCallback),1);
	if (!callback || !srealloc(connectCallbacks, sizeof(ConnectCallback*)*(numConnectCallbacks+1))) {
		freeaddrinfo(addr);
		free(callback);
		dnsData[i]->failProc(id);
		return;
	}
	callback->id = id;
	callback->currentAddr = callback->firstAddr = addr;
	callback->failProc = dnsData[i]->failProc;
	callback->initProc = dnsData[i]->initProc;
	callback->sock = INVALID_SOCKET;
	callback->hWnd = dnsData[i]->hWnd;
	callback->message = dnsData[i]->message;
	callback->index = numConnectCallbacks;
	connectCallbacks[numConnectCallbacks++] = callback;
	dnsData[i]->firstAddr = 0;
	if (callback->initProc(callback, id))
		TrySocket(callback);
	else
		CleanupCallback(callback);
//*/
	return 0;
}

#define SOCKET_CONNECTED	2
#define SOCKET_CONNECTING	1
#define SOCKET_DISCONNECTED	0

void SocketStatus(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*) s.objectVal->values[0].stringVal->value;
	CreateNullValue(s);
	if (CreateListValue(s, 2)) {
		int status = SOCKET_DISCONNECTED;
		if (socket->connected == 1) {
			status = SOCKET_CONNECTED;
		}
		else if (socket->connected == 0) {
			status = SOCKET_CONNECTING;
		}
		s.listVal->PushBack(status);
		s.listVal->PushBack(socket->outBuffer.used);
	}
}

void SocketReadFrom(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*) s.objectVal->values[0].stringVal->value;
	unsigned long len;
	int res = ioctlsocket(socket->callback->sock, FIONREAD, &len);
	// shouldn't be needed, but just in case...
	CreateNullValue(s);
	if (socket->connected <= 0 || socket->callback->proto != IPPROTO_UDP) return;
	if (res == SOCKET_ERROR) {
		PostMessage(ghWnd, WMU_SCRIPTED_SOCKET, socket->callback->sock, FD_CLOSE);
	}
	ScriptValue sv;
	sockaddr_in addr;
	if (len && AllocateStringValue(sv, len)) {
		int fromLen = sizeof(addr);
		int len2 = recvfrom(socket->callback->sock, (char*)sv.stringVal->value, len, 0, (SOCKADDR*)&addr, &fromLen);
		if (len2 > 0) {
			if (len2 != len) {
				ResizeStringValue(sv, len2);
			}
			if (CreateListValue(s, 2)) {
				s.listVal->PushBack(sv);
				if (fromLen == sizeof(addr) && addr.sin_family == AF_INET && CreateObjectValue(sv, AddrType)) {
					if (CreateStringValue(sv.objectVal->values[0], (unsigned char*)&addr, sizeof(addr))) {
						CreateIntValue(sv.objectVal->values[1], AF_INET);
						CreateIntValue(sv.objectVal->values[2], SOCK_DGRAM);
						CreateIntValue(sv.objectVal->values[3], IPPROTO_UDP);
					}
					s.listVal->PushBack(sv);
				}
				else {
					s.listVal->PushBack();
				}
			}
		}
		else {
			sv.stringVal->Release();
		}
	}
}

void SocketRead(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*) s.objectVal->values[0].stringVal->value;
	int len = args[0].i32;
	if (len <= 0 || len > socket->inBuffer.used) len = socket->inBuffer.used;
	CreateNullValue(s);
	if (socket->callback->proto == IPPROTO_UDP) {
		return;
	}
	else if ((len || socket->connected > -1) && AllocateStringValue(s, len)) {
		memcpy(s.stringVal->value, socket->inBuffer.data, len);
		memmove(socket->inBuffer.data, socket->inBuffer.data + len, socket->inBuffer.used -= len);
		if (socket->inBuffer.size - socket->inBuffer.used >= 1000) {
			socket->inBuffer.size = socket->inBuffer.used;
			srealloc(socket->inBuffer.data, socket->inBuffer.size);
		}
		if (socket->inBuffer.used) {
			PostMessage(ghWnd, WMU_SCRIPTED_SOCKET, socket->callback->sock, FD_READ);
		}
	}
}

int TlsSend(ScriptedSocket *socket, unsigned char *data, int len, unsigned char type) {
	if (socket->connected <= 0) return 0;
	if (len + socket->outBuffer.used + 5 > socket->outBuffer.size) {
		if (!srealloc(socket->outBuffer.data, socket->outBuffer.used + len+5)) return 0;
	}
	socket->outBuffer.data[socket->outBuffer.used] = type;
	socket->outBuffer.data[socket->outBuffer.used+1] = 3;
	socket->outBuffer.data[socket->outBuffer.used+2] = 1;
	socket->outBuffer.data[socket->outBuffer.used+3] = (unsigned char)(len/256);
	socket->outBuffer.data[socket->outBuffer.used+4] = (unsigned char)len;
	memcpy(socket->outBuffer.data + 5 + socket->outBuffer.used, data, len*sizeof(char));
	socket->outBuffer.used += len + 5;

	PostMessage(ghWnd, WMU_SCRIPTED_SOCKET, socket->callback->sock, FD_WRITE);
	return 1;
}

void SocketStartTLS(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*) s.objectVal->values[0].stringVal->value;
	CreateNullValue(s);
	if (!socket->tls) {
		char *data = 0;
		ClientHello hello;
		memset(&hello, 0, sizeof(hello));
		hello.type = 1;
		hello.size[2] = sizeof(hello)-4;
		hello.client_version.major = 3;
		hello.client_version.minor = 1;
		hello.numCompressionMethodBytes = 1;
		hello.numCipherSuiteBytes = sizeof(hello.cipher_suites);
		for (int i=0; i<sizeof(hello.cipher_suites)/2; i++) {
			hello.cipher_suites[i].byte[1] = (char)1+i;
		}
		for (int i=0; i<sizeof(hello.random.random_bytes); i++) {
			hello.random.random_bytes[i]=i;
		}
		hello.random.gmt_unix_time = htonl((int) time(0));
		/*
		unsigned char test[] = {0x01, 0x00, 0x00, 0x5d, 0x03, 0x01, 0x44, 0x97, 0x7a, 0x83, 0xa2, 0x63, 0x69, 0xec, 0xf3, 0x45, 0xe1, 0x57, 0x88, 0x6d, 0x15, 0x4c, 0xa9, 0xd8, 0x4e, 0x7c, 0xea, 0xfb, 0xc3, 0xc6, 0x7e, 0xca, 0x51, 0x06, 0xf2, 0xa2, 0xab, 0x3e, 0x00, 0x00, 0x36, 0x00, 0x39, 0x00, 0x38, 0x00, 0x35, 0x00, 0x16, 0x00, 0x13, 0x00, 0x0a, 0x00, 0x33, 0x00, 0x32, 0x00, 0x2f, 0x00, 0x07, 0x00, 0x66, 0x00, 0x05, 0x00, 0x04, 0x00, 0x63, 0x00, 0x62, 0x00, 0x61, 0x00, 0x15, 0x00, 0x12, 0x00, 0x09, 0x00, 0x65, 0x00, 0x64, 0x00, 0x60, 0x00, 0x14, 0x00, 0x11, 0x00, 0x08, 0x00, 0x06, 0x00, 0x03, 0x01, 0x00};

		FILE *goat = fopen("goat.txt", "wb");
		for (int i=0; i<sizeof(test); i++) {
			fprintf(goat, "%02X ", test[i]);
		}
		fprintf(goat, "\n");
		for (int i=0; i<sizeof(hello); i++) {
			fprintf(goat, "%02X ", ((unsigned char*)&hello)[i]);
		}
		fclose(goat);
		//*/
		//if (!TlsSend(socket, (unsigned char*)&test, sizeof(test), 22)) {
		if (!TlsSend(socket, (unsigned char*)&hello, sizeof(hello), 22)) {
			return;
		}
		socket->tls = 1;
	}
}

/*<?xml version='1.0' encoding='UTF-8'?>

<stream:stream
xmlns:stream="http://etherx.jabber.org/streams"
xmlns="jabber:client"
from="supergoat"
id="84b1b26f"
xml:lang="en"
version="1.0">

<stream:features>

<starttls xmlns="urn:ietf:params:xml:ns:xmpp-tls">
</starttls>

<mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">
	<mechanism>DIGEST-MD5</mechanism>
	<mechanism>PLAIN</mechanism>
	<mechanism>ANONYMOUS</mechanism>
	<mechanism>CRAM-MD5</mechanism>
</mechanisms>

<compression xmlns="http://jabber.org/features/compress">
	<method>zlib</method>
</compression>

<auth xmlns="http://jabber.org/features/iq-auth"/>
<register xmlns="http://jabber.org/features/iq-register"/>
</stream:features>
//*/
void CleanupSocket(ScriptValue &s, ScriptValue *args) {
	ScriptedSocket *socket = (ScriptedSocket*) s.objectVal->values[0].stringVal->value;
	if (socket->connected && socket->callback) CleanupCallback(socket->callback);
	//if (socket->ip) socket->ip->Release();
	for (int i=0; i<numSockets; i++) {
		if (sockets[i] == s.objectVal) {
			sockets[i] = sockets[--numSockets];
			break;
		}
	}
	if (socket->stack) {
		socket->stack->PushNull();
		PostMessage(ghWnd, WMU_CALL_PROC, (WPARAM)socket->stack, (LPARAM)RunStack);
	}
	if (socket->replyTo) {
		socket->replyTo->Release();
	}
	free(socket->inBuffer.data);
	free(socket->outBuffer.data);
}

void CleanupPings() {
	while (numPings) RemovePing(0);
	for (int i = numSockets-1; i>=0; i--) {
		ScriptedSocket *socket = (ScriptedSocket *) sockets[i]->values[0].stringVal->value;
		sockets[i]->AddRef();
		if (socket->stack) {
			VM.CleanupStack(socket->stack);
			socket->stack = 0;
		}
		if (socket->callback) {
			CleanupCallback(socket->callback);
			socket->callback = 0;
			socket->connected = -1;
		}
		ObjectValue *o = socket->replyTo;
		socket->replyTo = 0;
		o->Release();
		sockets[i]->Release();
	}
	while (numPings) RemovePing(0);
	free(pings);
}
