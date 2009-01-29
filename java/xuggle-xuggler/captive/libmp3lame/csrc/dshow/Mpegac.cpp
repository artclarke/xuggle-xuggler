/*
 *  LAME MP3 encoder for DirectShow
 *  DirectShow filter implementation
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
//#include <olectlid.h>
#include "uids.h"
#include "iaudioprops.h"
#include "mpegac.h"
#include "resource.h"

#include "PropPage.h"
#include "PropPage_adv.h"
#include "aboutprp.h"

#include "Encoder.h"
#include "Reg.h"

#ifndef _INC_MMREG
#include <mmreg.h>
#endif

// default parameters
#define         DEFAULT_LAYER               3
#define         DEFAULT_STEREO_MODE         JOINT_STEREO
#define         DEFAULT_FORCE_MS            0
#define         DEFAULT_MODE_FIXED          0
#define         DEFAULT_ENFORCE_MIN         0
#define         DEFAULT_VOICE               0
#define         DEFAULT_KEEP_ALL_FREQ       0
#define         DEFAULT_STRICT_ISO          0
#define         DEFAULT_DISABLE_SHORT_BLOCK 0
#define         DEFAULT_XING_TAG            0
#define         DEFAULT_SAMPLE_RATE         44100
#define         DEFAULT_BITRATE             128
#define         DEFAULT_VARIABLE            0
#define         DEFAULT_CRC                 0
#define         DEFAULT_FORCE_MONO          0
#define         DEFAULT_SET_DURATION        1
#define         DEFAULT_SAMPLE_OVERLAP      1
#define         DEFAULT_COPYRIGHT           0
#define         DEFAULT_ORIGINAL            0
#define         DEFAULT_VARIABLEMIN         80
#define         DEFAULT_VARIABLEMAX         160
#define         DEFAULT_ENCODING_QUALITY    5
#define         DEFAULT_VBR_QUALITY         4
#define         DEFAULT_PES                 0


/*  Registration setup stuff */
//  Setup data

#define OUT_STREAM_TYPE MEDIATYPE_Stream
#define OUT_STREAM_SUBTYPE MEDIASUBTYPE_MPEG1Audio
//#define OUT_STREAM_SUBTYPE MEDIASUBTYPE_NULL

AMOVIESETUP_MEDIATYPE sudMpgInputType[] =
{
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM }
};
AMOVIESETUP_MEDIATYPE sudMpgOutputType[] =
{
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload },
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO },
	{ &OUT_STREAM_TYPE, &OUT_STREAM_SUBTYPE},
};

AMOVIESETUP_PIN sudMpgPins[] =
{
    { L"PCM Input",
      FALSE,                               // bRendered
      FALSE,                               // bOutput
      FALSE,                               // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgInputType),            // Number of media types
      sudMpgInputType
    },
    { L"MPEG Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      FALSE,                               // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgOutputType),           // Number of media types
      sudMpgOutputType
    }
};

AMOVIESETUP_FILTER sudMpgAEnc =
{
	&CLSID_LAMEDShowFilter,
    L"LAME Audio Encoder",
    MERIT_SW_COMPRESSOR,                   // Don't use us for real!
    NUMELMS(sudMpgPins),                   // 3 pins
    sudMpgPins
};

/*****************************************************************************/
// COM Global table of objects in this dll
CFactoryTemplate g_Templates[] = 
{
	{ L"LAME Audio Encoder", &CLSID_LAMEDShowFilter, CMpegAudEnc::CreateInstance, NULL, &sudMpgAEnc },
  { L"LAME Audio Encoder Property Page", &CLSID_LAMEDShow_PropertyPage, CMpegAudEncPropertyPage::CreateInstance},
  { L"LAME Audio Encoder Property Page", &CLSID_LAMEDShow_PropertyPageAdv, CMpegAudEncPropertyPageAdv::CreateInstance},
  { L"LAME Audio Encoder About", &CLSID_LAMEDShow_About, CMAEAbout::CreateInstance}
};
// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

CUnknown *CMpegAudEnc::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CMpegAudEnc(lpunk, phr);
    if (punk == NULL) 
        *phr = E_OUTOFMEMORY;
    return punk;
}

CMpegAudEnc::CMpegAudEnc(LPUNKNOWN lpunk, HRESULT *phr)
 :  CTransformFilter(NAME("LAME Audio Encoder"), lpunk, 
    CLSID_LAMEDShowFilter),
    CPersistStream(lpunk, phr)
{
    MPEG_ENCODER_CONFIG mec;
    ReadPresetSettings(&mec);
    m_Encoder.SetOutputType(mec);

    m_hasFinished = TRUE;
}

CMpegAudEnc::~CMpegAudEnc(void)
{
}

LPAMOVIESETUP_FILTER CMpegAudEnc::GetSetupData()
{
    return &sudMpgAEnc;
}


