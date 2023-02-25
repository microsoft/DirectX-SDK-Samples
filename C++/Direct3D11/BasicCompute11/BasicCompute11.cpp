//--------------------------------------------------------------------------------------
// File: BasicCompute11.cpp
//
// Demonstrates the basics to get DirectX 11 Compute Shader (aka DirectCompute) up and
// running by implementing Array A + Array B
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <crtdbg.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

// Comment out the following line to use raw buffers instead of structured buffers
#define USE_STRUCTURED_BUFFERS

// If defined, then the hardware/driver must report support for double-precision CS 5.0 shaders or the sample fails to run
//#define TEST_DOUBLE

// The number of elements in a buffer to be tested
const UINT NUM_ELEMENTS = 1024;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
HRESULT CreateComputeDevice( ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppContextOut, BOOL bForceRef );
HRESULT CreateComputeShader( LPCWSTR pSrcFile, LPCSTR pFunctionName, 
                             ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut );
HRESULT CreateStructuredBuffer( ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut );
HRESULT CreateRawBuffer( ID3D11Device* pDevice, UINT uSize, VOID* pInitData, ID3D11Buffer** ppBufOut );
HRESULT CreateBufferSRV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut );
HRESULT CreateBufferUAV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** pUAVOut );
ID3D11Buffer* CreateAndCopyToDebugBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer );
void RunComputeShader( ID3D11DeviceContext* pd3dImmediateContext,
                       ID3D11ComputeShader* pComputeShader,
                       UINT nNumViews, ID3D11ShaderResourceView** pShaderResourceViews, 
                       ID3D11Buffer* pCBCS, void* pCSData, DWORD dwNumDataBytes,
                       ID3D11UnorderedAccessView* pUnorderedAccessView,
                       UINT X, UINT Y, UINT Z );
HRESULT FindDXSDKShaderFileCch( __in_ecount(cchDest) WCHAR* strDestPath,
                                int cchDest, 
                                __in LPCWSTR strFilename );

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3D11Device*               g_pDevice = NULL;
ID3D11DeviceContext*        g_pContext = NULL;
ID3D11ComputeShader*        g_pCS = NULL;

ID3D11Buffer*               g_pBuf0 = NULL;
ID3D11Buffer*               g_pBuf1 = NULL;
ID3D11Buffer*               g_pBufResult = NULL;

ID3D11ShaderResourceView*   g_pBuf0SRV = NULL;
ID3D11ShaderResourceView*   g_pBuf1SRV = NULL;
ID3D11UnorderedAccessView*  g_pBufResultUAV = NULL;

struct BufType
{
    int i;
    float f;
#ifdef TEST_DOUBLE
    double d;
#endif
};
BufType g_vBuf0[NUM_ELEMENTS];
BufType g_vBuf1[NUM_ELEMENTS];

//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int __cdecl main()
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif    

    printf( "Creating device..." );
    if ( FAILED( CreateComputeDevice( &g_pDevice, &g_pContext, FALSE ) ) )
        return 1;
    printf( "done\n" );

    printf( "Creating Compute Shader..." );
    CreateComputeShader( L"BasicCompute11.hlsl", "CSMain", g_pDevice, &g_pCS );
    printf( "done\n" );

    printf( "Creating buffers and filling them with initial data..." );
    for ( int i = 0; i < NUM_ELEMENTS; ++i ) 
    {
        g_vBuf0[i].i = i;
        g_vBuf0[i].f = (float)i;
#ifdef TEST_DOUBLE
        g_vBuf0[i].d = (double)i;
#endif

        g_vBuf1[i].i = i;
        g_vBuf1[i].f = (float)i;
#ifdef TEST_DOUBLE
        g_vBuf1[i].d = (double)i;
#endif
    }

#ifdef USE_STRUCTURED_BUFFERS
    CreateStructuredBuffer( g_pDevice, sizeof(BufType), NUM_ELEMENTS, &g_vBuf0[0], &g_pBuf0 );
    CreateStructuredBuffer( g_pDevice, sizeof(BufType), NUM_ELEMENTS, &g_vBuf1[0], &g_pBuf1 );
    CreateStructuredBuffer( g_pDevice, sizeof(BufType), NUM_ELEMENTS, NULL, &g_pBufResult );
#else
    CreateRawBuffer( g_pDevice, NUM_ELEMENTS * sizeof(BufType), &g_vBuf0[0], &g_pBuf0 );
    CreateRawBuffer( g_pDevice, NUM_ELEMENTS * sizeof(BufType), &g_vBuf1[0], &g_pBuf1 );
    CreateRawBuffer( g_pDevice, NUM_ELEMENTS * sizeof(BufType), NULL, &g_pBufResult );
