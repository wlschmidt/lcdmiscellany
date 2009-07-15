#include "ScriptIP.h"
#include "../../config.h"
#include "../../http/httpGetPage.h"
#include "../../timer.h"
#include "../../stringUtil.h"
__int64 nextDNS = 0, nextRIP = 0;
int RIPFreq, RIPRetry, DNSFreq;
unsigned int remoteIP = 0;
NetworkNames *names = 0;
unsigned char *IPLookups = 0;
int lastIPAttempt = 0;

void UpdateDNS(void *v) {
	NetworkNames** n =  (NetworkNames**)v;
	if (n && n[0]) {
		if (names) {
			int flags = 0;
			if (names->remote && n[0]->remote) {
				if (strcmp(names->remote->ip, n[0]->remote->ip)) {
					flags |= 4;
				}
				if (strcmp(names->remote->dns, n[0]->remote->dns)) {
					flags |= 8;
				}
			}
			if (names->locals[0] && n[0]->locals[0]) {
				if (strcmp(names->locals[0]->ip, n[0]->locals[0]->ip)) {
					flags |= 1;
				}
				if (strcmp(names->locals[0]->dns, n[0]->locals[0]->dns)) {
					flags |= 2;
				}
			}
			if (flags) {
				//IpChanged(flags);
			}
			names->Cleanup();
		}
		names = n[0];
		n[0] = 0;
	}
}

void CheckDNS() {
	__int64 t = time64i();
	if (!DNSFreq) {
		config.SaveConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"DNS Timer", 4*60);
		DNSFreq = config.GetConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"DNS Timer", 4*60);
		nextDNS = t;
		if (DNSFreq < 60) DNSFreq = 60;
	}
	if (t - nextDNS >= 0) {
		GetNamesAsync(WMU_CALL_PROC, ghWnd, remoteIP, (LPARAM)UpdateDNS);
		nextDNS = t + DNSFreq;
	}
}

inline void IPLookupFail() {
	nextRIP = time64i() + RIPRetry;
}

void FindIP(void *s) {
	BufferedHttpSocket *socket = GetBufferedHttpSocket((int)(__int64)s);
	if (socket) {
		if (socket->result) {
			char *text = socket->result->data;
			//text = "test72.14.205.99 204.92.73.10supergoat";
			int len = socket->result->len;
			int result = 0;
			int quality = -10000;
			int ip = 0;
			for (int i=0; i<len; i++) {
				int j = i;
				int res[4] = {0,0,0,0};
				int bit = 0;
				while (1) {
					int count = 0;
					while ('0'<=text[j] && text[j]<='9' && j < len) {
						res[bit] = res[bit]*10 + text[j]-'0';
						count = 1;
						if (res[bit] > 255) {
							break;
						}
						j++;
					}
					if (!count || res[bit] > 255) {
						break;
					}
					bit ++;
					if (bit == 4) break;
					if (text[j] != '.') break;
					j++;
				}
				if (bit == 4) {
					int score = 0;
					if (i && text[i-1] == '/') score --;
					if (i && !IsWhitespace(text[i-1])) score -=2*(2-IsPunctuation(text[i-1]));
					if (j < len - 1) {
						if (!IsWhitespace(text[j])) score -=2*(2-IsPunctuation(text[j]));
					}
					if (res[0] == 0 && res[1] == 0 && res[2] == 0 && res[3] == 0) score -= 100;
					else if (res[0] >= 244 && res[239]<=239) score -= 100;
					else if (res[0] == 10) score -= 20;
					else if (res[0] == 0) score -= 50;
					else if (res[0] == 172 && res[1] >= 16 && res[2] <= 31) score -= 20;
					else if (res[0] == 192 && res[1] == 168) score -= 20;
					else if (res[0] == 169 && res[1] == 254) score -= 40;
					else if (res[0] == 117) score -= 40;
					if (score > quality) {
						quality = score;
						ip = (res[0] << 24) + (res[1]<<16) + (res[2]<<8) + res[3];
						if (!quality) break;
					}
				}
				i = j;
			}
			remoteIP = ip;
			if (ip) {
				nextDNS = time64i();
				CheckDNS();
				//systemStats.IPTimer.ReInit(0, 120, RUN_ONCE, SAVE_DELAY);
				//lastIPLookupSuccess = GetTickCount()/1000;
				//lastLocalIPLookup = 0;
				//DNSTimerProc(0);
			}
			else {
				IPLookupFail();
			}
		}
		//char test[1000];
		//gethostname(test, 1000);
		socket->Cleanup();
		//nextDNS = time(0);
		//CheckDNS();
	}
}


