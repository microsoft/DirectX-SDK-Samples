//-----------------------------------------------------------------------------
// File: Tut03_Variable.cpp
//
// Desc: This is the third tutorial for using the XACT API. This tutorial 
//       shows how to use categories and XACT variables.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------- ------------------------------------------------
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
VOID CleanupXACT();
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename );
bool DoesCommandLineContainAuditionSwitch();

//-----------------------------------------------------------------------------
// Struct to hold audio game state
//-----------------------------------------------------------------------------
struct AUDIO_STATE
{
    XACTINDEX iZap;
    XACTINDEX iEngine;
    XACTINDEX iSong;

    XACTCATEGORY iMusicCategory;
    XACTCATEGORY iGlobalCategory;
    XACTVARIABLEINDEX iRPMVariable;
    XACTVARIABLEVALUE nRPM;

    bool bMusicPaused;
    float fMusicVolume;

    bool bGlobalPaused;
    float fGlobalVolume;

    IXACT3Engine* pEngine;
    IXACT3SoundBank* pSoundBank;
    IXACT3WaveBank* pWaveBank;
    IXACT3Cue* pMusicCue;

    // Handles to audio files to be closed upon cleanup
    VOID* pbWaveBank; // Handle to wave bank data.  Its memory mapped so call UnmapViewOfFile() upon cleanup to release file
    VOID* pbSoundBank; // Pointer to sound bank data.  Call delete on it when the sound bank is destroyed
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
    g_hWnd = CreateWindow( L"XACTTutorial", L"XACT Tutorial 3: Variable",
                           WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
                           NULL, NULL, hInst, NULL );
    SetTimer( g_hWnd, 0, 100, NULL ); // repaint every so often -- just to avoid manually triggering when to repaint

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

    // Enter the message loop
    bool bGotMsg;
    MSG msg;
    msg.message = WM_NULL;

    while( WM_QUIT != msg.message )
    {
        // Use PeekMessage() so we can use idle time to render the scene and call XACTDoWork()
        bGotMsg = ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            //-----------------------------------------------------------------------------------------
            // It is important to allow XACT to do periodic work by calling XACTDoWork().  
            // However this must function be call often enough.  If you call it too infrequently, 
            // streaming will suffer and resources will not be managed promptly.  On the other hand 
            // if you call it too frequently, it will negatively affect performance. Calling it once 
            // per frame is usually a good balance.
            //
            // In this tutorial since there is no 3D rendering taking place, we just call this while 
            // idle and sleep afterward to yield CPU time
            //-----------------------------------------------------------------------------------------
            if( g_audioState.pEngine )
                g_audioState.pEngine->DoWork();
            Sleep( 10 ); // Yield CPU time to other apps.  Note that this is not normally needed when rendering
        }
    }

    // Clean up 
    UnregisterClass( L"XACT Tutorial", NULL );
    CleanupXACT();

    return 0;
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
    // TODO: handle any notification needed here.  Make sure global data is thread safe
}


