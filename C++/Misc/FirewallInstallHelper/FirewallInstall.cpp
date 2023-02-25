//-----------------------------------------------------------------------------
// File: FirewallInstall.cpp
//
// Desc: Sample code that calls FirewallInstallHelper sample dll and displays the results.
//
// (C) Copyright Microsoft Corp.  All rights reserved.
//-----------------------------------------------------------------------------
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
// The Microsoft Platform SDK or Microsoft Windows SDK is required to compile this sample
#include <rpcsal.h>
#include <gameux.h>
#include <strsafe.h>
#include <shlobj.h>
#include <wbemidl.h>
#include <objbase.h>
#include "FirewallInstallHelper.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif



//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and pops
//       up a message box with the results of the FirewallInstallHelper calls
//-----------------------------------------------------------------------------
int
PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                LPSTR strCmdLine, int nCmdShow )
{
    bool bFailure = false;

    WCHAR strBinPath[MAX_PATH];
    WCHAR szMsg[MAX_PATH];
    HRESULT hr;

    int nNumArgs;
    LPWSTR* pstrArgList = CommandLineToArgvW( GetCommandLine(), &nNumArgs );
    if( nNumArgs == 2 )
    {
        WCHAR* strCmdLine = pstrArgList[1];
        StringCchPrintf( strBinPath, MAX_PATH, L"%s", strCmdLine );
    }
    else
    {
        WCHAR strSystem[MAX_PATH];
        GetSystemDirectory( strSystem, MAX_PATH );
        StringCchPrintf( strBinPath, MAX_PATH, L"%s\\notepad.exe", strSystem );
    }

    if( IDYES == MessageBox( NULL, L"Add Game?", L"FirewallInstall", MB_YESNO ) )
    {
        bFailure = false;

        // Upon install do this
        // Change the paths in this call to be correct
        hr = AddApplicationToExceptionList( strBinPath, strBinPath ); // Typically the 2nd arg is a friendly name like "Solitare"
        if( FAILED( hr ) )
        {
            StringCchPrintf( szMsg, 256, L"Adding game failed: 0x%0.8x", hr );
            MessageBox( NULL, szMsg, TEXT( "AddApplicationToExceptionList" ), MB_OK | MB_ICONINFORMATION );
            bFailure = true;
        }

        if( !bFailure )
        {
            StringCchPrintf( szMsg, 256, L"Adding game succeeded" );
            MessageBox( NULL, szMsg, TEXT( "AddApplicationToExceptionList" ), MB_OK | MB_ICONINFORMATION );
        }
    }

    if( IDYES == MessageBox( NULL, L"Remove Game?", L"FirewallInstall", MB_YESNO ) )
    {
        bFailure = false;

        // Upon uninstall do this
        hr = RemoveApplicationFromExceptionList( strBinPath );
        if( FAILED( hr ) )
        {
            StringCchPrintf( szMsg, 256, L"Removing game failed: 0x%0.8x", hr );
            MessageBox( NULL, szMsg, TEXT( "RemoveFromGameExplorer" ), MB_OK | MB_ICONINFORMATION );
            bFailure = true;
        }

        if( !bFailure )
        {
            StringCchPrintf( szMsg, 256, L"Removing game succeeded" );
            MessageBox( NULL, szMsg, TEXT( "RemoveFromGameExplorer" ), MB_OK | MB_ICONINFORMATION );
        }
    }


    LocalFree( pstrArgList );
    return 0;
}


