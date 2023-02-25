//--------------------------------------------------------------------------------------
// File: XAudio2BasicStream.cpp
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
#include <Xact3wb.h>

#define STREAMING_BUFFER_SIZE 65536
#define MAX_BUFFER_COUNT 3

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
// Callback structure
//--------------------------------------------------------------------------------------
struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
            STDMETHOD_( void, OnVoiceProcessingPassStart )( UINT32 )
            {
            }
            STDMETHOD_( void, OnVoiceProcessingPassEnd )()
            {
            }
            STDMETHOD_( void, OnStreamEnd )()
            {
            }
            STDMETHOD_( void, OnBufferStart )( void* )
            {
            }
            STDMETHOD_( void, OnBufferEnd )( void* )
            {
                SetEvent( hBufferEndEvent );
            }
            STDMETHOD_( void, OnLoopEnd )( void* )
            {
            }
            STDMETHOD_( void, OnVoiceError )( void*, HRESULT )
            {
            }

    HANDLE hBufferEndEvent;

            StreamingVoiceContext() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) )
            {
            }
    virtual ~StreamingVoiceContext()
    {
        CloseHandle( hBufferEndEvent );
    }
};


//--------------------------------------------------------------------------------------
// Wavebank class
//--------------------------------------------------------------------------------------
class Wavebank
{
public:
            Wavebank() : entries( NULL )
            {
            }
            ~Wavebank()
            {
                Release();
            }

    HRESULT Load( const WCHAR* wbfile );
    void    Release()
    {
        SAFE_DELETE_ARRAY( entries );
    }

    DWORD   GetEntryCount() const
    {
        return data.dwEntryCount;
    }
    DWORD   GetEntryLengthInBytes( DWORD index )
    {
        return ( index < data.dwEntryCount ) ? ( entries[index].PlayRegion.dwLength ) : 0;
    }
    DWORD   GetEntryOffset( DWORD index )
    {
        return ( index < data.dwEntryCount ) ? ( entries[index].PlayRegion.dwOffset ) : 0;
    }
    HRESULT GetEntryFormat( DWORD index, WAVEFORMATEX* pFormat );

private:
    WAVEBANKHEADER header;
    WAVEBANKDATA data;
    WAVEBANKENTRY* entries;
};


