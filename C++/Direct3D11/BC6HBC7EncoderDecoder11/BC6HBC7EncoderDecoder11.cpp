//--------------------------------------------------------------------------------------
// File: BC6HBC7EncoderDecoder11.cpp
//
// The main file of the Compute Shader Accelerated BC6H BC7 Encoder/Decoder
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdio.h>
#include <crtdbg.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <d3dx10math.h>    // for D3DXFloat16To32Array
#include <math.h>
#include <vector>
#include <string>
#include "utils.h"
#include "BC6HEncodeDecode.h"
#include "BC7EncodeDecode.h"


ID3D11Device*               g_pDevice = NULL;
ID3D11DeviceContext*        g_pContext = NULL;
ID3D11Texture2D*            g_pSourceTexture = NULL;
BOOL                        g_bCS4xAvailable = FALSE;

CGPUBC6HEncoder             g_GPUBC6HEncoder;
CGPUBC6HDecoder             g_GPUBC6HDecoder;

CGPUBC7Encoder              g_GPUBC7Encoder;
CGPUBC7Decoder              g_GPUBC7Decoder;

HRESULT EncodeDecodeBC7( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture );
HRESULT EncodeDecodeBC6H( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture );
HRESULT DecodeBC7( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture );
HRESULT DecodeBC6H( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture );

HRESULT CPU_EncodeDecode( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                          ID3D11Texture2D* pSrcTexture,
                          DXGI_FORMAT dstFormat, ID3D11Texture2D** ppDstTextureOut );

struct CCommandLineOptions 
{
    enum Mode
    {
        MODE_ENCODE_BC6HS,
        MODE_ENCODE_BC6HU,
        MODE_ENCODE_BC7,
        MODE_DECODE,
        MODE_NOT_SET
    } mode;

    BOOL bShowReconstructionError;
    BOOL bSaveReconstructionFile;
    bool bForceCPU;

    CCommandLineOptions() :
        mode(MODE_NOT_SET),
        bShowReconstructionError(FALSE),
        bSaveReconstructionFile(FALSE),
        bForceCPU(FALSE)
    {
    }

    BOOL SetMode( Mode mode )
    {
        // Three modes are mutually exclusive
        if ( this->mode == MODE_NOT_SET )
        {
            this->mode = mode;
            return TRUE;
        }

        printf( "Only one of the /bc6hs, /bc6hu, /bc7 and /decode options can be set at a time\n" );
        return FALSE;
    }

} g_CommandLineOptions;

VOID Cleanup()
{
    g_GPUBC6HEncoder.Cleanup();
    g_GPUBC6HDecoder.Cleanup();
    g_GPUBC7Encoder.Cleanup();
    g_GPUBC7Decoder.Cleanup();
    SAFE_RELEASE( g_pSourceTexture );
    SAFE_RELEASE( g_pContext );
    SAFE_RELEASE( g_pDevice );
}