//-----------------------------------------------------------------------------------------
// This function does the following steps:
//
//      1. Initialize XACT by calling pEngine->Initialize 
//      2. Create the XACT wave bank(s) you want to use
//      3. Create the XACT sound bank(s) you want to use
//      4. Store indices to the XACT cue(s) your game uses
//      5. Store indices to the XACT categories your game uses
//      6. Plays the back ground music that is stored in a in-memory wave bank
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
    g_audioState.fGlobalVolume = 1.0f;
    g_audioState.fMusicVolume = 0.3f;
    g_audioState.nRPM = 1000.0f;

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
    if( SUCCEEDED( FindMediaFileCch( str, MAX_PATH, L"Tut03_Variable.xgs" ) ) )
    {
        hFile = CreateFile( str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if( hFile )
        {
            dwGlobalSettingsFileSize = GetFileSize( hFile, NULL );
            if( dwGlobalSettingsFileSize != INVALID_FILE_SIZE )
            {
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

    if( FAILED( hr = FindMediaFileCch( str, MAX_PATH, L"Sounds.xwb" ) ) )
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
                g_audioState.pbWaveBank = MapViewOfFile( hMapFile, FILE_MAP_READ, 0, 0, 0 );
                if( g_audioState.pbWaveBank )
                {
                    hr = g_audioState.pEngine->CreateInMemoryWaveBank( g_audioState.pbWaveBank, dwFileSize, 0,
                                                                       0, &g_audioState.pWaveBank );
                }
                CloseHandle( hMapFile ); // pbWaveBank maintains a handle on the file so close this unneeded handle
            }
        }
        CloseHandle( hFile ); // pbWaveBank maintains a handle on the file so close this unneeded handle
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
        CloseHandle( hFile ); // pbWaveBank maintains a handle on the file so close this unneeded handle
    }
    if( FAILED( hr ) )
        return E_FAIL; // CleanupXACT() will cleanup state before exiting

    // Get the cue indices from the sound bank
    g_audioState.iZap = g_audioState.pSoundBank->GetCueIndex( "zap" );
    g_audioState.iEngine = g_audioState.pSoundBank->GetCueIndex( "engine" );
    g_audioState.iSong = g_audioState.pSoundBank->GetCueIndex( "song1" );

    // Get indices to XACT categories 
    g_audioState.iMusicCategory = g_audioState.pEngine->GetCategory( "Music" );
    g_audioState.iGlobalCategory = g_audioState.pEngine->GetCategory( "Global" );

    // Get indices to XACT variables
    g_audioState.iRPMVariable = g_audioState.pEngine->GetGlobalVariableIndex( "RPM" );

    g_audioState.pEngine->GetGlobalVariable( g_audioState.iRPMVariable, &g_audioState.nRPM );

    // Start playing the background music since it is in a in-memory wave bank
    g_audioState.pSoundBank->Play( g_audioState.iSong, 0, 0, NULL );
    g_audioState.pSoundBank->Play( g_audioState.iEngine, 0, 0, NULL );

    return S_OK;
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
            if( g_audioState.pEngine == NULL )
                break;

            // Upon a game event, play a cue.  For this simple tutorial, 
            // pressing the space bar is a game event that will play a cue
            if( wParam == VK_SPACE )
                g_audioState.pSoundBank->Play( g_audioState.iZap, 0, 0, NULL );

            // Pause or unpause the category upon keypress
            if( wParam == 'M' )
            {
                g_audioState.bMusicPaused = !g_audioState.bMusicPaused;
                g_audioState.pEngine->Pause( g_audioState.iMusicCategory, g_audioState.bMusicPaused );
            }
            if( wParam == 'P' )
            {
                g_audioState.bGlobalPaused = !g_audioState.bGlobalPaused;

                // When you unpause or pause a category, all child categories are also paused/unpaused.
                // All categories are a child of the "Global" category 
                g_audioState.bMusicPaused = g_audioState.bGlobalPaused;

                g_audioState.pEngine->Pause( g_audioState.iGlobalCategory, g_audioState.bGlobalPaused );
            }

            // Adjust the volume of the category
            if( wParam == 'J' )
            {
                g_audioState.fMusicVolume -= 0.05f;
                if( g_audioState.fMusicVolume < 0.0f )
                    g_audioState.fMusicVolume = 0.0f;
                g_audioState.pEngine->SetVolume( g_audioState.iMusicCategory, g_audioState.fMusicVolume );
            }
            if( wParam == 'K' )
            {
                g_audioState.fMusicVolume += 0.05f;
                if( g_audioState.fMusicVolume > 1.0f )
                    g_audioState.fMusicVolume = 1.0f;
                g_audioState.pEngine->SetVolume( g_audioState.iMusicCategory, g_audioState.fMusicVolume );
            }
            if( wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT )
            {
                g_audioState.fGlobalVolume -= 0.05f;
                if( g_audioState.fGlobalVolume < 0.0f )
                    g_audioState.fGlobalVolume = 0.0f;
                g_audioState.pEngine->SetVolume( g_audioState.iGlobalCategory, g_audioState.fGlobalVolume );
            }
            if( wParam == VK_OEM_PLUS || wParam == VK_ADD )
            {
                g_audioState.fGlobalVolume += 0.05f;
                if( g_audioState.fGlobalVolume > 1.0f )
                    g_audioState.fGlobalVolume = 1.0f;
                g_audioState.pEngine->SetVolume( g_audioState.iGlobalCategory, g_audioState.fGlobalVolume );
            }

            // Adjust the XACT variable based on some change 
            if( wParam == 'Q' )
            {
                g_audioState.nRPM -= 500.0f;
                if( g_audioState.nRPM < 500.0f )
                    g_audioState.nRPM = 500.0f;
                g_audioState.pEngine->SetGlobalVariable( g_audioState.iRPMVariable, g_audioState.nRPM );
            }
            if( wParam == 'W' )
            {
                g_audioState.nRPM += 500.0f;
                if( g_audioState.nRPM > 8000.0f )
                    g_audioState.nRPM = 8000.0f;
                g_audioState.pEngine->SetGlobalVariable( g_audioState.iRPMVariable, g_audioState.nRPM );
            }

            if( wParam == VK_ESCAPE )
                PostQuitMessage( 0 );
            break;
        }

        case WM_TIMER:
        {
            // Update message string every so often
            WCHAR sz[1024];
            StringCchPrintf( sz, 1024,
                             L"Press '-' and '+' to adjust the global volume level\n"
                             L"Press 'P' to pause the globally pause the sound\n"
                             L"Global volume: %0.1f%%, paused: %d\n"
                             L"\n"
                             L"Press 'J' and 'K' to adjust the background music volume level\n"
                             L"Press 'M' to pause the background music\n"
                             L"Background music volume: %0.1f%%, paused: %d\n"
                             L"\n"
                             L"Press space to play an XACT cue called 'zap' which plays\n"
                             L"from an in-memory wave bank\n"
                             L"\n"
                             L"Press 'Q' and 'W' to change the values of a custom global variable\n"
                             L"called 'RPM'. This variable is linked to an XACT RPC that is set to\n"
                             L"alter the pitch of the engine sound based on a ramp.\n"
                             L"RPM: %0.1f\n"
                             L"\n",
                             g_audioState.fGlobalVolume * 100.0f, g_audioState.bGlobalPaused,
                             g_audioState.fMusicVolume * 100.0f, g_audioState.bMusicPaused,
                             g_audioState.nRPM );

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
            rect.top = 30;
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
    if( g_audioState.pEngine )
    {
        g_audioState.pEngine->ShutDown();
        g_audioState.pEngine->Release();
    }

    // After XACTShutDown() returns, it is safe to release audio file memory
    if( g_audioState.pbSoundBank )
        delete[] g_audioState.pbSoundBank;
    g_audioState.pbSoundBank = NULL;

    if( g_audioState.pbWaveBank )
        UnmapViewOfFile( g_audioState.pbWaveBank );
    g_audioState.pbWaveBank = NULL;

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


