//-------------------------------------------------------------------------------------
// File: TimeLimitAPI.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------
#define _WIN32_DCOM
#include "TimeLimitAPI.h"
#include <servprov.h>
#include <msxml2.h>
#include <rpcsal.h>
#include <oleauto.h>
#include <wbemidl.h>
#include <sddl.h>
#include <time.h>
#include <assert.h>
#include <sddl.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )

#pragma comment( lib, "wbemuuid.lib" )
#pragma comment( lib, "msxml2.lib" )

// anonymous namespace for internal linkage
namespace
{

// In case msxml2.h does not define CLSID_DOMDocument60
#if !defined( __DOMDocument60_FWD_DEFINED__ )
        class DECLSPEC_UUID( "88d96a05-f192-11d4-a65f-0040963251e5" ) DOMDocument60;
const CLSID CLSID_DOMDocument60 = __uuidof( DOMDocument60 );
#endif

// internal function prototypes
HRESULT Init();
HRESULT TimeRestrictionsEnabled( IWbemClassObject* pUserSettings, BOOL* pfEnabled );
HRESULT GetSidStringForCurrentUser( PWSTR* pszSID );
HRESULT GetUserSettings( IWbemServices* pWmiServices, PCWSTR pwszUserSID,
                         IWbemClassObject** ppUserSettings );
HRESULT ConnectToWpc( IWbemServices** ppWmiServices );
}

//-------------------------------------------------------------------------------------
// Name: TimeLimitGetTimeRemaining()
//-------------------------------------------------------------------------------------
HRESULT TimeLimitGetTimeRemaining( TimeRemaining* pTime )
{
    if( 0 == pTime )
        return E_INVALIDARG;

    // zero out the structure
    ZeroMemory( pTime, sizeof( TimeRemaining ) );

    // Initialize the com subsystem
    HRESULT hr = Init();
    if( SUCCEEDED( hr ) )
    {
        tm tmStruct = { 0 };
        time_t ltTime;
        time( &ltTime );

#ifdef __STDC_SECURE_LIB__
        // Make use of the secure CRT functions
#if defined(_AMD64_)
        if( 0 != _localtime64_s( &tmStruct, &ltTime ) )
#else
		__time32_t t32;
		if( 0 != _localtime32_s( &tmStruct, &t32 ) )
#endif
        {
            hr = E_FAIL;
        }
#else
        // Secure CRT functions are not available.
        const tm* pTM = localtime( &ltTime );
        if( 0 == pTM )
        {
            tmStruct = *pTM;
        }
        else
        {
            hr = E_FAIL;
        }
#endif
        if( SUCCEEDED( hr ) )
        {
            HourlyRestrictions restrictions;
            hr = TimeLimitGetHourlyRestrictions( &restrictions );
            if( SUCCEEDED( hr ) )
            {
                BOOL logout = FALSE;
                int hours = 0;
                int d = tmStruct.tm_wday;
                int h = tmStruct.tm_hour;
                while( FALSE == logout && hours < ( 7 * 24 ) )
                {
                    if( restrictions.days[d] & ( 1 << h ) )
                    {
                        // this hour is allowed
                        ++hours;
                        ++h;
                        if( h == 24 )   // next day, reset hour index and increment day index
                        {
                            h = 0;
                            ++d;
                            if( d == 7 )    // end of week, reset day index to Sunday
                            {
                                d = 0;
                            }
                        }
                    }
                    else
                    {
                        // this hour is not allowed, we're done
                        logout = TRUE;
                    }
                }

                // assuming we didn't scan the whole week (if we did, there are no
                // blocked hours), calculate the exact hours, minutes, and seconds
                // left before logout.
                if( hours < ( 7 * 24 ) )
                {
                    int minutes = tmStruct.tm_min;
                    int seconds = tmStruct.tm_sec;
                    if( seconds > 0 )
                    {
                        // if seconds are greater than zero, then we need to account
                        // the partial minute represented by the seconds component
                        ++minutes;
                    }
                    if( minutes > 0 )
                    {
                        // if minutes are greater than zero, then we need to account
                        // for the partial hour represented by the minutes, since
                        // we've counted the hours (rather than subtracting the
                        // current hour from some fixed amount) we subtract rather
                        // than add, as we did for minutes.
                        --hours;
                    }
                    // we're subtracting the current minutes and seconds from 60 to
                    // get the time left until the beginning of the next whole hour
                    // if minutes or seconds are zero, we make them 60 so that
                    // subtraction turns up 00, as it should.
                    if( 0 == minutes )
                    {
                        minutes = 60;
                    }
                    if( 0 == seconds )
                    {
                        seconds = 60;
                    }
                    if( hours >= 0 )
                    {
                        pTime->days = static_cast<unsigned char>( hours / 24 );
                        pTime->hours = static_cast<unsigned char>( hours % 24 );
                        pTime->minutes = static_cast<unsigned char>( 60 - minutes );
                        pTime->seconds = static_cast<unsigned char>( 60 - seconds );
                    }
                    else
                    {
                        // at this point, if the hour count is negative, then
                        // the user has no time remaining.
                        pTime->days = 0;
                        pTime->hours = 0;
                        pTime->minutes = 0;
                        pTime->seconds = 0;
                    }
                }
                else
                {
                    // scanned the whole week and no blocked hours wer found
                    hr = S_FALSE;
                }
            }
        }
    }
    return hr;
}

