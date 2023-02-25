//--------------------------------------------------------------------------------------
// File: MsiUtil.cpp
//
// MSI support functions for DirectX apps
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "dxut.h"
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <process.h>
#define _WIN32_MSI 300
#include <msi.h>
#include "msiutil.h"


#define COMBINED_FEATUREPROGTICK 512


//--------------------------------------------------------------------------------------
// Converts the string (from current pos. to next whitespace or '\0')
// to an integer.
//--------------------------------------------------------------------------------------
int FGetInteger( WCHAR*& rpch )
{
    WCHAR* pchPrev = rpch;
    while( *rpch && *rpch != L' ' )
        rpch++;
    *rpch = L'\0';
    int i = _wtoi( pchPrev );
    return i;
}


//--------------------------------------------------------------------------------------
// Parses the progress data message sent to the INSTALLUI_HANDLER callback
//--------------------------------------------------------------------------------------
BOOL ParseProgressString( LPWSTR sz, int* pnFields )
{
    WCHAR* pch = sz;
    if( 0 == *pch )
        return FALSE; // no msg

    while( *pch != 0 )
    {
        WCHAR chField = *pch++;
        pch++; // for ':'
        pch++; // for sp
        switch( chField )
        {
            case L'1': // field 1
            {
                // progress message type
                if( 0 == isdigit( *pch ) )
                    return FALSE; // blank record
                pnFields[0] = *pch++ - L'0';
                break;
            }
            case L'2': // field 2
            {
                pnFields[1] = FGetInteger( pch );
                if( pnFields[0] == 2 || pnFields[0] == 3 )
                    return TRUE; // done processing
                break;
            }
            case '3': // field 3
            {
                pnFields[2] = FGetInteger( pch );
                if( pnFields[0] == 1 )
                    return TRUE; // done processing
                break;
            }
            case '4': // field 4
            {
                pnFields[3] = FGetInteger( pch );
                return TRUE; // done processing
            }
            default: // unknown field
            {
                return FALSE;
            }
        }
        pch++; // for space (' ') between fields
    }

    return TRUE;
}


CMsiUtil::CMsiUtil()
{
    InitializeCriticalSection( &m_CritSec );

    *m_szProduct = L'\0';
    m_hThread = NULL;
    m_hEvent = NULL;
    m_cPassiveEnable = 0;
    m_uLastError = ERROR_SUCCESS;
    m_CurrentRequest.Type = NONE;
    m_szFeatureSet[0] = L'\0';
    m_bCurrentReqActive = false;
    m_bAbort = false;
    m_nProgress = m_nProgressMax = 0;
    m_cFeaturesReq = m_cFeaturesDone = 0;
    m_cPassiveFeaturesReq = m_cPassiveFeaturesDone = 0;
    m_bThreadShutdown = false;

    m_bForwardProgress = false;
    m_bScriptInProgress = false;
    m_nIncrStep = 0;
    m_bEnableActionData = false;
    m_bInstallFinalize = false;
}


CMsiUtil::~CMsiUtil()
{
    if( m_hThread )
    {
        // Signal the worker thread to exit
        InterlockedExchange( ( LONG* )&m_bThreadShutdown, true );
        SetEvent( m_hEvent );
        AbortCurrentRequest();

        // Wait a maximum of 10 seconds for worker thread to exit.
        WaitForSingleObject( m_hThread, 10000 );
        CloseHandle( m_hThread );
    }

    if( m_hEvent )
        CloseHandle( m_hEvent );

    DeleteCriticalSection( &m_CritSec );
}


void CMsiUtil::EnablePassiveInstall( bool bEnable )
{
    if( bEnable )
        InterlockedIncrement( ( LONG* )&m_cPassiveEnable );
    else
    {
        InterlockedDecrement( ( LONG* )&m_cPassiveEnable );
        // Min cap at 0
        InterlockedCompareExchange( ( LONG* )&m_cPassiveEnable, 0, -1 );
    }

    // If we just went from disabled to enabled, signal the event object
    // in case the worker thread is suspended on wait.
    if( 1 == m_cPassiveEnable )
        SetEvent( m_hEvent );
}


