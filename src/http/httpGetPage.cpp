#include "httpGetPage.h"
#include <stdlib.h>
#include <time.h>
#include "..\\stringUtil.h"
#include <Winsock2.h>
#include "..\\util.h"
#include "..\\malloc.h"
#include "..\\images\\inflate.h"
#include "..\\Timer.h"

BufferedHttpSocket **httpSocket = 0;
int numHttpSockets = 0;

/*
struct IdleHttpSocket {
	__int64 closeTime;
	SOCKET socket;
	char *dns;
	unsigned short port;
};
IdleHttpSocket *idleHttpSockets = 0;
int numIdleHttpSockets = 0;

void CleanupIdleHttpSocket(int num) {
	if (idleHttpSockets[num].socket != INVALID_SOCKET) {
		closesocket(idleHttpSockets[num].socket);
	}
	if (idleHttpSockets[num].dns) free(idleHttpSockets[num].dns);
	idleHttpSockets[num] = idleHttpSockets[--numIdleHttpSockets];
}

void CheckIdleHttpSockets(__int64 time, __int64 timeFix) {
	for (int i=0; i<numIdleHttpSockets; i++) {
		if (time >= (idleHttpSockets[i].closeTime += timeFix)) {
			CleanupIdleHttpSocket(i);
			i--;
		}
	}
}
//*/
extern NetworkNames *names;

void BufferedHttpSocket::Cleanup() {
	// Works but not too efficient.  I'm lazy.
	if (id && GetBufferedHttpSocket(id)) {
		CleanupHttpSocket(id);
		return;
	}
	if (url) free(url);
	if (dns) free(dns);
	if (path) free(path);
	if (connectCallback) CleanupCallback(connectCallback);
	//if (firstAddr) freeaddrinfo(firstAddr);
	if (buffer) free(buffer);
	if (outBuffer) free(outBuffer);
	if (result) result->Cleanup();
	if (type) free(type);
	if (userName) free(userName);
	if (pass) free(pass);
	//if (sock != INVALID_SOCKET) closesocket(sock);
	free(this);
}

void BufferedHttpSocket::Try() {
	Hangup();
	if (result) {
		result->Cleanup();
		result = 0;
	}
	if (type) {
		free(type);
		type = 0;
	}
	expected = 0;
	status = STATUS_CONNECTING;
	TrySocket(connectCallback);
	//currentAddr = currentAddr->ai_next;
	//TryConnect();
	//PostMessage(hWnd, WMU_START_CONNECTING, id, 0);
}

void BufferedHttpSocket::TryNext() {
	Hangup();
	if (result) {
		result->Cleanup();
		result = 0;
	}
	if (type) {
		free(type);
		type = 0;
	}
	status = STATUS_CONNECTING;
	expected = 0;
	TryNextSocket(connectCallback);
	//currentAddr = currentAddr->ai_next;
	//TryConnect();
	//PostMessage(hWnd, WMU_START_CONNECTING, id, 0);
}

void HttpFailProc(__int64 id) {
	BufferedHttpSocket *s = GetBufferedHttpSocket((int)id);
	if (s) {
		//CleanupCallback(s->connectCallback);
		s->connectCallback = 0;
		s->status = STATUS_FAILED;
		PostMessage(s->hWnd, s->completionMessage, s->id, s->lParam);
	}
}

int HttpSockInit(ConnectCallback* callback, __int64 id) {
	BufferedHttpSocket *s = GetBufferedHttpSocket((int)id);
	if (!s) return 0;
	s->connectCallback = callback;
	s->status = STATUS_CONNECTING;
	return 1;
}