//-------------------------------------------------------------------------------------
// Name: TimeLimitGetSecondsRemaining()
//-------------------------------------------------------------------------------------
INT TimeLimitGetSecondsRemaining()
{
    TimeRemaining timeRemaining;
    HRESULT hr = TimeLimitGetTimeRemaining( &timeRemaining );
    if( hr != S_OK )
        return -1;

    return timeRemaining.seconds +
        ( timeRemaining.minutes * 60 ) +
        ( timeRemaining.hours * 3600 ) +
        ( timeRemaining.days * 86400 );
}

//-------------------------------------------------------------------------------------
// Name: TimeLimitGetHourlyRestrictions()
//-------------------------------------------------------------------------------------
HRESULT TimeLimitGetHourlyRestrictions( HourlyRestrictions* pRestrictions )
{
    // initialize the structure to all hours allowed
    for( int i = 0; i < 7; ++i )
    {
        pRestrictions->days[i] = 0x00ffffff;
    }

    HRESULT hr = S_OK;
    if( NULL == pRestrictions )
    {
        hr = E_INVALIDARG;
    }

    hr = Init();
    if( SUCCEEDED( hr ) )
    {
        IWbemServices* pWmi = NULL;
        hr = ConnectToWpc( &pWmi );
        if( SUCCEEDED( hr ) )
        {
            // get the user settings instance
            IWbemClassObject* pSettings = NULL;
            PWSTR pwszSID = NULL;
            hr = GetSidStringForCurrentUser( &pwszSID );
            if( SUCCEEDED( hr ) )
            {
                hr = GetUserSettings( pWmi, pwszSID, &pSettings );
                if( SUCCEEDED( hr ) )
                {
                    // determine if time restrictiosn are enabled
                    BOOL fTimeRestrictions = FALSE;
                    hr = TimeRestrictionsEnabled( pSettings, &fTimeRestrictions );
                    if( SUCCEEDED( hr ) && fTimeRestrictions )
                    {
                        VARIANT var;
                        VariantInit( &var );

                        // get the actual logon hours data from the user settings
                        hr = pSettings->Get( L"LogonHours", 0, &var, NULL, NULL );
                        if( SUCCEEDED( hr ) && var.vt == ( VT_I4 | VT_ARRAY ) )
                        {
                            long lUpper = 0;
                            long lLower = 0;
                            hr = SafeArrayGetUBound( var.parray, 1, &lUpper );
                            if( SUCCEEDED( hr ) )
                            {
                                hr = SafeArrayGetLBound( var.parray, 1, &lLower );
                                if( SUCCEEDED( hr ) )
                                {
                                    long* pData;
                                    hr = SafeArrayAccessData( var.parray, ( void HUGEP** )( &pData ) );
                                    if( SUCCEEDED( hr ) )
                                    {
                                        for( long i = lLower; i <= lUpper; ++i )
                                        {
                                            pRestrictions->days[i] = pData[i];
                                        }
                                        SafeArrayUnaccessData( var.parray );
                                    }
                                }
                            }
                        }
                        VariantClear( &var );
                    }

                    if( FALSE == fTimeRestrictions )
                    {
                        hr = E_FAIL;
                    }
                    // release the settings instance
                    pSettings->Release();
                }
                LocalFree( pwszSID );
                pwszSID = NULL;
            }

            // release the WMI connection
            pWmi->Release();
        }
    }
    return hr;
}

