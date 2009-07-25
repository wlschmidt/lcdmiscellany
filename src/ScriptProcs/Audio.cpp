
#include "Audio.h"
#include "../global.h"
#include "../malloc.h"
#include "../vm.h"
#include <Mmsystem.h>
#include "../util.h"

#include <mmdeviceapi.h>
#include <endpointvolume.h>

struct MixerLineControls : public MIXERLINE {
	MIXERCONTROL *controls;
};

struct MixerSrc : public MixerLineControls {
};

struct MixerDest : public MixerLineControls {
	MixerSrc *srcs;
};

struct MixerInfo : public MIXERCAPS {
	HMIXER hmixer;
	MixerDest *dests;
};
MixerInfo *mixers = 0;
unsigned int numMixers = 0;

int GetSources(unsigned int i, unsigned j);

int GetDests(unsigned int i) {
	if (i >= numMixers) return 0;
	if (mixers[i].dests) return 1;
	if (!mixers[i].hmixer) return 0;
	if (!(mixers[i].dests = (MixerDest*) calloc(mixers[i].cDestinations, sizeof(MixerDest))))
		return 0;
	for (unsigned int j=0; j<mixers[i].cDestinations; j++) {
		memset(&mixers[i].dests[j], 0, sizeof(mixers[i].dests[j]));
		mixers[i].dests[j].cbStruct = sizeof(MIXERLINE);
		mixers[i].dests[j].dwDestination = j;
		if (mixers[i].hmixer) {
			if (MMSYSERR_NOERROR == mixerGetLineInfo((HMIXEROBJ)mixers[i].hmixer, &mixers[i].dests[j], MIXER_GETLINEINFOF_DESTINATION) &&
				(mixers[i].dests[j].srcs = (MixerSrc*) calloc(1, mixers[i].dests[j].cConnections * sizeof(MixerSrc) + mixers[i].dests[j].cControls * sizeof(MIXERCONTROL)))) {
					mixers[i].dests[j].controls = (MIXERCONTROL*) &mixers[i].dests[j].srcs[mixers[i].dests[j].cConnections];
					MIXERLINECONTROLS c;
					c.cControls = mixers[i].dests[j].cControls;
					c.cbStruct = sizeof(c);
					c.dwLineID = mixers[i].dests[j].dwLineID;
					c.pamxctrl = (MIXERCONTROL*)mixers[i].dests[j].controls;
					c.cbmxctrl = sizeof(MIXERCONTROL);
					if (MMSYSERR_NOERROR != mixerGetLineControls((HMIXEROBJ)mixers[i].hmixer, &c, MIXER_GETLINECONTROLSF_ALL)) {
						memset(mixers[i].dests[j].controls, 0, mixers[i].dests[j].cControls * sizeof(MIXERCONTROL));
					}
					GetSources(i, j);
			}
		}
	}
	return 1;
}

int GetSources(unsigned int i, unsigned j) {
	if (i >= numMixers) return 0;
	if (!mixers[i].dests && !GetDests(i)) return 0;
	if (!mixers[i].dests[j].srcs) return 0;
	for (unsigned int k=0; k<mixers[i].dests[j].cConnections; k++) {
		memset(&mixers[i].dests[j].srcs[k], 0, sizeof(mixers[i].dests[j].srcs[k]));
		mixers[i].dests[j].srcs[k].cbStruct = sizeof(MIXERLINE);
		mixers[i].dests[j].srcs[k].dwDestination = j;
		mixers[i].dests[j].srcs[k].dwSource = k;
		if (mixers[i].hmixer) {
			if (MMSYSERR_NOERROR == mixerGetLineInfo((HMIXEROBJ)mixers[i].hmixer, &mixers[i].dests[j].srcs[k], MIXER_GETLINEINFOF_SOURCE) &&
				(mixers[i].dests[j].srcs[k].controls = (MIXERCONTROL*) calloc(mixers[i].dests[j].srcs[k].cControls, sizeof(MIXERCONTROL)))) {
					MIXERLINECONTROLS c;
					c.cControls = mixers[i].dests[j].srcs[k].cControls;
					c.cbStruct = sizeof(c);
					c.dwLineID = mixers[i].dests[j].srcs[k].dwLineID;
					c.pamxctrl = (MIXERCONTROL*)mixers[i].dests[j].srcs[k].controls;
					c.cbmxctrl = sizeof(MIXERCONTROL);
					if (MMSYSERR_NOERROR != mixerGetLineControls((HMIXEROBJ)mixers[i].hmixer, &c, MIXER_GETLINECONTROLSF_ALL)) {
						memset(mixers[i].dests[j].srcs[k].controls, 0, mixers[i].dests[j].srcs[k].cControls * sizeof(MIXERCONTROL));
					}
			}
		}
	}
	return 1;
}