HRESULT CMpegAudEnc::Receive(IMediaSample * pSample)
{
    CAutoLock lock(&m_cs);

    if (!pSample)
        return S_OK;

    BYTE * pSourceBuffer = NULL;

    if (pSample->GetPointer(&pSourceBuffer) != S_OK || !pSourceBuffer)
        return S_OK;

    long sample_size = pSample->GetActualDataLength();

    REFERENCE_TIME rtStart, rtStop;
    BOOL gotValidTime = (pSample->GetTime(&rtStart, &rtStop) != VFW_E_SAMPLE_TIME_NOT_SET);

    if (sample_size <= 0 || pSourceBuffer == NULL || m_hasFinished || (gotValidTime && rtStart < 0))
        return S_OK;

    if (gotValidTime)
    {
        if (m_rtStreamTime < 0)
        {
            m_rtStreamTime = rtStart;
            m_rtEstimated = rtStart;
        }
        else
        {
            resync_point_t * sync = m_sync + m_sync_in_idx;

            if (sync->applied)
            {
                REFERENCE_TIME rtGap = rtStart - m_rtEstimated;

                // if old sync data is applied and gap is greater than 1 ms
                // then make a new synchronization point
                if (rtGap > 10000 || (m_allowOverlap && rtGap < -10000))
                {
                    sync->sample    = m_samplesIn;
                    sync->delta     = rtGap;
                    sync->applied   = FALSE;

                    m_rtEstimated  += sync->delta;

                    if (m_sync_in_idx < (RESYNC_COUNT - 1))
                        m_sync_in_idx++;
                    else
                        m_sync_in_idx = 0;
                }
            }
        }
    }

    m_rtEstimated   += (LONGLONG)(m_bytesToDuration * sample_size);
    m_samplesIn     += sample_size / m_bytesPerSample;

    while (sample_size > 0)
    {
        int bytes_processed = m_Encoder.Encode((short *)pSourceBuffer, sample_size);

        if (bytes_processed <= 0)
            return S_OK;

        FlushEncodedSamples();

        sample_size     -= bytes_processed;
        pSourceBuffer   += bytes_processed;
    }

    return S_OK;
}




