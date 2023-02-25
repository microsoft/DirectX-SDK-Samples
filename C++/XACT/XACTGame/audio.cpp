//--------------------------------------------------------------------------------------
// File: audio.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "mmreg.h"
#include <xact3.h>
#include <xact3d3.h>
#include "audio.h"


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern CFirstPersonCamera   g_Camera;

AUDIO_STATE                 g_audioState;
bool DoesCommandLineContainAuditionSwitch();
void WINAPI XACTNotificationCallback( const XACT_NOTIFICATION* pNotification );


//-----------------------------------------------------------------------------------------
// Preparing to use XACT is accomplished by doing the following:
//
//      1. Initialize XACT by calling XACTInitialize
//      2. Create the XACT wave bank(s) you want to use
//      3. Create the XACT sound bank(s) you want to use
//      4. Store indices to the XACT cue(s) your game uses
//-----------------------------------------------------------------------------------------
HRESULT PrepareAudio()
{
    HRESULT hr;
    WCHAR str[MAX_PATH];
    HANDLE hFile;
    DWORD dwFileSize;
    HANDLE hMapFile;

    // Clear struct
    ZeroMemory( &g_audioState, sizeof( AUDIO_STATE ) );
    g_audioState.bUse3DAudio = true;
    g_audioState.fMusicVolume = 1.0f;
    g_audioState.fSoundFXVolume = 1.0f;
    InitializeCriticalSection( &g_audioState.cs );

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );  // COINIT_APARTMENTTHREADED will work too
    if( SUCCEEDED( hr ) )
    {
        // Switch to auditioning mode based on command line.  Change if desired
        bool bAuditionMode = DoesCommandLineContainAuditionSwitch();
        bool bDebugMode = false;

        DWORD dwCreationFlags = 0;
        if( bAuditionMode ) dwCreationFlags |= XACT_FLAG_API_AUDITION_MODE;
        if( bDebugMode ) dwCreationFlags |= XACT_FLAG_API_DEBUG_MODE;

        hr = XACT3CreateEngine( dwCreationFlags, &g_audioState.pEngine );
    }
    if( FAILED( hr ) || g_audioState.pEngine == NULL )
        return E_FAIL;

    // Load the global settings file and pass it into XACTInitialize
    VOID* pGlobalSettingsData = NULL;
    DWORD dwGlobalSettingsFileSize = 0;
    bool bSuccess = false;
    DWORD dwBytesRead;
    if( SUCCEEDED( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"audio\\droid.xgs" ) ) )
    {
        hFile = CreateFile( str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if( hFile )
        {
            dwGlobalSettingsFileSize = GetFileSize( hFile, NULL );
            if( dwGlobalSettingsFileSize != INVALID_FILE_SIZE )
            {
                // Using CoTaskMemAlloc so that XACT can clean up this data when its done
                pGlobalSettingsData = CoTaskMemAlloc( dwGlobalSettingsFileSize );
                if( pGlobalSettingsData )
                {
                    if( 0 != ReadFile( hFile, pGlobalSettingsData, dwGlobalSettingsFileSize, &dwBytesRead, NULL ) )
                    {
                        bSuccess = true;
                    }
                }
            }
            CloseHandle( hFile );
        }
    }
    if( !bSuccess )
    {
        if( pGlobalSettingsData )
            CoTaskMemFree( pGlobalSettingsData );
        pGlobalSettingsData = NULL;
        dwGlobalSettingsFileSize = 0;
    }

    // Initialize & create the XACT runtime
    XACT_RUNTIME_PARAMETERS xrParams = {0};
    xrParams.pGlobalSettingsBuffer = pGlobalSettingsData;
    xrParams.globalSettingsBufferSize = dwGlobalSettingsFileSize;
    xrParams.globalSettingsFlags = XACT_FLAG_GLOBAL_SETTINGS_MANAGEDATA; // this will tell XACT to delete[] the buffer when its unneeded
    xrParams.fnNotificationCallback = XACTNotificationCallback;
    xrParams.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    hr = g_audioState.pEngine->Initialize( &xrParams );
    if( FAILED( hr ) )
        return hr;

    // Setup 3D audio structs
    ZeroMemory( &g_audioState.listener, sizeof( X3DAUDIO_LISTENER ) );
    g_audioState.listener.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    g_audioState.listener.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    g_audioState.listener.Position = D3DXVECTOR3( 0, 0, 0 );
    g_audioState.listener.Velocity = D3DXVECTOR3( 0, 0, 0 );

    ZeroMemory( &g_audioState.emitter, sizeof( X3DAUDIO_EMITTER ) );
    g_audioState.emitter.pCone = NULL;
    g_audioState.emitter.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    g_audioState.emitter.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    g_audioState.emitter.Position = D3DXVECTOR3( 0, 0, 0 );
    g_audioState.emitter.Velocity = D3DXVECTOR3( 0, 0, 0 );
    g_audioState.emitter.ChannelCount = 2;
    g_audioState.emitter.ChannelRadius = 1.0f;
    g_audioState.emitter.pChannelAzimuths = NULL;
    g_audioState.emitter.pVolumeCurve = NULL;
    g_audioState.emitter.pLFECurve = NULL;
    g_audioState.emitter.pLPFDirectCurve = NULL;
    g_audioState.emitter.pLPFReverbCurve = NULL;
    g_audioState.emitter.pReverbCurve = NULL;
    g_audioState.emitter.CurveDistanceScaler = 1.0f;
    g_audioState.emitter.DopplerScaler = NULL;

    g_audioState.DelayTimes[0] = 0.0f;
    g_audioState.DelayTimes[1] = 0.0f;

    // Init MatrixCoefficients. XACT will fill in the values
    ZeroMemory( &g_audioState.MatrixCoefficients, sizeof( FLOAT32 ) * 8 * 2 );

    // query number of channels on the final mix
    WAVEFORMATEXTENSIBLE wfxFinalMixFormat;
    hr = g_audioState.pEngine->GetFinalMixFormat( &wfxFinalMixFormat );
    if( FAILED( hr ) ) return hr;
    WORD nDstChannelCount = wfxFinalMixFormat.Format.nChannels;

    ZeroMemory( &g_audioState.dspSettings, sizeof( X3DAUDIO_DSP_SETTINGS ) );
    g_audioState.dspSettings.pMatrixCoefficients = g_audioState.MatrixCoefficients;
    g_audioState.dspSettings.pDelayTimes = g_audioState.DelayTimes;
    g_audioState.dspSettings.SrcChannelCount = 2;
    g_audioState.dspSettings.DstChannelCount = nDstChannelCount;

    // Initialize 3D settings
    hr = XACT3DInitialize( g_audioState.pEngine, g_audioState.x3DInstance );
    if( FAILED( hr ) )
        return E_FAIL;

    if( FAILED( hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"audio\\droid.xwb" ) ) )
        return hr;

    // Read and register the wave bank file with XACT using memory mapped file IO
    // Memory mapped files tend to be the fastest for most situations assuming you
    // have enough virtual address space for a full map of the file
    hr = E_FAIL; // assume failure
    hFile = CreateFile( str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwFileSize = GetFileSize( hFile, NULL );
        if( dwFileSize != -1 )
        {
            hMapFile = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, dwFileSize, NULL );
            if( hMapFile )
            {
                g_audioState.pbWaveBank = MapViewOfFile( hMapFile, FILE_MAP_READ, 0, 0, 0 );
                if( g_audioState.pbWaveBank )
                {
                    hr = g_audioState.pEngine->CreateInMemoryWaveBank( g_audioState.pbWaveBank, dwFileSize, 0,
                                                                       0, &g_audioState.pWaveBank );
                }
                CloseHandle( hMapFile ); // pbWaveBank is maintains a handle on the file so close this unneeded handle
            }
        }
        CloseHandle( hFile ); // pbWaveBank is maintains a handle on the file so close this unneeded handle
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    // Read and register the sound bank file with XACT.  Do not use memory mapped file IO because the
    // memory needs to be read/write and the working set of sound banks are small.
    if( FAILED( hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"audio\\droid.xsb" ) ) )
        return hr;
    hr = E_FAIL; // assume failure
    hFile = CreateFile( str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        dwFileSize = GetFileSize( hFile, NULL );
        if( dwFileSize != -1 )
        {
            // Allocate the data here and free the data when recieving the sound bank destroyed notification
            g_audioState.pbSoundBank = new BYTE[dwFileSize];
            if( g_audioState.pbSoundBank )
            {
                if( 0 != ReadFile( hFile, g_audioState.pbSoundBank, dwFileSize, &dwBytesRead, NULL ) )
                {
                    hr = g_audioState.pEngine->CreateSoundBank( g_audioState.pbSoundBank, dwFileSize, 0,
                                                                0, &g_audioState.pSoundBank );
                }
            }
        }
        CloseHandle( hFile );
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    // Get the sound cue index from the sound bank
    g_audioState.iAmmoBounce = g_audioState.pSoundBank->GetCueIndex( "ammo_bounce" );
    g_audioState.iAmmoFire = g_audioState.pSoundBank->GetCueIndex( "ammo_fire" );
    g_audioState.iDroidDestroyed = g_audioState.pSoundBank->GetCueIndex( "droid_destroyed" );
    g_audioState.iDroidScan = g_audioState.pSoundBank->GetCueIndex( "droid_scan" );
    g_audioState.iBackgroundMusic = g_audioState.pSoundBank->GetCueIndex( "techno_music" );
    g_audioState.iRoomRumble = g_audioState.pSoundBank->GetCueIndex( "room_rumble" );
    g_audioState.iNumDroidDestroyedCueInstances = g_audioState.pEngine->GetGlobalVariableIndex(
        "NumDroidDestroyedCueInstances" );
    g_audioState.iNumDroids = g_audioState.pEngine->GetGlobalVariableIndex( "NumDroids" );

    // Register for XACT notifications
    XACT_NOTIFICATION_DESCRIPTION desc = {0};
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED;
    desc.pSoundBank = g_audioState.pSoundBank;
    desc.cueIndex = g_audioState.iDroidDestroyed;
    hr = g_audioState.pEngine->RegisterNotification( &desc );
    if( FAILED( hr ) )
        return E_FAIL;

    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_CUESTOP;
    desc.cueIndex = XACTINDEX_INVALID;
    desc.pSoundBank = NULL;
    hr = g_audioState.pEngine->RegisterNotification( &desc );
    if( FAILED( hr ) )
        return E_FAIL;

    g_audioState.bInitialized = true;

    return S_OK;
}


