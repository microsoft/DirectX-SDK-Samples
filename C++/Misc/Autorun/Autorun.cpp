//-----------------------------------------------------------------------------
// File: Autorun.cpp
//
// The program entry point for the Autorun sample.
// Implementation of the base class for the Autorun functionality.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Common.h"
#include "Autorun.h"
#include "AutorunMsi.h"
#include "resource.h"
#include <new>
using std::nothrow;


// Older SDKs do not define this flag.
#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
#endif


//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );


//-----------------------------------------------------------------------------
// Program entry point.
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, LPWSTR, int )
{
    Settings settings;

    // Use a mutex to force a single instance.
    HANDLE hMutex = CreateMutex( NULL, TRUE, settings.AutorunMutex() );
    DWORD dwError = GetLastError();
    if( ERROR_ACCESS_DENIED == dwError || ERROR_ALREADY_EXISTS == dwError )
    {
        // Another instance is running.
        if( hMutex )
        {
            CloseHandle( hMutex );
        }
        return 0;
    }

    // Select the correct autorun functionality.
    Autorun* pAutorun = NULL;
    if( settings.UseMsi() )
        pAutorun = new(nothrow) AutorunMsi( settings );
    else
        pAutorun = new(nothrow) Autorun( settings );
    if( !pAutorun )
    {
        // Out of memory.
        return 0;
    }

    // Show dialog for autorun.
    DialogBoxParam( hInstance,
                    MAKEINTRESOURCE( IDD_AUTORUN ),
                    NULL,
                    DialogProc,
                    ( LPARAM )pAutorun );

    // Cleanup.
    delete pAutorun;
    if( hMutex )
    {
        ReleaseMutex( hMutex );
        CloseHandle( hMutex );
    }
    
    return 0;
}


//-----------------------------------------------------------------------------
// Message handler for the dialog.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
    // Static data used for the dialog.
    static Autorun* pAutorun = NULL;
    static HWND hInstall = NULL;
    static HWND hPlay = NULL;
    static BOOL bInstalled = FALSE;

    switch( Msg )
    {
        case WM_INITDIALOG:
        {
            // Get handles to the dialog buttons.
            hInstall = GetDlgItem( hDlg, IDC_INSTALL );
            hPlay = GetDlgItem( hDlg, IDC_PLAY );

            // Set the icon for the dialog.
            HICON hIcon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_AUTORUN ) );
            SendMessage( hDlg, WM_SETICON, ICON_BIG, ( LPARAM )hIcon );

            // Get the autorun functionality.
            pAutorun = ( Autorun* )lParam;
            if( pAutorun )
            {
                bInstalled = pAutorun->IsGameInstalled();
            }
            
            // Force an update of the UI.
            SendMessage( hDlg, WM_UPDATEUISTATE, 0, 0 );
        }
        return FALSE;
        
        case WM_UPDATEUISTATE:
        {
            // Enable/disable buttons and adjust focus based on whether the game is installed.
            EnableWindow( hInstall, !bInstalled );
            EnableWindow( hPlay, bInstalled );

            if( bInstalled )
            {
                Button_SetStyle( hInstall, 0, TRUE );
                Button_SetStyle( hPlay, BS_DEFPUSHBUTTON, TRUE );
                SetFocus( hPlay );
            }
            else
            {
                Button_SetStyle( hInstall, BS_DEFPUSHBUTTON, TRUE );
                Button_SetStyle( hPlay, 0, TRUE );
                SetFocus( hInstall );
            }
        }
        return TRUE;
        
        case WM_COMMAND:
        {
            switch( LOWORD( wParam ) )
            {
                case IDC_INSTALL:
                {
                    if( !pAutorun )
                        return FALSE;
                
                    if( pAutorun->InstallGame() )
                    {
                        // Periodically check if game is installed.
                        SetTimer( hDlg, 0, 1000, NULL );
                    }
                    else
                    {
                        // Error launching installer... user may have
                        //   the wrong disc in the drive.
                        MessageBox( hDlg,
                                    L"Installer could not be launched.\n"
                                    L"Please ensure that the installation disc is in the drive.",
                                    L"Autorun",
                                    MB_ICONWARNING | MB_OK );
                    }
                }
                return TRUE;
                
                case IDC_PLAY:
                {
                    if( !pAutorun )
                        return FALSE;

                    if( pAutorun->LaunchGame() )
                    {
                        // Done.
                        EndDialog( hDlg, 0 );
                    }
                    else
                    {
                        // Error launching game... installation
                        //   may have been corrupted.
                        MessageBox( hDlg,
                                    L"Game could not be launched.\n"
                                    L"Please try reinstalling the game.",
                                    L"Autorun",
                                    MB_ICONWARNING | MB_OK );

                        // Enable the install button.
                        EnableWindow( GetDlgItem( hDlg, IDC_INSTALL ), TRUE );
                    }
                }
                return TRUE;
                
                case IDCANCEL:
                {
                    EndDialog( hDlg, 0 );
                }
                return TRUE;
            }
        }
        break;
        
        case WM_TIMER:
        {
            if( !pAutorun )
                return FALSE;

            // Check on installation status.
            bInstalled = pAutorun->IsGameInstalled();

            if( bInstalled )
            {
                // Update the UI now that the game is installed.
                SendMessage( hDlg, WM_UPDATEUISTATE, 0, 0 );

                // Stop checking.
                KillTimer( hDlg, 0 );

                if( pAutorun->GetSettings().AutorunAfterInstall() )
                {
                    // Automatically run the game.
                    SendMessage( hDlg, WM_COMMAND, IDC_PLAY, 0 );
                }
            }
        }        
        return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
