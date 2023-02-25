//-----------------------------------------------------------------------------
// File: AudioController.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <XInput.h> // XInput API
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h> // DirectSound API
#include <basetsd.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include "resource.h"


//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#define NUM_PLAY_NOTIFICATIONS  16
#define NUM_BUFFERS             16
#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

struct CONTROLER_STATE
{
    // XInput controller state
    bool bConnected;
    XINPUT_STATE state;

    // DirectSound device guids from XInput API.
    // These are different for each controller and are used
    // to create the IDirectSound and IDirectSoundCapture devices
    GUID dsRenderGuid;
    GUID dsCaptureGuid;

    // DirectSound buffers for capture and playback
    LPDIRECTSOUND8 pDS;
    LPDIRECTSOUNDCAPTURE8 pDSCapture;
    LPDIRECTSOUNDBUFFER pDSBPrimary;
    LPDIRECTSOUNDBUFFER pDSBOutput;
    LPDIRECTSOUNDCAPTUREBUFFER pDSBCapture;
    LPDIRECTSOUNDNOTIFY8 pDSNotify;

    // DirectSound state
    DSBPOSITIONNOTIFY aPosNotify[ NUM_PLAY_NOTIFICATIONS ];
    HANDLE hNotificationEvent;
    DWORD dwOutputBufferSize;
    DWORD dwCaptureBufferSize;
    DWORD dwNextOutputOffset;
    DWORD dwNextCaptureOffset;
    DWORD dwNotifySize;
};

CONTROLER_STATE g_Controllers[MAX_CONTROLLERS];
WCHAR g_szMessage[4][1024] = {0};
HWND g_hWnd;


//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
HRESULT UpdateControllerState();
void RenderFrame();
HRESULT InitDirectSound( CONTROLER_STATE* pState, HWND hWnd );
HRESULT StartBuffers( CONTROLER_STATE* pState );
HRESULT StopBuffers( CONTROLER_STATE* pState );
HRESULT RestoreBuffer( LPDIRECTSOUNDBUFFER pDSBuffer, BOOL* pbRestored );
HRESULT HandleNotification( CONTROLER_STATE* pState );
HRESULT FreeDirectSound( CONTROLER_STATE* pState );


//-----------------------------------------------------------------------------
int APIENTRY wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, int )
{
    // Initialize COM
    HRESULT hr;
    if( FAILED( hr = CoInitialize( NULL ) ) )
        return hr;

    // Register the window class
    HBRUSH hBrush = CreateSolidBrush( 0xFF0000 );
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), 0, MsgProc, 0L, 0L, hInst, NULL,
        LoadCursor( NULL, IDC_ARROW ), hBrush,
        NULL, L"XInputSample", NULL
    };
    RegisterClassEx( &wc );

    // Create the application's window
    g_hWnd = CreateWindow( L"XInputSample", L"XInput Sample: AudioController",
                           WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
                           NULL, NULL, hInst, NULL );

    // Init state
    ZeroMemory( g_Controllers, sizeof( CONTROLER_STATE ) * MAX_CONTROLLERS );
    UpdateControllerState();

    // Init DirectSound
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        hr = InitDirectSound( &g_Controllers[i], g_hWnd );
        if( FAILED( hr ) )
        {
            MessageBox( g_hWnd, L"Error initializing DirectSound. "
                        L"The sample will now exit.", L"AudioController Sample",
                        MB_OK | MB_ICONERROR );
            return 1;
        }

        StartBuffers( &g_Controllers[i] );
    }

    // Enter the message loop
    MSG msg;
    msg.message = WM_NULL;

    DWORD dwResult;
    bool bDone = FALSE;

    HANDLE hNotificationArray[MAX_CONTROLLERS];
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
        hNotificationArray[i] = g_Controllers[i].hNotificationEvent;

    while( !bDone )
    {
        dwResult = MsgWaitForMultipleObjects( 4, hNotificationArray,
                                              FALSE, 20, QS_ALLEVENTS );
        switch( dwResult )
        {
            case WAIT_OBJECT_0 + 0:
            case WAIT_OBJECT_0 + 1:
            case WAIT_OBJECT_0 + 2:
            case WAIT_OBJECT_0 + 3:
                {
                    // g_hNotificationEvent is signaled
                    //
                    // This means that DirectSound just finished playing 
                    // a piece of the buffer, so we need to fill the circular 
                    // buffer with new sound from the wav file

                    int iPort = dwResult - WAIT_OBJECT_0; // which controller 

                    if( FAILED( hr = HandleNotification( &g_Controllers[iPort] ) ) )
                    {
                        MessageBox( g_hWnd, L"Error handling DirectSound notifications. "
                                    L"The sample will now exit.", L"AudioController Sample",
                                    MB_OK | MB_ICONERROR );
                        bDone = true;
                    }
                    break;
                }

            case WAIT_OBJECT_0 + 4:
            {
                // Messages are available
                while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );

                    if( msg.message == WM_QUIT )
                        bDone = true;
                }
                break;
            }

            case WAIT_TIMEOUT:
            {
                UpdateControllerState();
                RenderFrame();
                break;
            }
        }
    }

    // Clean up 
    UnregisterClass( L"XInputSample", NULL );
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
        FreeDirectSound( &g_Controllers[i] );
    CoUninitialize();

    return 0;
}