//-------------------------------------------------------------------------------------
// Internal Function Implementations
namespace
{
//---------------------------------------------------------------------------------
// Name: Init
// Desc: Initializes COM subsystem.  A successful intialization will also result
//       in COM shutdown at end of program.
//---------------------------------------------------------------------------------
HRESULT Init()
{
    // The following local structure is responsible for initializing
    // and cleaning up of COM resources.
    struct WpcComLifetime
    {
WpcComLifetime() : hr( CoInitializeEx( 0, COINIT_MULTITHREADED ) )
{
    if( SUCCEEDED( hr ) )
    {
        // Set COM security levels
        hr = CoInitializeSecurity( NULL,                        // Security Descriptor
                                   -1,                          // COM authentication
                                   NULL,                        // Authentication services
                                   NULL,                        // Reserved
                                   RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
                                   RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
                                   NULL,                        // Authentication info
                                   EOAC_NONE,                   // Additional capabilities
                                   NULL                         // Reserved
                                   );
    }
}
~WpcComLifetime()
{
    if( SUCCEEDED( hr ) )
    {
        // if( g_hSubWinlogon ) EvtClose( g_hSubWinlogon );    // close outstanding subscription
        CoUninitialize();
    }
}
        HRESULT hr; // holds HRESULT of com initialization
    };

    static WpcComLifetime s_lifetime;  // constructed on first access, destroyed at end of program
    return s_lifetime.hr;
}

//---------------------------------------------------------------------------------
// Name: TimeRestrictionsEnabled()
// Desc: Reads the user settings to determine if time restrictions are turned on
//       for the user.  pfEnabled returns TRUE if they are, FALSE otherwise.
//---------------------------------------------------------------------------------
HRESULT TimeRestrictionsEnabled( IWbemClassObject* pUserSettings, BOOL* pfEnabled )
{
    BOOL fResult = FALSE;
    VARIANT var;
    VariantInit( &var );
    HRESULT hr = pUserSettings->Get( L"HourlyRestrictions", 0, &var, NULL, NULL );
    if( SUCCEEDED( hr ) && var.vt == VT_BOOL )
    {
        fResult = var.boolVal;
    }
    VariantClear( &var );
    *pfEnabled = fResult;
    return hr;
}

//---------------------------------------------------------------------------------
// Name: GetHourlyRestrictions()
// Desc: Retrieves the logon hours array from the user settings instance and stores
//       the bit flag values (one for each day of the week) in the
//       pRestrictions structure.
//---------------------------------------------------------------------------------
HRESULT GetHourlyRestrictions( IWbemClassObject* pUserSettings, HourlyRestrictions* pRestrictions )
{
    HRESULT hr = S_OK;
    if( NULL == pUserSettings || NULL == pRestrictions )
    {
        hr = E_INVALIDARG;
    }

    VARIANT var;
    VariantInit( &var );

    // initialize the structure to all hours allowed
    for( int i = 0; i < 7; ++i )
    {
        pRestrictions->days[i] = 0xffffffff;
    }

    // get the actual logon hours data from the user settings
    hr = pUserSettings->Get( L"LogonHours", 0, &var, NULL, NULL );
    if( SUCCEEDED( hr ) && var.vt == ( VT_I4 | VT_ARRAY ) )
    {
        long lUpper = 0;
        long lLower = 0;
        hr = SafeArrayGetUBound( var.parray, 1, &lUpper );
        if( SUCCEEDED( hr ) )
        {
            hr = SafeArrayGetLBound( var.parray, 1, &lLower );
            if( SUCCEEDED( hr ) )
            {
                long* pData;
                hr = SafeArrayAccessData( var.parray, ( void HUGEP** )( &pData ) );
                if( SUCCEEDED( hr ) )
                {
                    for( long i = lLower; i <= lUpper; ++i )
                    {
                        pRestrictions->days[i] = pData[i];
                    }
                    SafeArrayUnaccessData( var.parray );
                }
            }
        }
    }
    VariantClear( &var );
    return hr;
}

//---------------------------------------------------------------------------------
// Name: GetSidStringForCurrentUser()
// Desc: Retrieves the SID in string form for the user of this program.
//---------------------------------------------------------------------------------
HRESULT GetSidStringForCurrentUser( PWSTR* ppwszSID )
{
    HRESULT hr = S_OK;
    if( !ppwszSID )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        HANDLE hToken = NULL;
        // Obtain token handle.  Try the thread token first
        if( !OpenThreadToken( GetCurrentThread(), TOKEN_READ, FALSE, &hToken ) )
        {
            DWORD dwErr = GetLastError();
            if( ERROR_NO_TOKEN == dwErr )
            {
                // Try the process token
                if( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32( dwErr );
            }
        }

        if( SUCCEEDED( hr ) )
        {
            TOKEN_USER* pTokenUser = NULL;
            DWORD cbToken = 0;
            // Call twice.  Call first time with NULL pTokenUser and 0 length to
            // retrieve required buffer length.  Returns non-zero (normally failure)
            if( !GetTokenInformation( hToken, TokenUser, pTokenUser, 0, &cbToken ) )
            {
                pTokenUser = ( TOKEN_USER* )LocalAlloc( LMEM_FIXED, cbToken );
                if( !pTokenUser )
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    // Call second time with required buffer
                    if( !GetTokenInformation( hToken, TokenUser, pTokenUser, cbToken, &cbToken ) )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                    }
                    else
                    {
                        // Convert PSID to SID string
                        if( !ConvertSidToStringSidW( pTokenUser->User.Sid, ppwszSID ) )
                        {
                            hr = HRESULT_FROM_WIN32( GetLastError() );
                        }
                    }
                    LocalFree( pTokenUser );
                }
            }
        }
        if( hToken )
        {
            CloseHandle( hToken );
        }
    }
    return hr;
}

