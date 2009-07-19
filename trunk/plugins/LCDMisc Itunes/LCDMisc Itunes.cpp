
#include "DllTypes.h"
#include "iTunesCOMInterface.h"


// LCDMisc Itunes.cpp : Defines the entry point for the DLL application.
//

IiTunes *iITunes=0;
int comInit = 0;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void* lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
	}
	if (fdwReason == DLL_PROCESS_DETACH) {
		if (iITunes) {
			iITunes->Release();
		}
		if (comInit) {
			CoUninitialize();
			comInit = 0;
		}
	}
    return 1;
}

void WINAPI Init(AppInfo *in, DllInfo *out) {
	if (in->size < sizeof(AppInfo) || in->maxDllVersion < 1 || out->size < sizeof(DllInfo)) return;
	out->free = free;
	out->dllVersion = 1;
}

void WINAPI ItunesNextTrack(ScriptVal *in, ScriptVal *out) {
	if (iITunes) {
		iITunes->NextTrack();
	}
}

void WINAPI ItunesPrevTrack(ScriptVal *in, ScriptVal *out) {
	if (iITunes) {
		iITunes->PreviousTrack();
	}
}

void WINAPI ItunesPlay(ScriptVal *in, ScriptVal *out) {
	if (!iITunes) {
		if (!comInit) {
			int res = CoInitialize(0);
			if (res == S_OK || res == S_FALSE)
				comInit = 1;
		}
		int res = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (void **)&iITunes);
	}
	if (iITunes) {
		iITunes->Play();
	}
}

void WINAPI ItunesPause(ScriptVal *in, ScriptVal *out) {
	if (iITunes) {
		iITunes->Pause();
	}
}

void WINAPI ItunesPlayPause(ScriptVal *in, ScriptVal *out) {
	if (!iITunes) {
		if (!comInit) {
			int res = CoInitialize(0);
			if (res == S_OK || res == S_FALSE)
				comInit = 1;
		}
		int res = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (void **)&iITunes);
	}
	if (iITunes) {
		iITunes->PlayPause();
	}
}

void WINAPI ItunesMute(ScriptVal *in, ScriptVal *out) {
	if (iITunes) {
		VARIANT_BOOL mute = VARIANT_TRUE;
		iITunes->get_Mute(&mute);
		iITunes->put_Mute(mute^VARIANT_TRUE^VARIANT_FALSE);
	}
}

void WINAPI ItunesStop(ScriptVal *in, ScriptVal *out) {
	if (iITunes) {
		iITunes->Stop();
	}
}

void WINAPI ItunesChangeVolume(ScriptVal *in, ScriptVal *out) {
	int dv = 0;
	if (iITunes) {
		if (in->type == SCRIPT_LIST && in->listValue.numVals >= 1 &&
			in->listValue.vals[0].type == SCRIPT_INT) {
				dv = (int) in->listValue.vals[0].intValue;
			}
		long volume = 0;
		if (S_OK == iITunes->get_SoundVolume(&volume)) {
			if (dv+(int)volume < 0) {
				volume = 0;
			}
			else {
				volume+=dv;
				if (volume > 100) volume = 100;
			}
			iITunes->put_SoundVolume(volume);
		}
	}
}

void WINAPI ItunesChangeMode(ScriptVal *in, ScriptVal *out) {
	IITPlaylist *playlist = 0;
	if (iITunes && S_OK == iITunes->get_CurrentPlaylist(&playlist)) {
		int dir = 1;
		if (in->type == SCRIPT_LIST && in->listValue.numVals >= 1 &&
			in->listValue.vals[0].type == SCRIPT_INT && in->listValue.vals[0].intValue<0) {
				dir = 0;
			}
		VARIANT_BOOL shuffle;
		ITPlaylistRepeatMode repeat;
		if (S_OK == playlist->get_Shuffle(&shuffle) && shuffle == VARIANT_TRUE) {
			playlist->put_Shuffle(0);
			if (dir) {
				playlist->put_SongRepeat(ITPlaylistRepeatModeOne);
			}
			else {
				playlist->put_SongRepeat(ITPlaylistRepeatModeOff);
			}
			return;
		}
		else if (S_OK == playlist->get_SongRepeat(&repeat)) {
			if (repeat == ITPlaylistRepeatModeOne) {
				playlist->put_Shuffle(!dir);
				if (dir) {
					playlist->put_SongRepeat(ITPlaylistRepeatModeAll);
				}
				else {
					playlist->put_SongRepeat(ITPlaylistRepeatModeOff);
				}
				return;
			}
			else if (repeat == ITPlaylistRepeatModeAll) {
				if (dir) playlist->put_SongRepeat(ITPlaylistRepeatModeOff);
				else playlist->put_SongRepeat(ITPlaylistRepeatModeOne);
				return;
			}
		}

		if (dir) {
			playlist->put_Shuffle(VARIANT_TRUE);
		}
		else playlist->put_SongRepeat(ITPlaylistRepeatModeAll);
	}
}

