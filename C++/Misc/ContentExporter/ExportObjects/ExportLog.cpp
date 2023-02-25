//-------------------------------------------------------------------------------------
//  ExportLog.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportlog.h"

namespace ATG
{
    BOOL g_bLoggingEnabled = TRUE;
    UINT g_uLogLevel = 10;
    typedef std::list< ILogListener* > LogListenerList;
    LogListenerList g_Listeners;

    DWORD g_dwWarningCount = 0;
    DWORD g_dwErrorCount = 0;

    typedef std::list<const CHAR*> StringList;
    StringList      g_WarningsList;
    StringList      g_ErrorsList;

    CHAR g_strBuf[500];
    VOID BroadcastMessage( UINT uMessageType, const CHAR* strMsg );

    VOID ExportLog::AddListener( ILogListener* pListener )
    {
        g_Listeners.push_back( pListener );
    }

    VOID ExportLog::ClearListeners()
    {
        g_Listeners.clear();
    }

    VOID ExportLog::EnableLogging( BOOL bEnable )
    {
        g_bLoggingEnabled = bEnable;
    }

    VOID ExportLog::SetLogLevel( UINT uLevel )
    {
        g_uLogLevel = uLevel;
    }

    UINT ExportLog::GetLogLevel()
    {
        return g_uLogLevel;
    }

    VOID ExportLog::GenerateLogReport( BOOL bEchoWarningsAndErrors )
    {
        LogMsg( 0, "%d warning(s), %d error(s).", g_dwWarningCount, g_dwErrorCount );
        if( !bEchoWarningsAndErrors )
            return;

        StringList::iterator iter = g_WarningsList.begin();
        StringList::iterator end = g_WarningsList.end();
        while( iter != end )
        {
            BroadcastMessage( 1, *iter );
            ++iter;
        }

        iter = g_ErrorsList.begin();
        end = g_ErrorsList.end();
        while( iter != end )
        {
            BroadcastMessage( 2, *iter );
            ++iter;
        }
    }

    VOID ExportLog::ResetCounters()
    {
        StringList::iterator iter = g_WarningsList.begin();
        StringList::iterator end = g_WarningsList.end();
        while( iter != end )
        {
            delete[] *iter;
            ++iter;
        }
        g_WarningsList.clear();

        iter = g_ErrorsList.begin();
        end = g_ErrorsList.end();
        while( iter != end )
        {
            delete[] *iter;
            ++iter;
        }
        g_ErrorsList.clear();

        g_dwWarningCount = 0;
        g_dwErrorCount = 0;
    }

    VOID BroadcastMessage( UINT uMessageType, const CHAR* strMsg )
    {
        LogListenerList::iterator iter = g_Listeners.begin();
        LogListenerList::iterator end = g_Listeners.end();

        while( iter != end )
        {
            switch( uMessageType )
            {
            case 1:
                (*iter)->LogWarning( strMsg );
                break;
            case 2:
                (*iter)->LogError( strMsg );
                break;
            default:
                (*iter)->LogMessage( strMsg );
                break;
            }
            ++iter;
        }
    }


    VOID ExportLog::LogCommand( DWORD dwCommand, VOID* pData )
    {
        LogListenerList::iterator iter = g_Listeners.begin();
        LogListenerList::iterator end = g_Listeners.end();

        while( iter != end )
        {
            (*iter)->LogCommand( dwCommand, pData );
            ++iter;
        }
    }


    VOID ExportLog::LogMsg( UINT uImportance, const CHAR* strFormat, ... )
    {
        if( !g_bLoggingEnabled || ( uImportance > g_uLogLevel ) )
            return;
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( g_strBuf, strFormat, args );

        BroadcastMessage( 0, g_strBuf );
    }


    VOID RecordMessage( StringList& DestStringList, const CHAR* strMessage )
    {
        DWORD dwLength = (DWORD)strlen( strMessage );
        CHAR* strCopy = new CHAR[ dwLength + 1 ];
        strcpy_s( strCopy, dwLength + 1, strMessage );
        DestStringList.push_back( strCopy );
    }


    VOID ExportLog::LogError( const CHAR* strFormat, ... )
    {
        if( !g_bLoggingEnabled )
            return;

        ++g_dwErrorCount;

        strcpy_s( g_strBuf, "ERROR: " );
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( g_strBuf + 7, ARRAYSIZE( g_strBuf ) - 7, strFormat, args );

        RecordMessage( g_ErrorsList, g_strBuf );
        BroadcastMessage( 2, g_strBuf );
    }

    VOID ExportLog::LogWarning( const CHAR* strFormat, ... )
    {
        if( !g_bLoggingEnabled )
            return;

        ++g_dwWarningCount;

        strcpy_s( g_strBuf, "WARNING: " );
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( g_strBuf + 9, ARRAYSIZE( g_strBuf ) - 9, strFormat, args );

        RecordMessage( g_WarningsList, g_strBuf );
        BroadcastMessage( 1, g_strBuf );
    }

    FileListener::FileListener()
        : m_hFileHandle( INVALID_HANDLE_VALUE )
    {
    }

    FileListener::~FileListener()
    {
        StopLogging();
    }

    VOID FileListener::StartLogging( const CHAR* strFileName )
    {
        assert( m_hFileHandle == INVALID_HANDLE_VALUE );
        m_hFileHandle = CreateFile( strFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    }

    VOID FileListener::StopLogging()
    {
        if( m_hFileHandle != INVALID_HANDLE_VALUE )
            CloseHandle( m_hFileHandle );
        m_hFileHandle = INVALID_HANDLE_VALUE;
    }

    VOID FileListener::LogMessage( const CHAR* strMessage )
    {
        if( m_hFileHandle == INVALID_HANDLE_VALUE )
            return;
        DWORD dwByteCount = 0;
        WriteFile( m_hFileHandle, strMessage, (DWORD)strlen( strMessage ), &dwByteCount, NULL );
        const CHAR* strNewline = "\r\n";
        WriteFile( m_hFileHandle, strNewline, 2, &dwByteCount, NULL );
    }
}