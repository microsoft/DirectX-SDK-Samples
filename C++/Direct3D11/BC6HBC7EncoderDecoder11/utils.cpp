//--------------------------------------------------------------------------------------
// File: utils.cpp
//
// Helper utilities for the Compute Shader Accelerated BC6H BC7 Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <shlobj.h>
#include <string>
#include "utils.h"


HRESULT WINAPI Dynamic_D3D11CreateDevice( IDXGIAdapter* pAdapter,
                                          D3D_DRIVER_TYPE DriverType,
                                          HMODULE Software,
                                          UINT32 Flags,
                                          D3D_FEATURE_LEVEL* pFeatureLevels,
                                          UINT FeatureLevels,
                                          UINT32 SDKVersion,
                                          ID3D11Device** ppDevice,
                                          D3D_FEATURE_LEVEL* pFeatureLevel,
                                          ID3D11DeviceContext** ppImmediateContext )
{
    typedef HRESULT (WINAPI * LPD3D11CREATEDEVICE)( IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT32, D3D_FEATURE_LEVEL*, UINT, UINT32, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** );
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

HRESULT CreateDevice( ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppContextOut, BOOL bForceRef /*= FALSE*/ )
{
    *ppDeviceOut = NULL;
    *ppContextOut = NULL;
    
    HRESULT hr = S_OK;

    UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
    uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL flOut = D3D_FEATURE_LEVEL_9_1;
    
    if ( !bForceRef )
    {
        hr = Dynamic_D3D11CreateDevice( NULL,                        // Use default graphics card
                                         D3D_DRIVER_TYPE_HARDWARE,    // Try to create a hardware accelerated device
                                         NULL,                        // Do not use external software rasterizer module
                                         uCreationFlags,              // Device creation flags
                                         NULL,                        // Try to get greatest feature level available
                                         0,                           // # of elements in the previous array
                                         D3D11_SDK_VERSION,           // SDK version
                                         ppDeviceOut,                 // Device out
                                         &flOut,                      // Actual feature level created
                                         ppContextOut );              // Context out                
    }
    
    if ( bForceRef || FAILED(hr) )
    {
        // Either because of failure on creating a hardware device or we are forced to create a ref device, we create a ref device here

        SAFE_RELEASE( *ppDeviceOut );
        SAFE_RELEASE( *ppContextOut );
        
        hr = Dynamic_D3D11CreateDevice( NULL,                        // Use default graphics card
                                         D3D_DRIVER_TYPE_REFERENCE,   // Try to create a hardware accelerated device
                                         NULL,                        // Do not use external software rasterizer module
                                         uCreationFlags,              // Device creation flags
                                         NULL,                        // Try to get greatest feature level available
                                         0,                           // # of elements in the previous array
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

void GetLocalCacheDir( WCHAR* strPath, UINT cchPath )
{
    WCHAR strFolder[MAX_PATH] = L"\\BC6HBC7EncoderDecoder11\\";

    SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath );
    wcscat_s( strPath, cchPath, strFolder );
}

HRESULT CreateComputeShaderCached( LPCWSTR pSrcFile, LPCSTR pFunctionName, LPCSTR pProfile,
                                   ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut )
{
    HRESULT hr;

    WCHAR strDirectory[MAX_PATH] = {0};
    GetLocalCacheDir( strDirectory, MAX_PATH );

    // First check whether our cache dir exists, if not, create it
    if ( 0xFFFFFFFF == GetFileAttributes( strDirectory ) )
        if ( !CreateDirectory( strDirectory, NULL ) )
            return E_FAIL;       

    // Finds the correct path for the shader file.
    // This is only required for this sample to be run correctly from within the Sample Browser,
    // in your own projects, these lines could be removed safely
    WCHAR str[MAX_PATH];
    hr = FindDXSDKShaderFileCch( str, MAX_PATH, pSrcFile );
    if ( FAILED(hr) )
    {
        printf( "Shader file(s) not found. Make sure that BC6HEncode.hlsl, BC6HDecode.hlsl, BC7Encode.hlsl and BC7Decode.hlsl are in directories the exe can find." );
        return hr;
    }

    // Then check whether our cached shader exists,
    std::wstring fname( str );
    std::wstring::size_type p = fname.rfind( L"\\" );
    if ( p == std::wstring::npos )
        p = fname.rfind( L"/" );
    if ( p != std::wstring::npos )
        fname.erase( 0, p + 1 );

    fname.erase( fname.rfind( '.' ), fname.length() );
    fname += '_';
    WCHAR buf[256];
    MultiByteToWideChar( CP_ACP, 0, pFunctionName, -1, buf, 256 );
    fname += buf;
    fname += L".blob";
    fname.insert( 0, strDirectory );

    ID3DBlob* pBlob = NULL;

again:
    if ( 0xFFFFFFFF == GetFileAttributes( &fname[0] ) )
    {
        // The cached shader file does not exist, then compile it and save it
        
        ID3DBlob* pErrorBlob = NULL;        
        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
        // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
        // Setting this flag improves the shader debugging experience, but still allows 
        // the shaders to be optimized and to run exactly the way they will run in 
        // the release configuration of this program.
        dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
        hr = D3DX11CompileFromFile( str, NULL, NULL, pFunctionName, pProfile, 
            dwShaderFlags, NULL, NULL, &pBlob, &pErrorBlob, NULL );
        if ( FAILED(hr) )
        {
            if ( pErrorBlob )
                OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );

            SAFE_RELEASE( pErrorBlob );
            SAFE_RELEASE( pBlob );    

            return hr;
        }  

        SAFE_RELEASE( pErrorBlob );

        // Write the compiled blob into the cache file
        HANDLE hFile = CreateFile( fname.c_str(), FILE_WRITE_DATA, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                   FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if ( INVALID_HANDLE_VALUE == hFile )
            return E_FAIL;

        DWORD dwVersion = D3DX11_SDK_VERSION;
        DWORD dwBytesWritten = 0;
        WriteFile( hFile, &dwVersion, sizeof(dwVersion), &dwBytesWritten, NULL );
        WriteFile( hFile, pBlob->GetBufferPointer(), (DWORD)pBlob->GetBufferSize(), &dwBytesWritten, NULL );

        CloseHandle( hFile );
    } else
    {
        // The cached shader file exists, load it as shader blob

        HANDLE hFile = CreateFile( fname.c_str(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                   FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if ( INVALID_HANDLE_VALUE == hFile )
            return E_FAIL;

        DWORD dwVersion = 0;
        DWORD dwBytesRead = 0;

        // Check if we have an updated hlsl compiler and re-generate the shader cache as necessary
        ReadFile( hFile, &dwVersion, sizeof(dwVersion), &dwBytesRead, NULL );
        if ( dwVersion != D3DX11_SDK_VERSION )
        {
            CloseHandle( hFile );
            DeleteFile( fname.c_str() );

            goto again;
        }        

        D3DCreateBlob( GetFileSize( hFile, NULL ) - sizeof(dwVersion), &pBlob );        
        ReadFile( hFile, pBlob->GetBufferPointer(), (DWORD)pBlob->GetBufferSize(), &dwBytesRead, NULL );
        CloseHandle( hFile );
    }
         
    hr = pDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, ppShaderOut );
#if defined(DEBUG) || defined(PROFILE)
    if ( *ppShaderOut )
        (*ppShaderOut)->SetPrivateData( WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName );
#endif
    SAFE_RELEASE( pBlob );

    return hr;
}

void RunComputeShader( ID3D11DeviceContext* pd3dImmediateContext,
                       ID3D11ComputeShader* pComputeShader,
                       ID3D11ShaderResourceView** pShaderResourceViews, 
					   UINT uNumSRVs,
                       ID3D11Buffer* pCBCS, 
                       ID3D11UnorderedAccessView* pUnorderedAccessView,
                       UINT X, UINT Y, UINT Z )
{    
    pd3dImmediateContext->CSSetShader( pComputeShader, NULL, 0 );
    pd3dImmediateContext->CSSetShaderResources( 0, uNumSRVs, pShaderResourceViews );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &pUnorderedAccessView, NULL );
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &pCBCS );

    pd3dImmediateContext->Dispatch( X, Y, Z );

    ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );
    ID3D11ShaderResourceView* ppSRVNULL[3] = { NULL, NULL, NULL };
    pd3dImmediateContext->CSSetShaderResources( 0, 3, ppSRVNULL );
    ID3D11Buffer* ppBufferNULL[1] = { NULL };
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppBufferNULL );
}

