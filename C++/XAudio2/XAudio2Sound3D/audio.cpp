//--------------------------------------------------------------------------------------
// File: audio.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "SDKwavefile.h"
#include "audio.h"


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
AUDIO_STATE g_audioState;

// Specify sound cone to add directionality to listener for artistic effect:
// Emitters behind the listener are defined here to be more attenuated,
// have a lower LPF cutoff frequency,
// yet have a slightly higher reverb send level.
static const X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI*5.0f/6.0f, X3DAUDIO_PI*11.0f/6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };

// Specify LFE level distance curve such that it rolls off much sooner than
// all non-LFE channels, making use of the subwoofer more dramatic.
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

// Specify reverb send level distance curve such that reverb send increases
// slightly with distance before rolling off to silence.
// With the direct channels being increasingly attenuated with distance,
// this has the effect of increasing the reverb-to-direct sound ratio,
// reinforcing the perception of distance.
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

// Must match order of g_PRESET_NAMES
XAUDIO2FX_REVERB_I3DL2_PARAMETERS g_PRESET_PARAMS[ NUM_PRESETS ] =
{
    XAUDIO2FX_I3DL2_PRESET_FOREST,
    XAUDIO2FX_I3DL2_PRESET_DEFAULT,
    XAUDIO2FX_I3DL2_PRESET_GENERIC,
    XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
    XAUDIO2FX_I3DL2_PRESET_ROOM,
    XAUDIO2FX_I3DL2_PRESET_BATHROOM,
    XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
    XAUDIO2FX_I3DL2_PRESET_STONEROOM,
    XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
    XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
    XAUDIO2FX_I3DL2_PRESET_CAVE,
    XAUDIO2FX_I3DL2_PRESET_ARENA,
    XAUDIO2FX_I3DL2_PRESET_HANGAR,
    XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
    XAUDIO2FX_I3DL2_PRESET_HALLWAY,
    XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
    XAUDIO2FX_I3DL2_PRESET_ALLEY,
    XAUDIO2FX_I3DL2_PRESET_CITY,
    XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
    XAUDIO2FX_I3DL2_PRESET_QUARRY,
    XAUDIO2FX_I3DL2_PRESET_PLAIN,
    XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
    XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
    XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
    XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
    XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
    XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
    XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
    XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
    XAUDIO2FX_I3DL2_PRESET_PLATE,
};

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
    if( FAILED( hr = g_audioState.pXAudio2->CreateMasteringVoice( &g_audioState.pMasteringVoice ) ) )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        return hr;
    }

    // Check device details to make sure it's within our sample supported parameters
    XAUDIO2_DEVICE_DETAILS details;
    if( FAILED( hr = g_audioState.pXAudio2->GetDeviceDetails( 0, &details ) ) )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        return hr;
    }

    if( details.OutputFormat.Format.nChannels > OUTPUTCHANNELS )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        return E_FAIL;
    }

    g_audioState.dwChannelMask = details.OutputFormat.dwChannelMask;
    g_audioState.nChannels = details.OutputFormat.Format.nChannels;

    //
    // Create reverb effect
    //
    flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2FX_DEBUG;
