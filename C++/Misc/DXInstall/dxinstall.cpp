//-----------------------------------------------------------------------------
// File: DxInstall.cpp
//
// Desc: Example code showing how to use DirectXSetup.
//
//       This file contains code that will handle all messages sent to the
//       DirectXSetupCallbackFunction, with the filtering level set at what the
//       user wants.  This way you can test to see which messages you want to
//       handle automatically or pass on to the user.
//
//       Call Tree:
//          DirectXInstallWndProc         See WINCODE.CPP
//             DirectXInstall             Set up the callback and handle return codes
//                 GetReply               See WINCODE.CPP
//             DirectXGetVersion          Display the results of DirectXSetupGetVersion()
//          DirectXSetupCallbackFunction  Called from DirectXSetup
//             GetReply                   See WINCODE.CPP
//             SetButtons                 See WINCODE.CPP
//                ShowButton              See WINCODE.CPP
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <windows.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <stdio.h>
#include <string.h>
#include <dsetup.h>
#include "resource.h"
#include "DXInstall.h"




//-----------------------------------------------------------------------------
// Externs for global variables
//-----------------------------------------------------------------------------
extern DWORD                        g_dwStatus;         // Filter setting for messages from DirectXSetup
extern HINSTANCE                    g_hInstance;        // Global instance handle
extern HWND                         g_hDlg;             // Window handle to dialog proc
extern WCHAR g_strAppTitle[256]; // Application title
extern INT                          g_iReply;           // Global value for dialog return



//--------------------------------------------------------------------------------------
// DirectSetup dynamic linking support -- calls top-level dsetup.dll APIs with graceful
// failure if APIs are not present.
//--------------------------------------------------------------------------------------
typedef INT ( WINAPI* LPDIRECTXSETUPSETCALLBACK )( DSETUP_CALLBACK Callback );
typedef INT ( WINAPI* LPDIRECTXSETUPGETVERSION )( DWORD* lpdwVersion, DWORD* lpdwMinorVersion );
#ifdef UNICODE
    typedef INT (WINAPI * LPDIRECTXSETUP)( HWND hWnd, LPWSTR lpszRootPath, DWORD dwFlags );
#else
typedef INT ( WINAPI* LPDIRECTXSETUP )( HWND hWnd, LPSTR lpszRootPath, DWORD dwFlags );
#endif

// Module and function pointers
static HMODULE                      s_hModDSetup = NULL;
static LPDIRECTXSETUPSETCALLBACK    s_DirectXSetupSetCallback = NULL;
static LPDIRECTXSETUPGETVERSION     s_DirectXSetupGetVersion = NULL;
static LPDIRECTXSETUP               s_DirectXSetup = NULL;


//-----------------------------------------------------------------------------
// Ensure function pointers are initialized
//-----------------------------------------------------------------------------
bool InitDirectSetupAPIs()
{
    // If module is non-NULL, this function has already been called.  Note
    // that this doesn't guarantee that all D3D9 procaddresses were found.
    if( s_hModDSetup != NULL )
        return true;

    // This may fail if DirectX 9 isn't installed
    s_hModDSetup = LoadLibrary( TEXT( "dsetup.dll" ) );
    if( s_hModDSetup == NULL )
        return false;

    s_DirectXSetupSetCallback = ( LPDIRECTXSETUPSETCALLBACK )GetProcAddress( s_hModDSetup, "DirectXSetupSetCallback" );
    s_DirectXSetupGetVersion = ( LPDIRECTXSETUPGETVERSION )GetProcAddress( s_hModDSetup, "DirectXSetupGetVersion" );
#ifdef UNICODE
    s_DirectXSetup = (LPDIRECTXSETUP)GetProcAddress( s_hModDSetup, "DirectXSetupW" );
#else
    s_DirectXSetup = ( LPDIRECTXSETUP )GetProcAddress( s_hModDSetup, "DirectXSetupA" );
#endif

    if( s_DirectXSetupGetVersion == NULL ||
        s_DirectXSetupSetCallback == NULL ||
        s_DirectXSetup == NULL )
        return false;

    return true;
}


