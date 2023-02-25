//-----------------------------------------------------------------------------
// File: ProcessInfo.cpp
//
// Implementation of the process information class.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Common.h"
#include "ProcessInfo.h"
#include <new>
using std::nothrow;


//-----------------------------------------------------------------------------
ProcessInfo* ProcessInfo::GetProcessInfo()
{
    BOOL bResult = FALSE;

    // Get process token.
    HANDLE hToken = NULL;
    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken ) || !hToken )
        return NULL;

    // Get user information from the process token.
    DWORD dwBytes = 0;
    GetTokenInformation( hToken, TokenUser, NULL, 0, &dwBytes );
    TOKEN_USER* tokenUser = ( TOKEN_USER* )( new(nothrow) BYTE[dwBytes] );
    if( !GetTokenInformation( hToken, TokenUser, tokenUser, dwBytes, &dwBytes ) )
    {
        delete [] tokenUser;
        CloseHandle( hToken );
        return NULL;
    }

    // Convert the user SID to a friendly name.
    WCHAR szUserName[MAX_PATH];
    WCHAR szDomain[MAX_PATH];
    DWORD cchUserName = ARRAYSIZE( szUserName );
    DWORD cchDomain = ARRAYSIZE( szDomain );
    SID_NAME_USE use;

    bResult = LookupAccountSid( NULL, tokenUser->User.Sid, szUserName, &cchUserName,
                                szDomain, &cchDomain, &use );
    delete [] tokenUser;
    
    if( !bResult )
    {
        CloseHandle( hToken );
        return NULL;
    }

    // Get elevation information from the process token.
    TOKEN_ELEVATION tokenElevation;
    bResult = GetTokenInformation( hToken, TokenElevation, &tokenElevation,
                                   sizeof( tokenElevation ), &dwBytes );
    CloseHandle( hToken );

    if( !bResult )
        return NULL;

    // Return information to caller.
    ProcessInfo* processInfo = new(nothrow) ProcessInfo();
    if( processInfo )
    {
        wcscpy_s( processInfo->m_szUserName, szDomain );
        wcscat_s( processInfo->m_szUserName, L"\\" );
        wcscat_s( processInfo->m_szUserName, szUserName );

        processInfo->m_bElevated = ( tokenElevation.TokenIsElevated ? TRUE : FALSE );
    }
    
    return processInfo;
}