Autorun::Autorun( const Settings& settings ) : m_Settings( settings ), m_bIsInstalled( FALSE )
{
}


//-----------------------------------------------------------------------------
// Determine if the game is installed.
//-----------------------------------------------------------------------------
BOOL Autorun::IsGameInstalled()
{
    if( m_Settings.WaitForInstaller() )
    {
        return m_bIsInstalled;
    }
    else
    {
        // If game executable exists, the game is installed.
        WCHAR szExePath[MAX_PATH];
        if( !GetGameExecutablePath( szExePath, ARRAYSIZE( szExePath ) ) )
            return FALSE;

        return PathFileExists( szExePath );
    }
}


//-----------------------------------------------------------------------------
// Obtain the path to the game executable.
//-----------------------------------------------------------------------------
BOOL Autorun::GetGameExecutablePath( WCHAR* szExePath, DWORD cchExePath )
{
    // Get the installation folder from the registry.
    WCHAR szInstallFolder[MAX_PATH] = { 0, };
    DWORD dwSize = sizeof( szInstallFolder );
    HKEY hKey = NULL;
    LONG lResult;
    DWORD dwType = 0;

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, m_Settings.RegistryKey(), 0, KEY_READ, &hKey );
    if( ERROR_SUCCESS != lResult )
        return FALSE;

    // Read the value, but ensure the string is null-terminated.
    lResult = RegQueryValueEx( hKey, L"InstallFolder", 0, &dwType, ( BYTE* )szInstallFolder, &dwSize );
    szInstallFolder[MAX_PATH - 1] = L'\0';

    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult || ( REG_SZ != dwType && REG_EXPAND_SZ != dwType ) )
        return FALSE;

    // Expand string, if needed.
    if( REG_EXPAND_SZ == dwType )
    {
        DWORD cchTemp = ExpandEnvironmentStrings( szInstallFolder, NULL, 0 );
        WCHAR *szTemp = new(nothrow) WCHAR[cchTemp];
        if( !szTemp )
        {
            // Out of memory.
            return FALSE;
        }

        ExpandEnvironmentStrings( szInstallFolder, szTemp, cchTemp );
        wcscpy_s( szInstallFolder, szTemp );
        delete [] szTemp;
    }

    // Construct the complete path.
    swprintf_s( szExePath, cchExePath, L"%s\\%s", szInstallFolder, m_Settings.GameExe() );
    return TRUE;
}


//-----------------------------------------------------------------------------
// Launch the installer for the game.
//-----------------------------------------------------------------------------
BOOL Autorun::InstallGame()
{
    WCHAR szPath[MAX_PATH];
 
    if( !FindFile( m_Settings.InstallerExe(), szPath, ARRAYSIZE( szPath ) ) )
        return FALSE;

    if( m_Settings.WaitForInstaller() )
    {
        // Spawn installer and wait for it to finish.
        DWORD dwExitCode = 0;
        BOOL bResult = SpawnProcessAndWait( szPath, m_Settings.InstallerArgs(), &dwExitCode );
        
        // Check the exit code of the installer for indication of successful installation.
        if( bResult && ( dwExitCode == 0 ) )
            m_bIsInstalled = TRUE;
        
        return bResult;
    }
    else
    {
        // Spawn installer and check for successful installation later.
        return SpawnProcess( szPath, m_Settings.InstallerArgs() );
    }
}