int CMsiUtil::GetProgress() const
{
    // Construct a virtual progress bar based on
    // the number of installed features so far and
    // the feature progress tick.

    // Use COMBINED_FEATUREPROGTICK for a complete feature progress.

    __int64 nCurrent;
    if( GetFeatureProgress() == 0 )
        nCurrent = 0;
    else
        nCurrent = GetFeatureProgressMax() ? ( __int64 )GetFeatureProgress() * COMBINED_FEATUREPROGTICK /
            GetFeatureProgressMax()
            : COMBINED_FEATUREPROGTICK;
    return int( GetCompletedFeatureCount() * COMBINED_FEATUREPROGTICK + nCurrent );
}


int CMsiUtil::GetProgressMax() const
{
    return GetRequestedFeatureCount() * COMBINED_FEATUREPROGTICK;
}


int CMsiUtil::GetFeatureProgress() const
{
    // If abort flag is set, the Current installation
    // progress is irrelevant, so return 0.
    return m_bAbort ? 0 : m_nProgress;
}


int CMsiUtil::GetFeatureProgressMax() const
{
    return m_bAbort ? 0 : m_nProgressMax;
}


int CMsiUtil::GetRequestedFeatureCount() const
{
    return m_cFeaturesReq;
}


int CMsiUtil::GetCompletedFeatureCount() const
{
    return m_cFeaturesDone;
}


int CMsiUtil::GetPassiveProgress() const
{
    // Construct a virtual progress bar based on
    // the number of installed features so far and
    // the feature progress tick.

    // Use COMBINED_FEATUREPROGTICK for a complete feature progress.

    __int64 nCurrent;
    if( GetFeatureProgress() == 0 || m_bCurrentReqActive )
        nCurrent = 0;
    else
        nCurrent = GetFeatureProgressMax() ? ( __int64 )GetFeatureProgress() * COMBINED_FEATUREPROGTICK /
            GetFeatureProgressMax()
            : COMBINED_FEATUREPROGTICK;
    return int( m_cPassiveFeaturesDone * COMBINED_FEATUREPROGTICK + nCurrent );
}


int CMsiUtil::GetPassiveProgressMax() const
{
    return m_cPassiveFeaturesReq * COMBINED_FEATUREPROGTICK;
}


int CMsiUtil::GetRequestedPassiveFeatureCount() const
{
    return m_cPassiveFeaturesReq;
}


int CMsiUtil::GetCompletedPassiveFeatureCount() const
{
    return m_cPassiveFeaturesDone;
}


//--------------------------------------------------------------------------------------
// Return the name of the feature currently being installed.  If the current installation
// is not feature-specific (full product), the string "Full Product" is returned. If
// no current installation is present, an empty string is returned.
//--------------------------------------------------------------------------------------
void CMsiUtil::GetCurrentFeatureName( LPWSTR szFeature, int cChSize )
{
    EnterCriticalSection( &m_CritSec );
    switch( m_CurrentRequest.Type )
    {
        case PRODUCT:
            StringCchCopy( szFeature, cChSize, L"Full Product" );
            break;
        case FEATURE:
            StringCchCopy( szFeature, cChSize, m_CurrentRequest.szFeature );
            break;
        case NONE:
            StringCchCopy( szFeature, cChSize, L"" );
            break;
    }
    LeaveCriticalSection( &m_CritSec );
}


