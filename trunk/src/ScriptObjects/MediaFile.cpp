#define _USE_MATH_DEFINES
#include "File.h"
#include "MediaFile.h"
#include "../global.h"
#include <mmreg.h>
#include <dshow.h>
#include <initguid.h>
#include <qnetwork.h>
#include "../malloc.h"
#include "../Unicode.h"
#include "../util.h"
#include "Qedit.h"
#include "ScriptObjects.h"
#include "../SymbolTable.h"
#include "../ScriptProcs/Event/Event.h"
#include "math.h"

static void FreeMediaType(AM_MEDIA_TYPE * mt) {
    if (mt->cbFormat != 0) {
        CoTaskMemFree((PVOID)mt->pbFormat);

        // Strictly unnecessary but tidier
        mt->cbFormat = 0;
        mt->pbFormat = NULL;
    }
    if (mt->pUnk != NULL) {
        mt->pUnk->Release();
        mt->pUnk = NULL;
    }
}



enum MediaStatus {MEDIA_STOPPED, MEDIA_PLAYING, MEDIA_PAUSED};

struct MediaFileData {
    IGraphBuilder *pGraph;
    IMediaControl *pControl;
    IMediaEventEx *pEvent;
	IBasicAudio *pAudio;
	IMediaSeeking *pSeeking;
	ISampleGrabber *pGrabber;
	StringValue *title;
	StringValue *author;
	StringValue *fileName;
	StringValue *copyright;
	StringValue *desc;
	double duration;
	int volume, balance;
	MediaStatus status:16;
	int com:15;
	unsigned int repeat:1;
};

unsigned int MediaFileType;

static inline MediaFileData* GetMediaFileData(ObjectValue *o) {
	if (o->values[0].type != SCRIPT_STRING) return 0;
	return (MediaFileData*)o->values[0].stringVal->value;
}

static inline MediaFileData* GetMediaFileData(ScriptValue &s) {
	if (s.type != SCRIPT_OBJECT || s.objectVal->type != MediaFileType || s.objectVal->values[0].type != SCRIPT_STRING) return 0;
	return (MediaFileData*)s.objectVal->values[0].stringVal->value;
}

static ObjectValue **MediaFiles = 0;
static int numMediaFiles = 0;

static inline void SetVolume(MediaFileData *mf) {
	if (mf->pGraph) {
		if ((!mf->pAudio && FAILED(mf->pGraph->QueryInterface(IID_IBasicAudio, (void **)&mf->pAudio))) || FAILED(mf->pAudio->put_Volume(100*mf->volume-10000))) {
			return;
		}
	}
}

inline void SetBalance(MediaFileData *mf) {
	if (mf->pGraph) {
		if ((!mf->pAudio && FAILED(mf->pGraph->QueryInterface(IID_IBasicAudio, (void **)&mf->pAudio))) || FAILED(mf->pAudio->put_Balance(100*mf->balance))) {
			return;
		}
	}
}

void MediaFileSetVolume(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf) {
		CreateIntValue(s, mf->volume);
		if (args[0].intVal >= 0 && args[0].intVal <= 100) {
			mf->volume = args[0].i32;
			SetVolume(mf);
		}
	}
}

void MediaFileSetBalance(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf) {
		CreateIntValue(s, mf->balance);
		if (args[0].intVal >= -100 && args[0].intVal <= 100) {
			mf->balance = args[0].i32;
			SetBalance(mf);
		}
	}
}

void MediaFileSetPosition(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf && mf->pGraph) {
		if ((mf->pSeeking || !FAILED(mf->pGraph->QueryInterface(IID_IMediaSeeking, (void **)&mf->pSeeking)))) {
			__int64 pos = (__int64)(10000000.0 * args[0].doubleVal);
			if (!FAILED(mf->pSeeking->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning))) {
				CreateIntValue(s, 1);
			}
		}
	}
}