void InitAudio() {
	UninitAudio();
	numMixers = mixerGetNumDevs();
	mixers = (MixerInfo*) calloc(numMixers, sizeof(MixerInfo));
	if (!mixers) numMixers = 0;
	for (unsigned int i=0; i<numMixers; i++) {
		if (MMSYSERR_NOERROR == mixerGetDevCaps(i, &mixers[i], sizeof(MIXERCAPS)) &&
			MMSYSERR_NOERROR == mixerOpen(&mixers[i].hmixer, i, (DWORD_PTR)ghWnd, 0, CALLBACK_WINDOW)) {
		}
		else {
			mixers[i].hmixer = 0;
		}
	}
}

void UninitAudio() {
	for (unsigned int i=0; i<numMixers; i++) {
		if (mixers[i].hmixer) {
			mixerClose(mixers[i].hmixer);
		}
		if (mixers[i].dests) {
			for (unsigned int j=0; j<mixers[i].cDestinations; j++) {
				if (mixers[i].dests[j].srcs) {
					for (unsigned int k=0; k<mixers[i].dests[j].cConnections; k++) {
						free(mixers[i].dests[j].srcs[k].controls);
					}
					free(mixers[i].dests[j].srcs);
				}
			}
			free(mixers[i].dests);
		}
	}
	free(mixers);
	mixers = 0;
	numMixers = 0;
}

void GetNumMixers(ScriptValue &s, ScriptValue *args) {
	//int t = mixerGetNumDevs();
	//if (t != numMixers) InitAudio();
	CreateIntValue(s, numMixers);
}

int GetAudioControl(unsigned int &dev, int &dest, int &src, int &control, MixerLineControls *&controls) {
	if (dev >= numMixers) InitAudio();
	if (dev < 0 || dev >= numMixers || (!mixers[dev].dests && !GetDests(dev))) return 0;
	if (dest < 0) {
		for (unsigned int i=0; i<mixers[dev].cDestinations; i++) {
			if (mixers[dev].dests[i].dwComponentType == -dest) {
				dest = i;
				break;
			}
		}
		if (dest < 0) return 0;
	}
	if (dev >= numMixers || dest >= (int)mixers[dev].cDestinations) return 0;
	if (src == 0) {
		controls = &mixers[dev].dests[dest];
	}
	else {
		if (src < 0) {
			for (unsigned int i=0; i<mixers[dev].dests[dest].cConnections; i++) {
				if (mixers[dev].dests[dest].srcs[i].dwComponentType == -src) {
					src = i;
					break;
				}
			}
			if (src < 0) return 0;
		}
		else src--;
		if (src >= (int) mixers[dev].dests[dest].cConnections) return 0;
		controls = &mixers[dev].dests[dest].srcs[src];
	}
	if (control < 0) {
		int w = MIXERCONTROL_CONTROLTYPE_VOLUME;
		for (unsigned int i=0; i<controls->cControls; i++) {
			if (controls->controls[i].dwControlType == -control) {
				control = i;
				break;
			}
		}
	}
	if (control < 0 || control >= (int)controls->cControls) return 0;
	return 1;
}

void SetAudioValue(ScriptValue &s, ScriptValue *args) {
	unsigned int dev = args[0].i32;
	int dest = args[1].i32;
	int src = args[2].i32;
	int control = args[3].i32;
	MixerLineControls *controls=0;
	if (!GetAudioControl(dev, dest, src, control, controls)) return;

	MIXERCONTROL *c = controls->controls + control;
	MIXERCONTROLDETAILS mcd;
	memset(&mcd, 0, sizeof(mcd));
	mcd.cbStruct = sizeof(mcd);
	mcd.hwndOwner = 0;
	mcd.dwControlID = c->dwControlID;
	mcd.cChannels = controls->cChannels;
	//if (args[4].i32)
	mcd.cChannels = 1;
	mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

	MIXERCONTROLDETAILS_UNSIGNED *mcdu = (MIXERCONTROLDETAILS_UNSIGNED*) alloca(sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mcd.cChannels);
	memset(mcdu, 0, sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mcd.cChannels);
	mcd.paDetails = mcdu;

	mcdu[0].dwValue = (unsigned int) args[4].i32;

	if (MMSYSERR_NOERROR != mixerSetControlDetails((HMIXEROBJ)mixers[dev].hmixer, &mcd, MIXER_GETCONTROLDETAILSF_VALUE)) return;
	CreateIntValue(s, 1);
}