BufferedHttpSocket *HTTPGet(char *url, HWND hWnd, int msg, LPARAM lParam) {
	if (!srealloc(httpSocket, sizeof(BufferedHttpSocket*)*(numHttpSockets+1))) return 0;
	BufferedHttpSocket *out = (BufferedHttpSocket*) calloc(1, sizeof(BufferedHttpSocket)+100);
	if (!out) return 0;
	out->url = strdup(url);
	if (!out->url) {
		free(out);
		return 0;
	}
	out->status = STATUS_GETTING_IP;
	//out->sock = INVALID_SOCKET;
	out->dns = GetDNSAndPath(out->url, out->path, out->port, out->protocol, out->userName, out->pass, 0);

	static int id = 0;
	id++;
	// disallow id of 0 for simplicity.
	while (GetBufferedHttpSocket(id) || !id) id++;
	// 3 minutes for finding a happy ip and getting the file.
	// currently only checked when openning a new socket
	out->startTime = GetTickCount()/1000;
	out->timeoutTime = out->startTime+180;

	out->id = id;
	out->hWnd = hWnd;
	out->completionMessage = msg;
	out->lParam = lParam;
	httpSocket[numHttpSockets++] = out;

	for (int i=numHttpSockets-2; i>=0; i--) {
		int delta = (int) (out->startTime - httpSocket[i]->timeoutTime);
		if ((delta > 0 || httpSocket[i]->timeoutTime < out->startTime && out->startTime > 180) && httpSocket[i]->status != STATUS_GETTING_IP) {
			PostMessage(httpSocket[i]->connectCallback->hWnd, httpSocket[i]->connectCallback->message, FD_CLOSE, (LPARAM)httpSocket[i]->connectCallback->sock);
		}
	}

	if (!out->dns || out->protocol != PROTOCOL_HTTP ||
		!LookupDNSThreadConnectCallback(out->dns, out->port, IPPROTO_TCP, HttpFailProc, HttpSockInit, WMU_HTTP_SOCKET, ghWnd, id)) {
		//!LookupDNSThread(out->dns, out->port, IPPROTO_TCP, HTTPGotDNS, ghWnd, id)) {
		// Needed to prevent a wait - loop until sockets other threads are working on close,
		// and STATUS_GETTING_IP is the flag for that...obviously could result in issues.
		out->status = STATUS_FAILED;
		out->Cleanup();
		return 0;
	}
	return out;
}



void HandleHttpSocketMessage(WPARAM wParam, LPARAM lParam) {
	SOCKET socket = (SOCKET) wParam;
	BufferedHttpSocket *sock = 0;
	for (int i=0; i<numHttpSockets; i++) {
		if (httpSocket[i]->connectCallback && httpSocket[i]->connectCallback->sock == socket) {
			sock = httpSocket[i];
			break;
		}
	}
	if (!sock) {
		/*for (int i=0; i<numIdleHttpSockets; i++) {
			if (idleHttpSockets[i].socket == socket) {
				CleanupIdleHttpSocket(i);
				return;
			}
		}//*/
		// Should already be closed if we've deleted the object already
		// or replaced an existing object's socket.
		// closesocket(socket);
		return;
	}
	int error =  WSAGETSELECTERROR(lParam);
	int msg =  WSAGETSELECTEVENT(lParam);
	if (sock->status == STATUS_SUCCESS) {
		CleanupCallback(sock->connectCallback);
		sock->connectCallback = 0;
		// Presumably I either put off or forgot to close the connection.
		// closesocket(socket);
		// sock->sock = INVALID_SOCKET;
	}
	else if (sock->status != STATUS_READING_HEADER && sock->status !=  STATUS_READING_FILE) {
		// Should be connecting.  If not, something's wrong so hangup anyways.
		if (error || msg != FD_CONNECT || sock->status != STATUS_CONNECTING) {
			sock->TryNext();
		}
		else {
			sock->SendRequest(sock->path);
		}
	}
	else if (msg == FD_WRITE && !error) {
		sock->SendData();
	}
	else if (msg == FD_READ && !error) {
		sock->ReadData();
	}
	else if (msg == FD_CLOSE || error) {
		// Check if already closed the connection and
		// cleaned up everything.
		if (sock->connectCallback) {
			sock->ReadData();
			sock->CloseConnection(error);
		}
	}
	else {
		sock->TryNext();
	}
}

int Base64(char *out, char *s, int len = -1) {
	if (len < 0) len = (int)strlen(s);
	const char *sub = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	//char *out = (char*)malloc(10 + len*4/3);
	int p = 0;
	int pos = 0;
	while (p < len) {
		unsigned int w = (s[p]<<16);
		if (p+1 < len) {
			w += s[p+1] << 8;
			if (p+2 < len) {
				w += s[p+2];
			}
		}
		for (int i=0; i<4; i++) {
			out[pos+i] = sub[(w>>((3-i)*6)) & 63];
		}
		if (p+2 >= len) {
			out[pos+3] = '=';
			if (p+1 >= len) {
				out[pos+2] = '=';
			}
		}

		pos += 4;
		p += 3;
	}
	out[pos] = 0;
	//free(out);
	//return 0;
	return pos;
}