void MediaFileGetPosition(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf && mf->pGraph) {
		if ((mf->pSeeking || !FAILED(mf->pGraph->QueryInterface(IID_IMediaSeeking, (void **)&mf->pSeeking)))) {
			__int64 pos;
			if (!FAILED(mf->pSeeking->GetCurrentPosition((LONGLONG*)&pos))) {
				CreateDoubleValue(s, pos/10000000.0);
			}
		}
	}
}

void MediaFileGetDuration(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf) {
		if (mf->pGraph && (mf->pSeeking || !FAILED(mf->pGraph->QueryInterface(IID_IMediaSeeking, (void **)&mf->pSeeking)))) {
			__int64 duration;
			if (!FAILED(mf->pSeeking->GetDuration((LONGLONG*)&duration))) {
				CreateDoubleValue(s, mf->duration = duration/10000000.0);
			}
		}
		else if (mf->duration > 0) {
			CreateDoubleValue(s, mf->duration);
		}
	}
}

static int bstrToStr(BSTR &bstr, StringValue* &s) {
	int q = ((int*) bstr)[-1] / 2;
	ScriptValue sv;
	int res = CreateStringValue(sv, (wchar_t*)bstr, ((int*) bstr)[-1] / 2);
	SysFreeString(bstr);
	if (res) s = sv.stringVal;
	return res;
}

static void GetMediaInfo(MediaFileData *mf) {
	IEnumFilters* ppEnum;
	if (!mf->author || !mf->title || !mf->desc || !mf->copyright) {
		if (SUCCEEDED(mf->pGraph->EnumFilters(&ppEnum))) {
			IBaseFilter* pFilter;
			while( ppEnum->Next( 1, &pFilter, NULL ) == S_OK ) {
				IAMMediaContent* pMediaContent;
				if( SUCCEEDED( pFilter->QueryInterface( IID_IAMMediaContent, (void **)&pMediaContent ) ) ) {
					pMediaContent->Release();
					BSTR bstr;
					if ((!mf->title && SUCCEEDED(pMediaContent->get_Title(&bstr)) && bstrToStr(bstr, mf->title)) |
						(!mf->author && SUCCEEDED(pMediaContent->get_AuthorName(&bstr)) && bstrToStr(bstr, mf->author)) |
						(!mf->copyright && SUCCEEDED(pMediaContent->get_Copyright(&bstr)) && bstrToStr(bstr, mf->copyright)) |
						(!mf->desc && SUCCEEDED(pMediaContent->get_Description(&bstr)) && bstrToStr(bstr, mf->desc))) {
					}
				}
				pFilter->Release();
			}
			ppEnum->Release();
		}
	}
}


static int InitMediaObjects(MediaFileData *mf, ScriptValue &s) {
	if (mf && mf->status != MEDIA_PLAYING) {
		if (mf->com || InitCom()) {
			mf->com = 1;
			int needNew = !mf->pGraph;
			int r;
			if (!mf->pGraph && FAILED(r=CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&mf->pGraph))) {
				return 0;
			}
			if (!mf->pGrabber) {
				IBaseFilter *temp;
				if (!FAILED(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&temp))) {
					if (FAILED(mf->pGraph->AddFilter(temp, L"Sample Grabber"))) {
					}
					else if (FAILED(temp->QueryInterface(IID_ISampleGrabber, (void**)&mf->pGrabber))) {
						mf->pGraph->RemoveFilter(temp);
					}
					else {
						AM_MEDIA_TYPE mt;
						memset(&mt, 0, sizeof(AM_MEDIA_TYPE));
						mt.majortype = MEDIATYPE_Audio;
						mt.subtype = MEDIASUBTYPE_PCM;
						mt.formattype = FORMAT_WaveFormatEx;
						if (FAILED(mf->pGrabber->SetMediaType(&mt)) ||
							FAILED(mf->pGrabber->SetBufferSamples(1))) {
							mf->pGraph->RemoveFilter(temp);
							mf->pGrabber->Release();
							mf->pGrabber = 0;
						}
						//FreeMediaType(mt);
					}
					temp->Release();
				}
			}
			if ((mf->pControl || SUCCEEDED(mf->pGraph->QueryInterface(IID_IMediaControl, (void **)&mf->pControl))) &&
				(mf->pEvent   || SUCCEEDED(mf->pGraph->QueryInterface(IID_IMediaEventEx, (void **)&mf->pEvent)))) {
					mf->pEvent->SetNotifyWindow((OAHWND)ghWnd, WMU_DSHOW_EVENT, (LONG_PTR)s.objectVal);
					HRESULT hr;
					if (needNew) {
						wchar_t *name = UTF8toUTF16Alloc(mf->fileName->value);
						if (name) {
							hr = mf->pGraph->RenderFile(name, 0);
							free(name);
						}
						else {
							hr = !S_OK;
						}
					}
					else hr = S_OK;


					if (hr == S_OK || hr == VFW_S_VIDEO_NOT_RENDERED || hr == VFW_S_PARTIAL_RENDER || hr == VFW_S_DUPLICATE_NAME) {
						GetMediaInfo(mf);
						SetVolume(mf);
						SetBalance(mf);
						MediaFileGetDuration(s, 0);
						return 1;
						//Seek(mf->lastTime);

						// Non-essential;
						//AdjustVolume(0);
						//AdjustBalance(0);
					}
			}
		}
	}
	return 0;
}



