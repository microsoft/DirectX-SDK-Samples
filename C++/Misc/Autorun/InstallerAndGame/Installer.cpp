//-----------------------------------------------------------------------------
// File: Installer.cpp
//
// The faux installer for the Autorun sample.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Common.h"
#include "ProcessInfo.h"
#include "resource.h"


//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------
static const WCHAR c_szRegistryKey[] = L"Software\\Publisher\\Title";
static const WCHAR c_szRegistryKeyRoot[] = L"Software\\Publisher";


//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
BOOL DoesRegistryKeyExist();
BOOL AddRegistryKey();
BOOL RemoveRegistryKey();


//-----------------------------------------------------------------------------
// Program entry point.
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, LPWSTR, int )
{
    if( DoesRegistryKeyExist() )
    {
        int result = MessageBox( NULL,
                                 L"The faux installer has detected that the registry was already "
                                 L"modified for the Autorun sample.\r\n\r\n"
                                 L"Would you like to have these modifications removed?",
                                 L"Installer",
                                 MB_YESNO | MB_ICONINFORMATION );

        if( result == IDYES )
        {
            if( RemoveRegistryKey() )
            {
                MessageBox( NULL,
                            L"Registry modifications were successfully removed.",
                            L"Installer",
                            MB_OK | MB_ICONINFORMATION );
            }
            else
            {
                MessageBox( NULL,
                            L"There was an error modifying the registry.\r\n\r\n"
                            L"Some modifications to the registry may still exist.",
                            L"Installer",
                            MB_OK | MB_ICONERROR );
            }
            return 0;
        }
    }
    else
    {
        int result = MessageBox( NULL,
                                 L"This faux installer will need to write to the registry "
                                 L"to permit the Autorun application to launch the faux game.\r\n\r\n"
                                 L"Do you want to continue?\r\n\r\n"
                                 L"NOTE: You may run the faux installer manually to remove these "
                                 L"modifications from the registry.",
                                 L"Installer",
                                 MB_YESNO | MB_ICONINFORMATION );

        if( result == IDNO )
        {
            return 0;
        }
    }

    // Show dialog for installer.
    DialogBox( hInstance,
               MAKEINTRESOURCE( IDD_DIALOG ),
               NULL,
               DialogProc );

    if( !AddRegistryKey() )
    {
        MessageBox( NULL,
                    L"There was an error modifying the registry.\r\n\r\n"
                    L"The rest of sample may not function correctly.",
                    L"Installer",
                    MB_OK | MB_ICONERROR );
    }

    return 0;
}


//-----------------------------------------------------------------------------
// Message handler for the dialog.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM /* lParam */ )
{
    switch( Msg )
    {
        case WM_INITDIALOG:
        {
            ProcessInfo *processInfo = ProcessInfo::GetProcessInfo();

            // Update UI.
            SetWindowText( hDlg, L"Installer" );
            SetDlgItemText( hDlg, IDC_DESCRIPTION,
                            L"This is the faux installer for the Autorun Sample.\r\n\r\n"
                            L"If the Autorun application is working, you should have seen "
                            L"a UAC prompt for elevation before this application started.  "
                            L"Confirm below that this application is elevated and running "
                            L"under the account of the user who supplied credentials for elevation." );

            WCHAR szBuffer[512];
            if( processInfo )
            {
                swprintf_s( szBuffer, L"This process is %s and is running as %s.",
                            ( processInfo->Elevated() ? L"elevated" : L"not elevated" ),
                            processInfo->UserName() );
                
                delete processInfo;
            }
            else
            {
                swprintf_s( szBuffer, L"Unable to determine run level and user account." );
            }
            SetDlgItemText( hDlg, IDC_EDIT, szBuffer );
        }
        return TRUE;
        
        case WM_COMMAND:
        {
            if( wParam == IDOK || wParam == IDCANCEL )
            {
                EndDialog( hDlg, ( INT_PTR )wParam );
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Determines if the registry contains the key for the sample.
//-----------------------------------------------------------------------------
BOOL DoesRegistryKeyExist()
{
    WCHAR szInstallFolder[MAX_PATH] = { 0, };
    DWORD dwSize = sizeof( szInstallFolder );
    HKEY hKey = NULL;
    LONG lResult;
    DWORD dwType = 0;

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szRegistryKey, 0, KEY_READ, &hKey );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    lResult = RegQueryValueEx( hKey, L"InstallFolder", 0, &dwType, ( BYTE* )szInstallFolder, &dwSize );

    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult )
        return FALSE;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Updates the registry for the sample.
//-----------------------------------------------------------------------------
BOOL AddRegistryKey()
{
    WCHAR szInstallFolder[MAX_PATH];
    if( !GetModuleFileName( GetModuleHandle( NULL ), szInstallFolder, ARRAYSIZE( szInstallFolder ) ) )
        return FALSE;
    PathRemoveFileSpec( szInstallFolder );

    HKEY hKey = NULL;
    LONG lResult;

    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, c_szRegistryKey, 0, NULL, 0,
                              KEY_ALL_ACCESS, NULL, &hKey, NULL );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    DWORD dwSize = ( DWORD )( wcslen( szInstallFolder ) + 1 ) * sizeof( WCHAR );
    lResult = RegSetValueEx( hKey, L"InstallFolder", 0, REG_SZ, ( BYTE* )szInstallFolder, dwSize );

    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult )
        return FALSE;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Cleans up the registry modifications for the sample.
//-----------------------------------------------------------------------------
BOOL RemoveRegistryKey()
{
    LONG lResult;

    lResult = RegDeleteKeyEx( HKEY_LOCAL_MACHINE, c_szRegistryKey, 0, 0 );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    lResult = RegDeleteKeyEx( HKEY_LOCAL_MACHINE, c_szRegistryKeyRoot, 0, 0 );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    return TRUE;
}
