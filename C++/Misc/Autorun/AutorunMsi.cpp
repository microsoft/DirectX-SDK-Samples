//-----------------------------------------------------------------------------
// File: AutorunMsi.cpp
//
// Implementation of the MSI-specific Autorun functionality.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "Common.h"
#include "AutorunMsi.h"

// The Microsoft Platform SDK or Microsoft Windows SDK is required to compile this sample.
#include <msi.h>
#include <msiquery.h>


//-----------------------------------------------------------------------------
AutorunMsi::AutorunMsi( const Settings& settings ) : Autorun( settings )
{
}


//-----------------------------------------------------------------------------
// Determine if the game is installed.
//-----------------------------------------------------------------------------
BOOL AutorunMsi::IsGameInstalled()
{
    INSTALLSTATE state = MsiQueryProductState( m_Settings.MsiProductCode() );
    if( INSTALLSTATE_DEFAULT == state )
        return TRUE;
    else
        return FALSE;
}


//-----------------------------------------------------------------------------
// Obtain the path to the game executable.
//-----------------------------------------------------------------------------
BOOL AutorunMsi::GetGameExecutablePath( WCHAR* szExePath, DWORD cchExePath )
{
    if( !szExePath )
        return FALSE;

    // Get the installation folder from MSI.
    WCHAR szInstallFolder[MAX_PATH];
    DWORD dwLength = ARRAYSIZE( szInstallFolder );

    UINT iResult = MsiGetProductInfo( m_Settings.MsiProductCode(),
                                       INSTALLPROPERTY_INSTALLLOCATION,
                                       szInstallFolder,
                                       &dwLength );
    if( ERROR_SUCCESS != iResult )
        return FALSE;

    // Construct the complete path.
    swprintf_s( szExePath, cchExePath, L"%s\\%s", szInstallFolder, m_Settings.GameExe() );
    return TRUE;
}