//-----------------------------------------------------------------------------
HRESULT InitDirectSound( CONTROLER_STATE* pState, HWND hWnd )
{
    HRESULT hr;

    pState->hNotificationEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( !pState->bConnected )
        return S_FALSE;
    if( memcmp( &pState->dsRenderGuid, &GUID_NULL, sizeof( GUID ) ) == 0 )
        return S_FALSE;

    ZeroMemory( &pState->aPosNotify, sizeof( DSBPOSITIONNOTIFY ) * ( NUM_PLAY_NOTIFICATIONS ) );
    pState->dwOutputBufferSize = 0;
    pState->dwCaptureBufferSize = 0;
    pState->dwNotifySize = 0;
    pState->dwNextOutputOffset = 0;

    // Create IDirectSound8 using the controller's render device
    if( FAILED( hr = DirectSoundCreate8( &pState->dsRenderGuid, &pState->pDS, NULL ) ) )
        return hr;

    // Set coop level to DSSCL_PRIORITY
    if( FAILED( hr = pState->pDS->SetCooperativeLevel( hWnd, DSSCL_NORMAL ) ) )
        return hr;

    // Create IDirectSoundCapture using the controller's capture device
    if( FAILED( hr = DirectSoundCaptureCreate8( &pState->dsCaptureGuid, &pState->pDSCapture, NULL ) ) )
        return hr;

    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof( wfx ) );
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nSamplesPerSec = 22050;
    wfx.wBitsPerSample = 16;
    wfx.nChannels = 1;
    wfx.nBlockAlign = wfx.nChannels * ( wfx.wBitsPerSample / 8 );
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

    // Set the notification size
    pState->dwNotifySize = max( 4096, wfx.nAvgBytesPerSec / 8 );
    pState->dwNotifySize -= pState->dwNotifySize % wfx.nBlockAlign;

    // Set the buffer sizes 
    pState->dwOutputBufferSize = NUM_BUFFERS * pState->dwNotifySize / 2;
    pState->dwCaptureBufferSize = NUM_BUFFERS * pState->dwNotifySize;

    SAFE_RELEASE( pState->pDSBCapture );

    // Create the capture buffer
    DSCBUFFERDESC dscbd;
    ZeroMemory( &dscbd, sizeof( dscbd ) );
    dscbd.dwSize = sizeof( dscbd );
    dscbd.dwBufferBytes = pState->dwCaptureBufferSize;
    dscbd.lpwfxFormat = &wfx; // Set the format during creatation

    if( FAILED( hr = pState->pDSCapture->CreateCaptureBuffer( &dscbd,
                                                              &pState->pDSBCapture,
                                                              NULL ) ) )
        return hr;

    // Create the direct sound buffer using the same format as the 
    // capture buffer. 
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof( DSBUFFERDESC ) );
    dsbd.dwSize = sizeof( DSBUFFERDESC );
    dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
    dsbd.dwBufferBytes = pState->dwOutputBufferSize;
    dsbd.guid3DAlgorithm = GUID_NULL;
    dsbd.lpwfxFormat = &wfx;

    // Create the DirectSound buffer 
    if( FAILED( hr = pState->pDS->CreateSoundBuffer( &dsbd, &pState->pDSBOutput, NULL ) ) )
        return hr;

    // Create a notification event, for when the sound stops playing
    if( FAILED( hr = pState->pDSBCapture->QueryInterface( IID_IDirectSoundNotify,
                                                          ( VOID** )&pState->pDSNotify ) ) )
        return hr;

    // Setup the notification positions
    for( INT i = 0; i < NUM_PLAY_NOTIFICATIONS; i++ )
    {
        pState->aPosNotify[i].dwOffset = ( pState->dwNotifySize * i ) + pState->dwNotifySize - 1;
        pState->aPosNotify[i].hEventNotify = pState->hNotificationEvent;
    }

    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
    if( FAILED( hr = pState->pDSNotify->SetNotificationPositions( NUM_PLAY_NOTIFICATIONS,
                                                                  pState->aPosNotify ) ) )
        return hr;

    return S_OK;
}


