//-----------------------------------------------------------------------------
// File: AutorunMsi.h
//
// Declaration of the MSI-specific Autorun functionality.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _AUTORUN_MSI_H_
#define _AUTORUN_MSI_H_

#include "Autorun.h"


//-----------------------------------------------------------------------------
// Name: AutorunMsi
// Desc: This class encapsulates the MSI-specific functionality for the
//       Autorun application.  Use this class when your installer is MSI-based.
//-----------------------------------------------------------------------------
class AutorunMsi : public Autorun
{
public:

    AutorunMsi( const Settings &settings );
    virtual ~AutorunMsi() { };

    virtual BOOL IsGameInstalled();
    virtual BOOL GetGameExecutablePath( WCHAR *szExePath, DWORD cchExePath );

    // Use inherited versions...
    //virtual BOOL InstallGame();
    //virtual BOOL LaunchGame();
};

#endif /* _AUTORUN_MSI_H_ */