void BufferedHttpSocket::SendRequest(char *location) {
	if (strchr(location, '?')) {
		location = location;
	}
	int len = (int) (strlen(location) + 110 + strlen(dns));
	int len2;
	if (userName && pass) {
		len2 = (int)(strlen(userName) + strlen(pass));
		len += 60 + 4*(len2)/3;
	}
	outBuffer = (char*) malloc(sizeof(char)*(len));
	if (!outBuffer) {
		TryNext();
	}
	else {
		status = STATUS_READING_HEADER;
		if (port == 80)
			sprintf(outBuffer, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: " APP_NAME "\r\nAccept-Encoding: gzip\r\n\r\n", location, dns);
		else
			sprintf(outBuffer, "GET %s HTTP/1.1\r\nHost: %s:%i\r\nUser-Agent: " APP_NAME "\r\nAccept-Encoding: gzip\r\n\r\n", location, dns, (int)port);
		outBufferLen = (int)strlen(outBuffer);
		if (userName && pass) {
			strcpy(outBuffer + (outBufferLen-2), "Authorization: Basic ");
			outBufferLen += 19;
			char *temp = outBuffer + (len - 2 - len2);
			sprintf(temp, "%s:%s", userName, pass);
			outBufferLen += Base64(outBuffer+outBufferLen, temp);
			strcpy(outBuffer + outBufferLen, "\r\n\r\n");
			outBufferLen += 4;
		}
		SendData();
	}
}

void BufferedHttpSocket::SendData() {
	if (outBufferLen) {
		int res = send(connectCallback->sock, outBuffer, outBufferLen, 0);
		if (res == SOCKET_ERROR || !res) {
			int test = WSAGetLastError();
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				TryNext();
			}
			return;
		}
		outBufferLen -= res;
		memmove(outBuffer, outBuffer+res, outBufferLen*sizeof(char));
		srealloc(outBuffer, sizeof(char)*outBufferLen);
	}
}

