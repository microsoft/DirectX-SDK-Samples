//--------------------------------------------------------------------------------------
// File: BC7EncodeDecode.cpp
//
// Compute Shader Accelerated BC7 Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx10.h>
#include <d3dx11.h>
#include "utils.h"
#include "BC7EncodeDecode.h"

struct BufferBC7
{
    UINT color[4];
};

struct BufferR8G8B8A8
{
    UCHAR color[4];
};

#define BLOCK_SIZE_Y			4
#define BLOCK_SIZE_X			4
#define BLOCK_SIZE				(BLOCK_SIZE_Y * BLOCK_SIZE_X)

HRESULT CGPUBC7Encoder::Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext )
{
    HRESULT hr = S_OK;

    m_pDevice = pDevice;
    m_pContext = pContext;

    // Compile and create Compute Shader 
    printf( "\tCreating Compute Shaders..." );
    V_RETURN( CreateComputeShaderCached( L"BC7Encode.hlsl", "TryMode456CS", "cs_4_0", m_pDevice, &m_pTryMode456CS ) );
    V_RETURN( CreateComputeShaderCached( L"BC7Encode.hlsl", "TryMode137CS", "cs_4_0", m_pDevice, &m_pTryMode137CS ) );
    V_RETURN( CreateComputeShaderCached( L"BC7Encode.hlsl", "TryMode02CS", "cs_4_0", m_pDevice, &m_pTryMode02CS ) );
    V_RETURN( CreateComputeShaderCached( L"BC7Encode.hlsl", "EncodeBlockCS", "cs_4_0", m_pDevice, &m_pEncodeBlockCS ) );
    printf( "done\n" );

    return hr;
}

void CGPUBC7Encoder::Cleanup()
{
    SAFE_RELEASE( m_pEncodedTextureAsBuf );
    SAFE_RELEASE( m_pTryMode456CS );
    SAFE_RELEASE( m_pTryMode137CS );
    SAFE_RELEASE( m_pTryMode02CS );
    SAFE_RELEASE( m_pEncodeBlockCS );
}

HRESULT CGPUBC7Encoder::GPU_BC7EncodeAndSave( ID3D11Texture2D* pSourceTexture, DXGI_FORMAT fmtEncode, WCHAR* strDstFilename )
{
    HRESULT hr = S_OK;

    SAFE_RELEASE( m_pEncodedTextureAsBuf );

    printf( "\tEncoding to BC7..." );
    V_RETURN( GPU_BC7Encode( m_pDevice, m_pContext,
                             pSourceTexture, 
                             fmtEncode, &m_pEncodedTextureAsBuf ) );
    printf( "done\n" );

    wprintf( L"\tSaving to %s...", strDstFilename );
    V_RETURN( GPU_SaveToFile( m_pDevice, m_pContext,
                              pSourceTexture, strDstFilename,
                              fmtEncode, m_pEncodedTextureAsBuf ) );
    printf( "done\n" );

    return hr;
}