#endif

#if defined(DEBUG) || defined(PROFILE)
    if ( g_pBuf0 )
        g_pBuf0->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Buffer0" ) - 1, "Buffer0" );
    if ( g_pBuf1 )
        g_pBuf1->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Buffer1" ) - 1, "Buffer1" );
    if ( g_pBufResult )
        g_pBufResult->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Result" ) - 1, "Result" );
#endif

    printf( "done\n" );

    printf( "Creating buffer views..." );
    CreateBufferSRV( g_pDevice, g_pBuf0, &g_pBuf0SRV );
    CreateBufferSRV( g_pDevice, g_pBuf1, &g_pBuf1SRV );
    CreateBufferUAV( g_pDevice, g_pBufResult, &g_pBufResultUAV );

#if defined(DEBUG) || defined(PROFILE)
    if ( g_pBuf0SRV )
        g_pBuf0SRV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Buffer0 SRV" ) - 1, "Buffer0 SRV" );
    if ( g_pBuf1SRV )
        g_pBuf1SRV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Buffer1 SRV" ) - 1, "Buffer1 SRV" );
    if ( g_pBufResultUAV )
        g_pBufResultUAV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Result UAV" ) - 1, "Result UAV" );
#endif

    printf( "done\n" );

    printf( "Running Compute Shader..." );
    ID3D11ShaderResourceView* aRViews[2] = { g_pBuf0SRV, g_pBuf1SRV };
    RunComputeShader( g_pContext, g_pCS, 2, aRViews, NULL, NULL, 0, g_pBufResultUAV, NUM_ELEMENTS, 1, 1 );
    printf( "done\n" );

    // Read back the result from GPU, verify its correctness against result computed by CPU
    {
        ID3D11Buffer* debugbuf = CreateAndCopyToDebugBuf( g_pDevice, g_pContext, g_pBufResult );
        D3D11_MAPPED_SUBRESOURCE MappedResource; 
        BufType *p;
        g_pContext->Map( debugbuf, 0, D3D11_MAP_READ, 0, &MappedResource );

        // Set a break point here and put down the expression "p, 1024" in your watch window to see what has been written out by our CS
        // This is also a common trick to debug CS programs.
        p = (BufType*)MappedResource.pData;

        // Verify that if Compute Shader has done right
        printf( "Verifying against CPU result..." );
        BOOL bSuccess = TRUE;
        for ( int i = 0; i < NUM_ELEMENTS; ++i )
            if ( (p[i].i != g_vBuf0[i].i + g_vBuf1[i].i)
                 || (p[i].f != g_vBuf0[i].f + g_vBuf1[i].f)
#ifdef TEST_DOUBLE
                 || (p[i].d != g_vBuf0[i].d + g_vBuf1[i].d)
#endif
               )
            {
                 printf( "failure\n" );
                 bSuccess = FALSE;

                 break;
            }
        if ( bSuccess )
            printf( "succeeded\n" );

        g_pContext->Unmap( debugbuf, 0 );

        SAFE_RELEASE( debugbuf );
    }
    
    printf( "Cleaning up...\n" );
    SAFE_RELEASE( g_pBuf0SRV );
    SAFE_RELEASE( g_pBuf1SRV );
    SAFE_RELEASE( g_pBufResultUAV );
    SAFE_RELEASE( g_pBuf0 );
    SAFE_RELEASE( g_pBuf1 );
    SAFE_RELEASE( g_pBufResult );
    SAFE_RELEASE( g_pCS );
    SAFE_RELEASE( g_pContext );
    SAFE_RELEASE( g_pDevice );

    return 0;
}

