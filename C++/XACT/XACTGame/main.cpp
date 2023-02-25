//--------------------------------------------------------------------------------------
// File: main.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include <stdio.h>
#include "resource.h"
#include "game.h"
#include "audio.h"


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    HRESULT hr = S_OK;

    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set the callback functions
    DXUTSetCallbackD3D9DeviceAcceptable( IsDeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnCreateDevice );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceReset( OnResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    // Hide the cursor and clip it when in full screen
    DXUTSetCursorSettings( false, true );
    DXUTInit( true, true ); // Parse the command line, handle the default hotkeys, and show msgboxes
    DXUTSetHotkeyHandling( false, false, false );

    // Application-specific initialization
    InitApp();

    hr = PrepareAudio();
    if( FAILED( hr ) )
    {
        OutputDebugString( L"PrepareAudio() failed.  Disabling audio support\n" );
    }

    // Initialize DXUT and create the desired Win32 window and Direct3D device 
    DXUTCreateWindow( L"XACTGame" );
    DXUTCreateDevice( false, 0, 0 );

    // Start the frame loop
    DXUTMainLoop();

    CleanupAudio();

    return DXUTGetExitCode();
}