void BufferedHttpSocket::ReadData() {
	unsigned long len;
	int res = ioctlsocket(connectCallback->sock, FIONREAD, &len);
	if (res == SOCKET_ERROR) {
		TryNext();
		return;
	}
	if (len == 0) return;
	int newLen = len + bufferLen;
	if (len + bufferLen > 50*1024*1024) {
		len = 50*1024*1024 - bufferLen;
	}
	if (!srealloc(buffer, (1+newLen)*sizeof(char))) {
		TryNext();
		return;
	}
	int w = recv(connectCallback->sock, buffer+bufferLen, len, 0);
	if (!len) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			TryNext();
		}
		return;
	}
	bufferLen += w;
	buffer[bufferLen] = 0;
	//*
	//FILE *out = fopen("temp.txt", "wb");
	//fwrite(buffer, 1, bufferLen, out);
	//fclose(out);
	while (status == STATUS_READING_HEADER) {
		int cmp;
		char* compare = "HTTP/";
		for (cmp = 0; cmp<5; cmp ++) {
			if (cmp == bufferLen) return;
			if (buffer[cmp] != compare[cmp]) {
				break;
			}
		}
		if (cmp == 5 && buffer[cmp] >= '0' && buffer[cmp] <= '9') {
			char *pos1 = buffer+cmp;
			while (buffer[cmp] >= '0' && buffer[cmp] <= '9' && cmp < 15 && cmp<bufferLen) cmp++;
			if (cmp<bufferLen && buffer[cmp]=='.') {
				cmp++;
				char *pos2 = buffer+cmp;
				if (cmp<bufferLen && buffer[cmp] >= '0' && buffer[cmp] <= '9') {
					while (buffer[cmp] >= '0' && buffer[cmp] <= '9' && cmp < 20 && cmp<bufferLen) cmp++;

					if (cmp < bufferLen && (buffer[cmp] == ' ' || buffer[cmp] == '\t')) {
						char *location = 0;
						char *end1 = strstr(buffer, "\n\r\n");
						char *end2 = strstr(buffer, "\n\n");
						if (end1 == 0 || (end2 && end2<end1))
							end1 = end2;
						if (end1 == 0) {
							if (len == 50*1024*1024) TryNext();
							return;
						}
						end1++;
						if (*end1 !='\n') end1++;
						*end1 = 0;
						end2 = pos1;

						// Whew.  We have what looks to be a header.
						httpMajor = strtoul(pos1, &pos1, 0);
						httpMinor = strtoul(pos2, &pos2, 0);
						httpCode = strtoul(buffer+cmp, &pos2, 0);

						while (pos1 = strstr(pos1, "\n ")) {
							*pos1 = ' ';
							if (pos1[-1] == '\r') pos1[-1] = ' ';
						}
						pos1 = buffer;
						while (pos1 = strstr(pos1, "\n\t")) {
							pos1[0] = ' ';
							pos1[1] = ' ';
							if (pos1[-1] == '\r') pos1[-1] = ' ';
						}
						pos1 = buffer;

						expected = -1;
						while (pos1 = strchr(pos1, '\n')) {
							pos1++;
							char *val = pos1;
							// find the value and tidy it up.  Replace tabs with spaces,
							// remove redundant spaces, remove spaces near commas, etc.
							while (*val && *val !=' ' && *val != '\t' && *val != '\r' && *val != '\n') val++;
							while (*val == '\t' || *val == ' ') val++;
							char *pos3 = val, *pos4=val;
							pos3[-1] = ' ';
							while (*pos4 && *pos4!='\r' && *pos4!='\n') {
								if (*pos4 == ' ' || *pos4 =='\t') {
									if (pos3[-1] != ' ' && pos3[-1] != ',') {
										pos3[0] = ' ';
										pos3++;
									}
								}
								else if (*pos4 == ',' && pos3[-1] == ' ') {
									pos3[-1] = ',';
								}
								else {
									pos3[0] = *pos4;
									pos3++;
								}
								pos4++;
							}
							if (substricmp(pos1, "Content-Type:") == 0) {
								srealloc(type, sizeof(char) * (1+pos3-val));
								if (!type) {
									if (location) free(location);
									TryNext();
									return;
								}
								memcpy(type, val, sizeof(char)*(pos3-val));
								type[pos3-val] = 0;
							}
							else if (substricmp(pos1, "Content-Length:") == 0) {
								if (val[0] >= '0' && val[0] <= '9')
									expected = strtoi64(val, &pos2, 0);
								else
									expected = -1;
							}
							/*else if (substricmp(pos1, "WWW-Authenticate:") == 0) {
								if (substricmp(val, "Basic") == 0) {
									if (val[0] >= '0' && val[0] <= '9')
										expected = strtoi64(val, &pos2, 0);
									else
										expected = -1;
								}
							}//*/
							else if (substricmp(pos1, "Connection:") == 0) {
								if (substricmp(val, "Close") == 0 && pos3-val == 5) {
									close = 1;
								}
							}
							else if (substricmp(pos1, "Transfer-Encoding:") == 0) {
								if (substricmp(val, "chunked") == 0 && pos3-val == 7) {
									chunked = -1;
								}
							}
							else if (substricmp(pos1, "Content-Encoding:") == 0) {
								if ((substricmp(val, "gzip") == 0 && pos3-val == 4) ||
									(substricmp(val, "x-gzip") == 0 && pos3-val == 6)) {
									gzip = 1;
								}
								else if (substricmp(val, "identity") && pos3-val > 1) {
									gzip = 1;
								}
							}
							else if (substricmp(pos1, "Location:") == 0) {
								srealloc(location, sizeof(char) * (1+pos3-val));
								if (!location) {
									TryNext();
									return;
								}
								memcpy(location, val, sizeof(char)*(pos3-val));
								location[pos3-val] = 0;
							}
						}
						if (expected > 50*1024*1024) {
							expected = -1;
						}

						memmove(buffer, end1+1, sizeof(char)*(1+(bufferLen-=(int)(end1+1-buffer))));
						// Pointless 1.1 keepalive thing.
						if (httpCode == 100) {
							if (location) free(location);
							continue;
						}
						status = STATUS_READING_FILE;
						if (httpCode == 302 || httpCode == 301 || httpCode == 303 || httpCode == 307) {
							if (!location) TryNext();
							if (strchr(location, ':')) {
								BufferedHttpSocket *b;
								if (b = HTTPGet(location, hWnd, completionMessage, lParam)) {
									free(location);
									CleanupHttpSocket(b->id);
								}
								else {
									free(location);
									TryNext();
								}
							}
							else if (httpMajor > 1 || httpMinor >= 1) {
								// Lame hack.  works with one website which is broken, anyways.
								if (location[0] != '/' && srealloc(location, sizeof(char)*(strlen(location)+ 2))) {
									memmove(location+1, location, sizeof(char)*(strlen(location)+1));
									location[0] = '/';
								}
								SendRequest("/pages/en.home.php");
								free(location);
								status = STATUS_READING_HEADER;
							}
							else {
								free(path);
								path = location;
								Try();
							}
							return;
						}
						else {
							if (location) free(location);
							if (httpCode == 200 || httpCode == 207) {
								if (expected)
									continue;
							}
							else TryNext();
							return;
						}
					}
				}
			}
		}
		if (cmp == bufferLen) {
			// don't have full "http/x.y" yet, but no contradictions, either.
			return;
		}
		// Eeep!  Assuming http/0.9.
		expected = -1;
		status = STATUS_READING_FILE;
		break;
	}
	//*/
	// reading file
	//buffer = strdup("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	//buffer = strdup("1a; ignore-stuff-here\r\nabcdefghijklmnopqrstuvwxyz\r\n10\r\n1234567890abcdef\r\n0\r\nsome-footer: some-value\r\nanother-footer: another-value\r\n\r\n");
	//bufferLen = strlen(buffer);
	//chunked = -1;
	if (chunked) {
		while (1) {
			if (chunked > bufferLen) {
				if (bufferLen == 50*1024*1024) {
					expected = -1;
					CloseConnection(0);
					return;
				}
				else return;
			}
			char *pos = buffer+chunked;
			char *dest;
			if (chunked == -1) {
				pos ++;
				dest = pos;
			}
			else {
				dest = pos;
				if (*pos == '\r') pos++;
				if (*pos == '\n') pos++;
			}
			if (pos - buffer >= bufferLen) break;
			char *end = strchr(pos, '\n');
			if (!end) break;
			__int64 len = 0;
			int digits = 0;
			while (1) {
				if (pos[digits] >= '0' && pos[digits] <= '9') {
					len = len*16 + pos[digits]-'0';
				}
				else if (pos[digits] >= 'a' && pos[digits] <= 'f') {
					len = len*16 + pos[digits]-'a'+10;
				}
				else if (pos[digits] >= 'A' && pos[digits] <= 'F') {
					len = len*16 + pos[digits]-'a'+10;
				}
				else break;
				digits ++;
			}
			if (digits == 0 || digits >= 13) {
				len = -1;
			}
			if (pos[digits] != ';' && pos[digits] != ',' && pos[digits] != '\r' && pos[digits] != '\n' &&
				pos[digits] != '-' && pos[digits] != ' ' && pos[digits] != '\t' && pos[digits] != '.') {
					len = -1;
			}
			if (len > 50*1024*1024) len = 50*1024*1024;
			if (len == -1) {
				CloseConnection(1);
				return;
			}
			if (len) {
				memmove(dest, end+1, sizeof(char)*(bufferLen-(end-buffer)));
				bufferLen-=(int)(end+1-dest);
				chunked = (int)(len+(pos-buffer));
			}
			else {
				char *end1 = strstr(pos, "\n\r\n");
				char *end2 = strstr(pos, "\n\n");
				if (!end1 || (end2 && end2 < end1)) end1 = end2-1;
				if (end1) {
					end1 += 3;
					memmove(dest, end1, sizeof(char)*(1+bufferLen-(end1-buffer)));
					bufferLen-=(int)(end1-dest);
					expected = chunked;
					chunked = 0;
					status = STATUS_SUCCESS;
					MakeResult((int)expected);
					return;
				}
			}
		}
	}
	if (expected == -1) {
		if (bufferLen == 50*1024*1024) {
			CloseConnection(0);
		}
	}
	else if (bufferLen >= expected) {
		status = STATUS_SUCCESS;
		MakeResult((int)expected);
	}
}