BOOL ParseCommandLine( int argc, WCHAR* argv[] )
{
    // Parse command line options
    for ( int i = 1; i < argc; ++i )
    {
        if ( wcscmp( argv[i], L"/bc6hs" ) == 0 )
        {
            if ( !g_CommandLineOptions.SetMode( CCommandLineOptions::MODE_ENCODE_BC6HS ) )
                return FALSE;
        } else
        if ( wcscmp( argv[i], L"/bc6hu" ) == 0 )
        {
            if ( !g_CommandLineOptions.SetMode( CCommandLineOptions::MODE_ENCODE_BC6HU ) )
                return FALSE;
        } else
        if ( wcscmp( argv[i], L"/bc7" ) == 0 )
        {
            if ( !g_CommandLineOptions.SetMode( CCommandLineOptions::MODE_ENCODE_BC7 ) )
                return FALSE;
        } else
        if ( wcscmp( argv[i], L"/decode" ) == 0 )
        {
            if ( !g_CommandLineOptions.SetMode( CCommandLineOptions::MODE_DECODE ) )
                return FALSE;
        } else
        if ( wcscmp( argv[i], L"/cpu" ) == 0 )
        {
            g_CommandLineOptions.bForceCPU = TRUE;
        } else
        if ( wcscmp( argv[i], L"/sre" ) == 0 )
        {
            g_CommandLineOptions.bShowReconstructionError = TRUE;
        } else
        if ( wcscmp( argv[i], L"/srf" ) == 0 )
        {
            g_CommandLineOptions.bSaveReconstructionFile = TRUE;
        } else
        if ( argv[i][0] == L'/' )
        {
            wprintf( L"Unknown option %s\n", argv[i] );
            return FALSE;
        }
    }

    if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_NOT_SET )
        return FALSE;

    return TRUE;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    printf( "Microsoft (R) Direct3D11 Compute Shader Accelerated BC6H BC7 Encoder/Decoder\n\n" );
    
    if ( argc < 3 )
    {                
        printf( "Usage: BC6HBC7EncoderDecoder11.exe (options) Filename0 Filename1 Filename2...\n\n" );

        printf( "\tWhere (options) can be the following:\n\n" );

        printf( "\t/bc6hs\t\tEncode to BC6H_SF16 and save the encoded texture\n" );
        printf( "\t/bc6hu\t\tEncode to BC6H_UF16 and save the encoded texture\n" );
        printf( "\t/bc7\t\tEncode to BC7 and save the encoded texture\n" );
        printf( "\t/decode\t\tDecode from BC6H or BC7 encoded texture\n\n" );

        printf( "\tOne and only one of the above options must be present, the following options are optional:\n\n" );                

        printf( "\t/cpu\t\tForce to use CPU encoder and decoder\n" );
        printf( "\t/sre\t\tShow reconstruction error\n" );
        printf( "\t/srf\t\tSave reconstruction file\n\n" );        
        
        printf( "\t/sre and /srf are only valid when encoding textures. \n\n\tAfter encoding to BC6H or BC7, \
/sre decodes the encoded texture and then compares this reconstructed texture with original input texture, and output the L2 error and bias. \
You can choose to save this reconstructed texture using /srf\n\n" );
        
        printf( "\tOnce a certain operation is chosen by the options above, the same operation will be performed on all input Filename[i]\n\n" );

        return 1;
    }

    if ( !ParseCommandLine( argc, argv ) )
        return 1;

    // Create the hardware device with the highest possible feature level
    printf( "Creating device..." );
    if ( FAILED( CreateDevice( &g_pDevice, &g_pContext ) ) )
        return 1;
    printf( "done\n" );

    // Check for Compute Shader 4.x support
    if ( !g_CommandLineOptions.bForceCPU )    
    {
        printf( "Checking CS4x capability..." );
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
        g_pDevice->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
        g_bCS4xAvailable = hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;                
    }

    if ( g_bCS4xAvailable )
    {
        // CS4x capability found, initialize our CS accelerated encoders and decoders
        printf( "Yes, using CS Accelerated Encoder/Decoder\n" );
        g_GPUBC6HEncoder.Initialize( g_pDevice, g_pContext );
        g_GPUBC6HDecoder.Initialize( g_pDevice, g_pContext );
        g_GPUBC7Encoder.Initialize( g_pDevice, g_pContext );
        g_GPUBC7Decoder.Initialize( g_pDevice, g_pContext );
    } else
    {
        // No CS4x support, or the user forces using CPU encoder, everything will be on CPU. 
        // In order for D3DX texture utility to work on BC6H and BC7 textures, a FL11 device is required
        if ( g_pDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_11_0 )
        {
            SAFE_RELEASE( g_pDevice );
            SAFE_RELEASE( g_pContext );
            if ( FAILED( CreateDevice( &g_pDevice, &g_pContext, TRUE ) ) )
                return 1;
        }        

        if ( g_CommandLineOptions.bForceCPU )
            printf( "Forcing CPU encoder, might be very very slow\n" );
        else
            printf( "No, using CPU encoder, might be very very slow\n" );
    }   

    // Process the input files
    for ( int i = 1; i < argc; ++i )
    {
        // Skip all command line options
        if ( argv[i][0] == L'/' )
            continue;

        if ( !FileExists( argv[i] ) )
        {
            wprintf( L"\nFile not found: %s\n", argv[i] );
            continue;
        }       
    
        wprintf( L"\nProcessing source texture %s...\n", argv[i] );

        DXGI_FORMAT fmtLoadAs = DXGI_FORMAT_FROM_FILE;
        if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HS ||
             g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HU )
            fmtLoadAs = DXGI_FORMAT_R32G32B32A32_FLOAT;
        else
        if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC7 )
            fmtLoadAs = DXGI_FORMAT_R8G8B8A8_UNORM;

        SAFE_RELEASE( g_pSourceTexture );
        if ( FAILED( LoadTextureFromFile( g_pDevice, argv[i], fmtLoadAs, &g_pSourceTexture ) ) )
        {
            printf( "error reading source texture file\n" );
            continue;
        }

        D3D11_TEXTURE2D_DESC srcTexDesc;
        g_pSourceTexture->GetDesc( &srcTexDesc );

        // Decide what to do according to command line options
        if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC7 )
        {
            // Encode to BC7
            if ( FAILED( EncodeDecodeBC7( argv[i], g_pSourceTexture ) ) )            
                continue;            
        } else
        if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HS || 
             g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HU )
        {
            // Encode to BC6H
            if ( FAILED( EncodeDecodeBC6H( argv[i], g_pSourceTexture ) ) )            
                continue;            
        } else
        if ( g_CommandLineOptions.mode == CCommandLineOptions::MODE_DECODE )
        {
            if ( srcTexDesc.Format == DXGI_FORMAT_BC7_UNORM )
            {
                // Decode to DXGI_FORMAT_R8G8B8A8_UNORM from BC7
                if ( FAILED( DecodeBC7( argv[i], g_pSourceTexture ) ) )
                    continue;
            } else
            if ( srcTexDesc.Format == DXGI_FORMAT_BC6H_SF16 || srcTexDesc.Format == DXGI_FORMAT_BC6H_UF16 )
            {
                // Decode to DXGI_FORMAT_R32G32B32A32_FLOAT from BC6H
                if ( FAILED( DecodeBC6H( argv[i], g_pSourceTexture ) ) )
                    continue;                  
            } else
            {
                printf( "\tNot BC6H nor BC7 input texture\n" );
            }
        } 
    }    


    Cleanup();

    return 0;
}