HRESULT CGPUBC7Encoder::GPU_SaveToFile( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                                        ID3D11Texture2D* pSrcTexture,
                                        WCHAR* strFilename,
                                        DXGI_FORMAT dstFormat, ID3D11Buffer* pTextureAsBuf )
{
    HRESULT hr = S_OK;

    // BC6H or BC7 can only be handled by feature level 11 device
    ID3D11Device* pDeviceFL11 = NULL;
    ID3D11DeviceContext* pContextFL11 = NULL;
    
    ID3D11Texture2D* pDstTexture = NULL;
    
    if ( pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 )
    {
        // If we are already on a FL11 device, use it directly

        pDeviceFL11 = pDevice;
        pContextFL11 = pContext;
        pDevice->AddRef();
        pContext->AddRef();
    }
    else
    {
        // Otherwise we create a FL11 NULL device

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
    }

    D3D11_TEXTURE2D_DESC desc;
    pSrcTexture->GetDesc( &desc );
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.Format = dstFormat;
    hr = pDeviceFL11->CreateTexture2D( &desc, NULL, &pDstTexture );
    if ( FAILED(hr) )
        return hr;
#if defined(DEBUG) || defined(PROFILE)
    pDstTexture->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 GPUResult" ) - 1, "BC7 GPUResult" );
#endif

    ID3D11Buffer* pReadbackbuf = CreateAndCopyToCPUBuf( pDevice, pContext, pTextureAsBuf );

    D3D11_MAPPED_SUBRESOURCE mappedSrc;
    D3D11_MAPPED_SUBRESOURCE mappedDst;

    pContext->Map( pReadbackbuf, 0, D3D11_MAP_READ, 0, &mappedSrc );
    pContextFL11->Map( pDstTexture, 0, D3D11_MAP_WRITE, 0, &mappedDst );    
    memcpy( mappedDst.pData, mappedSrc.pData, desc.Height * desc.Width * sizeof(BufferBC7) / BLOCK_SIZE );
    pContext->Unmap( pReadbackbuf, 0 );
    pContextFL11->Unmap( pDstTexture, 0 );

    SAFE_RELEASE( pReadbackbuf );

    hr = D3DX11SaveTextureToFile( pContextFL11, pDstTexture, D3DX11_IFF_DDS, strFilename );

    SAFE_RELEASE( pDstTexture );
    SAFE_RELEASE( pDeviceFL11 );
    SAFE_RELEASE( pContextFL11 );

    return hr;
}

