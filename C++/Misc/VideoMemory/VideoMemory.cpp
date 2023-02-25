//----------------------------------------------------------------------------
// File: VideoMemory.cpp
//
// Copyright (c) Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define INITGUID
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <d3d9.h>


//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

HRESULT GetVideoMemoryViaDxDiag( HMONITOR hMonitor, DWORD* pdwDisplayMemory );
HRESULT GetVideoMemoryViaDirectDraw( HMONITOR hMonitor, DWORD* pdwAvailableVidMem );
HRESULT GetVideoMemoryViaWMI( HMONITOR hMonitor, DWORD* pdwAdapterRam );
HRESULT GetVideoMemoryViaDXGI( HMONITOR hMonitor, SIZE_T* pDedicatedVideoMemory, SIZE_T* pDedicatedSystemMemory,
                               SIZE_T* pSharedSystemMemory );
HRESULT GetVideoMemoryViaD3D9( HMONITOR hMonitor, UINT* pdwAvailableTextureMem );

HRESULT GetHMonitorFromD3D9Device( IDirect3DDevice9* pd3dDevice, HMONITOR hMonitor );


//-----------------------------------------------------------------------------
int main()
{
    // This sample loops over all d3d9 adapters and outputs info about them
    IDirect3D9* pD3D9 = NULL;
    pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
    if( pD3D9 )
    {
        UINT dwAdapterCount = pD3D9->GetAdapterCount();
        for( UINT iAdapter = 0; iAdapter < dwAdapterCount; iAdapter++ )
        {
            D3DADAPTER_IDENTIFIER9 id;
            ZeroMemory( &id, sizeof( D3DADAPTER_IDENTIFIER9 ) );
            pD3D9->GetAdapterIdentifier( iAdapter, 0, &id );
            wprintf( L"\nD3D9 Adapter: %d\nDriver: %S\nDescription: %S\n", iAdapter, id.Driver, id.Description );

            HMONITOR hMonitor = pD3D9->GetAdapterMonitor( iAdapter );
            wprintf( L"hMonitor: 0x%0.8x\n", ( DWORD_PTR )hMonitor );

            MONITORINFOEX mi;
            mi.cbSize = sizeof( MONITORINFOEX );
            GetMonitorInfo( hMonitor, &mi );
            wprintf( L"hMonitor Device Name: %s\n", mi.szDevice );

            DWORD dwAvailableVidMem;
            if( SUCCEEDED( GetVideoMemoryViaDirectDraw( hMonitor, &dwAvailableVidMem ) ) )
                wprintf( L"\tGetVideoMemoryViaDirectDraw\n\t\tdwAvailableVidMem: %d MB (%d)\n",
                         dwAvailableVidMem / 1024 / 1024, dwAvailableVidMem );
            else
                wprintf( L"\tGetVideoMemoryViaDirectDraw\n\t\tn/a\n" );

            DWORD dwDisplayMemory;
            if( SUCCEEDED( GetVideoMemoryViaDxDiag( hMonitor, &dwDisplayMemory ) ) )
                wprintf( L"\tGetVideoMemoryViaDxDiag\n\t\tdwDisplayMemory: %d MB (%d)\n", dwDisplayMemory / 1024 /
                         1024, dwDisplayMemory );
            else
                wprintf( L"\tGetVideoMemoryViaDxDiag\n\t\tn/a\n" );

            DWORD dwAdapterRAM;
            if( SUCCEEDED( GetVideoMemoryViaWMI( hMonitor, &dwAdapterRAM ) ) )
                wprintf( L"\tGetVideoMemoryViaWMI\n\t\tdwAdapterRAM: %d MB (%d)\n", dwAdapterRAM / 1024 / 1024,
                         dwAdapterRAM );
            else
                wprintf( L"\tGetVideoMemoryViaWMI\n\t\tn/a\n" );

            SIZE_T DedicatedVideoMemory;
            SIZE_T DedicatedSystemMemory;
            SIZE_T SharedSystemMemory;
            if( SUCCEEDED( GetVideoMemoryViaDXGI( hMonitor, &DedicatedVideoMemory, &DedicatedSystemMemory,
                                                  &SharedSystemMemory ) ) )
            {
                wprintf(
                    L"\tGetVideoMemoryViaDXGI\n\t\tDedicatedVideoMemory: %d MB (%d)\n\t\tDedicatedSystemMemory: %d MB (%d)\n\t\tSharedSystemMemory: %d MB (%d)\n",
                    DedicatedVideoMemory / 1024 / 1024, DedicatedVideoMemory,
                    DedicatedSystemMemory / 1024 / 1024, DedicatedSystemMemory,
                    SharedSystemMemory / 1024 / 1024, SharedSystemMemory );
            }
            else
            {
                wprintf( L"\tGetVideoMemoryViaDXGI\n\t\tn/a\n" );
            }

            UINT dwAvailableTextureMem;
            if( SUCCEEDED( GetVideoMemoryViaD3D9( hMonitor, &dwAvailableTextureMem ) ) )
                wprintf( L"\tGetVideoMemoryViaD3D9\n\t\tdwAvailableTextureMem: %d MB (%d)\n", dwAvailableTextureMem /
                         1024 / 1024, dwAvailableTextureMem );
            else
                wprintf( L"\tGetVideoMemoryViaD3D9\n\t\tn/a\n" );
        }

        SAFE_RELEASE( pD3D9 );
        return 0;
    }
    else
    {
        wprintf( L"Can't create D3D9 object\n" );
        return -1;
    }

}