HRESULT CreateAndCopyToDebugArray( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dContext, ID3D11Texture2D* pTexture, LPVOID* ppData, UINT &uNumByte)
{
    HRESULT hr = S_OK;

    ID3D11Texture2D* pDebugTex = NULL;

    D3D11_TEXTURE2D_DESC texDesc;
    pTexture->GetDesc( &texDesc );

    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.BindFlags = 0;
    texDesc.MiscFlags = 0;
    V_RETURN( pd3dDevice->CreateTexture2D(&texDesc, NULL, &pDebugTex) );
#if defined(DEBUG) || defined(PROFILE)
    if ( pDebugTex )
        pDebugTex->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Debug" ) - 1, "Debug" );
#endif

    pd3dContext->CopyResource( pDebugTex, pTexture );

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    pd3dContext->Map( pDebugTex, 0, D3D11_MAP_READ, 0, &mappedResource );

    *ppData = new BYTE[mappedResource.DepthPitch];
    memcpy( *ppData, mappedResource.pData, mappedResource.DepthPitch );
    uNumByte = mappedResource.DepthPitch;

    pd3dContext->Unmap( pDebugTex, 0 );

    SAFE_RELEASE(pDebugTex);

    return hr;
}

void BC6HAnalyzeAndDisplayResult( ID3D11Device* pd3dDevice,
                                 ID3D11DeviceContext* pd3dContext,
                                 ID3D11Texture2D* pSourceTexture,
                                 ID3D11Texture2D* pRestoredTexture )
{
    D3D11_TEXTURE2D_DESC texSrcDesc;
    pSourceTexture->GetDesc(&texSrcDesc);

    float* pDataSource = NULL;
    UINT nNumByteSource = 0;
    CreateAndCopyToDebugArray( pd3dDevice, pd3dContext, pSourceTexture, (LPVOID*)&pDataSource, nNumByteSource );

    BYTE* pDataRestoredByte = NULL;
    UINT nNumByteRestored = 0;
    CreateAndCopyToDebugArray( pd3dDevice, pd3dContext, pRestoredTexture, (LPVOID*)&pDataRestoredByte, nNumByteRestored );

    D3D11_TEXTURE2D_DESC texResDesc;
    pRestoredTexture->GetDesc(&texResDesc);

    float* pDataRestored;
    if ( DXGI_FORMAT_R16G16B16A16_FLOAT == texResDesc.Format )
    {
        pDataRestored = new float[nNumByteRestored / sizeof(D3DXFLOAT16)];
        D3DXFloat16To32Array( pDataRestored, (D3DXFLOAT16*)pDataRestoredByte, nNumByteRestored / sizeof(D3DXFLOAT16) );
    }
    else
    {
        pDataRestored = (float*)pDataRestoredByte;
    }

    double bias = 0;
    double error = 0;
    for ( UINT i = 0; i < texSrcDesc.Height; i ++ )
    {
        for ( UINT j = 0; j < texSrcDesc.Width; j ++ )
        {
            for ( UINT k = 0; k < 3; k ++ )
            {
                double tmp = pDataRestored[(i * texSrcDesc.Width + j) * 4 + k] - pDataSource[(i * texSrcDesc.Width + j) * 4 + k];
                double sqr_tmp = tmp * tmp;
                bias += tmp;
                error += sqr_tmp;
            }
        }
    }
    bias /= texSrcDesc.Width * texSrcDesc.Height * 3;
    error = sqrt( error / (texSrcDesc.Width * texSrcDesc.Height * 3) );

    printf( "\n" );
    printf( "Image Encode-Decode L2 Error ( sqrt(ave((r-t)^2)) ): %.10f\n", error );
    printf( "Image Encode-Decode Bias ( ave(r-t) ): %.10f\n", bias );
    printf( "\n" );

    delete[] pDataSource;
    if ( DXGI_FORMAT_R16G16B16A16_FLOAT == texResDesc.Format )
    {
        delete[] pDataRestored;
    }
    delete[] pDataRestoredByte;
}