//--------------------------------------------------------------------------------------
// This is equivalent to D3D11CreateDevice, except it dynamically loads d3d11.dll,
// this gives us a graceful way to message the user on systems with no d3d11 installed
//--------------------------------------------------------------------------------------
HRESULT WINAPI Dynamic_D3D11CreateDevice( IDXGIAdapter* pAdapter,
                                          D3D_DRIVER_TYPE DriverType,
                                          HMODULE Software,
                                          UINT32 Flags,
                                          CONST D3D_FEATURE_LEVEL* pFeatureLevels,
                                          UINT FeatureLevels,
                                          UINT32 SDKVersion,
                                          ID3D11Device** ppDevice,
                                          D3D_FEATURE_LEVEL* pFeatureLevel,
                                          ID3D11DeviceContext** ppImmediateContext )
{
    typedef HRESULT (WINAPI * LPD3D11CREATEDEVICE)( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, CONST D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
    static LPD3D11CREATEDEVICE  s_DynamicD3D11CreateDevice = NULL;
    
    if ( s_DynamicD3D11CreateDevice == NULL )
    {            
        HMODULE hModD3D11 = LoadLibrary( L"d3d11.dll" );

        if ( hModD3D11 == NULL )
        {
            // Ensure this "D3D11 absent" message is shown only once. As sometimes, the app would like to try
            // to create device multiple times
            static bool bMessageAlreadyShwon = false;
            
            if ( !bMessageAlreadyShwon )
            {
                OSVERSIONINFOEX osv;
                memset( &osv, 0, sizeof(osv) );
                osv.dwOSVersionInfoSize = sizeof(osv);
                GetVersionEx( (LPOSVERSIONINFO)&osv );

                if ( ( osv.dwMajorVersion > 6 )
                    || ( osv.dwMajorVersion == 6 && osv.dwMinorVersion >= 1 ) 
                    || ( osv.dwMajorVersion == 6 && osv.dwMinorVersion == 0 && osv.dwBuildNumber > 6002 ) )
                {

                    MessageBox( 0, L"Direct3D 11 components were not found.", L"Error", MB_ICONEXCLAMATION );
                    // This should not happen, but is here for completeness as the system could be
                    // corrupted or some future OS version could pull D3D11.DLL for some reason
                }
                else if ( osv.dwMajorVersion == 6 && osv.dwMinorVersion == 0 && osv.dwBuildNumber == 6002 )
                {

                    MessageBox( 0, L"Direct3D 11 components were not found, but are available for"\
                        L" this version of Windows.\n"\
                        L"For details see Microsoft Knowledge Base Article #971644\n"\
                        L"http://support.microsoft.com/default.aspx/kb/971644/", L"Error", MB_ICONEXCLAMATION );

                }
                else if ( osv.dwMajorVersion == 6 && osv.dwMinorVersion == 0 )
                {
                    MessageBox( 0, L"Direct3D 11 components were not found. Please install the latest Service Pack.\n"\
                        L"For details see Microsoft Knowledge Base Article #935791\n"\
                        L" http://support.microsoft.com/default.aspx/kb/935791", L"Error", MB_ICONEXCLAMATION );

                }
                else
                {
                    MessageBox( 0, L"Direct3D 11 is not supported on this OS.", L"Error", MB_ICONEXCLAMATION );
                }

                bMessageAlreadyShwon = true;
            }            

            return E_FAIL;
        }

        s_DynamicD3D11CreateDevice = ( LPD3D11CREATEDEVICE )GetProcAddress( hModD3D11, "D3D11CreateDevice" );           
    }

    return s_DynamicD3D11CreateDevice( pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels,
                                       SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext );
}

//--------------------------------------------------------------------------------------
// Create the D3D device and device context suitable for running Compute Shaders(CS)
//--------------------------------------------------------------------------------------
HRESULT CreateComputeDevice( ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppContextOut, BOOL bForceRef )
{    
    *ppDeviceOut = NULL;
    *ppContextOut = NULL;
    
    HRESULT hr = S_OK;

    UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
    uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL flOut;
    static const D3D_FEATURE_LEVEL flvl[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
    
    BOOL bNeedRefDevice = FALSE;
    if ( !bForceRef )
    {
        hr = Dynamic_D3D11CreateDevice( NULL,                        // Use default graphics card
                                        D3D_DRIVER_TYPE_HARDWARE,    // Try to create a hardware accelerated device
                                        NULL,                        // Do not use external software rasterizer module
                                        uCreationFlags,              // Device creation flags
                                        flvl,
                                        sizeof(flvl) / sizeof(D3D_FEATURE_LEVEL),
                                        D3D11_SDK_VERSION,           // SDK version
                                        ppDeviceOut,                 // Device out
                                        &flOut,                      // Actual feature level created
                                        ppContextOut );              // Context out
        
        if ( SUCCEEDED( hr ) )
        {
            // A hardware accelerated device has been created, so check for Compute Shader support

            // If we have a device >= D3D_FEATURE_LEVEL_11_0 created, full CS5.0 support is guaranteed, no need for further checks
            if ( flOut < D3D_FEATURE_LEVEL_11_0 )            
            {
#ifdef TEST_DOUBLE
                bNeedRefDevice = TRUE;
                printf( "No hardware Compute Shader 5.0 capable device found (required for doubles), trying to create ref device.\n" );
#else
                // Otherwise, we need further check whether this device support CS4.x (Compute on 10)
                D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
                (*ppDeviceOut)->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
                if ( !hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
                {
                    bNeedRefDevice = TRUE;
                    printf( "No hardware Compute Shader capable device found, trying to create ref device.\n" );
                }
#endif
            }

#ifdef TEST_DOUBLE
            else
            {
                // Double-precision support is an optional feature of CS 5.0
                D3D11_FEATURE_DATA_DOUBLES hwopts;
                (*ppDeviceOut)->CheckFeatureSupport( D3D11_FEATURE_DOUBLES, &hwopts, sizeof(hwopts) );
                if ( !hwopts.DoublePrecisionFloatShaderOps )
                {
                    bNeedRefDevice = TRUE;
                    printf( "No hardware double-precision capable device found, trying to create ref device.\n" );
                }
            }
#endif
        }
    }
    
    if ( bForceRef || FAILED(hr) || bNeedRefDevice )
    {
        // Either because of failure on creating a hardware device or hardware lacking CS capability, we create a ref device here

        SAFE_RELEASE( *ppDeviceOut );
        SAFE_RELEASE( *ppContextOut );
        
        hr = Dynamic_D3D11CreateDevice( NULL,                        // Use default graphics card
                                        D3D_DRIVER_TYPE_REFERENCE,   // Try to create a hardware accelerated device
                                        NULL,                        // Do not use external software rasterizer module
                                        uCreationFlags,              // Device creation flags
                                        flvl,
                                        sizeof(flvl) / sizeof(D3D_FEATURE_LEVEL),
                                        D3D11_SDK_VERSION,           // SDK version
                                        ppDeviceOut,                 // Device out
                                        &flOut,                      // Actual feature level created
                                        ppContextOut );              // Context out
        if ( FAILED(hr) )
        {
            printf( "Reference rasterizer device create failure\n" );
            return hr;
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Compile and create the CS
//--------------------------------------------------------------------------------------
HRESULT CreateComputeShader( LPCWSTR pSrcFile, LPCSTR pFunctionName, 
                             ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut )
{
    HRESULT hr;

    // Finds the correct path for the shader file.
    // This is only required for this sample to be run correctly from within the Sample Browser,
    // in your own projects, these lines could be removed safely
    WCHAR str[MAX_PATH];
    hr = FindDXSDKShaderFileCch( str, MAX_PATH, pSrcFile );
    if ( FAILED(hr) )
        return hr;
    
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    const D3D_SHADER_MACRO defines[] = 
    {
#ifdef USE_STRUCTURED_BUFFERS
        "USE_STRUCTURED_BUFFERS", "1",
#endif

#ifdef TEST_DOUBLE
        "TEST_DOUBLE", "1",
#endif
        NULL, NULL
    };

    // We generally prefer to use the higher CS shader profile when possible as CS 5.0 is better performance on 11-class hardware
    LPCSTR pProfile = ( pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 ) ? "cs_5_0" : "cs_4_0";

    ID3DBlob* pErrorBlob = NULL;
    ID3DBlob* pBlob = NULL;
    hr = D3DX11CompileFromFile( str, defines, NULL, pFunctionName, pProfile, 
        dwShaderFlags, NULL, NULL, &pBlob, &pErrorBlob, NULL );
    if ( FAILED(hr) )
    {
        if ( pErrorBlob )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );

        SAFE_RELEASE( pErrorBlob );
        SAFE_RELEASE( pBlob );    

        return hr;
    }    

    hr = pDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, ppShaderOut );

#if defined(DEBUG) || defined(PROFILE)
    if ( *ppShaderOut )
        (*ppShaderOut)->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName );
#endif

    SAFE_RELEASE( pErrorBlob );
    SAFE_RELEASE( pBlob );

    return hr;
}

//--------------------------------------------------------------------------------------
// Create Structured Buffer
//--------------------------------------------------------------------------------------
HRESULT CreateStructuredBuffer( ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut )
{
    *ppBufOut = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.ByteWidth = uElementSize * uCount;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = uElementSize;

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
    } else
        return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

//--------------------------------------------------------------------------------------
// Create Raw Buffer
//--------------------------------------------------------------------------------------
HRESULT CreateRawBuffer( ID3D11Device* pDevice, UINT uSize, VOID* pInitData, ID3D11Buffer** ppBufOut )
{
    *ppBufOut = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = uSize;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
    } else
        return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured or Raw Buffers
//--------------------------------------------------------------------------------------
HRESULT CreateBufferSRV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof(descBuf) );
    pBuffer->GetDesc( &descBuf );

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    desc.BufferEx.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
    {
        // This is a Raw Buffer

        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
    } else
    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
    {
        // This is a Structured Buffer

        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
    } else
    {
        return E_INVALIDARG;
    }

    return pDevice->CreateShaderResourceView( pBuffer, &desc, ppSRVOut );
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured or Raw Buffers
//-------------------------------------------------------------------------------------- 
HRESULT CreateBufferUAV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof(descBuf) );
    pBuffer->GetDesc( &descBuf );
        
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
    {
        // This is a Raw Buffer

        desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
        desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
    } else
    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
    {
        // This is a Structured Buffer

        desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
        desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
    } else
    {
        return E_INVALIDARG;
    }
    
    return pDevice->CreateUnorderedAccessView( pBuffer, &desc, ppUAVOut );
}

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer and download the content of a GPU buffer into it
// This function is very useful for debugging CS programs
//-------------------------------------------------------------------------------------- 
ID3D11Buffer* CreateAndCopyToDebugBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer )
{
    ID3D11Buffer* debugbuf = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    pBuffer->GetDesc( &desc );
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    if ( SUCCEEDED(pDevice->CreateBuffer(&desc, NULL, &debugbuf)) )
    {
#if defined(DEBUG) || defined(PROFILE)
        debugbuf->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Debug" ) - 1, "Debug" );
#endif

        pd3dImmediateContext->CopyResource( debugbuf, pBuffer );
    }

    return debugbuf;
}