//-----------------------------------------------------------------------------
// Launch the game.
//-----------------------------------------------------------------------------
BOOL Autorun::LaunchGame()
{
    WCHAR szExePath[MAX_PATH];
    
    if( !GetGameExecutablePath( szExePath, ARRAYSIZE( szExePath ) ) )
        return FALSE;

    return SpawnProcess( szExePath, m_Settings.GameArgs() );
}


//-----------------------------------------------------------------------------
// Finds the specified file.
//-----------------------------------------------------------------------------
BOOL Autorun::FindFile( const WCHAR* szFile, WCHAR* szPath, DWORD cchPath )
{
    if( !szFile || !szPath )
        return FALSE;

    WCHAR szTemp[MAX_PATH];

    // Try the current directory.    
    GetCurrentDirectory( ARRAYSIZE( szTemp ), szTemp );
    wcscat_s( szTemp, L"\\" );
    wcscat_s( szTemp, szFile );
    if( PathFileExists( szTemp ) )
    {
        wcscpy_s( szPath, cchPath, szTemp );
        return TRUE;
    }

    // Try the directory of the running process.
    GetModuleFileName( NULL, szTemp, ARRAYSIZE( szTemp ) );
    PathRemoveFileSpec( szTemp );
    wcscat_s( szTemp, L"\\" );
    wcscat_s( szTemp, szFile );
    if( PathFileExists( szTemp ) )
    {
        wcscpy_s( szPath, cchPath, szTemp );
        return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Runs the specified program.
//-----------------------------------------------------------------------------
BOOL Autorun::SpawnProcess( const WCHAR* szExePath, const WCHAR* szExeArgs )
{
    if( !szExePath )
        return FALSE;

    // NOTE: szExeArgs can be NULL.

    // Get working directory from executable path.    
    WCHAR szDirectory[MAX_PATH];
    wcscpy_s( szDirectory, szExePath );
    PathRemoveFileSpec( szDirectory );

    // ShellExecute or ShellExecuteEx must be used instead of CreateProcess
    // to permit the shell to display a UAC prompt asking for consent to
    // elevate when the target executable's manifest specifies a run level
    // of "requireAdministrator".
    //
    // You can only use CreateProcess if you know that the application you
    // are spawning will be at the same run level as the current process.
    // Otherwise, you will receive ERROR_ACCESS_DENIED if the elevation
    // consent could not be obtained.
    SHELLEXECUTEINFO info;
    ZeroMemory( &info, sizeof( info ) );
    info.cbSize = sizeof( info );
    info.lpVerb = L"open";
    info.fMask = SEE_MASK_FLAG_NO_UI;
    info.lpFile = szExePath;
    info.lpParameters = szExeArgs;
    info.lpDirectory = szDirectory;
    info.nShow = SW_SHOW;
    if( !ShellExecuteEx( &info ) )
        return FALSE;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Runs the specified program and waits for it to exit.
//-----------------------------------------------------------------------------
BOOL Autorun::SpawnProcessAndWait( const WCHAR *szExePath, const WCHAR *szExeArgs, DWORD *pdwExitCode )
{
    if( !szExePath )
        return FALSE;

    // NOTE: szExeArgs can be NULL.
    // NOTE: pExitCode can be NULL.

    // Get working directory from executable path.    
    WCHAR szDirectory[MAX_PATH];
    wcscpy_s( szDirectory, szExePath );
    PathRemoveFileSpec( szDirectory );

    // See SpawnProcess for information why ShellExecute or ShellExecuteEx
    // must be used instead of CreateProcess.
    SHELLEXECUTEINFO info;
    ZeroMemory( &info, sizeof( info ) );
    info.cbSize = sizeof( info );
    info.lpVerb = L"open";
    info.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    info.lpFile = szExePath;
    info.lpParameters = szExeArgs;
    info.lpDirectory = szDirectory;
    info.nShow = SW_SHOW;
    if( !ShellExecuteEx( &info ) )
        return FALSE;
    
    // Wait for process to finish.
    WaitForSingleObject( info.hProcess, INFINITE );
    
    // Return exit code from process, if requested by caller.
    if( pdwExitCode )
        GetExitCodeProcess( info.hProcess, pdwExitCode );

    CloseHandle( info.hProcess );
    return TRUE;
}