//---------------------------------------------------------------------------------
// Name: GetUserSettings()
// Desc: Takes a IWbemServices object and a SID in string form and retreives the
//       WPC user settings WMI instance for that user (if it exists).  Caller is
//       responsible for releasing the object.
//---------------------------------------------------------------------------------
HRESULT GetUserSettings( IWbemServices* pWmiServices, PCWSTR pwszUserSID,
                         IWbemClassObject** ppUserSettings )
{
    HRESULT hr = S_OK;
    if( NULL == ppUserSettings || NULL == pwszUserSID )
    {
        hr = E_INVALIDARG;
    }
    if( SUCCEEDED( hr ) )
    {
        WCHAR szBuffer[MAX_PATH];
        hr = StringCchPrintf( szBuffer, MAX_PATH, L"WpcUserSettings.SID=\"%s\"", pwszUserSID );
        if( SUCCEEDED( hr ) )
        {
            BSTR bstrInstancePath = SysAllocString( szBuffer );
            if( !bstrInstancePath )
            {
                hr = E_OUTOFMEMORY;
            }
            if( SUCCEEDED( hr ) )
            {
                hr = pWmiServices->GetObject( bstrInstancePath,
                                              WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                              NULL,
                                              ppUserSettings,
                                              NULL
                                              );
                SysFreeString( bstrInstancePath );
            }
        }
    }
    return hr;
}

//---------------------------------------------------------------------------------
// Name: ConnectToWpc()
// Desc: Connect to the Windows Parental Controls WMI provider.
//       ppWmiServices will contain a IWbemServices object which can be used to
//       get systems settings and user settings.  Caller is responsible for
//       releasing this object.
//---------------------------------------------------------------------------------
HRESULT ConnectToWpc( IWbemServices** ppWmiServices )
{
    HRESULT hr = S_OK;
    if( NULL == ppWmiServices )
    {
        hr = E_INVALIDARG;
    }
    if( SUCCEEDED( hr ) )
    {
        IWbemLocator* pLocator = NULL;
        hr = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, ( LPVOID* )&pLocator );
        if( SUCCEEDED( hr ) )
        {
            BSTR bstrNamespace = SysAllocString( L"ROOT\\CIMV2\\APPLICATIONS\\WINDOWSPARENTALCONTROLS" );
            if( !bstrNamespace )
            {
                hr = E_OUTOFMEMORY;
            }
            if( SUCCEEDED( hr ) )
            {
                hr = pLocator->ConnectServer(
                    bstrNamespace,  // Object path of WMI namespace
                    NULL,           // User name. NULL = current user
                    NULL,           // User password. NULL = current user
                    0,              // Locale.  0 indicates current
                    NULL,           // Security flags.
                    0,              // Authority (e.g. Kerberos)
                    0,              // Context object
                    ppWmiServices   // pointer to IWbemServices proxy
                    );
                SysFreeString( bstrNamespace );
            }
        }
        pLocator->Release();
    }
    return hr;
}
}

