//--------------------------------------------------------------------------------------
// File: XACTCodeDriven.cpp
//
// This sample demonstrates the XACT code-driven API by showing
// how to create and play sounds without authoring a wave bank with the XACT tool
//
// (C) Copyright Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <xact3.h>
#include <strsafe.h>
#include <shellapi.h>
#include <conio.h>


//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//--------------------------------------------------------------------------------------
// Example file and properties. The properties must match the data in the file
//--------------------------------------------------------------------------------------
#define EXAMPLE_FILENAME    L"Example.wav"
#define EXAMPLE_SECONDS     2
#define EXAMPLE_SAMPLERATE  44100
#define EXAMPLE_CHANNELS    1
#define EXAMPLE_BITRATE     16

#define EXAMPLE2_FILENAME    L"Example2.wav"
#define EXAMPLE2_SECONDS     8
#define EXAMPLE2_SAMPLERATE  44100
#define EXAMPLE2_CHANNELS    2
#define EXAMPLE2_BITRATE     16


//--------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------
VOID DebugSpewV( LPCWSTR strMsg, ... );
bool DoesCommandLineContainAuditionSwitch();
HRESULT FindMediaFileCch( WCHAR* strDestPath, int cchDest, LPCWSTR strFilename );

HRESULT InitXACT( IXACT3Engine** ppXACTEngine );
HRESULT LoadWaveFileInMemory( IXACT3Engine* pXACTEngine, WCHAR* strFile, DWORD dwSeconds,
                              DWORD dwSampleRate, DWORD dwChannels, DWORD dwBits,
                              IXACT3Wave** ppXACTWave, BYTE** ppWaveBuffer );
HRESULT LoadWaveFileStreaming( IXACT3Engine* pXACTEngine, WCHAR* strFile, DWORD dwSeconds,
                               DWORD dwSampleRate, DWORD dwChannels, DWORD dwBits,
                               IXACT3Wave** ppXACTWave, HANDLE* phStreamingFile );
VOID CleanupXACT( IXACT3Engine* pXACTEngine );


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
    HRESULT hr;

    IXACT3Engine* pXACTEngine = NULL;
    IXACT3Wave* pXACTWave = NULL;
    BYTE* pWaveBuffer = NULL;
    HANDLE hStreamingFile = NULL;
    DWORD dwState = 0;

    // Init XACT 
    wprintf( L"Initializing XACT audio engine...\n\n" );
    if( FAILED( hr = InitXACT( &pXACTEngine ) ) )
    {
        wprintf( L"Failed to init XACT" );
        CleanupXACT( pXACTEngine );
        return 0;
    }

    // Load in-memory wave file
    wprintf( L"Loading %s into an in-memory IXACTWave interface...\n", EXAMPLE_FILENAME );
    if( FAILED( hr = LoadWaveFileInMemory( pXACTEngine, EXAMPLE_FILENAME, EXAMPLE_SECONDS,
                                           EXAMPLE_SAMPLERATE, EXAMPLE_CHANNELS, EXAMPLE_BITRATE,
                                           &pXACTWave, &pWaveBuffer ) ) )
    {
        if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            wprintf( L"Failed to load wave file because media not found\n" );
        else
            wprintf( L"Failed to load wave file" );
        CleanupXACT( pXACTEngine );
        return 0;
    }

    // Play in-memory wave file
    wprintf( L"Playing in-memory XACT wave... Press escape to stop\n\n" );
    if( FAILED( hr = pXACTWave->Play() ) )
    {
        wprintf( L"Failed to play XACT wave file\n" );
        CleanupXACT( pXACTEngine );
        return 0;
    }

    // Loop while allowing XACT to do work.  Rendering could be intermixed here
    dwState = 0;
    while( dwState != XACT_STATE_STOPPED )
    {
        pXACTEngine->DoWork();
        Sleep( 10 );
        pXACTWave->GetState( &dwState );

        if( GetAsyncKeyState( VK_ESCAPE ) )
            pXACTWave->Stop( XACT_FLAG_STOP_IMMEDIATE );
    }

    // Now that the in-memory wave has stopped playing, its safe to delete the memory
    SAFE_DELETE_ARRAY( pWaveBuffer );

    // Wait till the escape key is released
    while( GetAsyncKeyState( VK_ESCAPE ) )
        Sleep( 10 );

    // Load streaming wave file
    wprintf( L"Loading %s into a streaming IXACTWave interface...\n", EXAMPLE2_FILENAME );
    if( FAILED( hr = LoadWaveFileStreaming( pXACTEngine, EXAMPLE2_FILENAME, EXAMPLE2_SECONDS,
                                            EXAMPLE2_SAMPLERATE, EXAMPLE2_CHANNELS, EXAMPLE2_BITRATE,
                                            &pXACTWave, &hStreamingFile ) ) )
    {
        if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            wprintf( L"Failed to load wave file because media not found\n" );
        else
            wprintf( L"Failed to load wave file" );
        CleanupXACT( pXACTEngine );
        return 0;
    }

    // Call DoWork until the wave is fully prepared
    dwState = 0;
    while( dwState != XACT_STATE_PREPARED )
    {
        pXACTEngine->DoWork();
        Sleep( 10 );
        pXACTWave->GetState( &dwState );
    }

    // Play in-memory wave file
    wprintf( L"Playing streaming XACT wave... Press escape to stop\n\n" );
    if( FAILED( hr = pXACTWave->Play() ) )
    {
        wprintf( L"Failed to play XACT wave file\n" );
        CleanupXACT( pXACTEngine );
        return 0;
    }

    // Loop while allowing XACT to do work.  Rendering could be intermixed here
    dwState = 0;
    while( dwState != XACT_STATE_STOPPED )
    {
        pXACTEngine->DoWork();
        Sleep( 10 );
        pXACTWave->GetState( &dwState );

        if( GetAsyncKeyState( VK_ESCAPE ) )
            pXACTWave->Stop( XACT_FLAG_STOP_IMMEDIATE );
    }

    // Now that the streaming wave has stopped playing, its safe to close the file handle
    CloseHandle( hStreamingFile );

    // Cleanup XACT state
    wprintf( L"Finished playing XACT wave\n" );
    CleanupXACT( pXACTEngine );
}


