//-----------------------------------------------------------------------------
// File: ProcessInfo.h
//
// Declaration of the process information class.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _PROCESS_INFO_H_
#define _PROCESS_INFO_H_


//-----------------------------------------------------------------------------
// Name: ProcessInfo
// Desc: This class encapsulates information about the current process.
//-----------------------------------------------------------------------------
class ProcessInfo
{
public:

    // Static construction.
    static ProcessInfo* GetProcessInfo();

    // Accessors.
    const WCHAR * UserName() const { return m_szUserName; }
    BOOL Elevated() const { return m_bElevated; }

private:

    // Construction.
    ProcessInfo() { };

    // Internal data.
    WCHAR m_szUserName[MAX_PATH];
    BOOL m_bElevated;

};

#endif /* _PROCESS_INFO_H_ */