//-----------------------------------------------------------------------------
// Name: GetReply()
// Desc: Waits for the user to click on a button on our simulated message box
//       See DlgProc for the code that sets g_wReply
//-----------------------------------------------------------------------------
DWORD GetReply( DWORD dwMsgType )
{
    DWORD dwDefaultButton = 0;

    // Wait until DlgProc() lets us know that the user clicked on a button
    while( g_iReply == -1 )
    {
        MSG msg;

        // Forward my messages...
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if( msg.message == WM_QUIT ||
                msg.message == WM_CLOSE ||
                msg.message == WM_SYSCOMMAND ||
                msg.message == WM_DESTROY )
            {
                // Put the message back on the queue and get out of here.
                PostMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam );
                break;
            }
            if( !IsDialogMessage( msg.hwnd, &msg ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
    }

    // Return the proper ID value for the button the user clicked on
    // This code simulates what MessageBox() would return
    switch( dwMsgType & 0x0000000F )
    {
        case MB_OKCANCEL:
            dwDefaultButton = ( g_iReply == IDBUT1 ) ? IDOK : IDCANCEL;
            break;

        case MB_OK:
            dwDefaultButton = IDOK;
            break;

        case MB_RETRYCANCEL:
            dwDefaultButton = ( g_iReply == IDBUT1 ) ? IDRETRY : IDCANCEL;
            break;

        case MB_ABORTRETRYIGNORE:
            if( g_iReply == IDBUT1 )
                dwDefaultButton = IDABORT;
            else if( g_iReply == IDBUT2 )
                dwDefaultButton = IDRETRY;
            else
                dwDefaultButton = IDIGNORE;
            break;

        case MB_YESNOCANCEL:
            if( g_iReply == IDBUT1 )
                dwDefaultButton = IDYES;
            else if( g_iReply == IDBUT2 )
                dwDefaultButton = IDNO;
            else
                dwDefaultButton = IDCANCEL;
            break;

        case MB_YESNO:
            dwDefaultButton = ( g_iReply == IDBUT1 ) ? IDYES : IDNO;
            break;

        default:
            dwDefaultButton = IDOK;
    }

    g_iReply = -1;

    return dwDefaultButton;
}




//-----------------------------------------------------------------------------
// Name: DirectXSetupCallbackFunction()
// Desc: Handle each reason for why the callback was called, filtering each
//       message by what the current state of g_fStatus is.
//-----------------------------------------------------------------------------
DWORD WINAPI DirectXSetupCallbackFunction( DWORD dwReason, DWORD dwMsgType,
                                           LPWSTR strMessage, LPWSTR strName,
                                           VOID* pInfo )
{
    if( strMessage == NULL && strName == NULL )
        return IDOK;

    if( dwReason == DSETUP_CB_MSG_PROGRESS )
    {
        // Add code to update progress meter if there was one
        return IDOK;
    }

    SetButtons( g_hDlg, 0xffffffff );
    if( g_dwStatus == SHOW_ALL )
    {
        // Show all messages from DirectSetup

        SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), strMessage );

        // When dwMsgType is equal to zero we can display status information but
        // should not wait for input from the user. 
        if( dwMsgType == 0 )
        {
            Sleep( 1000 );
            return IDOK;
        }

        SetButtons( g_hDlg, dwMsgType );
    }
    else if( g_dwStatus == SHOW_UPGRADES )
    {
        // Show only upgrade messages

        switch( dwReason )
        {
            case DSETUP_CB_MSG_INTERNAL_ERROR:
                SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), strMessage );
                SetButtons( g_hDlg, dwMsgType );
                break;

            default:
                return IDOK;
        }
    }
    else if( g_dwStatus == SHOW_PROBLEMS )
    {
        // Show only problem messages

        switch( dwReason )
        {
            case DSETUP_CB_MSG_INTERNAL_ERROR:
                SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), strMessage );
                SetButtons( g_hDlg, dwMsgType );
                break;

            default:
                return IDOK;
        }
    }
    else if( g_dwStatus == SHOW_NONE )
    {
        // Don't show any messages
        return IDOK;
    }

    return GetReply( dwMsgType );
}