//-----------------------------------------------------------------------------
HRESULT FreeDirectSound( CONTROLER_STATE* pState )
{
    SAFE_RELEASE( pState->pDSNotify );
    SAFE_RELEASE( pState->pDSBPrimary );
    SAFE_RELEASE( pState->pDSBOutput );
    SAFE_RELEASE( pState->pDSBCapture );
    SAFE_RELEASE( pState->pDSCapture );
    SAFE_RELEASE( pState->pDS );
    CloseHandle( pState->hNotificationEvent );

    return S_OK;
}


//-----------------------------------------------------------------------------
HRESULT StartBuffers( CONTROLER_STATE* pState )
{
    VOID* pDSLockedBuffer = NULL;
    DWORD dwDSLockedBufferSize;
    HRESULT hr;

    if( pState->pDS == NULL )
        return S_FALSE;

    // Restore lost buffers
    if( FAILED( hr = RestoreBuffer( pState->pDSBOutput, NULL ) ) )
        return hr;

    // Reset the buffers
    pState->dwNextOutputOffset = 0;
    pState->pDSBOutput->SetCurrentPosition( 0 );

    // Find out where the capture buffer is right now, then write data 
    // some extra amount forward to make sure we're ahead of the write cursor
    pState->pDSBCapture->GetCurrentPosition( &pState->dwNextCaptureOffset, NULL );
    pState->dwNextCaptureOffset -= pState->dwNextCaptureOffset % pState->dwNotifySize;

    pState->dwNextOutputOffset = pState->dwNextCaptureOffset + ( 2 * pState->dwNotifySize );
    pState->dwNextOutputOffset %= pState->dwOutputBufferSize;  // Circular buffer

    // Tell the capture buffer to start recording
    pState->pDSBCapture->Start( DSCBSTART_LOOPING );

    // Rewind the output buffer, fill it with silence, and play it
    pState->pDSBOutput->SetCurrentPosition( pState->dwNextOutputOffset );

    // Get the format of the output buffer
    WAVEFORMATEX wfxOutput;
    ZeroMemory( &wfxOutput, sizeof( wfxOutput ) );
    pState->pDSBOutput->GetFormat( &wfxOutput, sizeof( wfxOutput ), NULL );

    // Fill the output buffer with silence at first
    // As capture data arrives, HandleNotifications() will fill
    // the output buffer with wave data.
    if( FAILED( hr = pState->pDSBOutput->Lock( 0, pState->dwOutputBufferSize,
                                               &pDSLockedBuffer, &dwDSLockedBufferSize,
                                               NULL, NULL, 0 ) ) )
        return hr;

    FillMemory( ( BYTE* )pDSLockedBuffer, dwDSLockedBufferSize,
                ( BYTE )( wfxOutput.wBitsPerSample == 8 ? 128 : 0 ) );
    pState->pDSBOutput->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, NULL );

    // Play the output buffer 
    pState->pDSBOutput->Play( 0, 0, DSBPLAY_LOOPING );

    return S_OK;
}


