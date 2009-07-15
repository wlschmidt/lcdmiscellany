#include "ScriptProcessManagement.h"
#include "../../global.h"
//#include "../../util.h"
//#include "../../stringUtil.h"
#include "../../unicode.h"

#include <DDEML.H>
#include "../../vm.h"

struct DDEData {
    HCONV hConv;
	Stack *stack;
	DWORD dataType;
};

DDEData *ddeData = 0;
int numDDEData = 0;

HDDEDATA CALLBACK DdeCallback(      
    UINT uType,
    UINT uFmt,
    HCONV hConv,
    HSZ hsz1,
    HSZ hsz2,
    HDDEDATA hddedata,
    ULONG_PTR dwData1,
    ULONG_PTR dwData2) {
		switch (uType) { 
			case XTYP_REGISTER:
			case XTYP_UNREGISTER:
			return (HDDEDATA) NULL;

			case XTYP_ADVDATA:
			return (HDDEDATA) DDE_FACK;

			case XTYP_XACT_COMPLETE:
				{
					int i=0;
					for (i=0; i<numDDEData; i++) {
						if (hConv == ddeData[i].hConv) {
							ScriptValue s;
							CreateNullValue(s);
							DWORD size;
							void* data = DdeAccessData(hddedata, &size);
							if (data) {
								if (ddeData[i].dataType == CF_TEXT) {
									CreateStringValue(s, (char*)data);
								}
								else if (ddeData[i].dataType == CF_UNICODETEXT) {
									CreateStringValue(s, (wchar_t*)data);
								}
								else {
									CreateStringValue(s, (unsigned char*)data, size);
								}
								DdeUnaccessData(hddedata);
							}
							DdeDisconnect(hConv);
							Stack *stack = ddeData[i].stack;
							stack->Push(s);
							ddeData[i] = ddeData[--numDDEData];
							RunStack(stack);
							return 0;
						}
					}
					DdeDisconnect(hConv);
				}
			//

			return 0;

			case XTYP_DISCONNECT: 

			// 

			return 0;

			default:
			return 0;
		}
		return 0;
}

DWORD ddeInst = 0;
static int DDEInit() {
	return (ddeInst || 
		DMLERR_NO_ERROR == DdeInitializeW(      
			&ddeInst,
			DdeCallback,
			APPCMD_CLIENTONLY | CBF_FAIL_EXECUTES,
			0)
	);
}

int DDEWait(ScriptValue *args, Stack *stack) {
	//if (dataType != CF_TEXT && dataType != CF_UNICODETEXT) return 0;
	int happy = 0;

	if (args->type != SCRIPT_LIST || args->listVal->numVals < 4) return 0;
	ScriptValue sv;

	CoerceIntNoRelease(args->listVal->vals[0], sv);
	DWORD type = sv.i32;

	CoerceIntNoRelease(args->listVal->vals[1], sv);
	DWORD dataType = sv.i32;
	int timeout = TIMEOUT_ASYNC;

	if (DDEInit()) {
		args->listVal->vals[2].AddRef();
		CoerceString(args->listVal->vals[2], sv);
		wchar_t *service = UTF8toUTF16Alloc(sv.stringVal->value);
		sv.Release();

		args->listVal->vals[3].AddRef();
		CoerceString(args->listVal->vals[3], sv);
		wchar_t *topic = UTF8toUTF16Alloc(sv.stringVal->value);
		sv.Release();

		wchar_t *item = 0;
		BYTE *data = 0;
		if (args->listVal->numVals >= 5) {
			args->listVal->vals[4].AddRef();
			CoerceString(args->listVal->vals[4], sv);
			item = UTF8toUTF16Alloc(sv.stringVal->value);
			sv.Release();
		}

		if (service && topic) {
			HSZ hService; 
			HSZ hTopic; 
			HSZ hItem;
			hService = DdeCreateStringHandle( 
				ddeInst,
				service,
				CP_WINUNICODE);
			 
			hTopic = DdeCreateStringHandle( 
				ddeInst,
				topic,
				CP_WINUNICODE);
			if (item && wcslen(item)) {
				hItem = DdeCreateStringHandle( 
					ddeInst,
					item,
					CP_WINUNICODE);
			}
			else {
				hItem = DdeCreateStringHandle( 
					ddeInst,
					L" ",
					CP_WINUNICODE);
			}
			if (hService && hTopic && hItem) {
				HCONV hConv = 0;
				hConv = DdeConnect(
					ddeInst,
					hService,
					hTopic,
					(PCONVCONTEXT) NULL);
				if (hConv) {
					BYTE *data = 0;
					DWORD dataLen = 0; 
					DWORD result;
					/*if (type == XTYP_EXECUTE || type == XTYP_POKE) {
						timeout = timeout;
						//timeout = 1000;
						//data = (BYTE*) " ";
						//dataLen = 1;
					}
					/*if (args->listVal->numVals >= 5 && (type == XTYP_EXECUTE || type == XTYP_POKE)) {
						args->listVal->vals[3].AddRef();
						CoerceString(args->listVal->vals[3], sv);
						data = (BYTE*) sv.stringVal->value;
						dataLen = sv.stringVal->len;
						if (type == XTYP_POKE) {
							void *data2
						}
					}//*/
					HDDEDATA ddedata = DdeClientTransaction(0, 0, hConv, hItem, dataType, type, timeout, &result);
					//if (data) sv.Release();
					if (ddedata) {
						if (timeout != TIMEOUT_ASYNC) {
							DdeFreeDataHandle(ddedata);
							happy = 1;
						}
						else if (srealloc(ddeData, sizeof(DDEData) * (numDDEData + 1))) {
							ddeData[numDDEData].dataType = dataType;
							ddeData[numDDEData].hConv = hConv;
							ddeData[numDDEData].stack = stack;
							numDDEData++;
							happy = 1;
						}
						/*
						DWORD size;
						void* data = DdeAccessData(ddedata, &size);
						if (data) {
							if (dataType == CF_TEXT) {
								happy = 1;
								CreateStringValue(s, (char*)data);
							}
							else if (dataType == CF_UNICODETEXT) {
								happy = 1;
								CreateStringValue(s, (wchar_t*)data);
							}
							DdeUnaccessData(ddedata);
						}
						DdeFreeDataHandle(ddedata);
						//*/
					}
					if (!happy) DdeDisconnect(hConv);
				}
			}
			else {
				if (hService) DdeFreeStringHandle(ddeInst, hService);
				if (hTopic) DdeFreeStringHandle(ddeInst, hTopic);
				if (hItem) DdeFreeStringHandle(ddeInst, hItem);
			}
		}//*/
		free(service);
		free(item);
		free(topic);
	}
	if (!happy) {
		return 0;
	}
	return 2;
}

void DDECleanup() {
	for (int i=0; i<numDDEData; i++) {
		DdeDisconnect(ddeData[i].hConv);
	}
	free(ddeData);
	ddeData = 0;
	numDDEData = 0;
	if (ddeInst) DdeUninitialize(ddeInst);
	ddeInst = 0;
}

