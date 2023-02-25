//--------------------------------------------------------------------------------------
// File: audio.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "SDKmisc.h"
#include "SDKwavefile.h"
#include "audio.h"


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
AUDIO_STATE g_audioState;

//-----------------------------------------------------------------------------------------
// Initialize the audio by creating the XAudio2 device, mastering voice, etc.
//-----------------------------------------------------------------------------------------
HRESULT InitAudio()
{
    // Clear struct
    ZeroMemory( &g_audioState, sizeof( AUDIO_STATE ) );

    //
    // Initialize XAudio2
    //
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    UINT32 flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    HRESULT hr;

    if( FAILED( hr = XAudio2Create( &g_audioState.pXAudio2, flags ) ) )
        return hr;

    //
    // Create a mastering voice
    //
    assert( g_audioState.pXAudio2 != NULL );
    if( FAILED( hr = g_audioState.pXAudio2->CreateMasteringVoice( &g_audioState.pMasteringVoice ) ) )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        return hr;
    }

    //
    // Done
    //
    g_audioState.bInitialized = true;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Prepare a looping wave
//-----------------------------------------------------------------------------
HRESULT PrepareAudio( const LPWSTR wavname )
{
    if( !g_audioState.bInitialized )
        return E_FAIL;

    if( g_audioState.pSourceVoice )
    {
        g_audioState.pSourceVoice->Stop( 0 );
        g_audioState.pSourceVoice->DestroyVoice();
        g_audioState.pSourceVoice = 0;
    }

    SAFE_DELETE( g_audioState.pPipePre );
    SAFE_DELETE( g_audioState.pPipePost );

    //
    // Search for media
    //

    WCHAR strFilePath[ MAX_PATH ];
    WCHAR wavFilePath[ MAX_PATH ];

    wcscpy_s( wavFilePath, MAX_PATH, L"Media\\Wavs\\" );
    wcscat_s( wavFilePath, MAX_PATH, wavname );

    HRESULT hr;

    V_RETURN( DXUTFindDXSDKMediaFileCch( strFilePath, MAX_PATH, wavFilePath ) );

    //
    // Read in the wave file
    //
    CWaveFile wav;
    V_RETURN( wav.Open( strFilePath, NULL, WAVEFILE_READ ) );

    // Get format of wave file
    WAVEFORMATEX* pwfx = wav.GetFormat();

    // Calculate how many bytes and samples are in the wave
    DWORD cbWaveSize = wav.GetSize();

    // Read the sample data into memory
    SAFE_DELETE_ARRAY( g_audioState.pbSampleData );

    g_audioState.pbSampleData = new BYTE[ cbWaveSize ];

    V_RETURN( wav.Read( g_audioState.pbSampleData, cbWaveSize, &cbWaveSize ) );

    //
    // Play the wave using a XAudio2SourceVoice
    //

    // Create the source voice
    assert( g_audioState.pXAudio2 != NULL );
    V_RETURN( g_audioState.pXAudio2->CreateSourceVoice( &g_audioState.pSourceVoice, pwfx, 0,
                                                        XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL ) );

    // Create the custom APO instances
    CSimpleAPO* pSimpleAPO = NULL;
    CSimpleAPO::CreateInstance( NULL, 0, &pSimpleAPO );

    CMonitorAPO* pMonitorPre = NULL;
    CMonitorAPO::CreateInstance( NULL, 0, &pMonitorPre );

    CMonitorAPO* pMonitorPost = NULL;
    CMonitorAPO::CreateInstance( NULL, 0, &pMonitorPost );

    // Create the effect chain
    XAUDIO2_EFFECT_DESCRIPTOR apoDesc[3] = {0};
    apoDesc[0].InitialState = true;
    apoDesc[0].OutputChannels = 1;
    apoDesc[0].pEffect = static_cast<IXAPO*>(pMonitorPre);
    apoDesc[1].InitialState = true;
    apoDesc[1].OutputChannels = 1;
    apoDesc[1].pEffect = static_cast<IXAPO*>(pSimpleAPO);
    apoDesc[2].InitialState = true;
    apoDesc[2].OutputChannels = 1;
    apoDesc[2].pEffect = static_cast<IXAPO*>(pMonitorPost);

    XAUDIO2_EFFECT_CHAIN chain = {0};
    chain.EffectCount = sizeof(apoDesc) / sizeof(apoDesc[0]);
    chain.pEffectDescriptors = apoDesc;

    assert( g_audioState.pSourceVoice != NULL );
    V_RETURN( g_audioState.pSourceVoice->SetEffectChain( &chain ) );

    // Don't need to keep them now that XAudio2 has ownership
    pSimpleAPO->Release();
    pMonitorPre->Release();
    pMonitorPost->Release();

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = g_audioState.pbSampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = cbWaveSize;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    V_RETURN( g_audioState.pSourceVoice->SubmitSourceBuffer( &buffer ) );

    V_RETURN( g_audioState.pSourceVoice->Start( 0 ) );

    // Set the initial effect params
    g_audioState.simpleParams.gain = 1.0f;
    V_RETURN( g_audioState.pSourceVoice->SetEffectParameters( 1, &g_audioState.simpleParams, sizeof( SimpleAPOParams ) ) );

    g_audioState.pPipePre = new MonitorAPOPipe;
    g_audioState.pPipePost = new MonitorAPOPipe;

    MonitorAPOParams mparams;
    mparams.pipe = g_audioState.pPipePre;
    V_RETURN( g_audioState.pSourceVoice->SetEffectParameters( 0, &mparams, sizeof(mparams) ) );

    mparams.pipe = g_audioState.pPipePost;
    V_RETURN( g_audioState.pSourceVoice->SetEffectParameters( 2, &mparams, sizeof(mparams) ) );

    return S_OK;
}


//-----------------------------------------------------------------------------
// Set simple APO gain
//-----------------------------------------------------------------------------
VOID SetSimpleGain( float gain )
{
    if( !g_audioState.bInitialized )
        return;

    g_audioState.simpleParams.gain = gain;
    g_audioState.pSourceVoice->SetEffectParameters( 1, &g_audioState.simpleParams, sizeof( SimpleAPOParams ) );
}


//-----------------------------------------------------------------------------
// Pause audio playback
//-----------------------------------------------------------------------------
VOID PauseAudio( bool resume )
{
    if( !g_audioState.bInitialized )
        return;

    assert( g_audioState.pXAudio2 != NULL );

    if( resume )
        g_audioState.pXAudio2->StartEngine();
    else
        g_audioState.pXAudio2->StopEngine();
}



//-----------------------------------------------------------------------------
// Releases XAudio2
//-----------------------------------------------------------------------------
VOID CleanupAudio()
{
    if( !g_audioState.bInitialized )
        return;

    if( g_audioState.pSourceVoice )
    {
        g_audioState.pSourceVoice->DestroyVoice();
        g_audioState.pSourceVoice = NULL;
    }

    if( g_audioState.pMasteringVoice )
    {
        g_audioState.pMasteringVoice->DestroyVoice();
        g_audioState.pMasteringVoice = NULL;
    }

    if ( g_audioState.pXAudio2 )
        g_audioState.pXAudio2->StopEngine();

    SAFE_RELEASE( g_audioState.pXAudio2 );

    SAFE_DELETE_ARRAY( g_audioState.pbSampleData );

    SAFE_DELETE( g_audioState.pPipePre );
    SAFE_DELETE( g_audioState.pPipePost )

    CoUninitialize();

    g_audioState.bInitialized = false;
}