void BC7AnalyzeAndDisplayResult( ID3D11Device* pd3dDevice,
                                ID3D11DeviceContext* pd3dContext,
                                ID3D11Texture2D* pSourceTexture,
                                ID3D11Texture2D* pRestoredTexture )
{
    D3D11_TEXTURE2D_DESC texSrcDesc;
    pSourceTexture->GetDesc(&texSrcDesc);

    BYTE* pDataSource = NULL;
    UINT nNumByteSource = 0;
    CreateAndCopyToDebugArray( pd3dDevice, pd3dContext, pSourceTexture, (LPVOID*)&pDataSource, nNumByteSource );

    BYTE* pDataRestored = NULL;
    UINT nNumByteRestored = 0;
    CreateAndCopyToDebugArray( pd3dDevice, pd3dContext, pRestoredTexture, (LPVOID*)&pDataRestored, nNumByteRestored );

    double bias = 0;
    double error = 0;
    for ( UINT i = 0; i < texSrcDesc.Height; i ++ )
    {
        for ( UINT j = 0; j < texSrcDesc.Width; j ++ )
        {
            for ( UINT k = 0; k < 4; k ++ )
            {
                double tmp = pDataRestored[(i * texSrcDesc.Width + j) * 4 + k] - pDataSource[(i * texSrcDesc.Width + j) * 4 + k];
                double sqr_tmp = tmp * tmp;
                bias += tmp;
                error += sqr_tmp;
            }
        }
    }
    bias /= texSrcDesc.Width * texSrcDesc.Height * 4;
    error = sqrt( error / (texSrcDesc.Width * texSrcDesc.Height * 4) );

    printf( "\n" );
    printf( "Image Encode-Decode L2 Error ( sqrt(ave((r-t)^2)) ): %.10f\n", error );
    printf( "Image Encode-Decode Bias ( ave(r-t) ): %.10f\n", bias );
    printf( "\n" );

    delete[] pDataSource;
    delete[] pDataRestored;
}

