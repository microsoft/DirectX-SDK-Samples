//--------------------------------------------------------------------------------------
// File: utils.h
//
// Helper utilities for the Compute Shader Accelerated BC6H BC7 Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef __UTILS_H
#define __UTILS_H

#pragma once

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef V_GOTO
#define V_GOTO( x ) { hr = (x); if ( FAILED(hr) ) {goto quit;} }
#endif

#ifndef V_RETURN
#define V_RETURN( x )    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

//--------------------------------------------------------------------------------------
// This is equivalent to D3D11CreateDevice, except it dynamically loads d3d11.dll,
// this gives us a graceful way to message the user on systems with no d3d11 installed
//--------------------------------------------------------------------------------------
HRESULT WINAPI Dynamic_D3D11CreateDevice( IDXGIAdapter* pAdapter,
                                          D3D_DRIVER_TYPE DriverType,
                                          HMODULE Software,
                                          UINT32 Flags,
                                          D3D_FEATURE_LEVEL* pFeatureLevels,
                                          UINT FeatureLevels,
                                          UINT32 SDKVersion,
                                          ID3D11Device** ppDevice,
                                          D3D_FEATURE_LEVEL* pFeatureLevel,
                                          ID3D11DeviceContext** ppImmediateContext );

//--------------------------------------------------------------------------------------
// Create the D3D device and device context
//--------------------------------------------------------------------------------------
HRESULT CreateDevice( ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppContextOut, BOOL bForceRef = FALSE );

//--------------------------------------------------------------------------------------
// Compiles and creates a Compute Shader from file
// This function tries to cache the compiled shader into the user's local dir,
// so the program can skip shader compilation and create the shader quickly next time it starts
//--------------------------------------------------------------------------------------
HRESULT CreateComputeShaderCached( LPCWSTR pSrcFile, LPCSTR pFunctionName, LPCSTR pProfile,
                                   ID3D11Device* pDevice, ID3D11ComputeShader** ppShaderOut );

//--------------------------------------------------------------------------------------
// Helper function which makes CS invocation more convenient
//--------------------------------------------------------------------------------------
void RunComputeShader( ID3D11DeviceContext* pd3dImmediateContext,
                       ID3D11ComputeShader* pComputeShader,
                       ID3D11ShaderResourceView** pShaderResourceViews, 
					   UINT uNumSRVs,
                       ID3D11Buffer* pCBCS, 
                       ID3D11UnorderedAccessView* pUnorderedAccessView,
                       UINT X, UINT Y, UINT Z );


//--------------------------------------------------------------------------------------
// Loads a texture from file
//-------------------------------------------------------------------------------------- 
HRESULT LoadTextureFromFile( ID3D11Device* pd3dDevice, LPCTSTR lpFileName, DXGI_FORMAT fmtLoadAs, ID3D11Texture2D** ppTextureOut );

//--------------------------------------------------------------------------------------
// Create a CPU accessible buffer and download the content of a GPU buffer into it
//-------------------------------------------------------------------------------------- 
ID3D11Buffer* CreateAndCopyToCPUBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer );

//--------------------------------------------------------------------------------------
// Tries to find the location of the shader file
// This is a trimmed down version of DXUTFindDXSDKMediaFileCch. It only addresses the
// following issue to allow the sample correctly run from within Sample Browser directly
//
// When running the sample from the Sample Browser directly, the executables are located
// in $(DXSDK_DIR)\Samples\C++\Direct3D11\Bin\x86 or x64\, however the shader file is
// in the sample's own dir
//--------------------------------------------------------------------------------------
HRESULT FindDXSDKShaderFileCch( __out_ecount(cchDest) WCHAR* strDestPath,
                                __in int cchDest, 
                                __in LPCWSTR strFilename );

BOOL FileExists( const WCHAR* pszFilename );

#endif