//-----------------------------------------------------------------------------------------
// This is the callback for handling XACT notifications.  This callback can be executed on a
// different thread than the app thread so shared data must be thread safe.  The game
// also needs to minimize the amount of time spent in this callbacks to avoid glitching,
// and a limited subset of XACT API can be called from inside the callback so
// it is sometimes necessary to handle the notification outside of this callback.
//-----------------------------------------------------------------------------------------
void WINAPI XACTNotificationCallback( const XACT_NOTIFICATION* pNotification )
{
    // Use the critical section properly to make shared data thread safe while avoiding deadlocks.
    //
    // To do this follow this advice:
    // 1) Use a specific CS only to protect the specific shared data structures between the callback and the app thread.
    // 2) Don’t make any API calls while holding the CS. Use it to access the shared data, make a local copy of the data, release the CS and then make the API call.
    // 3) Spend minimal amount of time in the CS (to prevent the callback thread from waiting too long causing a glitch).
    //
    // Instead of using a CS, you can also use a non-blocking queues to keep track of notifications meaning
    // callback will push never pop only push and the app thread will only pop never push

    if( pNotification->type == XACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED &&
        pNotification->variable.pSoundBank == g_audioState.pSoundBank &&
        pNotification->variable.cueIndex == g_audioState.iDroidDestroyed &&
        pNotification->variable.variableIndex == 0 ) // NumCueInstances
    {
        // Can't call SetGlobalVariable() inside callback so process it ouside
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.fNumDroidDestroyedCueInstances = pNotification->variable.variableValue;
        g_audioState.bHandleCueInstanceChange = true;
        LeaveCriticalSection( &g_audioState.cs );
    }

    if( pNotification->type == XACTNOTIFICATIONTYPE_CUESTOP )
    {
        EnterCriticalSection( &g_audioState.cs );
        for( int i = 0; i < g_audioState.v3DCues.GetSize(); i++ )
        {
            if( g_audioState.v3DCues[i].pCue == pNotification->cue.pCue )
            {
                g_audioState.v3DCues[i].bActive = false;
                break;
            }
        }
        LeaveCriticalSection( &g_audioState.cs );
    }
}