HRESULT CMsiUtil::Initialize( LPCWSTR szProduct )
{
    // Save product GUID string
    StringCchCopy( m_szProduct, 39, szProduct );

    // Disable MSI's internal UI
    MsiSetInternalUI( INSTALLUILEVEL_NONE, NULL );

    // Empty the passive install queue.
    EnterCriticalSection( &m_CritSec );
    AbortCurrentRequest();
    m_LoQ.RemoveAll();
    InterlockedExchange( ( LONG* )&m_cPassiveFeaturesReq, 0 );
    InterlockedExchange( ( LONG* )&m_cPassiveFeaturesDone, 0 );
    LeaveCriticalSection( &m_CritSec );

    // Enumerate all features of this product, then
    // add them to the passive install queue.
    DWORD dwIndex = 0;
    for(; ; ++dwIndex )
    {
        WCHAR szFeature[MAX_FEATURE_CHARS+1];

        if( ERROR_SUCCESS != MsiEnumFeatures( szProduct, dwIndex, szFeature, NULL ) )
            break;

        INSTALLSTATE State = MsiQueryFeatureState( szProduct, szFeature );
        if( INSTALLSTATE_LOCAL == State )
            continue;

        if( INSTALLSTATE_ADVERTISED != State )
            break;

        DXUTTRACE( L"Feature: %s\n", szFeature );

        // Successfully enumerated a feature. Add it to the passive install queue.
        MSIUTIL_INSTALLREQUEST Request;
        Request.Type = FEATURE;
        StringCchCopy( Request.szFeature, MAX_FEATURE_CHARS + 1, szFeature );
        Request.hEvent = NULL;

        EnterCriticalSection( &m_CritSec );
        m_LoQ.Add( Request );
        InterlockedIncrement( ( LONG* )&m_cPassiveFeaturesReq );
        LeaveCriticalSection( &m_CritSec );
    }

    // If it's hasn't been done, spawn off a worker thread to
    // call MSI for handling installation requests.  Initially
    // this thread will be suspended the queue would be empty.
    if( !m_hThread )
    {
        UINT ThreadId;
        m_hThread = ( HANDLE )_beginthreadex( NULL, 0, CMsiUtil::InstallThreadProc, ( LPVOID )this, 0, &ThreadId );

        if( !m_hThread )
            return E_FAIL;
    }

    if( !m_hEvent )
    {
        m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( !m_hEvent )
            return E_FAIL;
    }

    return S_OK;
}


INT CMsiUtil::InstallUiHandler( UINT iMessageType, LPCTSTR szMessage )
{
    // If abort is signaled, return IDCANCEL immediately to end the installation.
    if( m_bAbort )
        return IDCANCEL;

    switch( iMessageType )
    {
        case INSTALLMESSAGE_INFO:
            if( wcsstr( szMessage, L"InstallFinalize" ) )
            {
                if( !wcsncmp( szMessage, L"Action start", 12 ) )
                    m_bInstallFinalize = true;
                else if( !wcsncmp( szMessage, L"Action ended", 12 ) )
                    m_bInstallFinalize = false;
            }
            break;

        case INSTALLMESSAGE_ACTIONDATA:
            // only act if progress total has been initialized
            if( 0 == m_nProgressMax )
                return IDOK;
            if( m_bEnableActionData )
            {
                InterlockedExchangeAdd( ( LONG* )&m_nProgress, m_nIncrStep );
            }
            return IDOK;

        case INSTALLMESSAGE_PROGRESS:
        {
            int rgiField[4];

            // Only care about progress information during InstallFinalize phase.
            if( m_bInstallFinalize )
            {
                if( ParseProgressString( const_cast<LPWSTR>( szMessage ), rgiField ) )
                {
                    // All fields off by 1 due to c array notation
                    switch( rgiField[0] )
                    {
                        case 0: // Reset progress bar
                        {
                            // Field 1 = 0, field 2 = total number of ticks, field 3 = direction, field 4 = in progress

                            // Get total number of ticks in progress bar
                            InterlockedExchange( ( LONG* )&m_nProgressMax, rgiField[1] );

                            // Determine direction
                            if( rgiField[2] == 0 )
                                m_bForwardProgress = true;
                            else // rgiField[2] == 1
                                m_bForwardProgress = false;

                            // If total number of ticks is non-zero, this is a valid
                            // progress reset. We reset current position as well.
                            if( m_nProgressMax != 0 )
                            {
                                // Get current position of progress bar, depends on direction
                                // If Forward direction, current position is 0
                                // If Backward direction, current position is Total # ticks
                                InterlockedExchange( ( LONG* )&m_nProgress, m_bForwardProgress ? 0 : m_nProgressMax );
                            }

                            // Determine new state
                            // If new state = 1 (script in progress), could send a "Please wait..." msg
                            m_bScriptInProgress = ( rgiField[3] == 1 ) ? true : false;
                            break;
                        }
                        case 1:
                        {
                            // Field 1 = 1, field 2 will contain the number of ticks to increment the bar
                            // Ignore if field 3 is zero
                            if( rgiField[2] )
                            {
                                // Movement direction determined by bForwardProgress set by reset progress msg
                                m_nIncrStep = m_bForwardProgress ? rgiField[1] : -rgiField[1];
                                m_bEnableActionData = TRUE;
                            }
                            else
                            {
                                m_bEnableActionData = FALSE;
                            }
                            break;
                        }
                        case 2:
                        {
                            // Only act if progress total has been initialized
                            if( 0 == m_nProgressMax )
                                break;
                            // If field 1 = 2, field 2 will contain the number of ticks the bar has moved
                            // Movement direction determined by bForwardProgress set by reset progress msg
                            InterlockedExchangeAdd( ( LONG* )&m_nProgress,
                                                    m_bForwardProgress ? rgiField[1] : -rgiField[1] );
                            break;
                        }
                        case 3: // Fall through (we don't care to handle it -- total tick count adjustment)
                        default:
                            {
                                break;
                            }
                    }
                }
            }
            else
            {
                InterlockedExchange( ( LONG* )&m_nProgressMax, 0 );
            }
        }

        default:
            return 0;
    }

    return IDOK;
}