#endif

    if( FAILED( hr = XAudio2CreateReverb( &g_audioState.pReverbEffect, flags ) ) )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        return hr;
    }

    //
    // Create a submix voice
    //

    // Performance tip: you need not run global FX with the sample number
    // of channels as the final mix.  For example, this sample runs
    // the reverb in mono mode, thus reducing CPU overhead.
    XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { g_audioState.pReverbEffect, TRUE, 1 } };
    XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

    if( FAILED( hr = g_audioState.pXAudio2->CreateSubmixVoice( &g_audioState.pSubmixVoice, 1,
                                                               details.OutputFormat.Format.nSamplesPerSec, 0, 0,
                                                               NULL, &effectChain ) ) )
    {
        SAFE_RELEASE( g_audioState.pXAudio2 );
        SAFE_RELEASE( g_audioState.pReverbEffect );
        return hr;
    }

    // Set default FX params
    XAUDIO2FX_REVERB_PARAMETERS native;
    ReverbConvertI3DL2ToNative( &g_PRESET_PARAMS[0], &native );
    g_audioState.pSubmixVoice->SetEffectParameters( 0, &native, sizeof( native ) );

    //
    // Initialize X3DAudio
    //  Speaker geometry configuration on the final mix, specifies assignment of channels
    //  to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask
    //
    //  SpeedOfSound - speed of sound in user-defined world units/second, used
    //  only for doppler calculations, it must be >= FLT_MIN
    //
    const float SPEEDOFSOUND = X3DAUDIO_SPEED_OF_SOUND;

    X3DAudioInitialize( details.OutputFormat.dwChannelMask, SPEEDOFSOUND, g_audioState.x3DInstance );

    g_audioState.vListenerPos = D3DXVECTOR3( 0, 0, 0 );
    g_audioState.vEmitterPos = D3DXVECTOR3( 0, 0, float( ZMAX ) );

    g_audioState.fListenerAngle = 0;
    g_audioState.fUseListenerCone = TRUE;
    g_audioState.fUseInnerRadius = TRUE;
    g_audioState.fUseRedirectToLFE = ((details.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY) != 0);

    //
    // Setup 3D audio structs
    //
    g_audioState.listener.Position = g_audioState.vListenerPos;
    g_audioState.listener.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    g_audioState.listener.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    g_audioState.listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

    g_audioState.emitter.pCone = &g_audioState.emitterCone;
    g_audioState.emitter.pCone->InnerAngle = 0.0f;
    // Setting the inner cone angles to X3DAUDIO_2PI and
    // outer cone other than 0 causes
    // the emitter to act like a point emitter using the
    // INNER cone settings only.
    g_audioState.emitter.pCone->OuterAngle = 0.0f;
    // Setting the outer cone angles to zero causes
    // the emitter to act like a point emitter using the
    // OUTER cone settings only.
    g_audioState.emitter.pCone->InnerVolume = 0.0f;
    g_audioState.emitter.pCone->OuterVolume = 1.0f;
    g_audioState.emitter.pCone->InnerLPF = 0.0f;
    g_audioState.emitter.pCone->OuterLPF = 1.0f;
    g_audioState.emitter.pCone->InnerReverb = 0.0f;
    g_audioState.emitter.pCone->OuterReverb = 1.0f;

    g_audioState.emitter.Position = g_audioState.vEmitterPos;
    g_audioState.emitter.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    g_audioState.emitter.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    g_audioState.emitter.ChannelCount = INPUTCHANNELS;
    g_audioState.emitter.ChannelRadius = 1.0f;
    g_audioState.emitter.pChannelAzimuths = g_audioState.emitterAzimuths;

    // Use of Inner radius allows for smoother transitions as
    // a sound travels directly through, above, or below the listener.
    // It also may be used to give elevation cues.
    g_audioState.emitter.InnerRadius = 2.0f;
    g_audioState.emitter.InnerRadiusAngle = X3DAUDIO_PI/4.0f;;

    g_audioState.emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
    g_audioState.emitter.pLFECurve    = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
    g_audioState.emitter.pLPFDirectCurve = NULL; // use default curve
    g_audioState.emitter.pLPFReverbCurve = NULL; // use default curve
    g_audioState.emitter.pReverbCurve    = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
    g_audioState.emitter.CurveDistanceScaler = 14.0f;
    g_audioState.emitter.DopplerScaler = 1.0f;

    g_audioState.dspSettings.SrcChannelCount = INPUTCHANNELS;
    g_audioState.dspSettings.DstChannelCount = g_audioState.nChannels;
    g_audioState.dspSettings.pMatrixCoefficients = g_audioState.matrixCoefficients;

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
    // Play the wave using a source voice that sends to both the submix and mastering voices
    //
    XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
    sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER; // LPF direct-path
    sendDescriptors[0].pOutputVoice = g_audioState.pMasteringVoice;
    sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
    sendDescriptors[1].pOutputVoice = g_audioState.pSubmixVoice;
    const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

    // create the source voice
    V_RETURN( g_audioState.pXAudio2->CreateSourceVoice( &g_audioState.pSourceVoice, pwfx, 0,
                                                        2.0f, NULL, &sendList ) );

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = g_audioState.pbSampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = cbWaveSize;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    V_RETURN( g_audioState.pSourceVoice->SubmitSourceBuffer( &buffer ) );

    V_RETURN( g_audioState.pSourceVoice->Start( 0 ) );

    g_audioState.nFrameToApply3DAudio = 0;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Perform per-frame update of audio
