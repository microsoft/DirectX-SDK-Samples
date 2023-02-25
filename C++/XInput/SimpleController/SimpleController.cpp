//-----------------------------------------------------------------------------
// File: SimpleController.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <XInput.h> // XInput API
#include <basetsd.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include "resource.h"


//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
HRESULT UpdateControllerState();
void RenderFrame();


//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

struct CONTROLER_STATE
{
    XINPUT_STATE state;
    bool bConnected;
};

CONTROLER_STATE g_Controllers[MAX_CONTROLLERS];
WCHAR g_szMessage[4][1024] = {0};
HWND    g_hWnd;
bool    g_bDeadZoneOn = true;


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application.  Since we use a simple dialog for 
//       user interaction we don't need to pump messages.
//-----------------------------------------------------------------------------
int APIENTRY wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, int )
{
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
    g_hWnd = CreateWindow( L"XInputSample", L"XInput Sample: SimpleController",
                           WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
                           NULL, NULL, hInst, NULL );

    // Init state
    ZeroMemory( g_Controllers, sizeof( CONTROLER_STATE ) * MAX_CONTROLLERS );

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
            UpdateControllerState();
            RenderFrame();
        }
    }

    // Clean up 
    UnregisterClass( L"XInputSample", NULL );

    return 0;
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
    }

    return S_OK;
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
            WORD wButtons = g_Controllers[i].state.Gamepad.wButtons;

            if( g_bDeadZoneOn )
            {
                // Zero value if thumbsticks are within the dead zone 
                if( ( g_Controllers[i].state.Gamepad.sThumbLX < INPUT_DEADZONE &&
                      g_Controllers[i].state.Gamepad.sThumbLX > -INPUT_DEADZONE ) &&
                    ( g_Controllers[i].state.Gamepad.sThumbLY < INPUT_DEADZONE &&
                      g_Controllers[i].state.Gamepad.sThumbLY > -INPUT_DEADZONE ) )
                {
                    g_Controllers[i].state.Gamepad.sThumbLX = 0;
                    g_Controllers[i].state.Gamepad.sThumbLY = 0;
                }

                if( ( g_Controllers[i].state.Gamepad.sThumbRX < INPUT_DEADZONE &&
                      g_Controllers[i].state.Gamepad.sThumbRX > -INPUT_DEADZONE ) &&
                    ( g_Controllers[i].state.Gamepad.sThumbRY < INPUT_DEADZONE &&
                      g_Controllers[i].state.Gamepad.sThumbRY > -INPUT_DEADZONE ) )
                {
                    g_Controllers[i].state.Gamepad.sThumbRX = 0;
                    g_Controllers[i].state.Gamepad.sThumbRY = 0;
                }
            }

            StringCchPrintfW( sz[i], 1024,
                              L"Controller %d: Connected\n"
                              L"  Buttons: %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n"
                              L"  Left Trigger: %d\n"
                              L"  Right Trigger: %d\n"
                              L"  Left Thumbstick: %d/%d\n"
                              L"  Right Thumbstick: %d/%d", i,
                              ( wButtons & XINPUT_GAMEPAD_DPAD_UP ) ? L"DPAD_UP " : L"",
                              ( wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) ? L"DPAD_DOWN " : L"",
                              ( wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) ? L"DPAD_LEFT " : L"",
                              ( wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) ? L"DPAD_RIGHT " : L"",
                              ( wButtons & XINPUT_GAMEPAD_START ) ? L"START " : L"",
                              ( wButtons & XINPUT_GAMEPAD_BACK ) ? L"BACK " : L"",
                              ( wButtons & XINPUT_GAMEPAD_LEFT_THUMB ) ? L"LEFT_THUMB " : L"",
                              ( wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ) ? L"RIGHT_THUMB " : L"",
                              ( wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ) ? L"LEFT_SHOULDER " : L"",
                              ( wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ) ? L"RIGHT_SHOULDER " : L"",
                              ( wButtons & XINPUT_GAMEPAD_A ) ? L"A " : L"",
                              ( wButtons & XINPUT_GAMEPAD_B ) ? L"B " : L"",
                              ( wButtons & XINPUT_GAMEPAD_X ) ? L"X " : L"",
                              ( wButtons & XINPUT_GAMEPAD_Y ) ? L"Y " : L"",
                              g_Controllers[i].state.Gamepad.bLeftTrigger,
                              g_Controllers[i].state.Gamepad.bRightTrigger,
                              g_Controllers[i].state.Gamepad.sThumbLX,
                              g_Controllers[i].state.Gamepad.sThumbLY,
                              g_Controllers[i].state.Gamepad.sThumbRX,
                              g_Controllers[i].state.Gamepad.sThumbRY );
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
        case WM_ACTIVATEAPP:
        {
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
        }

        case WM_KEYDOWN:
        {
            if( wParam == 'D' ) g_bDeadZoneOn = !g_bDeadZoneOn;
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

            rect.top = 20;
            rect.left = 20;
            DrawText( hDC,
                      L"This sample displays the state of all 4 XInput controllers\nPress 'D' to toggle dead zone clamping.", -1, &rect, 0 );

            for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
            {
                rect.top = i * 120 + 70;
                rect.left = 20;
                DrawText( hDC, g_szMessage[i], -1, &rect, 0 );
            }

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