//-----------------------------------------------------------------------------
// Calling XACTDoWork() allows XACT to do required periodic work.  This must be
// called frequently (such as once per frame).  In this tutorial since there
// is no 3D rendering taking place, we just call this while idle
//-----------------------------------------------------------------------------
HRESULT UpdateAudio()
{
    HRESULT hr;

    if( !g_audioState.bInitialized )
        return S_FALSE;

    EnterCriticalSection( &g_audioState.cs );
    bool bHandleCueInstanceChange = g_audioState.bHandleCueInstanceChange;
    XACTVARIABLEVALUE fNumDroidDestroyedCueInstances = g_audioState.fNumDroidDestroyedCueInstances;
    LeaveCriticalSection( &g_audioState.cs );

    if( bHandleCueInstanceChange )
    {
        g_audioState.pEngine->SetGlobalVariable( g_audioState.iNumDroidDestroyedCueInstances,
                                                 fNumDroidDestroyedCueInstances );

        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleCueInstanceChange = false;
        LeaveCriticalSection( &g_audioState.cs );
    }

    if( g_audioState.bUse3DAudio && g_audioState.nFrameToApply3DAudio == 0 )
    {
        g_audioState.listener.OrientFront = *g_Camera.GetWorldAhead();
        g_audioState.listener.OrientTop = *g_Camera.GetWorldUp();
        g_audioState.listener.Position = *g_Camera.GetEyePt();

        // Background music positioning
        g_audioState.emitter.Position = D3DXVECTOR3( 0, 0, 0 );
        V( XACT3DCalculate( g_audioState.x3DInstance, &g_audioState.listener, &g_audioState.emitter,
                            &g_audioState.dspSettings ) );
        V( XACT3DApply( &g_audioState.dspSettings, g_audioState.pBackgroundMusic ) );

        for( int i = 0; i < g_audioState.v3DCues.GetSize(); i++ )
        {
            if( g_audioState.v3DCues[i].bActive )
            {
                g_audioState.emitter.Position = g_audioState.v3DCues[i].vEmitterPosition;
                V( XACT3DCalculate( g_audioState.x3DInstance, &g_audioState.listener, &g_audioState.emitter,
                                    &g_audioState.dspSettings ) );
                V( XACT3DApply( &g_audioState.dspSettings, g_audioState.v3DCues[i].pCue ) );
            }
        }
    }
    g_audioState.nFrameToApply3DAudio++;
    g_audioState.nFrameToApply3DAudio %= 2;

    if( g_audioState.pEngine )
        g_audioState.pEngine->DoWork();

    return S_OK;
}