void CheckRIP() {
	if (!nextRIP) {
		config.SaveConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"Remote IP Timer", 120*60);
		config.SaveConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"Remote IP Retry Timer", 4*60);
		RIPFreq = config.GetConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"Remote IP Timer", 120*60);
		RIPRetry = config.GetConfigInt((unsigned char*)"Timer Delay", (unsigned char*)"Remote IP Retry Timer", 4*60);
		if (RIPFreq < 600) RIPFreq = 600;
		if (RIPRetry < 60) RIPRetry = 60;
		nextRIP = time64i();
		IPLookups = config.GetConfigString((unsigned char*)"Status", (unsigned char*)"IP Lookup", (unsigned char*)"");
		if (!IPLookups || IPLookups[0] == 0) {
			free(IPLookups);
			IPLookups = 0;
			lastIPAttempt = -1;
		}
		else {
			int count = 1;
			char *start = (char*)IPLookups;
			while (1) {
				start = strchr(start, ' ');
				if (!start) break;
				while(*start == ' ' || *start == '\t')
					start++;
				count++;
			}
			lastIPAttempt = (int) ((rand()/(float) RAND_MAX) * count);
		}
	}
	__int64 t = time64i();
	if (t - nextRIP >= 0) {
		if (IPLookups) {
			nextRIP = t + RIPFreq;
			unsigned char *start = IPLookups;
			int i = ++lastIPAttempt;
			while (i) {
				i--;
				start = strchr(start, ' ');
				if (!start) {
					i = 0;
					start = IPLookups;
					lastIPAttempt = 0;
					break;
				}
				while(*start == ' ' || *start == '\t')
					start++;
			}
			if (start && start[0]) {
				unsigned char *s = strchr(start, ' ');
				unsigned char *s2 = strchr(start, '\t');
				if (s2 && (!s || s2 < s)) {
					s = s2;
				}
				if (s) *s = 0;
				if (!HTTPGet((char*)start, ghWnd, WMU_CALL_PROC, (LPARAM) FindIP)) {
					IPLookupFail();
					//lastIPLookupFail = (unsigned int) t;
				}
				if (s) *s = ' ';
			}
			else
				IPLookupFail();
		}
	}
	else CheckDNS();
}

void GetLocalIP(ScriptValue &s, ScriptValue *args) {
	CheckDNS();
	char *c;
	if (!names || !names->numLocals) c = "0.0.0.0";
	else {
		c = names->locals[0]->ip;
		for (int i=0; i<names->numLocals; i++) {
			if (names->locals[i]->family == AF_INET) {
				c = names->locals[i]->ip;
				break;
			}
		}
	}
	CreateStringValue(s, (unsigned char*)c);
}
void GetRemoteIP(ScriptValue &s, ScriptValue *args) {
	CheckRIP();
	char *c;
	if (!names || !names->remote) c = "0.0.0.0";
	else c = names->remote->ip;
	CreateStringValue(s, (unsigned char*)c);
}
void GetRemoteDNS(ScriptValue &s, ScriptValue *args) {
	CheckRIP();
	char *c;
	if (!names || !names->remote) c = "0.0.0.0";
	else c = names->remote->dns;
	CreateStringValue(s, (unsigned char*)c);
}
void GetLocalDNS(ScriptValue &s, ScriptValue *args) {
	CheckDNS();
	char *c;
	if (!names || !names->remote) c = "localhost";
	else {
		c = names->locals[0]->dns;
		for (int i=0; i<names->numLocals; i++) {
			if (names->locals[i]->family == AF_INET) {
				c = names->locals[i]->dns;
				break;
			}
		}
	}
	CreateStringValue(s, (unsigned char*)c);
}

void CleanupIP() {
	if (names) names->Cleanup();
	free(IPLookups);
	names = 0;
	IPLookups = 0;
}
