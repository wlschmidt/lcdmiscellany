#ifdef GOAT

#include <stdlib.h>
#include "WMI.h"
#include "ScriptObjects.h"
#include "../global.h"
#include "../malloc.h"
#include <wbemidl.h>
#include "../unicode.h"
#include "../util.h"


struct WMIServer {
	StringValue *name;
	IWbemServices *services;
	int refs;
};

struct WMIData {
//	StringValue
};

WMIServer *wmiServers = 0;
int numWmiServers = 0;

unsigned int WMIValueType;

void WMIMonitor(ScriptValue &s, ScriptValue *args) {
	HRESULT hr;
	int i;
	for (i=0; i<numWmiServers; i++) {
		if (!scriptstrcmp(args[0].stringVal, wmiServers[i].name)) break;
	}
	if (i == numWmiServers) {
		if (!srealloc(wmiServers, sizeof(WMIServer)*(numWmiServers+1))) return;

		InitCom();
		if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_SECURE_REFS, NULL))) {
			return;
		}

		IWbemLocator *locator;
		hr = CoCreateInstance(CLSID_WbemLocator, 0, 
			CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &locator);
	 
		if (FAILED(hr)) {
			locator = 0;
			UninitCom();
			return;
		}

		wmiServers[i].name = args[0].stringVal;

		BSTR bstr = UTF8toBSTR(args[0].stringVal->value, &args[0].stringVal->len);
		if (!bstr) {
			locator->Release();
			UninitCom();
			return;
		}
		wmiServers[i].services = 0;
		hr = locator->ConnectServer(bstr, 0, 0, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &wmiServers[i].services);
		freeBSTR(bstr);
		if (FAILED(hr)) {
			locator->Release();
			UninitCom();
			return;
		}

		args[0].stringVal->AddRef();
		numWmiServers++;
	}
	wmiServers[i].refs++;
	BSTR bstr = UTF8toBSTR(args[1].stringVal->value, &args[1].stringVal->len);
	if (bstr) {
		IEnumWbemClassObject *penum;
		HRESULT res = wmiServers[i].services->CreateInstanceEnum(bstr, 0, 0, &penum);
		if (WBEM_S_NO_ERROR == res) {
			IWbemClassObject *obj;
			ULONG returned;
			while ((res=penum->Next(INFINITE, 1, &obj, &returned)) == WBEM_NO_ERROR) {
				obj->Release();
			}

			penum->Release();
		}
		//wmiServers[i].services->GetObjectW(bstr, 0, 0, 
		freeBSTR(bstr);
	}
}

void CleanupServer(int i) {
	wmiServers[i].name->Release();
	wmiServers[i].services->Release();
	wmiServers[i] = wmiServers[--numWmiServers];
	UninitCom();
}

void CleanupWMI() {
	while (numWmiServers)
		CleanupServer(0);
}

void WMIGetVals(ScriptValue &s, ScriptValue *args) {
}

void DeleteWMIMonitor(ScriptValue &s, ScriptValue *args) {
}

#endif