INT CALLBACK CMsiUtil::StaticInstallUiHandler( LPVOID pvContext, UINT iMessageType, LPCTSTR szMessage )
{
    // Uncomment the next line to see what messages MSI is sending.
    //    DXUTTRACE( L"MSI: %08X  %s\n", iMessageType, szMessage );
    return reinterpret_cast<CMsiUtil*>( pvContext )->InstallUiHandler( iMessageType, szMessage );
}


//--------------------------------------------------------------------------------------
// Thread proc for the installation thread. It processes requests in queue as they come
// in.
//--------------------------------------------------------------------------------------
#pragma warning( disable : 4701 )
unsigned __stdcall CMsiUtil::InstallThreadProc( void* pData )
{
    CMsiUtil* pCls = reinterpret_cast<CMsiUtil*>( pData );
    MSIUTIL_INSTALLREQUEST Request;

    // Enter a loop that processes installation requests in the queue.
    // When the queue is empty, the thread waits for m_hEvent. The main
    // thread will signal this event when a new request comes in.

    while( !pCls->m_bThreadShutdown )
    {
        // Clear abort flag
        InterlockedExchange( ( LONG* )&pCls->m_bAbort, false );

        // Try to extract the first item in the active queue.
        INSTALLSTATE FeatureState;
        EnterCriticalSection( &pCls->m_CritSec );
        int QSize = pCls->m_HiQ.GetSize();
        if( QSize > 0 )
        {
            // Extract the first queued request to m_CurrentRequest as well
            // as creating a local copy for this thread's access.
            Request = pCls->m_HiQ.GetAt( 0 );

            // Process only if this is product install or the feature is advertised.
            if( PRODUCT != Request.Type )
                FeatureState = MsiQueryFeatureState( pCls->m_szProduct, Request.szFeature );
            if( PRODUCT == Request.Type || INSTALLSTATE_ADVERTISED == FeatureState )
            {
                pCls->m_CurrentRequest = Request;
                InterlockedExchange( ( LONG* )&pCls->m_bCurrentReqActive, true );
            }

            // Remove this request from the queue
            pCls->m_HiQ.Remove( 0 );
        }
        else // Active install queue empty.
        // Process passive install queue only if enabled
        if( pCls->m_cPassiveEnable > 0 )
        {
            QSize = pCls->m_LoQ.GetSize();
            if( QSize > 0 )
            {
                // Extract the first queued request to m_CurrentRequest as well
                // as creating a local copy for this thread's access.
                Request = pCls->m_LoQ.GetAt( 0 );

                // Process only if the feature is advertised.
                FeatureState = MsiQueryFeatureState( pCls->m_szProduct, Request.szFeature );
                if( INSTALLSTATE_ADVERTISED == FeatureState )
                {
                    pCls->m_CurrentRequest = Request;
                    InterlockedExchange( ( LONG* )&pCls->m_bCurrentReqActive, false );
                }
                else
                {
                    // Just increment the processed feature count and move on.
                    InterlockedIncrement( ( LONG* )&pCls->m_cPassiveFeaturesDone );
                }

                // Remove this request from the queue
                pCls->m_LoQ.Remove( 0 );
            }
        }
        LeaveCriticalSection( &pCls->m_CritSec );

        // If either queue is not empty, a request has been extracted.
        if( QSize > 0 )
        {
            // Process the request if it is advertised.
            if( PRODUCT == Request.Type || INSTALLSTATE_ADVERTISED == FeatureState )
            {
                DXUTTRACE( L"Processing %s install request on feature: %s\n",
                           pCls->m_bCurrentReqActive ? L"active" : L"passive",
                           Request.szFeature );

                // Initialize the installation UI state data.
                pCls->m_bForwardProgress = false;
                pCls->m_bScriptInProgress = false;
                pCls->m_nIncrStep = 0;
                pCls->m_bEnableActionData = false;
                pCls->m_bInstallFinalize = false;

                // Set up installation progress notification and disable default UI.
                INSTALLUI_HANDLER OldHandler = MsiSetExternalUI( StaticInstallUiHandler,
                                                                 INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_INFO,
                                                                 pData );

                UINT uReturn;
                if( PRODUCT == Request.Type )
                {
                    // Full product install
                    uReturn = MsiConfigureProduct( pCls->m_szProduct, INSTALLLEVEL_MAXIMUM, INSTALLSTATE_LOCAL );
                }
                else
                {
                    // Feature install
                    uReturn = MsiConfigureFeature( pCls->m_szProduct, Request.szFeature, INSTALLSTATE_DEFAULT );
                }
                MsiSetExternalUI( OldHandler, 0, NULL );
                // If installation successful, update processed feature count.
                if( ERROR_SUCCESS == uReturn )
                {
                    if( pCls->m_bCurrentReqActive )
                        InterlockedIncrement( ( LONG* )&pCls->m_cFeaturesDone );
                    else
                        InterlockedIncrement( ( LONG* )&pCls->m_cPassiveFeaturesDone );
                }

                InterlockedExchange( ( LONG* )&pCls->m_nProgress, 0 );  // Clear progress bar
                InterlockedExchange( ( LONG* )&pCls->m_nProgressMax, 0 );
                InterlockedExchange( ( LONG* )&pCls->m_uLastError, uReturn ); // Set last error code
                InterlockedExchange( ( LONG* )&pCls->m_bAbort, false ); // Clear abort flag

                EnterCriticalSection( &pCls->m_CritSec );
                // Signal the application-supplied event object. Note that we
                // use m_CurrentRequest's hEvent for signalling instead of the
                // local copy. This is done because the main thread may have a
                // reason to supress notification for the current request. In
                // this case it sets m_CurrentRequest's hEvent to NULL.
                HANDLE hEvent;
                hEvent = pCls->m_CurrentRequest.hEvent;
                if( hEvent )
                    SetEvent( hEvent );

                // If we have just completed the last request in the active install queue,
                // we have completed an entire feature set, so clear the active feature
                // set name.
                if( pCls->m_HiQ.GetSize() == 0 && pCls->m_bCurrentReqActive )
                    pCls->m_szFeatureSet[0] = L'\0';

                // Clear the current request status.
                pCls->m_CurrentRequest.Type = NONE;

                LeaveCriticalSection( &pCls->m_CritSec );
            }
        }
        else
        {
            // The queue is empty. Wait on m_hEvent for new requests.
            WaitForSingleObject( pCls->m_hEvent, INFINITE );
        }
    }

    return 0;
}
#pragma warning( default : 4701 )