HRESULT CMpegAudEnc::FlushEncodedSamples()
{
    IMediaSample * pOutSample = NULL;
    BYTE * pDst = NULL;

	if(m_bStreamOutput)
	{
		HRESULT hr = S_OK;
		const unsigned char *   pblock      = NULL;
		int iBufferSize;
		int iBlockLenght = m_Encoder.GetBlockAligned(&pblock, &iBufferSize, m_cbStramAlignment);
		
		if(!iBlockLenght)
			return S_OK;

		hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
		if (hr == S_OK && pOutSample)
		{
			hr = pOutSample->GetPointer(&pDst);
			if (hr == S_OK && pDst)
			{
				CopyMemory(pDst, pblock, iBlockLenght);
				REFERENCE_TIME rtEndPos = m_rtBytePos + iBufferSize;
				EXECUTE_ASSERT(S_OK == pOutSample->SetTime(&m_rtBytePos, &rtEndPos));
				pOutSample->SetActualDataLength(iBufferSize);
				m_rtBytePos += iBlockLenght;
				m_pOutput->Deliver(pOutSample);
			}

			pOutSample->Release();
		}
		return S_OK;
	}

    if (m_rtStreamTime < 0)
        m_rtStreamTime = 0;

    while (1)
    {
        const unsigned char *   pframe      = NULL;
        int                     frame_size  = m_Encoder.GetFrame(&pframe);

        if (frame_size <= 0 || !pframe)
            break;

        if (!m_sync[m_sync_out_idx].applied && m_sync[m_sync_out_idx].sample <= m_samplesOut)
        {
            m_rtStreamTime += m_sync[m_sync_out_idx].delta;
            m_sync[m_sync_out_idx].applied = TRUE;

            if (m_sync_out_idx < (RESYNC_COUNT - 1))
                m_sync_out_idx++;
            else
                m_sync_out_idx = 0;
        }

        REFERENCE_TIME rtStart = m_rtStreamTime;
        REFERENCE_TIME rtStop = rtStart + m_rtFrameTime;

		HRESULT hr = S_OK;
		
		hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
		if (hr == S_OK && pOutSample)
		{
			hr = pOutSample->GetPointer(&pDst);
			if (hr == S_OK && pDst)
			{
				CopyMemory(pDst, pframe, frame_size);
				pOutSample->SetActualDataLength(frame_size);
			
				pOutSample->SetSyncPoint(TRUE);
				pOutSample->SetTime(&rtStart, m_setDuration ? &rtStop : NULL);
			

				m_pOutput->Deliver(pOutSample);
			}

			pOutSample->Release();
		}
	

        m_samplesOut += m_samplesPerFrame;
        m_rtStreamTime = rtStop;
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//  StartStreaming - prepare to receive new data
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::StartStreaming()
{
    WAVEFORMATEX * pwfxIn  = (WAVEFORMATEX *) m_pInput->CurrentMediaType().Format();

    m_bytesPerSample    = pwfxIn->nChannels * sizeof(short);
	DWORD dwOutSampleRate;
	if(MEDIATYPE_Stream == m_pOutput->CurrentMediaType().majortype)
	{
		MPEG_ENCODER_CONFIG mcfg;
		if(FAILED(m_Encoder.GetOutputType(&mcfg)))
			return E_FAIL;
		
		dwOutSampleRate = mcfg.dwSampleRate;
	}
	else
	{
		dwOutSampleRate = ((WAVEFORMATEX *) m_pOutput->CurrentMediaType().Format())->nSamplesPerSec;
	}
	m_samplesPerFrame   = (dwOutSampleRate >= 32000) ? 1152 : 576;

    m_rtFrameTime = MulDiv(10000000, m_samplesPerFrame, dwOutSampleRate);

    m_samplesIn = m_samplesOut = 0;
    m_rtStreamTime = -1;
	m_rtBytePos = 0;

    // initialize encoder
    m_Encoder.Init();

    m_hasFinished   = FALSE;

    for (int i = 0; i < RESYNC_COUNT; i++)
    {
        m_sync[i].sample   = 0;
        m_sync[i].delta    = 0;
        m_sync[i].applied  = TRUE;
    }

    m_sync_in_idx = 0;
    m_sync_out_idx = 0;

    get_SetDuration(&m_setDuration);
    get_SampleOverlap(&m_allowOverlap);

	return S_OK;
}


HRESULT CMpegAudEnc::StopStreaming()
{
	IStream *pStream = NULL;
	if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
	{
		IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
		if(pDwnstrmInputPin && FAILED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
		{
			pStream = NULL;
		}
	}
	

	m_Encoder.Close(pStream);

	if(pStream)
		pStream->Release();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//  EndOfStream - stop data processing 
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::EndOfStream()
{
    CAutoLock lock(&m_cs);

    // Flush data
    m_Encoder.Finish();
    FlushEncodedSamples();

	IStream *pStream = NULL;
    if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
	{
		IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
		if(pDwnstrmInputPin)
		{
			if(FAILED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
			{
				pStream = NULL;	
			}
		}
	}

	if(pStream)
	{
		ULARGE_INTEGER size;
		size.QuadPart = m_rtBytePos;
		pStream->SetSize(size);	
	}

	m_Encoder.Close(pStream);

	if(pStream)
		pStream->Release();

    m_hasFinished = TRUE;

    return CTransformFilter::EndOfStream();
}


////////////////////////////////////////////////////////////////////////////
//  BeginFlush  - stop data processing 
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::BeginFlush()
{
    HRESULT hr = CTransformFilter::BeginFlush();

    if (SUCCEEDED(hr))
    {
        CAutoLock lock(&m_cs);

        DWORD dwDstSize = 0;

        // Flush data
        m_Encoder.Finish();
        FlushEncodedSamples();

		IStream *pStream = NULL;
		if(m_bStreamOutput && m_pOutput->IsConnected() != FALSE)
		{
			IPin * pDwnstrmInputPin = m_pOutput->GetConnected();
			if(pDwnstrmInputPin && SUCCEEDED(pDwnstrmInputPin->QueryInterface(IID_IStream, (LPVOID*)(&pStream))))
			{
				ULARGE_INTEGER size;
				size.QuadPart = m_rtBytePos;
				pStream->SetSize(size);	
				pStream->Release();
			}
		}
	    m_rtStreamTime = -1;
		m_rtBytePos = 0;
    }

    return hr;
}



////////////////////////////////////////////////////////////////////////////
//	SetMediaType - called when filters are connecting
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::SetMediaType(PIN_DIRECTION direction, const CMediaType * pmt)
{
    HRESULT hr = S_OK;

    if (direction == PINDIR_INPUT)
    {
		if (*pmt->FormatType() != FORMAT_WaveFormatEx)
        return VFW_E_INVALIDMEDIATYPE;

		if (pmt->FormatLength() < sizeof(WAVEFORMATEX))
			return VFW_E_INVALIDMEDIATYPE;

        DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::SetMediaType(), direction = PINDIR_INPUT")));

        // Pass input media type to encoder
        m_Encoder.SetInputType((LPWAVEFORMATEX)pmt->Format());

        WAVEFORMATEX * pwfx = (WAVEFORMATEX *)pmt->Format();

        if (pwfx)
            m_bytesToDuration = (float)1.e7 / (float)(pwfx->nChannels * sizeof(short) * pwfx->nSamplesPerSec);
        else
            m_bytesToDuration = 0.0;

        Reconnect();
    }
    else if (direction == PINDIR_OUTPUT)
    {
		if(MEDIATYPE_Stream != *pmt->Type())
		{
			WAVEFORMATEX wfIn;
			m_Encoder.GetInputType(&wfIn);

			if (wfIn.nSamplesPerSec %
				((LPWAVEFORMATEX)pmt->Format())->nSamplesPerSec != 0)
	            return VFW_E_TYPE_NOT_ACCEPTED;
		}
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// CheckInputType - check if you can support mtIn
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::CheckInputType(const CMediaType* mtIn)
{
    if (*mtIn->Type() == MEDIATYPE_Audio && *mtIn->FormatType() == FORMAT_WaveFormatEx)
        if (mtIn->FormatLength() >= sizeof(WAVEFORMATEX))
            if (mtIn->IsTemporalCompressed() == FALSE)
                return m_Encoder.SetInputType((LPWAVEFORMATEX)mtIn->Format(), true);

    return E_INVALIDARG;
}

////////////////////////////////////////////////////////////////////////////
// CheckTransform - checks if we can support the transform from this input to this output
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(MEDIATYPE_Stream != mtOut->majortype)
	{
		if (*mtOut->FormatType() != FORMAT_WaveFormatEx)
			return VFW_E_INVALIDMEDIATYPE;

		if (mtOut->FormatLength() < sizeof(WAVEFORMATEX))
			return VFW_E_INVALIDMEDIATYPE;

		MPEG_ENCODER_CONFIG	mec;
		if(FAILED(m_Encoder.GetOutputType(&mec)))
			return S_OK;

		if (((LPWAVEFORMATEX)mtIn->Format())->nSamplesPerSec % mec.dwSampleRate != 0)
			return S_OK;

		if (mec.dwSampleRate != ((LPWAVEFORMATEX)mtOut->Format())->nSamplesPerSec)
			return VFW_E_TYPE_NOT_ACCEPTED;

		return S_OK;
	}
	else if(mtOut->subtype == OUT_STREAM_SUBTYPE)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}

////////////////////////////////////////////////////////////////////////////
// DecideBufferSize - sets output buffers number and size
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::DecideBufferSize(
                        IMemAllocator*		  pAllocator,
                        ALLOCATOR_PROPERTIES* pProperties)
{
    HRESULT hr = S_OK;

	m_bStreamOutput = (OUT_STREAM_TYPE == m_pOutput->CurrentMediaType().majortype);
	if(m_bStreamOutput)
		m_cbStramAlignment = pProperties->cbAlign;

    ///
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = OUT_BUFFER_SIZE;
	//
	
	ASSERT(pProperties->cbBuffer);
	
    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
        return hr;

    if (Actual.cbBuffer < pProperties->cbBuffer ||
        Actual.cBuffers < pProperties->cBuffers) 
    {// can't use this allocator
        return E_INVALIDARG;
    }
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// GetMediaType - overrideble for suggesting outpu pin media types
////////////////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::GetMediaType()")));

    if (iPosition < 0)
        return E_INVALIDARG;

    switch (iPosition)
    {
    case 0:
        {// We can support two output streams - PES or AES	
            if(m_Encoder.IsPES())
            {
                pMediaType->SetType(&MEDIATYPE_MPEG2_PES);
                pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_AUDIO);
            }
            else
            {
                pMediaType->SetType(&MEDIATYPE_Audio);
                pMediaType->SetSubtype(&MEDIASUBTYPE_MP3);
            }
            break;
        }
	case 1:
		{
			pMediaType->SetType(&MEDIATYPE_Stream);
			pMediaType->SetSubtype(&OUT_STREAM_SUBTYPE);
			break;
		}
    default:
        return VFW_S_NO_MORE_ITEMS;
    }

    if (m_pInput->IsConnected() == FALSE)
    {
        pMediaType->SetFormatType(&FORMAT_None);
        return NOERROR;
    }

    SetOutMediaType();
    
	if(iPosition != 1)
	{
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetSampleSize(OUT_BUFFER_SIZE);
		pMediaType->AllocFormatBuffer(sizeof(MPEGLAYER3WAVEFORMAT));
		pMediaType->SetFormat((LPBYTE)&m_mwf, sizeof(MPEGLAYER3WAVEFORMAT));
		pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
	}
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//	SetOutMediaType - sets filter output media type according to 
//  current encoder out MPEG audio properties 
////////////////////////////////////////////////////////////////////////////
void CMpegAudEnc::SetOutMediaType()
{
	MPEG_ENCODER_CONFIG mec;
	if(FAILED(m_Encoder.GetOutputType(&mec)))
		return ;
	WAVEFORMATEX wf;
	if(FAILED(m_Encoder.GetInputType(&wf)))
		return ;

//	BOOL bDirty = FALSE;

	if ((wf.nSamplesPerSec % mec.dwSampleRate) != 0)
	{
		mec.dwSampleRate = wf.nSamplesPerSec;
		m_Encoder.SetOutputType(mec);
//		bDirty = TRUE;
	}

	if (wf.nChannels == 1 && mec.ChMode != MONO)
	{
		mec.ChMode = MONO;
		m_Encoder.SetOutputType(mec);
//		bDirty = TRUE;
	}

	if (wf.nChannels == 2 && !mec.bForceMono && mec.ChMode == MONO)
	{
		mec.ChMode = STEREO;
		m_Encoder.SetOutputType(mec);
//		bDirty = TRUE;
	}

	if (wf.nChannels == 2 && mec.bForceMono)
	{
		mec.ChMode = MONO;
		m_Encoder.SetOutputType(mec);
//		bDirty = TRUE;
	}
/*
    if (bDirty)
	{
		// we are tied to the registry, especially our configuration
		// so when we change the incorrect sample rate, we need
		// to change the value in registry too. Same to channel mode
		SaveAudioEncoderPropertiesToRegistry();
		DbgLog((LOG_TRACE, 1, TEXT("Changed sampling rate internally")));
	}
*/
	// Fill MPEG1WAVEFORMAT DirectShow SDK structure
	m_mwf.wfx.wFormatTag		= WAVE_FORMAT_MPEGLAYER3;
	m_mwf.wfx.cbSize			= sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);

	if(mec.ChMode == MONO)
		m_mwf.wfx.nChannels = 1;
	else
		m_mwf.wfx.nChannels = 2;

	m_mwf.wfx.nSamplesPerSec	= mec.dwSampleRate;
	m_mwf.wfx.nBlockAlign		= 1;

	m_mwf.wfx.wBitsPerSample	= 0;
	m_mwf.wfx.nAvgBytesPerSec	= mec.dwBitrate * 1000 / 8;


	m_mwf.wID = MPEGLAYER3_ID_MPEG;
	m_mwf.fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
	m_mwf.nBlockSize = 1;
	m_mwf.nFramesPerBlock = 1;
	m_mwf.nCodecDelay = 0;
}

HRESULT CMpegAudEnc::Reconnect()
{
    HRESULT hr = S_FALSE;

    if (m_pOutput && m_pOutput->IsConnected() && m_State == State_Stopped)
    {
        MPEG_ENCODER_CONFIG mec;
        hr = m_Encoder.GetOutputType(&mec);

        if ((hr = m_Encoder.SetOutputType(mec)) == S_OK)
        {
            CMediaType cmt;
			cmt.InitMediaType();
			if(!m_bStreamOutput)
				GetMediaType(0, &cmt);
			else
				GetMediaType(1, &cmt);

            if (S_OK == (hr = m_pOutput->GetConnected()->QueryAccept(&cmt)))
                hr = m_pGraph->Reconnect(m_pOutput);
            else
                hr = m_pOutput->SetMediaType(&cmt);
        }
    }

    return hr;
}

//
// Read persistent configuration from Registry
//
void CMpegAudEnc::ReadPresetSettings(MPEG_ENCODER_CONFIG * pmec)
{
    DbgLog((LOG_TRACE,1,TEXT("CMpegAudEnc::ReadPresetSettings()")));

    CRegKey rk(HKEY_CURRENT_USER, KEY_LAME_ENCODER);

    pmec->dwBitrate         = rk.getDWORD(VALUE_BITRATE,DEFAULT_BITRATE);
    pmec->dwVariableMin     = rk.getDWORD(VALUE_VARIABLEMIN,DEFAULT_VARIABLEMIN);
    pmec->dwVariableMax     = rk.getDWORD(VALUE_VARIABLEMAX,DEFAULT_VARIABLEMAX);
    pmec->vmVariable        = rk.getDWORD(VALUE_VARIABLE, DEFAULT_VARIABLE) ? vbr_rh : vbr_off;
    pmec->dwQuality         = rk.getDWORD(VALUE_QUALITY,DEFAULT_ENCODING_QUALITY);
    pmec->dwVBRq            = rk.getDWORD(VALUE_VBR_QUALITY,DEFAULT_VBR_QUALITY);
    pmec->lLayer            = rk.getDWORD(VALUE_LAYER, DEFAULT_LAYER);
    pmec->bCRCProtect       = rk.getDWORD(VALUE_CRC, DEFAULT_CRC);
    pmec->bForceMono        = rk.getDWORD(VALUE_FORCE_MONO, DEFAULT_FORCE_MONO);
    pmec->bSetDuration      = rk.getDWORD(VALUE_SET_DURATION, DEFAULT_SET_DURATION);
    pmec->bSampleOverlap    = rk.getDWORD(VALUE_SAMPLE_OVERLAP, DEFAULT_SAMPLE_OVERLAP);
    pmec->bCopyright        = rk.getDWORD(VALUE_COPYRIGHT, DEFAULT_COPYRIGHT);
    pmec->bOriginal         = rk.getDWORD(VALUE_ORIGINAL, DEFAULT_ORIGINAL);
    pmec->dwSampleRate      = rk.getDWORD(VALUE_SAMPLE_RATE, DEFAULT_SAMPLE_RATE);
    pmec->dwPES             = rk.getDWORD(VALUE_PES, DEFAULT_PES);

    pmec->ChMode            = (MPEG_mode)rk.getDWORD(VALUE_STEREO_MODE, DEFAULT_STEREO_MODE);
    pmec->dwForceMS         = rk.getDWORD(VALUE_FORCE_MS, DEFAULT_FORCE_MS);

    pmec->dwEnforceVBRmin   = rk.getDWORD(VALUE_ENFORCE_MIN, DEFAULT_ENFORCE_MIN);
    pmec->dwVoiceMode       = rk.getDWORD(VALUE_VOICE, DEFAULT_VOICE);
    pmec->dwKeepAllFreq     = rk.getDWORD(VALUE_KEEP_ALL_FREQ, DEFAULT_KEEP_ALL_FREQ);
    pmec->dwStrictISO       = rk.getDWORD(VALUE_STRICT_ISO, DEFAULT_STRICT_ISO);
    pmec->dwNoShortBlock    = rk.getDWORD(VALUE_DISABLE_SHORT_BLOCK, DEFAULT_DISABLE_SHORT_BLOCK);
    pmec->dwXingTag         = rk.getDWORD(VALUE_XING_TAG, DEFAULT_XING_TAG);
    pmec->dwModeFixed       = rk.getDWORD(VALUE_MODE_FIXED, DEFAULT_MODE_FIXED);

    rk.Close();
}

////////////////////////////////////////////////////////////////
//  Property page handling 
////////////////////////////////////////////////////////////////
HRESULT CMpegAudEnc::GetPages(CAUUID *pcauuid) 
{
    GUID *pguid;

    pcauuid->cElems = 3;
    pcauuid->pElems = pguid = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pcauuid->cElems);

    if (pcauuid->pElems == NULL)
        return E_OUTOFMEMORY;

    pguid[0] = CLSID_LAMEDShow_PropertyPage;
    pguid[1] = CLSID_LAMEDShow_PropertyPageAdv;
    pguid[2] = CLSID_LAMEDShow_About;

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::NonDelegatingQueryInterface(REFIID riid, void ** ppv) 
{

    if (riid == IID_ISpecifyPropertyPages)
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
	else if(riid == IID_IPersistStream)
        return GetInterface((IPersistStream *)this, ppv);
//    else if (riid == IID_IVAudioEncSettings)
//        return GetInterface((IVAudioEncSettings*) this, ppv);
    else if (riid == IID_IAudioEncoderProperties)
        return GetInterface((IAudioEncoderProperties*) this, ppv);

    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

////////////////////////////////////////////////////////////////
//IVAudioEncSettings interface methods
////////////////////////////////////////////////////////////////

//
// IAudioEncoderProperties
//
STDMETHODIMP CMpegAudEnc::get_PESOutputEnabled(DWORD *dwEnabled)
{
    *dwEnabled = (DWORD)m_Encoder.IsPES();
    DbgLog((LOG_TRACE, 1, TEXT("get_PESOutputEnabled -> %d"), *dwEnabled));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_PESOutputEnabled(DWORD dwEnabled)
{
    m_Encoder.SetPES((BOOL)!!dwEnabled);
    DbgLog((LOG_TRACE, 1, TEXT("set_PESOutputEnabled(%d)"), !!dwEnabled));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_MPEGLayer(DWORD *dwLayer)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwLayer = (DWORD)mec.lLayer;

    DbgLog((LOG_TRACE, 1, TEXT("get_MPEGLayer -> %d"), *dwLayer));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_MPEGLayer(DWORD dwLayer)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    if (dwLayer == 2)
        mec.lLayer = 2;
    else if (dwLayer == 1)
        mec.lLayer = 1;
    m_Encoder.SetOutputType(mec);

    DbgLog((LOG_TRACE, 1, TEXT("set_MPEGLayer(%d)"), dwLayer));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Bitrate(DWORD *dwBitrate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwBitrate = (DWORD)mec.dwBitrate;
    DbgLog((LOG_TRACE, 1, TEXT("get_Bitrate -> %d"), *dwBitrate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Bitrate(DWORD dwBitrate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwBitrate = dwBitrate;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Bitrate(%d)"), dwBitrate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Variable(DWORD *dwVariable)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwVariable = (DWORD)(mec.vmVariable == vbr_off ? 0 : 1);
    DbgLog((LOG_TRACE, 1, TEXT("get_Variable -> %d"), *dwVariable));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Variable(DWORD dwVariable)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);

    mec.vmVariable = dwVariable ? vbr_rh : vbr_off;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variable(%d)"), dwVariable));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VariableMin(DWORD *dwMin)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwMin = (DWORD)mec.dwVariableMin;
    DbgLog((LOG_TRACE, 1, TEXT("get_Variablemin -> %d"), *dwMin));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableMin(DWORD dwMin)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVariableMin = dwMin;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variablemin(%d)"), dwMin));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VariableMax(DWORD *dwMax)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwMax = (DWORD)mec.dwVariableMax;
    DbgLog((LOG_TRACE, 1, TEXT("get_Variablemax -> %d"), *dwMax));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableMax(DWORD dwMax)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVariableMax = dwMax;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Variablemax(%d)"), dwMax));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_Quality(DWORD *dwQuality)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwQuality=(DWORD)mec.dwQuality;
    DbgLog((LOG_TRACE, 1, TEXT("get_Quality -> %d"), *dwQuality));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_Quality(DWORD dwQuality)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwQuality = dwQuality;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_Quality(%d)"), dwQuality));
    return S_OK;
}
STDMETHODIMP CMpegAudEnc::get_VariableQ(DWORD *dwVBRq)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwVBRq=(DWORD)mec.dwVBRq;
    DbgLog((LOG_TRACE, 1, TEXT("get_VariableQ -> %d"), *dwVBRq));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VariableQ(DWORD dwVBRq)             
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVBRq = dwVBRq;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_VariableQ(%d)"), dwVBRq));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::get_SourceSampleRate(DWORD *dwSampleRate)
{
    *dwSampleRate = 0;

    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
        return E_FAIL;

    *dwSampleRate = wf.nSamplesPerSec;
    DbgLog((LOG_TRACE, 1, TEXT("get_SourceSampleRate -> %d"), *dwSampleRate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SourceChannels(DWORD *dwChannels)
{
    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
        return E_FAIL;

    *dwChannels = wf.nChannels;
    DbgLog((LOG_TRACE, 1, TEXT("get_SourceChannels -> %d"), *dwChannels));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SampleRate(DWORD *dwSampleRate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwSampleRate = mec.dwSampleRate;
    DbgLog((LOG_TRACE, 1, TEXT("get_SampleRate -> %d"), *dwSampleRate));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SampleRate(DWORD dwSampleRate)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    DWORD dwOldSampleRate = mec.dwSampleRate;
    mec.dwSampleRate = dwSampleRate;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SampleRate(%d)"), dwSampleRate));

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ChannelMode(DWORD *dwChannelMode)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwChannelMode = mec.ChMode;
    DbgLog((LOG_TRACE, 1, TEXT("get_ChannelMode -> %d"), *dwChannelMode));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ChannelMode(DWORD dwChannelMode)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.ChMode = (MPEG_mode)dwChannelMode;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ChannelMode(%d)"), dwChannelMode));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ForceMS(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwForceMS;
    DbgLog((LOG_TRACE, 1, TEXT("get_ForceMS -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ForceMS(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwForceMS = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ForceMS(%d)"), dwFlag));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::get_CRCFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bCRCProtect;
    DbgLog((LOG_TRACE, 1, TEXT("get_CRCFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ForceMono(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bForceMono;
    DbgLog((LOG_TRACE, 1, TEXT("get_ForceMono -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SetDuration(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bSetDuration;
    DbgLog((LOG_TRACE, 1, TEXT("get_SetDuration -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_SampleOverlap(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bSampleOverlap;
    DbgLog((LOG_TRACE, 1, TEXT("get_SampleOverlap -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_CRCFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bCRCProtect = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_CRCFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ForceMono(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bForceMono = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ForceMono(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SetDuration(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bSetDuration = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SetDuration(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_SampleOverlap(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bSampleOverlap = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_SampleOverlap(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_EnforceVBRmin(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwEnforceVBRmin;
    DbgLog((LOG_TRACE, 1, TEXT("get_EnforceVBRmin -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_EnforceVBRmin(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwEnforceVBRmin = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_EnforceVBRmin(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_VoiceMode(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwVoiceMode;
    DbgLog((LOG_TRACE, 1, TEXT("get_VoiceMode -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_VoiceMode(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwVoiceMode = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_VoiceMode(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_KeepAllFreq(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwKeepAllFreq;
    DbgLog((LOG_TRACE, 1, TEXT("get_KeepAllFreq -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_KeepAllFreq(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwKeepAllFreq = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_KeepAllFreq(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_StrictISO(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.dwStrictISO;
    DbgLog((LOG_TRACE, 1, TEXT("get_StrictISO -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_StrictISO(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwStrictISO = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_StrictISO(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_NoShortBlock(DWORD *dwNoShortBlock)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwNoShortBlock = mec.dwNoShortBlock;
    DbgLog((LOG_TRACE, 1, TEXT("get_NoShortBlock -> %d"), *dwNoShortBlock));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_NoShortBlock(DWORD dwNoShortBlock)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwNoShortBlock = dwNoShortBlock;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_NoShortBlock(%d)"), dwNoShortBlock));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_XingTag(DWORD *dwXingTag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwXingTag = mec.dwXingTag;
    DbgLog((LOG_TRACE, 1, TEXT("get_XingTag -> %d"), *dwXingTag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_XingTag(DWORD dwXingTag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwXingTag = dwXingTag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_XingTag(%d)"), dwXingTag));
    return S_OK;
}



STDMETHODIMP CMpegAudEnc::get_OriginalFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bOriginal;
    DbgLog((LOG_TRACE, 1, TEXT("get_OriginalFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_OriginalFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bOriginal = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_OriginalFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_CopyrightFlag(DWORD *dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwFlag = mec.bCopyright;
    DbgLog((LOG_TRACE, 1, TEXT("get_CopyrightFlag -> %d"), *dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_CopyrightFlag(DWORD dwFlag)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.bCopyright = dwFlag;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_CopyrightFlag(%d)"), dwFlag));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ModeFixed(DWORD *dwModeFixed)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    *dwModeFixed = mec.dwModeFixed;
    DbgLog((LOG_TRACE, 1, TEXT("get_ModeFixed -> %d"), *dwModeFixed));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::set_ModeFixed(DWORD dwModeFixed)
{
    MPEG_ENCODER_CONFIG mec;
    m_Encoder.GetOutputType(&mec);
    mec.dwModeFixed = dwModeFixed;
    m_Encoder.SetOutputType(mec);
    DbgLog((LOG_TRACE, 1, TEXT("set_ModeFixed(%d)"), dwModeFixed));
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::get_ParameterBlockSize(BYTE *pcBlock, DWORD *pdwSize)
{
    DbgLog((LOG_TRACE, 1, TEXT("get_ParameterBlockSize -> %d%d"), *pcBlock, *pdwSize));

    if (pcBlock != NULL) {
        if (*pdwSize >= sizeof(MPEG_ENCODER_CONFIG)) {
            m_Encoder.GetOutputType((MPEG_ENCODER_CONFIG*)pcBlock);
            return S_OK;
        }
        else {
            *pdwSize = sizeof(MPEG_ENCODER_CONFIG);
            return E_FAIL;
        }
    }
    else if (pdwSize != NULL) {
        *pdwSize = sizeof(MPEG_ENCODER_CONFIG);
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CMpegAudEnc::set_ParameterBlockSize(BYTE *pcBlock, DWORD dwSize)
{
    DbgLog((LOG_TRACE, 1, TEXT("get_ParameterBlockSize(%d, %d)"), *pcBlock, dwSize));
    if (sizeof(MPEG_ENCODER_CONFIG) == dwSize){
        m_Encoder.SetOutputType(*(MPEG_ENCODER_CONFIG*)pcBlock);
        return S_OK;
    }
    else return E_FAIL; 
}


STDMETHODIMP CMpegAudEnc::DefaultAudioEncoderProperties()
{
    DbgLog((LOG_TRACE, 1, TEXT("DefaultAudioEncoderProperties()")));

    HRESULT hr = InputTypeDefined();
    if (FAILED(hr))
        return hr;

    DWORD dwSourceSampleRate;
    get_SourceSampleRate(&dwSourceSampleRate);

    set_PESOutputEnabled(DEFAULT_PES);
    set_MPEGLayer(DEFAULT_LAYER);

    set_Bitrate(DEFAULT_BITRATE);
    set_Variable(FALSE);
    set_VariableMin(DEFAULT_VARIABLEMIN);
    set_VariableMax(DEFAULT_VARIABLEMAX);
    set_Quality(DEFAULT_ENCODING_QUALITY);
    set_VariableQ(DEFAULT_VBR_QUALITY);

    set_SampleRate(dwSourceSampleRate);
    set_CRCFlag(DEFAULT_CRC);
    set_ForceMono(DEFAULT_FORCE_MONO);
    set_SetDuration(DEFAULT_SET_DURATION);
    set_SampleOverlap(DEFAULT_SAMPLE_OVERLAP);
    set_OriginalFlag(DEFAULT_ORIGINAL);
    set_CopyrightFlag(DEFAULT_COPYRIGHT);

    set_EnforceVBRmin(DEFAULT_ENFORCE_MIN);
    set_VoiceMode(DEFAULT_VOICE);
    set_KeepAllFreq(DEFAULT_KEEP_ALL_FREQ);
    set_StrictISO(DEFAULT_STRICT_ISO);
    set_NoShortBlock(DEFAULT_DISABLE_SHORT_BLOCK);
    set_XingTag(DEFAULT_XING_TAG);
    set_ForceMS(DEFAULT_FORCE_MS);
    set_ChannelMode(DEFAULT_STEREO_MODE);
    set_ModeFixed(DEFAULT_MODE_FIXED);

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::LoadAudioEncoderPropertiesFromRegistry()
{
    DbgLog((LOG_TRACE, 1, TEXT("LoadAudioEncoderPropertiesFromRegistry()")));

    MPEG_ENCODER_CONFIG mec;
    ReadPresetSettings(&mec);
    if(m_Encoder.SetOutputType(mec) == S_FALSE)
        return S_FALSE;
    return S_OK;
}

STDMETHODIMP CMpegAudEnc::SaveAudioEncoderPropertiesToRegistry()
{
    DbgLog((LOG_TRACE, 1, TEXT("SaveAudioEncoderPropertiesToRegistry()")));
    CRegKey rk;

    MPEG_ENCODER_CONFIG mec;
    if(m_Encoder.GetOutputType(&mec) == S_FALSE)
        return E_FAIL;

    if(rk.Create(HKEY_CURRENT_USER, KEY_LAME_ENCODER))
    {
        rk.setDWORD(VALUE_BITRATE, mec.dwBitrate);
        rk.setDWORD(VALUE_VARIABLE, mec.vmVariable);
        rk.setDWORD(VALUE_VARIABLEMIN, mec.dwVariableMin);
        rk.setDWORD(VALUE_VARIABLEMAX, mec.dwVariableMax);
        rk.setDWORD(VALUE_QUALITY, mec.dwQuality);
        rk.setDWORD(VALUE_VBR_QUALITY, mec.dwVBRq);

        rk.setDWORD(VALUE_CRC, mec.bCRCProtect);
        rk.setDWORD(VALUE_FORCE_MONO, mec.bForceMono);
        rk.setDWORD(VALUE_SET_DURATION, mec.bSetDuration);
        rk.setDWORD(VALUE_SAMPLE_OVERLAP, mec.bSampleOverlap);
        rk.setDWORD(VALUE_PES, mec.dwPES);
        rk.setDWORD(VALUE_COPYRIGHT, mec.bCopyright);
        rk.setDWORD(VALUE_ORIGINAL, mec.bOriginal);
        rk.setDWORD(VALUE_SAMPLE_RATE, mec.dwSampleRate);

        rk.setDWORD(VALUE_STEREO_MODE, mec.ChMode);
        rk.setDWORD(VALUE_FORCE_MS, mec.dwForceMS);
        rk.setDWORD(VALUE_XING_TAG, mec.dwXingTag);
        rk.setDWORD(VALUE_DISABLE_SHORT_BLOCK, mec.dwNoShortBlock);
        rk.setDWORD(VALUE_STRICT_ISO, mec.dwStrictISO);
        rk.setDWORD(VALUE_KEEP_ALL_FREQ, mec.dwKeepAllFreq);
        rk.setDWORD(VALUE_VOICE, mec.dwVoiceMode);
        rk.setDWORD(VALUE_ENFORCE_MIN, mec.dwEnforceVBRmin);
        rk.setDWORD(VALUE_MODE_FIXED, mec.dwModeFixed);

        rk.Close();
    }

    // Reconnect filter graph
    Reconnect();

    return S_OK;
}

STDMETHODIMP CMpegAudEnc::InputTypeDefined()
{
    WAVEFORMATEX wf;
    if(FAILED(m_Encoder.GetInputType(&wf)))
    {
        DbgLog((LOG_TRACE, 1, TEXT("!InputTypeDefined()")));
        return E_FAIL;
    }

    DbgLog((LOG_TRACE, 1, TEXT("InputTypeDefined()")));
    return S_OK;
}


STDMETHODIMP CMpegAudEnc::ApplyChanges()
{
    return Reconnect();
}

//
// CPersistStream stuff
//

// what is our class ID?
STDMETHODIMP CMpegAudEnc::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_LAMEDShowFilter;
    return S_OK;
}

HRESULT CMpegAudEnc::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("WriteToStream()")));

	MPEG_ENCODER_CONFIG mec;

	if(m_Encoder.GetOutputType(&mec) == S_FALSE)
		return E_FAIL;

    return pStream->Write(&mec, sizeof(mec), 0);
}


// what device should we use?  Used to re-create a .GRF file that we
// are in
HRESULT CMpegAudEnc::ReadFromStream(IStream *pStream)
{
	MPEG_ENCODER_CONFIG mec;

    HRESULT hr = pStream->Read(&mec, sizeof(mec), 0);
    if(FAILED(hr))
        return hr;

	if(m_Encoder.SetOutputType(mec) == S_FALSE)
		return S_FALSE;

    DbgLog((LOG_TRACE,1,TEXT("ReadFromStream() succeeded")));

    hr = S_OK;
    return hr;
}


// How long is our data?
int CMpegAudEnc::SizeMax()
{
    return sizeof(MPEG_ENCODER_CONFIG);
}



////////////////////////////////////////////
STDAPI DllRegisterServer()
{
	// Create entry in HKEY_CLASSES_ROOT\Filter
	OLECHAR szCLSID[CHARS_IN_GUID];
	TCHAR achTemp[MAX_PATH];
	HKEY hKey;

	HRESULT hr = StringFromGUID2(*g_Templates[0].m_ClsID, szCLSID, CHARS_IN_GUID);
	wsprintf(achTemp, TEXT("Filter\\%ls"), szCLSID);
	// create key
	RegCreateKey(HKEY_CLASSES_ROOT, (LPCTSTR)achTemp, &hKey);
	RegCloseKey(hKey);
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// Delete entry in HKEY_CLASSES_ROOT\Filter
	OLECHAR szCLSID[CHARS_IN_GUID];
	TCHAR achTemp[MAX_PATH];

	HRESULT hr = StringFromGUID2(*g_Templates[0].m_ClsID, szCLSID, CHARS_IN_GUID);
	wsprintf(achTemp, TEXT("Filter\\%ls"), szCLSID);
	// create key
	RegDeleteKey(HKEY_CLASSES_ROOT, (LPCTSTR)achTemp);
	return AMovieDllRegisterServer2(FALSE);
}