void BufferedHttpSocket::MakeResult(int size) {
	result = (HTTPFile*) malloc(sizeof(HTTPFile));
	char *data = 0;
	unsigned int dataSize;
	if (expected < 0) expected = bufferLen;
	else if (expected > bufferLen) expected = bufferLen;
	if (gzip) {
		char *uncompressed = InflateGzip(buffer, bufferLen, dataSize);
		if (!uncompressed) {
			free(buffer);
			buffer = 0;
			bufferLen = 0;
			result = 0;
			size = 0;
			expected = 0;
			status = STATUS_FAILED;
			// Think this works here.
			TryNext();
			// Think this is right...
			return;
		}
		bufferLen -= (int)expected;
		if (bufferLen) {
			free(buffer);
			buffer = 0;
			expected = 0;
		}
		else {
			memmove(buffer, buffer+(int)expected, sizeof(char)*bufferLen);
			srealloc(buffer, bufferLen);
		}
		data = uncompressed;
	}
	else {
		dataSize = (int)expected;
		if (expected == bufferLen) {
			data = buffer;
			bufferLen = 0;
			buffer = 0;
		}
		else {
			data = (char*) malloc(sizeof(char)*(int)expected);
			if (data)
				memcpy(data, buffer, sizeof(char)*(int)expected);
			memmove(buffer, buffer+expected, sizeof(char)*(bufferLen-=(int)expected));
			srealloc(buffer, bufferLen);
		}
		expected = 0;
	}
	if (!data || !result) {
		free(data);
		free(result);
		result = 0;
		status = STATUS_FAILED;
		// Think this works here.
		TryNext();
		// Think this is right...
		return;
	}
	else {
		result->data = data;
		result->len = dataSize;
	}
	if (close) {
		Hangup();
	}
	/*else {
		if (numIdleHttpSockets %16 == 0 && !srealloc(idleHttpSockets, (numIdleHttpSockets+16) * sizeof(IdleHttpSocket))) {
			Hangup();
		}
		else {
			idleHttpSockets[numIdleHttpSockets].dns = dns;
			idleHttpSockets[numIdleHttpSockets].socket = connectCallback->sock;
			idleHttpSockets[numIdleHttpSockets].port = port;
			idleHttpSockets[numIdleHttpSockets].closeTime = time64i();
			dns = 0;
			connectCallback->sock = INVALID_SOCKET;
			numIdleHttpSockets++;
		}
	}//*/
	PostMessage(hWnd, completionMessage, id, lParam);
}