HRESULT CGPUBC7Encoder::GPU_BC7Encode( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                                       ID3D11Texture2D* pSrcTexture, 
                                       DXGI_FORMAT dstFormat, ID3D11Buffer** ppDstTextureAsBufOut )
{
    ID3D11ShaderResourceView* pSRV = NULL;
    ID3D11Buffer* pErrBestModeBuffer[2] = { NULL, NULL };
    ID3D11UnorderedAccessView* pUAV = NULL;
    ID3D11UnorderedAccessView* pErrBestModeUAV[2] = { NULL, NULL };
    ID3D11ShaderResourceView* pErrBestModeSRV[2] = { NULL, NULL };
    ID3D11Buffer* pCBCS = NULL;

    if ( !(dstFormat == DXGI_FORMAT_BC7_UNORM || dstFormat == DXGI_FORMAT_BC7_UNORM_SRGB) || 
         ppDstTextureAsBufOut == NULL )
        return E_INVALIDARG;

    D3D11_TEXTURE2D_DESC texSrcDesc;
    pSrcTexture->GetDesc( &texSrcDesc );

    HRESULT hr = S_OK;

    // Create a SRV for input texture        
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Texture2D.MipLevels = texSrcDesc.MipLevels;
        SRVDesc.Texture2D.MostDetailedMip = 0;
        SRVDesc.Format = texSrcDesc.Format;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        V_GOTO( pDevice->CreateShaderResourceView( pSrcTexture, &SRVDesc, &pSRV ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pSRV )
            pSRV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 SRV" ) - 1, "BC7 SRV" );
#endif
    }

    // Create output buffer    
    D3D11_BUFFER_DESC sbOutDesc;
    {
        sbOutDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        sbOutDesc.CPUAccessFlags = 0;
        sbOutDesc.Usage = D3D11_USAGE_DEFAULT;
        sbOutDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        sbOutDesc.StructureByteStride = sizeof( BufferBC7 );
        sbOutDesc.ByteWidth = texSrcDesc.Height * texSrcDesc.Width * sizeof( BufferBC7 ) / BLOCK_SIZE;
        //+ texSrcDesc.Height * texSrcDesc.Width * sizeof( BufferBC7 ) * 5;//For dump
        V_GOTO( pDevice->CreateBuffer(&sbOutDesc, NULL, ppDstTextureAsBufOut) );
        V_GOTO( pDevice->CreateBuffer(&sbOutDesc, NULL, &pErrBestModeBuffer[0]) );
        V_GOTO( pDevice->CreateBuffer(&sbOutDesc, NULL, &pErrBestModeBuffer[1]) );

#if defined(DEBUG) || defined(PROFILE)
        if ( *ppDstTextureAsBufOut )
            (*ppDstTextureAsBufOut)->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 Dest" ) - 1, "BC7 Dest" );
        if ( pErrBestModeBuffer[0] )
            pErrBestModeBuffer[0]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest0" ) - 1, "BC7 ErrBest0" );
        if ( pErrBestModeBuffer[1] )
            pErrBestModeBuffer[1]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest1" ) - 1, "BC7 ErrBest1" );
#endif
    }

    // Create UAV of the output texture    
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
        ZeroMemory( &UAVDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
        UAVDesc.Buffer.FirstElement = 0;
        UAVDesc.Buffer.NumElements = sbOutDesc.ByteWidth / sbOutDesc.StructureByteStride;
        UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        V_GOTO( pDevice->CreateUnorderedAccessView( *ppDstTextureAsBufOut, &UAVDesc, &pUAV ) );
        V_GOTO( pDevice->CreateUnorderedAccessView( pErrBestModeBuffer[0], &UAVDesc, &pErrBestModeUAV[0] ) );
        V_GOTO( pDevice->CreateUnorderedAccessView( pErrBestModeBuffer[1], &UAVDesc, &pErrBestModeUAV[1] ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pUAV )
            pUAV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 Dest UAV" ) - 1, "BC7 Dest UAV" );
        if ( pErrBestModeUAV[0] )
            pErrBestModeUAV[0]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest0 UAV" ) - 1, "BC7 ErrBest0 UAV" );
        if ( pErrBestModeUAV[1] )
            pErrBestModeUAV[1]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest1 UAV" ) - 1, "BC7 ErrBest1 UAV" );
#endif
    }
    
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;     
        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Buffer.FirstElement = 0;
        SRVDesc.Buffer.NumElements = texSrcDesc.Height * texSrcDesc.Width / BLOCK_SIZE;
        SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        V_GOTO( pDevice->CreateShaderResourceView( pErrBestModeBuffer[0], &SRVDesc, &pErrBestModeSRV[0]) );
        V_GOTO( pDevice->CreateShaderResourceView( pErrBestModeBuffer[1], &SRVDesc, &pErrBestModeSRV[1]) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pErrBestModeSRV[0] )
            pErrBestModeSRV[0]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest0 SRV" ) - 1, "BC7 ErrBest0 SRV" );
        if ( pErrBestModeSRV[1] )
            pErrBestModeSRV[1]->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 ErrBest1 SRV" ) - 1, "BC7 ErrBest1 SRV" );
#endif
    }

    // Create constant buffer    
    {
        D3D11_BUFFER_DESC cbDesc;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;
        cbDesc.ByteWidth = sizeof( UINT ) * 8;
        V_GOTO( pDevice->CreateBuffer( &cbDesc, NULL, &pCBCS ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pCBCS )
            pCBCS->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7Encode" ) - 1, "BC7Encode" );
#endif
    }

    int const MAX_BLOCK_BATCH = 64;

    int num_total_blocks = texSrcDesc.Width / BLOCK_SIZE_X * texSrcDesc.Height / BLOCK_SIZE_Y;
    int num_blocks = num_total_blocks;
    int start_block_id = 0;
    while (num_blocks > 0)
    {
        int n = min(num_blocks, MAX_BLOCK_BATCH);
        UINT uThreadGroupCount = n;

        {
            D3D11_MAPPED_SUBRESOURCE cbMapped;
            pContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped );

            UINT param[8];
            param[0] = texSrcDesc.Width;
            param[1] = texSrcDesc.Width / BLOCK_SIZE_X;
            param[2] = dstFormat;// fixed a bug in v0.2
            param[3] = 0;
            param[4] = start_block_id;
            param[5] = num_total_blocks;
            memcpy( cbMapped.pData, param, sizeof( param ) );
            pContext->Unmap( pCBCS, 0 );
        }

        ID3D11ShaderResourceView* pSRVs[] = { pSRV, NULL };
        RunComputeShader( pContext, m_pTryMode456CS, pSRVs, 2, pCBCS, pErrBestModeUAV[0], uThreadGroupCount / 4, 1, 1 );        

        for (int i = 0; i < 3; ++ i)
        {
            int modes[] = { 1, 3, 7 };
            {
                D3D11_MAPPED_SUBRESOURCE cbMapped;
                pContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped );

                UINT param[8];
                param[0] = texSrcDesc.Width;
                param[1] = texSrcDesc.Width / BLOCK_SIZE_X;
                param[2] = dstFormat;// fixed a bug in v0.2
                param[3] = modes[i];
                param[4] = start_block_id;
                param[5] = num_total_blocks;
                memcpy( cbMapped.pData, param, sizeof( param ) );
                pContext->Unmap( pCBCS, 0 );
            }

            pSRVs[1] = pErrBestModeSRV[i & 1];
            RunComputeShader( pContext, m_pTryMode137CS, pSRVs, 2, pCBCS,  pErrBestModeUAV[!(i & 1)], uThreadGroupCount, 1, 1 );
        }               

        for (int i = 0; i < 2; ++ i)
        {
            int modes[] = { 0, 2 };
            {
                D3D11_MAPPED_SUBRESOURCE cbMapped;
                pContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped );

                UINT param[8];
                param[0] = texSrcDesc.Width;
                param[1] = texSrcDesc.Width / BLOCK_SIZE_X;
                param[2] = dstFormat;// fixed a bug in v0.2
                param[3] = modes[i];
                param[4] = start_block_id;
                param[5] = num_total_blocks;
                memcpy( cbMapped.pData, param, sizeof( param ) );
                pContext->Unmap( pCBCS, 0 );
            }

            pSRVs[1] = pErrBestModeSRV[!(i & 1)];
            RunComputeShader( pContext, m_pTryMode02CS, pSRVs, 2, pCBCS,  pErrBestModeUAV[i & 1], uThreadGroupCount, 1, 1 );
        }

        pSRVs[1] = pErrBestModeSRV[1];
        RunComputeShader( pContext, m_pEncodeBlockCS, pSRVs, 2, pCBCS,  pUAV, uThreadGroupCount / 4, 1, 1 );        

        start_block_id += n;
        num_blocks -= n;
    }