//-----------------------------------------------------------------------------
// Name: DirectXGetVersion( HWND hWnd )
// Desc: Shows the results of a call to DirectXSetupGetVersion()
//-----------------------------------------------------------------------------
VOID DirectXGetVersion( HWND hWnd )
{
    DWORD dwVersion;
    DWORD dwRevision;

    if( InitDirectSetupAPIs() == false )
    {
        MessageBox( hWnd,
                    L"dsetup.dll could not be found.  In order to use the DirectSetup API, copy the entire contents of the \\Redist folder from the DirectX SDK into the same folder as this sample.  See the documenation and source for further details.", L"Error", MB_OK | MB_ICONINFORMATION );
        return;
    }

    s_DirectXSetupGetVersion( &dwVersion, &dwRevision );
    // Use HIWORD(dwVersion); to get the DirectX major version
    // Use LOWORD(dwVersion); to get the DirectX minor version
    // For example: for DirectX 5 dwVersion == 0x00040005

    WCHAR strBuf[128];
    StringCchPrintf( strBuf, 128, L"Version 0x%08lX\nRevision %ld", dwVersion, dwRevision );
    MessageBox( hWnd, strBuf, L"Results:", MB_OK | MB_ICONINFORMATION );
}




//-----------------------------------------------------------------------------
// Name: EnableReboot()
// Desc: Enables reboot functionality on Win2k+
//-----------------------------------------------------------------------------
VOID EnableReboot( VOID )
{
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hToken;
    if( OpenProcessToken( hProcess, TOKEN_ALL_ACCESS, &hToken ) )
    {
        LUID luidShutDown;
        if( LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &luidShutDown ) )
        {
            TOKEN_PRIVILEGES Privilege;
            Privilege.PrivilegeCount = 1;
            Privilege.Privileges[0].Luid = luidShutDown;
            Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges( hToken, FALSE, &Privilege, 0, NULL, NULL );
        }
        CloseHandle( hToken );
    }
}




