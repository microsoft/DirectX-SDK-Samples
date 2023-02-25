//-----------------------------------------------------------------------------
// File: Settings.cpp
//
// Implementation of the applications settings class.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Common.h"
#include "Settings.h"
#include "resource.h"


//-----------------------------------------------------------------------------
Settings::Settings()
{
    GetString( IDS_AUTORUN_MUTEX, m_szAutorunMutex, ARRAYSIZE( m_szAutorunMutex ) );
    GetString( IDS_INSTALLER_EXE, m_szInstallerExe, ARRAYSIZE( m_szInstallerExe ) );
    GetString( IDS_INSTALLER_ARGS, m_szInstallerArgs, ARRAYSIZE( m_szInstallerArgs ) );
    GetString( IDS_GAME_EXE, m_szGameExe, ARRAYSIZE( m_szGameExe ) );
    GetString( IDS_GAME_ARGS, m_szGameArgs, ARRAYSIZE( m_szGameArgs ) );
    GetString( IDS_MSI_PRODUCT_CODE, m_szMsiProductCode, ARRAYSIZE( m_szMsiProductCode ) );
    GetString( IDS_REGISTRY_KEY, m_szRegistryKey, ARRAYSIZE( m_szRegistryKey ) );

    m_bUseMsi = GetBool( IDS_USE_MSI );
    m_bWaitForInstaller = GetBool( IDS_WAIT_FOR_INSTALLER );
    m_bAutorunAfterInstall = GetBool( IDS_AUTORUN_AFTER_INSTALL );
}


//-----------------------------------------------------------------------------
// Load a string value.
//-----------------------------------------------------------------------------
VOID Settings::GetString( UINT uID, WCHAR* szBuffer, DWORD cchBuffer )
{
    if( !szBuffer )
        return;

    if( !LoadString( GetModuleHandle( NULL ), uID, szBuffer, cchBuffer ) )
    {
        // Return empty string if ID could not be found.
        *szBuffer = L'\0';
    }
}


//-----------------------------------------------------------------------------
// Load a boolean value.
//-----------------------------------------------------------------------------
BOOL Settings::GetBool( UINT uID )
{
    WCHAR szTemp[32];
    GetString( uID, szTemp, ARRAYSIZE( szTemp ) );

    if( !_wcsicmp( szTemp, L"TRUE" ) || !_wcsicmp( szTemp, L"YES" ) )
        return TRUE;

    return FALSE;
}