void MediaFile(ScriptValue &s, ScriptValue *args) {
	ScriptValue out;
	if (CreateObjectValue(out, MediaFileType)) {
		MediaFileData* mf;
		if (AllocateStringValue(out.objectVal->values[0], sizeof(MediaFileData)) && (mf = GetMediaFileData(out))) {
			memset(mf, 0, sizeof(*mf));
			//ScriptValue sv;
			//ResolvePath(sv, args);
			//if (sv.type != SCRIPT_STRING) {
				mf->fileName = args[0].stringVal;
				args[0].stringVal->AddRef();
			//}
			//else
			//	mf->fileName = sv.stringVal;
			mf->volume = 100;
			if (srealloc(MediaFiles, sizeof(ObjectValue*) * (1+numMediaFiles))) {
				ScriptValue sv = out;
				if (args[1].intVal != 1 || InitMediaObjects(mf, sv)) {
					s = out;
					MediaFiles[numMediaFiles++] = out.objectVal;
					return;
				}
			}
		}
		out.Release();
	}
}

void MediaFilePlay(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (InitMediaObjects(mf, s)) {
		mf->status = MEDIA_PLAYING;
		if (!FAILED(mf->pControl->Run())) {
			CreateIntValue(s, 1);
			return;
		}
	}
	ScriptValue stoparg;
	CreateIntValue(stoparg, 1);
	MediaFileStop(s, &stoparg);
}