//-----------------------------------------------------------------------------
// Name: DirectXInstall()
// Desc: Set up the callback function for DirectXSetup and handle the return
//       results.  This function starts a modeless version of MessageBox() so
//       that the user can see the progress of the DirectX installation.
//-----------------------------------------------------------------------------
BOOL DirectXInstall( HWND hWnd )
{
    WCHAR strSource[MAX_PATH] = {0};
    WCHAR* strLastSlash = NULL;
    WCHAR string[256];
    INT iRetCode;
    DWORD dwFlags;

    if( InitDirectSetupAPIs() == false )
    {
        MessageBox( hWnd,
                    L"dsetup.dll could not be found.  In order to use the DirectSetup API, copy the entire contents of the \\Redist folder from the DirectX SDK into the same folder as this sample.  See the documenation and source for further details.", L"Error", MB_OK | MB_ICONINFORMATION );
        return false;
    }

    // The DSETUP DLLs should be at the current path, along with the DirectX
    // redist directory so that it can be found and set up.  Get the exe 
    // name, and exe path
    GetModuleFileName( NULL, strSource, MAX_PATH );
    strSource[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strSource, L'\\' );
    if( strLastSlash ) // Chop the exe name from the exe path
        *strLastSlash = 0;

    // If the user wants any messages, bring up the simulated MessageBox
    // dialog
    if( g_dwStatus != SHOW_NONE )
    {
        // Create a modeless dialog box so we can show messages that don't
        // need user input
        g_hDlg = CreateDialog( g_hInstance, L"INSTDX", hWnd, ( DLGPROC )DlgProc );
        if( g_hDlg == NULL )
        {
            WCHAR buf[200];
            LoadString( g_hInstance, STR_NODIALOG, buf, 200 );
            MessageBox( hWnd, string, g_strAppTitle, MB_ICONSTOP | MB_OK );
            return FALSE;
        }

        ShowWindow( GetDlgItem( g_hDlg, IDBUT1 ), SW_HIDE );
        ShowWindow( GetDlgItem( g_hDlg, IDBUT2 ), SW_HIDE );
        ShowWindow( GetDlgItem( g_hDlg, IDBUT3 ), SW_HIDE );
        ShowWindow( g_hDlg, SW_NORMAL );
    }

    // Set the callback function up before calling DirectXSetup
    s_DirectXSetupSetCallback( ( DSETUP_CALLBACK )DirectXSetupCallbackFunction );
    if( g_dwStatus != SHOW_NONE )
    {
        LoadString( g_hInstance, STR_STARTSETUP, string, 256 );
        SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), string );
        SetButtons( g_hDlg, 0xffffffff );
        Sleep( 1000 );
    }

    dwFlags = DSETUP_DIRECTX;
    //-------------------------------------------------------------------------
    // Notes: 
    //
    // 1) Test Install
    //    If you just want to test the install process without
    //    actually installing then add the DSETUP_TESTINSTALL flag like so:
    //    dwFlags |= DSETUP_TESTINSTALL;
    // 
    //  2) Managed DirectX
    //     To install Managed DirectX you must add the DSETUP_MANAGEDDX flag like so:
    //     dwFlags |= DSETUP_MANAGEDDX
    //
    //     Managed DirectX requires the .NET Framework to be installed before 
    //     installing DirectX.  DirectX does NOT install the .NET Framework.    
    //-------------------------------------------------------------------------

    iRetCode = s_DirectXSetup( hWnd, strSource, dwFlags );

    // If the user didn't want any message, we now need to bring up the dialog
    // to reflect the return message from DirectXSetup
    if( g_dwStatus == SHOW_NONE )
    {
        g_hDlg = CreateDialog( g_hInstance, L"INSTDX", hWnd, ( DLGPROC )DlgProc );
        if( g_hDlg == NULL )
        {
            WCHAR buf[200];
            LoadString( g_hInstance, STR_NODIALOG, buf, 200 );
            MessageBox( hWnd, string, g_strAppTitle, MB_ICONSTOP | MB_OK );
            return FALSE;
        }

        ShowWindow( GetDlgItem( g_hDlg, IDBUT1 ), SW_HIDE );
        ShowWindow( GetDlgItem( g_hDlg, IDBUT2 ), SW_HIDE );
        ShowWindow( GetDlgItem( g_hDlg, IDBUT3 ), SW_HIDE );
        ShowWindow( g_hDlg, SW_NORMAL );
    }

    switch( iRetCode )
    {
            // Since our MessageBox dialog is still up, display the results in it
        case DSETUPERR_NEWERVERSION:
            MessageBox( hWnd, L"Installation is newer than one being installed",
                        L"DirectX Setup", MB_OK );
            break;

        case DSETUPERR_SUCCESS_RESTART:
            LoadString( g_hInstance, STR_RESTART, string, 256 );
            SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), string );
            SetButtons( g_hDlg, 0xffffffff );
            SetButtons( g_hDlg, MB_YESNO );
            if( GetReply( MB_YESNO ) == IDYES )
            {
                // Restart Windows
                EnableReboot();
                InitiateSystemShutdownEx( NULL,
                                          NULL,
                                          0,
                                          FALSE,
                                          TRUE,
                                          SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
                                          SHTDN_REASON_MINOR_UPGRADE |
                                          SHTDN_REASON_FLAG_PLANNED );
            }
            break;

        case DSETUPERR_SUCCESS:
            LoadString( g_hInstance, STR_SUCCESS, string, 256 );
            SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), string );
            SetButtons( g_hDlg, 0xffffffff );
            SetButtons( g_hDlg, MB_OK );
            GetReply( MB_OK );
            break;

        case DSETUPERR_BADWINDOWSVERSION:
        case DSETUPERR_SOURCEFILENOTFOUND:
        case DSETUPERR_NOCOPY:
        case DSETUPERR_OUTOFDISKSPACE:
        case DSETUPERR_CANTFINDINF:
        case DSETUPERR_CANTFINDDIR:
        case DSETUPERR_INTERNAL:
        case DSETUPERR_UNKNOWNOS:
            LoadString( g_hInstance, STR_ERRORRETURN + ( iRetCode * -1 ) - 1, string, 256 );
            SetWindowText( GetDlgItem( g_hDlg, ID_MYMESSAGE ), string );
            SetButtons( g_hDlg, 0xffffffff );
            SetButtons( g_hDlg, MB_OK );
            GetReply( MB_OK );
            break;
    }

    DestroyWindow( g_hDlg );
    g_hDlg = NULL;

    return TRUE;
}