bool CMsiUtil::IsInstallInProgress()
{
    EnterCriticalSection( &m_CritSec );
    bool bInProgress = ( ( m_CurrentRequest.Type != NONE ) && m_bCurrentReqActive ) || m_HiQ.GetSize() != 0;
    LeaveCriticalSection( &m_CritSec );

    return bInProgress;
}


//--------------------------------------------------------------------------------------
// Call this method before using resources of a feature.  If this feature has been
// installed, the method returns and bInstalled is set to true.  Otherwise,
// a installation request is queued, and bInstalled is et to false before the method
// returns.
HRESULT CMsiUtil::UseFeature( LPCWSTR szFeature, HANDLE hEvent, bool& bInstalled )
{
    HRESULT hr = S_OK;

    // MSI issue workaround:
    // It seems that MsiQueryFeatureState will return INSTALLSTATE_LOCAL shortly
    // after MsiConfigureFeature is called, even before the copying is
    // finished.  For our purpose, we can't access the files before
    // copying is done, so we don't consider the feature installed
    // if MSI is still installing.
    bool bFeatureIsCurrent = false;
    EnterCriticalSection( &m_CritSec );
    if( m_CurrentRequest.Type == FEATURE && !lstrcmpW( m_CurrentRequest.szFeature, szFeature ) )
        bFeatureIsCurrent = true;
    LeaveCriticalSection( &m_CritSec );

    if( bFeatureIsCurrent )
    {
        // Feature is still being installed. Treat the feature as
        // advertised, so set bInstalled to false.
        bInstalled = false;
        return hr;
    }

    // Call MSI to query the state of the feature we are interested in.
    // This feature may not have been installed. In this case,
    // queue up a install request.

    INSTALLSTATE State = MsiQueryFeatureState( m_szProduct, szFeature );
    if( INSTALLSTATE_ADVERTISED == State )
    {
        bInstalled = false;

        // Fill a new request struct
        MSIUTIL_INSTALLREQUEST Request;
        Request.Type = FEATURE;
        StringCchCopy( Request.szFeature, MAX_FEATURE_CHARS + 1, szFeature );
        Request.hEvent = hEvent;

        //
        // Queue up the request
        //
        EnterCriticalSection( &m_CritSec );
        hr = m_HiQ.Add( Request );

        // Update the feature install request count.
        InterlockedIncrement( ( LONG* )&m_cFeaturesReq );

        AbortCurrentRequest();
        LeaveCriticalSection( &m_CritSec );
    }
    else if( INSTALLSTATE_LOCAL == State || INSTALLSTATE_SOURCE == State )
    {
        bInstalled = true;
    }
    else
    {
        // Bad state returned by MsiQueryFeatureState.
        hr = E_FAIL;
    }

    return hr;
}