HRESULT EncodeDecodeBC7( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture )
{
    HRESULT hr = S_OK;
    
    D3D11_TEXTURE2D_DESC srcTexDesc;
    pSourceTexture->GetDesc( &srcTexDesc );

    std::wstring fname = strSrcFilename;
    
    INT pos = (INT)fname.rfind( '.' );
    fname.insert( pos, L"_BC7" );
    pos = (INT)fname.rfind( '.' );
    fname.erase( pos + 1, fname.length() );
    fname += std::wstring( L"dds" );

    if ( g_bCS4xAvailable )
    {
        // GPU path

        V_RETURN( g_GPUBC7Encoder.GPU_BC7EncodeAndSave( pSourceTexture, DXGI_FORMAT_BC7_UNORM, &fname[0] ) ); 
        
        if ( g_CommandLineOptions.bSaveReconstructionFile || g_CommandLineOptions.bShowReconstructionError )
        {
            fname.insert( fname.rfind( '.' ), L"_Restored" );

            // Either the user request to save reconstruction file or show reconstruction error, we need to decode
            V_RETURN( g_GPUBC7Decoder.GPU_BC7DecodeAndSaveInternal( g_GPUBC7Encoder.GetEncodedTextureAsBuf(), srcTexDesc.Width, srcTexDesc.Height, DXGI_FORMAT_BC7_UNORM, 
                                                                    g_CommandLineOptions.bSaveReconstructionFile ? &fname[0] : NULL ) );

            if ( g_CommandLineOptions.bShowReconstructionError )
            {
                BC7AnalyzeAndDisplayResult( g_pDevice, g_pContext, pSourceTexture, g_GPUBC7Decoder.GetRestoredTexture() );
            }
        }
    } else
    {
        // CPU path

        ID3D11Texture2D* pTexEncoded = NULL;
        ID3D11Texture2D* pTexRestored = NULL;
        printf( "\tEncoding to BC7..." );
        V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pSourceTexture, DXGI_FORMAT_BC7_UNORM, &pTexEncoded ) );
        printf( "done\n" );

        wprintf( L"\tSaving to %s...", &fname[0] );
        D3DX11SaveTextureToFile( g_pContext, pTexEncoded, D3DX11_IFF_DDS, &fname[0] );
        printf( "done\n" );

        if ( g_CommandLineOptions.bSaveReconstructionFile || g_CommandLineOptions.bShowReconstructionError )
        {
            fname.insert( fname.rfind( '.' ), L"_Restored" );

            printf( "\tDecoding from BC7..." );
            V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pTexEncoded, DXGI_FORMAT_R8G8B8A8_UNORM, &pTexRestored ) );
            printf( "done\n" );

            if ( g_CommandLineOptions.bSaveReconstructionFile )
            {
                wprintf( L"\tSaving to %s...", &fname[0] );
                D3DX11SaveTextureToFile( g_pContext, pTexRestored, D3DX11_IFF_DDS, &fname[0] );
                printf( "done\n" );
            }

            if ( g_CommandLineOptions.bShowReconstructionError )
            {
                BC7AnalyzeAndDisplayResult( g_pDevice, g_pContext, pSourceTexture, pTexRestored );
            }
        }

        SAFE_RELEASE( pTexEncoded );
        SAFE_RELEASE( pTexRestored );
    }    

    return hr;
}