//-----------------------------------------------------------------------------
// Plays an XACT cue
//-----------------------------------------------------------------------------
HRESULT PlayBGMusic()
{
    if( !g_audioState.bInitialized )
        return S_FALSE;

    if( !g_audioState.bMusicPlaying )
    {
        g_audioState.bMusicPlaying = true;
        g_audioState.pSoundBank->Play( g_audioState.iBackgroundMusic, 0, 0, &g_audioState.pBackgroundMusic );
        g_audioState.pSoundBank->Play( g_audioState.iRoomRumble, 0, 0, &g_audioState.pRoomRumble );
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Plays an XACT cue as a fire and forget cue
//-----------------------------------------------------------------------------
HRESULT PlayAudioCue( XACTINDEX iCueIndex )
{
    if( !g_audioState.bInitialized )
        return S_FALSE;

    return g_audioState.pSoundBank->Play( iCueIndex, 0, 0, NULL );
}


//-----------------------------------------------------------------------------
// Plays an XACT cue as a 3D audio cue
//-----------------------------------------------------------------------------
HRESULT Play3DAudioCue( XACTINDEX iCueIndex, D3DXVECTOR3* pvPosition )
{
    if( !g_audioState.bInitialized )
        return S_FALSE;

    if( !g_audioState.bUse3DAudio )
        return PlayAudioCue( iCueIndex );

    // Play the cue as a 3D audio cue
    HRESULT hr;
    XACTGAME_3DCUE* pCue3D = NULL;

    EnterCriticalSection( &g_audioState.cs );
    for( int i = 0; i < g_audioState.v3DCues.GetSize(); i++ )
    {
        if( !g_audioState.v3DCues[i].bActive )
        {
            pCue3D = &g_audioState.v3DCues[i];
            break;
        }
    }
    if( pCue3D == NULL )
    {
        XACTGAME_3DCUE cue3D;
        g_audioState.v3DCues.Add( cue3D );
        pCue3D = &g_audioState.v3DCues[ g_audioState.v3DCues.GetSize() - 1];
    }
    pCue3D->bActive = true;
    LeaveCriticalSection( &g_audioState.cs );

    V( g_audioState.pSoundBank->Prepare( iCueIndex, 0, 0, &pCue3D->pCue ) );
    pCue3D->vEmitterPosition = *pvPosition;

    g_audioState.emitter.Position = pCue3D->vEmitterPosition;
    V( XACT3DCalculate( g_audioState.x3DInstance, &g_audioState.listener, &g_audioState.emitter,
                        &g_audioState.dspSettings ) );
    V( XACT3DApply( &g_audioState.dspSettings, pCue3D->pCue ) );

    return pCue3D->pCue->Play();
}


//-----------------------------------------------------------------------------
void SetNumDroidsForAudio( int nDroidCount )
{
    // Update global state
    if( g_audioState.pEngine )
    {
        g_audioState.pEngine->SetGlobalVariable( g_audioState.iNumDroids,
                                                 ( XACTVARIABLEVALUE )nDroidCount );
    }
}


//-----------------------------------------------------------------------------
void SetMusicVolume( float fVolume )
{
    if( g_audioState.bInitialized && g_audioState.pEngine )
    {
        g_audioState.iMusicCategory = g_audioState.pEngine->GetCategory( "Music" );
        g_audioState.pEngine->SetVolume( g_audioState.iMusicCategory, fVolume );
    }
}


//-----------------------------------------------------------------------------
void SetSoundFxVolume( float fVolume )
{
    if( g_audioState.bInitialized && g_audioState.pEngine )
    {
        g_audioState.iSoundFXCategory = g_audioState.pEngine->GetCategory( "Default" );
        g_audioState.pEngine->SetVolume( g_audioState.iSoundFXCategory, fVolume );
    }
}


//-----------------------------------------------------------------------------
// Releases all previously initialized XACT objects
//-----------------------------------------------------------------------------
VOID CleanupAudio()
{
    if( !g_audioState.bInitialized )
        return;

    // Shutdown XACT
    if( g_audioState.pEngine )
    {
        g_audioState.pEngine->ShutDown();
        SAFE_RELEASE( g_audioState.pEngine );
    }

    if( g_audioState.pbSoundBank )
        delete[] g_audioState.pbSoundBank;

    // Release memory mapped files
    if( g_audioState.pbWaveBank )
        UnmapViewOfFile( g_audioState.pbWaveBank );

    CoUninitialize();
    DeleteCriticalSection( &g_audioState.cs );
}


//--------------------------------------------------------------------------------------
bool DoesCommandLineContainAuditionSwitch()
{
    const WCHAR* strAuditioning = L"-audition"; size_t nArgLen; int nNumArgs;
    LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLine(), &nNumArgs );
    for( int iArg = 1; iArg < nNumArgs; iArg++ )
    {
        nArgLen = lstrlen( pstrArgList[iArg]);
        if( _wcsnicmp( pstrArgList[iArg], strAuditioning, nArgLen ) == 0 && nArgLen == 9 )
            return true;
    }
    LocalFree( pstrArgList );
    return false;
}