void BufferedHttpSocket::CloseConnection(int error) {
	if (status != STATUS_SUCCESS) {
		if (bufferLen == 50*1024*1024 || error) {
			status = STATUS_CONFUSED;
		}
		else if (expected == -1)
			status = STATUS_NO_LENGTH;
		else status = STATUS_CONFUSED;
		MakeResult(bufferLen);
	}
	Hangup();
}

void BufferedHttpSocket::Hangup() {
	chunked = 0;
	close = 0;
	if (connectCallback && connectCallback->sock != INVALID_SOCKET) {
		closesocket(connectCallback->sock);
		connectCallback->sock = INVALID_SOCKET;
	}
	if (buffer) {
		free(buffer);
		buffer = 0;
		bufferLen = 0;
	}
	if (outBuffer) {
		free(outBuffer);
		outBuffer = 0;
		outBufferLen = 0;
	}
}

BufferedHttpSocket *GetBufferedHttpSocket(int id) {
	for (int i=0; i<numHttpSockets; i++) {
		if (httpSocket[i]->id == id) {
			return httpSocket[i];
		}
	}
	return 0;
}


// Blocks if a socket is trying to get an ip.
void CleanupHttpSocket(int id) {
	for (int i=0; i<numHttpSockets; i++) {
		if (httpSocket[i]->id == id) {
			// Only cleanup sockets in main thread.  Secondary ones just lookup ips/dnses.
			//while (httpSocket[i]->status == STATUS_GETTING_IP) {
			//	Sleep(1000);
			//}
			// simple recursion prevention.
			httpSocket[i]->id = 0;
			httpSocket[i]->Cleanup();
			httpSocket[i] = httpSocket[--numHttpSockets];
			srealloc(httpSocket, sizeof(BufferedHttpSocket*)*numHttpSockets);
			break;
		}
	}
}