void GetAudioValue(ScriptValue &s, ScriptValue *args) {
	//int w = MIXERCONTROL_CONTROLTYPE_MUTE;
	unsigned int dev = args[0].i32;
	int dest = args[1].i32;
	int src = args[2].i32;
	int control = args[3].i32;
	MixerLineControls *controls=0;
	if (!GetAudioControl(dev, dest, src, control, controls)) return;

	MIXERCONTROL *c = controls->controls + control;
	MIXERCONTROLDETAILS mcd;
	memset(&mcd, 0, sizeof(mcd));
	mcd.cbStruct = sizeof(mcd);
	mcd.hwndOwner = 0;
	mcd.dwControlID = c->dwControlID;
	mcd.cChannels = controls->cChannels;
	if (args[4].i32) mcd.cChannels = 1;
	mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

	MIXERCONTROLDETAILS_UNSIGNED *mcdu = (MIXERCONTROLDETAILS_UNSIGNED*) alloca(sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mcd.cChannels);
	memset(mcdu, 0, sizeof(MIXERCONTROLDETAILS_UNSIGNED) * mcd.cChannels);
	mcd.paDetails = mcdu;

	if (MMSYSERR_NOERROR != mixerGetControlDetails((HMIXEROBJ)mixers[dev].hmixer, &mcd, MIXER_GETCONTROLDETAILSF_VALUE)) return;
	if (args[4].i32) {
		if (MIXERCONTROL_CT_UNITS_SIGNED == (c->dwControlType & MIXERCONTROL_CT_UNITS_MASK)) {
			CreateIntValue(s, (int)mcdu[0].dwValue);
		}
		else {
			CreateIntValue(s, mcdu[0].dwValue);
		}
	}
	else if (CreateListValue(s, mcd.cChannels)) {
		for (unsigned int i=0; i<mcd.cChannels; i++) {
			if (MIXERCONTROL_CT_UNITS_SIGNED == (c->dwControlType & MIXERCONTROL_CT_UNITS_MASK)) {
				s.listVal->PushBack((int)mcdu[i].dwValue);
			}
			else {
				s.listVal->PushBack(mcdu[i].dwValue);
			}
		}
	}
}

wchar_t *GetAudioTypeAndString(ScriptValue *args, DWORD &type) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals == 0) return 0;
	ScriptValue val;
	CoerceIntNoRelease(args->listVal->vals[0], val);

	if (val.intVal >= numMixers) InitAudio();
	if (val.intVal < 0 || val.intVal >= numMixers || (!mixers[val.i32].dests && !GetDests(val.i32))) return 0;

	MixerInfo *mixer = &mixers[val.i32];
	if (args->listVal->numVals == 1) {
		return mixer->szPname;
	}

	CoerceIntNoRelease(args->listVal->vals[1], val);
	if (val.intVal < 0 || val.intVal >= mixer->cDestinations) return 0;

	MixerDest *dest = &mixer->dests[val.i32];
	if (args->listVal->numVals == 2) {
		type = dest->dwComponentType;
		return dest->szName;
	}

	CoerceIntNoRelease(args->listVal->vals[2], val);
	if (val.intVal == 0) {
		if (args->listVal->numVals != 4) return 0;
		CoerceIntNoRelease(args->listVal->vals[3], val);
		if (val.intVal<0 || val.intVal >= dest->cChannels) return 0;
		type = dest->controls[val.i32].dwControlType;
		return dest->controls[val.i32].szName;
	}
	val.intVal--;
	if (val.intVal < 0 || val.intVal >= dest->cConnections) return 0;

	MixerSrc *src = &dest->srcs[val.i32];
	if (args->listVal->numVals == 3) {
		type = src->dwComponentType;
		return src->szName;
	}
	if (args->listVal->numVals != 4) return 0;
	CoerceIntNoRelease(args->listVal->vals[3], val);
	if (val.intVal < 0 || val.intVal >= src->cControls) return 0;
	type = src->controls[val.intVal].dwControlType;
	return src->controls[val.intVal].szName;
}

void GetAudioType(ScriptValue &s, ScriptValue *args) {
	if (args->type != SCRIPT_LIST || args->listVal->numVals <= 1) return;
	DWORD type;
	if (GetAudioTypeAndString(args, type)) {
		CreateIntValue(s, type);
	}
}

void GetAudioString(ScriptValue &s, ScriptValue *args) {
	DWORD type;
	wchar_t *name = GetAudioTypeAndString(args, type);
	if (name) {
		CreateStringValue(s, name);
	}
}

class CMMNotificationClient : public IMMNotificationClient, public IAudioEndpointVolumeCallback {
    LONG refs;
public:
	CMMNotificationClient() : refs(1) {
	}
	~CMMNotificationClient() {}