//--------------------------------------------------------------------------------------
// Run CS
//-------------------------------------------------------------------------------------- 
void RunComputeShader( ID3D11DeviceContext* pd3dImmediateContext,
                      ID3D11ComputeShader* pComputeShader,
                      UINT nNumViews, ID3D11ShaderResourceView** pShaderResourceViews, 
                      ID3D11Buffer* pCBCS, void* pCSData, DWORD dwNumDataBytes,
                      ID3D11UnorderedAccessView* pUnorderedAccessView,
                      UINT X, UINT Y, UINT Z )
{
    pd3dImmediateContext->CSSetShader( pComputeShader, NULL, 0 );
    pd3dImmediateContext->CSSetShaderResources( 0, nNumViews, pShaderResourceViews );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &pUnorderedAccessView, NULL );
    if ( pCBCS )
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        pd3dImmediateContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
        memcpy( MappedResource.pData, pCSData, dwNumDataBytes );
        pd3dImmediateContext->Unmap( pCBCS, 0 );
        ID3D11Buffer* ppCB[1] = { pCBCS };
        pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppCB );
    }

    pd3dImmediateContext->Dispatch( X, Y, Z );

    pd3dImmediateContext->CSSetShader( NULL, NULL, 0 );

    ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );

    ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
    pd3dImmediateContext->CSSetShaderResources( 0, 2, ppSRVNULL );

    ID3D11Buffer* ppCBNULL[1] = { NULL };
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppCBNULL );
}