//--------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------
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
    // Find our wave bank file
    //
    WCHAR wavebank[ MAX_PATH ];

    if( FAILED( hr = FindMediaFileCch( wavebank, MAX_PATH, L"wavebank.xwb" ) ) )
    {
        wprintf( L"Failed to find media file (%#X)\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Extract wavebank data (entries, formats, offsets, and sizes)
    //
    // Note we are only using XACTBLD to create sector-aligned streaming data to allow us to use
    // async unbuffered I/O. Raw .WAV files do not meet these requirements.
    //
    Wavebank wb;

    if( FAILED( hr = wb.Load( wavebank ) ) )
    {
        wprintf( L"Failed to wavebank data (%#X)\n", hr );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Open the wavebank file for async operations
    //

    HANDLE hAsync = CreateFile( wavebank, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );

    if( hAsync == INVALID_HANDLE_VALUE )
    {
        wprintf( L"Failed to open wavebank for async (%#X)\n", HRESULT_FROM_WIN32( GetLastError() ) );
        SAFE_RELEASE( pXAudio2 );
        CoUninitialize();
        return 0;
    }

    //
    // Repeated loop through all the wavebank entries
    //
    bool exit = false;

    while( !exit )
    {
        for( DWORD i = 0; i < wb.GetEntryCount(); ++i )
        {
            wprintf( L"Now playing wave entry %d", i );

            //
            // Get the info we need to play back this wave (need enough space for PCM, ADPCM, and xWMA formats)
            //
            char formatBuff[ 64 ]; 
            WAVEFORMATEX *wfx = reinterpret_cast<WAVEFORMATEX*>(&formatBuff);

            if( FAILED( hr = wb.GetEntryFormat( i, wfx ) ) )
            {
                wprintf( L"\nCouldn't get wave format for entry %d: error 0x%x\n", i, hr );
                exit = true;
                break;
            }

            DWORD waveOffset = wb.GetEntryOffset( i );
            DWORD waveLength = wb.GetEntryLengthInBytes( i );

            //
            // Create an XAudio2 voice to stream this wave
            //
            StreamingVoiceContext voiceContext;

            IXAudio2SourceVoice* pSourceVoice;
            if( FAILED( hr = pXAudio2->CreateSourceVoice( &pSourceVoice, wfx, 0, 1.0f, &voiceContext ) ) )
            {
                wprintf( L"\nError %#X creating source voice\n", hr );
                exit = true;
                break;
            }
            pSourceVoice->Start( 0, 0 );

            //
            // Create an overlapped structure and buffers to handle the async I/O
            //
            OVERLAPPED ovlCurrentRequest = {0};
            ovlCurrentRequest.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

            BYTE buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];
            DWORD currentDiskReadBuffer = 0;
            DWORD currentPosition = 0;

            //
            // This sample code shows the simplest way to manage asynchronous
            // streaming. There are three different processes involved. One is the management
            // process, which is what we're writing here. The other two processes are
            // essentially hardware operations: disk reads from the I/O system, and
            // audio processing from XAudio2. Disk reads and audio playback both happen
            // without much intervention from our application, so our job is just to make
            // sure that the data being read off the disk makes it over to the audio
            // processor in time to be played back.
            //
            // There are two events that can happen in this system. The disk I/O system can
            // signal that data is ready, and the audio system can signal that it's done
            // playing back data. We can handle either or both of these events either synchronously
            // (via polling) or asynchronously (via callbacks or by waiting on an event
            // object).
            //
            while( currentPosition < waveLength )
            {
                wprintf( L"." );

                if( GetAsyncKeyState( VK_ESCAPE ) )
                {
                    exit = true;

                    while( GetAsyncKeyState( VK_ESCAPE ) )
                        Sleep( 10 );

                    break;
                }

                //
                // Issue a request.
                //
                // Note: although the file read will be done asynchronously, it is possible for the
                // call to ReadFileEx to block for longer than you might think. If the I/O system needs
                // to read the file allocation table in order to satisfy the read, it will do that
                // BEFORE returning from ReadFileEx. That means that this call could potentially
                // block for several milliseconds! In order to get "true" async I/O you should put
                // this entire loop on a separate thread.
                //
                // Second note: async requests have to be a multiple of the disk sector size. Rather than
                // handle this conditionally, make all reads the same size but remember how many
                // bytes we actually want and only submit that many to the voice.
                //
                DWORD cbValid = min( STREAMING_BUFFER_SIZE, waveLength - currentPosition );
                ovlCurrentRequest.Offset = waveOffset + currentPosition;

                if( !ReadFileEx( hAsync, buffers[ currentDiskReadBuffer ], STREAMING_BUFFER_SIZE, &ovlCurrentRequest,
                                 NULL ) )
                {
                    wprintf( L"\nCouldn't start async read: error %#X\n", HRESULT_FROM_WIN32( GetLastError() ) );
                    exit = true;
                    break;
                }

                currentPosition += cbValid;

                //
                // At this point the read is progressing in the background and we are free to do
                // other processing while we wait for it to finish. For the purposes of this sample,
                // however, we'll just go to sleep until the read is done.
                //
                DWORD cb;
                GetOverlappedResult( hAsync, &ovlCurrentRequest, &cb, TRUE );

                //
                // Now that the event has been signaled, we know we have audio available. The next
                // question is whether our XAudio2 source voice has played enough data for us to give
                // it another buffer full of audio. We'd like to keep no more than MAX_BUFFER_COUNT - 1
                // buffers on the queue, so that one buffer is always free for disk I/O.
                //
                XAUDIO2_VOICE_STATE state;
                for(; ; )
                {
                    pSourceVoice->GetState( &state );
                    if( state.BuffersQueued < MAX_BUFFER_COUNT - 1 )
                        break;

                    WaitForSingleObject( voiceContext.hBufferEndEvent, INFINITE );
                }

                //
                // At this point we have a buffer full of audio and enough room to submit it, so
                // let's submit it and get another read request going.
                //
                XAUDIO2_BUFFER buf = {0};
                buf.AudioBytes = cbValid;
                buf.pAudioData = buffers[currentDiskReadBuffer];
                if( currentPosition >= waveLength )
                    buf.Flags = XAUDIO2_END_OF_STREAM;

                pSourceVoice->SubmitSourceBuffer( &buf );

                currentDiskReadBuffer++;
                currentDiskReadBuffer %= MAX_BUFFER_COUNT;
            }

            if( !exit )
            {
                wprintf( L"done streaming.." );

                XAUDIO2_VOICE_STATE state;
                for(; ; )
                {
                    pSourceVoice->GetState( &state );
                    if( !state.BuffersQueued )
                        break;

                    wprintf( L"." );
                    WaitForSingleObject( voiceContext.hBufferEndEvent, INFINITE );
                }
            }

            //
            // Clean up
            //
            pSourceVoice->Stop( 0 );
            pSourceVoice->DestroyVoice();

            CloseHandle( ovlCurrentRequest.hEvent );

            wprintf( L"stopped\n" );

            if( exit )
                break;

            Sleep( 500 );
        }
    }

    //
    // Cleanup XAudio2
    //

    // All XAudio2 interfaces are released when the engine is destroyed, but being tidy
    pMasteringVoice->DestroyVoice();

    CloseHandle( hAsync );

    SAFE_RELEASE( pXAudio2 );
    CoUninitialize();
}


//--------------------------------------------------------------------------------------
// Wavebank methods
//--------------------------------------------------------------------------------------
HRESULT Wavebank::Load( const WCHAR* wbfile )
{
    Release();

    HANDLE hFile = CreateFile( wbfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0L, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Read and verify header
    DWORD bytes;
    if( !ReadFile( hFile, &header, sizeof( header ), &bytes, NULL )
        || bytes != sizeof( header ) )
    {
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if( header.dwSignature != WAVEBANK_HEADER_SIGNATURE
        || ( WAVEBANK_HEADER_VERSION < header.dwHeaderVersion ) )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    // Load wavebank data
    SetFilePointer( hFile, header.Segments[ WAVEBANK_SEGIDX_BANKDATA ].dwOffset, 0, SEEK_SET );

    if( !ReadFile( hFile, &data, sizeof( data ), &bytes, NULL )
        || bytes != sizeof( data ) )
    {
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Load entries
    DWORD cbEntries = header.Segments[ WAVEBANK_SEGIDX_ENTRYMETADATA ].dwLength;

    if( data.dwEntryCount != ( cbEntries / sizeof( WAVEBANKENTRY ) ) )
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    entries = new WAVEBANKENTRY[ data.dwEntryCount ];

    SetFilePointer( hFile, header.Segments[ WAVEBANK_SEGIDX_ENTRYMETADATA ].dwOffset, 0, SEEK_SET );

    if( !ReadFile( hFile, entries, cbEntries, &bytes, NULL )
        || bytes != cbEntries )
    {
        SAFE_DELETE_ARRAY( entries );
        CloseHandle( hFile );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    CloseHandle( hFile );

    return S_OK;
}

HRESULT Wavebank::GetEntryFormat( DWORD index, WAVEFORMATEX* pFormat )
{
    if( index >= data.dwEntryCount || !entries )
        return E_FAIL;

    WAVEBANKMINIWAVEFORMAT& miniFmt = ( data.dwFlags & WAVEBANK_FLAGS_COMPACT )
        ? data.CompactFormat : ( entries[ index ].Format );

    switch( miniFmt.wFormatTag )
    {
        case WAVEBANKMINIFORMAT_TAG_PCM:
            pFormat->wFormatTag = WAVE_FORMAT_PCM;
            pFormat->cbSize = 0;
            break;

        case WAVEBANKMINIFORMAT_TAG_ADPCM:
            pFormat->wFormatTag = WAVE_FORMAT_ADPCM;
            pFormat->cbSize = 32; /* MSADPCM_FORMAT_EXTRA_BYTES */
            {
                ADPCMWAVEFORMAT *adpcmFmt = reinterpret_cast<ADPCMWAVEFORMAT*>(pFormat);
                adpcmFmt->wSamplesPerBlock = (WORD) miniFmt.AdpcmSamplesPerBlock();
                miniFmt.AdpcmFillCoefficientTable( adpcmFmt );
            }
            break;

        case WAVEBANKMINIFORMAT_TAG_WMA:
            pFormat->wFormatTag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            pFormat->cbSize = 0;
            break;

        default:
            // WAVEBANKMINIFORMAT_TAG_XMA is only valid for Xbox
            return E_FAIL;
    }

    pFormat->nChannels = miniFmt.nChannels;
    pFormat->wBitsPerSample = miniFmt.BitsPerSample();
    pFormat->nBlockAlign = (WORD) miniFmt.BlockAlign();
    pFormat->nSamplesPerSec = miniFmt.nSamplesPerSec;
    pFormat->nAvgBytesPerSec = miniFmt.AvgBytesPerSec();

    return S_OK;
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
