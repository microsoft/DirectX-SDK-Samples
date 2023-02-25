//-----------------------------------------------------------------------------
// File: Autorun.h
//
// Declaration of the base class for the Autorun functionality.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _AUTORUN_H_
#define _AUTORUN_H_

#include "Settings.h"


//-----------------------------------------------------------------------------
// Name: Autorun
// Desc: This class encapsulates the base functionality for the Autorun
//       application: checking whether the game is installed, spawning the
//       installation of the game and launching the game itself.
//-----------------------------------------------------------------------------
class Autorun
{
public:

    // Construction and destruction.
    Autorun( const Settings &settings );
    virtual ~Autorun() { };

    // Overridable methods.
    virtual BOOL IsGameInstalled();
    virtual BOOL GetGameExecutablePath( WCHAR *szExePath, DWORD cchExePath );
    virtual BOOL InstallGame();
    virtual BOOL LaunchGame();

    // Accessors.    
    const Settings & GetSettings() const { return m_Settings; };
    
protected:

    // Internal methods.
    BOOL FindFile( const WCHAR *szFile, WCHAR *szPath, DWORD cchPath );
    BOOL SpawnProcess( const WCHAR *szExePath, const WCHAR *szExeArgs );
    BOOL SpawnProcessAndWait( const WCHAR *szExePath, const WCHAR *szExeArgs, DWORD *pdwExitCode );

    // Internal data.
    Settings m_Settings;
    BOOL m_bIsInstalled;

};

#endif /* _AUTORUN_H_ */