class CFeatureString
{
    WCHAR   m_sz[MAX_FEATURE_CHARS+1];

public:
            CFeatureString()
            {
                *m_sz = L'\0';
            }
            CFeatureString( LPCWSTR sz )
            {
                StringCchCopy( m_sz, MAX_FEATURE_CHARS + 1, sz );
            }
            CFeatureString( const CFeatureString& rhs )
            {
                *this = rhs;
            }
    CFeatureString& operator=( const CFeatureString& rhs )
    {
        if( this == &rhs )
            return *this;

        StringCchCopy( m_sz, MAX_FEATURE_CHARS + 1, rhs.m_sz );

        return *this;
    }
    bool    operator==( const CFeatureString& rhs ) const
    {
        return !lstrcmpW( m_sz, rhs.m_sz );
    }
};


// Signal that the feature szFeature and all sub-features are to be used.
// If any sub-feature is not installed, queue up a request to install it.
// On return, bInstalled is set to true only if all sub-features are already
// installed. hEvent is set for the last subfeature so that only one
// notification is sent to the application.
HRESULT CMsiUtil::UseFeatureSet( LPCWSTR szFeature, HANDLE hEvent, bool& bInstalled )
{
    HRESULT hr = S_OK;

    bInstalled = true;
    CGrowableArray <CFeatureString> ParentSet;  // Set of possible parents under szFeature
    ParentSet.Add( szFeature );
    bool bSubFeatureInstalled;

    EnterCriticalSection( &m_CritSec );

    // If the requested feature set is the same as the Current feature set,
    // this is already in progress, and nothing more needs to be done.
    if( !lstrcmpW( m_szFeatureSet, szFeature ) )
    {
        bInstalled = false;
        LeaveCriticalSection( &m_CritSec );
        return hr;
    }

    // Abort all active install requests
    AbortAllRequests();

    // Reset feature install count
    InterlockedExchange( ( LONG* )&m_cFeaturesReq, 0 );
    InterlockedExchange( ( LONG* )&m_cFeaturesDone, 0 );

    // Queue the top level feature itself.
    if( FAILED( UseFeature( szFeature, NULL, bSubFeatureInstalled ) ) || false == bSubFeatureInstalled )
        bInstalled = false;

    DWORD dwIndex = 0;
    for(; ; ++dwIndex )
    {
        WCHAR szEnumFeature[MAX_FEATURE_CHARS+1];
        WCHAR szEnumParent[MAX_FEATURE_CHARS+1];

        UINT uRc = MsiEnumFeatures( m_szProduct, dwIndex, szEnumFeature, szEnumParent );
        if( ERROR_SUCCESS != uRc )
            break;

        // If the parent is one of the features in ParentSet, this is
        // a feature that belongs to the feature set under szFeature.
        if( ParentSet.IndexOf( szEnumParent ) != -1 )
        {
            if( FAILED( UseFeature( szEnumFeature, NULL, bSubFeatureInstalled ) ) || false == bSubFeatureInstalled )
                bInstalled = false;

            // Add this sub-feature to the parent set, since any child of this
            // feature is part of the feature set as well.
            hr = ParentSet.Add( szEnumFeature );
        }
    }

    // If any sub-feature is not installed, a request has been placed.
    // We update the last added request's event handle to hEvent.
    // We want the application to be notified when the entire feature
    // set is installed. This happens when the last queued sub-feature
    // is installed.
    if( !bInstalled )
    {
        m_HiQ.GetAt( m_HiQ.GetSize() - 1 ).hEvent = hEvent;

        // Update the active feature set name
        StringCchCopyW( m_szFeatureSet, MAX_FEATURE_CHARS + 1, szFeature );
    }

    LeaveCriticalSection( &m_CritSec );

    //
    // Signal the worker thread that new requests are available.
    //
    SetEvent( m_hEvent );

    return hr;
}