//-----------------------------------------------------------------------------
HRESULT StopBuffers( CONTROLER_STATE* pState )
{
    if( pState->pDSBCapture )
        pState->pDSBCapture->Stop();
    if( pState->pDSBOutput )
        pState->pDSBOutput->Stop();

    return S_OK;
}


//-----------------------------------------------------------------------------
// Restores a lost buffer. *pbWasRestored returns TRUE if the buffer was 
// restored.  It can also NULL if the information is not needed.
//-----------------------------------------------------------------------------
HRESULT RestoreBuffer( LPDIRECTSOUNDBUFFER pDSBuffer, BOOL* pbRestored )
{
    HRESULT hr;

    if( pbRestored != NULL )
        *pbRestored = FALSE;

    if( NULL == pDSBuffer )
        return S_FALSE;

    DWORD dwStatus;
    if( FAILED( hr = pDSBuffer->GetStatus( &dwStatus ) ) )
        return hr;

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do
        {
            hr = pDSBuffer->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        } while( ( hr = pDSBuffer->Restore() ) == DSERR_BUFFERLOST );

        if( pbRestored != NULL )
            *pbRestored = TRUE;

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}


//-----------------------------------------------------------------------------
// Handle the notification that tells us to copy data from the 
// capture buffer to the output buffer 
//-----------------------------------------------------------------------------
HRESULT HandleNotification( CONTROLER_STATE* pState )
{
    HRESULT hr;
    VOID* pDSCaptureLockedBuffer = NULL;
    VOID* pDSOutputLockedBuffer = NULL;
    DWORD dwDSCaptureLockedBufferSize;
    DWORD dwDSOutputLockedBufferSize;

    DWORD dwStatus;

    // Make sure buffers were not lost, if the were we need 
    // to start the capture again
    if( FAILED( hr = pState->pDSBOutput->GetStatus( &dwStatus ) ) )
        return hr;

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        if( FAILED( hr = StartBuffers( pState ) ) )
            return hr;

        return S_OK;
    }

    // Lock the capture buffer down
    if( FAILED( hr = pState->pDSBCapture->Lock( pState->dwNextCaptureOffset, pState->dwNotifySize,
                                                &pDSCaptureLockedBuffer,
                                                &dwDSCaptureLockedBufferSize,
                                                NULL, NULL, 0L ) ) )
        return hr;

    // Lock the output buffer down
    if( FAILED( hr = pState->pDSBOutput->Lock( pState->dwNextOutputOffset, pState->dwNotifySize,
                                               &pDSOutputLockedBuffer,
                                               &dwDSOutputLockedBufferSize,
                                               NULL, NULL, 0L ) ) )
        return hr;

    // These should be equal
    if( dwDSOutputLockedBufferSize != dwDSCaptureLockedBufferSize )
        return E_FAIL;  // Sanity check unhandled case 

    // This sample just copies the memory from the capture buffer to the playback buffer 
    // Typically the capture data would be sent across the network to another headset
    CopyMemory( pDSOutputLockedBuffer, pDSCaptureLockedBuffer, dwDSOutputLockedBufferSize );

    // Unlock the play buffer
    pState->pDSBOutput->Unlock( pDSOutputLockedBuffer, dwDSOutputLockedBufferSize,
                                NULL, 0 );

    // Unlock the capture buffer
    pState->pDSBCapture->Unlock( pDSCaptureLockedBuffer, dwDSCaptureLockedBufferSize,
                                 NULL, 0 );

    // Move the capture offset along
    pState->dwNextCaptureOffset += dwDSCaptureLockedBufferSize;
    pState->dwNextCaptureOffset %= pState->dwCaptureBufferSize; // Circular buffer

    // Move the playback offset along
    pState->dwNextOutputOffset += dwDSOutputLockedBufferSize;
    pState->dwNextOutputOffset %= pState->dwOutputBufferSize; // Circular buffer

    return S_OK;
}


