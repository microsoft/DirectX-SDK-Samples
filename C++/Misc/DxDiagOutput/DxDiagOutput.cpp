//----------------------------------------------------------------------------
// File: DxDiagOutput.cpp
//
// Desc: Sample app to read info from dxdiagn.dll by enumeration
//
// Copyright (c) Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define INITGUID
#include <windows.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <stdio.h>
#include <assert.h>
#include <initguid.h>
#include <dxdiag.h>




//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }




//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
HRESULT PrintContainerAndChildren( WCHAR* wszParentName, IDxDiagContainer* pDxDiagContainer );




//-----------------------------------------------------------------------------
// Name: main()
// Desc: Entry point for the application.  We use just the console window
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
    HRESULT hr;

    CoInitialize( NULL );

    IDxDiagProvider* pDxDiagProvider = NULL;
    IDxDiagContainer* pDxDiagRoot = NULL;

    // CoCreate a IDxDiagProvider*
    hr = CoCreateInstance( CLSID_DxDiagProvider,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDxDiagProvider,
                           ( LPVOID* )&pDxDiagProvider );
    if( SUCCEEDED( hr ) ) // if FAILED(hr) then DirectX 9 is not installed
    {
        // Fill out a DXDIAG_INIT_PARAMS struct and pass it to IDxDiagContainer::Initialize
        // Passing in TRUE for bAllowWHQLChecks, allows dxdiag to check if drivers are
        // digital signed as logo'd by WHQL which may connect via internet to update
        // WHQL certificates.
        DXDIAG_INIT_PARAMS dxDiagInitParam;
        ZeroMemory( &dxDiagInitParam, sizeof( DXDIAG_INIT_PARAMS ) );

        dxDiagInitParam.dwSize = sizeof( DXDIAG_INIT_PARAMS );
        dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        dxDiagInitParam.bAllowWHQLChecks = TRUE;
        dxDiagInitParam.pReserved = NULL;

        hr = pDxDiagProvider->Initialize( &dxDiagInitParam );
        if( FAILED( hr ) )
            goto LCleanup;

        hr = pDxDiagProvider->GetRootContainer( &pDxDiagRoot );
        if( FAILED( hr ) )
            goto LCleanup;

        // This function will recursivly print the properties
        // the root node and all its child.
        hr = PrintContainerAndChildren( NULL, pDxDiagRoot );
        if( FAILED( hr ) )
            goto LCleanup;
    }

LCleanup:
    SAFE_RELEASE( pDxDiagRoot );
    SAFE_RELEASE( pDxDiagProvider );

    CoUninitialize();

    return 0;
}




//-----------------------------------------------------------------------------
// Name: PrintContainerAndChildren()
// Desc: Recursivly print the properties the root node and all its child
//       to the console window
//-----------------------------------------------------------------------------
HRESULT PrintContainerAndChildren( WCHAR* wszParentName, IDxDiagContainer* pDxDiagContainer )
{
    HRESULT hr;

    DWORD dwPropCount;
    DWORD dwPropIndex;
    WCHAR wszPropName[256];
    VARIANT var;
    WCHAR wszPropValue[256];

    DWORD dwChildCount;
    DWORD dwChildIndex;
    WCHAR wszChildName[256];
    IDxDiagContainer* pChildContainer = NULL;

    VariantInit( &var );

    hr = pDxDiagContainer->GetNumberOfProps( &dwPropCount );
    if( SUCCEEDED( hr ) )
    {
        // Print each property in this container
        for( dwPropIndex = 0; dwPropIndex < dwPropCount; dwPropIndex++ )
        {
            hr = pDxDiagContainer->EnumPropNames( dwPropIndex, wszPropName, 256 );
            if( SUCCEEDED( hr ) )
            {
                hr = pDxDiagContainer->GetProp( wszPropName, &var );
                if( SUCCEEDED( hr ) )
                {
                    // Switch off the type.  There's 4 different types:
                    switch( var.vt )
                    {
                        case VT_UI4:
                            StringCchPrintfW( wszPropValue, 256, L"%d", var.ulVal );
                            break;
                        case VT_I4:
                            StringCchPrintfW( wszPropValue, 256, L"%d", var.lVal );
                            break;
                        case VT_BOOL:
                            StringCchCopyW( wszPropValue, 256, ( var.boolVal ) ? L"true" : L"false" );
                            break;
                        case VT_BSTR:
                            StringCchCopyW( wszPropValue, 256, var.bstrVal );
                            break;
                    }

                    // Add the parent name to the front if there's one, so that
                    // its easier to read on the screen
                    if( wszParentName )
                        wprintf( L"%s.%s = %s\n", wszParentName, wszPropName, wszPropValue );
                    else
                        wprintf( L"%s = %s\n", wszPropName, wszPropValue );

                    // Clear the variant (this is needed to free BSTR memory)
                    VariantClear( &var );
                }
            }
        }
    }

    // Recursivly call this function for each of its child containers
    hr = pDxDiagContainer->GetNumberOfChildContainers( &dwChildCount );
    if( SUCCEEDED( hr ) )
    {
        for( dwChildIndex = 0; dwChildIndex < dwChildCount; dwChildIndex++ )
        {
            hr = pDxDiagContainer->EnumChildContainerNames( dwChildIndex, wszChildName, 256 );
            if( SUCCEEDED( hr ) )
            {
                hr = pDxDiagContainer->GetChildContainer( wszChildName, &pChildContainer );
                if( SUCCEEDED( hr ) )
                {
                    // wszFullChildName isn't needed but is used for text output
                    WCHAR wszFullChildName[256];
                    if( wszParentName )
                        StringCchPrintfW( wszFullChildName, 256, L"%s.%s", wszParentName, wszChildName );
                    else
                        StringCchCopyW( wszFullChildName, 256, wszChildName );
                    PrintContainerAndChildren( wszFullChildName, pChildContainer );

                    SAFE_RELEASE( pChildContainer );
                }
            }
        }
    }

    return S_OK;
}
