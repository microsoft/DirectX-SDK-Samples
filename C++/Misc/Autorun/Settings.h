//-----------------------------------------------------------------------------
// File: Settings.h
//
// Declaration of the applications settings class.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _SETTINGS_H_
#define _SETTINGS_H_


//-----------------------------------------------------------------------------
// Name: Settings
// Desc: This class encapsulates the application settings found in the
//       string table resource.
//-----------------------------------------------------------------------------
class Settings
{
public:

    // Construction and destruction.
    Settings();
    ~Settings() { };

    // Accessors.
    const WCHAR * AutorunMutex() const { return m_szAutorunMutex; };
    const WCHAR * InstallerExe() const { return m_szInstallerExe; };
    const WCHAR * InstallerArgs() const { return m_szInstallerArgs; };
    const WCHAR * GameExe() const { return m_szGameExe; };
    const WCHAR * GameArgs() const { return m_szGameArgs; };
    BOOL UseMsi() const { return m_bUseMsi; };
    const WCHAR * MsiProductCode() const { return m_szMsiProductCode; };
    const WCHAR * RegistryKey() const { return m_szRegistryKey; };
    BOOL WaitForInstaller() const { return m_bAutorunAfterInstall; };
    BOOL AutorunAfterInstall() const { return m_bAutorunAfterInstall; };

private:

    // Internal methods.
    VOID GetString( UINT uID, WCHAR *szBuffer, DWORD cchBuffer );
    BOOL GetBool( UINT uID );
    
    // Internal data.
    WCHAR m_szAutorunMutex[MAX_PATH];
    WCHAR m_szInstallerExe[MAX_PATH];
    WCHAR m_szInstallerArgs[MAX_PATH];
    WCHAR m_szGameExe[MAX_PATH];
    WCHAR m_szGameArgs[MAX_PATH];
    BOOL m_bUseMsi;
    WCHAR m_szMsiProductCode[64]; // This needs to be big enough to hold a GUID.
    WCHAR m_szRegistryKey[MAX_PATH];
    BOOL m_bWaitForInstaller;
    BOOL m_bAutorunAfterInstall;
};

#endif /* _SETTINGS_H_ */