//-----------------------------------------------------------------------------
HRESULT UpdateControllerState()
{
    DWORD dwResult;
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState( i, &g_Controllers[i].state );

        if( dwResult == ERROR_SUCCESS )
            g_Controllers[i].bConnected = true;
        else
            g_Controllers[i].bConnected = false;

        if( g_Controllers[i].bConnected )
            XInputGetDSoundAudioDeviceGuids( i, &g_Controllers[i].dsRenderGuid, &g_Controllers[i].dsCaptureGuid );
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
HRESULT ConvertGUIDToStringCch( const GUID* pGuidSrc, WCHAR* strDest, int cchDestChar )
{
    return StringCchPrintf( strDest, cchDestChar,
                            TEXT( "{%0.8X-%0.4X-%0.4X-%0.2X%0.2X-%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X}" ),
                            pGuidSrc->Data1, pGuidSrc->Data2, pGuidSrc->Data3,
                            pGuidSrc->Data4[0], pGuidSrc->Data4[1],
                            pGuidSrc->Data4[2], pGuidSrc->Data4[3],
                            pGuidSrc->Data4[4], pGuidSrc->Data4[5],
                            pGuidSrc->Data4[6], pGuidSrc->Data4[7] );
}


//-----------------------------------------------------------------------------
void RenderFrame()
{
    bool bRepaint = false;

    WCHAR sz[4][1024];
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        if( g_Controllers[i].bConnected )
        {
            WCHAR szRenderGuid[256];
            WCHAR szCaptureGuid[256];

            ConvertGUIDToStringCch( &g_Controllers[i].dsRenderGuid, szRenderGuid, 256 );
            ConvertGUIDToStringCch( &g_Controllers[i].dsCaptureGuid, szCaptureGuid, 256 );
            StringCchPrintfW( sz[i], 1024,
                              L"Controller %d: Connected\n"
                              L"  DirectSound Render GUID: %s\n"
                              L"  DirectSound Capture GUID: %s\n",
                              i, szRenderGuid, szCaptureGuid );
        }
        else
        {
            StringCchPrintf( sz[i], 1024,
                             L"Controller %d: Not connected", i );
        }

        if( wcscmp( sz[i], g_szMessage[i] ) != 0 )
        {
            StringCchCopy( g_szMessage[i], 1024, sz[i] );
            bRepaint = true;
        }
    }

    if( bRepaint )
    {
        // Repaint the window if needed 
        InvalidateRect( g_hWnd, NULL, TRUE );
        UpdateWindow( g_hWnd );
    }

    // This sample doesn't use Direct3D.  Instead, it just yields CPU time to other 
    // apps but this is not typically done when rendering
    Sleep( 10 );
}


//-----------------------------------------------------------------------------
// Window message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_KEYDOWN:
            if( wParam == VK_ESCAPE )
                PostQuitMessage( 0 );
            break;

        case WM_PAINT:
        {
            // Paint some simple explanation text
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint( hWnd, &ps );
            SetBkColor( hDC, 0xFF0000 );
            SetTextColor( hDC, 0xFFFFFF );
            RECT rect;
            GetClientRect( hWnd, &rect );

            rect.top = 20;
            rect.left = 20;
            DrawText( hDC, L"This sample captures data from the headset microphone and\n"
                      L"simply echoes the data back to the headset speaker.\n"
                      L"It does this for all controllers that have attached headsets.", -1, &rect, 0 );

            for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
            {
                rect.top = i * 70 + 120;
                rect.left = 20;
                DrawText( hDC, g_szMessage[i], -1, &rect, 0 );
            }

            EndPaint( hWnd, &ps );
            return 0;
        }

        case WM_ACTIVATEAPP:
            for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
            {
                if( wParam == FALSE )
                    StopBuffers( &g_Controllers[i] );
                else
                    StartBuffers( &g_Controllers[i] );
            }

            if( wParam == TRUE )
            {
                // App is now active, so re-enable XInput
                XInputEnable( true );
            }
            else
            {
                // App is now inactive, so disable XInput to prevent
                // user input from effecting application and to 
                // disable rumble. 
                XInputEnable( false );
            }
            break;

        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            break;
        }
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}