quit:
    SAFE_RELEASE(pSRV);
    SAFE_RELEASE(pUAV);
    SAFE_RELEASE(pErrBestModeSRV[0]);
    SAFE_RELEASE(pErrBestModeSRV[1]);
    SAFE_RELEASE(pErrBestModeUAV[0]);
    SAFE_RELEASE(pErrBestModeUAV[1]);
    SAFE_RELEASE(pErrBestModeBuffer[0]);
    SAFE_RELEASE(pErrBestModeBuffer[1]);
    SAFE_RELEASE(pCBCS);

    return hr;
}

HRESULT CGPUBC7Decoder::Initialize( ID3D11Device* pDevice, ID3D11DeviceContext* pContext )
{
    HRESULT hr = S_OK;

    m_pDevice = pDevice;
    m_pContext = pContext;

    // Compile and create Compute Shader 
    printf( "\tCreating Compute Shaders..." );
    V_RETURN( CreateComputeShaderCached( L"BC7Decode.hlsl", "main", "cs_4_0", m_pDevice, &m_pCS ) );
    printf( "done\n" );

    return hr;
}

void CGPUBC7Decoder::Cleanup()
{
    SAFE_RELEASE( m_pRestoredTexture );
    SAFE_RELEASE( m_pCS );
}

HRESULT CGPUBC7Decoder::GPU_BC7DecodeAndSave( ID3D11Texture2D* pSourceTexture, WCHAR* strDstFilename )
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC texSrcDesc;
    pSourceTexture->GetDesc( &texSrcDesc );

    if ( !( texSrcDesc.Format == DXGI_FORMAT_BC7_UNORM || texSrcDesc.Format == DXGI_FORMAT_BC7_UNORM_SRGB ) )
        return E_INVALIDARG;

    // Since pSourceTexture is a BC7 file, and CS4x cannot sample from it, we have to convert
    // it to a resource type which can be accessed
    ID3D11Buffer* pBufferInput = NULL;
    ID3D11Device* pSrcTexDevice = NULL;    
    ID3D11DeviceContext* pSrcTexContext = NULL;
    pSourceTexture->GetDevice( &pSrcTexDevice ); pSrcTexDevice->Release();
    pSrcTexDevice->GetImmediateContext( &pSrcTexContext ); pSrcTexContext->Release();
    {
        D3D11_MAPPED_SUBRESOURCE sourceMapped;
        pSrcTexContext->Map( pSourceTexture, 0, D3D11_MAP_READ, 0, &sourceMapped );

        D3D11_BUFFER_DESC sbInputDesc;
        ZeroMemory( &sbInputDesc, sizeof( sbInputDesc ) );
        sbInputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sbInputDesc.CPUAccessFlags = 0;
        sbInputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        sbInputDesc.StructureByteStride = sizeof( BufferBC7 );
        sbInputDesc.ByteWidth = texSrcDesc.Height * texSrcDesc.Width / BLOCK_SIZE * sbInputDesc.StructureByteStride;
        sbInputDesc.Usage = D3D11_USAGE_DEFAULT;
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = sourceMapped.pData;
        InitData.SysMemPitch = sourceMapped.RowPitch;
        InitData.SysMemSlicePitch = 0;
        hr = m_pDevice->CreateBuffer( &sbInputDesc, &InitData, &pBufferInput );
        pSrcTexContext->Unmap( pSourceTexture, 0 );
        if ( FAILED( hr ) )
            return hr;
#if defined(DEBUG) || defined(PROFILE)
        pBufferInput->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7Decode" ) - 1, "BC7Decode" );
#endif
    }

    hr = GPU_BC7DecodeAndSaveInternal( pBufferInput, texSrcDesc.Width, texSrcDesc.Height, texSrcDesc.Format, strDstFilename );

    SAFE_RELEASE( pBufferInput );

    return hr;
}