//--------------------------------------------------------------------------------------
// Initialize XACT engine
//--------------------------------------------------------------------------------------
HRESULT InitXACT( IXACT3Engine** ppXACTEngine )
{
    // Init COM and create XACT engine
    HRESULT hr;
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( SUCCEEDED( hr ) )
    {
        // Switch to auditioning mode based on command line.  Change if desired
        bool bAuditionMode = DoesCommandLineContainAuditionSwitch();
        bool bDebugMode = false;

        DWORD dwCreationFlags = 0;
        if( bAuditionMode ) dwCreationFlags |= XACT_FLAG_API_AUDITION_MODE;
        if( bDebugMode ) dwCreationFlags |= XACT_FLAG_API_DEBUG_MODE;

        hr = XACT3CreateEngine( dwCreationFlags, ppXACTEngine );
    }
    if( FAILED( hr ) || *ppXACTEngine == NULL )
        return E_FAIL;

    XACT_RENDERER_DETAILS rendererDetails = {0};
    if( FAILED( hr = ( *ppXACTEngine )->GetRendererDetails( 0, &rendererDetails ) ) )
        return hr;

    XACT_RUNTIME_PARAMETERS xrParams = {0};
    xrParams.fnNotificationCallback = NULL;
    xrParams.lookAheadTime = XACT_ENGINE_LOOKAHEAD_DEFAULT;
    xrParams.pRendererID = rendererDetails.rendererID;

    if( FAILED( hr = ( *ppXACTEngine )->Initialize( &xrParams ) ) )
        return hr;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Load PCM wave data into a in-memory IXACT3Wave*
//--------------------------------------------------------------------------------------
HRESULT LoadWaveFileInMemory( IXACT3Engine* pXACTEngine, WCHAR* strFile, DWORD dwSeconds,
                              DWORD dwSampleRate, DWORD dwChannels, DWORD dwBits,
                              IXACT3Wave** ppXACTWave, BYTE** ppWaveBuffer )
{
    HRESULT hr;

    WCHAR strFilePath[MAX_PATH];
    if( FAILED( hr = FindMediaFileCch( strFilePath, MAX_PATH, strFile ) ) )
        return hr;

    // Open PCM audio data in file
    HANDLE hFile = CreateFile( strFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    if( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    DWORD dwBlockAlign = dwChannels * dwBits / 8;
    DWORD dwTotalNumBytes = dwSeconds * dwSampleRate * dwBlockAlign;

    // Initialize WAVEBANKENTRY struct
    WAVEBANKENTRY entry;
    entry.Format.wFormatTag = WAVEBANKMINIFORMAT_TAG_PCM;
    entry.Format.wBitsPerSample = ( dwBits == 16 ) ? WAVEBANKMINIFORMAT_BITDEPTH_16 : WAVEBANKMINIFORMAT_BITDEPTH_8;
    entry.Format.nChannels = dwChannels;
    entry.Format.wBlockAlign = dwChannels * ( dwBits / 8 );
    entry.Format.nSamplesPerSec = dwSampleRate;
    entry.Duration = dwSeconds * dwSampleRate;
    entry.LoopRegion.dwStartSample = 0;
    entry.LoopRegion.dwTotalSamples = 0;
    entry.PlayRegion.dwOffset = 0;
    entry.PlayRegion.dwLength = dwTotalNumBytes;
    entry.dwFlags = 0;

    BYTE* pXACTWaveBuffer = new BYTE[dwTotalNumBytes];
    DWORD dwBytesRead = 0;

    // Read PCM audio data from a file
    ReadFile( hFile, pXACTWaveBuffer, 44, &dwBytesRead, NULL );  // Skip wave format chunk
    ReadFile( hFile, pXACTWaveBuffer, dwTotalNumBytes, &dwBytesRead, NULL );
    if( dwBytesRead != dwTotalNumBytes )
    {
        SAFE_DELETE_ARRAY( pXACTWaveBuffer );
        CloseHandle( hFile );
        return E_FAIL;
    }
    CloseHandle( hFile );

    // Create an in-memory IXACTWave interface using wave file data
    if( FAILED( hr = pXACTEngine->PrepareInMemoryWave( XACT_FLAG_UNITS_MS, entry, NULL,
                                                       pXACTWaveBuffer, 0, 0, ppXACTWave ) ) )
    {
        SAFE_DELETE_ARRAY( pXACTWaveBuffer );
        return hr;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Load PCM wave data into a streaming IXACT3Wave*
//--------------------------------------------------------------------------------------
HRESULT LoadWaveFileStreaming( IXACT3Engine* pXACTEngine, WCHAR* strFile, DWORD dwSeconds,
                               DWORD dwSampleRate, DWORD dwChannels, DWORD dwBits,
                               IXACT3Wave** ppXACTWave, HANDLE* phStreamingFile )
{
    HRESULT hr;

    WCHAR strFilePath[MAX_PATH];
    if( FAILED( hr = FindMediaFileCch( strFilePath, MAX_PATH, strFile ) ) )
        return hr;

    // Open PCM audio data in file
    HANDLE hFile = CreateFile( strFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    if( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    DWORD dwBlockAlign = dwChannels * dwBits / 8;
    DWORD dwTotalNumBytes = dwSeconds * dwSampleRate * dwBlockAlign;

    // Initialize WAVEBANKENTRY struct
    WAVEBANKENTRY entry;
    entry.Format.wFormatTag = WAVEBANKMINIFORMAT_TAG_PCM;
    entry.Format.wBitsPerSample = ( dwBits == 16 ) ? WAVEBANKMINIFORMAT_BITDEPTH_16 : WAVEBANKMINIFORMAT_BITDEPTH_8;
    entry.Format.nChannels = dwChannels;
    entry.Format.wBlockAlign = dwChannels * ( dwBits / 8 );
    entry.Format.nSamplesPerSec = dwSampleRate;
    entry.Duration = dwSeconds * dwSampleRate;
    entry.LoopRegion.dwStartSample = 0;
    entry.LoopRegion.dwTotalSamples = 0;
    entry.PlayRegion.dwOffset = 0;
    entry.PlayRegion.dwLength = dwTotalNumBytes;
    entry.dwFlags = 0;

    // Load PCM into an streaming IXACTWave
    *phStreamingFile = hFile;

    XACT_STREAMING_PARAMETERS streamingParams;
    streamingParams.file = hFile;
    streamingParams.flags = 0;
    streamingParams.offset = 44;
    streamingParams.packetSize = ( WORD )dwBits;

    // Create an streaming IXACTWave interface using wave file data
    if( FAILED( hr = pXACTEngine->PrepareStreamingWave( XACT_FLAG_UNITS_MS, entry, streamingParams,
                                                        4096, NULL, 0, 0, ppXACTWave ) ) )
    {
        return hr;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Shutdown XACT 
//--------------------------------------------------------------------------------------
VOID CleanupXACT( IXACT3Engine* pXACTEngine )
{
    if( pXACTEngine )
    {
        pXACTEngine->ShutDown();
        pXACTEngine->Release();
    }
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

        StringCchPrintf( strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName );
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
// Returns true if the command line contains an -audition switch
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


//--------------------------------------------------------------------------------------
// Outputs to the debug stream a formatted Unicode string with a variable-argument list.
//--------------------------------------------------------------------------------------
VOID DebugSpewV( LPCWSTR strMsg, ... )
{
    WCHAR strBuffer[512];

    va_list args;
    va_start( args, strMsg );
    StringCchVPrintfW( strBuffer, 512, strMsg, args );
    strBuffer[511] = L'\0';
    va_end( args );

    OutputDebugString( strBuffer );
}