	// IUnknown methods -- AddRef, Release, and QueryInterface

	ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&refs);
	}

	ULONG STDMETHODCALLTYPE Release() {
		LONG refs = InterlockedDecrement(&this->refs);
		if (!refs) {
			delete this;
		}
		return refs;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) {
		if (IID_IUnknown == riid) {
			AddRef();
			*ppvInterface = (IMMNotificationClient*)this;
		}
		else if (__uuidof(IMMNotificationClient) == riid) {
			AddRef();
			*ppvInterface = (IMMNotificationClient*)this;
		}
		else if (__uuidof(IAudioEndpointVolumeCallback) == riid) {
			AddRef();
			*ppvInterface = (IAudioEndpointVolumeCallback*)this;
		}
		else {
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) {
		PostMessage(ghWnd, WMU_VISTA_VOLUME_CHANGE, 0, 0);
		return S_OK;
	}


	// Callback methods for device-event notifications.

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
								EDataFlow flow, ERole role,
								LPCWSTR pwstrDeviceId) {
		PostMessage(ghWnd, WMU_VISTA_AUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) {
		PostMessage(ghWnd, WMU_VISTA_AUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	};

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
		PostMessage(ghWnd, WMU_VISTA_AUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
								LPCWSTR pwstrDeviceId,
								DWORD dwNewState) {
		PostMessage(ghWnd, WMU_VISTA_AUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
								LPCWSTR pwstrDeviceId,
								const PROPERTYKEY key) {
		PostMessage(ghWnd, WMU_VISTA_AUDIO_DEV_CHANGE, 0, 0);
		return S_OK;
	}
};

CMMNotificationClient *client = 0;

IMMDeviceEnumerator *deviceEnumerator = 0;
IMMDevice *defaultDevice = 0;
IAudioEndpointVolume *endpointVolume = 0;

void SetupVistaVolume() {
	HRESULT hr;
	static char com = 0;
	if (!com) {
		if (!InitCom()) return;
		com = 1;
	}
	if (!client) {
		client = new CMMNotificationClient();
	}
	if (!deviceEnumerator) {
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
		if (hr != S_OK) {
			deviceEnumerator = 0;
			delete client;
			client = 0;
			UninitCom();
			com = 0;
			return;
		}
		hr = deviceEnumerator->RegisterEndpointNotificationCallback(client);
		if (hr != S_OK) {
			deviceEnumerator->Release();
			deviceEnumerator = 0;
			return;
		}
	}
	if (defaultDevice) {
		defaultDevice->Release();
		defaultDevice = 0;
	}
	if (endpointVolume) {
		endpointVolume->UnregisterControlChangeNotify(client);
		endpointVolume->Release();
		endpointVolume = 0;
	}
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	if (hr != S_OK) {
		defaultDevice = 0;
		return;
	}

	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	if (hr != S_OK) {
		endpointVolume = 0;
		return;
	}
	hr = endpointVolume->RegisterControlChangeNotify(client);
	if (hr != S_OK) {
		endpointVolume->Release();
		endpointVolume = 0;
		return;
	}

}

void CleanupVistaVolume() {
	if (endpointVolume) {
		endpointVolume->UnregisterControlChangeNotify(client);
		endpointVolume->Release();
		endpointVolume = 0;
	}
	if (defaultDevice) {
		defaultDevice->Release();
		defaultDevice = 0;
	}
	if (deviceEnumerator) {
		deviceEnumerator->UnregisterEndpointNotificationCallback(client);
		deviceEnumerator->Release();
		deviceEnumerator = 0;
	}
	if (client) {
		client->Release();
		client = 0;
	}
}

void GetVistaMasterVolume(ScriptValue &s, ScriptValue *args) {
	if (!endpointVolume) return;
	BOOL mute = 0;
	float vol = 0;
	if (S_OK == endpointVolume->GetMasterVolumeLevelScalar(&vol) &&
		S_OK == endpointVolume->GetMute(&mute) &&
		CreateListValue(s, 2)) {
			ScriptValue sv;
			CreateDoubleValue(sv, vol);
			s.listVal->PushBack(sv);
			s.listVal->PushBack(mute);
	}
	/*
	static char com = 0;
	if (!com) {
		if (!InitCom()) return;
		com = 1;
	}
	IMMDeviceEnumerator *deviceEnumerator = 0;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	if (hr != S_OK) return;
	IMMDevice *defaultDevice = 0;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	if (hr != S_OK) return;

	IAudioEndpointVolume *endpointVolume = 0;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	if (hr != S_OK) return;

	// -------------------------
	float vol = 0;
	BOOL mute = 0;
	int happy = 1;
	if (S_OK != endpointVolume->GetMasterVolumeLevelScalar(&vol)) {
		happy = 0;
	}
	if (S_OK != endpointVolume->GetMute(&mute)) {
		happy = 0;
	}
	if (happy && CreateListValue(s, 2)) {
		ScriptValue sv;
		CreateDoubleValue(sv, vol);
		s.listVal->PushBack(sv);
		s.listVal->PushBack(mute);
	}
	endpointVolume->Release();
	//*/
}