HRESULT CMsiUtil::UseProduct( HANDLE hEvent )
{
    bool bDoNothing = false;

    EnterCriticalSection( &m_CritSec );
    if( PRODUCT == m_CurrentRequest.Type )
    {
        // If the full product is already being installed, nothing
        // needs to be done.
        bDoNothing = true;
    }
    else
    {
        // Set m_CurrentRequest's hEvent to NULL to supress notification.
        // Since we are manually cancelling the current installation
        // and immediately perform full product installation, we
        // don't care about the canceled request.
        m_CurrentRequest.hEvent = NULL;
    }
    LeaveCriticalSection( &m_CritSec );

    if( bDoNothing )
        return S_OK;

    // First abort current and all queued requests. Product install
    // will include every feature.
    AbortAllRequests();

    // Reset feature install count
    InterlockedExchange( ( LONG* )&m_cFeaturesReq, 0 );
    InterlockedExchange( ( LONG* )&m_cFeaturesDone, 0 );

    // Fill a new request struct
    MSIUTIL_INSTALLREQUEST Request;
    Request.Type = PRODUCT;
    Request.szFeature[0] = L'\0';
    Request.hEvent = hEvent;

    //
    // Queue up the request
    //
    EnterCriticalSection( &m_CritSec );
    HRESULT hr = m_HiQ.Add( Request );
    LeaveCriticalSection( &m_CritSec );

    if( SUCCEEDED( hr ) )
        InterlockedIncrement( ( LONG* )&m_cFeaturesReq );

    //
    // Signal the worker thread that new requests are available.
    //
    SetEvent( m_hEvent );

    return hr;
}


void CMsiUtil::AbortCurrentRequest()
{
    // If a passive install request is installing, abort it,
    // but add it back to the passive install queue again so that
    // it will eventually be installed when there is enough
    // processor cycles. This is done so that we never omit a
    // feature in passive install.

    EnterCriticalSection( &m_CritSec );
    if( FEATURE == m_CurrentRequest.Type && !m_bCurrentReqActive )
    {
        // Add to back to the passive queue.
        MSIUTIL_INSTALLREQUEST Request;
        Request = m_CurrentRequest;
        m_LoQ.Add( Request );
    }
    LeaveCriticalSection( &m_CritSec );

    InterlockedExchange( ( LONG* )&m_bAbort, true );
}


void CMsiUtil::AbortAllRequests()
{
    // First clear the queue.
    EnterCriticalSection( &m_CritSec );
    m_HiQ.RemoveAll();
    LeaveCriticalSection( &m_CritSec );

    // Abort current installation.
    AbortCurrentRequest();
}