void MediaFileStop(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (mf) {
		if (!args[0].intVal) {
			// seek!
			if (mf->pControl) {
				mf->pControl->Stop();
			}
			if (mf->pSeeking) {
				LONGLONG pos2 = 0;
				mf->pSeeking->SetPositions(&pos2, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
			}
		}
		else {
			if (mf->pGrabber) {
				IBaseFilter * filter;
				if (SUCCEEDED(mf->pGrabber->QueryInterface(IID_IBaseFilter, (void**)&filter))) {
					mf->pGraph->RemoveFilter(filter);
					filter->Release();
				}
				mf->pGrabber->Release();
				mf->pGrabber = 0;
			}
			if (mf->pAudio) {
				mf->pAudio->Release();
				mf->pAudio = 0;
			}
			if (mf->pSeeking) {
				mf->pSeeking->Release();
				mf->pSeeking = 0;
			}
			if (mf->pEvent) {
				mf->pEvent->Release();
				mf->pEvent = 0;
			}
			if (mf->pControl) {
				mf->pControl->Stop();
				//mf->pControl->StopWhenReady();
				mf->pControl->Release();
				mf->pControl = 0;
			}
			if (mf->pGraph) {
				mf->pGraph->Release();
				mf->pGraph = 0;
			}
			if (args[0].intVal == 2 && mf->com) {
				mf->com = 0;
				UninitCom();
			}
			//if (mf->status != MEDIA_NONE)
		}
		mf->status = MEDIA_STOPPED;
	}
	CreateNullValue(s);
}

void MediaFileAuthor(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (!mf || !mf->title) {
		CreateNullValue(s);
		return;
	}
	CreateStringValue(s, mf->author);
	mf->author->AddRef();
}

void MediaFileTitle(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (!mf || !mf->title) {
		CreateNullValue(s);
		return;
	}
	CreateStringValue(s, mf->title);
	mf->title->AddRef();
}

void MediaFileDescription(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (!mf || !mf->desc) {
		CreateNullValue(s);
		return;
	}
	CreateStringValue(s, mf->desc);
	mf->desc->AddRef();
}

void MediaFileCopyright(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (!mf || !mf->copyright) {
		CreateNullValue(s);
		return;
	}
	CreateStringValue(s, mf->copyright);
	mf->copyright->AddRef();
}

void MediaFilePause(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	if (mf) {
		if (mf->status == MEDIA_PLAYING) {
			if (mf->pControl) {
				mf->pControl->Pause();
				mf->status = MEDIA_PAUSED;
			}
			else {
				ScriptValue sv;
				CreateIntValue(sv, 0);
				MediaFileStop(s, &sv);
			}
		}
	}
	CreateNullValue(s);
}


///////////////////////////////////////////////////////////
// return needed bits for fft
///////////////////////////////////////////////////////////

unsigned int NumberOfBitsNeeded( unsigned int p_nSamples )
{

    int i;

    if( p_nSamples < 2 )
    {
        return 0;
    }

    for ( i=0; ; i++ )
    {
        if( p_nSamples & (1 << i) ) return i;
    }

}



///////////////////////////////////////////////////////////
// ?
///////////////////////////////////////////////////////////

unsigned int ReverseBits(unsigned int p_nIndex, unsigned int p_nBits)
{

    unsigned int i, rev;

    for(i=rev=0; i < p_nBits; i++)
    {
        rev = (rev << 1) | (p_nIndex & 1);
        p_nIndex >>= 1;
    }

    return rev;
}


void fft_double (unsigned int p_nSamples, 
    double *p_lpRealIn, 
    double *p_lpRealOut, double *p_lpImagOut) {


    unsigned int NumBits;
    unsigned int i, j, k, n;
    unsigned int BlockSize, BlockEnd;

    double angle_numerator = 2.0 * M_PI;
    double tr, ti;

    NumBits = NumberOfBitsNeeded ( p_nSamples );


    for( i=0; i < p_nSamples; i++ )
    {
        j = ReverseBits ( i, NumBits );
        p_lpRealOut[j] = p_lpRealIn[i];
        p_lpImagOut[j] = 0.0;
    }


    BlockEnd = 1;
    for( BlockSize = 2; BlockSize <= p_nSamples; BlockSize <<= 1 )
    {
        double delta_angle = angle_numerator / (double)BlockSize;
        double sm2 = sin ( -2 * delta_angle );
        double sm1 = sin ( -delta_angle );
        double cm2 = cos ( -2 * delta_angle );
        double cm1 = cos ( -delta_angle );
        double w = 2 * cm1;
        double ar[3], ai[3];

        for( i=0; i < p_nSamples; i += BlockSize )
        {

            ar[2] = cm2;
            ar[1] = cm1;

            ai[2] = sm2;
            ai[1] = sm1;

            for ( j=i, n=0; n < BlockEnd; j++, n++ )
            {

                ar[0] = w*ar[1] - ar[2];
                ar[2] = ar[1];
                ar[1] = ar[0];

                ai[0] = w*ai[1] - ai[2];
                ai[2] = ai[1];
                ai[1] = ai[0];

                k = j + BlockEnd;
                tr = ar[0]*p_lpRealOut[k] - ai[0]*p_lpImagOut[k];
                ti = ar[0]*p_lpImagOut[k] + ai[0]*p_lpRealOut[k];

                p_lpRealOut[k] = p_lpRealOut[j] - tr;
                p_lpImagOut[k] = p_lpImagOut[j] - ti;

                p_lpRealOut[j] += tr;
                p_lpImagOut[j] += ti;

            }
        }

        BlockEnd = BlockSize;

    }
}

#define FFT_LEN (2*128)
inline double GetFrequencyIntensity(double re, double im)
{
    return sqrt((re*re)+(im*im));
}
#define mag_sqrd(re,im) (re*re+im*im)
#define Decibels(re,im) ((re == 0 && im == 0) ? (0) :  \
        10.0 * log10(double(mag_sqrd(re,im))))