HRESULT EncodeDecodeBC6H( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture )
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC srcTexDesc;
    pSourceTexture->GetDesc( &srcTexDesc );

    std::wstring fname = strSrcFilename;

    INT pos = (INT)fname.rfind( '.' );
    fname.insert( pos, L"_BC6H" );
    pos = (INT)fname.rfind( '.' );
    fname.erase( pos + 1, fname.length() );
    fname += std::wstring( L"dds" );

    if ( g_bCS4xAvailable )
    {
        // GPU path

        V_RETURN( g_GPUBC6HEncoder.GPU_BC6HEncodeAndSave( pSourceTexture, 
            g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HS ? DXGI_FORMAT_BC6H_SF16 : DXGI_FORMAT_BC6H_UF16, &fname[0] ) );
        
        if ( g_CommandLineOptions.bSaveReconstructionFile || g_CommandLineOptions.bShowReconstructionError )
        {
            fname.insert( fname.rfind( '.' ), L"_Restored" );

            // Either the user request to save reconstruction file or show reconstruction error, we need to decode
            V_RETURN( g_GPUBC6HDecoder.GPU_BC6HDecodeAndSaveInternal( g_GPUBC6HEncoder.GetEncodedTextureAsBuf(), srcTexDesc.Width, srcTexDesc.Height, 
                g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HS ? DXGI_FORMAT_BC6H_SF16 : DXGI_FORMAT_BC6H_UF16, 
                g_CommandLineOptions.bSaveReconstructionFile ? &fname[0] : NULL ) ); 

            if ( g_CommandLineOptions.bShowReconstructionError )
            {
                BC6HAnalyzeAndDisplayResult( g_pDevice, g_pContext, pSourceTexture, g_GPUBC6HDecoder.GetRestoredTexture() );
            }
        }       
    } else
    {
        // CPU path

        ID3D11Texture2D* pTexEncoded = NULL;
        ID3D11Texture2D* pTexRestored = NULL;
        printf( "\tEncoding to BC6H..." );
        V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pSourceTexture, 
            g_CommandLineOptions.mode == CCommandLineOptions::MODE_ENCODE_BC6HS ? DXGI_FORMAT_BC6H_SF16 : DXGI_FORMAT_BC6H_UF16, &pTexEncoded ) );
        printf( "done\n" );

        wprintf( L"\tSaving to %s...", &fname[0] );
        D3DX11SaveTextureToFile( g_pContext, pTexEncoded, D3DX11_IFF_DDS, &fname[0] );
        printf( "done\n" );

        if ( g_CommandLineOptions.bSaveReconstructionFile || g_CommandLineOptions.bShowReconstructionError )
        {
            fname.insert( fname.rfind( '.' ), L"_Restored" );

            printf( "\tDecoding from BC6H..." );
            V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pTexEncoded, DXGI_FORMAT_R32G32B32A32_FLOAT, &pTexRestored ) );
            printf( "done\n" );
            
            if ( g_CommandLineOptions.bSaveReconstructionFile )
            {
                wprintf( L"\tSaving to %s...", &fname[0] );
                D3DX11SaveTextureToFile( g_pContext, pTexRestored, D3DX11_IFF_DDS, &fname[0] );
                printf( "done\n" );
            }

            if ( g_CommandLineOptions.bShowReconstructionError )
            {
                BC6HAnalyzeAndDisplayResult( g_pDevice, g_pContext, pSourceTexture, pTexRestored );
            }
        }

        SAFE_RELEASE( pTexEncoded );
        SAFE_RELEASE( pTexRestored );
    }

    return hr;
}