//-----------------------------------------------------------------------------
HRESULT UpdateAudio( float fElapsedTime )
{
    if( !g_audioState.bInitialized )
        return S_FALSE;

    if( g_audioState.nFrameToApply3DAudio == 0 )
    {
        // Calculate listener orientation in x-z plane
        if( g_audioState.vListenerPos.x != g_audioState.listener.Position.x
            || g_audioState.vListenerPos.z != g_audioState.listener.Position.z )
        {
            D3DXVECTOR3 vDelta = g_audioState.vListenerPos - g_audioState.listener.Position;

            g_audioState.fListenerAngle = float( atan2( vDelta.x, vDelta.z ) );

            vDelta.y = 0.0f;
            D3DXVec3Normalize( &vDelta, &vDelta );

            g_audioState.listener.OrientFront.x = vDelta.x;
            g_audioState.listener.OrientFront.y = 0.f;
            g_audioState.listener.OrientFront.z = vDelta.z;
        }

        if (g_audioState.fUseListenerCone)
        {
            g_audioState.listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;
        }
        else
        {
            g_audioState.listener.pCone = NULL;
        }
        if (g_audioState.fUseInnerRadius)
        {
            g_audioState.emitter.InnerRadius = 2.0f;
            g_audioState.emitter.InnerRadiusAngle = X3DAUDIO_PI/4.0f;
        }
        else
        {
            g_audioState.emitter.InnerRadius = 0.0f;
            g_audioState.emitter.InnerRadiusAngle = 0.0f;
        }

        if( fElapsedTime > 0 )
        {
            D3DXVECTOR3 lVelocity = ( g_audioState.vListenerPos - g_audioState.listener.Position ) / fElapsedTime;
            g_audioState.listener.Position = g_audioState.vListenerPos;
            g_audioState.listener.Velocity = lVelocity;

            D3DXVECTOR3 eVelocity = ( g_audioState.vEmitterPos - g_audioState.emitter.Position ) / fElapsedTime;
            g_audioState.emitter.Position = g_audioState.vEmitterPos;
            g_audioState.emitter.Velocity = eVelocity;
        }

        DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
            | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
            | X3DAUDIO_CALCULATE_REVERB;
        if (g_audioState.fUseRedirectToLFE)
        {
            // On devices with an LFE channel, allow the mono source data
            // to be routed to the LFE destination channel.
            dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
        }

        X3DAudioCalculate( g_audioState.x3DInstance, &g_audioState.listener, &g_audioState.emitter, dwCalcFlags,
                           &g_audioState.dspSettings );

        IXAudio2SourceVoice* voice = g_audioState.pSourceVoice;
        if( voice )
        {
            // Apply X3DAudio generated DSP settings to XAudio2
            voice->SetFrequencyRatio( g_audioState.dspSettings.DopplerFactor );
            voice->SetOutputMatrix( g_audioState.pMasteringVoice, INPUTCHANNELS, g_audioState.nChannels,
                                    g_audioState.matrixCoefficients );

            voice->SetOutputMatrix(g_audioState.pSubmixVoice, 1, 1, &g_audioState.dspSettings.ReverbLevel);

            XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * g_audioState.dspSettings.LPFDirectCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
            voice->SetOutputFilterParameters(g_audioState.pMasteringVoice, &FilterParametersDirect);
            XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * g_audioState.dspSettings.LPFReverbCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
            voice->SetOutputFilterParameters(g_audioState.pSubmixVoice, &FilterParametersReverb);
        }
    }

    g_audioState.nFrameToApply3DAudio++;
    g_audioState.nFrameToApply3DAudio &= 1;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Set reverb effect
//-----------------------------------------------------------------------------
HRESULT SetReverb( int nReverb )
{
    if( !g_audioState.bInitialized )
        return S_FALSE;

    if( nReverb < 0 || nReverb >= NUM_PRESETS )
        return E_FAIL;

    if( g_audioState.pSubmixVoice )
    {
        XAUDIO2FX_REVERB_PARAMETERS native;
        ReverbConvertI3DL2ToNative( &g_PRESET_PARAMS[ nReverb ], &native );
        g_audioState.pSubmixVoice->SetEffectParameters( 0, &native, sizeof( native ) );
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Pause audio playback
//-----------------------------------------------------------------------------
VOID PauseAudio( bool resume )
{
    if( !g_audioState.bInitialized )
        return;

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

    if( g_audioState.pSubmixVoice )
    {
        g_audioState.pSubmixVoice->DestroyVoice();
        g_audioState.pSubmixVoice = NULL;
    }

    if( g_audioState.pMasteringVoice )
    {
        g_audioState.pMasteringVoice->DestroyVoice();
        g_audioState.pMasteringVoice = NULL;
    }

    g_audioState.pXAudio2->StopEngine();
    SAFE_RELEASE( g_audioState.pXAudio2 );
    SAFE_RELEASE( g_audioState.pReverbEffect );

    SAFE_DELETE_ARRAY( g_audioState.pbSampleData );

    CoUninitialize();

    g_audioState.bInitialized = false;
}