HRESULT CGPUBC7Decoder::GPU_BC7DecodeAndSaveInternal( ID3D11Buffer* pSrcTextureAsBuf, UINT uWidth, UINT uHeight, DXGI_FORMAT fmtSrc, WCHAR* strDstFilename )
{
    ID3D11ShaderResourceView* pSRV = NULL;
    ID3D11Buffer* pDstBuffer = NULL;
    ID3D11UnorderedAccessView* pUAV = NULL;
    ID3D11Buffer* pCBCS = NULL;

    if ( pSrcTextureAsBuf == NULL )
        return E_INVALIDARG;

    if ( strDstFilename )
        wprintf( L"\tDecoding from BC7 and saving to %s...", strDstFilename );
    else
        wprintf( L"\tDecoding from BC7..." );

    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC sbInputDesc;
    pSrcTextureAsBuf->GetDesc( &sbInputDesc );
    
    // Create a SRV of the input buffer    
    {                
        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Buffer.FirstElement = 0;
        SRVDesc.Buffer.NumElements = sbInputDesc.ByteWidth / sizeof( BufferBC7 );
        SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        V_GOTO( m_pDevice->CreateShaderResourceView( pSrcTextureAsBuf, &SRVDesc, &pSRV ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pSRV )
            pSRV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7Decode SRV" ) - 1, "BC7Decode SRV" );
#endif
    }

    // Creates output buffer    
    D3D11_BUFFER_DESC sbOutDesc;
    {
        sbOutDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        sbOutDesc.CPUAccessFlags = 0;
        sbOutDesc.Usage = D3D11_USAGE_DEFAULT;
        sbOutDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        sbOutDesc.StructureByteStride = sizeof( BufferR8G8B8A8 );
        sbOutDesc.ByteWidth = uWidth * uHeight * sbOutDesc.StructureByteStride;
        V_GOTO( m_pDevice->CreateBuffer( &sbOutDesc, NULL, &pDstBuffer ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pDstBuffer )
            pDstBuffer->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 DestBuf" ) - 1, "BC7 DestBuf" );
#endif
    }

    // Create a UAV of the output buffer    
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
        ZeroMemory( &UAVDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
        UAVDesc.Buffer.FirstElement = 0;
        UAVDesc.Buffer.NumElements = sbOutDesc.ByteWidth / sbOutDesc.StructureByteStride;
        UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        V_GOTO( m_pDevice->CreateUnorderedAccessView( pDstBuffer, &UAVDesc, &pUAV ) );

#if defined(DEBUG) || defined(PROFILE)
        if ( pUAV )
            pUAV->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7 DestBuf UAV" ) - 1, "BC7 DestBuf UAV" );
#endif
    }

    // Create constant buffer    
    {
        D3D11_BUFFER_DESC cbDesc;
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbDesc.MiscFlags = 0;    
        cbDesc.ByteWidth = sizeof( UINT ) * 8;
        V_GOTO( m_pDevice->CreateBuffer( &cbDesc, NULL, &pCBCS ) );
#if defined(DEBUG) || defined(PROFILE)
        if ( pCBCS )
            pCBCS->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "BC7Decode" ) - 1, "BC7Decode" );
#endif
    }

    // Run Compute Shader    
    {
        int const MAX_BLOCK_BATCH = 32768;

        int num_blocks = uWidth / BLOCK_SIZE_X * uHeight / BLOCK_SIZE_Y;
        int start_block_id = 0;
        while (num_blocks > 0)
        {
            int n = min(num_blocks, MAX_BLOCK_BATCH);

            const int THREAD_GROUP_SIZE = 64;
            UINT uThreadGroupCount = (n * BLOCK_SIZE + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;

            D3D11_MAPPED_SUBRESOURCE cbMapped;
            m_pContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped );

            UINT param[8] = { 0 };
            param[0] = uWidth;
            param[1] = uWidth / BLOCK_SIZE_X;
            param[2] = fmtSrc;
            param[3] = uWidth * uHeight;
            param[4] = start_block_id;
            memcpy( cbMapped.pData, param, sizeof( param ) );
            m_pContext->Unmap( pCBCS, 0 );

            RunComputeShader( m_pContext, m_pCS, &pSRV, 1, pCBCS,  pUAV, uThreadGroupCount, 1, 1 );

            start_block_id += n;
            num_blocks -= n;
        }        
    }

    {
        SAFE_RELEASE( m_pRestoredTexture );

        ID3D11Buffer* pBufferDump = CreateAndCopyToCPUBuf( m_pDevice, m_pContext, pDstBuffer );
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        V_GOTO( m_pContext->Map( pBufferDump, 0, D3D11_MAP_READ, 0, &mappedResource ) );

        D3D11_TEXTURE2D_DESC texDstDesc;
        ZeroMemory( &texDstDesc, sizeof( texDstDesc ) );
        texDstDesc.Width = uWidth;
        texDstDesc.Height = uHeight;
        texDstDesc.MipLevels = 1;
        texDstDesc.ArraySize = 1;
        texDstDesc.Usage = D3D11_USAGE_STAGING;
        texDstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        texDstDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDstDesc.SampleDesc.Count = 1;
        V_GOTO( m_pDevice->CreateTexture2D( &texDstDesc, NULL, &m_pRestoredTexture ) );
#if defined(DEBUG) || defined(PROFILE)
        if ( m_pRestoredTexture )
            m_pRestoredTexture->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof("BC7Restored" ) - 1, "BC7Restored" );
#endif

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        V_GOTO( m_pContext->Map( m_pRestoredTexture, 0, D3D11_MAP_WRITE, 0, &mappedTexture) );
        BYTE* pDst = static_cast<BYTE*>(mappedTexture.pData);
        BYTE* pSrc = static_cast<BYTE*>(mappedResource.pData);
        for ( UINT i = 0; i < texDstDesc.Height; i ++ )
        {
            memcpy( pDst + i * mappedTexture.RowPitch, pSrc + i * uWidth * sizeof( BufferR8G8B8A8 ),
                uWidth * sizeof( BufferR8G8B8A8 ) );
        }
        m_pContext->Unmap( m_pRestoredTexture, 0 );
        m_pContext->Unmap( pBufferDump, 0 );

        SAFE_RELEASE(pBufferDump);
    }

    if ( strDstFilename )
        hr = D3DX11SaveTextureToFile( m_pContext, m_pRestoredTexture, D3DX11_IFF_DDS, strDstFilename );

    printf( "done\n" );

quit:
    SAFE_RELEASE( pSRV );
    SAFE_RELEASE( pDstBuffer );
    SAFE_RELEASE( pUAV );
    SAFE_RELEASE( pCBCS );

    return hr;
}