HRESULT DecodeBC7( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture )
{
    HRESULT hr = S_OK;        

    std::wstring fname = strSrcFilename;
    fname.insert( fname.rfind( '.' ), L"_Restored" );

    if ( g_bCS4xAvailable )
    {
        V_RETURN( g_GPUBC7Decoder.GPU_BC7DecodeAndSave( pSourceTexture, &fname[0] ) );
    } else
    {
        ID3D11Texture2D* pTexRestored = NULL;
        printf( "\tDecoding from BC7..." );
        V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pSourceTexture, DXGI_FORMAT_R8G8B8A8_UNORM, &pTexRestored ) );
        printf( "done\n" );

        wprintf( L"\tSaving to %s...", &fname[0] );
        D3DX11SaveTextureToFile( g_pContext, pTexRestored, D3DX11_IFF_DDS, &fname[0] );
        printf( "done\n" );
        SAFE_RELEASE( pTexRestored );
    }

    return hr;
}

HRESULT DecodeBC6H( WCHAR* strSrcFilename, ID3D11Texture2D* pSourceTexture )
{
    HRESULT hr = S_OK;

    std::wstring fname = strSrcFilename;
    fname.insert( fname.rfind( '.' ), L"_Restored" );

    if ( g_bCS4xAvailable )
    {
        V_RETURN( g_GPUBC6HDecoder.GPU_BC6HDecodeAndSave( pSourceTexture, &fname[0] ) );            
    } else
    {
        ID3D11Texture2D* pTexRestored = NULL;
        printf( "\tDecoding from BC6H..." );
        V_RETURN( CPU_EncodeDecode( g_pDevice, g_pContext, pSourceTexture, DXGI_FORMAT_R32G32B32A32_FLOAT, &pTexRestored ) );
        printf( "done\n" );

        wprintf( L"\tSaving to %s...", &fname[0] );
        D3DX11SaveTextureToFile( g_pContext, pTexRestored, D3DX11_IFF_DDS, &fname[0] );
        printf( "done\n" );
        SAFE_RELEASE( pTexRestored );
    }

    return hr;
}

HRESULT CPU_EncodeDecode( ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                          ID3D11Texture2D* pSrcTexture,
                          DXGI_FORMAT dstFormat, ID3D11Texture2D** ppDstTextureOut )
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC texDesc;
    pSrcTexture->GetDesc( &texDesc );

    texDesc.Format = dstFormat;
    SAFE_RELEASE( *ppDstTextureOut );
    V_RETURN( pDevice->CreateTexture2D( &texDesc, NULL, ppDstTextureOut ) );
#if defined(DEBUG) || defined(PROFILE)
    if ( *ppDstTextureOut )
        (*ppDstTextureOut)->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Result" ) - 1, "Result" );
#endif

    D3DX11_TEXTURE_LOAD_INFO tex_load_info;
    tex_load_info.pSrcBox = NULL;
    tex_load_info.pDstBox = NULL;
    tex_load_info.SrcFirstMip = 0;
    tex_load_info.DstFirstMip = 0;
    tex_load_info.NumMips = texDesc.MipLevels;
    tex_load_info.SrcFirstElement = 0;
    tex_load_info.DstFirstElement = 0;
    tex_load_info.NumElements = D3DX11_DEFAULT;
    tex_load_info.Filter = D3DX11_DEFAULT;
    tex_load_info.MipFilter = D3DX11_DEFAULT;

    hr = D3DX11LoadTextureFromTexture( pContext, pSrcTexture, &tex_load_info, *ppDstTextureOut );
    if ( FAILED( hr ) )
        SAFE_RELEASE( *ppDstTextureOut );

    return hr;
}