#define Amplitude(re,im,len) (GetFrequencyIntensity(re,im)/(len))
#define AmplitudeScaled(re,im,len,scale) ((int)Amplitude(re,im,len)%scale)


void MediaFileGetFFT(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf && mf->pGrabber) {
		AM_MEDIA_TYPE mt;
		if (!FAILED(mf->pGrabber->GetConnectedMediaType(&mt))) {
			if (mt.formattype == FORMAT_WaveFormatEx &&
				mt.cbFormat >= sizeof(WAVEFORMATEX)) {
					WAVEFORMATEX *w = (WAVEFORMATEX *) mt.pbFormat;
					WAVEFORMATEXTENSIBLE *w2 = (WAVEFORMATEXTENSIBLE *) mt.pbFormat;
					long size = 0;
					int res = mf->pGrabber->GetCurrentBuffer(&size, 0);
					long *buffer;
					if (res == S_OK && (buffer=(long*) malloc(size))) {
						AM_MEDIA_TYPE type;
						mf->pGrabber->GetConnectedMediaType(&type);
						res = mf->pGrabber->GetCurrentBuffer(&size, buffer);
						buffer = buffer;

    DWORD nCount = 0;
	double finleft[FFT_LEN];
	double finright[FFT_LEN];
	double fout[FFT_LEN];
	double foutimg[FFT_LEN];
	double fdraw[FFT_LEN];
    for (DWORD dw = 0; dw < FFT_LEN; dw++)
    {
        {
            //copy audio signal to fft real component for left channel
            finleft[nCount] = (double)((short*)buffer)[dw++];
            //copy audio signal to fft real component for right channel
            finright[nCount++] = (double)((short*)buffer)[dw];
        }
    }
    //Perform FFT on left channel
    fft_double(FFT_LEN/2,finleft,fout,foutimg);
    double re,im,fmax=-99999.9f,fmin=99999.9f;
	if (CreateListValue(s, 130)) {
		for (int i=1; i<62; i++) {
			ScriptValue sv;
			CreateDoubleValue(sv, fdraw[i]);
			s.listVal->PushBack(sv);
		}
	}
    for(int i=0;i < FFT_LEN/4;i++)
    //Use FFT_LEN/4 since the data is mirrored within the array.
    {
        re = fout[i];
        im = foutimg[i];
        //get amplitude and scale to 0..256 range
        fdraw[i]=AmplitudeScaled(re,im,FFT_LEN/2,256);
        if (fdraw[i] > fmax)
        {
            fmax = fdraw[i];
        }
        if (fdraw[i] < fmin)
        {
            fmin = fdraw[i];
        }
    }
    //Use this to send the average band amplitude to something
    int nAvg, nBars=16, nCur = 0;
	if (CreateListValue(s, 64)) {
		for (int i=0; i<FFT_LEN/4; i++) {
			ScriptValue sv;
			CreateDoubleValue(sv, sqrt(fout[i]*fout[i]+foutimg[i]*foutimg[i]));
			s.listVal->PushBack(sv);
		}
	}
    for(int i=1;i < FFT_LEN/4;i++)
    {
        nAvg = 0;
        for (int n=0; n < nBars; n++)
        {
            nAvg += (int)fdraw[i];
        }
        nAvg /= nBars;
        //Send data here to something,
        //nothing to send it to so we print it.
        //TRACE("Average for Bar#%d is %d\n",nCur++,nAvg);
        i+=nBars-1;
    }
	/*
    DataHolder* pDataHolder = (DataHolder*)lpData;
    // Draw left channel
    CFrequencyGraph* pPeak = (CFrequencyGraph*)pDataHolder->pData;
    if (::IsWindow(pPeak->GetSafeHwnd()))
    {
        pPeak->SetYRange(0,256);
        pPeak->Update(FFT_LEN/4,fdraw);
    }
    
    // Perform FFT on right channel
    fmax=-99999.9f,fmin=99999.9f;
    fft_double(FFT_LEN/2,0,finright,NULL,fout,foutimg);
    fdraw[0] = fdraw[FFT_LEN/4] = 0;
    for(i=1;i < FFT_LEN/4-1;i++)
    //Use FFT_LEN/4 since the data is mirrored within the array.
    {
        re = fout[i];
        im = foutimg[i];
        //get Decibels in 0-110 range
        fdraw[i] = Decibels(re,im);
        if (fdraw[i] > fmax)
        {
            fmax = fdraw[i];
        }
        if (fdraw[i] < fmin)
        {
            fmin = fdraw[i];
        }
    }
    //Draw right channel
    CFrequencyGraph* pPeak2 = (CFrequencyGraph*)pDataHolder->pData2;
    if (::IsWindow(pPeak2->GetSafeHwnd()))
    {
        pPeak2->SetNumberOfSteps(50);
        //Use updated dynamic range for scaling
        pPeak2->SetYRange((int)fmin,(int)fmax);
        pPeak2->Update(FFT_LEN/4,fdraw);
    }//*/
						free(buffer);
					}
			}
			FreeMediaType(&mt);
		}
	}
}

