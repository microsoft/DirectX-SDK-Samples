//--------------------------------------------------------------------------------------
// File: XAudio2BasicSound.cpp
//
// XNA Developer Connection
// (C) Copyright Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <xaudio2.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include "SDKwavefile.h"

//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//--------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------
HRESULT PlayPCM( IXAudio2* pXaudio2, LPCWSTR szFilename );
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename );


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int main()
{
    HRESULT hr;

    //
    // Initialize XAudio2
    //
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    IXAudio2* pXAudio2 = NULL;

    UINT32 flags = 0;
#ifdef _DEBUG
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    if( FAILED( hr = XAudio2Create( &pXAudio2, flags ) ) )
    {
        wprintf( L"Failed to init XAudio2 engine: %#X\n", hr );
        CoUninitialize();
        return 0;
    }

    //
    // Create a mastering voice
    //
    IXAudio2MasteringVoice* pMasteringVoice = NULL;

    if( FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice ) ) )
    {
        wprintf( L"Failed creating mastering voice: %#X\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Play a PCM wave file
    //
    wprintf( L"Playing mono WAV PCM file..." );
    if( FAILED( hr = PlayPCM( pXAudio2, L"Media\\Wavs\\MusicMono.wav" ) ) )
    {
        wprintf( L"Failed creating source voice: %#X\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Play a 5.1 PCM wave extensible file
    //
    wprintf( L"\nPlaying 5.1 WAV PCM file..." );
    if( FAILED( hr = PlayPCM( pXAudio2, L"Media\\Wavs\\MusicSurround.wav" ) ) )
    {
        wprintf( L"Failed creating source voice: %#X\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Cleanup XAudio2
    //
    wprintf( L"\nFinished playing\n" );

    // All XAudio2 interfaces are released when the engine is destroyed, but being tidy
    pMasteringVoice->DestroyVoice();

    SAFE_RELEASE( pXAudio2 );
    CoUninitialize();
}


//--------------------------------------------------------------------------------------
// Name: PlayPCM
// Desc: Plays a wave and blocks until the wave finishes playing
//--------------------------------------------------------------------------------------
HRESULT PlayPCM( IXAudio2* pXaudio2, LPCWSTR szFilename )
{
    HRESULT hr = S_OK;

    //
    // Locate the wave file
    //
    WCHAR strFilePath[MAX_PATH];
    if( FAILED( hr = FindMediaFileCch( strFilePath, MAX_PATH, szFilename ) ) )
    {
        wprintf( L"Failed to find media file: %s\n", szFilename );
        return hr;
    }

    //
    // Read in the wave file
    //
    CWaveFile wav;
    if( FAILED( hr = wav.Open( strFilePath, NULL, WAVEFILE_READ ) ) )
    {
        wprintf( L"Failed reading WAV file: %#X (%s)\n", hr, strFilePath );
        return hr;
    }

    // Get format of wave file
    WAVEFORMATEX* pwfx = wav.GetFormat();

    // Calculate how many bytes and samples are in the wave
    DWORD cbWaveSize = wav.GetSize();

    // Read the sample data into memory
    BYTE* pbWaveData = new BYTE[ cbWaveSize ];

    if( FAILED( hr = wav.Read( pbWaveData, cbWaveSize, &cbWaveSize ) ) )
    {
        wprintf( L"Failed to read WAV data: %#X\n", hr );
        SAFE_DELETE_ARRAY( pbWaveData );
        return hr;
    }

    //
    // Play the wave using a XAudio2SourceVoice
    //

    // Create the source voice
    IXAudio2SourceVoice* pSourceVoice;
    if( FAILED( hr = pXaudio2->CreateSourceVoice( &pSourceVoice, pwfx ) ) )
    {
        wprintf( L"Error %#X creating source voice\n", hr );
        SAFE_DELETE_ARRAY( pbWaveData );
        return hr;
    }

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = pbWaveData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = cbWaveSize;

    if( FAILED( hr = pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
    {
        wprintf( L"Error %#X submitting source buffer\n", hr );
        pSourceVoice->DestroyVoice();
        SAFE_DELETE_ARRAY( pbWaveData );
        return hr;
    }

    hr = pSourceVoice->Start( 0 );

    // Let the sound play
    BOOL isRunning = TRUE;
    while( SUCCEEDED( hr ) && isRunning )
    {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState( &state );
        isRunning = ( state.BuffersQueued > 0 ) != 0;

        // Wait till the escape key is pressed
        if( GetAsyncKeyState( VK_ESCAPE ) )
            break;

        Sleep( 10 );
    }

    // Wait till the escape key is released
    while( GetAsyncKeyState( VK_ESCAPE ) )
        Sleep( 10 );

    pSourceVoice->DestroyVoice();
    SAFE_DELETE_ARRAY( pbWaveData );

    return hr;
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
        wcscpy_s( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    wcscpy_s( strDestPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return S_OK;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    WCHAR strLeafName[MAX_PATH] = {0};
    wcscpy_s( strLeafName, MAX_PATH, strFilename );

    WCHAR strFullPath[MAX_PATH] = {0};
    WCHAR strFullFileName[MAX_PATH] = {0};
    WCHAR strSearch[MAX_PATH] = {0};
    WCHAR* strFilePart = NULL;

    GetFullPathName( L".", MAX_PATH, strFullPath, &strFilePart );
    if( strFilePart == NULL )
        return E_FAIL;

    while( strFilePart != NULL && *strFilePart != '\0' )
    {
        swprintf_s( strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            wcscpy_s( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        swprintf_s( strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName );
        if( GetFileAttributes( strFullFileName ) != 0xFFFFFFFF )
        {
            wcscpy_s( strDestPath, cchDest, strFullFileName );
            bFound = true;
            break;
        }

        swprintf_s( strSearch, MAX_PATH, L"%s\\..", strFullPath );
        GetFullPathName( strSearch, MAX_PATH, strFullPath, &strFilePart );
    }
    if( bFound )
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s( strDestPath, cchDest, strFilename );

    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}