void CleanupHttpSockets() {
	while (numHttpSockets) {
		CleanupHttpSocket(httpSocket[0]->id);
	}
	/*while (numIdleHttpSockets) {
		CleanupIdleHttpSocket(0);
	}//*/
}

char *GetDNSAndPath(const char *url, char *&path, unsigned short &port, unsigned int &protocol, char *&userName, char *&pass, int host) {
	port = 80;
	if (substricmp(url, "http:") == 0) {
		protocol = PROTOCOL_HTTP;
	}
	else if (substricmp(url, "https:") == 0) {
		protocol = PROTOCOL_HTTPS;
	}
	else protocol = PROTOCOL_UNKNOWN;
	int i = 0;
	while (url[i] && url[i] != ':') i++;
	i++;
	while (url[i] && url[i] == '/') i++;
	if (!url[i] || i<2 || url[i-2]!='/')
		return 0;

	int j=i;
	while (url[j] && url[j]!='/') j++;
	int k = j;
	while (k>i && url[k]!=':' && url[k]!='@') {
		k--;
	}
	// end of dns;
	int t = j;
	//if (!host) {
		if (url[k] == ':') {
			//k++;
			for (t=k+1; t<j; t++) {
				if (url[t]<'0' || url[t]>'9') break;
			}
			if (t==j && k<j-1) {
				t = k;
				int port2;
				port2 = strtoul(&url[k+1], 0, 0);
				if (port2 <= 0 || port2 > 0xFFFF) return 0;
				port = (unsigned short) port2;
			}
			else {
				t = j;
			}
		}
	//}
	while (k>i && url[k] != '@') {
		k--;
	}

	char *out;
	if (url[k] == '@') {
		int q = i;
		while (q < k && url[q] != ':') q++;
		userName = (char*)malloc((q-i+1) * sizeof(char));
		if (userName) {
			memcpy(userName, url+i, (q-i) * sizeof(char));
			userName[q-i] = 0;
			q++;
			if (q < k) {
				pass = (char*)malloc((k-q+1) * sizeof(char));
				if (pass) {
					memcpy(pass, url+q, (k-q) * sizeof(char));
					pass[k-q] = 0;
				}
			}
		}
		k++;
	}

	if (k>=t) return 0;

	out = (char*) malloc(sizeof(char)*(t-k+1));
	if (!out) return 0;
	memcpy(out, &url[k], t-k);
	out[t-k] = 0;
	if (host) return out;

	int p = 1;
	while (url[j] && url[j+1]=='/') j++;
	if (!url[j]) p=2;
	path = HTTPEscape(&url[j],(int) strlen(&url[j]), p, 0);
	//path = strdup("http://www.test.net/announce");
	if (!path) {
		free(out);
		return 0;
	}
	return out;
}