void MediaFileGetState(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	CreateNullValue(s);
	if (mf) {
		CreateIntValue(s, mf->status);
	}
}

void DestroyMediaFile(ScriptValue &s, ScriptValue *args) {
	MediaFileData* mf = GetMediaFileData(s);
	for (int i=0; i<numMediaFiles; i++) {
		if (MediaFiles[i] == s.objectVal) {
			MediaFiles[i] = MediaFiles[--numMediaFiles];
			srealloc(MediaFiles, sizeof(ObjectValue*) * numMediaFiles);
			break;
		}
	}
	if (mf) {
		ScriptValue sv;
		CreateIntValue(sv, 2);
		MediaFileStop(s, &sv);
		if (mf->title) mf->title->Release();
		if (mf->author) mf->author->Release();
		if (mf->fileName) mf->fileName->Release();
		if (mf->copyright) mf->desc->Release();
		if (mf->desc) mf->desc->Release();
		if (mf->com) UninitCom();
		//if (mf-) mf-->Release();
		/*
		StringValue *author;
		StringValue *fileName;
		StringValue *copyright;
		StringValue *desc;
		//*/
	}
}


void HandleDshowEvent(size_t wParam, size_t lParam) {
	ObjectValue *o = (ObjectValue *)wParam;
	int i;
	for (i=0; i<numMediaFiles; i++) {
		if (MediaFiles[i] == o) {
			break;
		}
	}
	if (i == numMediaFiles) return;
	long code;
	LONG_PTR lParam1, lParam2;
	MediaFileData* mf = GetMediaFileData(o);
	ScriptValue args[2];
	CreateIntValue(args[1], 0);
	// Just in case.
	CreateObjectValue(args[0], o);
	int stat;
	if (mf && mf->pEvent && S_OK == mf->pEvent->GetEvent(&code, &lParam1, &lParam2, 0)) {
		switch(code) {
			case EC_COMPLETE:
				stat = mf->status;
				mf->status = MEDIA_STOPPED;
				if (mf->pControl) mf->pControl->Stop();
				if (!FAILED(lParam1)) {
					args[1].intVal = 1;
				}
				else {
					args[1].intVal = -1;
				}
				o->AddRef();
				TriggerEvent(mediaEvent, &args[0], &args[1]);
				break;
			case EC_GRAPH_CHANGED:
				GetMediaInfo(mf);
				o->AddRef();
				TriggerEvent(mediaEvent, &args[0], &args[1]);
				break;
			default:
				code = code;
				break;
		}
		mf->pEvent->FreeEventParams(code, lParam1, lParam2);
	}
}