HRESULT LoadTextureFromFile( ID3D11Device* pd3dDevice, LPCTSTR lpFileName, DXGI_FORMAT fmtLoadAs, ID3D11Texture2D** ppTextureOut )
{
    HRESULT hr = S_OK;

    D3DX11_IMAGE_LOAD_INFO img_load_info;
    img_load_info.Width = D3DX11_DEFAULT;
    img_load_info.Height = D3DX11_DEFAULT;
    img_load_info.Depth = D3DX11_DEFAULT;
    img_load_info.FirstMipLevel = D3DX11_DEFAULT;
    img_load_info.MipLevels = 1;
    img_load_info.Usage = D3D11_USAGE_STAGING;
    img_load_info.BindFlags = 0;
    img_load_info.CpuAccessFlags = D3D11_CPU_ACCESS_READ;
    img_load_info.MiscFlags = 0;
    img_load_info.Format = fmtLoadAs;    
    img_load_info.Filter = D3DX11_DEFAULT;
    img_load_info.MipFilter = D3DX11_DEFAULT;
    img_load_info.pSrcInfo = NULL;

    D3DX11CreateTextureFromFile( pd3dDevice, lpFileName, &img_load_info, NULL, 
        (ID3D11Resource**)(ppTextureOut), &hr );

    if ( FAILED( hr ) && (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0) && FileExists( lpFileName ) )
    {
        // The texture file exists but failed to be loaded, and our device < FL11
        // Might be a BC6H or BC7 texture, which needs a FL11 device to load it

        ID3D11Device* pDeviceFL11 = NULL;
        ID3D11DeviceContext* pContextFL11 = NULL;

        UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(DEBUG) || defined(_DEBUG)
        uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL flIn[] = { D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL flOut = D3D_FEATURE_LEVEL_9_1;

        hr = Dynamic_D3D11CreateDevice( NULL,
                                D3D_DRIVER_TYPE_NULL,
                                NULL,
                                uCreationFlags,              // Device creation flags
                                flIn,
                                1,
                                D3D11_SDK_VERSION,           // SDK version
                                &pDeviceFL11,                // Device out
                                &flOut,                      // Actual feature level created
                                &pContextFL11 );             // Context out 
        if ( FAILED( hr ) )
            return hr;

        // Later this texture will be converted on CPU to a FL10 accessible resource,
        // so change the flags to make the texture CPU friendly
        img_load_info.BindFlags = 0;
        img_load_info.CpuAccessFlags = D3D11_CPU_ACCESS_READ;
        img_load_info.Usage = D3D11_USAGE_STAGING;

        D3DX11CreateTextureFromFile( pDeviceFL11, lpFileName, &img_load_info, NULL, 
            (ID3D11Resource**)(ppTextureOut), &hr );

        SAFE_RELEASE( pDeviceFL11 );
        SAFE_RELEASE( pContextFL11 );
    }

    return hr;
}

ID3D11Buffer* CreateAndCopyToCPUBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer )
{
    ID3D11Buffer* cpubuf = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    pBuffer->GetDesc( &desc );
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    if ( SUCCEEDED(pDevice->CreateBuffer(&desc, NULL, &cpubuf)) )
    {
#if defined(DEBUG) || defined(PROFILE)
        cpubuf->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "CPU" ) - 1, "CPU" );
#endif
        pd3dImmediateContext->CopyResource( cpubuf, pBuffer );
    }

    return cpubuf;
}

HRESULT FindDXSDKShaderFileCch( WCHAR* strDestPath, int cchDest, 
                                LPCWSTR strFilename )
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

BOOL FileExists( const WCHAR* pszFilename )
{
    FILE *f = NULL;

    if ( _wfopen_s( &f, pszFilename, L"rb" ) != 0 )
    {
        return FALSE;
    }
    fclose(f);

    return TRUE;
}
