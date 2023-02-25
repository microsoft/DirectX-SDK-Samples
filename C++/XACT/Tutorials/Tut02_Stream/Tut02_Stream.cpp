//-----------------------------------------------------------------------------
// File: Tut02_Stream.cpp
//
// Desc: This is the second tutorial for using the XACT API. This tutorial 
//       differs from the first tutorial by loading a streaming XACT wave bank 
//       and playing background music that is streamed from this wave bank.
//       It also shows how to do zero-latency streaming.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define _WIN32_DCOM
#include <windows.h>
#include <commdlg.h>
#include <xact3.h>
#include <shellapi.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )


//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
HRESULT PrepareXACT();
void UpdateAudio();
VOID CleanupXACT();
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
void WINAPI XACTNotificationCallback( const XACT_NOTIFICATION* pNotification );
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename );
bool DoesCommandLineContainAuditionSwitch();


//-----------------------------------------------------------------------------
// Struct to hold audio game state
//-----------------------------------------------------------------------------
struct AUDIO_STATE
{
    XACTINDEX iZap;
    XACTINDEX iRev;
    XACTINDEX iSong[3];

    IXACT3Engine* pEngine;
    IXACT3SoundBank* pSoundBank;
    IXACT3WaveBank* pInMemoryWaveBank;
    IXACT3WaveBank* pStreamingWaveBank;

    IXACT3Cue* pZeroLatencyRevCue;

    // Handles to audio files to be closed upon cleanup
    HANDLE hStreamingWaveBankFile;
    VOID* pbInMemoryWaveBank; // Handle to wave bank data.  Its memory mapped so call UnmapViewOfFile() upon cleanup to release file
    VOID* pbSoundBank; // Pointer to sound bank data.  Call delete on it when the sound bank is destroyed

    CRITICAL_SECTION cs;
    bool bHandleStreamingWaveBankPrepared;
    bool bHandleZeroLatencyCueStop;
    bool bHandleSongStopped;

    int nCurSongPlaying;
};

AUDIO_STATE g_audioState;
HWND        g_hWnd;


//-----------------------------------------------------------------------------------------
// This tutorial does the follow XACT related steps: 
//
//      1. Prepare to use XACT
//      2. Start playing background music when the streaming wave bank is prepared
//      3. Allow XACT to do work periodically when the message pump is idle
//      4. Plays sounds using XACT upon a user event
//      5. XACT shutdown and cleanup 
//
// We will look at each of these steps in detail below.
//-----------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
{
    // Register the window class
    HBRUSH hBrush = CreateSolidBrush( 0xFF0000 );
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), 0, MsgProc, 0L, 0L, hInst, NULL,
        LoadCursor( NULL, IDC_ARROW ), hBrush,
        NULL, L"XACTTutorial", NULL
    };
    RegisterClassEx( &wc );

    // Create the application's window
    g_hWnd = CreateWindow( L"XACTTutorial", L"XACT Tutorial 2: Streaming",
                           WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
                           NULL, NULL, hInst, NULL );
    SetTimer( g_hWnd, 0, 100, NULL ); // update the text every so often

    // Prepare to use XACT
    HRESULT hr;
    if( FAILED( hr = PrepareXACT() ) )
    {
        if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            MessageBox( g_hWnd, L"Failed to init XACT because media not found.", L"XACT Tutorial", MB_OK );
        else
            MessageBox( g_hWnd, L"Failed to init XACT.", L"XACT Tutorial", MB_OK );
        CleanupXACT();
        return 0;
    }

    // Book keeping - no song currently playing.
    g_audioState.nCurSongPlaying = -1;

    // Enter the message loop
    bool bGotMsg;
    MSG msg;
    msg.message = WM_NULL;

    while( WM_QUIT != msg.message )
    {
        // Use PeekMessage() so we can use idle time to render the scene and call pEngine->DoWork()
        bGotMsg = ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            UpdateAudio();

            Sleep( 10 ); // Yield CPU time to other apps.  Note that this is not normally needed when rendering
        }
    }

    // Clean up 
    UnregisterClass( L"XACT Tutorial", NULL );
    CleanupXACT();

    return 0;
}