int ParseHTMLHeader(int &argc, char ** &argv, char *s, int size) {
	int numRows=0;
	int pos=-2;
	int something=1;
	while (something) {
		something = 0;
		pos+=2;
		while ((pos < size && (s[pos] != '\r' && s[pos] != '\n')) ||
			   (pos < size-1 && (s[pos+1] == ' ' || s[pos+1] == '\t')) ||
			   (pos < size-2 && (s[pos] == '\r' && s[pos+1] == '\n') && (s[pos+2] == ' ' || s[pos+2] == '\t'))) {

			if (s[pos] == '\r' || s[pos] == '\n') {
				int p2 = pos;
				while (p2 && (s[p2-1]==' ' || s[p2-1]=='\t')) p2--;
				pos+=2;
				if (s[pos] == '\r' && s[pos+1] == '\n') pos++;
				while (pos < size && (s[pos]==' ' || s[pos]=='\t')) pos++;
				s[p2] = ' ';
				memcpy(&s[p2+1], &s[pos], size-pos);
				size = (p2+1+size-pos);
				pos = p2;
			}
			something = 1;
			pos++;
		}
		if (something) numRows++;
		if (s[pos] == '\r' && pos<size && s[pos+1] == '\n') pos ++;
	}
	if (numRows == 0 || (s[pos] !='\r' && s[pos] !='\n')) return 0;

	//int endPos = pos+2;
	argv = (char**) malloc(2 * (numRows) * sizeof(char*));
	if (!argv) {
		argv = 0;
		argc = 0;
		return 0;
	}

	argc = numRows;
	pos = 0;
	for (int i=0; i<numRows; i++) {
		while (s[pos]==' ' || s[pos]=='\t') pos++;
		argv[i*2] = &s[pos];
		argv[i*2+1] = 0;
		if (i<numRows) {
			while (pos < size && (s[pos] !='\r' && s[pos] !='\n') && s[pos]!=':') pos++;
			if (s[pos]==':') {
				pos++;
				while (s[pos]==' ' || s[pos]=='\t') pos++;
				argv[i*2+1] = &s[pos];
			}
			while (pos < size && (s[pos] !='\r' && s[pos] !='\n')) pos++;
			int temp = pos;
			while (pos<size && (s[pos] == '\r' || s[pos] == '\n')) pos ++;
			if (temp<size) s[temp] = 0;
		}
		/*else {
			//while (argv[i*2][0] == ' ' || argv[i*2][0] == '\t') argv[i*2]++;
			argv[i*2]+=2;
		}*/
	}
	return 1;
}

char *HTTPUnescape(const char *s, int *len) {
	char *out = (char*) malloc(sizeof(char) * (strlen(s)+1));
	if (!out) return 0;
	int i=0;
	int j=0;
	while (s[i]) {
		if (s[i] != '%') {
			out[j++] = s[i++];
		}
		else {
			i++;
			int res = 0;
			if (s[i]>='0' && s[i]<='9') {
				res = s[i]-'0';
			}
			else {
				char c = (char)UCASE(s[i]);
				if (c>='A' && c<='F') {
					res = c-'A'+ 10;
				}
				else {
					out[j++] = 0;
					continue;
				}
			}
			i++;
			if (s[i]>='0' && s[i]<='9') {
				res = res*16 + s[i]-'0';
				i++;
			}
			else {
				char c = (char)UCASE(s[i]);
				if (c>='A' && c<='F') {
					res = res*16 + c-'A'+ 10;
					i++;
				}
			}
			out[j++] = ((char*)&res)[0];
		}
	}
	out[j] = 0;
	*len = j;
	return out;
}

/*
__forceinline int IsHex(char c) {
	return ('0'<=c && c<='9') || ('A' <= UCASE(c) && UCASE(c)<='F');
}//*/

char *HTTPEscape(const char *s, const int len, int path, int start) {
	char* temp = (char*) malloc(sizeof(char)*(3*len+1+path));
	if (!temp) return 0;
	const unsigned long nonReserved[8] = {0x0, 0x3FF6382, 0x87FFFFFE, 0x07FFFFFE, 0x0, 0x0, 0x0, 0x0};
	const char hex [16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	int p=0;
	int i=0;
	while (i<start) {
		temp[p++] = s[i++];
	}
	int foundQ = 0;
	for (; i<len; i++) {
		if (nonReserved[((unsigned char*)s)[i]/32]&(1<<((unsigned char*)s)[i]%32) ||
			(path && s[i]=='/') ||
			//Note:  This is just to allow escaped urls
			(path && i<len-2 && s[i]=='%' && IsHex(s[i+1]) && IsHex(s[i+2])) || (s[i] == '?' && !foundQ && (foundQ=1))
			|| (s[i]=='&' && foundQ) || (s[i]=='=' && foundQ)) {

			temp[p++] = s[i];
		}
		else {
			temp[p++] = '%';
			temp[p++] = hex[((unsigned char*)s)[i]/16];
			temp[p++] = hex[((unsigned char*)s)[i]%16];
		}
	}
	if (path==2) temp[p++] = '/';
	temp[p]=0;
	return temp;
}
