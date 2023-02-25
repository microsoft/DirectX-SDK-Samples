//--------------------------------------------------------------------------------------
// File: MsiUtil.h
//
// Header file for MSI support functions for DirectX apps
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifndef __MSIUTIL_H__
#define __MSIUTIL_H__


#include "msi.h"


enum MSIUTIL_INSTALLTYPE
{
    NONE,
    FEATURE,
    PRODUCT
};

// INSTALLREQUEST is one usage request to MSI.  Type specifies
// the type of usage. If Type = PRODUCT, szFeature is ignored
// and CMsiUtil will ask MSI to install the entire product.
// If Type = FEATURE, CMsiUtil will ask MSI to install the feature
// specified by szFeature.
struct MSIUTIL_INSTALLREQUEST
{
    MSIUTIL_INSTALLTYPE Type;             // Install type
    WCHAR szFeature[MAX_FEATURE_CHARS+1]; // Feature name for feature installs
    HANDLE hEvent;                        // Event handle to signal when this request is fulfilled
};


class CMsiUtil
{
public: // Public methods
                                CMsiUtil();
                                ~CMsiUtil();

    HRESULT                     Initialize( LPCWSTR szProduct );
    HRESULT                     UseFeatureSet( LPCWSTR szFeature, HANDLE hEvent, bool& bInstalled );
    HRESULT                     UseProduct( HANDLE hEvent );
    void                        EnablePassiveInstall( bool bEnable );
    void                        AbortCurrentRequest();
    void                        AbortAllRequests();
    UINT                        GetLastError() const
    {
        return m_uLastError;
    }
    bool                        IsInstallInProgress();
    int                         GetProgress() const;
    int                         GetProgressMax() const;
    int                         GetFeatureProgress() const;
    int                         GetFeatureProgressMax() const;
    int                         GetRequestedFeatureCount() const;
    int                         GetCompletedFeatureCount() const;
    int                         GetPassiveProgress() const;
    int                         GetPassiveProgressMax() const;
    int                         GetRequestedPassiveFeatureCount() const;
    int                         GetCompletedPassiveFeatureCount() const;
    void                        GetCurrentFeatureName( LPWSTR szFeature, int cChSize );

private: // Private methods
    INT                         InstallUiHandler( UINT iMessageType, LPCTSTR szMessage );
    HRESULT                     UseFeature( LPCWSTR szFeature, HANDLE hEvent, bool& bInstalled );

    static INT CALLBACK         StaticInstallUiHandler( LPVOID pvContext, UINT iMessageType, LPCTSTR szMessage );
    static unsigned __stdcall   InstallThreadProc( void* pData );  // Worker thread proc

private: // Private data
    CRITICAL_SECTION m_CritSec;               // Critical section to sync member data access
    WCHAR                       m_szProduct[39];         // Product GUID string
    int m_cPassiveEnable;        // Passive installation enable count. If >0, passive install is enabled.
    HANDLE m_hThread;               // Install thread handle
    HANDLE m_hEvent;                // Event object for synchronization between 2 threads
    CGrowableArray <MSIUTIL_INSTALLREQUEST> m_HiQ; // High priority installation request queue used for active install.
    // Accesses must be protected by the critical section m_CritSec.
    CGrowableArray <MSIUTIL_INSTALLREQUEST> m_LoQ; // Low priority installation request queue used for passive install.
    // Accesses must be protected by the critical section m_CritSec.
    MSIUTIL_INSTALLREQUEST m_CurrentRequest;  // Request that's being currently handled. Must protect with m_CritSec.
    WCHAR                       m_szFeatureSet[MAX_FEATURE_CHARS+1]; // Feature set that is actively installing. Protect with m_CritSec.
    bool m_bCurrentReqActive;     // Whether or not the current request is from high priority queue (active install).

    UINT m_uLastError;            // Last error code returned by MSI install function.
    bool m_bAbort;                // Set to true to signal the install thread to abort
    // current installation.
    int m_nProgress;             // Current progress bar value
    int m_nProgressMax;          // Maximum progress bar value
    int m_cFeaturesReq;          // Total number of features requested with UseFeatureSet()
    int m_cFeaturesDone;         // Number of feature installs completed
    int m_cPassiveFeaturesReq;   // Total number of passive install requests queued from the start
    int m_cPassiveFeaturesDone;  // Total number of passive install requests processed.
    bool m_bThreadShutdown;       // True when the class is destructed to let worker
    // thread exit gracefully.

    // State data used by install UI handler. The main thread should not
    // access these variables. As such these do not need to be protected
    // by synchronization objects.
    bool m_bForwardProgress;
    bool m_bScriptInProgress;
    int m_nIncrStep;             // Number of ticks to increment progress bar for each step
    bool m_bEnableActionData;
    bool m_bInstallFinalize;
};


#endif
