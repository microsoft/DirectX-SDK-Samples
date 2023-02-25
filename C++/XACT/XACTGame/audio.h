//--------------------------------------------------------------------------------------
// File: audio.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <xact3.h>
#include <xact3d3.h>

struct XACTGAME_3DCUE
{
    bool bActive;
    IXACT3Cue* pCue;
    D3DXVECTOR3 vEmitterPosition;
};

//-----------------------------------------------------------------------------
// Struct to hold audio game state
//-----------------------------------------------------------------------------
struct AUDIO_STATE
{
    CRITICAL_SECTION cs;
    bool bInitialized;
    bool bMusicPlaying;
    bool bUse3DAudio;

    // Cues
    XACTINDEX iAmmoBounce;
    XACTINDEX iAmmoFire;
    XACTINDEX iDroidDestroyed;
    XACTINDEX iDroidScan;
    XACTINDEX iBackgroundMusic;
    XACTINDEX iRoomRumble;

    // Global vars
    XACTINDEX iNumDroidDestroyedCueInstances;
    XACTINDEX iNumDroids;

    XACTVARIABLEVALUE fNumDroidDestroyedCueInstances;
    bool bHandleCueInstanceChange;

    XACTCATEGORY iMusicCategory;
    XACTCATEGORY iSoundFXCategory;
    float fSoundFXVolume;
    float fMusicVolume;

    // Banks
    IXACT3WaveBank* pWaveBank;
    IXACT3SoundBank* pSoundBank;

    // Engine
    IXACT3Engine* pEngine;

    // 3D
    X3DAUDIO_HANDLE x3DInstance;

    IXACT3Cue* pBackgroundMusic;
    IXACT3Cue* pRoomRumble;

    VOID* pbWaveBank; // Handle to wave bank data.  Its memory mapped so call UnmapViewOfFile() upon cleanup to release file
    VOID* pbSoundBank; // Pointer to sound bank data.  Call delete on it when the sound bank is destroyed

    int nFrameToApply3DAudio; // Counter to apply 3D audio occasionally
    X3DAUDIO_DSP_SETTINGS dspSettings;
    X3DAUDIO_LISTENER listener;
    X3DAUDIO_EMITTER emitter;
    FLOAT32 DelayTimes[2];
    FLOAT32 MatrixCoefficients[2 * 8];
    CGrowableArray <XACTGAME_3DCUE> v3DCues;
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern AUDIO_STATE g_audioState;


//--------------------------------------------------------------------------------------
// External functions
//--------------------------------------------------------------------------------------
HRESULT PrepareAudio();
HRESULT UpdateAudio();
HRESULT PlayAudioCue( XACTINDEX iCueIndex );
HRESULT Play3DAudioCue( XACTINDEX iCueIndex, D3DXVECTOR3* pvPosition );
void SetNumDroidsForAudio( int nDroidCount );
void SetMusicVolume( float fVolume );
void SetSoundFxVolume( float fVolume );
VOID CleanupAudio();
HRESULT PlayBGMusic();



