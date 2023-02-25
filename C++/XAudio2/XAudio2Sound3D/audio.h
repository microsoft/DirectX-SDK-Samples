//--------------------------------------------------------------------------------------
// File: audio.h
//
// XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>

//-----------------------------------------------------------------------------
// Global defines
//-----------------------------------------------------------------------------
#define INPUTCHANNELS 1  // number of source channels
#define OUTPUTCHANNELS 8 // maximum number of destination channels supported in this sample

#define NUM_PRESETS 30

// Constants to define our world space
const INT           XMIN = -10;
const INT           XMAX = 10;
const INT           ZMIN = -10;
const INT           ZMAX = 10;

//-----------------------------------------------------------------------------
// Struct to hold audio game state
//-----------------------------------------------------------------------------
struct AUDIO_STATE
{
    bool bInitialized;

    // XAudio2
    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasteringVoice;
    IXAudio2SourceVoice* pSourceVoice;
    IXAudio2SubmixVoice* pSubmixVoice;
    IUnknown* pReverbEffect;
    BYTE* pbSampleData;

    // 3D
    X3DAUDIO_HANDLE x3DInstance;
    int nFrameToApply3DAudio;

    DWORD dwChannelMask;
    UINT32 nChannels;

    X3DAUDIO_DSP_SETTINGS dspSettings;
    X3DAUDIO_LISTENER listener;
    X3DAUDIO_EMITTER emitter;
    X3DAUDIO_CONE emitterCone;

    D3DXVECTOR3 vListenerPos;
    D3DXVECTOR3 vEmitterPos;
    float fListenerAngle;
    bool  fUseListenerCone;
    bool  fUseInnerRadius;
    bool  fUseRedirectToLFE;

    FLOAT32 emitterAzimuths[INPUTCHANNELS];
    FLOAT32 matrixCoefficients[INPUTCHANNELS * OUTPUTCHANNELS];
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern AUDIO_STATE  g_audioState;


//--------------------------------------------------------------------------------------
// External functions
//--------------------------------------------------------------------------------------
HRESULT InitAudio();
HRESULT PrepareAudio( const LPWSTR wavname );
HRESULT UpdateAudio( float fElapsedTime );
HRESULT SetReverb( int nReverb );
VOID PauseAudio( bool resume );
VOID CleanupAudio();
