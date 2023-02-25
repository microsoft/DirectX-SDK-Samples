//--------------------------------------------------------------------------------------
// File: audio.h
//
// XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>

#include "SimpleAPO.h"
#include "MonitorAPO.h"

//-----------------------------------------------------------------------------
// Global defines
//-----------------------------------------------------------------------------

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
    BYTE* pbSampleData;

    // APOs
    SimpleAPOParams simpleParams;
    MonitorAPOPipe *pPipePre;
    MonitorAPOPipe *pPipePost;
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
VOID SetSimpleGain( float gain );
VOID PauseAudio( bool resume );
VOID CleanupAudio();
