//-------------------------------------------------------------------------------------
//  ExportLog.h
//
//  Classes and interfaces for a DCC-independent pluggable message logging system.
//  The system supports warnings, errors, and different levels of message logging.
//  Two log listeners are implemented here - a debug spew listener and a file listener.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

class ILogListener
{
public:
    virtual VOID LogMessage( const CHAR* strMessage ) = NULL;
    virtual VOID LogWarning( const CHAR* strMessage ) { LogMessage( strMessage ); }
    virtual VOID LogError( const CHAR* strMessage ) { LogMessage( strMessage ); }
    virtual VOID LogCommand( DWORD dwCommand, VOID* pData ) { UNREFERENCED_PARAMETER( dwCommand ); UNREFERENCED_PARAMETER( pData ); }
};

class DebugSpewListener : public ILogListener
{
public:
    virtual VOID LogMessage( const CHAR* strMessage )
    {
        OutputDebugStringA( strMessage );
        OutputDebugStringA( "\n" );
    }
};

class FileListener : public ILogListener
{
public:
    FileListener();
    ~FileListener();
    VOID StartLogging( const CHAR* strFileName );
    VOID StopLogging();
    virtual VOID LogMessage( const CHAR* strMessage );
protected:
    HANDLE      m_hFileHandle;
};

class ExportLog
{
public:
    enum LogCommands
    {
        ELC_CLEAR = 0,
        ELC_STARTEXPORT,
        ELC_ENDEXPORT
    };

    static VOID EnableLogging( BOOL bEnable );
    static VOID SetLogLevel( UINT uLevel );
    static UINT GetLogLevel();
    static VOID AddListener( ILogListener* pListener );
    static VOID ClearListeners();

    static VOID GenerateLogReport( BOOL bEchoWarningsAndErrors = TRUE );
    static VOID ResetCounters();

    static VOID LogCommand( DWORD dwCommand, VOID* pData = NULL );
    static VOID LogError( const CHAR* strFormat, ... );
    static VOID LogWarning( const CHAR* strFormat, ... );
    static VOID LogMsg( UINT uImportance, const CHAR* strFormat, ... );
};

};