//-----------------------------------------------------------------------------------------
// This function does the following steps:
//
//      1. Initialize XACT by calling pEngine->Initialize 
//      2. Register for the XACT notification desired
//      3. Create the in memory XACT wave bank(s) you want to use
//      4. Create the streaming XACT wave bank(s) you want to use
//      5. Create the XACT sound bank(s) you want to use
//      6. Store indices to the XACT cue(s) your game uses
//-----------------------------------------------------------------------------------------
HRESULT PrepareXACT()
{
    HRESULT hr;
    WCHAR str[MAX_PATH];
    HANDLE hFile;
    DWORD dwFileSize;
    DWORD dwBytesRead;
    HANDLE hMapFile;

    // Clear struct
    ZeroMemory( &g_audioState, sizeof( AUDIO_STATE ) );
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

    // Initialize & create the XACT runtime 
    XACT_RUNTIME_PARAMETERS xrParams = {0};
    xrParams.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    xrParams.fnNotificationCallback = XACTNotificationCallback;
    hr = g_audioState.pEngine->Initialize( &xrParams );
    if( FAILED( hr ) )
        return hr;

    //-----------------------------------------------------------------------------------------
    // Register for XACT notifications
    //-----------------------------------------------------------------------------------------

    // The "wave bank prepared" notification will let the app know when it is save to use
    // play cues that reference streaming wave data.
    XACT_NOTIFICATION_DESCRIPTION desc = {0};
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_WAVEBANKPREPARED;
    g_audioState.pEngine->RegisterNotification( &desc );

    // The "sound bank destroyed" notification will let the app know when it is save to use
    // play cues that reference streaming wave data.
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED;
    g_audioState.pEngine->RegisterNotification( &desc );

    // The "cue stop" notification will let the app know when it a song stops so a new one 
    // can be played
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_CUESTOP;
    desc.cueIndex = XACTINDEX_INVALID;
    g_audioState.pEngine->RegisterNotification( &desc );

    // The "cue prepared" notification will let the app know when it a a cue that uses 
    // streaming data has been prepared so it is ready to be used for zero latency streaming
    desc.flags = XACT_FLAG_NOTIFICATION_PERSIST;
    desc.type = XACTNOTIFICATIONTYPE_CUEPREPARED;
    desc.cueIndex = XACTINDEX_INVALID;
    g_audioState.pEngine->RegisterNotification( &desc );


    if( FAILED( hr = FindMediaFileCch( str, MAX_PATH, L"InMemoryWaveBank.xwb" ) ) )
        return hr;

    // Create an "in memory" XACT wave bank file using memory mapped file IO
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
                g_audioState.pbInMemoryWaveBank = MapViewOfFile( hMapFile, FILE_MAP_READ, 0, 0, 0 );
                if( g_audioState.pbInMemoryWaveBank )
                {
                    hr = g_audioState.pEngine->CreateInMemoryWaveBank( g_audioState.pbInMemoryWaveBank, dwFileSize, 0,
                                                                       0, &g_audioState.pInMemoryWaveBank );
                }
                CloseHandle( hMapFile ); // pbInMemoryWaveBank maintains a handle on the file so close this unneeded handle
            }
        }
        CloseHandle( hFile ); // pbInMemoryWaveBank maintains a handle on the file so close this unneeded handle
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    //-----------------------------------------------------------------------------------------
    // Create a streaming XACT wave bank file.  
    // Take note of the following:
    // 1) This wave bank in the XACT project file must marked as a streaming wave bank 
    //    This is set inside the XACT authoring tool)
    // 2) Use FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING flags when opening the file 
    // 3) To use cues that reference this streaming wave bank, you must wait for the 
    //    wave bank to prepared first or the playing the cue will fail
    //-----------------------------------------------------------------------------------------
    if( FAILED( hr = FindMediaFileCch( str, MAX_PATH, L"StreamingWaveBank.xwb" ) ) )
        return hr;
    hr = E_FAIL; // assume failure
    g_audioState.hStreamingWaveBankFile = CreateFile( str,
                                                      GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                                      FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    if( g_audioState.hStreamingWaveBankFile != INVALID_HANDLE_VALUE )
    {
        XACT_WAVEBANK_STREAMING_PARAMETERS wsParams;
        ZeroMemory( &wsParams, sizeof( XACT_WAVEBANK_STREAMING_PARAMETERS ) );
        wsParams.file = g_audioState.hStreamingWaveBankFile;
        wsParams.offset = 0;

        // 64 means to allocate a 64 * 2k buffer for streaming.  
        // This is a good size for DVD streaming and takes good advantage of the read ahead cache
        wsParams.packetSize = 64;

        hr = g_audioState.pEngine->CreateStreamingWaveBank( &wsParams, &g_audioState.pStreamingWaveBank );
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    // Create the XACT sound bank file with using memory mapped file IO
    if( FAILED( hr = FindMediaFileCch( str, MAX_PATH, L"sounds.xsb" ) ) )
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
        CloseHandle( hFile ); // pbInMemoryWaveBank maintains a handle on the file so close this unneeded handle
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    // Get the cue indices from the sound bank
    g_audioState.iZap = g_audioState.pSoundBank->GetCueIndex( "zap" );
    g_audioState.iRev = g_audioState.pSoundBank->GetCueIndex( "rev" );
    for( int i = 0; i < 3; i++ )
    {
        CHAR sz[256];
        StringCchPrintfA( sz, 256, "song%d", i + 1 );
        g_audioState.iSong[i] = g_audioState.pSoundBank->GetCueIndex( sz );
    }

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

    // In this simple tutorial, we will respond to a cue stop notification for the song 
    // cues by simply playing another song but its ultimately it's up the application 
    // and sound designer to decide what to do when a notification is received. 
    if( pNotification->type == XACTNOTIFICATIONTYPE_CUESTOP &&
        ( pNotification->cue.cueIndex == g_audioState.iSong[0] ||
          pNotification->cue.cueIndex == g_audioState.iSong[1] ||
          pNotification->cue.cueIndex == g_audioState.iSong[2] ) )
    {
        // The previous background song ended, so pick and new song to play it
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.nCurSongPlaying++; g_audioState.nCurSongPlaying %= 3;
        g_audioState.bHandleSongStopped = true;
        LeaveCriticalSection( &g_audioState.cs );
    }

    if( pNotification->type == XACTNOTIFICATIONTYPE_WAVEBANKPREPARED &&
        pNotification->waveBank.pWaveBank == g_audioState.pStreamingWaveBank )
    {
        // Respond to this notification outside of this callback so Prepare() can be called
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleStreamingWaveBankPrepared = true;
        LeaveCriticalSection( &g_audioState.cs );
    }

    if( pNotification->type == XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED &&
        pNotification->soundBank.pSoundBank == g_audioState.pSoundBank )
    {
        // Cleanup sound bank memory
        if( g_audioState.pbSoundBank )
        {
            delete[] g_audioState.pbSoundBank;
            g_audioState.pbSoundBank = NULL;
        }
    }

    if( pNotification->type == XACTNOTIFICATIONTYPE_CUEPREPARED &&
        pNotification->cue.pCue == g_audioState.pZeroLatencyRevCue )
    {
        // No need to handle this notification here but its 
        // done so for demonstration purposes.  This is typically useful
        // for triggering animation as soon as zero-latency audio is prepared 
        // to ensure the audio and animation are in sync
    }

    if( pNotification->type == XACTNOTIFICATIONTYPE_CUESTOP &&
        pNotification->cue.pCue == g_audioState.pZeroLatencyRevCue )
    {
        // Respond to this notification outside of this callback so Prepare() can be called
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleZeroLatencyCueStop = true;
        LeaveCriticalSection( &g_audioState.cs );
    }
}


//-----------------------------------------------------------------------------------------
// Handle these notifications outside of the callback and call pEngine->DoWork()
//-----------------------------------------------------------------------------------------
void UpdateAudio()
{
    // Handle these notifications outside of the callback because
    // only a subset of XACT APIs can be called inside the callback.

    // Use the critical section properly to make shared data thread safe while avoiding deadlocks.  
    //
    // To do this follow this advice:
    // 1) Use a specific CS only to protect the specific shared data structures between the callback and the app thread.
    // 2) Don’t make any API calls while holding the CS. Use it to access the shared data, make a local copy of the data, release the CS and then make the API call.
    // 3) Spend minimal amount of time in the CS (to prevent the callback thread from waiting too long causing a glitch).   
    // 
    // Instead of using a CS, you can also use a non-blocking queues to keep track of notifications meaning 
    // callback will push never pop only push and the app thread will only pop never push

    EnterCriticalSection( &g_audioState.cs );
    bool bHandleStreamingWaveBankPrepared = g_audioState.bHandleStreamingWaveBankPrepared;
    bool bHandleZeroLatencyCueStop = g_audioState.bHandleZeroLatencyCueStop;
    LeaveCriticalSection( &g_audioState.cs );

    if( bHandleStreamingWaveBankPrepared )
    {
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleStreamingWaveBankPrepared = false;
        LeaveCriticalSection( &g_audioState.cs );

        // Starting playing background music after the streaming wave bank 
        // has been prepared but no sooner.  The background music does not need to be 
        // zero-latency so the cues do not need to be prepared first 
        g_audioState.nCurSongPlaying = 0;
        g_audioState.pSoundBank->Play( g_audioState.iSong[g_audioState.nCurSongPlaying], 0, 0, NULL );

        // Prepare a new cue for zero-latency playback now that the wave bank is prepared
        g_audioState.pSoundBank->Prepare( g_audioState.iRev, 0, 0, &g_audioState.pZeroLatencyRevCue );
    }

    if( bHandleZeroLatencyCueStop )
    {
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleZeroLatencyCueStop = false;
        LeaveCriticalSection( &g_audioState.cs );

        // Destroy the cue when it stops
        g_audioState.pZeroLatencyRevCue->Destroy();
        g_audioState.pZeroLatencyRevCue = NULL;

        // For this tutorial, we will prepare another zero-latency cue
        // after the current one stops playing, but this isn't typical done
        // Its up to the application to define its own behavior
        g_audioState.pSoundBank->Prepare( g_audioState.iRev, 0, 0, &g_audioState.pZeroLatencyRevCue );
    }

    if( g_audioState.bHandleSongStopped )
    {
        EnterCriticalSection( &g_audioState.cs );
        g_audioState.bHandleSongStopped = false;
        LeaveCriticalSection( &g_audioState.cs );

        g_audioState.pSoundBank->Play( g_audioState.iSong[g_audioState.nCurSongPlaying], 0, 0, NULL );
    }


    // It is important to allow XACT to do periodic work by calling pEngine->DoWork().  
    // However this must function be call often enough.  If you call it too infrequently, 
    // streaming will suffer and resources will not be managed promptly.  On the other hand 
    // if you call it too frequently, it will negatively affect performance. Calling it once 
    // per frame is usually a good balance.
    if( g_audioState.pEngine )
        g_audioState.pEngine->DoWork();
}


//-----------------------------------------------------------------------------
// Window message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static WCHAR s_szMessage[1024] = {0};

    switch( msg )
    {
        case WM_KEYDOWN:
        {
            // Upon a game event, play a cue.  For this simple tutorial, 
            // pressing the space bar is a game event that will play a cue
            if( wParam == VK_SPACE )
                g_audioState.pSoundBank->Play( g_audioState.iZap, 0, 0, NULL );

            // To play a zero-latency cue: 
            // 1) prepare it
            // 2) wait for it to be prepared
            // 3) play it with using the cue instance returned from the Prepare() function.
            if( wParam == 'A' )
            {
                // The Play() call on a cue will only succeed if the cue is either preparing or 
                // prepared meaning it can not be playing, etc.
                DWORD dwState;
                g_audioState.pZeroLatencyRevCue->GetState( &dwState );
                if( ( dwState & ( XACT_CUESTATE_PREPARING | XACT_CUESTATE_PREPARED ) ) != 0 )
                    g_audioState.pZeroLatencyRevCue->Play();
            }

            if( wParam == VK_ESCAPE )
                PostQuitMessage( 0 );
            break;
        }

        case WM_TIMER:
        {
            // Update message string every so often
            WCHAR szState[512];
            if( g_audioState.pZeroLatencyRevCue )
            {
                DWORD dwState;
                g_audioState.pZeroLatencyRevCue->GetState( &dwState );
                switch( dwState )
                {
                    case XACT_CUESTATE_CREATED:
                        StringCchCopy( szState, 512, L"Created, but nothing else" ); break;
                    case XACT_CUESTATE_PREPARING:
                        StringCchCopy( szState, 512, L"Preparing to play" ); break;
                    case XACT_CUESTATE_PREPARED:
                        StringCchCopy( szState, 512, L"Prepared, but not yet played" ); break;
                    case XACT_CUESTATE_PLAYING:
                        StringCchCopy( szState, 512, L"Playing, but can be paused" ); break;
                    case XACT_CUESTATE_STOPPING:
                        StringCchCopy( szState, 512, L"Stopping" ); break;
                    case XACT_CUESTATE_STOPPED:
                        StringCchCopy( szState, 512, L"Stopped" ); break;
                    case XACT_CUESTATE_PAUSED:
                        StringCchCopy( szState, 512, L"Paused" ); break;
                }
            }
            else
            {
                StringCchCopy( szState, 512, L"Not created" );
            }

            EnterCriticalSection( &g_audioState.cs );
            WCHAR sz[512];
            StringCchPrintf( sz, 512,
                             L"Press space to play an XACT cue called 'zap' which plays\n"
                             L"from an in-memory wave bank\n"
                             L"\n"
                             L"Press 'A' to play a zero-latency cue when it is preparing or prepared.\n"
                             L"When this cue stops, the tutorial releases it and prepares a new cue\n"
                             L"Cue state: %s\n"
                             L"\n"
                             L"This tutorial is also playing background music in that is\n"
                             L"contained in a streaming wave bank.  When the background\n"
                             L"music stops, a new song cue is played by the tutorial.\n"
                             L"Currently playing: Song %d\n", szState, g_audioState.nCurSongPlaying + 1 );
            LeaveCriticalSection( &g_audioState.cs );

            if( wcscmp( sz, s_szMessage ) != 0 )
            {
                // Repaint the window if needed 
                StringCchCopy( s_szMessage, 1024, sz );
                InvalidateRect( g_hWnd, NULL, TRUE );
                UpdateWindow( g_hWnd );
            }
            break;
        }

        case WM_PAINT:
        {
            // Paint some simple explanation text
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint( hWnd, &ps );
            SetBkColor( hDC, 0xFF0000 );
            SetTextColor( hDC, 0xFFFFFF );
            RECT rect;
            GetClientRect( hWnd, &rect );
            rect.top = 70;
            DrawText( hDC, s_szMessage, -1, &rect, DT_CENTER );
            EndPaint( hWnd, &ps );
            return 0;
        }

        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            break;
        }
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Releases all previously initialized XACT objects
//-----------------------------------------------------------------------------
VOID CleanupXACT()
{
    // Shutdown XACT
    //
    // Note that XACTShutDown is synchronous and will take some time to complete 
    // if there are still playing cues.  Also XACTShutDown() is generally only 
    // called when a game exits and is not the preferred method of changing audio 
    // resources. To know when it is safe to free wave/sound bank data without 
    // shutting down the XACT engine, use the XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED 
    // or XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED notifications 
    if( g_audioState.pEngine )
    {
        g_audioState.pEngine->ShutDown();
        g_audioState.pEngine->Release();
    }

    // After pEngine->ShutDown() returns, it is safe to release audio file memory
    if( g_audioState.hStreamingWaveBankFile != INVALID_HANDLE_VALUE && g_audioState.hStreamingWaveBankFile != NULL )
        CloseHandle( g_audioState.hStreamingWaveBankFile );

    if( g_audioState.pbSoundBank )
        delete[] g_audioState.pbSoundBank;
    g_audioState.pbSoundBank = NULL;

    if( g_audioState.pbInMemoryWaveBank )
        UnmapViewOfFile( g_audioState.pbInMemoryWaveBank );
    g_audioState.pbInMemoryWaveBank = NULL;

    DeleteCriticalSection( &g_audioState.cs );

    CoUninitialize();
}


//--------------------------------------------------------------------------------------
// Helper function to try to find the location of a media file
//--------------------------------------------------------------------------------------
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename )
{
    bool bFound = false;

    if( NULL == strFilename || strFilename[0] == 0 || NULL == strDestPath || cchDest < 10 )
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = {0};
    WCHAR strExeName[MAX_PATH] = {0};
    WCHAR* strLastSlash = NULL;
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        StringCchCopy( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    StringCchCopy( strDestPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return S_OK;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    WCHAR strLeafName[MAX_PATH] = {0};
    StringCchCopy( strLeafName, MAX_PATH, strFilename );

    WCHAR strFullPath[MAX_PATH] = {0};
    WCHAR strFullFileName[MAX_PATH] = {0};
    WCHAR strSearch[MAX_PATH] = {0};
    WCHAR* strFilePart = NULL;

    GetFullPathName( L".", MAX_PATH, strFullPath, &strFilePart );
    if( strFilePart == NULL )
        return E_FAIL;

    while( strFilePart != NULL && *strFilePart != '\0' )
    {
        StringCchPrintf( strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            StringCchCopy( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        StringCchPrintf( strFullFileName, MAX_PATH, L"%s\\Tutorials\\%s\\%s", strFullPath, strExeName, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            StringCchCopy( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        StringCchPrintf( strSearch, MAX_PATH, L"%s\\..", strFullPath );
        GetFullPathName( strSearch, MAX_PATH, strFullPath, &strFilePart );
    }
    if( bFound )
        return S_OK;

    // On failure, return the file as the path but also return an error code
    StringCchCopy( strDestPath, cchDest, strFilename );

    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}


//--------------------------------------------------------------------------------------
bool DoesCommandLineContainAuditionSwitch()
{
    const WCHAR* strAuditioning = L"-audition"; size_t nArgLen; int nNumArgs;
    LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLine(), &nNumArgs );
    for( int iArg = 1; iArg < nNumArgs; iArg++ )
    {
        StringCchLength( pstrArgList[iArg], 256, &nArgLen );
        if( _wcsnicmp( pstrArgList[iArg], strAuditioning, nArgLen ) == 0 && nArgLen == 9 )
            return true;
    }
    LocalFree( pstrArgList );
    return false;
}