//--------------------------------------------------------------------------------------
// Tries to find the location of the shader file
// This is a trimmed down version of DXUTFindDXSDKMediaFileCch. It only addresses the
// following issue to allow the sample correctly run from within Sample Browser directly
//
// When running the sample from the Sample Browser directly, the executables are located
// in $(DXSDK_DIR)\Samples\C++\Direct3D11\Bin\x86 or x64\, however the shader file is
// in the sample's own dir
//--------------------------------------------------------------------------------------
HRESULT FindDXSDKShaderFileCch( __in_ecount(cchDest) WCHAR* strDestPath,
                                int cchDest, 
                                __in LPCWSTR strFilename )
{
    if( NULL == strFilename || strFilename[0] == 0 || NULL == strDestPath || cchDest < 10 )
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] =
    {
        0
    };
    WCHAR strExeName[MAX_PATH] =
    {
        0
    };
    WCHAR* strLastSlash = NULL;
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr( strExePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        wcscpy_s( strExeName, MAX_PATH, &strLastSlash[1] );

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr( strExeName, TEXT( '.' ) );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    // Search in directories:
    //      .\
    //      %EXE_DIR%\..\..\%EXE_NAME%

    wcscpy_s( strDestPath, cchDest, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return true;

    swprintf_s( strDestPath, cchDest, L"%s\\..\\..\\%s\\%s", strExePath, strExeName, strFilename );
    if( GetFileAttributes( strDestPath ) != 0xFFFFFFFF )
        return true;    

    // On failure, return the file as the path but also return an error code
    wcscpy_s( strDestPath, cchDest, strFilename );

    return E_FAIL;
}