void WINAPI GetItunesInfo(ScriptVal *in, ScriptVal *out) {
	MakeListValue(out);
	ScriptList *list = &out->listValue;

	if (!iITunes) {
		int res = !S_OK;
		if (FindWindowA(0, "iTunes")) {
			if (!comInit) {
				int res = CoInitialize(0);
				if (res == S_OK || res == S_FALSE)
					comInit = 1;
			}
			int res = CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (void **)&iITunes);
		}
		if (res != S_OK) {
			list->PushBack(-1);
			return;
		}
	}
	ITPlayerState playerState;
	if (S_OK != iITunes->get_PlayerState(&playerState)) {
		// itunes shut down.
		iITunes->Release();
		iITunes = 0;
		if (comInit) {
			CoUninitialize();
			comInit = 0;
		}
		list->PushBack(-1);
		return;
	}
	if (playerState == ITPlayerStateStopped) {
		list->PushBack((__int64)0);
	}
	else {
		list->PushBack(2);
	}


	long volume = 0;
	iITunes->get_SoundVolume(&volume);
	list->PushBack(volume);

	VARIANT_BOOL mute;
	iITunes->get_Mute(&mute);
	list->PushBack(mute);



	IITTrack *iTrack = 0;
	ScriptVal sv;

	if (S_OK != iITunes->get_CurrentTrack(&iTrack)) return;

	BSTR str;
	MakeNullValue(&sv);
	if (S_OK == iTrack->get_Name(&str) && str) {
		MakeStringValue(&sv, str);
		SysFreeString(str);
	}
	list->PushBack(&sv);

	MakeNullValue(&sv);
	if (S_OK == iTrack->get_Artist(&str) && str) {
		MakeStringValue(&sv, str);
		SysFreeString(str);
	}
	list->PushBack(&sv);

	long trackLen = 0;
	iTrack->get_Duration(&trackLen);
	list->PushBack(trackLen);



	long playerPos = 0;
	iITunes->get_PlayerPosition(&playerPos);
	list->PushBack(playerPos);

	// Seems to be the only way to tell if it's stopped or paused.
	if (playerPos > 0 && !list->vals[0].intValue) {
		list->vals[0].intValue = 1;
	}



	long trackIndex = 1;
	long trackCount = 1;
	char *mode = "Once";
	iTrack->get_PlayOrderIndex(&trackIndex);
	iTrack->Release();


	IITPlaylist *playlist = 0;
	if (S_OK == iITunes->get_CurrentPlaylist(&playlist)) {
		IITTrackCollection *trackCollection = 0;
		if (S_OK == playlist->get_Tracks(&trackCollection)) {
			trackCollection->get_Count(&trackCount);
			trackCollection->Release();
		}
		VARIANT_BOOL shuffle;
		ITPlaylistRepeatMode repeat;
		if (S_OK == playlist->get_Shuffle(&shuffle) && shuffle) {
			mode = "Shuffle";
		}
		else if (S_OK == playlist->get_SongRepeat(&repeat)) {
			if (repeat == ITPlaylistRepeatModeOne) {
				mode = "Repeat 1";
			}
			else if (repeat == ITPlaylistRepeatModeAll) {
				mode = "Repeat";
			}
		}
		playlist->Release();
	}
	if (trackCount <= 1) {
		trackCount = 0;
		trackIndex = 0;
	}


	list->PushBack(trackIndex);
	list->PushBack(trackCount);
	MakeStringValue(&sv, mode);
	list->PushBack(&sv);
}
